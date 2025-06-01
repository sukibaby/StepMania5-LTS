#include "global.h"
#include "ProductInfo.h"
#include "RageSoundDriver_PulseAudio.h"
#include "RageLog.h"
#include "RageSound.h"
#include "RageSoundManager.h"
#include "RageUtil.h"
#include "RageTimer.h"
#include "PrefsManager.h"
#include "ver.h"
#include <pulse/error.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <cstddef>
#include <cstdint>
#include <vector>


/* Register the RageSoundDriver_Pulseaudio class as sound driver "Pulse" */
REGISTER_SOUND_DRIVER_CLASS2( Pulse, PulseAudio );

/* Constructor */
RageSoundDriver_PulseAudio::RageSoundDriver_PulseAudio()
: RageSoundDriver(),
m_LastPosition(0), m_Error(nullptr),
m_Sem("Pulseaudio Synchronization Semaphore"),
m_PulseMainLoop(nullptr), m_PulseCtx(nullptr), m_PulseStream(nullptr)
{
	m_ss.rate = PREFSMAN->m_iSoundPreferredSampleRate;
	if( m_ss.rate == 0 )
		m_ss.rate = 44100;
}

RageSoundDriver_PulseAudio::~RageSoundDriver_PulseAudio()
{
	pa_context_disconnect(m_PulseCtx);
	pa_context_unref(m_PulseCtx);
	pa_threaded_mainloop_stop(m_PulseMainLoop);
	pa_threaded_mainloop_free(m_PulseMainLoop);

	if(m_Error != nullptr)
	{
		free(m_Error);
	}
}

/* Initialization */
RString RageSoundDriver_PulseAudio::Init()
{
	int error = 0;

	LOG->Trace("Pulse: pa_threaded_mainloop_new()...");
	m_PulseMainLoop = pa_threaded_mainloop_new();
	if(m_PulseMainLoop == nullptr)
	{
		return "pa_threaded_mainloop_new() failed!";
	}

#ifdef PA_PROP_APPLICATION_NAME /* proplist available only since 0.9.11 */
	pa_proplist *plist = pa_proplist_new();
	pa_proplist_sets(plist, PA_PROP_APPLICATION_NAME, PRODUCT_FAMILY);
	pa_proplist_sets(plist, PA_PROP_APPLICATION_VERSION, product_version);
	pa_proplist_sets(plist, PA_PROP_MEDIA_ROLE, "game");

	LOG->Trace("Pulse: pa_context_new_with_proplist()...");

	m_PulseCtx = pa_context_new_with_proplist(
			pa_threaded_mainloop_get_api(m_PulseMainLoop),
			PRODUCT_FAMILY, plist);
	pa_proplist_free(plist);

	if(m_PulseCtx == nullptr)
	{
		return "pa_context_new_with_proplist() failed!";
	}
#else
	LOG->Trace("Pulse: pa_context_new()...");
	m_PulseCtx = pa_context_new(
			pa_threaded_mainloop_get_api(m_PulseMainLoop),
			PRODUCT_FAMILY);
	if(m_PulseCtx == nullptr)
	{
		return "pa_context_new() failed!";
	}
#endif

	pa_context_set_state_callback(m_PulseCtx, StaticCtxStateCb, this);

	LOG->Trace("Pulse: pa_context_connect()...");
	error = pa_context_connect(m_PulseCtx, nullptr, (pa_context_flags_t)0, nullptr);

	if(error < 0)
	{
		return ssprintf("pa_contect_connect(): %s",
			pa_strerror(pa_context_errno(m_PulseCtx)));
	}

	LOG->Trace("Pulse: pa_threaded_mainloop_start()...");
	error = pa_threaded_mainloop_start(m_PulseMainLoop);
	if(error < 0)
	{
		return ssprintf("pa_threaded_mainloop_start() returned %i", error);
	}

	/* Create the decode thread, this will be needed for Mix(), that we
 	 * will use as soon as a stream is ready. */
	StartDecodeThread();

	/* Wait for the pulseaudio stream to be ready before returning.
	* An error may occur, if it appends, m_Error becomes non-nullptr. */
	m_Sem.Wait();

	if(m_Error == nullptr)
	{
		return "";
	}
	else
	{
		return m_Error;
	}
}

void RageSoundDriver_PulseAudio::m_InitStream(void)
{
	int error;
	pa_sample_spec ss_local; // Use a local pa_sample_spec for setup
	pa_channel_map map;

	/* init sample spec */
	ss_local.format = PA_SAMPLE_S16LE;
	ss_local.channels = 2;
	ss_local.rate = m_ss.rate; // Use the rate initialized in the constructor's m_ss

	/* init channel map */
	pa_channel_map_init_stereo(&map);

	/* check sample spec */
	if(!pa_sample_spec_valid(&ss_local))
	{
		if(asprintf(&m_Error, "invalid sample spec!") == -1)
		{
			m_Error = nullptr; // asprintf failed to allocate memory
		}
		m_Sem.Post();
		return;
	}

	/* log the used sample spec */
	char specstring[PA_SAMPLE_SPEC_SNPRINT_MAX];
	pa_sample_spec_snprint(specstring, sizeof(specstring), &ss_local);
	LOG->Trace("Pulse: using sample spec: %s", specstring);

	/* create the stream */
	LOG->Trace("Pulse: pa_stream_new()...");
	m_PulseStream = pa_stream_new(m_PulseCtx, PRODUCT_FAMILY " Audio", &ss_local, &map);
	if(m_PulseStream == nullptr)
	{
		if(asprintf(&m_Error, "pa_stream_new(): %s", pa_strerror(pa_context_errno(m_PulseCtx))) == -1)
		{
			m_Error = nullptr; // asprintf failed to allocate memory
		}
		m_Sem.Post();
		return;
	}

	/* set the write callback, it will be called when the sound server
	* needs data */
	pa_stream_set_write_callback(m_PulseStream, StaticStreamWriteCb, this);

	/* set the state callback, it will be called when the stream state will
	* change */
	pa_stream_set_state_callback(m_PulseStream, StaticStreamStateCb, this);

	/* configure attributes of the stream */
	pa_buffer_attr attr;
	memset(&attr, 0x00, sizeof(attr)); // Initialize all members to 0/nullptr

	/* tlength: Target length of the buffer.
	*
	* "The server tries to assure that at least tlength bytes are always
	*  available in the per-stream server-side playback buffer. It is
	*  recommended to set this to (uint32_t) -1, which will initialize
	*  this to a value that is deemed sensible by the server. However,
	*  this value will default to something like 2s, i.e. for applications
	*  that have specific latency requirements this value should be set to
	*  the maximum latency that the application can deal with."
	*
	* We don't want the default here, we want a small latency.
	* We use pa_usec_to_bytes() to convert a latency to a buffer size.
	*/
	attr.tlength = pa_usec_to_bytes(20*PA_USEC_PER_MSEC, &ss_local); // Use local ss_local

	/* maxlength: Maximum length of the buffer
	*
	* "Setting this to (uint32_t) -1 will initialize this to the maximum
	*  value supported by server, which is recommended."
	*
	* (uint32_t)-1 is NOT working here, setting it to tlength*2, like
	* openal-soft-pulseaudio does.
	*/
	attr.maxlength = attr.tlength*2;

	/* minreq: Minimum request
	*
	* "The server does not request less than minreq bytes from the client,
	*  instead waits until the buffer is free enough to request more bytes
	*  at once. It is recommended to set this to (uint32_t) -1, which will
	*  initialize this to a value that is deemed sensible by the server."
	*
	* (uint32_t)-1 is NOT working here, setting it to 0, like
	* openal-soft-pulseaudio does.
	*/
	attr.minreq = 0; // Setting to (uint32_t)-1 caused issues in some environments. 0 is safer.

	/* prebuf: Pre-buffering
	*
	* "The server does not start with playback before at least prebuf
	*  bytes are available in the buffer. It is recommended to set this
	*  to (uint32_t) -1, which will initialize this to the same value as
	*  tlength"
	*/
	attr.prebuf = (uint32_t)-1;

	/* fragsize: Deprecated. Use tlength, prebuf, minreq, maxlength instead.
	 * Ensure it's not used by setting to (uint32_t)-1, as per PA documentation.
	 */
	attr.fragsize = (uint32_t)-1;


	/* log the used target buffer length */
	LOG->Trace("Pulse: using target buffer length of %i bytes", attr.tlength);

	 /* connect the stream for playback */
	LOG->Trace("Pulse: pa_stream_connect_playback()...");
	const int flags = PA_STREAM_INTERPOLATE_TIMING
		| PA_STREAM_NOT_MONOTONIC // mHostTime may not be monotonic in some cases
		| PA_STREAM_AUTO_TIMING_UPDATE
		| PA_STREAM_ADJUST_LATENCY; // Allow server to adjust latency based on our buffer_attr
	error = pa_stream_connect_playback(m_PulseStream, nullptr, &attr,
			static_cast<pa_stream_flags_t>(flags), nullptr, nullptr);
	if(error < 0)
	{
		if(asprintf(&m_Error, "pa_stream_connect_playback(): %s",
				pa_strerror(pa_context_errno(m_PulseCtx))) == -1)
		{
			m_Error = nullptr; // asprintf failed to allocate memory
		}
		m_Sem.Post();
		return;
	}

	// m_ss is the member variable. After successfully connecting the stream,
	// we can be more confident about the sample spec being used.
	// It's generally good practice to update m_ss with the spec that was
	// actually used to create the stream, in case the server negotiated
	// something slightly different (though with PA_SAMPLE_S16LE, 2ch, and specific rates,
	// it's less likely to change).
	m_ss = ss_local;

	// Semaphore is posted in StreamStateCb when stream becomes PA_STREAM_READY
}

void RageSoundDriver_PulseAudio::CtxStateCb(pa_context *c)
{
	switch (pa_context_get_state(m_PulseCtx))
	{
	case PA_CONTEXT_CONNECTING:
		LOG->Trace("Pulse: Context connecting...");
		break;
	case PA_CONTEXT_AUTHORIZING:
		LOG->Trace("Pulse: Context authorizing...");
		break;
	case PA_CONTEXT_SETTING_NAME:
		LOG->Trace("Pulse: Context setting name...");
		break;
	case PA_CONTEXT_READY:
		LOG->Trace("Pulse: Context ready now.");
		m_InitStream();
		break;
	case PA_CONTEXT_TERMINATED:
	case PA_CONTEXT_FAILED:
		if(asprintf(&m_Error, "context connection failed: %s", pa_strerror(pa_context_errno(m_PulseCtx))) == -1)
		{
			m_Error = nullptr;
		}
		m_Sem.Post();
		return;
		break;
	default:
		break;
	}
}

void RageSoundDriver_PulseAudio::StreamStateCb(pa_stream *s)
{
	switch(pa_stream_get_state(m_PulseStream))
	{
	case PA_STREAM_CREATING:
		LOG->Trace("Pulse: Stream creating...");
		break;
	case PA_STREAM_READY:
		LOG->Trace("Pulse: Stream ready now/");
		m_Sem.Post();
		return;
		break;
	case PA_STREAM_UNCONNECTED:
	case PA_STREAM_TERMINATED:
	case PA_STREAM_FAILED:
		if(asprintf(&m_Error, "stream connection failed: %s",
		    pa_strerror(pa_context_errno(m_PulseCtx))) == -1)
		{
		}
		m_Sem.Post();
		return;
		break;
	}
}

int64_t RageSoundDriver_PulseAudio::GetPosition() const
{
	pa_threaded_mainloop_lock(m_PulseMainLoop);
	int64_t position = GetPositionUnlocked();
	pa_threaded_mainloop_unlock(m_PulseMainLoop);
	return position;
}

int64_t RageSoundDriver_PulseAudio::GetPositionUnlocked() const
{
	pa_usec_t usec;
	if(pa_stream_get_time(m_PulseStream, &usec) < 0)
	{
		int paErrno = pa_context_errno(m_PulseCtx);

		// We might get no data error if the stream has just been started and hasn't received any timing data yet
		if(paErrno == PA_ERR_NODATA)
			return 0;
		else
			RageException::Throw("Pulse: pa_stream_get_time() failed: %s", pa_strerror(paErrno));
	}

	size_t length = pa_usec_to_bytes(usec, &m_ss);
	return length / (sizeof(int16_t) * 2); /* we use 16-bit frames and 2 channels */
}

void RageSoundDriver_PulseAudio::StreamWriteCb(pa_stream *s, size_t length)
{
	int64_t curPos = GetPositionUnlocked();
	while(length > 0)
	{
		void* buf;
		size_t bufsize = length;
		if(pa_stream_begin_write(m_PulseStream, &buf, &bufsize) < 0)
		{
			RageException::Throw("Pulse: pa_stream_begin_write() failed: %s", pa_strerror(pa_context_errno(m_PulseCtx)));
		}

		const size_t nbframes = bufsize / sizeof(int16_t); /* we use 16-bit frames */
		int64_t pos1 = m_LastPosition;
		int64_t pos2 = pos1 + nbframes/2; /* Mix() position in stereo frames */
		this->Mix( reinterpret_cast<int16_t*>(buf), pos2-pos1, pos1, curPos);

		if(pa_stream_write(m_PulseStream, buf, bufsize, nullptr, 0, PA_SEEK_RELATIVE) < 0)
		{
			RageException::Throw("Pulse: pa_stream_write() failed: %s", pa_strerror(pa_context_errno(m_PulseCtx)));
		}

		m_LastPosition = pos2;
		length -= bufsize;
	}
}

/* Static wrappers, because pulseaudio is a C API, it uses callbacks.
 * So we have to write wrappers that will call our objects callbacks. */
void RageSoundDriver_PulseAudio::StaticCtxStateCb(pa_context *c, void *user)
{
	RageSoundDriver_PulseAudio *obj = (RageSoundDriver_PulseAudio*)user;
	obj->CtxStateCb(c);
}
void RageSoundDriver_PulseAudio::StaticStreamStateCb(pa_stream *s, void *user)
{
	RageSoundDriver_PulseAudio *obj = (RageSoundDriver_PulseAudio*)user;
	obj->StreamStateCb(s);
}
void RageSoundDriver_PulseAudio::StaticStreamWriteCb(pa_stream *s, size_t length, void *user)
{
	 RageSoundDriver_PulseAudio *obj = (RageSoundDriver_PulseAudio*)user;
	 obj->StreamWriteCb(s, length);
}

/*
 * (c) 2009 Damien Thebault
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

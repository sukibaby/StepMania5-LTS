#include "global.h"
#include "RageSoundDriver.h"
#include "PrefsManager.h"
#include "RageLog.h"
#include "RageSound.h"
#include "RageUtil.h"
#include "RageSoundMixBuffer.h"
#include "RageSoundReader.h"

#include <cmath>
#include <cstdint>
#include <cinttypes>
#if defined(_MSC_VER) && !defined(PRIu64)
	#define PRIu64 "I64u"
#endif
/* Note(sukibaby): If you need to activate a log method
 * below, and it's casting a variable to int instead of
 * using the correct format specifier, please modify it
 * to use PRId64 for int64_t, or PRIu64 for uint64_t */

static const int channels = 2;

static int frames_to_buffer;

/* 512 is about 10ms, which is big enough for the tolerance of most schedulers. */
static int chunksize() { return 512; }

static int underruns = 0, logged_underruns = 0;

RageSoundDriver::Sound::Sound()
{
	m_pSound = nullptr;
	m_State = AVAILABLE;
	m_bPaused = false;
}

void RageSoundDriver::Sound::Allocate( int iFrames )
{
	/* Reserve enough blocks in the buffer to hold the buffer.  Add one, to account for
	 * the fact that we may have a partial block due to a previous Mix() call. */
	const int iFramesPerBlock = samples_per_block / channels;
	const int iBlocksToPrebuffer = iFrames / iFramesPerBlock;
	m_Buffer.reserve( iBlocksToPrebuffer + 1 );
	m_PosMapQueue.reserve( 32 );
}

void RageSoundDriver::Sound::Deallocate()
{
	m_Buffer.reserve( 0 );
	m_PosMapQueue.reserve( 0 );
}

int RageSoundDriver::DecodeThread_start( void *p )
{
	((RageSoundDriver *) p)->DecodeThread();
	return 0;
}

RageSoundMixBuffer &RageSoundDriver::MixIntoBuffer( int iFrames, int64_t iFrameNumber, int64_t iCurrentFrame )
{
	ASSERT_M( m_DecodeThread.IsCreated(), "RageSoundDriver::StartDecodeThread() was never called" );

	static RageSoundMixBuffer mix;

	for( unsigned i = 0; i < ARRAYLEN(m_Sounds); ++i )
	{
		/* s.m_pSound can not safely be accessed from here. */
		Sound &s = m_Sounds[i];
		if( s.m_State == Sound::HALTING )
		{
			/* This indicates that this stream can be reused. */
			s.m_bPaused = false;
			s.m_State = Sound::STOPPED;

//			LOG->Trace("set %p from HALTING to STOPPED", m_Sounds[i].m_pSound);
			continue;
		}

		if( s.m_State != Sound::STOPPING && s.m_State != Sound::PLAYING )
			continue;

		/* STOPPING or PLAYING.  Read sound data. */
		if( m_Sounds[i].m_bPaused )
			continue;

		int iGotFrames = 0;
		int iFramesLeft = iFrames;

		/* Does the sound have a start time? */
		if( !s.m_StartTime.IsZero() && iCurrentFrame != -1 )
		{
			/* If the sound is supposed to start at a time past this buffer, insert silence. */
			const int64_t iFramesUntilThisBuffer = iFrameNumber - iCurrentFrame;
			const float fSecondsBeforeStart = -s.m_StartTime.Ago();
			const int64_t iFramesBeforeStart = int64_t(fSecondsBeforeStart * GetSampleRate());
			const int iSilentFramesInThisBuffer = std::clamp( int(iFramesBeforeStart-iFramesUntilThisBuffer), 0, iFramesLeft );

			iGotFrames += iSilentFramesInThisBuffer;
			iFramesLeft -= iSilentFramesInThisBuffer;

			/* If we didn't completely fill the buffer, then we've written all of the silence. */
			if( iFramesLeft )
				s.m_StartTime.SetZero();
		}

		/* Fill actual data. */
		sound_block *p[2];
		unsigned pSize[2];
		s.m_Buffer.get_read_pointers( p, pSize );

		while( iFramesLeft && pSize[0] )
		{
			if( !p[0]->m_FramesInBuffer )
			{
				/* We've processed all of the sound in this block.  Mark it read. */
				s.m_Buffer.advance_read_pointer( 1 );
				++p[0];
				--pSize[0];

				/* If we have more data in p[0], keep going. */
				if( pSize[0] )
					continue; // more data

				/* We've used up p[0].  Try p[1]. */
				std::swap( p[0], p[1] );
				std::swap( pSize[0], pSize[1] );
				continue;
			}

			/* Note that, until we call advance_read_pointer, we can safely write to p[0]. */
			const int frames_to_read = std::min( iFramesLeft, p[0]->m_FramesInBuffer );
			mix.SetWriteOffset( iGotFrames*channels );
			mix.write( p[0]->m_BufferNext, frames_to_read * channels );

			{
				Sound::QueuedPosMap pos;
				pos.iStreamFrame = iFrameNumber+iGotFrames;
				pos.iHardwareFrame = p[0]->m_iPosition;
				pos.iFrames = frames_to_read;

				s.m_PosMapQueue.write( &pos, 1 );
			}

			p[0]->m_BufferNext += frames_to_read*channels;
			p[0]->m_FramesInBuffer -= frames_to_read;
			p[0]->m_iPosition += frames_to_read;

//			LOG->Trace( "incr fr rd += %i (state %i) (%p)",
//				(int) frames_to_read, s.m_State, s.m_pSound );

			iGotFrames += frames_to_read;
			iFramesLeft -= frames_to_read;
		}

		/* If we don't have enough to fill the buffer, we've underrun. */
		if( iGotFrames < iFrames && s.m_State == Sound::PLAYING )
			++underruns;
	}

	return mix;
}

void RageSoundDriver::Mix( int16_t *pBuf, int iFrames, int64_t iFrameNumber, int64_t iCurrentFrame )
{
	memset( pBuf, 0, iFrames*channels*sizeof(int16_t) );
	MixIntoBuffer( iFrames, iFrameNumber, iCurrentFrame ).read( pBuf );
}

void RageSoundDriver::Mix( float *pBuf, int iFrames, int64_t iFrameNumber, int64_t iCurrentFrame )
{
	memset( pBuf, 0, iFrames*channels*sizeof(float) );
	MixIntoBuffer( iFrames, iFrameNumber, iCurrentFrame ).read( pBuf );
}

void RageSoundDriver::MixDeinterlaced( float **pBufs, int iChannels, int iFrames, int64_t iFrameNumber, int64_t iCurrentFrame )
{
	for (int i = 0; i < iChannels; ++i )
		memset( pBufs[i], 0, iFrames*sizeof(float) );
	MixIntoBuffer( iFrames, iFrameNumber, iCurrentFrame ).read_deinterlace( pBufs, iChannels );
}

void RageSoundDriver::DecodeThread()
{
	SetupDecodingThread();

	while( !m_bShutdownDecodeThread )
	{
		/* Fill each playing sound, round-robin. */
		{
			int iSampleRate = GetSampleRate();
			ASSERT_M( iSampleRate > 0, ssprintf("%i", iSampleRate) );
			int iUsecs = 1000000*chunksize() / iSampleRate;
			usleep( iUsecs );
		}

		LockMut( m_Mutex );
//		LOG->Trace("begin mix");

		for( unsigned i = 0; i < ARRAYLEN(m_Sounds); ++i )
		{
			if( m_Sounds[i].m_State != Sound::PLAYING )
				continue;

			Sound *pSound = &m_Sounds[i];

			CHECKPOINT_M("Processing the sound while buffers are available.");
			while( pSound->m_Buffer.num_writable() )
			{
				int iWrote = GetDataForSound( *pSound );
				if( iWrote == RageSoundReader::WOULD_BLOCK )
					break;
				if( iWrote < 0 )
				{
					/* This sound is finishing. */
					pSound->m_State = Sound::STOPPING;
					break;
//					LOG->Trace("mixer: (#%i) eof (%p)", i, pSound->m_pSound );
				}
			}
		}
//		LOG->Trace("end mix");
	}
}

/* Buffer a block of sound data for the given sound.  Return the number of
 * frames buffered, or a RageSoundReader return code. */
int RageSoundDriver::GetDataForSound( Sound &s )
{
	sound_block *p[2];
	unsigned psize[2];
	s.m_Buffer.get_write_pointers( p, psize );

	/* If we have no open buffer slot, we have a buffer overflow. */
	ASSERT( psize[0] > 0 );

	sound_block *pBlock = p[0];
	int size = ARRAYLEN(pBlock->m_Buffer)/channels;
	int iRet = s.m_pSound->GetDataToPlay( pBlock->m_Buffer, size, pBlock->m_iPosition, pBlock->m_FramesInBuffer );
	if( iRet > 0 )
	{
		pBlock->m_BufferNext = pBlock->m_Buffer;
		s.m_Buffer.advance_write_pointer( 1 );
	}

//	LOG->Trace( "incr fr wr %i (state %i) (%p)",
//		(int) pBlock->m_FramesInBuffer, s.m_State, s.m_pSound );

	return iRet;
}


void RageSoundDriver::Update()
{
	m_Mutex.Lock();
	for( unsigned i = 0; i < ARRAYLEN(m_Sounds); ++i )
	{
		{
			Sound::QueuedPosMap p;
			while( m_Sounds[i].m_PosMapQueue.read( &p, 1 ) )
			{
				RageSoundBase *pSound = m_Sounds[i].m_pSound;
				if( pSound != nullptr )
					pSound->CommitPlayingPosition( p.iStreamFrame, p.iHardwareFrame, p.iFrames );
			}
		}

		switch( m_Sounds[i].m_State )
		{
		case Sound::STOPPED:
			m_Sounds[i].Deallocate();
			m_Sounds[i].m_State = Sound::AVAILABLE;
			continue;
		case Sound::STOPPING:
			break;
		default:
			continue;
		}

		if( m_Sounds[i].m_Buffer.num_readable() != 0 )
			continue;

//		LOG->Trace("finishing sound %i", i);

		m_Sounds[i].m_pSound->SoundIsFinishedPlaying();
		m_Sounds[i].m_pSound = nullptr;

		/* This sound is done.  Set it to HALTING, since the mixer thread might
		 * be accessing it; it'll change it back to STOPPED once it's ready to
		 * be used again. */
		m_Sounds[i].m_State = Sound::HALTING;
//		LOG->Trace("set (#%i) %p from STOPPING to HALTING", i, m_Sounds[i].m_pSound);
	}

	constexpr uint64_t iUsecs = 1000000;
	static uint64_t fNextUsecs = 0;
	if (RageTimer::GetTimeSinceStartMicroseconds() >= fNextUsecs)
	{
		/* Lockless: only Mix() can write to underruns. */
		int current_underruns = underruns;
		if( current_underruns > logged_underruns )
		{
			LOG->MapLog( "GenericMixingUnderruns", "Mixing underruns: %i", current_underruns - logged_underruns );
			LOG->Trace( "Mixing underruns: %i", current_underruns - logged_underruns );
			logged_underruns = current_underruns;

			/* Don't log again for at least a second, or we'll burst output
			 * and possibly cause more underruns. */
			fNextUsecs = RageTimer::GetTimeSinceStartMicroseconds() + iUsecs;
		}
	}

	m_Mutex.Unlock();
}

void RageSoundDriver::StartMixing( RageSoundBase *pSound )
{
	/* Lock available m_Sounds[], and reserve a slot. */
	m_SoundListMutex.Lock();

	unsigned i;
	for( i = 0; i < ARRAYLEN(m_Sounds); ++i )
		if( m_Sounds[i].m_State == Sound::AVAILABLE )
			break;
	if( i == ARRAYLEN(m_Sounds) )
	{
		m_SoundListMutex.Unlock();
		return;
	}

	Sound &s = m_Sounds[i];
	s.m_State = Sound::BUFFERING;

	/* We've reserved our slot; we can safely unlock now.  Don't hold onto it longer
	 * than needed, since prebuffering might take some time. */
	m_SoundListMutex.Unlock();

	s.m_pSound = pSound;
	s.m_StartTime = pSound->GetStartTime();
	s.m_Buffer.clear();

	/* Initialize the sound buffer. */
	int BufferSize = frames_to_buffer;

	s.Allocate( BufferSize );

//	LOG->Trace("StartMixing(%s) (%p)", s.m_pSound->GetLoadedFilePath().c_str(), s.m_pSound );

	/* Prebuffer some frames before changing the sound to PLAYING. */
	while( s.m_Buffer.num_writable() )
	{
//		LOG->Trace("StartMixing: (#%i) buffering %i (%i writable) (%p)", i, (int) frames_to_buffer, s.buffer.num_writable(), s.m_pSound );
		int iWrote = GetDataForSound( s );
		if( iWrote < 0 )
			break;
	}

	s.m_State = Sound::PLAYING;

//	LOG->Trace("StartMixing: (#%i) finished prebuffering(%s) (%p)", i, s.m_pSound->GetLoadedFilePath().c_str(), s.m_pSound );
}

void RageSoundDriver::StopMixing( RageSoundBase *pSound )
{
	/* Lock, to make sure the decoder thread isn't running on this sound while we do this. */
	m_Mutex.Lock();

	/* Find the sound. */
	unsigned i;
	for( i = 0; i < ARRAYLEN(m_Sounds); ++i )
		if( m_Sounds[i].m_State != Sound::AVAILABLE && m_Sounds[i].m_pSound == pSound )
			break;
	if( i == ARRAYLEN(m_Sounds) )
	{
		m_Mutex.Unlock();
		LOG->Trace( "not stopping a sound because it's not playing" );
		return;
	}

	/* If we're already in STOPPED, there's nothing to do. */
	if( m_Sounds[i].m_State == Sound::STOPPED )
	{
		m_Mutex.Unlock();
		LOG->Trace( "not stopping a sound because it's already in STOPPED" );
		return;
	}

//	LOG->Trace("StopMixing: set %p (%s) to HALTING", m_Sounds[i].m_pSound, m_Sounds[i].m_pSound->GetLoadedFilePath().c_str());

	/* Tell the mixing thread to flush the buffer.  We don't have to worry about
	 * the decoding thread, since we've locked m_Mutex. */
	m_Sounds[i].m_State = Sound::HALTING;

	/* Invalidate the m_pSound pointer to guarantee we don't make any further references to
	 * it.  Once this call returns, the sound may no longer exist. */
	m_Sounds[i].m_pSound = nullptr;
//	LOG->Trace("end StopMixing");

	m_Mutex.Unlock();

	pSound->SoundIsFinishedPlaying();
}


bool RageSoundDriver::PauseMixing( RageSoundBase *pSound, bool bStop )
{
	LockMut( m_Mutex );

	/* Find the sound. */
	unsigned i;
	for( i = 0; i < ARRAYLEN(m_Sounds); ++i )
		if( m_Sounds[i].m_State != Sound::AVAILABLE && m_Sounds[i].m_pSound == pSound )
			break;

	/* A sound can be paused in PLAYING or STOPPING.  (STOPPING means the sound
	 * has been decoded to the end, and we're waiting for that data to finish, so
	 * externally it looks and acts like PLAYING.) */
	if( i == ARRAYLEN(m_Sounds) ||
		(m_Sounds[i].m_State != Sound::PLAYING && m_Sounds[i].m_State != Sound::STOPPING) )
	{
		LOG->Trace( "not pausing a sound because it's not playing" );
		return false;
	}

	m_Sounds[i].m_bPaused = bStop;

	return true;
}

void RageSoundDriver::StartDecodeThread()
{
	ASSERT( !m_DecodeThread.IsCreated() );

	m_DecodeThread.Create( DecodeThread_start, this );
}

void RageSoundDriver::SetDecodeBufferSize( int iFrames )
{
	ASSERT( !m_DecodeThread.IsCreated() );

	frames_to_buffer = iFrames;
}

RageSoundDriver::RageSoundDriver():
	m_Mutex("RageSoundDriver"),
	m_SoundListMutex("SoundListMutex")
{
	m_bShutdownDecodeThread = false;
	m_iMaxHardwareFrame = 0;
	m_iVMaxHardwareFrame = 0;
	SetDecodeBufferSize( 4096 );
	m_DecodeThread.SetName("Decode thread");
}

RageSoundDriver::~RageSoundDriver()
{
	/* Signal the decoding thread to quit. */
	if( m_DecodeThread.IsCreated() )
	{
		m_bShutdownDecodeThread = true;
		LOG->Trace("Shutting down decode thread ...");
		LOG->Flush();
		m_DecodeThread.Wait();
		LOG->Trace("Decode thread shut down.");
		LOG->Flush();
	}
}

int64_t RageSoundDriver::ClampHardwareFrame( int64_t iHardwareFrame ) const
{
	/* It's sometimes possible for the hardware position to move backwards, usually
	 * on underrun.  We can try to prevent this in each driver, but it's an obscure
	 * error, so let's clamp the result here instead. */
	if( iHardwareFrame < m_iMaxHardwareFrame )
	{
		/* Clamp the output to one per second, so one underruns don't cascade due to
		 * output spam. */
		static int64_t lastTime = 0;
		int64_t currentTime = RageTimer::GetTimeSinceStartMicroseconds();
		if( lastTime == 0 || (currentTime - lastTime) > 1000000 )
		{
			LOG->Trace("RageSoundDriver: driver returned a lesser position (%" PRId64 " < %" PRId64 ")", iHardwareFrame, m_iMaxHardwareFrame);
			lastTime = currentTime;
		}
		return m_iMaxHardwareFrame;
	}
	if( iHardwareFrame > m_iMaxHardwareFrame )
	{
		m_iMaxHardwareFrame = iHardwareFrame;
	}
	return m_iMaxHardwareFrame;
}

int64_t RageSoundDriver::GetHardwareFrame( RageTimer *pTimestamp=nullptr ) const
{
	if( pTimestamp == nullptr )
		return ClampHardwareFrame( GetPosition() );

	/*
	 * We may have unpredictable scheduling delays between updating the timestamp
	 * and reading the sound position.  If we're preempted while doing this and
	 * it may have caused the timestamp to not match the returned time, retry.
	 *
	 * As a failsafe, only allow a few attempts.  If this has to try more than
	 * a few times, then probably we have thread contention that's causing more
	 * severe performance problems, anyway.
	 */
	int iTries = 3;
	int64_t iPositionFrames;
	uint64_t iStartTime;
	const uint64_t iThreshold = 2000ULL;

	do
	{
		iStartTime = RageTimer::GetTimeSinceStartMicroseconds();
		iPositionFrames = GetPosition();
		uint64_t elapsedTime = RageTimer::GetTimeSinceStartMicroseconds() - iStartTime;
		if (elapsedTime <= iThreshold) break;
	} while (--iTries);

	if( iTries == 0 )
	{
		static bool bLogged = false;
		if( !bLogged )
		{
			bLogged = true;
			LOG->Warn( "RageSoundDriver::GetHardwareFrame: too many tries" );
		}
	}

	return ClampHardwareFrame( iPositionFrames );
}

/*
 * (c) 2002-2004 Glenn Maynard
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

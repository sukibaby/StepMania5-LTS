#include "global.h"
#include "ArchHooks_Win32.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "RageThreads.h"
#include "ProductInfo.h"
#include "archutils/win32/AppInstance.h"
#include "archutils/win32/crash.h"
#include "archutils/win32/DebugInfoHunt.h"
#include "archutils/win32/ErrorStrings.h"
#include "archutils/win32/RestartProgram.h"
#include "archutils/Win32/RegistryAccess.h"

#include "VersionHelpers.h"

#include <cstdint>
#include <vector>


static HANDLE g_hInstanceMutex;
static bool g_bIsMultipleInstance = false;

void InvalidParameterHandler( const wchar_t *szExpression, const wchar_t *szFunction, const wchar_t *szFile,
					  unsigned int iLine, uintptr_t pReserved )
{
	FAIL_M( "Invalid parameter" ); //TODO: Make this more informative
}

ArchHooks_Win32::ArchHooks_Win32()
{
	HOOKS = this;

	/* Disable critical errors, and handle them internally.  We never want the
	 * "drive not ready", etc. dialogs to pop up. */
	SetErrorMode( SetErrorMode(0) | SEM_FAILCRITICALERRORS );

	CrashHandler::CrashHandlerHandleArgs( g_argc, g_argv );
	SetUnhandledExceptionFilter( CrashHandler::ExceptionHandler );

	_set_invalid_parameter_handler( InvalidParameterHandler );

	/* Windows boosts priority on keyboard input, among other things.  Disable that for
	 * the main thread. */
	SetThreadPriorityBoost( GetCurrentThread(), TRUE );

	g_hInstanceMutex = CreateMutex( nullptr, TRUE, PRODUCT_ID );

	g_bIsMultipleInstance = false;
	if( GetLastError() == ERROR_ALREADY_EXISTS )
		g_bIsMultipleInstance = true;
}

ArchHooks_Win32::~ArchHooks_Win32()
{
	CloseHandle( g_hInstanceMutex );
}

void ArchHooks_Win32::DumpDebugInfo()
{
	/* This is a good time to do the debug search: before we actually
	 * start OpenGL (in case something goes wrong). */
	SearchForDebugInfo();
}

struct CallbackData
{
	HWND hParent;
	HWND hResult;
};

// Like GW_ENABLEDPOPUP:
static BOOL CALLBACK GetEnabledPopup( HWND hWnd, LPARAM lParam )
{
	CallbackData *pData = (CallbackData *) lParam;
	if( GetParent(hWnd) != pData->hParent )
		return TRUE;
	if( (GetWindowLong(hWnd, GWL_STYLE) & WS_POPUP) != WS_POPUP )
		return TRUE;
	if( !IsWindowEnabled(hWnd) )
		return TRUE;

	pData->hResult = hWnd;
	return FALSE;
}

bool ArchHooks_Win32::CheckForMultipleInstances(int argc, char* argv[])
{
	if( !g_bIsMultipleInstance )
		return false;

	/* Search for the existing window.  Prefer to use the class name, which is less likely to
	 * have a false match, and will match the gameplay window.  If that fails, try the window
	 * name, which should match the loading window. */
	HWND hWnd = FindWindow( PRODUCT_ID, nullptr );
	if( hWnd == nullptr )
		hWnd = FindWindow( nullptr, PRODUCT_ID );

	if( hWnd != nullptr )
	{
		/* If the application has a model dialog box open, we want to be sure to give focus to it,
		 * not the main window. */
		CallbackData data;
		data.hParent = hWnd;
		data.hResult = nullptr;
		EnumWindows( GetEnabledPopup, (LPARAM) &data );

		if( data.hResult != nullptr )
			SetForegroundWindow( data.hResult );
		else
			SetForegroundWindow( hWnd );

		// Send the command line to the existing window.
		std::vector<RString> vsArgs;
		for( int i=0; i<argc; i++ )
			vsArgs.push_back( argv[i] );
		RString sAllArgs = join("|", vsArgs);
		COPYDATASTRUCT cds;
		cds.dwData = 0;
		cds.cbData = sAllArgs.size();
		cds.lpData = (void*)sAllArgs.data();
		SendMessage(
			(HWND)hWnd, // HWND hWnd = handle of destination window
			WM_COPYDATA,
			(WPARAM)nullptr, // HANDLE OF SENDING WINDOW
			(LPARAM)&cds ); // 2nd msg parameter = pointer to COPYDATASTRUCT
	}

	return true;
}

void ArchHooks_Win32::RestartProgram()
{
	Win32RestartProgram();
}

void ArchHooks_Win32::SetTime( tm newtime )
{
	SYSTEMTIME st;
	ZERO( st );
	st.wYear = (WORD)newtime.tm_year+1900;
	st.wMonth = (WORD)newtime.tm_mon+1;
	st.wDay = (WORD)newtime.tm_mday;
	st.wHour = (WORD)newtime.tm_hour;
	st.wMinute = (WORD)newtime.tm_min;
	st.wSecond = (WORD)newtime.tm_sec;
	st.wMilliseconds = 0;
	SetLocalTime( &st );
}

void ArchHooks_Win32::BoostPriority()
{
	// We just want a slight boost, so we don't skip needlessly if something happens
	// in the background. We don't really want to be high-priority—above normal should be enough.
	//
	// Be sure to boost the app, not the thread, to make sure the
	// sound thread stays higher priority than the main thread.
	//
	// Also note that high priority won't prevent the game from being interrupted by Windows
	// notifications - that needs to be handled within ArchUtils.
	SetPriorityClass( GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS );
}

void ArchHooks_Win32::UnBoostPriority()
{
	SetPriorityClass( GetCurrentProcess(), NORMAL_PRIORITY_CLASS );
}

void ArchHooks_Win32::SetupConcurrentRenderingThread()
{
	SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL );
}

float ArchHooks_Win32::GetDisplayAspectRatio()
{
	DEVMODE dm;
	ZERO( dm );
	dm.dmSize = sizeof(dm);
	BOOL bResult = EnumDisplaySettings( nullptr, ENUM_REGISTRY_SETTINGS, &dm );
	ASSERT( bResult != 0 );
	return dm.dmPelsWidth / (float)dm.dmPelsHeight;
}

RString ArchHooks_Win32::GetClipboard()
{
	HGLOBAL hgl;
	LPTSTR lpstr;
	RString ret;

	// First make sure that the clipboard actually contains a string
	// (or something stringifiable)
	if(unlikely( !IsClipboardFormatAvailable( CF_TEXT ) )) return "";

	// Yes. All this mess just to gain access to the string stored by the clipboard.
	// I'm having flashbacks to Berkeley sockets.
	if(unlikely( !OpenClipboard( nullptr ) ))
		{ LOG->Warn(werr_ssprintf( GetLastError(), "InputHandler_DirectInput: OpenClipboard() failed" )); return ""; }

	hgl = GetClipboardData( CF_TEXT );
	if(unlikely( hgl == nullptr ))
		{ LOG->Warn(werr_ssprintf( GetLastError(), "InputHandler_DirectInput: GetClipboardData() failed" )); CloseClipboard(); return ""; }

	lpstr = (LPTSTR) GlobalLock( hgl );
	if(unlikely( lpstr == nullptr ))
		{ LOG->Warn(werr_ssprintf( GetLastError(), "InputHandler_DirectInput: GlobalLock() failed" )); CloseClipboard(); return ""; }

	// And finally, we have a char (or wchar_t) array of the clipboard contents,
	// pointed to by sToPaste.
	// (Hopefully.)

#ifdef UNICODE
	ret = WStringToRString( wstring()+*lpstr );
#else
	ret = RString( lpstr );
#endif

	// And now we clean up.
	GlobalUnlock( hgl );
	CloseClipboard();

	return ret;
}

#include <audioclient.h>
#include <comdef.h>
#include <combaseapi.h>
#include <mmdeviceapi.h>
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
uint32_t ArchHooks_Win32::DetermineSampleRate() const
{
	HRESULT hr;
	uint32_t sampleRate = 0;

	// We need to initialize COM before using WASAPI methods.
	// Neither WaveOut nor DirectSound require this, but those API's aren't
	// capable of determining the sample rate of the default audio endpoint.
	hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	bool comInitialized = SUCCEEDED(hr);
	IMMDeviceEnumerator* pEnumerator = nullptr;
	IMMDevice* pDevice = nullptr;
	IAudioClient* pAudioClient = nullptr;
	WAVEFORMATEX* pwfx = nullptr;

	// Here we try to get the default audio endpoint and its sample rate.
	// The do-while loop prevents a series of ugly if statements repeatedly
	// checking the HRESULT. Once we have the sample rate, we can break out of
	// the loop.
	do {
		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
								__uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
		if (FAILED(hr)) break;

		hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
		if (FAILED(hr)) break;

		hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&pAudioClient);
		if (FAILED(hr)) break;

		hr = pAudioClient->GetMixFormat(&pwfx);
		if (FAILED(hr)) break;

		sampleRate = pwfx->nSamplesPerSec;
	} while (false);

	// Clean up resources as needed.
	if (pwfx) {
		CoTaskMemFree(pwfx);
	}
	if (pAudioClient) {
		pAudioClient->Release();
	}
	if (pDevice) {
		pDevice->Release();
	}
	if (pEnumerator) {
		pEnumerator->Release();
	}
	if (comInitialized) {
		CoUninitialize();
	}

	return sampleRate;
}


/*
 * (c) 2003-2004 Glenn Maynard, Chris Danford
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

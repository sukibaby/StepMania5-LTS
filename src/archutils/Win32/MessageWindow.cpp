#include "global.h"
#include "MessageWindow.h"
#include "RageUtil.h"
#include "AppInstance.h"
#include "archutils/Win32/ErrorStrings.h"

MessageWindow::MessageWindow( const RString &sClassName )
{
	AppInstance inst;
	WNDCLASS WindowClass =
	{
		CS_OWNDC | CS_BYTEALIGNCLIENT,
		WndProc,
		0,				/* cbClsExtra */
		0,				/* cbWndExtra */
		inst,				/* hInstance */
		nullptr,				/* set icon later */
		LoadCursor( nullptr, IDC_ARROW ),	/* default cursor */
		nullptr,				/* hbrBackground */
		nullptr,				/* lpszMenuName */
		sClassName.c_str()			/* lpszClassName */
	}; 

	if( !RegisterClassA(&WindowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS )
		RageException::Throw( "%s", werr_ssprintf( GetLastError(), "RegisterClass" ).c_str() );

	// XXX: on 2k/XP, use HWND_MESSAGE as parent
	m_hWnd = CreateWindow( sClassName.c_str(), sClassName.c_str(), WS_DISABLED, 0, 0, 0, 0, nullptr, nullptr, inst, nullptr );
	ASSERT( m_hWnd != nullptr );

	SetProp( m_hWnd, "MessageWindow", this );
}

MessageWindow::~MessageWindow()
{
	RemoveProp( m_hWnd, "MessageWindow" );
	DestroyWindow( m_hWnd );
}

void MessageWindow::Run()
{
	/* Process messages until StopRunning is called. */
	m_bDone = false;
	while( !m_bDone )
	{
		MSG msg;
		int iRet = GetMessage( &msg, m_hWnd, 0, 0 );
		ASSERT( iRet != -1 );
		if( iRet == 0 )
			break;
		DispatchMessage( &msg );
	}
}

void MessageWindow::StopRunning()
{
	m_bDone = true;
}

LRESULT CALLBACK MessageWindow::WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	MessageWindow *pThis = (MessageWindow *) GetProp( hWnd, "MessageWindow" );
	if( pThis != nullptr && pThis->HandleMessage(msg, wParam, lParam) )
		return 0;

	return DefWindowProc( hWnd, msg, wParam, lParam );
}

/*
 * (c) 2006 Glenn Maynard
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

#include "global.h"
#include "Threads_Win32.h"
#include "RageUtil.h"
#include "RageThreads.h"
#include "RageTimer.h"
#include "archutils/Win32/ErrorStrings.h"

#include <cstdint>
#include <mutex>
#include <memory> 

const int MAX_THREADS = 128;

static std::unique_ptr<MutexImpl_Win32> g_pThreadIdMutex;
static bool g_ThreadIdMutexInitialized = false;
static std::mutex g_ThreadIdMutexInitLock;

static void InitThreadIdMutex()
{
	std::lock_guard<std::mutex> lock(g_ThreadIdMutexInitLock);
	if (g_ThreadIdMutexInitialized)
		return;

	std::unique_ptr<MutexImpl_Win32> temp = std::make_unique<MutexImpl_Win32>(nullptr);
	g_ThreadIdMutexInitialized = true;

	g_pThreadIdMutex = std::move(temp);
}

static uint64_t g_ThreadIds[MAX_THREADS];
static HANDLE g_ThreadHandles[MAX_THREADS];
static std::mutex g_ThreadDataMutex;

HANDLE Win32ThreadIdToHandle(uint64_t iID)
{
	std::lock_guard<std::mutex> lock(g_ThreadDataMutex);
	for (int i = 0; i < MAX_THREADS; ++i)
	{
		if (g_ThreadIds[i] == iID)
			return g_ThreadHandles[i];
	}

	return nullptr;
}

void ThreadImpl_Win32::Halt(bool Kill)
{
	if (Kill)
	{
		ExitThread(0); // 0 indicates the thread was terminated
	}
	else
	{
		SuspendThread(ThreadHandle);
	}
}

void ThreadImpl_Win32::Resume()
{
	ResumeThread( ThreadHandle );
}

uint64_t ThreadImpl_Win32::GetThreadId() const
{
	return static_cast<uint64_t>(ThreadId);
}

int ThreadImpl_Win32::Wait()
{
	WaitForSingleObject( ThreadHandle, INFINITE );

	DWORD ret;
	GetExitCodeThread( ThreadHandle, &ret );

	CloseHandle( ThreadHandle );
	ThreadHandle = nullptr;

	return ret;
}

// SetThreadName magic comes from VirtualDub.
#define MS_VC_EXCEPTION 0x406d1388

typedef struct tagTHREADNAME_INFO
{
	DWORD dwType;		// must be 0x1000
	LPCSTR szName;		// pointer to name (in same addr space)
	DWORD dwThreadID;	// thread ID (-1 caller thread)
	DWORD dwFlags;		// reserved for future use, must be zero
} THREADNAME_INFO;

static void SetThreadName( DWORD dwThreadID, LPCTSTR szThreadName )
{
#if defined(_MSC_VER)
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = szThreadName;
	info.dwThreadID = dwThreadID;
	info.dwFlags = 0;

#ifdef _DEBUG
	__try
	{
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(DWORD), (ULONG_PTR*)&info);
	}
	__except (EXCEPTION_CONTINUE_EXECUTION) // NOTE(sukibaby): compiler suggests trying EXCEPTION_EXECUTE_HANDLER to avoid possible infinite loop.
	{
	}
#endif

#elif defined(__GNUC__)
	pthread_setname_np(pthread_self(), szThreadName);
#endif
}

static DWORD WINAPI StartThread( LPVOID pData )
{
	ThreadImpl_Win32 *pThis = static_cast<ThreadImpl_Win32 *>(pData);

	SetThreadName( GetCurrentThreadId(), RageThread::GetCurrentThreadName() );

	DWORD ret = static_cast<DWORD>(pThis->m_pFunc(pThis->m_pData));

	std::lock_guard<std::mutex> lock(g_ThreadDataMutex);
	for (int i = 0; i < MAX_THREADS; ++i)
	{
		if( g_ThreadIds[i] == RageThread::GetCurrentThreadID() )
		{
			g_ThreadHandles[i] = nullptr;
			g_ThreadIds[i] = 0;
			break;
		}
	}

	return ret;
}

static int GetOpenSlot( uint64_t iID )
{
	InitThreadIdMutex();

	g_pThreadIdMutex->Lock();

	// Find an open slot in g_ThreadIds.
	int slot = 0;
	while( slot < MAX_THREADS && g_ThreadIds[slot] != 0 )
		++slot;
	ASSERT( slot < MAX_THREADS );

	g_ThreadIds[slot] = iID;

	g_pThreadIdMutex->Unlock();

	return slot;
}

ThreadImpl *MakeThisThread()
{
	std::unique_ptr<ThreadImpl_Win32> thread = std::make_unique<ThreadImpl_Win32>();

	SetThreadName( GetCurrentThreadId(), RageThread::GetCurrentThreadName() );

	const HANDLE CurProc = GetCurrentProcess();
	int ret = DuplicateHandle( CurProc, GetCurrentThread(), CurProc,
		&thread->ThreadHandle, 0, false, DUPLICATE_SAME_ACCESS );

	if( !ret )
	{
		thread->ThreadHandle = nullptr;
	}

	thread->ThreadId = GetCurrentThreadId();

	int slot = GetOpenSlot( GetCurrentThreadId() );
	g_ThreadHandles[slot] = thread->ThreadHandle;

	return thread.release();
}

ThreadImpl *MakeThread( int (*pFunc)(void *pData), void *pData, uint64_t *piThreadID )
{
	std::unique_ptr<ThreadImpl_Win32> thread = std::make_unique<ThreadImpl_Win32>();
	thread->m_pFunc = pFunc;
	thread->m_pData = pData;

	thread->ThreadHandle = CreateThread(nullptr, 0, &StartThread, thread.get(), CREATE_SUSPENDED, &thread->ThreadId);
	*piThreadID = static_cast<uint64_t>(thread->ThreadId);
	ASSERT_M(thread->ThreadHandle != nullptr, ssprintf("%s", werr_ssprintf(GetLastError(), "CreateThread").c_str()));

	int slot = GetOpenSlot( thread->ThreadId );
	g_ThreadHandles[slot] = thread->ThreadHandle;

	int iRet = ResumeThread( thread->ThreadHandle );
	ASSERT_M( iRet == 1, ssprintf("%s", werr_ssprintf(GetLastError(), "ResumeThread").c_str() ) );

	return thread.release();
}


MutexImpl_Win32::MutexImpl_Win32( RageMutex *pParent ):
	MutexImpl( pParent )
{
	mutex = CreateMutex( nullptr, false, nullptr );
	ASSERT_M( mutex != nullptr, werr_ssprintf(GetLastError(), "CreateMutex") );
}

MutexImpl_Win32::~MutexImpl_Win32()
{
	CloseHandle( mutex );
}

/* NOTE(sukibaby):  the function name here is a bit misleading.
 * It provides additional error handling and assertions which
 * are not present in WaitForSingleObject.
 * It also consolidates some switch case logic we need to use
 * mutliple times within this file.
 * For these reasons I've left this function alone. */
static bool SimpleWaitForSingleObject( HANDLE h, DWORD ms )
{
	ASSERT( h != nullptr );

	DWORD ret = WaitForSingleObject( h, ms );
	switch( ret )
	{
	case WAIT_OBJECT_0:
		return true;

	case WAIT_TIMEOUT:
		return false;

	case WAIT_ABANDONED:
		FAIL_M("WAIT_ABANDONED");

	case WAIT_FAILED:
		FAIL_M(werr_ssprintf(GetLastError(), "WaitForSingleObject"));

	default:
		FAIL_M("unknown");
	}
}

bool MutexImpl_Win32::Lock()
{
	DWORD dwWaitResult = WaitForSingleObject(mutex, INFINITE);
	switch (dwWaitResult)
	{
	case WAIT_OBJECT_0:
		return true;

	case WAIT_TIMEOUT:
		return false;

	case WAIT_ABANDONED:
		return false;

	default:
		FAIL_M("WaitForSingleObject failed in a way that shouldn't have been possible");
	}
}

bool MutexImpl_Win32::TryLock()
{
	return SimpleWaitForSingleObject( mutex, 0 );
}

void MutexImpl_Win32::Unlock()
{
	const bool ret = !!ReleaseMutex( mutex );

	/* We can't ASSERT here, since this is called from checkpoints,
	 * which is called from ASSERT. */
	if( !ret )
		sm_crash( werr_ssprintf( GetLastError(), "ReleaseMutex failed" ) );
}

uint64_t GetThisThreadId()
{
	return GetCurrentThreadId();
}

uint64_t GetInvalidThreadId()
{
	return 0;
}

MutexImpl *MakeMutex( RageMutex *pParent )
{
	return new MutexImpl_Win32( pParent );
}

EventImpl_Win32::EventImpl_Win32( MutexImpl_Win32 *pParent )
{
	m_pParent = pParent;
	m_iNumWaiting = 0;
	m_WakeupSema = CreateSemaphore( nullptr, 0, 0x7fffffff, nullptr );
	InitializeCriticalSection( &m_iNumWaitingLock );
	m_WaitersDone = CreateEvent( nullptr, FALSE, FALSE, nullptr );
}

EventImpl_Win32::~EventImpl_Win32()
{
	ASSERT_M( m_iNumWaiting == 0, ssprintf("event destroyed while still in use (%i)", m_iNumWaiting) );

	// We don't own m_pParent; don't free it.
	CloseHandle( m_WakeupSema );
	DeleteCriticalSection( &m_iNumWaitingLock );
	CloseHandle( m_WaitersDone );
}

// Event logic from http://www.cs.wustl.edu/~schmidt/win32-cv-1.html.
bool EventImpl_Win32::Wait( RageTimer *pTimeout )
{
	EnterCriticalSection( &m_iNumWaitingLock );
	++m_iNumWaiting;
	LeaveCriticalSection( &m_iNumWaitingLock );

	unsigned iMilliseconds = INFINITE;
	if( pTimeout != nullptr )
	{
		float fSecondsInFuture = -pTimeout->Ago();
		iMilliseconds = static_cast<unsigned>(std::max(0, static_cast<int>(fSecondsInFuture * 1000)));
	}

	// Unlock the mutex and wait for a signal.
	bool bSuccess = (SignalObjectAndWait(m_pParent->mutex, m_WakeupSema, iMilliseconds, FALSE) == WAIT_OBJECT_0);

	EnterCriticalSection( &m_iNumWaitingLock );
	if( !bSuccess )
	{
		/* Avoid a race condition: someone may have signalled the object
		 * between SignalObjectAndWait and EnterCriticalSection.
		 * While we hold m_iNumWaitingLock, poll (with a zero timeout) the
		 * object one last time. */
		if( WaitForSingleObject( m_WakeupSema, 0 ) == WAIT_OBJECT_0 )
		{
			bSuccess = true;
		}
	}
	--m_iNumWaiting;
	bool bLastWaiting = (m_iNumWaiting == 0);
	LeaveCriticalSection( &m_iNumWaitingLock );

	/* If we're the last waiter to wake up, and we were actually woken by
	 * another thread (not by timeout), wake up the signaller. */
	if (bLastWaiting && bSuccess)
	{
		SignalObjectAndWait(m_WaitersDone, m_pParent->mutex, INFINITE, FALSE);
	}
	else
	{
		WaitForSingleObject(m_pParent->mutex, INFINITE);
	}

	return bSuccess;
}

void EventImpl_Win32::Signal()
{
	EnterCriticalSection( &m_iNumWaitingLock );

	if( m_iNumWaiting == 0 )
	{
		LeaveCriticalSection( &m_iNumWaitingLock );
		return;
	}

	ReleaseSemaphore( m_WakeupSema, 1, 0 );

	LeaveCriticalSection( &m_iNumWaitingLock );

	// The waiter will touch m_WaitersDone.
	WaitForSingleObject( m_WaitersDone, INFINITE );
}

void EventImpl_Win32::Broadcast()
{
	EnterCriticalSection( &m_iNumWaitingLock );

	if( m_iNumWaiting == 0 )
	{
		LeaveCriticalSection( &m_iNumWaitingLock );
		return;
	}

	ReleaseSemaphore( m_WakeupSema, m_iNumWaiting, 0 );

	LeaveCriticalSection( &m_iNumWaitingLock );

	/* The last waiter will touch m_WaitersDone, so we wait for all waiters
	 * to wake up and start waiting for the mutex before returning. */
	WaitForSingleObject( m_WaitersDone, INFINITE );
}

EventImpl *MakeEvent( MutexImpl *pMutex )
{
	MutexImpl_Win32* pWin32Mutex = static_cast<MutexImpl_Win32*>(pMutex);

	return new EventImpl_Win32( pWin32Mutex );
}

SemaImpl_Win32::SemaImpl_Win32( int iInitialValue )
{
	sem = CreateSemaphore( nullptr, iInitialValue, 999999999, nullptr );
	m_iCounter = iInitialValue;
}

SemaImpl_Win32::~SemaImpl_Win32()
{
	CloseHandle( sem );
}

void SemaImpl_Win32::Post()
{
	++m_iCounter;
	ReleaseSemaphore( sem, 1, nullptr );
}

bool SemaImpl_Win32::Wait()
{
	int len = 15000;
	int tries = 5;

	while( tries-- )
	{
		/* Wait for 15 seconds. If it takes longer than that, we're
		 * probably deadlocked. */
		if( SimpleWaitForSingleObject( sem, len ) )
		{
			--m_iCounter;
			return true;
		}

		/* Timed out; probably deadlocked. Try again a few more times,
		 * with a smaller timeout, just in case we're debugging and
		 * happened to stop while waiting on the mutex. */
		len = 1000;
	}

	return false;
}

bool SemaImpl_Win32::TryWait()
{
	if( !SimpleWaitForSingleObject( sem, 0 ) )
		return false;

	--m_iCounter;
	return true;
}

SemaImpl *MakeSemaphore( int iInitialValue )
{
	return new SemaImpl_Win32( iInitialValue );
}

/*
 * (c) 2001-2004 Glenn Maynard
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

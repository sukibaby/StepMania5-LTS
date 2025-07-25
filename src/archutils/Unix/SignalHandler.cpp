#include "global.h"

#include "RageLog.h"
#include "SignalHandler.h"
#include "GetSysInfo.h"

#if defined(HAVE_LIBPTHREAD)
#include "archutils/Common/PthreadHelpers.h"
#endif

#include <cerrno>
#include <cstring>
#include <memory>
#include <vector>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <sys/mman.h>

#if defined(MACOSX)
extern "C" int sigaltstack(const stack_t * __restrict, stack_t * __restrict);
#endif
#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
# define MAP_ANONYMOUS MAP_ANON
#endif

#if defined(__GNUC__)
# define NOINLINE __attribute__((__noinline__))
#else
# define NOINLINE
#endif

static int find_stack_direction2( char *p ) NOINLINE;
static int find_stack_direction() NOINLINE;

static std::vector<SignalHandler::handler> handlers;
std::unique_ptr<SaveSignals> saved_sigs;

static int signals[] =
{
	SIGALRM, SIGBUS, SIGFPE, SIGHUP, SIGILL, SIGINT, SIGABRT,
	SIGQUIT, SIGSEGV, SIGTRAP, SIGTERM, SIGVTALRM, SIGXCPU, SIGXFSZ,
#if defined(HAVE_DECL_SIGPWR) && HAVE_DECL_SIGPWR
	SIGPWR,
#endif
#if defined(HAVE_DECL_SIGUSR1) && HAVE_DECL_SIGUSR1
	SIGUSR1,
#endif
	-1
};

/*
 * Store signals and restore them.  This lets us clean up after libraries
 * that like to mess with our signal handler.  (Of course, if we undo
 * signal handlers for libraries, we need to be sure to handle whatever it
 * was cleaning up by hand.)
 */

SaveSignals::SaveSignals()
{
	/* Store the old signal handlers. */
	for( int i = 0; signals[i] != -1; ++i )
	{
		struct sigaction sa;
		sigaction( signals[i], nullptr, &sa );
		old_handlers.push_back( sa );
	}
}

SaveSignals::~SaveSignals()
{
	/* Restore the old signal handlers. */
	for( unsigned i = 0; i < old_handlers.size(); ++i )
		sigaction( signals[i], &old_handlers[i], nullptr );
}

static void SigHandler( int signal, siginfo_t *si, void *ucp )
{
	bool bMaskSignal = false;
	for( unsigned i = 0; i < handlers.size(); ++i )
		bMaskSignal |= handlers[i]( signal, si, (const ucontext_t *)ucp );
	if( !bMaskSignal )
	{
		struct sigaction sa;
		sa.sa_flags = 0;
		sigemptyset( &sa.sa_mask );

		/* Set up the default signal handler. */
		sa.sa_handler = SIG_DFL;

		struct sigaction old;
		sigaction( signal, &sa, &old );
		raise( signal );
		sigaction( signal, &old, nullptr );
	}
}

int find_stack_direction2( char *p )
{
	char c;
	return (&c > p) ? +1:-1;
}

int find_stack_direction()
{
	char c;
	return find_stack_direction2( &c );
}

/* Create a stack with a barrier page at the end to guard against stack overflow. */
static void *CreateStack( int size )
{
	const long PageSize = getpagesize();

	/* Round size up to the nearest PageSize. */
	size += PageSize-1;
	size -= (size%PageSize);

	/* mmap always returns page-aligned data.
	 *
	 * mmap entries always show up individually in /proc/#/maps.  We could use posix_memalign as
	 * a fallback, but we'd have to put a barrier page on both sides to guarantee that. */
	char *p = nullptr;
	p = (char *) mmap( nullptr, size+PageSize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0 );

	if( p == (void *) -1 )
		return nullptr;
	// if( posix_memalign( (void**) &p, PageSize, RealSize ) != 0 )
	//	return nullptr;

	if( find_stack_direction() < 0 )
	{
		/* The stack grows towards smaller addresses.  Protect the first page. */
		mprotect( p, PageSize, PROT_NONE );
		p += PageSize;
	}
	else
	{
		/* The stack grows towards larger addresses.  Protect the last page. */
		mprotect( p+size, PageSize, PROT_NONE );
	}

	return p;
}

/* Hook up events to fatal signals, so we can clean up if we're killed. */
void SignalHandler::OnClose( handler h )
{
	//if the unique_ptr has not been set, then enter.
	if( !saved_sigs )
	{
		saved_sigs.reset(new SaveSignals());
		bool bUseAltSigStack = true;

#if defined(LINUX)
		/* Linuxthreads (pre-NPTL) sigaltstack is broken. */
		if( !UsingNPTL() )
			bUseAltSigStack = false;
#endif

		/* Allocate a separate signal stack.  This makes the crash handler work
		 * if we run out of stack space. */
		const int AltStackSize = 1024*64;
		void *p = nullptr;
		if( bUseAltSigStack )
			p = CreateStack( AltStackSize );

		if( p != nullptr )
		{
			stack_t ss;
			ss.ss_sp = (char*)p; /* cast for Darwin */
			ss.ss_size = AltStackSize;
			ss.ss_flags = 0;
			if( sigaltstack( &ss, nullptr ) == -1 )
			{
				LOG->Info( "sigaltstack failed: %s", strerror(errno) );
				p = nullptr; /* no SA_ONSTACK */
			}
		}

		struct sigaction sa;

		sa.sa_flags = 0;
		if( p != nullptr )
			sa.sa_flags |= SA_ONSTACK;
		sa.sa_flags |= SA_NODEFER;
		sa.sa_flags |= SA_SIGINFO;
		sigemptyset(&sa.sa_mask);

		/* Set up our signal handlers. */
		sa.sa_sigaction = SigHandler;
		for( int i = 0; signals[i] != -1; ++i )
			sigaction( signals[i], &sa, nullptr );

		/* Block SIGPIPE, so we get EPIPE. */
		sa.sa_handler = SIG_IGN;
		sigaction( SIGPIPE, &sa, nullptr );
	}
	handlers.push_back(h);
}

/*
 * (c) 2003-2004 Glenn Maynard
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


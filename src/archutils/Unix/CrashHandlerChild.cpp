#define __USE_GNU
#include "global.h"

#include "Backtrace.h"
#include "BacktraceNames.h"

#include "RageUtil.h"
#include "CrashHandler.h"
#include "CrashHandlerInternal.h"
#include "RageLog.h" /* for RageLog::GetAdditionalLog, etc. only */
#include "ProductInfo.h"
#include "arch/ArchHooks/ArchHooks.h"

#if defined(MACOSX)
#include "archutils/Darwin/Crash.h"
#endif

#include "ver.h"

#include <cstdio>
#include <cstring>
#include <cerrno>
#include <vector>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>

bool child_read( int fd, void *p, int size );

const char *g_pCrashHandlerArgv0 = nullptr;


static void output_stack_trace( FILE *out, const void **BacktracePointers )
{
	if( BacktracePointers[0] == BACKTRACE_METHOD_NOT_AVAILABLE )
	{
		fprintf( out, "No backtrace method available.\n");
		return;
	}

	if( !BacktracePointers[0] )
	{
		fprintf( out, "Backtrace was empty.\n");
		return;
	}

	for( int i = 0; BacktracePointers[i]; ++i)
	{
		BacktraceNames bn;
		bn.FromAddr( const_cast<void *>(BacktracePointers[i]) );
		bn.Demangle();

		/* Don't show the main module name. */
		if( bn.File == g_pCrashHandlerArgv0 && !bn.Symbol.empty() )
			bn.File = "";

		if( bn.Symbol == "__libc_start_main" )
			break;

		fprintf( out, "%s\n", bn.Format().c_str() );
	}
}

bool child_read( int fd, void *p, int size )
{
	char *buf = (char *) p;
	int got = 0;
	while( got < size )
	{
		int ret = read( fd, buf+got, size-got );
		if( ret == -1 )
		{
			if( errno == EINTR )
				continue;
			fprintf( stderr, "Crash handler: error communicating with parent: %s\n", strerror(errno) );
			return false;
		}

		if( ret == 0 )
		{
			fprintf( stderr, "Crash handler: EOF communicating with parent.\n" );
			return false;
		}

		got += ret;
	}

	return true;
}

/* Once we get here, we should be * safe to do whatever we want;
* heavyweights like malloc and RString are OK. (Don't crash!) */
static void child_process()
{
	/* 1. Read the CrashData. */
	CrashData crash;
	if( !child_read(3, &crash, sizeof(CrashData)) )
		return;

	/* 2. Read info. */
	int size;
	if( !child_read(3, &size, sizeof(size)) )
		return;
	char *Info = new char [size];
	if( !child_read(3, Info, size) )
		return;

	/* 3. Read AdditionalLog. */
	if( !child_read(3, &size, sizeof(size)) )
		return;

	char *AdditionalLog = new char [size];
	if( !child_read(3, AdditionalLog, size) )
		return;

	/* 4. Read RecentLogs. */
	int cnt = 0;
	if( !child_read(3, &cnt, sizeof(cnt)) )
		return;
	char *Recent[1024];
	for( int i = 0; i < cnt; ++i )
	{
		if( !child_read(3, &size, sizeof(size)) )
			return;
		Recent[i] = new char [size];
		if( !child_read(3, Recent[i], size) )
			return;
	}

	/* 5. Read CHECKPOINTs. */
	if( !child_read(3, &size, sizeof(size)) )
		return;

	char *temp = new char [size];
	if( !child_read(3, temp, size) )
		return;

	std::vector<RString> Checkpoints;
	split(temp, "$$", Checkpoints);
	delete [] temp;

	/* 6. Read the crashed thread's name. */
	if( !child_read(3, &size, sizeof(size)) )
		return;
	temp = new char [size];
	if( !child_read(3, temp, size) )
		return;
	const RString CrashedThread(temp);
	delete[] temp;

	/* Wait for the child to either finish cleaning up or die. */
	fd_set rs;
	struct timeval timeout = { 5, 0 }; // 5 seconds

	FD_ZERO( &rs );
	FD_SET( 3, &rs );
	int ret = select( 4, &rs, nullptr, nullptr, &timeout );

	if( ret == 0 )
	{
		fputs( "Timeout exceeded.\n", stderr );
	}
	else if( (ret == -1 && errno != EPIPE) || ret != 1 )
	{
		fprintf( stderr, "Unexpected return from select() result: %d (%s)\n", ret, strerror(errno) );
		// Keep going.
	}
	else
	{
		char x;

		// No need to check FD_ISSET( 3, &rs ) because it was the only descriptor in the set.
		ret = read( 3, &x, sizeof(x) );
		if( ret > 0 )
		{
			fprintf( stderr, "Unexpected child read() result: %i\n", ret );
			/* keep going */
		}
		else if( (ret == -1 && errno != EPIPE) || ret != 0 )
		{
			/* We expect an EOF or EPIPE.  What happened? */
			fprintf( stderr, "Unexpected child read() result: %i (%s)\n", ret, strerror(errno) );
			/* keep going */
		}
	}

	RString sCrashInfoPath = "/tmp";
#if defined(MACOSX)
	sCrashInfoPath = CrashHandler::GetLogsDirectory();
#else
	const char *home = getenv( "HOME" );
	if( home )
		sCrashInfoPath = home;
#endif
	sCrashInfoPath += "/crashinfo.txt";

	FILE *CrashDump = fopen( sCrashInfoPath.c_str(), "w+" );
	if(CrashDump == nullptr)
	{
		fprintf( stderr, "Couldn't open %s: %s\n", sCrashInfoPath.c_str(), strerror(errno) );
		exit(1);
	}

	fprintf( CrashDump, "%s%s crash report", PRODUCT_FAMILY, product_version );
	fprintf( CrashDump, " (build %s, %s @ %s)", ::sm_version_git_hash, version_date, version_time );
	fprintf( CrashDump, "\n" );
	fprintf( CrashDump, "--------------------------------------\n" );
	fprintf( CrashDump, "\n" );

	RString reason;
	switch( crash.type )
	{
	case CrashData::SIGNAL:
	{
		reason = ssprintf( "%s - %s", SignalName(crash.signal), SignalCodeName(crash.signal, crash.si.si_code) );

		/* Linux puts the PID that sent the signal in si_addr for SI_USER. */
		if( crash.si.si_code == SI_USER )
		{
			reason += ssprintf( " from pid %li", (long) crash.si.si_addr );
		}
		else
		{
			switch( crash.signal )
			{
			case SIGILL:
			case SIGFPE:
			case SIGSEGV:
			case SIGBUS:
				reason += ssprintf( " at 0x%0*lx", int(sizeof(void*)*2), (unsigned long) crash.si.si_addr );
			}
			break;
		}
		[[fallthrough]];
	}
	case CrashData::FORCE_CRASH:
		crash.reason[sizeof(crash.reason)-1] = 0;
		reason = crash.reason;
		break;
	}

	fprintf( CrashDump, "Architecture:   %s\n", HOOKS->GetArchName().c_str() );
	fprintf( CrashDump, "Crash reason:   %s\n", reason.c_str() );
	fprintf( CrashDump, "Crashed thread: %s\n\n", CrashedThread.c_str() );

	fprintf(CrashDump, "Checkpoints:\n");
	for( unsigned i=0; i<Checkpoints.size(); ++i )
		fputs( Checkpoints[i].c_str(), CrashDump );
	fprintf( CrashDump, "\n" );

	for( int i = 0; i < CrashData::MAX_BACKTRACE_THREADS; ++i )
	{
		if( !crash.BacktracePointers[i][0] )
			break;
		fprintf( CrashDump, "Thread: %s\n", crash.m_ThreadName[i] );
		output_stack_trace( CrashDump, crash.BacktracePointers[i] );
		fprintf( CrashDump, "\n" );
	}

	fprintf( CrashDump, "Static log:\n" );
	fprintf( CrashDump, "%s", Info );
	fprintf( CrashDump, "%s", AdditionalLog );
	fprintf(CrashDump, "\nPartial log:\n" );
	for( int i = 0; i < cnt; ++i )
		fprintf( CrashDump, "%s\n", Recent[i] );
	fprintf( CrashDump, "\n" );
	fprintf( CrashDump, "-- End of report\n" );
	fclose( CrashDump) ;

#if defined(MACOSX)
	CrashHandler::InformUserOfCrash( sCrashInfoPath );
#else
	/* stdout may have been inadvertently closed by the crash in the parent;
	 * write to /dev/tty instead. */
	FILE *tty = fopen( "/dev/tty", "w" );
	if( tty == nullptr )
		tty = stderr;

	fputs(( 	"\n"
		 PRODUCT_ID " has crashed.  Debug information has been output to\n"
		 "\n"
		 "    " + sCrashInfoPath + "\n"
		 "\n"
		 "Please report a bug at:\n"
		 "\n"
		 "    " REPORT_BUG_URL "\n"
		 "\n").c_str(), tty );
#endif
}


void CrashHandler::CrashHandlerHandleArgs( int argc, char* argv[] )
{
	g_pCrashHandlerArgv0 = argv[0];
	if( argc == 2 && !strcmp(argv[1], CHILD_MAGIC_PARAMETER) )
	{
		child_process();
		exit(0);
	}
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

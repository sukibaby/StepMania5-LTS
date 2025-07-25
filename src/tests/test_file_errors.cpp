#include "global.h"
#include "PrefsManager.h"
#include "RageLog.h"
#include "RageFile.h"
#include "RageFileDriver.h"
#include "RageUtil.h"
#include "RageUtil_FileDB.h"
#include "test_misc.h"

#include <cstddef>
#include <errno.h>

static RString g_TestFile;
static RString g_TestFilename;
static int g_BytesUntilError = 6;

/*
 * This driver
 *  1: reads from a single "file", contained in g_TestFile, with fn g_TestFilename;
 *  2: writes to the bit bucket,
 *  3: triggers an error when g_BytesUntilError reaches 0.
 */
class RageFileDriverTest: public RageFileDriver
{
public:
	RageFileDriverTest( RString root );

	RageFileObj *Open( const RString &path, int mode, int &err );
	bool Remove( const RString &sPath ) { return false; }
	bool Ready() { return true; }

private:
	RString root;
};

static struct FileDriverEntry_TEST: public FileDriverEntry
{
	FileDriverEntry_TEST(): FileDriverEntry( "TEST" ) { }
	RageFileDriver *Create( const RString &sRoot ) const { return new RageFileDriverTest( sRoot ); }
} const g_RegisterDriver;

class TestFilenameDB: public FilenameDB
{
protected:
	virtual void PopulateFileSet( FileSet &fs, const RString &sPath )
	{
		if( sPath != "." )
			return;
		if( g_TestFile == "" )
			return;

		fs.files.clear();

		File f;
		f.SetName( g_TestFilename );
		f.dir = false;
		f.size = g_TestFile.size();
		f.hash = GetHashForString( g_TestFile );
		fs.files.insert(f);
	}

	RString root;

public:
	TestFilenameDB( RString root_ )
	{
		root = root_;
		if( root == "./" )
			root = "";
	}

};

class RageFileObjTest: public RageFileObj
{
public:
	RageFileObjTest( const RString &path );
	RageFileObjTest( const RageFileObjTest &cpy );
	int ReadInternal(void *buffer, size_t bytes);
	int WriteInternal(const void *buffer, size_t bytes);
	int Flush();
	void Rewind() { pos = 0; }
	int Seek( int offset )
	{
		pos = std::min( offset, (int) g_TestFile.size() );
		return pos;
	}
	RageFileObj *Copy() const;
	RString GetDisplayPath() const { return path; }
	int GetFileSize() const { return g_TestFile.size(); }

private:
	RString path; /* for Copy */
	int pos;
};


RageFileDriverTest::RageFileDriverTest( RString root_ ):
	RageFileDriver( new TestFilenameDB(root_) ),
	root(root_)
{
	if( Right(root, 1) != "/" )
		root += '/';
}

RageFileObjTest::RageFileObjTest( const RageFileObjTest &cpy ):
	RageFileObj(cpy)
{
	path = cpy.path;
	pos = cpy.pos;
}

RageFileObj *RageFileDriverTest::Open( const RString &path, int mode, int &err )
{
	if( path != g_TestFilename )
	{
		err = ENOENT;
		return NULL;
	}

	return new RageFileObjTest( root + path );
}

RageFileObj *RageFileObjTest::Copy() const
{
	return new RageFileObjTest( *this );
}

static const unsigned int BUFSIZE = 1024*64;
RageFileObjTest::RageFileObjTest( const RString &path_ )
{
	path = path_;
	pos = 0;
}

int RageFileObjTest::ReadInternal( void *buf, size_t bytes )
{
	bytes = std::min( bytes, g_TestFile.size()-pos );

	if( g_BytesUntilError != -1 )
	{
		if( (int) bytes > g_BytesUntilError )
		{
			SetError( "Fake error" );
			g_BytesUntilError = -1;
			return -1;
		}

		g_BytesUntilError -= std::min( g_BytesUntilError, (int) bytes );
	}

	memcpy( buf, g_TestFile.data()+pos, bytes );
	pos += bytes;
	return bytes;
}

int RageFileObjTest::WriteInternal( const void *buf, size_t bytes )
{
	if( g_BytesUntilError != -1 )
	{
		if( (int) bytes > g_BytesUntilError )
		{
			SetError( "Fake error" );
			g_BytesUntilError = -1;
			return -1;
		}

		g_BytesUntilError -= std::min( g_BytesUntilError, (int) bytes );
	}

	return bytes;
}

/*
 * Testing Flush() is important.  Writing may buffer.  That buffer will be
 * flushed, at the latest, when the file is closed in the RageFile dtor.  However,
 * if you wait until then, there's no way to check for an error; the file is
 * going away.  The primary purpose of Flush() is to give the user a chance to
 * check for these errors.  (Flush() is only intended to flush our buffers;
 * it is *not* intended to sync data to disk.)
 */
int RageFileObjTest::Flush()
{
	if( g_BytesUntilError != -1 )
	{
		SetError( "Fake error" );
		g_BytesUntilError = -1;
		return -1;
	}

	return 0;
}

#define Fail(s...) \
	{ \
		LOG->Warn( s ); break; \
	}

void SanityCheck()
{
	/* Read sanity check. */
	do {
		g_TestFile = "hello";
		g_TestFilename = "file";

		RageFile test;
		if( !test.Open("/test/file", RageFile::READ ) )
			Fail( "Sanity check Open() failed: %s", test.GetError().c_str() );

		RString str;
		int got = test.GetLine( str );
		if( got <= 0 )
			Fail( "Sanity check GetLine(): got %i", got );

		if( str != "hello" )
			Fail( "Sanity check Read(): expected \"hello\", got \"%s\"", str.c_str() );
	} while(false);

	/* Read error sanity check. */
	do {
		g_TestFile = "hello world";
		g_TestFilename = "file";
		g_BytesUntilError = 5;

		RageFile test;
		if( !test.Open("/test/file", RageFile::READ ) )
			Fail( "Sanity check 2 Open() failed: %s", test.GetError().c_str() );

		RString str;
		int got = test.Read( str, 5 );
		if( got != 5 )
			Fail( "Sanity check 2 Read(): got %i", got );

		if( str != "hello" )
			Fail( "Sanity check 2 Read(): expected \"hello\", got \"%s\"", str.c_str() );

		got = test.Read( str, 5 );
		if( got != -1 )
			Fail( "Sanity check 2 GetLine(): expected -1, got %i", got );

		if( test.GetError() != "Fake error" )
			Fail( "Sanity check 2 GetError(): expected \"Fake error\", got \"%s\"", test.GetError().c_str() );
	} while(false);

	/* Write error sanity check. */
	do {
		g_TestFilename = "file";
		g_BytesUntilError = 5;

		RageFile test;
		if( !test.Open("/test/file", RageFile::WRITE ) )
			Fail( "Write error check Open() failed: %s", test.GetError().c_str() );

		int wrote = test.Write( "test", 4 );
		if( wrote != 4 )
			Fail( "Write error check Write(): wrote %i", wrote );

		wrote = test.Write( "ing", 3 );
		if( wrote != -1 )
			Fail( "Write error check Write(): expected -1, got %i", wrote );

		if( test.GetError() != "Fake error" )
			Fail( "Write error check GetError(): expected \"Fake error\", got \"%s\"", test.GetError().c_str() );
	} while(false);
}

#include "IniFile.h"
void IniTest()
{
	g_TestFilename = "file";

	/* Read check. */
	do {
		g_TestFile =
			"[test]\n"
			"abc=def";

		g_BytesUntilError = -1;

		IniFile test;
		if( !test.ReadFile( "test/file" ) )
			Fail( "INI: ReadFile failed: %s", test.GetError().c_str() );

		RString sStr;
		if( !test.GetValue( "test", "abc", sStr ) )
			Fail( "INI: GetValue failed" );
		if( sStr != "def" )
			Fail( "INI: GetValue failed: expected \"def\", got \"%s\"", sStr.c_str() );
	} while(false);

	/* Read error check. */
	do {
		g_TestFile =
			"[test]\n"
			"abc=def";

		g_BytesUntilError = 5;

		IniFile test;
		if( test.ReadFile( "test/file" ) )
			Fail( "INI: ReadFile should have failed" );

		if( test.GetError() != "Fake error" )
			Fail( "INI: ReadFile error check: wrong error return: got \"%s\"", test.GetError().c_str() );
	} while(false);

	/* Write error check. */
	do {
		g_BytesUntilError = 5;

		IniFile test;
		test.SetValue( "foo", "bar", RString("baz") );
		if( test.WriteFile( "test/file" ) )
			Fail( "INI: WriteFile should have failed" );

		if( test.GetError() != "Fake error" )
			Fail( "INI: ReadFile error check: wrong error return: got \"%s\"", test.GetError().c_str() );
	} while(false);
}

#include "MsdFile.h"
void MsdTest()
{
	g_TestFilename = "file";

	/* Read check. */
	do {
		g_TestFile = "#FOO;";

		g_BytesUntilError = -1;

		MsdFile test;
		if( !test.ReadFile("test/file", false) )
			Fail( "MSD: ReadFile failed: %s", test.GetError().c_str() );

		if( test.GetNumValues() != 1 )
			Fail( "MSD: GetNumValues: expected 1, got %i", test.GetNumValues() );
		if( test.GetNumParams(0) != 1 )
			Fail( "MSD: GetNumParams(0): expected 1, got %i", test.GetNumParams(0) );
		RString sStr = test.GetValue(0)[0];
		if( sStr != "FOO" )
			Fail( "MSD: GetValue failed: expected \"FOO\", got \"%s\"", sStr.c_str() );
	} while(false);

	/* Read error check. */
	do {
		g_TestFile = "#FOO:BAR:BAZ;";
		g_BytesUntilError = 5;

		MsdFile test;
		if( test.ReadFile("test/file", false) )
			Fail( "MSD: ReadFile should have failed" );

		if( test.GetError() != "Fake error" )
			Fail( "MSD: ReadFile error check: wrong error return: got \"%s\"", test.GetError().c_str() );
	} while(false);
}

#include "CryptManager.h"
void CryptoTest()
{
	PREFSMAN = new PrefsManager; /* CRYPTMAN needs PREFSMAN */
	PREFSMAN->m_bSignProfileData.Set( true );

	CRYPTMAN = new CryptManager;

	g_TestFile = "hello world";
	g_TestFilename = "file";
	FlushDirCache();

	do {
		g_BytesUntilError = -1;
		CRYPTMAN->SignFileToFile( "test/file", "output" );

		FlushDirCache();

		if( !CRYPTMAN->VerifyFileWithFile( "test/file", "output" ) )
			Fail( "Crypto: VerifyFileWithFile failed");
	} while(false);


	/* Write error check. */
	do {
		g_BytesUntilError = 5;
		CRYPTMAN->SignFileToFile( "test/file", "output" );
	} while(false);

	/* Read error check. */
	do {
		g_BytesUntilError = 5;
		CRYPTMAN->SignFileToFile( "output", "test/file" );
	} while(false);

	delete PREFSMAN;
	delete CRYPTMAN;
}

int main( int argc, char *argv[] )
{
	test_handle_args( argc, argv );

	test_init();

	/* Setup. */
	FILEMAN->Mount( "TEST", ".", "/test" );

	SanityCheck();

	IniTest();
	MsdTest();
	CryptoTest();

	test_deinit();

	exit(0);
}

/*
 * Copyright (c) 2004 Glenn Maynard
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


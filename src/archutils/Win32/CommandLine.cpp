#include "global.h"
#include "CommandLine.h"
#include <windows.h>
#include <shellapi.h>
#include <vector>
#include <string>

/* Use CommandLineToArgvW to parse the command line arguments. */
int GetWin32CmdLine( char** &argv )
{
	LPWSTR* argvW = nullptr;
	int argc = 0;
	argvW = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (argvW == nullptr)
	{
		argv = nullptr;
		return -1;
	}

	std::vector<std::string> args;
	for (int i = 0; i < argc; ++i)
	{
		int size_needed = WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, nullptr, 0, nullptr, nullptr);
		std::string arg(size_needed, 0);
		WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, &arg[0], size_needed, nullptr, nullptr);
		args.push_back(arg);
	}

	LocalFree(argvW);

	argv = new char* [args.size()];
	for (size_t i = 0; i < args.size(); ++i)
	{
		argv[i] = new char[args[i].size() + 1];
		strcpy(argv[i], args[i].c_str());
	}

	return argc;
}

/*
 * (c) 2006 Chris Danford
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

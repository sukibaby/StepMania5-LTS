#ifndef ARCH_HOOKS_WIN32_H
#define ARCH_HOOKS_WIN32_H

#include "ArchHooks.h"

#include <cstdint>

class RageMutex;

class ArchHooks_Win32: public ArchHooks
{
public:
	ArchHooks_Win32();
	~ArchHooks_Win32();
	RString GetArchName() const { return "Windows"; }
	void DumpDebugInfo();
	void RestartProgram();
	bool CheckForMultipleInstances(int argc, char* argv[]);

	void SetTime( tm newtime );

	void BoostPriority();
	void UnBoostPriority();
	void SetupConcurrentRenderingThread();

	uint32_t DetermineSampleRate() const override;

	virtual float GetDisplayAspectRatio();
	RString GetClipboard();
};

#ifdef ARCH_HOOKS
#error "More than one ArchHooks selected!"
#endif
#define ARCH_HOOKS ArchHooks_Win32

#endif
/*
 * (c) 2002-2004 Glenn Maynard, Chris Danford
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

#ifndef DARWIN_THREAD_HELPERS_H
#define DARWIN_THREAD_HELPERS_H

#include <cstdint>
#include <string>

/**
 * @brief Attempt to suspend the specified thread.
 * @param threadHandle the thread to suspend.
 * @return true if the thread is suspended, false otherwise. */
bool SuspendThread( uint64_t threadHandle );
/**
 * @brief Attempt to resume the specified thread.
 * @param threadHandle the thread to resume.
 * @return true if the thread is resumed, false otherwise. */
bool ResumeThread( uint64_t threadHandle );
/**
 * @brief Retrieve the current thread ID.
 * @return the current thread ID. */
uint64_t GetCurrentThreadId();
/**
 * @brief Set the precedence for the thread.
 *
 * Valid values for the thread are from 0.0f to 1.0f.
 * 0.5f is the default.
 * @param prec the precedence to set. */
std::string SetThreadPrecedence( float prec );

#endif

/**
 * @file
 * @author Steve Checkoway (c) 2004-2006
 * @section LICENSE
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

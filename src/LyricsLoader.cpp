#include "global.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "LyricsLoader.h"
#include "ThemeManager.h"
#include "RageFile.h"
#include "Song.h"

#include <map>
#include <vector>


// TODO: Use a marker for default color instead of a specific color that may
// accidentally get written back into a lyrics file.
#define LYRICS_DEFAULT_COLOR	THEME->GetMetricC("ScreenGameplay","LyricDisplayDefaultColor")

static int CompareLyricSegments(const LyricSegment &seg1, const LyricSegment &seg2)
{
   return seg1.m_fStartTime < seg2.m_fStartTime;
}

bool LyricsLoader::LoadFromLRCFile(const RString& sPath, Song& out)
{
	LOG->Trace( "LyricsLoader::LoadFromLRCFile(%s)", sPath.c_str() );

	RageFile input;
	if( !input.Open(sPath) )
	{
		LuaHelpers::ReportScriptErrorFmt("Error opening file '%s' for reading: %s", sPath.c_str(), input.GetError().c_str() );
		return false;
	}

	RageColor CurrentColor = LYRICS_DEFAULT_COLOR;

	out.m_LyricSegments.clear();

	for(;;)
	{
		RString line;
		int ret = input.GetLine( line );
		if( ret == 0 )
		{
			break;
		}
		if( ret == -1 )
		{
			LuaHelpers::ReportScriptErrorFmt("Error reading %s: %s", input.GetPath().c_str(), input.GetError().c_str() );
			break;
		}

		utf8_remove_bom( line );

		if(!line.compare(0, 2, "//"))
		{
			continue;
		}
		// (most tags are in the format of...)
		// "[data1] data2".  Ignore whitespace at the beginning of the line.
		static Regex x("^ *\\[([^]]+)\\] *(.*)$");

		std::vector<RString> matches;
		if(!x.Compare(line, matches))
		{
			continue;
		}
		ASSERT( matches.size() == 2 );

		RString &sValueName = matches[0];
		RString &sValueData = matches[1];
		StripCrnl(sValueData);

		// handle the data
		if( EqualsNoCase(sValueName, "COLOUR") || EqualsNoCase(sValueName, "COLOR") )
		{
			// set color var here for this segment
			unsigned int r, g, b;
			int result = sscanf( sValueData.c_str(), "0x%2x%2x%2x", &r, &g, &b );
			// According to the Dance With Intensity readme, one can set up to
			// ten colors in a line and access them via "{cX}", where X is 0-9.
			if(result != 3)
			{
				LOG->Trace( "The color value '%s' in '%s' is invalid.",
				sValueData.c_str(), sPath.c_str() );
				continue;
			}

			CurrentColor = RageColor(r / 256.0f, g / 256.0f, b / 256.0f, 1);
			continue;
		}

		// todo: handle [offset:xxxx] tag? -aj (xxxx is milliseconds)
		// offsets each timestamp after the offset by that amount.
		//float fLyricOffset = 0.0f;

		// Enforce strict timestamp format to prevent crashing the program.
		std::vector<RString> dummy;
		static Regex timestamp("^([0-9]+:){0,2}[0-9]+(.[0-9]*)?$");
		if (timestamp.Compare(sValueName, dummy))
		{
			LyricSegment seg;
			seg.m_Color = CurrentColor;
			seg.m_fStartTime = HHMMSSToSeconds(sValueName);
			seg.m_sLyric = sValueData;

			Replace(seg.m_sLyric, "|", "\n"); // Pipe symbols denote a new line in LRC files
			out.AddLyricSegment( seg );
		}
	}

	sort( out.m_LyricSegments.begin(), out.m_LyricSegments.end(), CompareLyricSegments );
	LOG->Trace( "LyricsLoader::LoadFromLRCFile done" );

	return true;
}

/*
 * (c) 2003 Kevin Slaughter, Glenn Maynard
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

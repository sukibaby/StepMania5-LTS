#include "global.h"
#include "Font.h"
#include "IniFile.h"

#include "RageTextureManager.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "FontManager.h"
#include "ThemeManager.h"
#include "FontCharmaps.h"
#include "FontCharAliases.h"
#include "arch/Dialog/Dialog.h"

#include <cmath>
#include <cstddef>
#include <vector>


FontPage::FontPage(): m_iHeight(0), m_iLineSpacing(0), m_fVshift(0),
	m_iDrawExtraPixelsLeft(0), m_iDrawExtraPixelsRight(0),
	m_FontPageTextures(), m_sTexturePath(""), m_aGlyphs(),
	m_iCharToGlyphNo() {}

void FontPage::Load( const FontPageSettings &cfg )
{
	m_sTexturePath = cfg.m_sTexturePath;

	// load texture
	RageTextureID ID1( m_sTexturePath );
	if( cfg.m_sTextureHints != "default" )
		ID1.AdditionalTextureHints = cfg.m_sTextureHints;

	m_FontPageTextures.m_pTextureMain = TEXTUREMAN->LoadTexture( ID1 );
	if(m_FontPageTextures.m_pTextureMain == nullptr)
	{
		LuaHelpers::ReportScriptErrorFmt(
			"Failed to load main texture %s for font page.",
			m_sTexturePath.c_str());
		m_FontPageTextures.m_pTextureMain= TEXTUREMAN->LoadTexture(
			TEXTUREMAN->GetDefaultTextureID());
	}

	RageTextureID ID2 = ID1;
	// "arial 20 16x16 [main].png" => "arial 20 16x16 [main-stroke].png"
	if( ID2.filename.find("]") != std::string::npos )
	{
		Replace(ID2.filename, "]", "-stroke]");
		if( IsAFile(ID2.filename) )
		{
			m_FontPageTextures.m_pTextureStroke = TEXTUREMAN->LoadTexture( ID2 );
			if(m_FontPageTextures.m_pTextureStroke == nullptr)
			{
				LuaHelpers::ReportScriptErrorFmt(
					"Failed to load stroke texture %s for font page.",
					ID2.filename.c_str());
				m_FontPageTextures.m_pTextureStroke=
					m_FontPageTextures.m_pTextureMain;
			}
			// If the source frame dimensions or number of frames don't match, set
			// the stroke layer to be the same as the main layer so that the
			// assumptions based on the frames are still safe. -Kyz
			if(m_FontPageTextures.m_pTextureMain->GetSourceFrameWidth() !=
				m_FontPageTextures.m_pTextureStroke->GetSourceFrameWidth())
			{
				LuaHelpers::ReportScriptErrorFmt(
					"'%s' and '%s' must have the same frame widths",
					ID1.filename.c_str(), ID2.filename.c_str());
				m_FontPageTextures.m_pTextureStroke=
					m_FontPageTextures.m_pTextureMain;
			}
			if(m_FontPageTextures.m_pTextureMain->GetNumFrames() !=
				m_FontPageTextures.m_pTextureStroke->GetNumFrames())
			{
				LuaHelpers::ReportScriptErrorFmt(
				"'%s' and '%s' must have the same frame dimensions",
				ID1.filename.c_str(), ID2.filename.c_str());
				m_FontPageTextures.m_pTextureStroke=
					m_FontPageTextures.m_pTextureMain;
			}
		}
	}

	// load character widths
	std::vector<int> aiFrameWidths;

	int default_width = m_FontPageTextures.m_pTextureMain->GetSourceFrameWidth();
	if( cfg.m_iDefaultWidth != -1 )
		default_width = cfg.m_iDefaultWidth;

	// Assume each character is the width of the frame by default.
	for( int i=0; i<m_FontPageTextures.m_pTextureMain->GetNumFrames(); i++ )
	{
		std::map<int, int>::const_iterator it = cfg.m_mapGlyphWidths.find(i);
		if( it != cfg.m_mapGlyphWidths.end() )
			aiFrameWidths.push_back( it->second );
		else
			aiFrameWidths.push_back( default_width );
	}

	if( cfg.m_iAddToAllWidths )
	{
		for( int i=0; i<m_FontPageTextures.m_pTextureMain->GetNumFrames(); i++ )
			aiFrameWidths[i] += cfg.m_iAddToAllWidths;
	}

	if( cfg.m_fScaleAllWidthsBy != 1 )
	{
		for( int i=0; i<m_FontPageTextures.m_pTextureMain->GetNumFrames(); i++ )
			aiFrameWidths[i] = std::lrint( aiFrameWidths[i] * cfg.m_fScaleAllWidthsBy );
	}

	m_iCharToGlyphNo = cfg.CharToGlyphNo;

	m_iLineSpacing = cfg.m_iLineSpacing;
	if( m_iLineSpacing == -1 )
		m_iLineSpacing = m_FontPageTextures.m_pTextureMain->GetSourceFrameHeight();

	int iBaseline=0;
	/* If we don't have a top and/or baseline, assume we're centered in the
	 * frame, and that LineSpacing is the total height. */
	iBaseline = cfg.m_iBaseline;
	if( iBaseline == -1 )
	{
		float center = m_FontPageTextures.m_pTextureMain->GetSourceFrameHeight()/2.0f;
		iBaseline = int( center + m_iLineSpacing/2 );
	}

	int iTop = cfg.m_iTop;
	if( iTop == -1 )
	{
		float center = m_FontPageTextures.m_pTextureMain->GetSourceFrameHeight()/2.0f;
		iTop = int( center - m_iLineSpacing/2 );
	}
	m_iHeight = iBaseline - iTop;
	m_iDrawExtraPixelsLeft = cfg.m_iDrawExtraPixelsLeft;
	m_iDrawExtraPixelsRight = cfg.m_iDrawExtraPixelsRight;

	// Shift the character up so the top will be rendered at the baseline.
	m_fVshift = (float) -iBaseline;

	SetTextureCoords( aiFrameWidths, cfg.m_iAdvanceExtraPixels );
	SetExtraPixels( cfg.m_iDrawExtraPixelsLeft, cfg.m_iDrawExtraPixelsRight );

//	LOG->Trace("Font %s: height %i, baseline %i ( == top %i)",
//		   m_sTexturePath.c_str(), height, baseline, baseline-height);
}

void FontPage::SetTextureCoords( const std::vector<int> &widths, int iAdvanceExtraPixels )
{
	for(int i = 0; i < m_FontPageTextures.m_pTextureMain->GetNumFrames(); ++i)
	{
		glyph g;

		g.m_pPage = this;

		/* Make a copy of each texture rect, reducing each to the actual dimensions
		 * of the character (most characters don't take a full block). */
		g.m_TexRect = *m_FontPageTextures.m_pTextureMain->GetTextureCoordRect(i);

		// Set the width and height to the width and line spacing, respectively.
		g.m_fWidth = float( widths[i] );
		g.m_fHeight = float(m_FontPageTextures.m_pTextureMain->GetSourceFrameHeight());

		g.m_iHadvance = int(g.m_fWidth) + iAdvanceExtraPixels;

		/* Do the same thing with X.  Do this by changing the actual rendered
		 * m_TexRect, instead of shifting it, so we don't render more than we
		 * need to. */
		g.m_fHshift = 0;
		{
			int iSourcePixelsToChopOff = m_FontPageTextures.m_pTextureMain->GetSourceFrameWidth() - widths[i];
			if( (iSourcePixelsToChopOff % 2) == 1 )
			{
				/* We don't want to chop off an odd number of pixels, since that'll
				 * put our texture coordinates between texels and make things blurrier.
				 * Note that, since we set m_iHadvance above, this merely expands what
				 * we render; it doesn't advance the cursor further.  So, glyphs
				 * that have an odd width should err to being a pixel offcenter left,
				 * not right. */
				--iSourcePixelsToChopOff;
				++g.m_fWidth;
			}

			const float fTexCoordsToChopOff = iSourcePixelsToChopOff * m_FontPageTextures.m_pTextureMain->GetSourceToTexCoordsRatioX();

			g.m_TexRect.left  += fTexCoordsToChopOff/2;
			g.m_TexRect.right -= fTexCoordsToChopOff/2;
		}

		g.m_FontPageTextures = m_FontPageTextures;

		m_aGlyphs.push_back(g);
	}
}

void FontPage::SetExtraPixels( int iDrawExtraPixelsLeft, int iDrawExtraPixelsRight )
{
	// Most fonts don't take the stroke into account, so if it shows up, it'll
	// be cut off. I now understand why this code was here before. -freem
	iDrawExtraPixelsRight++;
	iDrawExtraPixelsLeft++;

	if( (iDrawExtraPixelsLeft % 2) == 1 )
		++iDrawExtraPixelsLeft;

	// Adjust for iDrawExtraPixelsLeft and iDrawExtraPixelsRight.
	for( unsigned i = 0; i < m_aGlyphs.size(); ++i )
	{
		int iFrameWidth = m_FontPageTextures.m_pTextureMain->GetSourceFrameWidth();
		float fCharWidth = m_aGlyphs[i].m_fWidth;

		/* Extra pixels to draw to the left and right.  We don't have to
		 * worry about alignment here; fCharWidth is always even (by
		 * SetTextureCoords) and iFrameWidth are almost always even. */
		float fExtraLeft = std::min( float(iDrawExtraPixelsLeft), (iFrameWidth-fCharWidth)/2.0f );
		float fExtraRight = std::min( float(iDrawExtraPixelsRight), (iFrameWidth-fCharWidth)/2.0f );

		// Move left and expand right.
		m_aGlyphs[i].m_TexRect.left -= fExtraLeft * m_FontPageTextures.m_pTextureMain->GetSourceToTexCoordsRatioX();
		m_aGlyphs[i].m_TexRect.right += fExtraRight * m_FontPageTextures.m_pTextureMain->GetSourceToTexCoordsRatioX();
		m_aGlyphs[i].m_fHshift -= fExtraLeft;
		m_aGlyphs[i].m_fWidth += fExtraLeft + fExtraRight;
	}
}

FontPage::~FontPage()
{
	if( m_FontPageTextures.m_pTextureMain != nullptr )
	{
		TEXTUREMAN->UnloadTexture( m_FontPageTextures.m_pTextureMain );
		m_FontPageTextures.m_pTextureMain = nullptr;
	}
	if( m_FontPageTextures.m_pTextureStroke != nullptr )
	{
		TEXTUREMAN->UnloadTexture( m_FontPageTextures.m_pTextureStroke );
		m_FontPageTextures.m_pTextureStroke = nullptr;
	}
}

int Font::GetLineWidthInSourcePixels( const std::wstring &szLine ) const
{
	int iLineWidth = 0;

	for( unsigned i=0; i<szLine.size(); i++ )
		iLineWidth += GetGlyph(szLine[i]).m_iHadvance;

	return iLineWidth;
}

int Font::GetLineHeightInSourcePixels( const std::wstring &szLine ) const
{
	int iLineHeight = 0;

	// The height of a line is the height of its tallest used font page.
	for( unsigned i=0; i<szLine.size(); i++ )
		iLineHeight = std::max( iLineHeight, GetGlyph(szLine[i]).m_pPage->m_iHeight );

	return iLineHeight;
}

// width is a pointer so that we can return the used width through it.
size_t Font::GetGlyphsThatFit(const std::wstring& line, int* width) const
{
	if(*width == 0)
	{
		*width= GetLineWidthInSourcePixels(line);
		return line.size();
	}
	int curr_width= 0;
	size_t i= 0;
	for(i= 0; i < line.size() && curr_width < *width; ++i)
	{
		curr_width+= GetGlyph(line[i]).m_iHadvance;
	}
	*width= curr_width;
	return i;
}

Font::Font(): m_iRefCount(1), path(""), m_apPages(), m_pDefault(nullptr),
	m_iCharToGlyph(), m_bRightToLeft(false), m_bDistanceField(false),
	// strokes aren't shown by default, hence the Color.
	m_DefaultStrokeColor(RageColor(0,0,0,0)), m_sChars("") {}
Font::~Font()
{
	Unload();
}

void Font::Unload()
{
	//LOG->Trace("Font:Unload '%s'",path.c_str());
	for( unsigned i = 0; i < m_apPages.size(); ++i )
		delete m_apPages[i];
	m_apPages.clear();

	m_iCharToGlyph.clear();
	m_pDefault = nullptr;

	/* Don't clear the refcount. We've unloaded, but that doesn't mean things
	 * aren't still pointing to us. */
}

void Font::Reload()
{
	if(path.empty())
	{
		LuaHelpers::ReportScriptError("Cannot reload a font that has an empty path.");
		return;
	}
	Unload();
	Load( path, m_sChars );
}


void Font::AddPage( FontPage *m_pPage )
{
	m_apPages.push_back( m_pPage );

	for( std::map<wchar_t,int>::const_iterator it = m_pPage->m_iCharToGlyphNo.begin();
		it != m_pPage->m_iCharToGlyphNo.end(); ++it )
	{
		m_iCharToGlyph[it->first] = &m_pPage->m_aGlyphs[it->second];
	}
}

void Font::MergeFont(Font &f)
{
	/* If we don't have a font page yet, and f does, grab the default font
	 * page.  It'll usually be overridden later on by one of our own font
	 * pages; this will be used only if we don't have any font pages at
	 * all. */
	if( m_pDefault == nullptr )
		m_pDefault = f.m_pDefault;

	for(std::map<wchar_t,glyph*>::iterator it = f.m_iCharToGlyph.begin();
		it != f.m_iCharToGlyph.end(); ++it)
	{
		m_iCharToGlyph[it->first] = it->second;
	}

	m_apPages.insert( m_apPages.end(), f.m_apPages.begin(), f.m_apPages.end() );

	f.m_apPages.clear();
}

const glyph &Font::GetGlyph( wchar_t c ) const
{
	/* XXX: This is kind of nasty, but the parts that touch this are dark and
	 * scary. --Colby
	 *
	 * Snagged from OpenITG, original comment:
	 * shooting a blank really...DarkLink kept running into the stupid assert
	 * with non-roman song titles, and looking at it, I'm gonna guess that
	 * this is how ITG2 prevented crashing with them --infamouspat */
	//ASSERT(c >= 0 && c <= 0xFFFFFF);

	// wchar_t can be signed or unsigned depending on the platform and the compiler.
	// We use WCHAR_MIN to determine a valid condition that won't emit a type-limits warning.
	#if WCHAR_MIN != 0
	if (c < 0 || c > 0xFFFFFF) {
	#else
	if (c > 0xFFFFFF) {
	#endif
		c = 1;
	}

	// Fast path:
	if( c < (int) ARRAYLEN(m_iCharToGlyphCache) && m_iCharToGlyphCache[c] )
		return *m_iCharToGlyphCache[c];

	// Try the regular character.
	std::map<wchar_t, glyph*>::const_iterator it = m_iCharToGlyph.find(c);

	// If that's missing, use the default glyph.
	if(it == m_iCharToGlyph.end())
		it = m_iCharToGlyph.find(FONT_DEFAULT_GLYPH);

	if(it == m_iCharToGlyph.end())
		RageException::Throw( "The default glyph is missing from the font \"%s\".", path.c_str() );

	return *it->second;
}

bool Font::FontCompleteForString( const std::wstring &str ) const
{
	std::map<wchar_t, glyph*>::const_iterator mapDefault = m_iCharToGlyph.find( FONT_DEFAULT_GLYPH );
	if( mapDefault == m_iCharToGlyph.end() )
		RageException::Throw( "The default glyph is missing from the font \"%s\".", path.c_str() );

	for( unsigned i = 0; i < str.size(); ++i )
	{
		// If the glyph for this character is the default glyph, we're incomplete.
		const glyph &g = GetGlyph( str[i] );
		if( &g == mapDefault->second )
			return false;
	}
	return true;
}

void Font::CapsOnly()
{
	/* For each uppercase character that we have a mapping for, add
	 * a lowercase one. */
	for( char c = 'A'; c <= 'Z'; ++c )
	{
		std::map<wchar_t, glyph*>::const_iterator it = m_iCharToGlyph.find(c);

		if(it == m_iCharToGlyph.end())
			continue;

		m_iCharToGlyph[(char) tolower(c)] = it->second;
	}
}

void Font::SetDefaultGlyph( FontPage *pPage )
{
	ASSERT( pPage != nullptr );
	if(pPage->m_aGlyphs.empty())
	{
		LuaHelpers::ReportScriptErrorFmt(
			"Attempted to set default glyphs for %s to a blank page.",
			path.c_str());
		return;
	}
	m_pDefault = pPage;
}


// Given the INI for a font, find all of the texture pages for the font.
void Font::GetFontPaths( const RString &sFontIniPath, std::vector<RString> &asTexturePathsOut )
{
	RString sPrefix = SetExtension( sFontIniPath, "" );
	std::vector<RString> asFiles;
	GetDirListing( sPrefix + "*", asFiles, false, true );

	for( unsigned i = 0; i < asFiles.size(); ++i )
	{
		if( !EqualsNoCase(Right(asFiles[i], 4), ".ini") )
			asTexturePathsOut.push_back( asFiles[i] );
	}
}

RString Font::GetPageNameFromFileName( const RString &sFilename )
{
	size_t begin = sFilename.find_first_of( '[' );
	if( begin == std::string::npos )
		return "main";

	size_t end = sFilename.find_first_of( ']', begin );
	if( end == std::string::npos )
		return "main";

	begin++;
	end--;
	if( end == begin )
		return "main";
	return sFilename.substr( begin, end-begin+1 );
}

void Font::LoadFontPageSettings( FontPageSettings &cfg, IniFile &ini, const RString &sTexturePath, const RString &sPageName, RString sChars )
{
	cfg.m_sTexturePath = sTexturePath;

	// If we have any characters to map, add them.
	for( unsigned n=0; n<sChars.size(); n++ )
	{
		char c = sChars[n];
		cfg.CharToGlyphNo[c] = n;
	}
	int num_frames_wide, num_frames_high;
	RageTexture::GetFrameDimensionsFromFileName( sTexturePath, &num_frames_wide, &num_frames_high );
	int iNumFrames = num_frames_wide * num_frames_high;

	// Deal with fonts generated by Bitmap Font Builder.
	ini.RenameKey("Char Widths", "main");

//	LOG->Trace("Loading font page '%s' settings from page name '%s'",
//		TexturePath.c_str(), sPageName.c_str());

	ini.GetValue( sPageName, "DrawExtraPixelsLeft", cfg.m_iDrawExtraPixelsLeft );
	ini.GetValue( sPageName, "DrawExtraPixelsRight", cfg.m_iDrawExtraPixelsRight );
	ini.GetValue( sPageName, "AddToAllWidths", cfg.m_iAddToAllWidths );
	ini.GetValue( sPageName, "ScaleAllWidthsBy", cfg.m_fScaleAllWidthsBy );
	ini.GetValue( sPageName, "LineSpacing", cfg.m_iLineSpacing );
	ini.GetValue( sPageName, "Top", cfg.m_iTop );
	ini.GetValue( sPageName, "Baseline", cfg.m_iBaseline );
	ini.GetValue( sPageName, "DefaultWidth", cfg.m_iDefaultWidth );
	ini.GetValue( sPageName, "AdvanceExtraPixels", cfg.m_iAdvanceExtraPixels );
	ini.GetValue( sPageName, "TextureHints", cfg.m_sTextureHints );

	// Iterate over all keys.
	const XNode* pNode = ini.GetChild( sPageName );
	if( pNode )
	{
		FOREACH_CONST_Attr( pNode, pAttr )
		{
			RString sName = pAttr->first;
			const XNodeValue *pValue = pAttr->second;

			MakeUpper(sName);

			// If val is an integer, it's a width, eg. "10=27".
			if( IsAnInt(sName) )
			{
				cfg.m_mapGlyphWidths[StringToInt(sName)] = pValue->GetValue<int>();
				continue;
			}

			// "map codepoint=frame" maps a char to a frame.
			if( sName.substr(0, 4) == "MAP " )
			{
				/* map CODEPOINT=frame. CODEPOINT can be
				 * 1. U+hexval
				 * 2. an alias ("oq")
				 * 3. a character in quotes ("X")
				 *
				 * map 1=2 is the same as
				 * range unicode #1-1=2
				 */
				RString sCodepoint = sName.substr(4); // "CODEPOINT"

				wchar_t c;
				if( sCodepoint.substr(0, 2) == "U+" && IsHexVal(sCodepoint.substr(2)) )
					sscanf( sCodepoint.substr(2).c_str(), "%lc", &c );
				else if( sCodepoint.size() > 0 &&
						utf8_get_char_len(sCodepoint[0]) == int(sCodepoint.size()) )
				{
					c = utf8_get_char( sCodepoint.c_str() );
					if(c == wchar_t(-1))
						LOG->Warn("Font definition '%s' has an invalid value '%s'.",
							ini.GetPath().c_str(), sName.c_str() );
				}
				else if( !FontCharAliases::GetChar(sCodepoint, c) )
				{
					LOG->Warn("Font definition '%s' has an invalid value '%s'.",
						ini.GetPath().c_str(), sName.c_str() );
					continue;
				}

				cfg.CharToGlyphNo[c] = pValue->GetValue<int>();

				continue;
			}

			if( sName.substr(0, 6) == "RANGE " )
			{
				/* range CODESET=first_frame or
				 * range CODESET #start-end=first_frame
				 * eg
				 * range CP1252=0       (default for 256-frame fonts)
				 * range ASCII=0        (default for 128-frame fonts)
				 *
				 * (Start and end are in hex.)
				 *
				 * Map two high-bit portions of ISO-8859- to one font:
				 * range ISO-8859-2 #80-FF=0
				 * range ISO-8859-3 #80-FF=128
				 *
				 * Map hiragana to 0-84:
				 * range Unicode #3041-3094=0
				 */
				std::vector<RString> asMatches;
				static Regex parse("^RANGE ([A-Z0-9\\-]+)( ?#([0-9A-F]+)-([0-9A-F]+))?$");
				bool match = parse.Compare( sName, asMatches );
				ASSERT( asMatches.size() == 4 ); // 4 parens
				if(!match || asMatches[0].empty())
				{
					LuaHelpers::ReportScriptErrorFmt(
						"Font definition \"%s\" has an invalid range \"%s\": parse error.",
						ini.GetPath().c_str(), sName.c_str());
					continue;
				}
				// We must have either 1 match (just the codeset) or 4 (the whole thing).
				int count = -1;
				unsigned int first = 0;
				if( !asMatches[2].empty() )
				{
					sscanf( asMatches[2].c_str(), "%x", &first );
					unsigned int last;
					sscanf( asMatches[3].c_str(), "%x", &last );
					if(last < first)
					{
						LuaHelpers::ReportScriptErrorFmt(
							"Font definition \"%s\" has an invalid range \"%s\": %i < %i.",
							ini.GetPath().c_str(), sName.c_str(), last, first);
						continue;
					}
					count = last - first + 1;
				}

				RString error_string = cfg.MapRange( asMatches[0], first, pValue->GetValue<int>(), count );
				if(!error_string.empty())
				{
					LuaHelpers::ReportScriptErrorFmt(
						"Font definition \"%s\" has an invalid range \"%s\": %s.",
						ini.GetPath().c_str(), sName.c_str(), error_string.c_str());
					continue;
				}

				continue;
			}

			if( sName.substr(0, 5) == "LINE " )
			{
				/* line ROW=CHAR1CHAR2CHAR3CHAR4
				 * eg.
				 * line 0=ABCDEFGH
				 *
				 * This lets us assign characters very compactly and readably. */

				RString row_str = sName.substr(5);
				TrimLeft(row_str);

				if(!IsAnInt(row_str))
				{
					LuaHelpers::ReportScriptErrorFmt("Line name %s is not a number.",
						row_str.c_str());
					continue;
				}
				const int row = StringToInt(row_str);
				const int first_frame = row * num_frames_wide;

				if(row >= num_frames_high)
				{
					LuaHelpers::ReportScriptErrorFmt(
						"The font definition \"%s\" tries to assign line %i, "
						"but the font is only %i characters high.  "
						"Line numbers start at 0.",
						ini.GetPath().c_str(), first_frame, num_frames_high);
					continue;
				}

				// Decode the string.
				const std::wstring wdata( RStringToWstring(pValue->GetValue<RString>()) );

				if(int(wdata.size()) > num_frames_wide)
				{
					LuaHelpers::ReportScriptErrorFmt(
						"The font definition \"%s\" assigns %i characters to row %i"
						"(\"%ls\"), but the font  is only %i characters wide.",
						ini.GetPath().c_str(), (int)wdata.size(), row, wdata.c_str(),
						num_frames_wide);
					continue;
				}

				for( unsigned i = 0; i < wdata.size(); ++i )
					cfg.CharToGlyphNo[wdata[i]] = first_frame+i;
			}
		}
	}

	/* If it's 128 or 256 frames, default to ASCII or CP1252,
	 * respectively. 5x3 and 4x4 numbers fonts are supported as well.
	 * If it's anything else, we don't know what it is, so don't make
	 * any default mappings (the INI needs to do that itself). */
	if( sPageName != "common" && cfg.CharToGlyphNo.empty() )
	{
		switch( iNumFrames )
		{
		case 128:
			cfg.MapRange( "ascii", 0, 0, -1 );
			break;
		case 256:
			cfg.MapRange( "cp1252", 0, 0, -1 );
			break;
		case 15:
		case 16:
			cfg.MapRange( "numbers", 0, 0, -1 );
			break;
		default:
			LOG->Trace( "Font page \"%s\" has no characters", sTexturePath.c_str() );
		}
	}

	// If ' ' is set and nbsp is not, set nbsp.
	if( cfg.CharToGlyphNo.find(' ') != cfg.CharToGlyphNo.end() )
		cfg.CharToGlyphNo[0x00A0] = cfg.CharToGlyphNo[' '];
}

RString FontPageSettings::MapRange( RString sMapping, int iMapOffset, int iGlyphNo, int iCount )
{
	if( !CompareNoCase(sMapping, "Unicode") )
	{
		// Special case.
		if( iCount == -1 )
			return "Can't map all of Unicode to one font page"; // don't do that

		/* What's a practical limit?  A 2048x2048 texture could contain 16x16
		 * characters, which is 16384 glyphs. (Use a grayscale map and that's
		 * only 4 megs.) Let's use that as a cap. (We don't want to go crazy
		 * if someone says "range Unicode #0-FFFFFFFF".) */
		if( iCount > 16384 )
			return ssprintf( "Can't map %i glyphs to one font page", iCount );

		while( iCount )
		{
			CharToGlyphNo[iMapOffset] = iGlyphNo;
			iMapOffset++;
			iGlyphNo++;
			iCount--;
		}

		return RString();
	}

	const wchar_t *pMapping = FontCharmaps::get_char_map( sMapping );
	if( pMapping == nullptr )
		return "Unknown mapping";

	while( *pMapping != 0 && iMapOffset )
	{
		pMapping++;
		--iMapOffset;
	}
	if( iMapOffset )
		return "Map overflow"; // there aren't enough characters in the map

	// If iCount is -1, set it to the number of characters in the map.
	if( iCount == -1 )
		for( iCount = 0; pMapping[iCount] != 0; ++iCount ) ;

	while( *pMapping != 0 )
	{
		if( *pMapping != FontCharmaps::M_SKIP )
			CharToGlyphNo[*pMapping] = iGlyphNo;
		pMapping++;
		iGlyphNo++;
		iCount--;
	}

	if( iCount )
		return "Map overflow"; // there aren't enough characters in the map

	return RString();
}

static std::vector<RString> LoadStack;

/* A font set is a set of files, eg:
 *
 * Normal 16x16.png
 * Normal [other] 16x16.png
 * Normal [more] 8x8.png
 * Normal 16x16.ini           (the 16x16 here is optional)
 *
 * One INI and at least one texture is required.
 *
 * The entire font can be redirected; that's handled in ThemeManager.
 * Individual font files can not be redirected.
 *
 * If a file has no characters and sChars is not set, it will receive a default
 * mapping of ASCII or ISO-8859-1 if the font has exactly 128 or 256 frames.
 * 5x3 (15 frames) and 4x4 (16 frames) numbers sheets are also supported.
 * However, if it doesn't, we don't know what it is and the font will receive
 * no default mapping.  A font isn't useful with no characters mapped.
 */
void Font::Load( const RString &sIniPath, RString sChars )
{
	if(CompareNoCase(GetExtension(sIniPath), "ini"))
	{
		LuaHelpers::ReportScriptErrorFmt(
			"%s is not an ini file.  Fonts can only be loaded from ini files.",
			sIniPath.c_str());
		return;
	}
	//LOG->Trace( "Font: Loading new font '%s'",sIniPath.c_str());

	// Check for recursion (recursive imports).
	for( unsigned i = 0; i < LoadStack.size(); ++i )
	{
		if( LoadStack[i] == sIniPath )
		{
			RString str = join("\n", LoadStack);
			str += "\nCurrent font: " + sIniPath;
			LuaHelpers::ReportScriptErrorFmt(
				"Font import recursion detected\n%s", str.c_str());
			return;
		}
	}
	LoadStack.push_back( sIniPath );

	// The font is not already loaded. Figure out what we have.
	CHECKPOINT_M( ssprintf("Font::Load(\"%s\",\"%s\").", sIniPath.c_str(), m_sChars.c_str()) );

	path = sIniPath;
	m_sChars = sChars;

	// Get the filenames associated with this font.
	std::vector<RString> asTexturePaths;
	GetFontPaths( sIniPath, asTexturePaths );

	bool bCapitalsOnly = false;

	// If we have an INI, load it.
	IniFile ini;
	if( !sIniPath.empty() )
	{
		ini.ReadFile( sIniPath );
		ini.RenameKey("Char Widths", "main");	// backward compat
		ini.GetValue( "common", "CapitalsOnly", bCapitalsOnly );
		ini.GetValue( "common", "RightToLeft", m_bRightToLeft );
		ini.GetValue( "common", "DistanceField", m_bDistanceField );
		RString s;
		if( ini.GetValue( "common", "DefaultStrokeColor", s ) )
			m_DefaultStrokeColor.FromString( s );
	}

	{
		std::vector<RString> ImportList;

		bool bIsTopLevelFont = LoadStack.size() == 1;

		// If this is a top-level font (not a subfont), load the default font first.
		if( bIsTopLevelFont )
			ImportList.push_back("Common default");

		/* Check to see if we need to import any other fonts.  Do this
		 * before loading this font, so any characters in this font
		 * override imported characters. */
		RString imports;
		ini.GetValue( "main", "import", imports );
		split(imports, ",", ImportList, true);

		if( bIsTopLevelFont  &&  imports.empty()  &&  asTexturePaths.empty() )
		{
			RString s = ssprintf( "Font \"%s\" is a top-level font with no textures or imports.", sIniPath.c_str() );
			Dialog::OK( s );
		}

		for(unsigned i = 0; i < ImportList.size(); ++i)
		{
			RString sPath = THEME->GetPathF( "", ImportList[i], true );
			if( sPath == "" )
			{
				RString s = ssprintf( "Font \"%s\" imports a font \"%s\" that doesn't exist", sIniPath.c_str(), ImportList[i].c_str() );
				Dialog::OK( s );
				continue;
			}

			Font subfont;
			subfont.Load(sPath,"");
			MergeFont(subfont);
		}
	}

	// Load each font page.
	for( unsigned i = 0; i < asTexturePaths.size(); ++i )
	{
		const RString &sTexturePath = asTexturePaths[i];

		// Grab the page name, eg "foo" from "Normal [foo].png".
		RString sPagename = GetPageNameFromFileName( sTexturePath );

		// Ignore stroke textures
		if( sTexturePath.find("-stroke") != std::string::npos )
			continue;

		// Create this down here so it doesn't leak if the continue gets triggered.
		FontPage *pPage = new FontPage;

		// Load settings for this page from the INI.
		FontPageSettings cfg;
		LoadFontPageSettings( cfg, ini, sTexturePath, "common", sChars );
		LoadFontPageSettings( cfg, ini, sTexturePath, sPagename, sChars );

		// Load.
		pPage->Load( cfg );

		/* Expect at least as many frames as we have premapped characters. */
		/* Make sure that we don't map characters to frames we don't actually
		 * have.  This can happen if the font is too small for an sChars. */
		for(std::map<wchar_t,int>::iterator it = pPage->m_iCharToGlyphNo.begin();
			it != pPage->m_iCharToGlyphNo.end(); ++it)
		{
			if( it->second < pPage->m_FontPageTextures.m_pTextureMain->GetNumFrames() )
				continue; /* OK */
			LuaHelpers::ReportScriptErrorFmt(
				"The font \"%s\" maps \"%s\" to frame %i, "
				"but the font only has %i frames.",
				sTexturePath.c_str(), WcharDisplayText(wchar_t(it->first)).c_str(),
				it->second,
				pPage->m_FontPageTextures.m_pTextureMain->GetNumFrames());
			it->second= 0;
		}

//		LOG->Trace( "Adding page %s (%s) to %s; %i glyphs",
//			TexturePaths[i].c_str(), sPagename.c_str(),
//			sFontOrTextureFilePath.c_str(), pPage->m_iCharToGlyphNo.size() );
		AddPage( pPage );

		/* If this is the first font loaded, or it's called "main", this page's
		 * properties become the font's properties. */
		if( i == 0 || sPagename == "main" )
			SetDefaultGlyph( pPage );
	}

	if( bCapitalsOnly )
		CapsOnly();

	if( m_iCharToGlyph.empty() )
		LuaHelpers::ReportScriptErrorFmt("Font %s has no characters", sIniPath.c_str());

	LoadStack.pop_back();

	if( LoadStack.empty() )
	{
		// Cache ASCII glyphs.
		ZERO( m_iCharToGlyphCache );
		std::map<wchar_t,glyph*>::iterator it;
		for( it = m_iCharToGlyph.begin(); it != m_iCharToGlyph.end(); ++it )
			if( it->first < (int) ARRAYLEN(m_iCharToGlyphCache) )
				m_iCharToGlyphCache[it->first] = it->second;
	}
}

/*
 * (c) 2001-2004 Glenn Maynard, Chris Danford
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

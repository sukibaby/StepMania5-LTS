#include "global.h"

#include "StepMania.h"

// Rage global classes
#include "RageLog.h"
#include "RageTextureManager.h"
#include "RageSoundManager.h"
#include "GameSoundManager.h"
#include "RageInput.h"
#include "RageTimer.h"
#include "RageMath.h"
#include "RageDisplay.h"
#include "RageThreads.h"
#include "LocalizedString.h"

#include "arch/ArchHooks/ArchHooks.h"
#include "arch/LoadingWindow/LoadingWindow.h"
#include "arch/Dialog/Dialog.h"

#include "ProductInfo.h"

#include "Screen.h"
#include "InputEventPlus.h"
#include "ScreenDimensions.h"
#include "CodeDetector.h"
#include "CommonMetrics.h"
#include "Game.h"
#include "RageSurface.h"
#include "RageSurface_Load.h"
#include "CommandLineActions.h"

#if !defined(SUPPORT_OPENGL) && !defined(SUPPORT_D3D)
#define SUPPORT_OPENGL
#endif

// StepMania global classes
#include "ThemeManager.h"
#include "NoteSkinManager.h"
#include "PrefsManager.h"
#include "Song.h"
#include "SongManager.h"
#include "CharacterManager.h"
#include "GameState.h"
#include "AnnouncerManager.h"
#include "ProfileManager.h"
#include "MemoryCardManager.h"
#include "ScreenManager.h"
#include "LuaManager.h"
#include "GameManager.h"
#include "FontManager.h"
#include "InputFilter.h"
#include "InputMapper.h"
#include "InputQueue.h"
#include "SongCacheIndex.h"
#include "ImageCache.h"
#include "UnlockManager.h"
#include "RageFileManager.h"
#include "Bookkeeper.h"
#include "LightsManager.h"
#include "ModelManager.h"
#include "CryptManager.h"
#include "NetworkManager.h"
#include "MessageManager.h"
#include "StatsManager.h"
#include "GameLoop.h"
#include "SpecialFiles.h"
#include "Profile.h"
#include "ActorUtil.h"
#include "ver.h"

#include <cmath>
#include <ctime>
#include <vector>

void ShutdownGame();
bool HandleGlobalInputs( const InputEventPlus &input );
void HandleInputEvents(float fDeltaTime);

static Preference<bool> g_bAllowMultipleInstances( "AllowMultipleInstances", false );

void StepMania::GetPreferredVideoModeParams( VideoModeParams &paramsOut )
{
	// resolution handling code that probably needs fixing
	int iWidth = PREFSMAN->m_iDisplayWidth;
	if( PREFSMAN->m_bWindowed )
	{
		//float fRatio = PREFSMAN->m_iDisplayHeight;
		//iWidth = PREFSMAN->m_iDisplayHeight * fRatio;
		iWidth = std::ceil(PREFSMAN->m_iDisplayHeight * PREFSMAN->m_fDisplayAspectRatio);
		// ceil causes the width to come out odd when it shouldn't.
		// 576 * 1.7778 = 1024.0128, which is rounded to 1025. -Kyz
		iWidth-= iWidth % 2;
	}

	paramsOut = VideoModeParams(
		PREFSMAN->m_bWindowed || PREFSMAN->m_bFullscreenIsBorderlessWindow,
		PREFSMAN->m_sDisplayId,
		iWidth,
		PREFSMAN->m_iDisplayHeight,
		PREFSMAN->m_iDisplayColorDepth,
		PREFSMAN->m_iRefreshRate,
		PREFSMAN->m_bVsync,
		PREFSMAN->m_bInterlaced,
		PREFSMAN->m_bSmoothLines,
		PREFSMAN->m_bTrilinearFiltering,
		PREFSMAN->m_bAnisotropicFiltering,
		!PREFSMAN->m_bWindowed && PREFSMAN->m_bFullscreenIsBorderlessWindow,
		CommonMetrics::WINDOW_TITLE,
		THEME->GetPathG("Common","window icon"),
		PREFSMAN->m_bPAL,
		PREFSMAN->m_fDisplayAspectRatio
	);
}

static LocalizedString COLOR			("StepMania","color");
static LocalizedString TEXTURE			("StepMania","texture");
static LocalizedString WINDOWED		("StepMania","Windowed");
static LocalizedString FULLSCREEN		("StepMania","Fullscreen");
static LocalizedString ANNOUNCER_		("StepMania","Announcer");
static LocalizedString VSYNC			("StepMania","Vsync");
static LocalizedString NO_VSYNC		("StepMania","NoVsync");
static LocalizedString SMOOTH_LINES	("StepMania","SmoothLines");
static LocalizedString NO_SMOOTH_LINES	("StepMania","NoSmoothLines");

static RString GetActualGraphicOptionsString()
{
	const VideoModeParams &params = DISPLAY->GetActualVideoModeParams();
	RString sFormat = "%s %s %dx%d %d "+COLOR.GetValue()+" %d "+TEXTURE.GetValue()+" %dHz %s %s";
	RString sLog = ssprintf( sFormat.c_str(),
		DISPLAY->GetApiDescription().c_str(),
		(params.windowed? WINDOWED : FULLSCREEN).GetValue().c_str(),
		(int)params.width,
		(int)params.height,
		(int)params.bpp,
		(int)PREFSMAN->m_iTextureColorDepth,
		(int)params.rate,
		(params.vsync? VSYNC : NO_VSYNC).GetValue().c_str(),
		(PREFSMAN->m_bSmoothLines? SMOOTH_LINES : NO_SMOOTH_LINES).GetValue().c_str() );
	return sLog;
}

static void StoreActualGraphicOptions()
{
	/* Store the settings that RageDisplay was actually able to use so that
	 * we don't go through the process of auto-detecting a usable video mode
	 * every time. */
	const VideoModeParams &params = DISPLAY->GetActualVideoModeParams();
	PREFSMAN->m_bWindowed.Set( params.windowed && !params.bWindowIsFullscreenBorderless );
	if (!params.windowed && !params.bWindowIsFullscreenBorderless) {
		// In all other cases, want to preserve the value of this preference,
		// but if DISPLAY decides to go fullscreen exclusive, we'll persist that decision
		PREFSMAN->m_bFullscreenIsBorderlessWindow.Set( false );
	}


	/* If we're windowed, we may have tweaked the width based on the aspect ratio.
	 * Don't save this new value over the preferred value. */
	if( !PREFSMAN->m_bWindowed )
	{
		PREFSMAN->m_iDisplayWidth	.Set( params.width );
		PREFSMAN->m_iDisplayHeight	.Set( params.height );
	}
	PREFSMAN->m_iDisplayColorDepth	.Set( params.bpp );
	if( PREFSMAN->m_iRefreshRate != REFRESH_DEFAULT )
		PREFSMAN->m_iRefreshRate.Set( params.rate );
	PREFSMAN->m_bVsync		.Set( params.vsync );

	Dialog::SetWindowed( params.windowed );
}

static RageDisplay *CreateDisplay();

bool StepMania::GetHighResolutionTextures()
{
	switch( PREFSMAN->m_HighResolutionTextures )
	{
	default:
	case HighResolutionTextures_Auto:
		{
			int height = PREFSMAN->m_iDisplayHeight;
			return height > THEME->GetMetricI("Common", "ScreenHeight");
		}
	case HighResolutionTextures_ForceOn:
		return true;
	case HighResolutionTextures_ForceOff:
		return false;
	}
}

static void update_centering()
{
	DISPLAY->ChangeCentering(
		PREFSMAN->m_iCenterImageTranslateX, PREFSMAN->m_iCenterImageTranslateY,
		PREFSMAN->m_fCenterImageAddWidth, PREFSMAN->m_fCenterImageAddHeight);
}

static void StartDisplay()
{
	if( DISPLAY != nullptr )
		return; // already started

	DISPLAY = CreateDisplay();

	update_centering();

	TEXTUREMAN	= new RageTextureManager;
	TEXTUREMAN->SetPrefs(
		RageTextureManagerPrefs(
			PREFSMAN->m_iTextureColorDepth,
			PREFSMAN->m_iMovieColorDepth,
			PREFSMAN->m_bDelayedTextureDelete,
			PREFSMAN->m_iMaxTextureResolution,
			StepMania::GetHighResolutionTextures(),
			PREFSMAN->m_bForceMipMaps
			)
		);

	MODELMAN	= new ModelManager;
	MODELMAN->SetPrefs(
		ModelManagerPrefs(
			PREFSMAN->m_bDelayedModelDelete
			)
		);
}

void StepMania::ApplyGraphicOptions()
{
	bool bNeedReload = false;

	VideoModeParams params;
	GetPreferredVideoModeParams( params );
	RString sError = DISPLAY->SetVideoMode( params, bNeedReload );
	if( sError != "" )
		RageException::Throw( "%s", sError.c_str() );

	update_centering();

	bNeedReload |= TEXTUREMAN->SetPrefs(
		RageTextureManagerPrefs(
			PREFSMAN->m_iTextureColorDepth,
			PREFSMAN->m_iMovieColorDepth,
			PREFSMAN->m_bDelayedTextureDelete,
			PREFSMAN->m_iMaxTextureResolution,
			StepMania::GetHighResolutionTextures(),
			PREFSMAN->m_bForceMipMaps
			)
		);

	bNeedReload |= MODELMAN->SetPrefs(
		ModelManagerPrefs(
			PREFSMAN->m_bDelayedModelDelete
			)
		);

	if( bNeedReload )
		TEXTUREMAN->ReloadAll();

	StoreActualGraphicOptions();
	if( SCREENMAN )
		SCREENMAN->SystemMessage( GetActualGraphicOptionsString() );

	// Give the input handlers a chance to re-open devices as necessary.
	INPUTMAN->WindowReset();
}

static bool CheckVideoDefaultSettings();

void StepMania::ResetPreferences()
{
	PREFSMAN->ResetToFactoryDefaults();
	SOUNDMAN->SetMixVolume();
	CheckVideoDefaultSettings();
	ApplyGraphicOptions();
}

/* Shutdown all global singletons. Note that this may be called partway through
 * initialization, due to an object failing to initialize, in which case some of
 * these may still be nullptr. */
void ShutdownGame()
{
	/* First, tell SOUNDMAN that we're shutting down. This signals sound drivers to
	 * stop sounds, which we want to do before any threads that may have started sounds
	 * are closed; this prevents annoying DirectSound glitches and delays. */
	if( SOUNDMAN )
		SOUNDMAN->Shutdown();

	/* Reset all lights to off.
	 * This is done before ~LightsManager as some drivers use SCREENMAN
	 * and similar when setting lights. */
	if( LIGHTSMAN )
	{
		LIGHTSMAN->TurnOffAllLights();
	}

	RageUtil::SafeDelete( NETWORK );
	RageUtil::SafeDelete( SCREENMAN );
	RageUtil::SafeDelete( STATSMAN );
	RageUtil::SafeDelete( MESSAGEMAN );
	/* Delete INPUTMAN before the other INPUTFILTER handlers, or an input
	 * driver may try to send a message to INPUTFILTER after we delete it. */
	RageUtil::SafeDelete( INPUTMAN );
	RageUtil::SafeDelete( INPUTQUEUE );
	RageUtil::SafeDelete( INPUTMAPPER );
	RageUtil::SafeDelete( INPUTFILTER );
	RageUtil::SafeDelete( MODELMAN );
	RageUtil::SafeDelete( PROFILEMAN ); // PROFILEMAN needs the songs still loaded
	RageUtil::SafeDelete( CHARMAN );
	RageUtil::SafeDelete( UNLOCKMAN );
	RageUtil::SafeDelete( CRYPTMAN );
	RageUtil::SafeDelete( MEMCARDMAN );
	RageUtil::SafeDelete( SONGMAN );
	RageUtil::SafeDelete( IMAGECACHE );
	RageUtil::SafeDelete( SONGINDEX );
	RageUtil::SafeDelete( SOUND ); // uses GAMESTATE, PREFSMAN
	RageUtil::SafeDelete( PREFSMAN );
	RageUtil::SafeDelete( GAMESTATE );
	RageUtil::SafeDelete( GAMEMAN );
	RageUtil::SafeDelete( NOTESKIN );
	RageUtil::SafeDelete( THEME );
	RageUtil::SafeDelete( ANNOUNCER );
	RageUtil::SafeDelete( BOOKKEEPER );
	RageUtil::SafeDelete( LIGHTSMAN );
	RageUtil::SafeDelete( SOUNDMAN );
	RageUtil::SafeDelete( FONT );
	RageUtil::SafeDelete( TEXTUREMAN );
	RageUtil::SafeDelete( DISPLAY );
	Dialog::Shutdown();
	RageUtil::SafeDelete( LOG );
	RageUtil::SafeDelete( FILEMAN );
	RageUtil::SafeDelete( LUA );
	RageUtil::SafeDelete( HOOKS );
}

static void HandleException( const RString &sError )
{
	if( g_bAutoRestart )
		HOOKS->RestartProgram();

	// Shut down first, so we exit graphics mode before trying to open a dialog.
	ShutdownGame();

	// Throw up a pretty error dialog.
	Dialog::Error( sError );
	Dialog::Shutdown(); // Shut it back down.
}

void StepMania::ResetGame()
{
	GAMESTATE->Reset();

	if( !THEME->DoesThemeExist( THEME->GetCurThemeName() ) )
	{
		RString sGameName = GAMESTATE->GetCurrentGame()->m_szName;
		if( !THEME->DoesThemeExist(sGameName) )
			sGameName = PREFSMAN->m_sDefaultTheme; // was previously "default" -aj
		THEME->SwitchThemeAndLanguage( sGameName, THEME->GetCurLanguage(), PREFSMAN->m_bPseudoLocalize );
		TEXTUREMAN->DoDelayedDelete();
	}

	PREFSMAN->SavePrefsToDisk();
}

ThemeMetric<RString>	INITIAL_SCREEN	("Common","InitialScreen");
RString StepMania::GetInitialScreen()
{
	if(PREFSMAN->m_sTestInitialScreen.Get() != "" &&
		SCREENMAN->IsScreenNameValid(PREFSMAN->m_sTestInitialScreen))
	{
		return PREFSMAN->m_sTestInitialScreen;
	}
	RString screen_name= INITIAL_SCREEN.GetValue();
	if(!SCREENMAN->IsScreenNameValid(screen_name))
	{
		screen_name= "ScreenInitialScreenIsInvalid";
	}
	return screen_name;
}
ThemeMetric<RString>	SELECT_MUSIC_SCREEN	("Common","SelectMusicScreen");
RString StepMania::GetSelectMusicScreen()
{
	return SELECT_MUSIC_SCREEN.GetValue();
}

#if defined(_WIN32)
#include "RageDisplay_D3D.h"
#include "archutils/Win32/VideoDriverInfo.h"
#endif

#if defined(SUPPORT_OPENGL)
#include "RageDisplay_OGL.h"
#endif

#if defined(SUPPORT_GLES2)
#include "RageDisplay_GLES2.h"
#endif

#include "RageDisplay_Null.h"


struct VideoCardDefaults
{
	RString sDriverRegex;
	RString sVideoRenderers;
	int iWidth;
	int iHeight;
	int iDisplayColor;
	int iTextureColor;
	int iMovieColor;
	int iTextureSize;
	bool bSmoothLines;

	VideoCardDefaults() {}
	VideoCardDefaults(
		RString sDriverRegex_,
		RString sVideoRenderers_,
		int iWidth_,
		int iHeight_,
		int iDisplayColor_,
		int iTextureColor_,
		int iMovieColor_,
		int iTextureSize_,
		bool bSmoothLines_
		)
	{
		sDriverRegex = sDriverRegex_;
		sVideoRenderers = sVideoRenderers_;
		iWidth = iWidth_;
		iHeight = iHeight_;
		iDisplayColor = iDisplayColor_;
		iTextureColor = iTextureColor_;
		iMovieColor = iMovieColor_;
		iTextureSize = iTextureSize_;
		bSmoothLines = bSmoothLines_;
	}
} const g_VideoCardDefaults[] =
{
	// These lines correspond to the struct defined above.
	VideoCardDefaults(
		"OpenGL",       // Video card name (generic Mac/Linux)
		"opengl",       // Available renderers
		1280,720,       // Default resolution
		32,             // Display color
		32,             // Texture color
		32,             // Movie color
		2048,           // Texture size
		true            // Smooth lines
	),
	VideoCardDefaults(
		"",             // Video card name (generic Windows)
		"opengl,d3d",   // Available renderers
		1280,720,       // Default resolution
		32,             // Display color
		32,             // Texture color
		32,             // Movie color
		2048,           // Texture size
		true            // Smooth lines
	),
};


static RString GetVideoDriverName()
{
#if defined(_WIN32)
	return GetPrimaryVideoDriverName();
#else
	return "OpenGL";
#endif
}

bool CheckVideoDefaultSettings()
{
	// Video card changed since last run
	RString sVideoDriver = GetVideoDriverName();

	LOG->Trace( "Last seen video driver: %s", PREFSMAN->m_sLastSeenVideoDriver.Get().c_str() );

	VideoCardDefaults defaults;

	unsigned i;
	for( i=0; i<ARRAYLEN(g_VideoCardDefaults); i++ )
	{
		defaults = g_VideoCardDefaults[i];

		RString sDriverRegex = defaults.sDriverRegex;
		Regex regex( sDriverRegex );
		if( regex.Compare(sVideoDriver) )
		{
			LOG->Trace( "Card matches '%s'.", sDriverRegex.size()? sDriverRegex.c_str():"(unknown card)" );
			break;
		}
	}
	if (i >= ARRAYLEN(g_VideoCardDefaults))
	{
		FAIL_M("Failed to match video driver");
	}

	bool bSetDefaultVideoParams = false;
	if( PREFSMAN->m_sVideoRenderers.Get() == "" )
	{
		bSetDefaultVideoParams = true;
		LOG->Trace( "Applying defaults for %s.", sVideoDriver.c_str() );
	}
	else if( PREFSMAN->m_sLastSeenVideoDriver.Get() != sVideoDriver )
	{
		bSetDefaultVideoParams = true;
		LOG->Trace( "Video card has changed from %s to %s.  Applying new defaults.", PREFSMAN->m_sLastSeenVideoDriver.Get().c_str(), sVideoDriver.c_str() );
	}

	if( bSetDefaultVideoParams )
	{
		PREFSMAN->m_sVideoRenderers.Set( defaults.sVideoRenderers );
		PREFSMAN->m_iDisplayWidth.Set( defaults.iWidth );
		PREFSMAN->m_iDisplayHeight.Set( defaults.iHeight );
		PREFSMAN->m_iDisplayColorDepth.Set( defaults.iDisplayColor );
		PREFSMAN->m_iTextureColorDepth.Set( defaults.iTextureColor );
		PREFSMAN->m_iMovieColorDepth.Set( defaults.iMovieColor );
		PREFSMAN->m_iMaxTextureResolution.Set( defaults.iTextureSize );
		PREFSMAN->m_bSmoothLines.Set( defaults.bSmoothLines );
		// this only worked when we started in fullscreen by default. -aj
		//PREFSMAN->m_fDisplayAspectRatio.Set( HOOKS->GetDisplayAspectRatio() );
		// now that we start in windowed mode, use the new default aspect ratio.
		PREFSMAN->m_fDisplayAspectRatio.Set( PREFSMAN->m_fDisplayAspectRatio );

		// Update last seen video card
		PREFSMAN->m_sLastSeenVideoDriver.Set( GetVideoDriverName() );
	}
	else if( PREFSMAN->m_sVideoRenderers.Get().CompareNoCase(defaults.sVideoRenderers) )
	{
		LOG->Warn("Video renderer list has been changed from '%s' to '%s'",
				defaults.sVideoRenderers.c_str(), PREFSMAN->m_sVideoRenderers.Get().c_str() );
	}

	LOG->Info( "Video renderers: '%s'", PREFSMAN->m_sVideoRenderers.Get().c_str() );
	return bSetDefaultVideoParams;
}

static LocalizedString ERROR_INITIALIZING_CARD		( "StepMania", "There was an error while initializing your video card." );
static LocalizedString ERROR_DONT_FILE_BUG		( "StepMania", "Please do not file this error as a bug!  Use the web page below to troubleshoot this problem." );
static LocalizedString ERROR_VIDEO_DRIVER		( "StepMania", "Video Driver: %s" );
static LocalizedString ERROR_NO_VIDEO_RENDERERS		( "StepMania", "No video renderers attempted." );
static LocalizedString ERROR_INITIALIZING		( "StepMania", "Initializing %s..." );
static LocalizedString ERROR_UNKNOWN_VIDEO_RENDERER	( "StepMania", "Unknown video renderer value: %s" );

RageDisplay *CreateDisplay()
{
	/* We never want to bother users with having to decide which API to use.
	 *
	 * Some cards simply are too troublesome with OpenGL to ever use it, eg. Voodoos.
	 * If D3D8 isn't installed on those, complain and refuse to run (by default).
	 * For others, always use OpenGL.  Allow forcing to D3D as an advanced option.
	 *
	 * If we're missing acceleration when we load D3D8 due to a card being in the
	 * D3D list, it means we need drivers and that they do exist.
	 *
	 * If we try to load OpenGL and we're missing acceleration, it may mean:
	 *  1. We're missing drivers, and they just need upgrading.
	 *  2. The card doesn't have drivers, and it should be using D3D8.
	 *     In other words, it needs an entry in this table.
	 *  3. The card doesn't have drivers for either.  (Sorry, no S3 868s.)
	 *     Can't play.
	 * In this case, fail to load; don't silently fall back on D3D.  We don't want
	 * people unknowingly using D3D8 with old drivers (and reporting obscure bugs
	 * due to driver problems).  We'll probably get bug reports for all three types.
	 * #2 is the only case that's actually a bug.
	 *
	 * Actually, right now we're falling back. I'm not sure which behavior is better.
	 */

	//bool bAppliedDefaults = CheckVideoDefaultSettings();
	CheckVideoDefaultSettings();

	VideoModeParams params;
	StepMania::GetPreferredVideoModeParams( params );

	RString error = ERROR_INITIALIZING_CARD.GetValue()+"\n\n"+
		ERROR_DONT_FILE_BUG.GetValue()+"\n\n"
		VIDEO_TROUBLESHOOTING_URL "\n\n"+
		ssprintf(ERROR_VIDEO_DRIVER.GetValue().c_str(), GetVideoDriverName().c_str())+"\n\n";

	std::vector<RString> asRenderers;
	split( PREFSMAN->m_sVideoRenderers, ",", asRenderers, true );

	if( asRenderers.empty() )
		RageException::Throw( "%s", ERROR_NO_VIDEO_RENDERERS.GetValue().c_str() );

	RageDisplay *pRet = nullptr;
	for( unsigned i=0; i<asRenderers.size(); i++ )
	{
		RString sRenderer = asRenderers[i];

		if( sRenderer.CompareNoCase("opengl")==0 )
		{
#if defined(SUPPORT_OPENGL)
			pRet = new RageDisplay_Legacy;
#endif
		}
		else if( sRenderer.CompareNoCase("gles2")==0 )
		{
#if defined(SUPPORT_GLES2)
			pRet = new RageDisplay_GLES2;
#endif
		}
		else if( sRenderer.CompareNoCase("d3d")==0 )
		{
// TODO: ANGLE/RageDisplay_Modern
#if defined(SUPPORT_D3D)
			pRet = new RageDisplay_D3D;
#endif
		}
		else if( sRenderer.CompareNoCase("null")==0 )
		{
			return new RageDisplay_Null;
		}
		else
		{
			RageException::Throw( ERROR_UNKNOWN_VIDEO_RENDERER.GetValue().c_str(), sRenderer.c_str() );
		}

		if( pRet == nullptr )
			continue;

		RString sError = pRet->Init( params, PREFSMAN->m_bAllowUnacceleratedRenderer );
		if( !sError.empty() )
		{
			error += ssprintf(ERROR_INITIALIZING.GetValue().c_str(), sRenderer.c_str())+"\n" + sError;
			RageUtil::SafeDelete( pRet );
			error += "\n\n\n";
			continue;
		}

		break; // the display is ready to go
	}

	if( pRet == nullptr)
		RageException::Throw( "%s", error.c_str() );

	return pRet;
}

static void SwitchToLastPlayedGame()
{
	const Game *pGame = GAMEMAN->StringToGame( PREFSMAN->GetCurrentGame() );

	// If the active game type isn't actually available, revert to the default.
	if( pGame == nullptr )
		pGame = GAMEMAN->GetDefaultGame();

	if( !GAMEMAN->IsGameEnabled( pGame ) && pGame != GAMEMAN->GetDefaultGame() )
	{
		pGame = GAMEMAN->GetDefaultGame();
		LOG->Warn( "Default NoteSkin for \"%s\" missing, reverting to \"%s\"",
			pGame->m_szName, GAMEMAN->GetDefaultGame()->m_szName );
	}

	ASSERT( GAMEMAN->IsGameEnabled(pGame) );

	StepMania::InitializeCurrentGame( pGame );
}

// This function is meant to only be called during start up.
void StepMania::InitializeCurrentGame( const Game* g )
{
	ASSERT( g != nullptr );
	ASSERT( GAMESTATE != nullptr );
	ASSERT( ANNOUNCER != nullptr );
	ASSERT( THEME != nullptr );

	GAMESTATE->SetCurGame( g );

	RString sAnnouncer = PREFSMAN->m_sAnnouncer;
	RString sTheme = PREFSMAN->m_sTheme;
	RString sGametype = GAMESTATE->GetCurrentGame()->m_szName;
	RString sLanguage = PREFSMAN->m_sLanguage;

	if( sAnnouncer.empty() )
		sAnnouncer = GAMESTATE->GetCurrentGame()->m_szName;
	RString argCurGame;
	if( GetCommandlineArgument( "game", &argCurGame) && argCurGame != sGametype )
	{
		Game const* new_game= GAMEMAN->StringToGame(argCurGame);
		if(new_game == nullptr)
		{
			LOG->Warn("%s is not a known game type, ignoring.", argCurGame.c_str());
		}
		else
		{
			PREFSMAN->SetCurrentGame(sGametype);
			GAMESTATE->SetCurGame(new_game);
		}
	}

	// It doesn't matter if sTheme is blank or invalid, THEME->STAL will set
	// a selectable theme for us. -Kyz

	// process gametype, theme and language command line arguments;
	// these change the preferences in order for transparent loading -aj
	RString argTheme;
	if( GetCommandlineArgument(	"theme",&argTheme) && argTheme != sTheme )
	{
		sTheme = argTheme;
		// set theme in preferences too for correct behavior  -aj
		PREFSMAN->m_sTheme.Set(sTheme);
	}

	RString argLanguage;
	if( GetCommandlineArgument(	"language",&argLanguage) )
	{
		sLanguage = argLanguage;
		// set language in preferences too for correct behavior -aj
		PREFSMAN->m_sLanguage.Set(sLanguage);
	}

	// it's OK to call these functions with names that don't exist.
	ANNOUNCER->SwitchAnnouncer( sAnnouncer );
	THEME->SwitchThemeAndLanguage( sTheme, sLanguage, PREFSMAN->m_bPseudoLocalize );

	// Set the input scheme for the new game, and load keymaps.
	if( INPUTMAPPER )
	{
		INPUTMAPPER->SetInputScheme( &g->m_InputScheme );
		INPUTMAPPER->ReadMappingsFromDisk();
	}
}

static void MountTreeOfZips( const RString &dir )
{
	std::vector<RString> dirs;
	dirs.push_back( dir );

	while( dirs.size() )
	{
		RString path = dirs.back();
		dirs.pop_back();

		if( !IsADirectory(path) )
			continue;

		std::vector<RString> zips;
		GetDirListing( path + "/*.zip", zips, false, true );
		GetDirListing( path + "/*.smzip", zips, false, true );

		for (const auto& zip : zips)
		{
			if( !IsAFile(zip) )
				continue;

			LOG->Trace( "VFS: found %s", zip.c_str() );
			FILEMAN->Mount( "zip", zip, "/" );
		}

		GetDirListing( path + "/*", dirs, true, true );
	}
}

static void MountFolders(const RString &type, const RString &realPathList, const RString &mountPoint)
{
	if (!realPathList.empty())
	{
		std::vector<RString> dirs;
		split(realPathList, ",", dirs, true);
		for (const auto& dir : dirs)
			FILEMAN->Mount(type, dir, mountPoint);
	}
}

static void WriteLogHeader()
{
	LOG->Info("%s%s", PRODUCT_FAMILY, product_version);

	LOG->Info( "Compiled %s @ %s (build %s)", version_date, version_time, ::sm_version_git_hash);

	time_t cur_time;
	time(&cur_time);
	struct tm now;
	localtime_r( &cur_time, &now );

	LOG->Info( "Log starting %.4d-%.2d-%.2d %.2d:%.2d:%.2d",
		1900+now.tm_year, now.tm_mon+1, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec );
	LOG->Trace( " " );

	if( g_argc > 1 )
	{
		RString args;
		for( int i = 1; i < g_argc; ++i )
		{
			if( i>1 )
				args += " ";

			// surround all params with some marker, as they might have whitespace.
			// using [[ and ]], as they are not likely to be in the params.
			args += ssprintf( "[[%s]]", g_argv[i] );
		}
		LOG->Info( "Command line args (count=%d): %s", (g_argc - 1), args.c_str());
	}
}

static void ApplyLogPreferences()
{
	LOG->SetShowLogOutput( PREFSMAN->m_bShowLogOutput );
	LOG->SetLogToDisk( PREFSMAN->m_bLogToDisk );
	LOG->SetInfoToDisk( PREFSMAN->m_bLogToDisk );
	LOG->SetUserLogToDisk( PREFSMAN->m_bLogToDisk );
	LOG->SetFlushing( PREFSMAN->m_bForceLogFlush );
	Checkpoints::LogCheckpoints( PREFSMAN->m_bLogCheckpoints );
}

static LocalizedString COULDNT_OPEN_LOADING_WINDOW( "LoadingWindow", "Couldn't open any loading windows." );

int sm_main(int argc, char* argv[])
{
	RageThreadRegister thread( "Main thread" );
	RageException::SetCleanupHandler( HandleException );

	SetCommandlineArguments( argc, argv );

	// Set up arch hooks first.  This may set up crash handling.
	HOOKS = ArchHooks::Create();
	HOOKS->Init();

	LUA		= new LuaManager;
	HOOKS->RegisterWithLua();

	// Initialize the file extension type lists so everything can ask ActorUtil
	// what the type of a file is.
	ActorUtil::InitFileTypeLists();

	// Almost everything uses this to read and write files.  Load this early.
	FILEMAN = new RageFileManager( argv[0] );
	FILEMAN->ProtectPath(SpecialFiles::DEFAULTS_INI_PATH);
	FILEMAN->ProtectPath(SpecialFiles::STATIC_INI_PATH);
	FILEMAN->ProtectPath(SpecialFiles::PREFERENCES_INI_PATH);
	FILEMAN->MountInitialFilesystems();

	bool bPortable = DoesFileExist("/Portable.ini");
	if( !bPortable )
		FILEMAN->MountUserFilesystems();

	// Set this up next. Do this early, since it's needed for RageException::Throw.
	LOG		= new RageLog;

	// Whew--we should be able to crash safely now!

	// load preferences and mount any alternative trees.
	PREFSMAN	= new PrefsManager;

	/* Allow HOOKS to check for multiple instances.  We need to do this after PREFS is initialized,
	 * so ArchHooks can use a preference to turn this off.  We want to do this before ApplyLogPreferences,
	 * so if we exit because of another instance, we don't try to clobber its log.  We also want to
	 * do this before opening the loading window, so if we give focus away, we don't flash the window. */
	if(!g_bAllowMultipleInstances.Get() && HOOKS->CheckForMultipleInstances(argc, argv))
	{
		ShutdownGame();
		return 0;
	}

	ApplyLogPreferences();

	WriteLogHeader();

	// Set up alternative filesystem trees.
	MountFolders("dirro", PREFSMAN->m_sAdditionalFoldersReadOnly.Get(), "/");
	MountFolders("dir", PREFSMAN->m_sAdditionalFoldersWritable.Get(), "/");
	MountFolders("dirro", PREFSMAN->m_sAdditionalSongFoldersReadOnly.Get(), "/Songs");
	MountFolders("dir", PREFSMAN->m_sAdditionalSongFoldersWritable.Get(), "/Songs");
	MountFolders("dirro", PREFSMAN->m_sAdditionalCourseFoldersReadOnly.Get(), "/Courses");
	MountFolders("dir", PREFSMAN->m_sAdditionalCourseFoldersWritable.Get(), "/Courses");

	MountTreeOfZips( SpecialFiles::PACKAGES_DIR );

	/* One of the above filesystems might contain files that affect preferences
	 * (e.g. Data/Static.ini). Re-read preferences. */
	PREFSMAN->ReadPrefsFromDisk();
	ApplyLogPreferences();

	// This needs PREFSMAN.
	Dialog::Init();

	// Set up the messaging system early to have well defined code.
	MESSAGEMAN	= new MessageManager;

	// Create game objects
	GAMESTATE	= new GameState;

	// This requires PREFSMAN, for PREFSMAN->m_bShowLoadingWindow.
	LoadingWindow *pLoadingWindow = LoadingWindow::Create();
	if(pLoadingWindow == nullptr)
		RageException::Throw("%s", COULDNT_OPEN_LOADING_WINDOW.GetValue().c_str());

	srand( time(nullptr) ); // seed number generator

	/* Do this early, so we have debugging output if anything else fails. LOG and
	 * Dialog must be set up first. It shouldn't take long, but it might take a
	 * little time; do this after the LoadingWindow is shown, since we don't want
	 * that to appear delayed. */
	HOOKS->DumpDebugInfo();

#if defined(HAVE_TLS)
	LOG->Info( "TLS is %savailable", RageThread::GetSupportsTLS()? "":"not " );
#endif

	GAMEMAN		= new GameManager;
	THEME		= new ThemeManager;
	ANNOUNCER	= new AnnouncerManager;
	NOTESKIN	= new NoteSkinManager;

	// Switch to the last used game type, and set up the theme and announcer.
	SwitchToLastPlayedGame();

	CommandLineActions::Handle(pLoadingWindow);

	if( GetCommandlineArgument("dopefish") )
		GAMESTATE->m_bDopefish = true;

	{
		/* Now that THEME is loaded, load the icon and splash for the current
		 * theme into the loading window. */
		RString sError;
		RageSurface *pSurface = RageSurfaceUtils::LoadFile( THEME->GetPathG( "Common", "window icon" ), sError );
		if( pSurface != nullptr )
			pLoadingWindow->SetIcon( pSurface );
		delete pSurface;
		pSurface = RageSurfaceUtils::LoadFile( THEME->GetPathG("Common","splash"), sError );
		if( pSurface != nullptr )
			pLoadingWindow->SetSplash( pSurface );
		delete pSurface;
	}

	if( PREFSMAN->m_iSoundWriteAhead )
		LOG->Info( "Sound writeahead has been overridden to %i", PREFSMAN->m_iSoundWriteAhead.Get() );

	SOUNDMAN	= new RageSoundManager;
	SOUNDMAN->Init();
	SOUNDMAN->SetMixVolume();
	SOUND		= new GameSoundManager;
	BOOKKEEPER	= new Bookkeeper;
	LIGHTSMAN	= new LightsManager;
	INPUTFILTER	= new InputFilter;
	INPUTMAPPER	= new InputMapper;

	StepMania::InitializeCurrentGame( GAMESTATE->GetCurrentGame() );

	INPUTQUEUE	= new InputQueue;
	SONGINDEX	= new SongCacheIndex;
	IMAGECACHE	= new ImageCache;

	// depends on SONGINDEX:
	SONGMAN		= new SongManager;
	SONGMAN->InitAll( pLoadingWindow, /*onlyAdditions=*/false );	// this takes a long time
	CRYPTMAN	= new CryptManager;		// need to do this before ProfileMan
	if( PREFSMAN->m_bSignProfileData )
		CRYPTMAN->GenerateGlobalKeys();
	MEMCARDMAN	= new MemoryCardManager;
	CHARMAN		= new CharacterManager;
	PROFILEMAN	= new ProfileManager;
	PROFILEMAN->Init();				// must load after SONGMAN
	UNLOCKMAN	= new UnlockManager;
	SONGMAN->UpdatePopular();
	SONGMAN->UpdatePreferredSort();
	NETWORK		= new NetworkManager;
	STATSMAN	= new StatsManager;

	// Initialize which courses are ranking courses here.
	SONGMAN->UpdateRankingCourses();

	RageUtil::SafeDelete( pLoadingWindow ); // destroy this before init'ing Display

	/* If the user has tried to quit during the loading, do it before creating
	* the main window. This prevents going to full screen just to quit. */
	if( ArchHooks::UserQuit() )
	{
		ShutdownGame();
		return 0;
	}

	StartDisplay();

	StoreActualGraphicOptions();
	LOG->Info( "%s", GetActualGraphicOptionsString().c_str() );

	SONGMAN->PreloadSongImages();

	/* Input handlers can have dependences on the video system so
	 * INPUTMAN must be initialized after DISPLAY. */
	INPUTMAN	= new RageInput;

	// These things depend on the TextureManager, so do them after!
	FONT		= new FontManager;
	SCREENMAN	= new ScreenManager;

	StepMania::ResetGame();

	/* Now that GAMESTATE is reset, tell SCREENMAN to update the theme (load
	 * overlay screens and global sounds), and load the initial screen. */
	SCREENMAN->ThemeChanged();
	SCREENMAN->SetNewScreen( StepMania::GetInitialScreen() );

	// Do this after ThemeChanged so that we can show a system message
	RString sMessage;
	if( INPUTMAPPER->CheckForChangedInputDevicesAndRemap(sMessage) )
		SCREENMAN->SystemMessage( sMessage );

	CodeDetector::RefreshCacheItems();

	// Run the main loop.
	GameLoop::RunGameLoop();

	PREFSMAN->SavePrefsToDisk();

	ShutdownGame();

	return 0;
}

RString StepMania::SaveScreenshot( RString Dir, bool SaveCompressed, bool MakeSignature, RString NamePrefix, RString NameSuffix )
{
	/* As of sm-ssc v1.0 rc2, screenshots are no longer named by an arbitrary
	 * index. This was causing naming issues for some unknown reason, so we have
	 * changed the screenshot names to a non-blocking format: date and time.
	 * As before, we ignore the extension. -aj */
	RString FileNameNoExtension = NamePrefix + DateTime::GetNowDateTime().GetString() + NameSuffix;
	// replace space with underscore.
	FileNameNoExtension.Replace(" ","_");
	// colons are illegal in filenames.
	FileNameNoExtension.Replace(":","");

	// Save the screenshot. If writing lossy to a memcard, use
	// SAVE_LOSSY_LOW_QUAL, so we don't eat up lots of space.
	RageDisplay::GraphicsFileFormat fmt;
	if( SaveCompressed )
		fmt = RageDisplay::SAVE_LOSSY_HIGH_QUAL;
	else
		fmt = RageDisplay::SAVE_LOSSLESS_SENSIBLE;

	RString FileName = FileNameNoExtension + "." + (SaveCompressed ? "jpg" : "png");
	RString Path = Dir+FileName;

	if( !DISPLAY->SaveScreenshot(Path, fmt) )
	{
		SCREENMAN->PlayInvalidSound();
		return RString();
	}

	SCREENMAN->PlayScreenshotSound();

	if( PREFSMAN->m_bSignProfileData && MakeSignature )
		CryptManager::SignFileToFile( Path );

	return FileName;
}

void StepMania::InsertCoin( int iNum, bool bCountInBookkeeping )
{
	if( bCountInBookkeeping )
	{
		LIGHTSMAN->PulseCoinCounter();
		BOOKKEEPER->CoinInserted();
	}
	int iNumCoinsOld = GAMESTATE->m_iCoins;

	// Don't allow GAMESTATE's coin count to become negative.
	if (GAMESTATE->m_iCoins + iNum >= 0)
	{
		GAMESTATE->m_iCoins.Set( GAMESTATE->m_iCoins + iNum );
	}

	int iCredits = GAMESTATE->m_iCoins / PREFSMAN->m_iCoinsPerCredit;
	bool bMaxCredits = iCredits >= PREFSMAN->m_iMaxNumCredits;
	if( bMaxCredits )
	{
		GAMESTATE->m_iCoins.Set( PREFSMAN->m_iMaxNumCredits * PREFSMAN->m_iCoinsPerCredit );
	}

	LOG->Trace("%i coins inserted, %i needed to play", GAMESTATE->m_iCoins.Get(), PREFSMAN->m_iCoinsPerCredit.Get() );

    // On InsertCoin, make sure to update Coins file
    BOOKKEEPER->WriteCoinsFile(GAMESTATE->m_iCoins.Get());

	// If inserting coins, play an appropriate sound; if deducting coins, don't play anything.
	if (iNum > 0)
	{
		if( iNumCoinsOld != GAMESTATE->m_iCoins )
		{
			SCREENMAN->PlayCoinSound();
		} else {
			SCREENMAN->PlayInvalidSound();
		}
	}

	/* If AutoJoin and a player is already joined, then try to join a player.
	 * (If no players are joined, they'll join on the first JoinInput.) */
	if( GAMESTATE->m_bAutoJoin.Get() && GAMESTATE->GetNumSidesJoined() > 0 )
	{
		if( GAMESTATE->GetNumSidesJoined() > 0 && GAMESTATE->JoinPlayers() )
			SCREENMAN->PlayStartSound();
	}

	// TODO: remove this redundant message and things that depend on it
	Message msg( "CoinInserted" );
	// below params are unused
	//msg.SetParam( "Coins", GAMESTATE->m_iCoins );
	//msg.SetParam( "Inserted", iNum );
	//msg.SetParam( "MaxCredits", bMaxCredits );
	MESSAGEMAN->Broadcast( msg );
}

void StepMania::InsertCredit()
{
	InsertCoin( PREFSMAN->m_iCoinsPerCredit, false );
}

void StepMania::ClearCredits()
{
	LOG->Trace("%i coins cleared", GAMESTATE->m_iCoins.Get() );
	GAMESTATE->m_iCoins.Set( 0 );
	SCREENMAN->PlayInvalidSound();

    // Update Coins file to make sure credits are cleared.
    BOOKKEEPER->WriteCoinsFile(GAMESTATE->m_iCoins.Get());

	// TODO: remove this redundant message and things that depend on it
	Message msg( "CoinInserted" );
	// below params are unused
	//msg.SetParam( "Coins", GAMESTATE->m_iCoins );
	//msg.SetParam( "Clear", true );
	MESSAGEMAN->Broadcast( msg );
}

/* Returns true if the key has been handled and should be discarded, false if
 * the key should be sent on to screens. */
static LocalizedString SERVICE_SWITCH_PRESSED ( "StepMania", "Service switch pressed" );
static LocalizedString RELOADED_METRICS( "ThemeManager", "Reloaded metrics" );
static LocalizedString RELOADED_METRICS_AND_TEXTURES( "ThemeManager", "Reloaded metrics and textures" );
static LocalizedString RELOADED_SCRIPTS( "ThemeManager", "Reloaded scripts" );
static LocalizedString RELOADED_OVERLAY_SCREENS( "ThemeManager", "Reloaded overlay screens" );
bool HandleGlobalInputs( const InputEventPlus &input )
{
	// None of the globals keys act on types other than FIRST_PRESS
	if( input.type != IET_FIRST_PRESS )
		return false;

	switch( input.MenuI )
	{
		case GAME_BUTTON_OPERATOR:
			/* Global operator key, to get quick access to the options menu. Don't
			 * do this if we're on a "system menu", which includes the editor
			 * (to prevent quitting without storing changes). */
			if( SCREENMAN->AllowOperatorMenuButton() )
			{
				SCREENMAN->SystemMessage( SERVICE_SWITCH_PRESSED );
				SCREENMAN->PopAllScreens();
				GAMESTATE->Reset();
				SCREENMAN->SetNewScreen( CommonMetrics::OPERATOR_MENU_SCREEN );
			}
			return true;

		case GAME_BUTTON_COIN:
			// Handle a coin insertion.
			if( GAMESTATE->IsEditing() )	// no coins while editing
			{
				LOG->Trace( "Ignored coin insertion (editing)" );
				break;
			}
			StepMania::InsertCoin();
			return false; // Attract needs to know because it goes to TitleMenu on > 1 credit
		default: break;
	}

	/* Re-added for StepMania 3.9 theming veterans, plus it's just faster than
	 * the debug menu. The Shift button only reloads the metrics, unlike in 3.9
	 * (where it saved bookkeeping and machine profile). -aj */
	bool bIsShiftHeld = INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT), &input.InputList) ||
		INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_RSHIFT), &input.InputList);
	bool bIsCtrlHeld = INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_LCTRL), &input.InputList) ||
		INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_RCTRL), &input.InputList);
	if( input.DeviceI == DeviceInput(DEVICE_KEYBOARD, KEY_F2) )
	{
		if( bIsShiftHeld && !bIsCtrlHeld )
		{
			// Shift+F2: refresh metrics,noteskin cache and CodeDetector cache only
			THEME->ReloadMetrics();
			NOTESKIN->RefreshNoteSkinData( GAMESTATE->m_pCurGame );
			CodeDetector::RefreshCacheItems();
			SCREENMAN->SystemMessage( RELOADED_METRICS );
		}
		else if( bIsCtrlHeld && !bIsShiftHeld )
		{
			// Ctrl+F2: reload scripts only
			THEME->UpdateLuaGlobals();
			SCREENMAN->SystemMessage( RELOADED_SCRIPTS );
		}
		else if( bIsCtrlHeld && bIsShiftHeld )
		{
			// Shift+Ctrl+F2: reload overlay screens (and metrics, since themers
			// are likely going to do this after changing metrics.)
			THEME->ReloadMetrics();
			SCREENMAN->ReloadOverlayScreens();
			SCREENMAN->SystemMessage( RELOADED_OVERLAY_SCREENS );
		}
		else
		{
			// F2 alone: refresh metrics, textures, noteskins, codedetector cache
			THEME->ReloadMetrics();
			TEXTUREMAN->ReloadAll();
			NOTESKIN->RefreshNoteSkinData( GAMESTATE->m_pCurGame );
			CodeDetector::RefreshCacheItems();
			SCREENMAN->SystemMessage( RELOADED_METRICS_AND_TEXTURES );
		}

		return true;
	}

	if( input.DeviceI == DeviceInput(DEVICE_KEYBOARD, KEY_PAUSE) )
	{
		Message msg("ToggleConsoleDisplay");
		MESSAGEMAN->Broadcast( msg );
		return true;
	}

#if !defined(MACOSX)
	if( input.DeviceI == DeviceInput(DEVICE_KEYBOARD, KEY_F4) )
	{
		if( INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_RALT), &input.InputList) ||
			INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_LALT), &input.InputList) )
		{
			// pressed Alt+F4
			ArchHooks::SetUserQuit();
			return true;
		}
	}
#else
	if( input.DeviceI == DeviceInput(DEVICE_KEYBOARD, KEY_Cq) &&
		(INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_LMETA), &input.InputList ) ||
		 INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_RMETA), &input.InputList )) )
	{
		/* The user quit is handled by the menu item so we don't need to set it
		 * here; however, we do want to return that it has been handled since
		 * this will happen first. */
		return true;
	}
#endif

	bool bDoScreenshot =
#if defined(MACOSX)
	// Notebooks don't have F13. Use cmd-F12 as well.
		input.DeviceI == DeviceInput( DEVICE_KEYBOARD, KEY_PRTSC ) ||
		input.DeviceI == DeviceInput( DEVICE_KEYBOARD, KEY_F13 ) ||
		( input.DeviceI == DeviceInput(DEVICE_KEYBOARD, KEY_F12) &&
		  (INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_LMETA), &input.InputList) ||
		   INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RMETA), &input.InputList)) );
#else
	/* The default Windows message handler will capture the desktop window upon
	 * pressing PrntScrn, or will capture the foreground with focus upon pressing
	 * Alt+PrntScrn. Windows will do this whether or not we save a screenshot
	 * ourself by dumping the frame buffer. */
	// "if pressing PrintScreen and not pressing Alt"
		input.DeviceI == DeviceInput(DEVICE_KEYBOARD, KEY_PRTSC) &&
		!INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_LALT), &input.InputList) &&
		!INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RALT), &input.InputList);
#endif
	if( bDoScreenshot )
	{
		// If holding Shift save uncompressed, else save compressed
		bool bHoldingShift = ( INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT) )
								|| INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_RSHIFT) ) );
		bool bSaveCompressed = !bHoldingShift;
		RageTimer timer;
		StepMania::SaveScreenshot("Screenshots/", bSaveCompressed, false, "", "");
		LOG->Trace( "Screenshot took %f seconds.", timer.GetDeltaTime() );
		return true; // handled
	}

	if( input.DeviceI == DeviceInput(DEVICE_KEYBOARD, KEY_ENTER) &&
		(INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RALT), &input.InputList) ||
		 INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_LALT), &input.InputList)) )
	{
		// alt-enter
		/* In macOS, this is a menu item and will be handled as such. This will
		 * happen first and then the lower priority GUI thread will happen second,
		 * causing the window to toggle twice. Another solution would be to put
		 * a timer in ArchHooks::SetToggleWindowed() and just not set the bool
		 * it if it's been less than, say, half a second. */
#if !defined(MACOSX)
		ArchHooks::SetToggleWindowed();
#endif
		return true;
	}

	return false;
}

void HandleInputEvents(float fDeltaTime)
{
	INPUTFILTER->Update( fDeltaTime );

	/* Hack: If the topmost screen hasn't been updated yet, don't process input,
	 * since we must not send inputs to a screen that hasn't at least had one
	 * update yet. (The first Update should be the very first thing a screen gets.)
	 * We'll process it next time. Call Update above, so the inputs are
	 * read and timestamped. */
	if( SCREENMAN->GetTopScreen()->IsFirstUpdate() )
		return;

	std::vector<InputEvent> ieArray;
	INPUTFILTER->GetInputEvents( ieArray );

	// If we don't have focus, discard input.
	if( !HOOKS->AppHasFocus() )
		return;

	for( unsigned i=0; i<ieArray.size(); i++ )
	{
		InputEventPlus input;
		input.DeviceI = ieArray[i].di;
		input.type = ieArray[i].type;
		swap( input.InputList, ieArray[i].m_ButtonState );

		// hack for testing (MultiPlayer) with only one joystick
		/*
		if( input.DeviceI.IsJoystick() )
		{
			if( INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD,KEY_LSHIFT) ) )
				input.DeviceI.device = (InputDevice)(input.DeviceI.device + 1);
			if( INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD,KEY_LCTRL) ) )
				input.DeviceI.device = (InputDevice)(input.DeviceI.device + 2);
			if( INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD,KEY_LALT) ) )
				input.DeviceI.device = (InputDevice)(input.DeviceI.device + 4);
			if( INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD,KEY_RALT) ) )
				input.DeviceI.device = (InputDevice)(input.DeviceI.device + 8);
			if( INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD,KEY_RCTRL) ) )
				input.DeviceI.device = (InputDevice)(input.DeviceI.device + 16);
		}
		*/

		INPUTMAPPER->DeviceToGame( input.DeviceI, input.GameI );

		input.mp = MultiPlayer_Invalid;

		{
			// Translate input to the appropriate MultiPlayer. Assume that all
			// joystick devices are mapped the same as the master player.
			if( input.DeviceI.IsJoystick() )
			{
				DeviceInput diTemp = input.DeviceI;
				diTemp.device = DEVICE_JOY1;
				GameInput gi;

				//LOG->Trace( "device %d, %d", diTemp.device, diTemp.button );
				if( INPUTMAPPER->DeviceToGame(diTemp, gi) )
				{
					if( GAMESTATE->m_bMultiplayer )
					{
						input.GameI = gi;
						//LOG->Trace( "game %d %d", input.GameI.controller, input.GameI.button );
					}

					input.mp = InputMapper::InputDeviceToMultiPlayer( input.DeviceI.device );
					//LOG->Trace( "multiplayer %d", input.mp );
					ASSERT( input.mp >= 0 && input.mp < NUM_MultiPlayer );
				}
			}
		}

		if( input.GameI.IsValid() )
		{
			input.MenuI = INPUTMAPPER->GameButtonToMenuButton( input.GameI.button );
			input.pn = INPUTMAPPER->ControllerToPlayerNumber( input.GameI.controller );
		}

		INPUTQUEUE->RememberInput( input );

		// When a GameButton is pressed, stop repeating other keys on the same controller.
		if( input.type == IET_FIRST_PRESS && input.MenuI != GameButton_Invalid )
		{
			FOREACH_ENUM( GameButton,  m )
			{
				if( input.MenuI != m )
					INPUTMAPPER->RepeatStopKey( m, input.pn );
			}
		}

		if( HandleGlobalInputs(input) )
			continue;	// skip

		// check back in event mode
		if( GAMESTATE->IsEventMode() &&
			CodeDetector::EnteredCode(input.GameI.controller,CODE_BACK_IN_EVENT_MODE) )
		{
			input.MenuI = GAME_BUTTON_BACK;
		}

		SCREENMAN->Input( input );
	}

	if( ArchHooks::GetAndClearToggleWindowed() )
	{
		PREFSMAN->m_bWindowed.Set( !PREFSMAN->m_bWindowed );
		StepMania::ApplyGraphicOptions();
	}
}

#include "LuaManager.h"
int LuaFunc_SaveScreenshot(lua_State *L);
int LuaFunc_SaveScreenshot(lua_State *L)
{
	// If pn is provided, save to that player's profile.
	// Otherwise, save to the machine.
	PlayerNumber pn= Enum::Check<PlayerNumber>(L, 1, true);
	bool compress= lua_toboolean(L, 2) > 0;
	bool sign= lua_toboolean(L, 3) > 0;
	RString prefix= luaL_optstring(L, 4, "");
	RString suffix= luaL_optstring(L, 5, "");
	RString dir;
	if(pn == PlayerNumber_Invalid)
	{
		dir= "Screenshots/";
	}
	else
	{
		dir= PROFILEMAN->GetProfileDir((ProfileSlot)pn) + "Screenshots/";
		if(PROFILEMAN->ProfileWasLoadedFromMemoryCard(pn))
		{
			MEMCARDMAN->MountCard(pn);
		}
	}
	RString filename= StepMania::SaveScreenshot(dir, compress, sign, prefix, suffix);
	if(pn != PlayerNumber_Invalid)
	{
		if(PROFILEMAN->ProfileWasLoadedFromMemoryCard(pn))
		{
			MEMCARDMAN->UnmountCard(pn);
		}
	}
	RString path= dir + filename;
	lua_pushboolean(L, !filename.empty());
	lua_pushstring(L, path.c_str());
	return 2;
}
void LuaFunc_Register_SaveScreenshot(lua_State *L);
void LuaFunc_Register_SaveScreenshot(lua_State *L)
{ lua_register(L, "SaveScreenshot", LuaFunc_SaveScreenshot); }
REGISTER_WITH_LUA_FUNCTION(LuaFunc_Register_SaveScreenshot);

static int LuaFunc_update_centering(lua_State* L)
{
	update_centering();
	return 0;
}
LUAFUNC_REGISTER_COMMON(update_centering);

/*
 * (c) 2001-2004 Chris Danford, Glenn Maynard
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

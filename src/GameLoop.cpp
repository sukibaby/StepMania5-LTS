#include "global.h"
#include "GameLoop.h"
#include "RageLog.h"
#include "RageTextureManager.h"
#include "RageSoundManager.h"
#include "PrefsManager.h"
#include "RageDisplay.h"

#include "arch/ArchHooks/ArchHooks.h"

#include "GameSoundManager.h"
#include "ThemeManager.h"
#include "SongManager.h"
#include "GameState.h"
#include "MemoryCardManager.h"
#include "ScreenManager.h"
#include "InputFilter.h"
#include "InputMapper.h"
#include "RageFileManager.h"
#include "LightsManager.h"
#include "RageTimer.h"
#include "RageInput.h"

#include <cmath>
#include <vector>

static RageTimer g_GameplayTimer;

static Preference<bool> g_bNeverBoostAppPriority( "NeverBoostAppPriority", false );

/* experimental: force a specific update rate. This prevents big  animation
 * jumps on frame skips. 0 to disable. */
static Preference<float> g_fConstantUpdateDeltaSeconds( "ConstantUpdateDeltaSeconds", 0 );

void HandleInputEvents( float fDeltaTime );

static float g_fUpdateRate = 1;
void GameLoop::SetUpdateRate( float fUpdateRate )
{
	g_fUpdateRate = fUpdateRate;
}

float GameLoop::GetUpdateRate() {
	return g_fUpdateRate;
}

static void CheckGameLoopTimerSkips( float fDeltaTime )
{
	static int iLastFPS = 0;
	int iThisFPS = DISPLAY->GetFPS();

	/* If vsync is on, and we have a solid framerate (vsync == refresh and we've
	 * sustained this for at least one second), we expect the amount of time for
	 * the last frame to be 1/FPS. */
	if( iThisFPS != DISPLAY->GetActualVideoModeParams().rate || iThisFPS != iLastFPS )
	{
		iLastFPS = iThisFPS;
		return;
	}

	const float fExpectedTime = 1.0f / iThisFPS;
	const float fDifference = fDeltaTime - fExpectedTime;
	if( std::abs(fDifference) > 0.002f && std::abs(fDifference) < 0.100f )
		LOG->Trace( "GameLoop timer skip: %i FPS, expected %.3f, got %.3f (%.3f difference)",
			iThisFPS, fExpectedTime, fDeltaTime, fDifference );
}

static bool ChangeAppPri()
{
	if( g_bNeverBoostAppPriority.Get() )
		return false;

	// If this is a debug build, don't. It makes the VC debugger sluggish.
#if defined(WIN32) && defined(DEBUG)
	return false;
#else
	return true;
#endif
}

static void CheckFocus()
{
	if( !HOOKS->AppFocusChanged() )
		return;

	// If we lose focus, we may lose input events, especially key releases.
	INPUTFILTER->Reset();
}

static void CheckInputDevices()
{
	if (INPUTMAN->DevicesChanged())
	{
		INPUTFILTER->Reset();    // fix "buttons stuck" if button held while unplugged
		INPUTMAN->LoadDrivers();
		RString sMessage;
		if (INPUTMAPPER->CheckForChangedInputDevicesAndRemap(sMessage))
			SCREENMAN->SystemMessage(sMessage);
	}
}

// On the next update, change themes, and load sNewScreen.
static RString g_NewTheme;
static RString g_NewGame;
void GameLoop::ChangeTheme(const RString &sNewTheme)
{
	g_NewTheme = sNewTheme;
}

void GameLoop::ChangeGame(const RString& new_game, const RString& new_theme)
{
	g_NewGame= new_game;
	g_NewTheme= new_theme;
}

#include "StepMania.h" // XXX
#include "GameManager.h"
#include "Game.h"
namespace
{
	RString GetNewScreenName()
	{
		if (THEME->HasMetric("Common", "AfterThemeChangeScreen"))
		{
			RString after_screen = THEME->GetMetric("Common", "AfterThemeChangeScreen");
			if (SCREENMAN->IsScreenNameValid(after_screen))
			{
				return after_screen;
			}
		}

		RString new_screen = THEME->GetMetric("Common", "InitialScreen");
		if (!SCREENMAN->IsScreenNameValid(new_screen))
		{
			return "ScreenInitialScreenIsInvalid";
		}
		return new_screen;
	}

	void DoChangeTheme()
	{
		RageUtil::SafeDelete( SCREENMAN );
		TEXTUREMAN->DoDelayedDelete();

		// In case the previous theme overloaded class bindings, reinitialize them.
		LUA->RegisterTypes();

		// We always need to force the theme to reload because we cleared the lua
		// state by calling RegisterTypes so the scripts in Scripts/ need to run.
		THEME->SwitchThemeAndLanguage( g_NewTheme, THEME->GetCurLanguage(), PREFSMAN->m_bPseudoLocalize, true );
		PREFSMAN->m_sTheme.Set( g_NewTheme );

		// Apply the new window title, icon and aspect ratio.
		StepMania::ApplyGraphicOptions();

		SCREENMAN = new ScreenManager();

		StepMania::ResetGame();
		SCREENMAN->ThemeChanged();
		// The previous system for changing the theme fetched the "NextScreen"
		// metric from the current theme, then changed the theme, then tried to
		// set the new screen to the name that had been fetched.
		// If the new screen didn't exist in the new theme, there would be a
		// crash.
		// So now the correct thing to do is for a theme to specify its entry
		// point after a theme change, ensuring that we are going to a valid
		// screen and not crashing. -Kyz
		RString newScreenName = GetNewScreenName();
		SCREENMAN->SetNewScreen(newScreenName);

		// Indicate no further theme change is needed
		g_NewTheme = RString();
	}

	void DoChangeGame()
	{
		const Game* g= GAMEMAN->StringToGame(g_NewGame);
		ASSERT(g != nullptr);
		GAMESTATE->SetCurGame(g);

		bool theme_changing= false;
		// The prefs allow specifying a different default theme to use for each
		// game type.  So if a theme name isn't passed in, fetch from the prefs.
		if(g_NewTheme.empty())
		{
			g_NewTheme= PREFSMAN->m_sTheme;
		}
		if(g_NewTheme != THEME->GetCurThemeName() && THEME->IsThemeSelectable(g_NewTheme))
		{
			theme_changing= true;
		}

		if(theme_changing)
		{
			RageUtil::SafeDelete(SCREENMAN);
			TEXTUREMAN->DoDelayedDelete();
			LUA->RegisterTypes();
			THEME->SwitchThemeAndLanguage(g_NewTheme, THEME->GetCurLanguage(),
				PREFSMAN->m_bPseudoLocalize);
			PREFSMAN->m_sTheme.Set(g_NewTheme);
			StepMania::ApplyGraphicOptions();
			SCREENMAN= new ScreenManager();
		}
		StepMania::ResetGame();
		RString new_screen= THEME->GetMetric("Common", "InitialScreen");
		RString after_screen;
		if(theme_changing)
		{
			SCREENMAN->ThemeChanged();
			if(THEME->HasMetric("Common", "AfterGameAndThemeChangeScreen"))
			{
				after_screen= THEME->GetMetric("Common", "AfterGameAndThemeChangeScreen");
			}
		}
		else
		{
			if(THEME->HasMetric("Common", "AfterGameChangeScreen"))
			{
				after_screen= THEME->GetMetric("Common", "AfterGameChangeScreen");
			}
		}
		if(SCREENMAN->IsScreenNameValid(after_screen))
		{
			new_screen= after_screen;
		}
		SCREENMAN->SetNewScreen(new_screen);

		// Set the input scheme for the new game, and load keymaps.
		if( INPUTMAPPER )
		{
			INPUTMAPPER->SetInputScheme(&g->m_InputScheme);
			INPUTMAPPER->ReadMappingsFromDisk();
		}
		// aj's comment transplanted from ScreenOptionsMasterPrefs.cpp:GameSel. -Kyz
		/* Reload metrics to force a refresh of CommonMetrics::DIFFICULTIES_TO_SHOW,
		 * mainly if we're not switching themes. I'm not sure if this was the
		 * case going from theme to theme, but if it was, it should be fixed
		 * now. There's probably be a better way to do it, but I'm not sure
		 * what it'd be. -aj */
		THEME->UpdateLuaGlobals();
		THEME->ReloadMetrics();
		g_NewGame= RString();
		g_NewTheme= RString();
	}
}

void GameLoop::UpdateAllButDraw(bool bRunningFromVBLANK)
{
	// Flag to indicate whether an update has been processed during the VBLANK period.
	static bool m_bUpdatedDuringVBLANK = false;

	// If we're running from VBLANK, and we've already updated during the VBLANK period,
	// don't update again. This is to prevent multiple updates during the same VBLANK period.
	if (!bRunningFromVBLANK && m_bUpdatedDuringVBLANK)
	{
		m_bUpdatedDuringVBLANK = false;
		return;
	}

	// If we're running from VBLANK, indicate we've updated during the VBLANK period.
	// Otherwise, make sure the flag is cleared.
	if (bRunningFromVBLANK)
	{
		m_bUpdatedDuringVBLANK = true;
	}
	else
	{
		m_bUpdatedDuringVBLANK = false;
	}

	// If the constant update delta is set, use that value. Otherwise, use the delta
	// time from the gameplay timer.
	float fDeltaTime = (g_fConstantUpdateDeltaSeconds > 0) 
		? g_fConstantUpdateDeltaSeconds 
		: g_GameplayTimer.GetDeltaTime();

	// Use a static boolean to check the preference once per game launch.
	// This is a rarely used debug feature, so we try to skip it if possible.
	static bool bLogSkips = PREFSMAN->m_bLogSkips;
	if (bLogSkips)
	{
		CheckGameLoopTimerSkips(fDeltaTime);
	}

	fDeltaTime *= g_fUpdateRate;

	// Update SOUNDMAN early (before any RageSound::GetPosition calls), to flush position data.
	SOUNDMAN->Update();

	/* Update song beat information -before- calling update on all the classes that
	 * depend on it. If you don't do this first, the classes are all acting on old
	 * information and will lag. (but no longer fatally, due to timestamping -glenn) */
	SOUND->Update(fDeltaTime);
	TEXTUREMAN->Update(fDeltaTime);
	GAMESTATE->Update(fDeltaTime);
	SCREENMAN->Update(fDeltaTime);
	MEMCARDMAN->Update();

	/* Important: Process input AFTER updating game logic, or input will be
	 * acting on song beat from last frame */
	HandleInputEvents(fDeltaTime);

	// Update the lights
	LIGHTSMAN->Update(fDeltaTime);
}

void GameLoop::RunGameLoop()
{
	/* People may want to do something else while songs are loading, so do
	 * this after loading songs. */
	if( ChangeAppPri() )
		HOOKS->BoostPriority();

	while( !ArchHooks::UserQuit() )
	{
		if(!g_NewGame.empty())
		{
			DoChangeGame();
		}
		if(!g_NewTheme.empty())
		{
			DoChangeTheme();
		}

		CheckFocus();

		UpdateAllButDraw(false);
		
		// Check input devices every 255 frames (uint8_t can hold 0-255).
		static uint8_t i_CheckInputDevices = 0;
		if (++i_CheckInputDevices == 0)
		{
			CheckInputDevices();
		}
		
		SCREENMAN->Draw();
	}

	// If we ended mid-game, finish up.
	GAMESTATE->SaveLocalData();

	if( ChangeAppPri() )
		HOOKS->UnBoostPriority();
}

class ConcurrentRenderer
{
public:
	ConcurrentRenderer();
	~ConcurrentRenderer();

	void Start();
	void Stop();

private:
	RageThread m_Thread;
	RageEvent m_Event;
	bool m_bShutdown;
	void RenderThread();
	static int StartRenderThread( void *p );

	enum State { RENDERING_IDLE, RENDERING_START, RENDERING_ACTIVE, RENDERING_END };
	State m_State;
};
static ConcurrentRenderer *g_pConcurrentRenderer = nullptr;

ConcurrentRenderer::ConcurrentRenderer():
	m_Event("ConcurrentRenderer")
{
	m_bShutdown = false;
	m_State = RENDERING_IDLE;

	m_Thread.SetName( "ConcurrentRenderer" );
	m_Thread.Create( StartRenderThread, this );
}

ConcurrentRenderer::~ConcurrentRenderer()
{
	ASSERT( m_State == RENDERING_IDLE );
	m_bShutdown = true;
	m_Thread.Wait();
}

void ConcurrentRenderer::Start()
{
	DISPLAY->BeginConcurrentRenderingMainThread();

	m_Event.Lock();
	ASSERT( m_State == RENDERING_IDLE );
	m_State = RENDERING_START;
	m_Event.Signal();
	while( m_State != RENDERING_ACTIVE )
		m_Event.Wait();
	m_Event.Unlock();
}

void ConcurrentRenderer::Stop()
{
	m_Event.Lock();
	ASSERT( m_State == RENDERING_ACTIVE );
	m_State = RENDERING_END;
	m_Event.Signal();
	while( m_State != RENDERING_IDLE )
		m_Event.Wait();
	m_Event.Unlock();

	DISPLAY->EndConcurrentRenderingMainThread();
}

void ConcurrentRenderer::RenderThread()
{
	ASSERT( SCREENMAN != nullptr );

	while( !m_bShutdown )
	{
		m_Event.Lock();
		while( m_State == RENDERING_IDLE && !m_bShutdown )
			m_Event.Wait();
		m_Event.Unlock();

		if( m_State == RENDERING_START )
		{
			/* We're starting to render. Set up, and then kick the event to wake
			 * up the calling thread. */
			DISPLAY->BeginConcurrentRendering();
			HOOKS->SetupConcurrentRenderingThread();

			LOG->Trace( "ConcurrentRenderer::RenderThread start" );

			m_Event.Lock();
			m_State = RENDERING_ACTIVE;
			m_Event.Signal();
			m_Event.Unlock();
		}

		/* This is started during Update(). The next thing the game loop
		 * will do is Draw, so shift operations around to put Draw at the
		 * top. This makes sure updates are seamless. */
		if( m_State == RENDERING_ACTIVE )
		{
			SCREENMAN->Draw();

			float fDeltaTime = g_GameplayTimer.GetDeltaTime();
			SCREENMAN->Update( fDeltaTime );
		}

		if( m_State == RENDERING_END )
		{
			LOG->Trace( "ConcurrentRenderer::RenderThread done" );

			DISPLAY->EndConcurrentRendering();

			m_Event.Lock();
			m_State = RENDERING_IDLE;
			m_Event.Signal();
			m_Event.Unlock();
		}
	}
}

int ConcurrentRenderer::StartRenderThread( void *p )
{
	((ConcurrentRenderer *) p)->RenderThread();
	return 0;
}

void GameLoop::StartConcurrentRendering()
{
	if( g_pConcurrentRenderer == nullptr )
		g_pConcurrentRenderer = new ConcurrentRenderer;
	g_pConcurrentRenderer->Start();
}

void GameLoop::FinishConcurrentRendering()
{
	g_pConcurrentRenderer->Stop();
}

/*
 * (c) 2001-2005 Chris Danford, Glenn Maynard
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

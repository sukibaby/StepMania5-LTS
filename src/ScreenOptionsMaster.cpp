#include "global.h"

#include "ScreenOptionsMaster.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "ThemeManager.h"
#include "GameManager.h"
#include "GameState.h"
#include "ScreenManager.h"
#include "SongManager.h"
#include "ProfileManager.h"
#include "PrefsManager.h"
#include "StepMania.h"
#include "RageSoundManager.h"
#include "OptionRowHandler.h"
#include "ScreenOptionsMasterPrefs.h"
#include "CommonMetrics.h"
#include "GameLoop.h"

#include <array>
#include <vector>

#define LINE_NAMES			THEME->GetMetric (m_sName,"LineNames")
#define LINE(sLineName)		THEME->GetMetric (m_sName,ssprintf("Line%s",sLineName.c_str()))
#define FORCE_ALL_PLAYERS	THEME->GetMetricB(m_sName,"ForceAllPlayers")
#define INPUT_MODE			THEME->GetMetric (m_sName,"InputMode")
#define NAVIGATION_MODE		THEME->GetMetric (m_sName,"NavigationMode")

REGISTER_SCREEN_CLASS( ScreenOptionsMaster );

void ScreenOptionsMaster::Init()
{
	std::vector<RString> asLineNames;
	split( LINE_NAMES, ",", asLineNames );
	if( asLineNames.empty() )
	{
		LuaHelpers::ReportScriptErrorFmt("\"%s:LineNames\" is empty.", m_sName.c_str());
	}

	if( FORCE_ALL_PLAYERS )
	{
		FOREACH_PlayerNumber( pn )
			GAMESTATE->JoinPlayer( pn );
	}

	if( NAVIGATION_MODE == "toggle" )
		SetNavigation( PREFSMAN->m_iArcadeOptionsNavigation? NAV_TOGGLE_THREE_KEY:NAV_TOGGLE_FIVE_KEY );
	else if( NAVIGATION_MODE == "menu" )
		SetNavigation( NAV_THREE_KEY_MENU );

	SetInputMode( StringToInputMode(INPUT_MODE) );

	// Call this after enabling players, if any.
	ScreenOptions::Init();

	std::vector<OptionRowHandler*> OptionRowHandlers;
	for( unsigned i = 0; i < asLineNames.size(); ++i )
	{
		RString sLineName = asLineNames[i];
		RString sRowCommands = LINE(sLineName);

		Commands cmds;
		ParseCommands( sRowCommands, cmds, false );

		OptionRowHandler *pHand = OptionRowHandlerUtil::Make( cmds );
		if( pHand == nullptr )
		{
			LuaHelpers::ReportScriptErrorFmt("Invalid OptionRowHandler \"%s\" in \"%s:Line:%s\".", cmds.GetOriginalCommandString().c_str(), m_sName.c_str(), sLineName.c_str());
		}
		else
		{
			OptionRowHandlers.push_back( pHand );
		}
	}
	InitMenu( OptionRowHandlers );
}

void ScreenOptionsMaster::ImportOptions( int r, const std::vector<PlayerNumber> &vpns )
{
	for (PlayerNumber const &pn : vpns)
	{
		ASSERT( GAMESTATE->IsHumanPlayer(pn) );
	}
	OptionRow &row = *m_pRows[r];
	row.ImportOptions( vpns );
}

void ScreenOptionsMaster::ExportOptions( int r, const std::vector<PlayerNumber> &vpns )
{
	CHECKPOINT_M( ssprintf("%i/%i", r, int(m_pRows.size())) );

	OptionRow &row = *m_pRows[r];
	bool bRowHasFocus[NUM_PLAYERS];
	ZERO( bRowHasFocus );
	for (PlayerNumber const &p : vpns)
	{
		int iCurRow = m_iCurrentRow[p];
		bRowHasFocus[p] = iCurRow == r;
	}
	m_iChangeMask |= row.ExportOptions( vpns, bRowHasFocus );
}

void ScreenOptionsMaster::HandleScreenMessage( const ScreenMessage SM )
{
	if( SM == SM_ExportOptions )
	{
		// Override ScreenOptions's calling of ExportOptions
		m_iChangeMask = 0;

		CHECKPOINT_M("Starting the export handling.");

		std::vector<PlayerNumber> vpns;
		FOREACH_OptionsPlayer( p )
			vpns.push_back( p );
		for( unsigned r=0; r<m_pRows.size(); r++ ) // foreach row
			ExportOptions( r, vpns );

		if (m_iChangeMask & OPT_APPLY_PROFILES)
		{
			// If we're changing profile settings such as sort order, we need to reload them
			PROFILEMAN->RefreshLocalProfilesFromDisk();
		}

		if( m_iChangeMask & OPT_APPLY_ASPECT_RATIO )
		{
			THEME->UpdateLuaGlobals();		// This needs to be done before resetting the projection matrix below
			THEME->ReloadSubscribers();	// SCREEN_* has changed, so re-read all subscribing ThemeMetrics
			SCREENMAN->ThemeChanged();		// recreate ScreenSystemLayer and SharedBGA
		}

		/* If the theme changes, we need to reset RageDisplay to apply the new window
		 * title and icon. If the aspect ratio changes, we need to reset RageDisplay
		 * so that the projection matrix is re-created using the new screen dimensions. */
		if( (m_iChangeMask & OPT_APPLY_THEME) ||
			(m_iChangeMask & OPT_APPLY_GRAPHICS) ||
			(m_iChangeMask & OPT_APPLY_ASPECT_RATIO) )
		{
			/* If the resolution or aspect ratio changes, always reload the theme.
			 * Otherwise, only reload it if it changed. */
			RString sNewTheme = PREFSMAN->m_sTheme.Get();
			GameLoop::ChangeTheme(sNewTheme);
		}

		if( m_iChangeMask & OPT_SAVE_PREFERENCES )
		{
			// Save preferences.
			LOG->Trace("ROW_CONFIG used; saving ...");
			PREFSMAN->SavePrefsToDisk();
		}

		if( m_iChangeMask & OPT_CHANGE_GAME )
		{
			GameLoop::ChangeGame(PREFSMAN->GetCurrentGame());
		}

		if( m_iChangeMask & OPT_APPLY_SOUND )
		{
			SOUNDMAN->SetMixVolume();
		}

		if( m_iChangeMask & OPT_APPLY_SONG )
			SONGMAN->SetPreferences();

		if( m_iChangeMask & OPT_RELOAD_SONGS )
		{
			SCREENMAN->GetTopScreen()->SetNextScreenName("ScreenReloadSongs");
			StartTransitioningScreen( SM_GoToNextScreen );
		}

		CHECKPOINT_M("Transferring to the next screen now.");
		this->HandleScreenMessage( SM_GoToNextScreen );
		return;
	}
	else
		ScreenOptions::HandleScreenMessage( SM );
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

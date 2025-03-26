#include "global.h"
#include "SongManager.h"
#include "RageFile.h"
#include "RageUtil.h"
#include "RageFileManager.h"
#include "RageLog.h"
#include "Song.h"
#include "SongCacheIndex.h"
#include "SongUtil.h"
#include "TitleSubstitution.h"
#include "Group.h"

#include <cstddef>
#include <tuple>
#include <vector>


/** @brief The file that contains the group information.
 * We name this Pack.ini over Group.ini to avoid conflict
 * with the Waiei Group.ini-lua project still actively
 * being developed.
*/
const RString INI_FILE = "Pack.ini";
const int INI_VERSION = 1;

Group::Group() {
    m_sDisplayTitle = "";
    m_sSortTitle = "";
    m_sPath = "";
    m_sGroupName = "";
    m_sTranslitTitle = "";
    m_sSeries = "";
    m_fSyncOffset = 0.0f;
    m_bHasPackIni = false;
    m_iYearReleased = 0;
    m_sBannerPath = "";
}

Group::~Group() 
{

}

Group::Group(const RString& sDir, const RString& sGroupDirName, bool bFromProfile) {
    RString sPackIniPath;
    if (bFromProfile) {
        sPackIniPath = sDir + "/" + INI_FILE;
        m_sPath = sDir;
        m_sGroupName = sDir.substr(1, sDir.find('/', 1) - 1);
        m_sDisplayTitle = sGroupDirName;
    } else {
        sPackIniPath = sDir + sGroupDirName + "/" + INI_FILE;
        m_sPath = sDir + sGroupDirName;
        m_sGroupName = Basename(sGroupDirName);
        m_sDisplayTitle = m_sGroupName;
    }
    m_sSortTitle = m_sGroupName;
    m_sTranslitTitle = m_sGroupName;
    m_sSeries = "";
    m_fSyncOffset = 0.0f;
    m_bHasPackIni = false;
    m_iYearReleased = 0;
    m_sBannerPath = "";
    m_iVersion = INI_VERSION;
    
    if (FILEMAN->DoesFileExist(sPackIniPath)) {
        IniFile ini;
        ini.ReadFile(sPackIniPath);

        RString sVersion = "";
        ini.GetValue("Group", "Version", sVersion);
        Trim(sVersion);

        // Only read the Pack.ini if the version is set
        // Otherwise log a warning and use default Pack.ini values
        if (!sVersion.empty()) {
            m_bHasPackIni = true;
            m_iVersion = StringToInt(sVersion);

            // Define a vector of key-value pairs to cleanly iterate
            std::vector<std::pair<RString, RString&>> vPackfields = {
                {"DisplayTitle", m_sDisplayTitle},
                {"SortTitle", m_sSortTitle},
                {"TranslitTitle", m_sTranslitTitle},
                {"Series", m_sSeries},
                {"Banner", m_sBannerPath}
            };

            for (auto& [key, value] : vPackfields) {
                ini.GetValue("Group", key, value);
                Trim(value);

                if (value.empty()) {
                    if (key == "DisplayTitle" || key == "SortTitle" ) {
                        value = m_sGroupName;
                    } else if (key == "TranslitTitle") {
                        value = m_sDisplayTitle;
                    } 
                }
            }
            
            RString sValue = "";
            ini.GetValue("Group", "SyncOffset", sValue);
            Trim(sValue);
            // Handle the group offset application.
            // 
            // The goal of Pack.ini is to make everything NULL synced internally.
            // 
            // The machine sync offset preference corresponds to what offset the user
            // would like to assume as a "default" for packs without a Pack.ini.
            //
            // If the machine sync bias is set to ITG, we want to remove the 9ms ITG bias
            // for all packs without a Pack.ini.
            // 
            // If the Pack.ini is specified (regardless of the machine sync bias) with
            // SyncOffset=ITG, we need to always remove the 9ms ITG bias.
            // Otherwise, if SyncOffset=NULL, we don't need to do anything since it's
            // already NULL synced.
            m_fSyncOffset = (PREFSMAN->m_DefaultSyncOffset == SyncOffset_NULL) ? 0.0f : -0.009f;
            if (!sValue.empty()) {
                if (sValue == "NULL") {
                    m_fSyncOffset = 0.0f;
                }
                else if (sValue == "ITG") {
                    m_fSyncOffset = -0.009f;
                }
                else {
                    LOG->Warn("Group::Group: Invalid SyncOffset value: %s in Pack.ini. Valid values are NULL and ITG. Using default value.", sValue.c_str());
                }
            }

            ini.GetValue("Group", "Year", m_iYearReleased);
        } else {
            LOG->Warn("Group::Group: Pack.ini version not set. Using default values.");
        }  
    }
}

const std::vector<Song *> &Group::GetSongs() const
{
    return SONGMAN->GetSongs(m_sGroupName);
}



#include "LuaBinding.h"

class LunaGroup: public Luna<Group>
{
public:
	static int GetGroupName(T* p, lua_State *L)
	{
		lua_pushstring(L, p->GetGroupName());
		return 1;
	}
	static int GetSortTitle(T* p, lua_State *L)
	{
		lua_pushstring(L, p->GetSortTitle());
        return 1;
    }

    static int GetDisplayTitle(T* p, lua_State *L)
    {
        lua_pushstring(L, p->GetDisplayTitle());
        return 1;
    }

    static int GetTranslitTitle(T* p, lua_State *L)
    {
        lua_pushstring(L, p->GetTranslitTitle());
        return 1;
    }

    static int GetSeries(T* p, lua_State *L)
    {
        lua_pushstring(L, p->GetSeries());
        return 1;
    }

    static int GetSyncOffset(T* p, lua_State *L)
    {
        lua_pushnumber(L, p->GetSyncOffset());
        return 1;
    }

    static int HasPackIni(T* p, lua_State *L)
    {
        lua_pushboolean(L, p->HasPackIni());
        return 1;
    }

    static int GetBannerPath(T* p, lua_State *L)
    {
        lua_pushstring(L, p->GetBannerPath());
        return 1;
    }

    static int GetYearReleased(T* p, lua_State *L)
    {
        lua_pushnumber(L, p->GetYearReleased());
        return 1;
    }

	LunaGroup()
	{
		ADD_METHOD(GetGroupName);
		ADD_METHOD(GetSortTitle);
		ADD_METHOD(GetDisplayTitle);
		ADD_METHOD(GetTranslitTitle);
		ADD_METHOD(GetSeries);
		ADD_METHOD(GetSyncOffset);
		ADD_METHOD(HasPackIni);
		ADD_METHOD(GetBannerPath);
        ADD_METHOD(GetYearReleased);
	}

};

LUA_REGISTER_CLASS(Group)

// lua end

/***************************************************************************
 * resource_manager.cpp  -  Resource Manager
 *
 * Copyright © 2009 - 2011 Florian Richter
 * Copyright © 2012-2017 The TSC Contributors
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#if defined(_WIN32)
#include <windows.h>
#elif defined(__unix__)
#include <limits.h>
#endif

#include "resource_manager.hpp"
#include "filesystem.hpp"
#include "../property_helper.hpp"
#include "../errors.hpp"
#include "../global_basic.hpp"
#include "../../user/preferences.hpp"

using namespace std;

namespace fs = boost::filesystem;
namespace errc = boost::system::errc;

namespace TSC {

/* *** *** *** *** *** *** cResource_Manager *** *** *** *** *** *** *** *** *** *** *** */

cResource_Manager::cResource_Manager(void)
{
    init_directories();

    // Note cResource_Manager is instanciated only once globally, therefore
    // printing this information here is fine.
    debug_print("Game data directory: %s\n", path_to_utf8(m_paths.game_data_dir).c_str());
    debug_print("User data directory: %s\n", path_to_utf8(m_paths.user_data_dir).c_str());
    debug_print("User cache directory: %s\n", path_to_utf8(m_paths.user_cache_dir).c_str());
    debug_print("User config directory: %s\n", path_to_utf8(m_paths.user_config_dir).c_str());
}

cResource_Manager::~cResource_Manager(void)
{

}

fs::path cResource_Manager::Get_Game_Data_Directory()
{
    return m_paths.game_data_dir;
}

void cResource_Manager::Init_User_Directory(void)
{
    // Create savegame directory
    if (!Dir_Exists(Get_User_Savegame_Directory())) {
        fs::create_directories(Get_User_Savegame_Directory());
    }
    // Create screenshot directory
    if (!Dir_Exists(Get_User_Screenshot_Directory())) {
        fs::create_directories(Get_User_Screenshot_Directory());
    }
    // Create level directory
    if (!Dir_Exists(Get_User_Level_Directory())) {
        fs::create_directories(Get_User_Level_Directory());
    }
    // Create world directory
    if (!Dir_Exists(Get_User_World_Directory())) {
        fs::create_directories(Get_User_World_Directory());
    }
    // Create campaign directory
    if (!Dir_Exists(Get_User_Campaign_Directory())) {
        fs::create_directories(Get_User_Campaign_Directory());
    }
    // Create scripting directory
    if (!Dir_Exists(Get_User_Scripting_Directory())) {
        fs::create_directories(Get_User_Scripting_Directory());
    }
    // Create cache directory
    if (!Dir_Exists(Get_User_Imgcache_Directory())) {
        fs::create_directories(Get_User_Imgcache_Directory());
    }
    // Create config directory
    if (!Dir_Exists(m_paths.user_config_dir)) {
        fs::create_directories(m_paths.user_config_dir);
    }
}

fs::path cResource_Manager::Get_Game_Pixmaps_Directory()
{
    return m_paths.game_data_dir / utf8_to_path(GAME_PIXMAPS_DIR);
}

fs::path cResource_Manager::Get_Game_Pixmap(std::string pixmap)
{
    return Get_Game_Pixmaps_Directory() / utf8_to_path(pixmap);
}

fs::path cResource_Manager::Get_User_Data_Directory()
{
    return m_paths.user_data_dir;
}

fs::path cResource_Manager::Get_User_Level_Directory()
{
    return m_paths.user_data_dir / utf8_to_path(USER_LEVEL_DIR);
}

fs::path cResource_Manager::Get_User_Level(std::string filename)
{
    return Get_User_Level_Directory() / utf8_to_path(filename);
}

fs::path cResource_Manager::Get_User_World(std::string filename)
{
    return Get_User_World_Directory() / utf8_to_path(filename);
}

fs::path cResource_Manager::Get_User_Pixmap(std::string pixmap)
{
    return Get_User_Pixmaps_Directory() / utf8_to_path("pixmaps") / utf8_to_path(pixmap);
}

fs::path cResource_Manager::Get_User_Savegame_Directory()
{
    return m_paths.user_data_dir / utf8_to_path(USER_SAVEGAME_DIR);
}

fs::path cResource_Manager::Get_User_Screenshot_Directory()
{
    return m_paths.user_data_dir / utf8_to_path(USER_SCREENSHOT_DIR);
}

fs::path cResource_Manager::Get_User_Scripting_Directory()
{
    return m_paths.user_data_dir / utf8_to_path(USER_SCRIPTING_DIR);
}

fs::path cResource_Manager::Get_User_World_Directory()
{
    return m_paths.user_data_dir / utf8_to_path(USER_WORLD_DIR);
}

fs::path cResource_Manager::Get_User_Campaign_Directory()
{
    return m_paths.user_data_dir / utf8_to_path(USER_CAMPAIGN_DIR);
}

fs::path cResource_Manager::Get_User_Imgcache_Directory()
{
    return m_paths.user_cache_dir / utf8_to_path(USER_IMGCACHE_DIR);
}

fs::path cResource_Manager::Get_User_Pixmaps_Directory()
{
    std::string resolution = int_to_string(pPreferences->m_video_screen_w) + "x" + int_to_string(pPreferences->m_video_screen_h);
    return Get_User_Imgcache_Directory() / utf8_to_path(resolution);
}

fs::path cResource_Manager::Get_User_CEGUI_Logfile()
{
    return m_paths.user_cache_dir / utf8_to_path("cegui.log");
}

fs::path cResource_Manager::Get_User_GameConsole_Logfile()
{
    return m_paths.user_cache_dir / path_to_utf8("gameconsole.log");
}

fs::path cResource_Manager::Get_Game_Schema_Directory()
{
    return m_paths.game_data_dir / utf8_to_path(GAME_SCHEMA_DIR);
}

fs::path cResource_Manager::Get_Game_Schema(std::string schema)
{
    return Get_Game_Schema_Directory() / utf8_to_path(schema);
}

fs::path cResource_Manager::Get_Game_Level_Directory()
{
    return m_paths.game_data_dir / utf8_to_path(GAME_LEVEL_DIR);
}

fs::path cResource_Manager::Get_Game_Level(std::string level)
{
    return Get_Game_Level_Directory() / utf8_to_path(level);
}

fs::path cResource_Manager::Get_Game_Translation_Directory()
{
    return m_paths.game_data_dir / utf8_to_path(GAME_TRANSLATION_DIR);
}

fs::path cResource_Manager::Get_Game_Translation(std::string transname)
{
    return Get_Game_Translation_Directory() / utf8_to_path(transname);
}

fs::path cResource_Manager::Get_Game_Sounds_Directory()
{
    return m_paths.game_data_dir / utf8_to_path(GAME_SOUNDS_DIR);
}

fs::path cResource_Manager::Get_Game_Sound(std::string sound)
{
    return Get_Game_Sounds_Directory() / utf8_to_path(sound);
}

fs::path cResource_Manager::Get_Game_Campaign_Directory()
{
    return m_paths.game_data_dir / utf8_to_path(GAME_CAMPAIGN_DIR);
}

fs::path cResource_Manager::Get_Game_Campaign(std::string campaign)
{
    return Get_Game_Campaign_Directory() / utf8_to_path(campaign);
}

fs::path cResource_Manager::Get_Game_Overworld_Directory()
{
    return m_paths.game_data_dir / utf8_to_path(GAME_OVERWORLD_DIR);
}

fs::path cResource_Manager::Get_Game_Overworld(std::string overworld)
{
    return Get_Game_Overworld_Directory() / utf8_to_path(overworld);
}

fs::path cResource_Manager::Get_Gui_Scheme_Directory()
{
    return m_paths.game_data_dir / utf8_to_path(GUI_SCHEME_DIR);
}

fs::path cResource_Manager::Get_Gui_Imageset_Directory()
{
    return m_paths.game_data_dir / utf8_to_path(GUI_IMAGESET_DIR);
}

fs::path cResource_Manager::Get_Gui_Font_Directory()
{
    return m_paths.game_data_dir / utf8_to_path(GUI_FONT_DIR);
}

fs::path cResource_Manager::Get_Gui_LookNFeel_Directory()
{
    return m_paths.game_data_dir / utf8_to_path(GUI_LOOKNFEEL_DIR);
}

fs::path cResource_Manager::Get_Gui_Layout_Directory()
{
    return m_paths.game_data_dir / utf8_to_path(GUI_LAYOUT_DIR);
}

fs::path cResource_Manager::Get_Gui_XmlSchema_Directory()
{
    return m_paths.game_data_dir / utf8_to_path(GUI_XMLSCHEMA_DIR);
}


fs::path cResource_Manager::Get_Game_Music_Directory()
{
    return m_paths.game_data_dir / utf8_to_path(GAME_MUSIC_DIR);
}

fs::path cResource_Manager::Get_Game_Music(std::string music)
{
    return Get_Game_Music_Directory() / utf8_to_path(music);
}

fs::path cResource_Manager::Get_Game_Editor_Directory()
{
    return m_paths.game_data_dir / utf8_to_path(GAME_EDITOR_DIR);
}

fs::path cResource_Manager::Get_Game_Editor(std::string editor)
{
    return Get_Game_Editor_Directory() / utf8_to_path(editor);
}

fs::path cResource_Manager::Get_Game_Scripting_Directory()
{
    return m_paths.game_data_dir / utf8_to_path(GAME_SCRIPTING_DIR);
}

fs::path cResource_Manager::Get_Game_Scripting(std::string script)
{
    return Get_Game_Scripting_Directory() / utf8_to_path(script);
}

fs::path cResource_Manager::Get_Game_Icon_Directory()
{
    return m_paths.game_data_dir / utf8_to_path(GAME_ICON_DIR);
}

fs::path cResource_Manager::Get_Game_Icon(std::string icon)
{
    return Get_Game_Icon_Directory() / utf8_to_path(icon);
}

fs::path cResource_Manager::Get_Preferences_File()
{
    return m_paths.user_config_dir / path_to_utf8("config.xml");
}

void cResource_Manager::init_directories()
{
    ////////// The (usually unwritable) game data directory //////////
#ifdef _WIN32
    // Windows has no standard pathes like Unix systems do. So
    // instead determine the resource path relative to the running
    // executable and force the user to use this directory layout.
    wchar_t path_data[MAX_PATH];
    if (GetModuleFileNameW(NULL, path_data, MAX_PATH) == 0)
        throw(ConfigurationError("Failed to retrieve the executable's path from the Win32API!"));
    std::string utf8_path = ucs2_to_utf8(path_data);

    m_paths.game_data_dir = utf8_to_path(utf8_path).parent_path().parent_path() / utf8_to_path("share") / utf8_to_path("tsc");
#else
    // Use the path configured at build time. If it is relative, construct
    // it from the install prefix, if it is absolute, take it as is.
    boost::filesystem::path datadir = utf8_to_path(INSTALL_DATADIR);

    if (datadir.is_relative())
        datadir = utf8_to_path(INSTALL_PREFIX) / datadir;

    // Append our program-specific subdirectory
    datadir /= utf8_to_path("tsc");

    m_paths.game_data_dir = datadir;
#endif

    ////////// The (writeable) user directories //////////
#ifdef __unix__
    m_paths.user_data_dir = xdg_get_directory("XDG_DATA_HOME", ".local/share") / utf8_to_path("tsc");
    m_paths.user_cache_dir = xdg_get_directory("XDG_CACHE_HOME", ".cache") / utf8_to_path("tsc");
    m_paths.user_config_dir = xdg_get_directory("XDG_CONFIG_HOME", ".config") / utf8_to_path("tsc");
#elif _WIN32
    wchar_t path_appdata[MAX_PATH + 1];

    // TODO: CSIDL_APPDATA has been deprecated with Windows Vista and upwards, and
    // has been replaced by FOLDERID_RoamingAppData (which is not available
    // on Windows XP). When dropping support for Windows XP, this should be changed.
    // See http://msdn.microsoft.com/en-us/library/windows/desktop/bb762181.aspx.
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path_appdata))) {
        cerr << "Error : Couldn't get Windows user data directory. Defaulting to ./data in the application directory." << endl;

        m_paths.user_data_dir = fs::current_path() / utf8_to_path("data");
        m_paths.user_cache_dir = fs::current_path() / utf8_to_path("data") / utf8_to_path("cache");
        m_paths.user_config_dir = fs::current_path() / utf8_to_path("data");
    }

    std::string str_path = ucs2_to_utf8(path_appdata);
    Convert_Path_Separators(str_path);
    fs::path app_path = utf8_to_path(str_path) / utf8_to_path("tsc");

    m_paths.user_data_dir = app_path;
    m_paths.user_cache_dir = app_path / utf8_to_path("cache");
    m_paths.user_config_dir = app_path;
#else
#error Dont know how to determine the user data directories on this platform!
#endif
}

#ifdef __unix__
fs::path cResource_Manager::xdg_get_directory(const std::string& envvarname, const fs::path defaultpath)
{
    char* envval = NULL;
    envval = getenv(envvarname.c_str());

    if (envval)
        return utf8_to_path(envval); // Envionment variables are UTF-8 on Linux
    else {
        envval = NULL;
        envval = getenv("HOME");

        if (envval)
            return utf8_to_path(envval) / defaultpath;
        else
            throw (ConfigurationError("$HOME environment variable is not set!"));
    }
}
#endif

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

cResource_Manager* pResource_Manager = NULL;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

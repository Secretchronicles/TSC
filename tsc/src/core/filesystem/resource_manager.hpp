/***************************************************************************
 * resource_manager.hpp
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

#ifndef TSC_RESOURCE_MANAGER_HPP
#define TSC_RESOURCE_MANAGER_HPP

#include "../../core/global_basic.hpp"
#include "../../core/global_game.hpp"

namespace TSC {

    struct PathInfo {
        boost::filesystem::path game_data_dir;
        boost::filesystem::path user_data_dir;
        boost::filesystem::path user_cache_dir;
        boost::filesystem::path user_config_dir;
    };

    /* *** *** *** *** *** cResource_Manager *** *** *** *** *** *** *** *** *** *** *** *** */

    /* This class manages TSC’s resource paths for graphics and other stuff.
     * It is divided in two parts: The files included with TSC itself,
     * and the files created by the user. The game files directory is usually
     * not writeable; it is determined
     *
     * a) on Windows by looking at the path of the running
     * executable and applying "../../share/tsc" to it, or
     *
     * b) on any other system by assuming the data resides in
     * TSC_DATA_DIR, which is set to the value of the
     * CMAKE_INSTALL_DATADIR option given at compile time plus "/tsc"
     * appended to it (see config.hpp.in). CMAKE_INSTALL_DATADIR
     * defaults to CMAKE_INSTALL_PREFIX plus "share" if not given.
     *
     * The user directory is determined at runtime platform-specifically.
     */
    class cResource_Manager {
    public:
        cResource_Manager(void);
        ~cResource_Manager(void);

        // Create the necessary folders in the user directory
        void Init_User_Directory(void);

        // Game data directory (files included with TSC)
        boost::filesystem::path Get_Game_Data_Directory();

        // The user’s data directory we can write to.
        //boost::filesystem::path Get_User_Data_Directory();

        // The path to the preferences file.
        boost::filesystem::path Get_Preferences_File();

        // Get the various uncached unwritable game directories.
        boost::filesystem::path Get_Game_Pixmaps_Directory();
        boost::filesystem::path Get_Game_Schema_Directory();
        boost::filesystem::path Get_Game_Level_Directory();
        boost::filesystem::path Get_Game_Translation_Directory();
        boost::filesystem::path Get_Game_Sounds_Directory();
        boost::filesystem::path Get_Game_Campaign_Directory();
        boost::filesystem::path Get_Game_Overworld_Directory();
        boost::filesystem::path Get_Game_Music_Directory();
        boost::filesystem::path Get_Game_Editor_Directory();
        boost::filesystem::path Get_Game_Scripting_Directory();
        boost::filesystem::path Get_Game_Icon_Directory();

        // CEGUI data paths
        boost::filesystem::path Get_Gui_Scheme_Directory();
        boost::filesystem::path Get_Gui_Imageset_Directory();
        boost::filesystem::path Get_Gui_Font_Directory();
        boost::filesystem::path Get_Gui_LookNFeel_Directory();
        boost::filesystem::path Get_Gui_Layout_Directory();
        boost::filesystem::path Get_Gui_XmlSchema_Directory();

        // Get files from the various uncached unwritable game directories
        boost::filesystem::path Get_Game_Pixmap(std::string pixmap);
        boost::filesystem::path Get_Game_Schema(std::string schema);
        boost::filesystem::path Get_Game_Level(std::string level);
        boost::filesystem::path Get_Game_Translation(std::string transname);
        boost::filesystem::path Get_Game_Sound(std::string sound);
        boost::filesystem::path Get_Game_Campaign(std::string campaign);
        boost::filesystem::path Get_Game_Overworld(std::string overworld);
        boost::filesystem::path Get_Game_Music(std::string music);
        boost::filesystem::path Get_Game_Editor(std::string editor);
        boost::filesystem::path Get_Game_Scripting(std::string script);
        boost::filesystem::path Get_Game_Icon(std::string icon);

        // Get the various directories in the user’s data directory
        boost::filesystem::path Get_User_Data_Directory();
        boost::filesystem::path Get_User_Level_Directory();
        boost::filesystem::path Get_User_Savegame_Directory();
        boost::filesystem::path Get_User_Screenshot_Directory();
        boost::filesystem::path Get_User_World_Directory();
        boost::filesystem::path Get_User_Campaign_Directory();
        boost::filesystem::path Get_User_Imgcache_Directory();
        boost::filesystem::path Get_User_Pixmaps_Directory();
        boost::filesystem::path Get_User_CEGUI_Logfile();
        boost::filesystem::path Get_User_GameConsole_Logfile();
        boost::filesystem::path Get_User_Scripting_Directory();

        // Get files from the various directories in the user’s data directory
        boost::filesystem::path Get_User_Level(std::string level);
        boost::filesystem::path Get_User_World(std::string world);
        boost::filesystem::path Get_User_Pixmap(std::string pixmap);

    private:
        // Main directory information
        struct PathInfo m_paths;

        // Sets up m_paths
        void init_directories();

#ifdef __unix__
        // Retrieve the path from the given XDG env variable or
        // return the default path relative to $HOME.
        boost::filesystem::path xdg_get_directory(const std::string& envvarname, const boost::filesystem::path defaultpath);
#endif
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

    // Resource Manager
    extern cResource_Manager* pResource_Manager;

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

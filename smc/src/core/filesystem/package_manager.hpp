/***************************************************************************
 * package_manager.hpp
 *
 * Copyright © 2009 - 2014 The SMC Contributors
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SMC_PACKAGE_MANAGER_HPP
#define SMC_PACKAGE_MANAGER_HPP

#include "../../core/global_basic.hpp"
#include "../../core/global_game.hpp"
#include "../../core/xml_attributes.hpp"

namespace SMC {

    struct PackageInfo {
        PackageInfo();

        // Internally set properties
        bool found_user;
        bool found_game;
        boost::filesystem::path game_data_dir;
        boost::filesystem::path user_data_dir;

        // Package properties
        std::string name;

        bool hidden;
        std::string desc;
        std::string menu_level;
        std::vector<std::string> dependencies;
    };

    /* *** *** *** *** *** cPackage_Loader *** *** *** *** *** *** *** *** *** *** *** *** */

    class cPackage_Loader : public xmlpp::SaxParser {
    public:
        cPackage_Loader();
        virtual ~cPackage_Loader();
        // Parse the file given by path.
        virtual void parse_file(boost::filesystem::path filename);

        PackageInfo Get_Package_Info();

    protected:
        // SAX parser callbacks
        virtual void on_start_document();
        virtual void on_end_document();
        virtual void on_start_element(const Glib::ustring& name, const xmlpp::SaxParser::AttributeList& properties);
        virtual void on_end_element(const Glib::ustring& name);

        // The package information we’re loading
        PackageInfo m_package;
        // The <property> results we found before the current tag
        XmlAttributes m_current_properties;
    };

    /* *** *** *** *** *** cPackage_Manager *** *** *** *** *** *** *** *** *** *** *** *** */

    class cPackage_Manager {
    public:
        cPackage_Manager(void);
        ~cPackage_Manager(void);

        // Get the list of known packages
        std::vector<PackageInfo> Get_Packages(void);
        PackageInfo Get_Package(const std::string& name);

        // Set the current package
        void Set_Current_Package(const std::string& name);
        std::string Get_Current_Package(void);

        // Create user paths
        void Init_User_Paths(void);

        // Return the path of the current package's data
        boost::filesystem::path Get_User_Data_Path(void);
        boost::filesystem::path Get_Game_Data_Path(void);

        // Levels, worlds, and campaigns come only from the currently selected package
        boost::filesystem::path Get_User_Level_Path(void);
        boost::filesystem::path Get_Game_Level_Path(void);
        boost::filesystem::path Get_Menu_Level_Path(void);

        boost::filesystem::path Get_User_Campaign_Path(void);
        boost::filesystem::path Get_Game_Campaign_Path(void);

        boost::filesystem::path Get_User_World_Path(void);
        boost::filesystem::path Get_Game_World_Path(void);

        // Get a script
        boost::filesystem::path Get_Scripting_Path(const std::string& package, const std::string& script);

        // Directories for screenshots adn savegames
        boost::filesystem::path Get_User_Savegame_Path(void);
        boost::filesystem::path Get_User_Screenshot_Path(void);

        // Find resources for reading
        boost::filesystem::path Get_Pixmap_Reading_Path(const std::string& pixmap, bool use_settings = false);
        boost::filesystem::path Get_Sound_Reading_Path(const std::string& sound);
        boost::filesystem::path Get_Music_Reading_Path(const std::string& music);

        // Find the relative path (for saving relative paths to files)
        boost::filesystem::path Get_Relative_Pixmap_Path(boost::filesystem::path path);
        boost::filesystem::path Get_Relative_Sound_Path(boost::filesystem::path path);
        boost::filesystem::path Get_Relative_Music_Path(boost::filesystem::path path);


    private:
        void Scan_Packages(boost::filesystem::path base, boost::filesystem::path path, bool user_packages);
        void Load_Package_Info( const boost::filesystem::path& dir, bool user_package );
        void Fix_Package_Paths( void );
        void Build_Search_Path(void);
        void Build_Search_Path_Helper(const std::string& package, std::vector<std::string>& processed);

        boost::filesystem::path Find_Reading_Path(boost::filesystem::path dir, boost::filesystem::path resource, std::vector<std::string> extra_ext);
        boost::filesystem::path Find_Relative_Path(boost::filesystem::path dir, boost::filesystem::path path);

        std::map <std::string, PackageInfo> m_packages;
        std::string m_current_package;
        std::vector<boost::filesystem::path> m_search_path;
        int m_package_start;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

    // Package Manager
    extern cPackage_Manager* pPackage_Manager;

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace SMC

#endif


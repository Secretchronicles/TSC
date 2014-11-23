/***************************************************************************
 * level_manager.h
 *
 * Copyright © 2007 - 2011 Florian Richter
 * Copyright © 2013 - 2014 The TSC Contributors
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TSC_LEVEL_MANAGER_HPP
#define TSC_LEVEL_MANAGER_HPP

#include "../core/global_basic.hpp"
#include "../core/obj_manager.hpp"
#include "../core/camera.hpp"
#include "../level/level.hpp"

namespace TSC {

// default files for levels
#define LEVEL_DEFAULT_MUSIC "land/land_5.ogg"
#define LEVEL_DEFAULT_BACKGROUND "game/background/green_junglehills.png"

    /* *** *** *** *** *** cLevel_Manager  *** *** *** *** *** *** *** *** *** *** *** *** */

    class cLevel_Manager : public cObject_Manager<cLevel> {
    public:
        cLevel_Manager(void);
        virtual ~cLevel_Manager(void);

        // Load level descriptions
        void Init(void);
        // Unload
        void Unload(void);

        /* Create a level and returns it if doesn't already exists
         * The new level is not set active.
        */
        cLevel* New(std::string levelname);
        /* Load level and returns it if successful
         * If the level is already loaded it is returned but not reloaded.
         * The loaded level is not set active.
        */
        cLevel* Load(std::string levelname, bool loading_sublevel = false);
        // Set active level
        bool Set_Active(cLevel* level);
        // Get level pointer
        cLevel* Get(const std::string& levelname);
        /* Return the level path if level is valid else empty().
         * check_only_user_dir : only check user directory for the level and
         * skip levels included in the game.
         */
        boost::filesystem::path Get_Path(const std::string& levelname, bool check_only_user_dir = false);
        // update
        void Update(void);
        // draw
        void Draw(void);

        /* Exits the level and
        * - walks to the next Overworld waypoint if a world level
        * - enters the menu if a custom level
        * win_music : play the level finished music
        */
        void Finish_Level(bool win_music = 0);
        /* Enters the given level on the entry
        * move_camera: set how to move the camera to the new position
        * path_identifier: set the identifier of the path to move the camera along
        */
        void Goto_Sub_Level(std::string str_level, const std::string& str_entry, Camera_movement move_camera = CAMERA_MOVE_FLY, const std::string& path_identifier = "");


        // level camera
        cCamera* m_camera;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

// The Level Manager
    extern cLevel_Manager* pLevel_Manager;

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

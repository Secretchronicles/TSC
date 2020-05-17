/***************************************************************************
 * overworld.h
 *
 * Copyright © 2003 - 2011 Florian Richter
 * Copyright © 2012-2020 The TSC Contributors
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TSC_OVERWORLD_HPP
#define TSC_OVERWORLD_HPP

#include "../overworld/world_manager.hpp"
#include "../overworld/world_waypoint.hpp"
#include "../overworld/world_layer.hpp"
#include "../overworld/world_player.hpp"
#include "../overworld/world_sprite_manager.hpp"
#include "../audio/random_sound.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** *** cOverworld_description *** *** *** *** *** *** *** *** *** */

    class cOverworld_description {
    public:
        cOverworld_description(void);
        virtual ~cOverworld_description(void);

        // Save
        void Save(void);

        // Save to the given file. Raises xmlpp::exception on error.
        void Save_To_File(boost::filesystem::path path);

        // Full path to the world directory
        boost::filesystem::path Get_Path();
        // Set path to the world directory, optionally overwriting the world’s name.
        void Set_Path(boost::filesystem::path directory, bool set_name = false);

        // Full path to the world directory
        // FIXME: This should really be in cOverworld itself as it has
        // exactly NOTHING to do with the description stuff handled by
        // this object.
        boost::filesystem::path m_path;
        // world name
        std::string m_name;
        // is visible on selection
        bool m_visible;
        /* 0 if only in game directory
         * 1 if only in user directory
         * 2 if in both
        */
        int m_user;

        // world comment
        std::string m_comment;
    };

    /* *** *** *** *** *** *** *** *** cOverworld *** *** *** *** *** *** *** *** *** */

// forward declare
    class cAnimation_Manager;

    typedef vector<cWaypoint*> WaypointList;

    class cOverworld {
    public:
        cOverworld(void);

        /// Load an overworld from a world directory.
        /// The returned instance must be freed by you.
        static cOverworld* Load_From_Directory(boost::filesystem::path directory, int user_dir = 0);

        virtual ~cOverworld(void);

        // New
        bool New(std::string name);
        // Unload
        void Unload(void);
        // Save
        void Save(void);

        // Save to the given directory. Raises xmlpp::exception on error.
        void Save_To_Directory(boost::filesystem::path path);

        // Enter
        void Enter(const GameMode old_mode = MODE_NOTHING);
        // Leave
        void Leave(const GameMode next_mode = MODE_NOTHING);

        // Replace the default description (which is always the
        // one of world_1) with something more useful.
        // FIXME: This method needs to be removed when m_path is
        // transferred from cOverworld_description to cOverworld.
        void Replace_Description(cOverworld_description* p_desc);

        // Draw
        void Draw(void);
        // Draw Layer 1 ( Backgrounds, World objects and Animations )
        void Draw_Layer_1(void);

        // Update
        void Update(void);
        // Updates the Camera
        void Update_Camera(void);

        void Process_Input();

        /* handle key down event
         * returns true if processed
        */
        bool Key_Down(const sf::Event& evt);
        /* handle key up event
         * returns true if processed
        */
        bool Key_Up(const sf::Event& evt);
        /* handle mouse button down event
         * returns true if processed
        */
        bool Mouse_Down(sf::Mouse::Button button);
        /* handle mouse button up event
         * returns true if processed
        */
        bool Mouse_Up(sf::Mouse::Button button);
        /* handle joystick button down event
         * returns true if processed
        */
        bool Joy_Button_Down(unsigned int button);
        /* handle joystick button up event
         * returns true if processed
        */
        bool Joy_Button_Up(unsigned int button);

        /* Returns a pointer to the Waypoint
         * if not found returns NULL
        */
        cWaypoint* Get_Waypoint(const std::string& name);
        cWaypoint* Get_Waypoint(unsigned int num);
        /* Returns the Waypoint array number if the level name matches
         * if not found returns -1
        */
        int Get_Level_Waypoint_Num(std::string level_name);
        /* Returns the Waypoint array number if the destination name matches
         * if not found returns -1
        */
        int Get_Waypoint_Num(const std::string& world_name);

        /* Check if the rect collides with a Waypoint
         * if no collision found returns -1
        */
        int Get_Waypoint_Collision(const GL_rect& rect_2);
        // returns the last accessible Waypoint
        int Get_Last_Valid_Waypoint(void);
        // update the Waypoint text
        void Update_Waypoint_text(void);

        // Use the line associated with the level exit name passed
        // and enable the next level on that line. If an empty
        // string is passed, legacy behaviour of forward direction
        // is used.
        bool Goto_Next_Level(std::string taken_exit = "");
        // Resets the Waypoint access to the default
        void Reset_Waypoints(void);

        // Return true if a world is loaded
        bool Is_Loaded(void) const;

        // map objects
        cWorld_Sprite_Manager* m_sprite_manager;
        // animation manager
        cAnimation_Manager* m_animation_manager;
        // waypoints
        WaypointList m_waypoints;
        // description
        cOverworld_description* m_description;
        // current Layer for collision checking
        cLayer* m_layer;

        /* *** *** *** Settings *** *** *** *** */

        // world engine version
        int m_engine_version;
        // last save time
        time_t m_last_saved;

        // background color
        Color m_background_color;
        // music filename
        // FIXME: Should be boost::filesystem::path
        std::string m_musicfile;

        float m_cheat_counter;

        // settings
        unsigned int m_player_start_waypoint;
        Moving_state m_player_moving_state;

        /* *** *** ***  *** *** *** *** */

        // goto next level on overworld enter
        bool m_next_level;
        // which level exit was taken for m_next_level
        std::string m_exit_for_next_level;

    private:
        // Common stuff for constructors
        void Init();

        // Save only the main overworld file, not layers and description files.
        void Save_To_File(boost::filesystem::path path);
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

/// Active overworld, if any. NULL otherwise.
    extern cOverworld* pActive_Overworld;

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

/***************************************************************************
 * world_player.h
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

#ifndef TSC_WORLD_PLAYER_HPP
#define TSC_WORLD_PLAYER_HPP

#include "../core/global_basic.hpp"
#include "../gui/hud.hpp"
#include "../overworld/world_layer.hpp"
#include "../level/level_player.hpp"
#include "../overworld/world_waypoint.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** cOverworld_Player *** *** *** *** *** *** *** *** *** *** */

    class cOverworld_Player : public cMovingSprite {
    public:
        cOverworld_Player(cSprite_Manager* sprite_manager, cOverworld* overworld);
        virtual ~cOverworld_Player(void);

        // Load images
        void Load_Images(void);
        // Unload images
        void Unload_Images(void);

        // Set the parent overworld
        void Set_Overworld(cOverworld* overworld);
        /* Set the direction
         * if new_start_direction is set also set the start/editor direction
        */
        virtual void Set_Direction(const ObjectDirection dir, bool new_start_direction = 0);

        // Set the Type
        void Set_Type(Alex_type new_type);

        // Update
        virtual void Update(void);
        // Draw
        virtual void Draw(cSurface_Request* request = NULL);
        // Draw debug text
        void Draw_Debug_Text(void);

        // Reset
        void Reset(void);

        // General input interact event
        void Action_Interact(input_identifier key_type);
        // Stop Interacting event
        void Action_Stop_Interact(input_identifier key_type);

        // Set the velocity for the current direction
        void Update_Vel(void);

        // Activates the current Waypoint
        void Activate_Waypoint(void);

        /* Start walking into the given direction
         * returns 0 if the next level is not accessible or not available
        */
        bool Start_Walk(ObjectDirection new_direction);
        /* Updates walking
         * checks for new line contacts and updates the Animation
        */
        void Update_Walk(void);
        // Start fixed walking into the given Waypoint
        void Start_Waypoint_Walk(int new_waypoint);
        /* Fixed walking into the Waypoint
         * moves alex smoothly into the found Waypoint
        */
        void Update_Waypoint_Walk(void);

        // Set Alex to the given Waypoint position
        bool Set_Waypoint(int waypoint, bool new_startpos = 0);
        // Get current Waypoint
        cWaypoint* Get_Waypoint(void);

        // Returns the current Waypoint front line
        cLayer_Line_Point_Start* Get_Front_Line(ObjectDirection dir) const;
        // Find the line that has a point with the given UID.
        cLayer_Line_Point_Start* Get_Line_By_UID(int uid);

        /* Checks how much Alex can walk into all directions
         * and sets the size into the path directions
        */
        void Update_Path_Diff(unsigned int check_size = 25);

        // Alex changes direction if a near path is found or turns around
        void Change_Direction(void);

        /* Automatically corrects Alex's position to the walkable Layer Line
         * with the given size and the minimal distance to not-walkable area
        */
        void Auto_Pos_Correction(float size = 1.7f, float min_distance = 5.0f);

        // parent overworld
        cOverworld* m_overworld;
        // state
        Alex_type m_alex_state;
        // current waypoint
        int m_current_waypoint;
        // line waypoint
        unsigned int m_line_waypoint;
        // current line ( if walking )
        int m_current_line;

        // if touched a Waypoint use fixed walking
        bool m_fixed_walking;

        // valid path detection lines
        cLine_collision m_line_hor;
        cLine_collision m_line_ver;

    private:
        // Debug last set data
        int m_debug_current_line_last;
        int m_debug_lines_last;
        int m_debug_current_waypoint_last;

        // Debug text sprites
        cHudSprite* m_debug_current_waypoint;
        cHudSprite* m_debug_lines;
        cHudSprite* m_debug_current_line;

        bool check_access_to_waypoint(cWaypoint* p_wp);
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

// Overworld Player
    extern cOverworld_Player* pOverworld_Player;

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

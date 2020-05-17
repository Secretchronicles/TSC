/***************************************************************************
 * save_overworld.hpp - Handler for saving a cOverworld instance to a savegame.
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

#ifndef TSC_SAVEGAME_SAVE_OVERWORLD_HPP
#define TSC_SAVEGAME_SAVE_OVERWORLD_HPP
#include "../../core/global_basic.hpp"
#include "../../overworld/world_waypoint.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** cSave_Overworld_Waypoint *** *** *** *** *** *** *** *** *** *** */
    // Overworld Waypoint save data
    class cSave_Overworld_Waypoint {
    public:
        cSave_Overworld_Waypoint(void);
        ~cSave_Overworld_Waypoint(void);

        std::string m_destination;
        bool m_access;
        std::vector<waypoint_exit> m_exits;
    };

    typedef vector<cSave_Overworld_Waypoint*> Save_Overworld_WaypointList;


    /* *** *** *** *** *** *** *** cSave_Overworld *** *** *** *** *** *** *** *** *** *** */
    // Overworld save data
    class cSave_Overworld {
    public:
        cSave_Overworld(void);
        ~cSave_Overworld(void);

        std::string m_name;

        // waypoint data
        Save_Overworld_WaypointList m_waypoints;
    };

    typedef vector<cSave_Overworld*> Save_OverworldList;

}
#endif

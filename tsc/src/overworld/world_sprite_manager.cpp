/***************************************************************************
 * world_sprite_manager.cpp  -  World Sprite Manager
 *
 * Copyright © 2008 - 2011 Florian Richter
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

#include "../overworld/world_sprite_manager.hpp"
#include "../core/game_core.hpp"

namespace TSC {

/* *** *** *** *** *** *** cWorld_Sprite_Manager *** *** *** *** *** *** *** *** *** *** *** */

cWorld_Sprite_Manager::cWorld_Sprite_Manager(cOverworld* overworld)
    : cSprite_Manager(500)
{
    m_overworld = overworld;
}

cWorld_Sprite_Manager::~cWorld_Sprite_Manager(void)
{
    Delete_All();
}

void cWorld_Sprite_Manager::Add(cSprite* sprite)
{
    // empty sprite
    if (!sprite) {
        return;
    }

    // add
    cSprite_Manager::Add(sprite);

    // Add to Waypoints array
    if (sprite->m_type == TYPE_OW_WAYPOINT) {
        m_overworld->m_waypoints.push_back(static_cast<cWaypoint*>(sprite));
    }
    // Add layer line point start to the world layer
    else if (sprite->m_type == TYPE_OW_LINE_START) {
        m_overworld->m_layer->Add(static_cast<cLayer_Line_Point_Start*>(sprite));
    }
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

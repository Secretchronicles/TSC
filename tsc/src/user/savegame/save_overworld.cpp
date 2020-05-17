/***************************************************************************
 * save_overworld.cpp - Handler for saving a cOverworld instance to a savegame.
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

#include "save_overworld.hpp"

using namespace TSC;

/* *** *** *** *** *** *** *** cSave_Overworld_Waypoint *** *** *** *** *** *** *** *** *** *** */

cSave_Overworld_Waypoint::cSave_Overworld_Waypoint(void)
{
    m_access = 0;
}

cSave_Overworld_Waypoint::~cSave_Overworld_Waypoint(void)
{
    //
}

/* *** *** *** *** *** *** *** cSave_Overworld *** *** *** *** *** *** *** *** *** *** */

cSave_Overworld::cSave_Overworld(void)
{
    //
}

cSave_Overworld::~cSave_Overworld(void)
{
    for (unsigned int i = 0; i < m_waypoints.size(); i++) {
        delete m_waypoints[i];
    }

    m_waypoints.clear();
}

/***************************************************************************
 * save.cpp  -  Main savegame save object
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

#include "save.hpp"
#include "../../core/property_helper.hpp"
#include "../../core/game_core.hpp"
#include "../../level/level_manager.hpp"
#include "savegame_loader.hpp"

using namespace TSC;

namespace fs = boost::filesystem;

/* *** *** *** *** *** *** *** cSave *** *** *** *** *** *** *** *** *** *** */

cSave::cSave(void)
{
    Init();
}

cSave::~cSave(void)
{
    for (Save_LevelList::iterator itr = m_levels.begin(); itr != m_levels.end(); ++itr) {
        delete *itr;
    }

    m_levels.clear();

    for (Save_OverworldList::iterator itr = m_overworlds.begin(); itr != m_overworlds.end(); ++itr) {
        delete *itr;
    }

    m_overworlds.clear();
}

void cSave::Init(void)
{
    // save
    m_save_time = 0;
    m_version = 0;

    // player
    m_lives = 0;
    m_points = 0;
    m_goldpieces = 0;
    m_player_type = 0;
    m_player_state = 0;
    m_itembox_item = 0;
    m_invincible = 0;
    m_invincible_star = 0;
    m_ghost_time = 0;
    m_ghost_time_mod = 0;

    //Player state (ie power ups)
    m_player_type = 0;
    m_player_type_temp_power = 0;

    // level
    m_level_time = 0;
    // set earliest available version
    m_level_engine_version = 39;

    // overworld
    m_overworld_current_waypoint = 0;
}

cSave* cSave::Load_From_File(fs::path filepath)
{
    debug_print("Loading savegame file '%s'\n", path_to_utf8(filepath).c_str());

    cSavegameLoader loader;
    loader.parse_file(filepath);
    return loader.Get_Save();
}

std::string cSave::Get_Active_Level(void)
{
    if (m_levels.empty()) {
        return "";
    }

    for (Save_LevelList::iterator itr = m_levels.begin(); itr != m_levels.end(); ++itr) {
        cSave_Level* save_level = (*itr);

        if (pLevel_Manager->Get_Path(save_level->m_name).empty()) {
            continue;
        }

        // if active level
        if (!Is_Float_Equal(save_level->m_level_pos_x, 0.0f) && !Is_Float_Equal(save_level->m_level_pos_y, 0.0f)) {
            return save_level->m_name;
        }
    }

    return "";
}

void cSave::Write_To_File(fs::path filepath)
{
    xmlpp::Document doc;
    xmlpp::Element* p_root = doc.create_root_node("savegame");
    xmlpp::Element* p_node = NULL;

    // <information>
#ifdef USE_LIBXMLPP3
    p_node = p_root->add_child_element("information");
#else
    p_node = p_root->add_child("information");
#endif
    Add_Property(p_node, "version", m_version);
    Add_Property(p_node, "level_engine_version", m_level_engine_version);
    Add_Property(p_node, "save_time", static_cast<uint64_t>(m_save_time));
    Add_Property(p_node, "description", m_description);
    // </information>

    // <player>
#ifdef USE_LIBXMLPP3
    p_node = p_root->add_child_element("player");
#else
    p_node = p_root->add_child("player");
#endif
    Add_Property(p_node, "lives", m_lives);
    Add_Property(p_node, "points", m_points);
    Add_Property(p_node, "goldpieces", m_goldpieces);
    Add_Property(p_node, "type", m_player_type);
    Add_Property(p_node, "type_temp_power", m_player_type_temp_power);
    Add_Property(p_node, "invincible_star", m_invincible_star);
    Add_Property(p_node, "invincible", m_invincible);
    Add_Property(p_node, "ghost_time", m_ghost_time);
    Add_Property(p_node, "ghost_time_mod", m_ghost_time_mod);

    Add_Property(p_node, "state", m_player_state);
    Add_Property(p_node, "itembox_item", m_itembox_item);
    // if a level is available
    if (!m_levels.empty())
        Add_Property(p_node, "level_time", m_level_time);
    Add_Property(p_node, "overworld_active", m_overworld_active);
    Add_Property(p_node, "overworld_current_waypoint", m_overworld_current_waypoint);
    // </player>

    // player return stack
    std::vector<cSave_Player_Return_Entry>::const_iterator return_iter;
    for (return_iter = m_return_entries.begin(); return_iter != m_return_entries.end(); return_iter++) {
        cSave_Player_Return_Entry item = *return_iter;

#ifdef USE_LIBXMLPP3
        p_node = p_root->add_child_element("return");
#else
        p_node = p_root->add_child("return");
#endif

        if (!item.m_level.empty())
            Add_Property(p_node, "level", item.m_level);
        if (!item.m_entry.empty())
            Add_Property(p_node, "entry", item.m_entry);
    }

    // levels
    Save_LevelList::const_iterator iter;
    for (iter=m_levels.begin(); iter != m_levels.end(); iter++) {
        cSave_Level* p_level = *iter;
        p_level->Save_To_Node(p_root);
    }

    // Overworlds
    Save_OverworldList::const_iterator oiter;
    for (oiter=m_overworlds.begin(); oiter != m_overworlds.end(); oiter++) {
        cSave_Overworld* p_overworld = *oiter;

        // <overworld>
#ifdef USE_LIBXMLPP3
        p_node = p_root->add_child_element("overworld");
        Add_Property(p_node, "name", p_overworld->m_name);
#else
        p_node = p_root->add_child("overworld");
        Add_Property(p_node, "name", p_overworld->m_name);
#endif

        Save_Overworld_WaypointList::const_iterator wpiter;
        for (wpiter=p_overworld->m_waypoints.begin(); wpiter != p_overworld->m_waypoints.end(); wpiter++) {
            cSave_Overworld_Waypoint* p_wp = *wpiter;

            // skip empty waypoints
            if (p_wp->m_destination.empty())
                continue;

            // <waypoint>
#ifdef USE_LIBXMLPP3
            xmlpp::Element* p_waypoint_node = p_node->add_child_element("waypoint");
#else
            xmlpp::Element* p_waypoint_node = p_node->add_child("waypoint");
#endif
            Add_Property(p_waypoint_node, "destination", p_wp->m_destination);
            Add_Property(p_waypoint_node, "access", p_wp->m_access);

            for(size_t i=0; i < p_wp->m_exits.size(); i++) {
                std::string str_pos     = int_to_string(i);
                const waypoint_exit& ex = p_wp->m_exits[i];

                Add_Property(p_waypoint_node, "waypoint_exit_" + str_pos + "_locked", ex.locked);
            }
            // </waypoint>
        }

        // </overworld>
    }

    // Write to file (raises xmlpp::exception on error)
    doc.write_to_file_formatted(Glib::filename_from_utf8(path_to_utf8(filepath)));
    debug_print("Wrote savegame file '%s'.\n", path_to_utf8(filepath).c_str());
}

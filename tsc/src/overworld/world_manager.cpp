/***************************************************************************
 * worlds.cpp  -  class for handling worlds data
 *
 * Copyright © 2004 - 2011 Florian Richter
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

#include "../overworld/world_manager.hpp"
#include "../core/game_core.hpp"
#include "../overworld/overworld.hpp"
#include "../core/filesystem/filesystem.hpp"
#include "../core/filesystem/resource_manager.hpp"
#include "../overworld/world_editor.hpp"
#include "../input/mouse.hpp"
#include "../video/animation.hpp"

namespace fs = boost::filesystem;

namespace TSC {

/* *** *** *** *** *** *** *** *** cOverworld_Manager *** *** *** *** *** *** *** *** *** */

cOverworld_Manager::cOverworld_Manager(cSprite_Manager* sprite_manager)
    : cObject_Manager<cOverworld>()
{
    m_debug_mode = 0;
    m_draw_layer = 0;
    m_camera_mode = 0;

    m_camera = new cCamera(sprite_manager);

    Init();
}

cOverworld_Manager::~cOverworld_Manager(void)
{
    Delete_All();

    delete m_camera;
}

bool cOverworld_Manager::New(std::string name)
{
    string_trim(name, ' ');

    // no name given
    if (name.empty()) {
        return 0;
    }

    // name already exists
    if (Get_from_Path(name)) {
        return 0;
    }

    cOverworld* overworld = new cOverworld();
    overworld->New(name);
    objects.push_back(overworld);

    return 1;
}

void cOverworld_Manager::Init(void)
{
    // if already loaded
    if (!objects.empty()) {
        Delete_All();
    }

    // Load Worlds
    Load_Dir(pResource_Manager->Get_User_World_Directory(), true);
    Load_Dir(pResource_Manager->Get_Game_Overworld_Directory());
}

void cOverworld_Manager::Load_Dir(const fs::path& dir, bool user_dir /* = false */)
{
    // set world directory
    vector<fs::path> subdirs = Get_Directory_Files(dir, "", true, false);
    std::sort(subdirs.begin(), subdirs.end());

    for (vector<fs::path>::iterator curdir = subdirs.begin(); curdir != subdirs.end(); ++curdir) {
        try {
            fs::path current_dir = *curdir;

            // only directories with an existing description
            if (File_Exists(current_dir / "description.xml")) {
                cOverworld* overworld = Get_from_Path(current_dir);

                // already available
                if (overworld) {
                    overworld->m_description->m_user = 2; // 2 = available in system *and* in user dir
                    continue;
                }

                overworld = cOverworld::Load_From_Directory(current_dir, user_dir);
                objects.push_back(overworld);
            }
        }
        catch (const std::exception& ex) {
            printf("%s %s\n", path_to_utf8(*curdir).c_str(), ex.what());
        }
    }
}

bool cOverworld_Manager::Set_Active(const std::string& str)
{
    return Set_Active(Get(str));
}

bool cOverworld_Manager::Set_Active(cOverworld* world)
{
    if (!world) {
        return 0;
    }

    pActive_Overworld = world;

    pWorld_Editor->Set_Sprite_Manager(world->m_sprite_manager);
    pWorld_Editor->Set_Overworld(world);
    m_camera->Set_Sprite_Manager(world->m_sprite_manager);
    pOverworld_Player->Set_Sprite_Manager(world->m_sprite_manager);
    pOverworld_Player->Set_Overworld(world);
    pOverworld_Player->Reset();

    // pre-update animations
    for (cSprite_List::iterator itr = world->m_sprite_manager->objects.begin(); itr != world->m_sprite_manager->objects.end(); ++itr) {
        cSprite* obj = (*itr);

        if (obj->m_type == TYPE_PARTICLE_EMITTER) {
            cParticle_Emitter* emitter = static_cast<cParticle_Emitter*>(obj);
            emitter->Pre_Update();
        }
    }

    return 1;
}

void cOverworld_Manager::Reset(void)
{
    // FIXME: Do not assume any specific overworld!
    // default Overworld
    Set_Active("World 1");

    // Set Player to first Waypoint
    pOverworld_Player->Set_Waypoint(pActive_Overworld->m_player_start_waypoint);

    // Reset all Waypoints
    for (vector<cOverworld*>::iterator itr = objects.begin(); itr != objects.end(); ++itr) {
        (*itr)->Reset_Waypoints();
    }
}

cOverworld* cOverworld_Manager::Get(const std::string& str)
{
    cOverworld* world = Get_from_Name(str);

    if (world) {
        return world;
    }

    return Get_from_Path(utf8_to_path(str));
}

cOverworld* cOverworld_Manager::Get_from_Path(const fs::path& path)
{
    for (vector<cOverworld*>::const_iterator itr = objects.begin(); itr != objects.end(); ++itr) {
        cOverworld* obj = (*itr);

        if (obj->m_description->m_path == path || obj->m_description->m_path.filename() == path) {
            return obj;
        }
    }

    return NULL;
}

cOverworld* cOverworld_Manager::Get_from_Name(const std::string& name)
{
    for (vector<cOverworld*>::iterator itr = objects.begin(); itr != objects.end(); ++itr) {
        cOverworld* obj = (*itr);

        if (obj->m_description->m_name.compare(name) == 0) {
            return obj;
        }
    }

    return NULL;
}

int cOverworld_Manager::Get_Array_Num(const std::string& path) const
{
    for (unsigned int i = 0; i < objects.size(); i++) {
        if (objects[i]->m_description->m_path.compare(path) == 0) {
            return i;
        }
    }

    return -1;
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

cOverworld_Manager* pOverworld_Manager = NULL;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

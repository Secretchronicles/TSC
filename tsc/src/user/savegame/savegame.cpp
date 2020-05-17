/***************************************************************************
 * savegame.cpp  -  Savegame handler
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

#include "savegame.hpp"
#include "savegame_loader.hpp"
#include "../preferences.hpp"
#include "../../core/game_core.hpp"
#include "../../core/obj_manager.hpp"
#include "../../core/errors.hpp"
#include "../../level/level.hpp"
#include "../../overworld/world_manager.hpp"
#include "../../level/level_player.hpp"
#include "../../overworld/overworld.hpp"
#include "../../core/i18n.hpp"
#include "../../core/filesystem/filesystem.hpp"
#include "../../core/filesystem/resource_manager.hpp"
#include "../../scripting/events/level_save_load_event.hpp"
#include "../../core/global_basic.hpp"
#include "../../audio/audio.hpp"
#include "../../enemies/army.hpp"
#include "../../gui/hud.hpp"

using namespace std;

namespace fs = boost::filesystem;

namespace TSC {

/* *** *** *** *** *** cSave_Player_Return_Entry *** *** *** *** *** *** *** *** */
cSave_Player_Return_Entry::cSave_Player_Return_Entry(const std::string& level, const std::string& entry) :
    m_level(level), m_entry(entry)
{
}

/* *** *** *** *** *** *** *** cSavegame *** *** *** *** *** *** *** *** *** *** */

cSavegame::cSavegame(void)
{
    m_savegame_dir = pResource_Manager->Get_User_Savegame_Directory();
}

cSavegame::~cSavegame(void)
{
    //
}

int cSavegame::Load_Game(unsigned int save_slot)
{
    cSave* savegame = Load(save_slot);

    if (!savegame) {
        return 0;
    }

    // check if unsupported save
    if (savegame->m_version <= SAVEGAME_VERSION_UNSUPPORTED) {
        cerr << "Warning : Savegame " << save_slot << " : Versions " << SAVEGAME_VERSION_UNSUPPORTED << " and below are unsupported" << endl;
    }

    // reset custom level mode type
    if (Game_Mode_Type == MODE_TYPE_LEVEL_CUSTOM) {
        Game_Mode_Type = MODE_TYPE_DEFAULT;
    }

    // #### Overworld ####

    // set overworld progress
    if (!savegame->m_overworlds.empty()) {
        for (Save_OverworldList::iterator itr = savegame->m_overworlds.begin(); itr != savegame->m_overworlds.end(); ++itr) {
            // get savegame overworld pointer
            cSave_Overworld* save_overworld = (*itr);

            // get overworld
            cOverworld* overworld = pOverworld_Manager->Get_from_Name(save_overworld->m_name);

            if (!overworld) {
                cerr << "Warning : Savegame " << save_slot << " : Overworld " << save_overworld->m_name << " not found" << endl;
                continue;
            }

            for (Save_Overworld_WaypointList::iterator wp_itr = save_overworld->m_waypoints.begin(); wp_itr != save_overworld->m_waypoints.end(); ++wp_itr) {
                // get savegame waypoint pointer
                cSave_Overworld_Waypoint* save_waypoint = (*wp_itr);

                // get overworld waypoint
                cWaypoint* waypoint = overworld->Get_Waypoint(overworld->Get_Waypoint_Num(save_waypoint->m_destination));

                // not found
                if (!waypoint) {
                    cerr << "Warning : Savegame " << save_slot << " : Overworld " << save_overworld->m_name << " Waypoint " << save_waypoint->m_destination << " not found" << endl;
                    continue;
                }

                // set access
                waypoint->Set_Access(save_waypoint->m_access);

                // set (un)lock state of alternate pathes
                for(size_t i=0; i < save_waypoint->m_exits.size(); i++) {
                    waypoint->m_exits[i].locked = save_waypoint->m_exits[i].locked;
                }
            }
        }
    }

    // if an overworld is active
    if (!savegame->m_overworld_active.empty()) {
        // Set active overworld
        if (!pOverworld_Manager->Set_Active(savegame->m_overworld_active)) {
            cerr << "Warning : Savegame " << save_slot << " : Couldn't set Overworld active " << savegame->m_overworld_active << endl;
        }

        // Current waypoint
        if (!pOverworld_Player->Set_Waypoint(savegame->m_overworld_current_waypoint)) {
            cerr << "Warning : Savegame " << save_slot << " : Overworld Current Waypoint " << savegame->m_overworld_current_waypoint << " is invalid" << endl;
        }
    }
    // overworld is not active
    else {
        // Set custom level mode
        Game_Mode_Type = MODE_TYPE_LEVEL_CUSTOM;
    }

    // #### Level ####

    // default is world savegame
    unsigned int save_type = 2;

    // load levels
    if (!savegame->m_levels.empty()) {
        for (Save_LevelList::iterator itr = savegame->m_levels.begin(); itr != savegame->m_levels.end(); ++itr) {
            cSave_Level* save_level = (*itr);
            cLevel* level = pLevel_Manager->Load(save_level->m_name);

            if (!level) {
                cerr << "Warning : Couldn't load Savegame Level " << save_level->m_name << endl;
                continue;
            }

            // active level
            if (!Is_Float_Equal(save_level->m_level_pos_x, 0.0f) && !Is_Float_Equal(save_level->m_level_pos_y, 0.0f)) {
                pLevel_Manager->Set_Active(level);
                level->Init();

                // if below version 9 : move y coordinate bottom to 0 and remove screen height adjustment
                if (savegame->m_version < 9) {
                    save_level->m_level_pos_y -= 1200.0f;
                }

                // time
                gp_hud->Set_Elapsed_Time(savegame->m_level_time);

                // position
                pLevel_Player->Set_Pos(save_level->m_level_pos_x, save_level->m_level_pos_y);

                // level savegame
                save_type = 1;
            }

            // spawned objects
            for (cSprite_List::iterator itr = save_level->m_spawned_objects.begin(); itr != save_level->m_spawned_objects.end(); ++itr) {
                cSprite* sprite = (*itr);

                sprite->Set_Sprite_Manager(level->m_sprite_manager);
                sprite->Set_Spawned(1);
                level->m_sprite_manager->Add(sprite);
            }

            save_level->m_spawned_objects.clear();

            // objects data
            for (Save_Level_ObjectList::iterator itr = save_level->m_level_objects.begin(); itr != save_level->m_level_objects.end(); ++itr) {
                cSave_Level_Object* save_object = (*itr);

                // get position
                int posx = string_to_int(save_object->Get_Value("posx"));
                int posy = string_to_int(save_object->Get_Value("posy"));

                cSprite* level_object = level->m_sprite_manager->Get_from_Position(posx, posy, save_object->m_type);

                // if not anymore available
                if (!level_object) {
                    cerr << "Warning : Savegame object type " << save_object->m_type << " on x " << posx << ", y " << posy << " not available" << endl;
                    continue;
                }

                level_object->Load_From_Savegame(save_object);
                //If the currently loaded object is a shell (loose or with army in it) and if it was linked, call the Get_Item
                //method to properly set it up with the player
                if (save_object ->m_type == TYPE_SHELL  || save_object -> m_type == TYPE_ARMY) {
                    cArmy* army = static_cast<cArmy*>(level_object);
                    if (army ->m_state == STA_OBJ_LINKED) {
                        pLevel_Player ->Get_Item(army ->m_type, false, army);
                    }
                }
            }

            // Feed the data stored in the file back to the
            // Saveload event. pSavegame holds the event
            // table required for this.
            Scripting::cLevel_SaveLoad_Event evt(false);
            evt.Set_Storage(save_level->m_script_datas);
            evt.Fire(level->m_mruby, pSavegame);
        }
    }

    // #### Player ####
    //Note: Some of these steps (ie invincibility) have to be done after level -> Init() is called above.
    //Otherwise, it will wipe them out.

    // below version 8 the state was the type
    if (savegame->m_version < 8) {
        pLevel_Player->Set_Type(static_cast<Alex_type>(savegame->m_player_state), 0, 0);
    }
    else {
        /*Set the player's power up type.
        For ghost alex, we first set the ghost power up as the type and then set the other power up as the type,
        marking a flag for the temporary ghost power up.  Logic for both fields is not included above in the version
        8 code because version 8 save files did not have the temporary power up field saved*/
        if (savegame -> m_player_type == ALEX_GHOST) {
            pLevel_Player->Set_Type(static_cast<Alex_type>(savegame->m_player_type), false, false);
            pLevel_Player->Set_Type(static_cast<Alex_type>(savegame->m_player_type_temp_power), false, false, true);
        }
        else {
            pLevel_Player->Set_Type(static_cast<Alex_type>(savegame->m_player_type), false, false);
        }
        pLevel_Player -> Set_Moving_State(static_cast<Moving_state>(savegame->m_player_state));
    }

    //Set invincibility time and star time for player
    pLevel_Player -> m_invincible = savegame->m_invincible;
    pLevel_Player -> m_invincible_star = savegame->m_invincible_star;

    //If the player had less than a second of invincibility or was not invincible (value of 0), give them a
    //second of invincibility.  This is only applicable if a level save state was loaded (not a world map save state).
    if (save_type == 1 && pLevel_Player->m_invincible < speedfactor_fps) {
        pLevel_Player->m_invincible = speedfactor_fps;
    }

    pLevel_Player -> m_ghost_time = savegame->m_ghost_time;
    pLevel_Player -> m_ghost_time_mod = savegame->m_ghost_time_mod;

    //Play the appropriate music
    if (!Is_Float_Equal(pLevel_Player -> m_invincible_star, 0.0f)) { //Player Invincible - Invincibility music
        pAudio->Play_Music("game/star.ogg", false, 1, 500);
        pAudio->Play_Music(pActive_Level->m_musicfile, true, 0);
    }
    else if (save_type == 1) { //Level loaded - level music
        pAudio->Play_Music(pActive_Level->m_musicfile, true, 1, 1000);
    }
    else { //World map loaded -- world map music
        pAudio->Play_Music(pActive_Overworld->m_musicfile, true, 1, 1000);
    }

    gp_hud->Set_Points(savegame->m_points);
    gp_hud->Set_Jewels(savegame->m_goldpieces);
    gp_hud->Set_Lives(savegame->m_lives);
    gp_hud->Set_Item(static_cast<SpriteType>(savegame->m_itembox_item), 0);
    gp_hud->Set_Text(_("Savegame ") + int_to_string(save_slot) + _(" loaded"));

    // #### Return stack ####

    pLevel_Player->Clear_Return();
    if (!savegame->m_return_entries.empty()) {
        std::vector<cSave_Player_Return_Entry>::const_iterator return_iter;
        for (return_iter = savegame->m_return_entries.begin(); return_iter != savegame->m_return_entries.end(); return_iter++) {
            pLevel_Player->Push_Return(return_iter->m_level, return_iter->m_entry);
        }
    }

    delete savegame;
    return save_type;
}

bool cSavegame::Save_Game(unsigned int save_slot, std::string description)
{
    if (pLevel_Player->m_alex_type == ALEX_DEAD || gp_hud->Get_Lives() < 0) {
        cerr << "Error : Couldn't save savegame " << description << " because of invalid game state" << endl;
        return 0;
    }

    cSave* savegame = new cSave();

    // General stuff
    savegame->m_version = SAVEGAME_VERSION;
    savegame->m_level_engine_version = level_engine_version;
    savegame->m_save_time = time(NULL);
    savegame->m_description = description;
    savegame->m_goldpieces = gp_hud->Get_Jewels();

    savegame->m_lives = gp_hud->Get_Lives();
    savegame->m_points = gp_hud->Get_Points();
    savegame->m_player_type = pLevel_Player->m_alex_type;
    savegame->m_player_type_temp_power = pLevel_Player->m_alex_type_temp_power;
    savegame->m_invincible = pLevel_Player->m_invincible;
    savegame->m_invincible_star = pLevel_Player->m_invincible_star;
    savegame->m_ghost_time = pLevel_Player -> m_ghost_time;
    savegame->m_ghost_time_mod = pLevel_Player -> m_ghost_time_mod;

    savegame->m_player_state = pLevel_Player->m_state;
    savegame->m_itembox_item = gp_hud->Get_Item();

    // player return stack
    for (std::vector<cLevel_Player_Return_Entry>::const_iterator itr = pLevel_Player->m_return_stack.begin(); itr != pLevel_Player->m_return_stack.end(); itr++) {
        savegame->m_return_entries.push_back(cSave_Player_Return_Entry(itr->level, itr->entry));
    }

    // if in a level
    if (pActive_Level->Is_Loaded()) {
        // General info relating to all loaded levels
        savegame->m_level_time = gp_hud->Get_Elapsed_Time();

        // Create save data for all loaded levels. Note that if a user entered
        // a sublevel, multiple levels can be loaded.
        for (vector<cLevel*>::iterator itr = pLevel_Manager->objects.begin(); itr != pLevel_Manager->objects.end(); ++itr) {
            cLevel* level = (*itr);

            if (!level->Is_Loaded()) {
                continue;
            }

            cSave_Level* save_level = new cSave_Level();

            // General info about the level
            save_level->m_name = path_to_utf8(Trim_Filename(level->m_level_filename, false, false));

            // Special treatment of the active level
            if (pActive_Level == level) {
                // Position.
                save_level->m_level_pos_x = pLevel_Player->m_pos_x;
                save_level->m_level_pos_y = pLevel_Player->m_pos_y - 5.0f;

                // Custom data a script writer wants to store in the
                // savegame (pSavegame holds the event table for the
                // level saving events).
                // TODO: Why not have mruby saving in sublevels?
                Scripting::cLevel_SaveLoad_Event evt(true);
                evt.Fire(pActive_Level->m_mruby, pSavegame);
                save_level->m_script_datas = evt.Get_Storage();
            }

            // All the sprites in the level.
            for (cSprite_List::iterator itr = level->m_sprite_manager->objects.begin(); itr != level->m_sprite_manager->objects.end(); ++itr) {
                cSprite* p_obj = (*itr);

                /* Spawned objects. These cannot be in the level XML
                 * by definition, e.g. they can be generated by level
                 * script or C++ code. */
                if (p_obj->m_spawned && !p_obj->m_suppress_save) {
                    if (!p_obj->m_auto_destroy) {
                        save_level->m_spawned_objects.push_back(p_obj);
                    }
                }

                // Base for every object; this will be loaded from the bare level XML.
                save_level->m_regular_objects.push_back(p_obj);
            }

            savegame->m_levels.push_back(save_level);
        }
    }

    // save overworld progress
    for (vector<cOverworld*>::iterator itr = pOverworld_Manager->objects.begin(); itr != pOverworld_Manager->objects.end(); ++itr) {
        // Get Overworld
        cOverworld* overworld = (*itr);

        // create Overworld
        cSave_Overworld* save_overworld = new cSave_Overworld();
        save_overworld->m_name = overworld->m_description->m_name;

        // Waypoints
        for (cSprite_List::iterator wp_itr = overworld->m_sprite_manager->objects.begin(); wp_itr != overworld->m_sprite_manager->objects.end(); ++wp_itr) {
            // get waypoint
            cSprite* obj = static_cast<cSprite*>(*wp_itr);

            if (obj->m_type != TYPE_OW_WAYPOINT) {
                continue;
            }

            // get waypoint
            cWaypoint* waypoint = static_cast<cWaypoint*>(obj);

            // create savegame waypoint
            cSave_Overworld_Waypoint* save_waypoint = new cSave_Overworld_Waypoint();

            // destination
            save_waypoint->m_destination = waypoint->Get_Destination();
            // set access
            save_waypoint->m_access = waypoint->m_access;
            // waypoint exits and their (un)lock state
            save_waypoint->m_exits = waypoint->m_exits;
            // save
            save_overworld->m_waypoints.push_back(save_waypoint);
        }

        savegame->m_overworlds.push_back(save_overworld);
    }

    // if an overworld is active and not custom level mode
    if (pActive_Overworld && Game_Mode_Type != MODE_TYPE_LEVEL_CUSTOM) {
        // set overworld name
        savegame->m_overworld_active = pActive_Overworld->m_description->m_name;

        // if valid waypoint
        if (pOverworld_Player->m_current_waypoint >= 0) {
            // set current waypoint
            savegame->m_overworld_current_waypoint = pOverworld_Player->m_current_waypoint;
        }
    }

    fs::path save_dir = pResource_Manager->Get_User_Savegame_Directory();
    fs::path filename = save_dir / utf8_to_path(int_to_string(save_slot) + ".tscsav");
    // remove old format savegame files
    fs::remove(save_dir / utf8_to_path(int_to_string(save_slot) + ".save"));
    fs::remove(save_dir / utf8_to_path(int_to_string(save_slot) + ".smcsav"));

    try {
        savegame->Write_To_File(filename);
    }
    catch (xmlpp::exception& e) {
        cerr << "Failed to save savegame '" << filename << "': " << e.what() << endl
             << "Is the file read-only?" << endl;
        gp_hud->Set_Text(_("Couldn't save savegame ") + path_to_utf8(filename));
    }

    gp_hud->Set_Text(_("Saved to Slot ") + int_to_string(save_slot));

    delete savegame;

    return 1;
}

cSave* cSavegame::Load(unsigned int save_slot)
{
    fs::path save_dir = pResource_Manager->Get_User_Savegame_Directory();
    fs::path filename = save_dir / utf8_to_path(int_to_string(save_slot) + ".tscsav");

    cSave* savegame = NULL; //The save game object read from the save state file

    //First try to load the game using the newer format
    if (File_Exists(filename)) {
        savegame = cSave::Load_From_File(filename);
    }
    else {
        //If it is not in the newer tscsav format, try the older .smcsav format
        fs::path filename_smc = save_dir / utf8_to_path(int_to_string(save_slot) + ".smcsav");
        if (File_Exists(filename_smc)) {
            savegame = cSave::Load_From_File(filename_smc);
        }
        else {
            //If it is not in the other two formats, try the very old .save format
            fs::path filename_old = m_savegame_dir / utf8_to_path(int_to_string(save_slot) + ".save");
            if (File_Exists(filename_old)) {
                savegame = cSave::Load_From_File(filename_old);
            }
            else {
                //There is not a file in any useful format -- throw an exception
                std::stringstream ss;
                ss << "No savegame found at slot " << save_slot << " (filename '" << filename << "' or '" << filename_old << "')!";
                throw(InvalidSavegameError(save_slot, ss.str()));
            }
        }
    }

    //Now check to make sure each level referenced in the save file exists
    if (!savegame->m_levels.empty()) {
        for (Save_LevelList::iterator itr = savegame->m_levels.begin(); itr != savegame->m_levels.end(); ++itr) {
            cSave_Level* save_level = (*itr);
            fs::path filename = pLevel_Manager->Get_Path(save_level->m_name);
            if (filename.empty()) {
                throw(InvalidLevelError("Empty level filename!"));
            }
            if (!File_Exists(filename)) {
                std::string msg = "Level file not found: " + path_to_utf8(filename);
                throw (InvalidLevelError(msg));
            }
        }

    }

    return savegame;
}

std::string cSavegame::Get_Description(unsigned int save_slot, bool only_description /* = 0 */)
{
    std::string str_description;

    if (!Is_Valid(save_slot)) {
        char str[255];

        // TRANS: %u is replaced by the number of the save slot, starting with 1.
        int count = sprintf(str, _("%u. Free Save"), save_slot);
        return std::string(str, count);
    }

    // Raises exceptions if fails; caller must take care of them.
    cSave* savegame = Load(save_slot);

    // complete description
    if (!only_description) {
        str_description = int_to_string(save_slot) + ". " + savegame->m_description;

        if (savegame->m_levels.empty()) {
            str_description += " - " + savegame->m_overworld_active;
        }
        else {
            bool found_active = 0;

            for (Save_LevelList::iterator itr = savegame->m_levels.begin(); itr != savegame->m_levels.end(); ++itr) {
                cSave_Level* level = (*itr);

                // if active level
                if (!Is_Float_Equal(level->m_level_pos_x, 0.0f) && !Is_Float_Equal(level->m_level_pos_y, 0.0f)) {
                    str_description += _(" -  Level ") + level->m_name;
                    found_active = 1;
                    break;
                }
            }

            if (!found_active) {
                str_description += _(" -  Unknown");
            }
        }

        str_description += _(" - Date ") + Time_to_String(savegame->m_save_time, "%Y-%m-%d  %H:%M:%S");
    }
    // only the user description
    else {
        str_description = savegame->m_description;
    }

    delete savegame;
    return str_description;
}

bool cSavegame::Is_Valid(unsigned int save_slot) const
{
    fs::path save_dir = pResource_Manager->Get_User_Savegame_Directory();
    return (File_Exists(save_dir / utf8_to_path(int_to_string(save_slot) + ".tscsav")) || File_Exists(save_dir / utf8_to_path(int_to_string(save_slot) + ".smcsav")) ||
            File_Exists(save_dir / utf8_to_path(int_to_string(save_slot) + ".save")));
}

cSavegame* pSavegame = NULL;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

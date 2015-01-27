/***************************************************************************
 * game_core.cpp  -  globally used variables and functions
 *
 * Copyright © 2003 - 2011 Florian Richter
 * Copyright © 2013 - 2014 The TSC Contributors
 ***************************************************************************/
/*
 *   This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../core/game_core.hpp"
#include "../audio/audio.hpp"
#include "../input/keyboard.hpp"
#include "../input/mouse.hpp"
#include "../input/joystick.hpp"
#include "../level/level_editor.hpp"
#include "../level/level_player.hpp"
#include "../video/renderer.hpp"
#include "../level/level.hpp"
#include "../core/sprite_manager.hpp"
#include "../overworld/overworld.hpp"
#include "../gui/menu.hpp"
#include "../core/i18n.hpp"
#include "../core/filesystem/filesystem.hpp"
#include "../objects/level_exit.hpp"
#include "../gui/menu_data.hpp"
#include "../user/savegame.hpp"
#include "../overworld/world_editor.hpp"
#include "filesystem/resource_manager.hpp"
#include "../core/global_basic.hpp"

using namespace std;

namespace fs = boost::filesystem;

namespace TSC {

/* *** *** *** *** *** *** *** *** Variables *** *** *** *** *** *** *** *** *** */

bool game_exit = 0;
GameMode Game_Mode = MODE_NOTHING;
GameModeType Game_Mode_Type = MODE_TYPE_DEFAULT;
GameAction Game_Action = GA_NONE;
CEGUI::XMLAttributes Game_Action_Data_Start;
CEGUI::XMLAttributes Game_Action_Data_Middle;
CEGUI::XMLAttributes Game_Action_Data_End;
void* Game_Action_ptr = NULL;

int game_res_w = 800;
int game_res_h = 600;

bool game_debug = 0;
bool game_debug_performance = 0;

SDL_Event input_event;

float global_upscalex = 1.0f;
float global_upscaley = 1.0f;
float global_downscalex = 1.0f;
float global_downscaley = 1.0f;

// FIXME: Duplicates the information in pLevel_Editor->m_enabled (set in cLevel::Enter())
bool editor_enabled = 0;
bool editor_level_enabled = 0;
bool editor_world_enabled = 0;

cCamera* pActive_Camera = NULL;
cSprite* pActive_Player = NULL;


/* *** *** *** *** *** *** *** Functions *** *** *** *** *** *** *** *** *** *** */

void Handle_Game_Events(void)
{
    // if game action is set
    while (Game_Action != GA_NONE) {
        // get current data
        const GameMode current_game_mode = Game_Mode;
        const GameModeType current_game_mode_type = Game_Mode_Type;
        const GameAction current_game_action = Game_Action;
        const CEGUI::XMLAttributes current_game_action_data_start = Game_Action_Data_Start;
        const CEGUI::XMLAttributes current_game_action_data_middle = Game_Action_Data_Middle;
        const CEGUI::XMLAttributes current_game_action_data_end = Game_Action_Data_End;
        void* current_game_action_ptr = Game_Action_ptr;
        // clear
        Game_Action = GA_NONE;
        Game_Action_Data_Start = CEGUI::XMLAttributes();
        Game_Action_Data_Middle = CEGUI::XMLAttributes();
        Game_Action_Data_End = CEGUI::XMLAttributes();
        Game_Action_ptr = NULL;

        // handle player downgrade
        if (current_game_action == GA_DOWNGRADE_PLAYER) {
            Handle_Generic_Game_Events(current_game_action_data_start);
            pLevel_Player->DownGrade_Player(false, current_game_action_data_middle.getValueAsBool("downgrade_force"), current_game_action_data_middle.getValueAsBool("downgrade_ignore_invincible"));
            Handle_Generic_Game_Events(current_game_action_data_middle);
            Handle_Generic_Game_Events(current_game_action_data_end);
        }
        // activate level exit
        else if (current_game_action == GA_ACTIVATE_LEVEL_EXIT) {
            Handle_Generic_Game_Events(current_game_action_data_start);
            cLevel_Exit* level_exit = static_cast<cLevel_Exit*>(current_game_action_ptr);
            level_exit->Activate();
            Handle_Generic_Game_Events(current_game_action_data_middle);
            Handle_Generic_Game_Events(current_game_action_data_end);
        }
        // full events
        else {
            GameMode new_mode = MODE_NOTHING;

            if (current_game_action == GA_ENTER_LEVEL) {
                new_mode = MODE_LEVEL;
            }
            else if (current_game_action == GA_ENTER_WORLD) {
                new_mode = MODE_OVERWORLD;
            }
            else if (current_game_action == GA_ENTER_MENU) {
                new_mode = MODE_MENU;
            }
            else if (current_game_action == GA_ENTER_LEVEL_SETTINGS) {
                new_mode = MODE_LEVEL_SETTINGS;
            }

            Handle_Generic_Game_Events(current_game_action_data_start);
            Leave_Game_Mode(new_mode);
            Handle_Generic_Game_Events(current_game_action_data_middle);
            Enter_Game_Mode(new_mode);
            Handle_Generic_Game_Events(current_game_action_data_end);
        }
    }
}

void Handle_Generic_Game_Events(const CEGUI::XMLAttributes& action_data)
{
    if (action_data.exists("music_fadeout")) {
        pAudio->Fadeout_Music(action_data.getValueAsInteger("music_fadeout"));
    }
    if (action_data.getValueAsBool("reset_save")) {
        pLevel_Player->Reset_Save();
    }
    if (action_data.getValueAsBool("unload_levels")) {
        pLevel_Manager->Unload();
    }
    if (action_data.getValueAsBool("unload_menu")) {
        pMenuCore->Unload();
    }
    if (action_data.exists("load_menu")) {
        MenuID menu = static_cast<MenuID>(action_data.getValueAsInteger("load_menu"));
        pMenuCore->Load(menu, static_cast<GameMode>(action_data.getValueAsInteger("menu_exit_back_to")));

        if (menu == MENU_START && action_data.exists("menu_start_current_level")) {
            cMenu_Start* menu_start = static_cast<cMenu_Start*>(pMenuCore->m_menu_data);
            menu_start->Highlight_Level(action_data.getValueAsString("menu_start_current_level").c_str());
        }
    }
    // set active world
    if (action_data.exists("enter_world")) {
        pOverworld_Manager->Set_Active(action_data.getValueAsString("enter_world").c_str());
    }
    // set player waypoint
    if (action_data.exists("world_player_waypoint")) {
        // get world waypoint
        int waypoint_num = pActive_Overworld->Get_Waypoint_Num(action_data.getValueAsString("world_player_waypoint").c_str());

        // waypoint available
        if (waypoint_num >= 0) {
            // set the previous waypoints accessible
            pActive_Overworld->Set_Progress(waypoint_num, 0);
            pOverworld_Player->Set_Waypoint(waypoint_num);
        }
    }
    if (action_data.exists("new_level")) {
        std::string str_level = action_data.getValueAsString("new_level").c_str();
        // new level
        cLevel* level = pLevel_Manager->New(str_level);

        if (level) {
            pLevel_Manager->Set_Active(level);
            level->Init();
        }
    }
    if (action_data.exists("load_level")) {
        bool loading_sublevel = action_data.exists("load_level_sublevel");
        std::string str_level = action_data.getValueAsString("load_level").c_str();
        // load the level
        cLevel* level = pLevel_Manager->Load(str_level, loading_sublevel);

        if (level) {
            pLevel_Manager->Set_Active(level);
            level->Init();

            if (action_data.exists("load_level_entry")) {
                std::string str_entry = action_data.getValueAsString("load_level_entry").c_str();
                cLevel_Entry* entry = level->Get_Entry(str_entry);

                // set camera position to show the entry
                if (entry) {
                    // set position
                    pLevel_Player->Set_Pos(entry->Get_Player_Pos_X(), entry->Get_Player_Pos_Y());
                    // center camera position
                    pActive_Camera->Center();
                    // set invisible for warp animation
                    pLevel_Player->Set_Active(0);
                }
                else if (!str_entry.empty()) {
                    cerr << "Warning : Level entry " << str_entry << " not found" << endl;
                }
            }
        }
        // loading failed
        else {
            cerr << "Error : Level not found " << str_level << endl;
            pHud_Debug->Set_Text(_("Loading Level failed : ") + str_level);

            pLevel_Manager->Finish_Level();
        }
    }
    if (action_data.exists("load_savegame")) {
        pSavegame->Load_Game(action_data.getValueAsInteger("load_savegame"));
    }
    if (action_data.exists("play_music")) {
        pAudio->Play_Music(action_data.getValueAsString("play_music").c_str(), action_data.getValueAsInteger("music_loops"), action_data.getValueAsBool("music_force", 1), action_data.getValueAsInteger("music_fadein"));
    }
    if (action_data.exists("screen_fadeout")) {
        Draw_Effect_Out(static_cast<Effect_Fadeout>(action_data.getValueAsInteger("screen_fadeout")), action_data.getValueAsFloat("screen_fadeout_speed", 1));
    }
    if (action_data.exists("screen_fadein")) {
        Draw_Effect_In(static_cast<Effect_Fadein>(action_data.getValueAsInteger("screen_fadein")), action_data.getValueAsFloat("screen_fadein_speed", 1));
    }
    if (action_data.exists("activate_level_entry")) {
        std::string str_entry = action_data.getValueAsString("activate_level_entry").c_str();
        cLevel_Entry* entry = pActive_Level->Get_Entry(str_entry);

        if (entry) {
            pLevel_Manager->Goto_Sub_Level("", entry->m_entry_name, CAMERA_MOVE_NONE);
        }
    }
    if (action_data.getValueAsBool("activate_editor")) {
        if (Game_Mode == MODE_LEVEL) {
            pLevel_Editor->Enable();
        }
        else if (Game_Mode == MODE_OVERWORLD) {
            pWorld_Editor->Enable();
        }
    }
}

void Leave_Game_Mode(const GameMode next_mode)
{
    if (Game_Mode == MODE_OVERWORLD) {
        pActive_Overworld->Leave(next_mode);
    }
    else if (Game_Mode == MODE_LEVEL) {
        pActive_Level->Leave(next_mode);
    }
    else if (Game_Mode == MODE_MENU) {
        pMenuCore->Leave(next_mode);
    }
    else if (Game_Mode == MODE_LEVEL_SETTINGS) {
        pLevel_Editor->m_settings_screen->Leave();
    }
}

void Enter_Game_Mode(const GameMode new_mode)
{
    // remember old mode
    GameMode old_mode = Game_Mode;
    // set new mode
    Game_Mode = new_mode;

    // mode gets level
    if (new_mode == MODE_LEVEL) {
        pActive_Level->Enter(old_mode);
    }
    // mode gets overworld
    else if (new_mode == MODE_OVERWORLD) {
        pActive_Overworld->Enter(old_mode);
    }
    // mode gets menu
    else if (new_mode == MODE_MENU) {
        pMenuCore->Enter(old_mode);
    }
    // mode gets settings
    else if (new_mode == MODE_LEVEL_SETTINGS) {
        pLevel_Editor->m_settings_screen->Enter();
    }
}

void Clear_Input_Events(void)
{
    while (SDL_PollEvent(&input_event)) {
        // todo : keep Windowmanager quit events ?
        // ignore all events
    }

    // Reset keys
    pKeyboard->Reset_Keys();
    pMouseCursor->Reset_Keys();
    pJoystick->Reset_keys();
}

void Preload_Images(bool draw_gui /* = 0 */)
{
    // progress bar
    CEGUI::ProgressBar* progress_bar = NULL;

    if (draw_gui) {
        // get progress bar
        progress_bar = static_cast<CEGUI::ProgressBar*>(CEGUI::WindowManager::getSingleton().getWindow("progress_bar"));
        progress_bar->setProgress(0);
        // set loading screen text
        Loading_Screen_Draw_Text(_("Loading Images"));
    }

    // image files
    vector<fs::path> image_files;

    // player
    vector<fs::path> player_small_images    = Get_Directory_Files(pResource_Manager->Get_Game_Pixmaps_Directory() / utf8_to_path("alex/small"), ".png", false, false);
    vector<fs::path> player_big_images      = Get_Directory_Files(pResource_Manager->Get_Game_Pixmaps_Directory() / utf8_to_path("alex/big"), ".png", false, false);
    vector<fs::path> player_fire_images     = Get_Directory_Files(pResource_Manager->Get_Game_Pixmaps_Directory() / utf8_to_path("alex/fire"), ".png", false, false);
    vector<fs::path> player_ice_images      = Get_Directory_Files(pResource_Manager->Get_Game_Pixmaps_Directory() / utf8_to_path("alex/ice"), ".png", false, false);
    vector<fs::path> player_ghost_images    = Get_Directory_Files(pResource_Manager->Get_Game_Pixmaps_Directory() / utf8_to_path("alex/ghost"), ".png", false, false);

    image_files.insert(image_files.end(), player_small_images.begin(), player_small_images.end());
    image_files.insert(image_files.end(), player_big_images.begin(), player_big_images.end());
    image_files.insert(image_files.end(), player_fire_images.begin(), player_fire_images.end());
    image_files.insert(image_files.end(), player_ice_images.begin(), player_ice_images.end());
    image_files.insert(image_files.end(), player_ghost_images.begin(), player_ghost_images.end());

    // Mushrooms
    image_files.push_back(utf8_to_path("game/items/mushroom_red.png"));
    image_files.push_back(utf8_to_path("game/items/mushroom_green.png"));
    image_files.push_back(utf8_to_path("game/items/mushroom_blue.png"));
    image_files.push_back(utf8_to_path("game/items/mushroom_ghost.png"));
    // Fireberry
    image_files.push_back(utf8_to_path("game/items/fireberry_1.png"));
    image_files.push_back(utf8_to_path("game/items/fireberry_2.png"));
    image_files.push_back(utf8_to_path("game/items/fireberry_3.png"));
    // Star
    image_files.push_back(utf8_to_path("game/items/star.png"));
    // Feather
    //image_files.push_back( "game/items/feather_1.png" );
    // Yellow Goldpiece
    image_files.push_back(utf8_to_path("game/items/goldpiece/yellow/1.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/yellow/2.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/yellow/3.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/yellow/4.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/yellow/5.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/yellow/6.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/yellow/7.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/yellow/8.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/yellow/9.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/yellow/10.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/yellow/1_falling.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/yellow/2_falling.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/yellow/3_falling.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/yellow/4_falling.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/yellow/5_falling.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/yellow/6_falling.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/yellow/7_falling.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/yellow/8_falling.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/yellow/9_falling.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/yellow/10_falling.png"));
    // Red Goldpiece
    image_files.push_back(utf8_to_path("game/items/goldpiece/red/1.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/red/2.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/red/3.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/red/4.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/red/5.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/red/6.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/red/7.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/red/8.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/red/9.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/red/10.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/red/1_falling.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/red/2_falling.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/red/3_falling.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/red/4_falling.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/red/5_falling.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/red/6_falling.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/red/7_falling.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/red/8_falling.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/red/9_falling.png"));
    image_files.push_back(utf8_to_path("game/items/goldpiece/red/10_falling.png"));

    // Brown Box
    image_files.push_back(utf8_to_path("game/box/brown1_1.png"));

    // Light animation
    image_files.push_back(utf8_to_path("animation/light_1/1.png"));
    image_files.push_back(utf8_to_path("animation/light_1/2.png"));
    image_files.push_back(utf8_to_path("animation/light_1/3.png"));
    // Particle animations
    image_files.push_back(utf8_to_path("animation/particles/fire_1.png"));
    image_files.push_back(utf8_to_path("animation/particles/fire_2.png"));
    image_files.push_back(utf8_to_path("animation/particles/fire_3.png"));
    image_files.push_back(utf8_to_path("animation/particles/fire_4.png"));
    image_files.push_back(utf8_to_path("animation/particles/smoke.png"));
    image_files.push_back(utf8_to_path("animation/particles/smoke_black.png"));
    image_files.push_back(utf8_to_path("animation/particles/light.png"));
    image_files.push_back(utf8_to_path("animation/particles/dirt.png"));
    image_files.push_back(utf8_to_path("animation/particles/ice_1.png"));
    image_files.push_back(utf8_to_path("animation/particles/cloud.png"));
    image_files.push_back(utf8_to_path("animation/particles/axis.png"));

    // Ball
    image_files.push_back(utf8_to_path("animation/fireball/1.png"));
    image_files.push_back(utf8_to_path("animation/iceball/1.png"));

    // HUD
    image_files.push_back(utf8_to_path("game/alex_l.png"));
    image_files.push_back(utf8_to_path("game/gold_m.png"));
    image_files.push_back(utf8_to_path("game/itembox.png"));

    unsigned int loaded_files = 0;
    unsigned int file_count = image_files.size();

    // load images
    for (vector<fs::path>::iterator itr = image_files.begin(); itr != image_files.end(); ++itr) {
        // get filename
        fs::path filename = (*itr);

        // preload image
        pVideo->Get_Surface(filename);

        // count files
        loaded_files++;

        if (draw_gui) {
            // update progress
            progress_bar->setProgress(static_cast<float>(loaded_files) / static_cast<float>(file_count));

            Loading_Screen_Draw();
        }
    }
}

void Preload_Sounds(bool draw_gui /* = 0 */)
{
    // skip caching if disabled
    if (!pAudio->m_sound_enabled) {
        return;
    }

    // progress bar
    CEGUI::ProgressBar* progress_bar = NULL;

    if (draw_gui) {
        // get progress bar
        progress_bar = static_cast<CEGUI::ProgressBar*>(CEGUI::WindowManager::getSingleton().getWindow("progress_bar"));
        progress_bar->setProgress(0);
        // set loading screen text
        Loading_Screen_Draw_Text(_("Loading Sounds"));
    }

    // sound files
    vector<fs::path> sound_files;

    // player
    sound_files.push_back(utf8_to_path("wall_hit.wav"));
    sound_files.push_back(utf8_to_path("player/dead.ogg"));
    sound_files.push_back(utf8_to_path("itembox_get.ogg"));
    sound_files.push_back(utf8_to_path("itembox_set.ogg"));
    sound_files.push_back(utf8_to_path("player/pickup_item.wav"));
    sound_files.push_back(utf8_to_path("player/jump_small.ogg"));
    sound_files.push_back(utf8_to_path("player/jump_small_power.ogg"));
    sound_files.push_back(utf8_to_path("player/jump_big.ogg"));
    sound_files.push_back(utf8_to_path("player/jump_big_power.ogg"));
    sound_files.push_back(utf8_to_path("player/jump_ghost.ogg"));
    // todo : create again
    //sound_files.push_back( utf8_to_path( "player/alex_au.ogg" ) );
    sound_files.push_back(utf8_to_path("player/powerdown.ogg"));
    sound_files.push_back(utf8_to_path("player/ghost_end.ogg"));
    sound_files.push_back(utf8_to_path("player/run_stop.ogg"));
    sound_files.push_back(utf8_to_path("enter_pipe.ogg"));
    sound_files.push_back(utf8_to_path("leave_pipe.ogg"));

    // items
    sound_files.push_back(utf8_to_path("item/star_kill.ogg"));
    sound_files.push_back(utf8_to_path("item/fireball.ogg"));
    sound_files.push_back(utf8_to_path("item/iceball.wav"));
    sound_files.push_back(utf8_to_path("item/ice_kill.wav"));
    sound_files.push_back(utf8_to_path("item/fireball_explode.wav"));
    sound_files.push_back(utf8_to_path("item/fireball_repelled.wav"));
    sound_files.push_back(utf8_to_path("item/fireball_explosion.wav"));
    sound_files.push_back(utf8_to_path("item/iceball_explosion.wav"));
    sound_files.push_back(utf8_to_path("item/fireplant.ogg"));
    sound_files.push_back(utf8_to_path("item/jewel_1.ogg"));
    sound_files.push_back(utf8_to_path("item/jewel_2.ogg"));
    sound_files.push_back(utf8_to_path("item/live_up.ogg"));
    sound_files.push_back(utf8_to_path("item/live_up_2.ogg"));
    sound_files.push_back(utf8_to_path("item/mushroom.ogg"));
    sound_files.push_back(utf8_to_path("item/mushroom_ghost.ogg"));
    sound_files.push_back(utf8_to_path("item/mushroom_blue.wav"));
    sound_files.push_back(utf8_to_path("item/moon.ogg"));

    // box
    sound_files.push_back(utf8_to_path("item/empty_box.wav"));

    // enemies
    // eato
    sound_files.push_back(utf8_to_path("enemy/eato/die.ogg"));
    // gee
    sound_files.push_back(utf8_to_path("enemy/gee/die.ogg"));
    // furball
    sound_files.push_back(utf8_to_path("enemy/furball/die.ogg"));
    // furball boss
    sound_files.push_back(utf8_to_path("enemy/boss/furball/hit.wav"));
    sound_files.push_back(utf8_to_path("enemy/boss/furball/hit_failed.wav"));
    // flyon
    sound_files.push_back(utf8_to_path("enemy/flyon/die.ogg"));
    // krush
    sound_files.push_back(utf8_to_path("enemy/krush/die.ogg"));
    // rokko
    sound_files.push_back(utf8_to_path("enemy/rokko/activate.wav"));
    sound_files.push_back(utf8_to_path("enemy/rokko/hit.wav"));
    // spika
    sound_files.push_back(utf8_to_path("enemy/spika/move.ogg"));
    // thromp
    sound_files.push_back(utf8_to_path("enemy/thromp/hit.ogg"));
    sound_files.push_back(utf8_to_path("enemy/thromp/die.ogg"));
    // army
    sound_files.push_back(utf8_to_path("enemy/army/hit.ogg"));
    sound_files.push_back(utf8_to_path("enemy/army/shell/hit.ogg"));
    sound_files.push_back(utf8_to_path("enemy/army/stand_up.wav"));
    // turtle boss
    sound_files.push_back(utf8_to_path("enemy/boss/turtle/big_hit.ogg"));
    sound_files.push_back(utf8_to_path("enemy/boss/turtle/shell_attack.ogg"));
    sound_files.push_back(utf8_to_path("enemy/boss/turtle/power_up.ogg"));

    // default
    sound_files.push_back(utf8_to_path("sprout_1.ogg"));
    sound_files.push_back(utf8_to_path("stomp_1.ogg"));
    sound_files.push_back(utf8_to_path("stomp_4.ogg"));

    // savegame
    sound_files.push_back(utf8_to_path("savegame_load.ogg"));
    sound_files.push_back(utf8_to_path("savegame_save.ogg"));

    // overworld
    sound_files.push_back(utf8_to_path("waypoint_reached.ogg"));

    unsigned int loaded_files = 0;
    unsigned int file_count = sound_files.size();

    // load images
    for (vector<fs::path>::iterator itr = sound_files.begin(); itr != sound_files.end(); ++itr) {
        // get filename
        fs::path filename = (*itr);

        // preload it
        pAudio->Get_Sound_File(filename);

        // count files
        loaded_files++;

        if (draw_gui) {
            // update progress
            progress_bar->setProgress(static_cast<float>(loaded_files) / static_cast<float>(file_count));

            Loading_Screen_Draw();
        }
    }
}

void Add_Property(xmlpp::Element* p_element, const Glib::ustring& name, const Glib::ustring& value)
{
    xmlpp::Element* p_propnode = p_element->add_child("property");
    p_propnode->set_attribute("name", name);
    p_propnode->set_attribute("value", value);
}

void Replace_Property(xmlpp::Element* p_element, const Glib::ustring& name, const Glib::ustring& value)
{
    // Determine if the property exists first
    xmlpp::Node::NodeList children = p_element->get_children("property");

    for(xmlpp::Node::NodeList::iterator it = children.begin(); it != children.end(); ++it) {
        xmlpp::Element* element = dynamic_cast<xmlpp::Element*>(*it);

        if(element && element->get_attribute_value("name") == name) {
            element->set_attribute("value", value);
            return;
        }
    }

    // No match found
    Add_Property(p_element, name, value);
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

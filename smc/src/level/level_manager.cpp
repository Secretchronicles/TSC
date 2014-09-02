/***************************************************************************
 * level_manager.cpp  -  class for handling levels
 *
 * Copyright © 2007 - 2011 The SMC Contributors
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../level/level_manager.hpp"
#include "../core/main.hpp"
#include "../core/game_core.hpp"
#include "../core/filesystem/filesystem.hpp"
#include "../overworld/overworld.hpp"
#include "../core/framerate.hpp"
#include "../objects/path.hpp"
#include "../audio/audio.hpp"
#include "../level/level_editor.hpp"
#include "../core/filesystem/resource_manager.hpp"
#include "../core/filesystem/package_manager.hpp"
#include "../core/filesystem/vfs.hpp"
#include "../input/mouse.hpp"
#include "../core/global_basic.hpp"

using namespace std;

namespace fs = boost::filesystem;

namespace SMC {

/* *** *** *** *** *** cLevel_Manager *** *** *** *** *** *** *** *** *** *** *** *** */

cLevel_Manager :: cLevel_Manager(void)
    : cObject_Manager<cLevel>()
{
    m_camera = new cCamera(NULL);

    // set the first camera available
    if (pActive_Camera == NULL) {
        pActive_Camera = m_camera;
    }

    // always have one level around for the sprite manager
    pActive_Level = new cLevel();
    Add(pActive_Level);

    m_camera->Set_Sprite_Manager(pActive_Level->m_sprite_manager);
}

cLevel_Manager :: ~cLevel_Manager(void)
{
    Delete_All();
    delete m_camera;
}

void cLevel_Manager :: Init(void)
{

}

void cLevel_Manager :: Unload(void)
{
    // disable fixed camera velocity
    pLevel_Manager->m_camera->m_fixed_hor_vel = 0.0f;

    // always keep one level
    if (size() > 1) {
        for (vector<cLevel*>::iterator itr = objects.begin(); itr != objects.end() - 1;) {
            cLevel* obj = (*itr);

            itr = objects.erase(itr);
            delete obj;
        }

        pActive_Level = objects.front();
    }

    // keep the managers valid
    if (Game_Mode == MODE_LEVEL) {
        pActive_Level->Set_Sprite_Manager();
    }
    // always keep the level player manager valid
    else {
        pLevel_Player->Set_Sprite_Manager(pActive_Level->m_sprite_manager);
    }

    pActive_Level->Unload();
}

cLevel* cLevel_Manager :: New(std::string levelname)
{
    // clear player return stack when creating a new level
    pLevel_Player->Clear_Return();

    // if it already exists
    if (!Get_Path(levelname, true).empty()) {
        return NULL;
    }

    cLevel* level = new cLevel();
    // if failed to create
    if (!level->New(levelname)) {
        delete level;
        return NULL;
    }

    Add(level);
    return level;
}

cLevel* cLevel_Manager :: Load(std::string levelname , bool loading_sublevel /* = false */)
{
    // clear player return stack when loading a level if not going to a sublevel
    if (!loading_sublevel)
        pLevel_Player->Clear_Return();

    cLevel* level = Get(levelname);
    // already loaded
    if (level) {
        return level;
    }

    // load
    fs::path filename = Get_Path(levelname);
    level = cLevel::Load_From_File(filename);

    Add(level);
    return level;
}

bool cLevel_Manager :: Set_Active(cLevel* level)
{
    if (!level) {
        return 0;
    }

    pActive_Level = level;

    return 1;
}

cLevel* cLevel_Manager :: Get(const std::string& levelname)
{
    for (vector<cLevel*>::iterator itr = objects.begin(); itr != objects.end(); ++itr) {
        cLevel* obj = (*itr);

        if (obj->Get_Level_Name().compare(levelname) == 0) {
            return obj;
        }
    }

    return NULL;
}

fs::path cLevel_Manager :: Get_Path(const std::string& levelname, bool check_only_user_dir /* = false */)
{
    // Strip off directories and file extension (although we should
    // only receive the plain level name here).
    fs::path filename = Trim_Filename(utf8_to_path(levelname));

    // user level directory as default
    fs::path user_filename = fs::absolute(filename, pPackage_Manager->Get_User_Level_Path());
    // use new file type as default
    user_filename.replace_extension(".smclvl");

    if (pVfs->File_Exists(user_filename)) {
        // found
        return user_filename;
    }

    // use old file type
    user_filename.replace_extension(".txt");

    if (pVfs->File_Exists(user_filename)) {
        // found
        return user_filename;
    }

    if (!check_only_user_dir) {
        fs::path game_filename = fs::absolute(filename, pPackage_Manager->Get_Game_Level_Path());

        // use new file type
        game_filename.replace_extension(".smclvl");

        if (pVfs->File_Exists(game_filename)) {
            // found
            return game_filename;
        }

        // use old file type
        game_filename.replace_extension(".txt");

        if (pVfs->File_Exists(game_filename)) {
            // found
            return game_filename;
        }
    }

    // not found
    return fs::path();
}

void cLevel_Manager :: Update(void)
{
    // input
    pActive_Level->Process_Input();
    pLevel_Editor->Process_Input();

    // update performance timer
    pFramerate->m_perf_timer[PERF_UPDATE_PROCESS_INPUT]->Update();

    // update
    pActive_Level->Update();

    // update performance timer
    pFramerate->m_perf_timer[PERF_UPDATE_LEVEL]->Update();

    // editor
    pLevel_Editor->Update();

    // update performance timer
    pFramerate->m_perf_timer[PERF_UPDATE_LEVEL_EDITOR]->Update();

    // hud
    pHud_Manager->Update();

    // update performance timer
    pFramerate->m_perf_timer[PERF_UPDATE_HUD]->Update();

    // player
    pLevel_Player->Update();

    // update performance timer
    pFramerate->m_perf_timer[PERF_UPDATE_PLAYER]->Update();

    // player collisions
    if (!editor_enabled) {
        pLevel_Player->Collide_Move();
        pLevel_Player->Handle_Collisions();
    }

    // update performance timer
    pFramerate->m_perf_timer[PERF_UPDATE_PLAYER_COLLISIONS]->Update();

    // late update for level objects
    pActive_Level->Update_Late();

    // update performance timer
    pFramerate->m_perf_timer[PERF_UPDATE_LATE_LEVEL]->Update();

    // level collisions
    if (!editor_enabled) {
        pActive_Level->m_sprite_manager->Handle_Collision_Items();
    }

    // update performance timer
    pFramerate->m_perf_timer[PERF_UPDATE_LEVEL_COLLISIONS]->Update();

    // Camera ( update after new player position was set )
    pActive_Camera->Update();

    // update performance timer
    pFramerate->m_perf_timer[PERF_UPDATE_CAMERA]->Update();
}

void cLevel_Manager :: Draw(void)
{
    // clear
    pVideo->Clear_Screen();

    // draw level layer 1
    pActive_Level->Draw_Layer_1();

    // update performance timer
    pFramerate->m_perf_timer[PERF_DRAW_LEVEL_LAYER1]->Update();

    // player draw
    pLevel_Player->Draw();

    // update performance timer
    pFramerate->m_perf_timer[PERF_DRAW_LEVEL_PLAYER]->Update();

    // draw level layer 2
    pActive_Level->Draw_Layer_2();

    // update performance timer
    pFramerate->m_perf_timer[PERF_DRAW_LEVEL_LAYER2]->Update();

    // hud
    pHud_Manager->Draw();

    // update performance timer
    pFramerate->m_perf_timer[PERF_DRAW_LEVEL_HUD]->Update();

    // level editor
    pLevel_Editor->Draw();

    // update performance timer
    pFramerate->m_perf_timer[PERF_DRAW_LEVEL_EDITOR]->Update();
}

void cLevel_Manager :: Finish_Level(bool win_music /* = 0 */)
{
    pHud_Time->Reset();

    // clear player return stack when finishing a level
    pLevel_Player->Clear_Return();

    // custom level
    if (Game_Mode_Type == MODE_TYPE_LEVEL_CUSTOM) {
        // Enter Menu
        Game_Action = GA_ENTER_MENU;
        Game_Action_Data_Start.add("screen_fadeout_speed", "1.5");
        Game_Action_Data_Middle.add("load_menu", int_to_string(MENU_MAIN));
        Game_Action_Data_Middle.add("menu_start_current_level", path_to_utf8(Trim_Filename(pActive_Level->m_level_filename, 0, 0)));
        Game_Action_Data_End.add("screen_fadein_speed", "1.5");
    }
    // normal level
    else {
        // Finish level
        pActive_Overworld->Goto_Next_Level();
        // Enter World
        Game_Action = GA_ENTER_WORLD;
    }

    Game_Action_Data_Start.add("music_fadeout", "1500");
    Game_Action_Data_Start.add("screen_fadeout", CEGUI::PropertyHelper::intToString(EFFECT_OUT_RANDOM));
    if (win_music) {
        Game_Action_Data_Middle.add("play_music", "game/courseclear.ogg");
    }
    // delay unload level
    Game_Action_Data_Middle.add("unload_levels", "1");
    Game_Action_Data_End.add("screen_fadein", CEGUI::PropertyHelper::intToString(EFFECT_IN_RANDOM));
}

void cLevel_Manager :: Goto_Sub_Level(std::string str_level, const std::string& str_entry, Camera_movement move_camera /* = CAMERA_MOVE_FLY */, const std::string& path_identifier /* = "" */)
{
    // if empty use same level
    if (str_level.empty()) {
        str_level = path_to_utf8(Trim_Filename(pActive_Level->m_level_filename, 0, 0));
    }

    // same level
    if (str_level.compare(path_to_utf8(Trim_Filename(pActive_Level->m_level_filename, 0, 0))) == 0) {
        pLevel_Player->Release_Item(1, 1);
        // fixme: reset does not release active_object correctly
        pLevel_Player->Reset(0);

        // get entry
        cLevel_Entry* entry = pActive_Level->Get_Entry(str_entry);

        // use entry position
        if (entry) {
            pLevel_Player->Set_Pos(entry->Get_Player_Pos_X(), entry->Get_Player_Pos_Y());
            // set invisible for warp animation
            pLevel_Player->Set_Active(0);
        }
        // not found
        else if (!str_entry.empty()) {
            cerr << "Warning : Level entry " << str_entry << " not found" << endl;
        }

        // move camera to new position
        if (move_camera != CAMERA_MOVE_NONE) {
            pFramerate->m_speed_factor = 1.0f;

            // fly to new position or move along path to new position
            if (move_camera == CAMERA_MOVE_FLY || move_camera == CAMERA_MOVE_ALONG_PATH || move_camera == CAMERA_MOVE_ALONG_PATH_BACKWARDS) {
                // move along path
                if (move_camera == CAMERA_MOVE_ALONG_PATH || move_camera == CAMERA_MOVE_ALONG_PATH_BACKWARDS) {
                    // create path state
                    cPath_State path_state = cPath_State(pActive_Level->m_sprite_manager);
                    // set parent path
                    path_state.Set_Path_Identifier(path_identifier);

                    // path found
                    if (path_state.m_path) {
                        // forward
                        if (move_camera == CAMERA_MOVE_ALONG_PATH) {
                            path_state.Move_Start_Forward();
                        }
                        // backward
                        else {
                            path_state.Move_Start_Backward();
                        }

                        // start path position
                        float start_path_pos_x = path_state.m_path->m_col_rect.m_x - (game_res_w * 0.5f);
                        float start_path_pos_y = path_state.m_path->m_col_rect.m_y - (game_res_h * 0.5f);
                        // move gradually to start point
                        pActive_Camera->Move_to_Position_Gradually(start_path_pos_x + path_state.m_pos_x, start_path_pos_y + path_state.m_pos_y, 20);
                        // reset offset
                        //pActive_Camera->x_offset = 0;
                        //pActive_Camera->y_offset = 0;

                        while (1) {
                            // move along path
                            if (path_state.Path_Move(30 * pFramerate->m_speed_factor) == 0) {
                                // we can not move further along the path
                                break;
                            }

                            // update audio
                            pAudio->Update();
                            // Move camera
                            pActive_Camera->Set_Pos(start_path_pos_x + path_state.m_pos_x, start_path_pos_y + path_state.m_pos_y);

                            // keep particles on screen
                            for (cSprite_List::iterator itr = pActive_Level->m_sprite_manager->objects.begin(); itr != pActive_Level->m_sprite_manager->objects.end(); ++itr) {
                                cSprite* obj = (*itr);

                                if (obj->m_type == TYPE_PARTICLE_EMITTER) {
                                    cParticle_Emitter* emitter = static_cast<cParticle_Emitter*>(obj);
                                    emitter->Update_Position();
                                }
                            }

                            // draw
                            Draw_Game();

                            pVideo->Render();
                            pFramerate->Update();
                        }
                    }
                    // not found
                    else {
                        cerr << "Warning : Level path " << path_identifier << " not found" << endl;
                    }
                }

                /* fly to player position
                * also needed after the move along path or it is not smooth
                */
                pActive_Camera->Move_to_Position_Gradually(pActive_Camera->Get_Center_Pos_X(), pActive_Camera->Get_Center_Pos_Y());
            }
            // blink to new position
            else {
                Draw_Effect_Out(EFFECT_OUT_BLACK, 5);
                pActive_Camera->Center();
                Draw_Effect_In(EFFECT_IN_BLACK, 5);
            }
        }

        // warp animation
        if (entry) {
            // set back visible
            pLevel_Player->Set_Active(1);
            // activate entry
            entry->Activate();
        }
    }
    // another level
    else {
        cLevel* level = pLevel_Manager->Load(str_level, true);

        if (level) {
            Game_Action = GA_ENTER_LEVEL;
            // only fade out music if different
            if (pActive_Level->Get_Music_Filename().filename().compare(level->Get_Music_Filename().filename()) != 0) {
                Game_Action_Data_Start.add("music_fadeout", "1000");
            }
            Game_Action_Data_Start.add("screen_fadeout", CEGUI::PropertyHelper::intToString(EFFECT_OUT_HORIZONTAL_VERTICAL));
            Game_Action_Data_Start.add("screen_fadeout_speed", "3");
            Game_Action_Data_Middle.add("load_level", str_level.c_str());
            Game_Action_Data_Middle.add("load_level_entry", str_entry.c_str());
            Game_Action_Data_Middle.add("load_level_sublevel", "1");
            Game_Action_Data_End.add("screen_fadein", CEGUI::PropertyHelper::intToString(EFFECT_IN_BLACK));
            Game_Action_Data_End.add("screen_fadein_speed", "3");
            Game_Action_Data_End.add("activate_level_entry", str_entry.c_str());
        }
    }
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

// Level information handler
cLevel_Manager* pLevel_Manager = NULL;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace SMC

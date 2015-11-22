/***************************************************************************
 * overworld.cpp  -  Overworld class
 *
 * Copyright © 2003 - 2011 Florian Richter
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

#include "../core/global_basic.hpp"
#include "../overworld/overworld.hpp"
#include "../audio/audio.hpp"
#include "../core/game_core.hpp"
#include "../level/level_editor.hpp"
#include "../overworld/world_editor.hpp"
#include "../core/framerate.hpp"
#include "../gui/menu.hpp"
#include "../user/preferences.hpp"
#include "../video/font.hpp"
#include "../input/mouse.hpp"
#include "../input/joystick.hpp"
#include "../input/keyboard.hpp"
#include "../level/level.hpp"
#include "../core/i18n.hpp"
#include "../core/filesystem/filesystem.hpp"
#include "../core/filesystem/resource_manager.hpp"
#include "../core/filesystem/package_manager.hpp"
#include "overworld_description_loader.hpp"
#include "overworld_layer_loader.hpp"
#include "overworld_loader.hpp"

namespace fs = boost::filesystem;

using namespace std;

namespace TSC {

/* *** *** *** *** *** *** *** *** cOverworld_description *** *** *** *** *** *** *** *** *** */

cOverworld_description::cOverworld_description(void)
{
    m_path = "world_1"; // FIXME: Don’t assume this world exists.
    m_name = _("Unnamed");
    m_visible = 1;
    m_user = 0;

    m_comment = _("Empty");
}

cOverworld_description::~cOverworld_description(void)
{
    //
}

void cOverworld_description::Save(void)
{
    fs::path filename = pPackage_Manager->Get_User_World_Path() / m_path.filename() / utf8_to_path("description.xml");

    try {
        Save_To_File(filename);
    }
    catch (xmlpp::exception& e) {
        cerr << "Failed to save world description file '" << path_to_utf8(filename) << "': " << e.what() << endl;
        pHud_Debug->Set_Text(_("Couldn't save world description ") + path_to_utf8(filename), speedfactor_fps * 5.0f);
        return;
    }
}

void cOverworld_description::Save_To_File(fs::path path)
{
    xmlpp::Document doc;
    xmlpp::Element* p_root = doc.create_root_node("description");
    xmlpp::Element* p_node = p_root->add_child("world");

    Add_Property(p_node, "name", m_name);
    Add_Property(p_node, "visible", m_visible);

    // Write to file (raises xmlpp::exception on error)
    doc.write_to_file_formatted(Glib::filename_from_utf8(path_to_utf8(path)));
    debug_print("Wrote world description file '%s'.\n", path_to_utf8(path).c_str());
}

fs::path cOverworld_description::Get_Path()
{
    return m_path;
}

void cOverworld_description::Set_Path(fs::path directory, bool set_name /* = false */)
{
    m_path = directory;

    if (set_name)
        m_name = path_to_utf8(directory);
}

/* *** *** *** *** *** *** *** *** cOverworld *** *** *** *** *** *** *** *** *** */

cOverworld::cOverworld(void)
{
    Init();
}

cOverworld* cOverworld::Load_From_Directory(fs::path directory, int user_dir /* = 0 */)
{
    // Overworld loading consists of three steps: Loading the description file,
    // loading the main world file and loading the layers file.
    debug_print("Loading world from directory '%s'\n", path_to_utf8(directory).c_str());

    //////// Step 1: Description file ////////
    cOverworldDescriptionLoader descloader;
    cOverworld_description* p_desc = NULL;
    descloader.parse_file(directory / utf8_to_path("description.xml"));
    p_desc = descloader.Get_Overworld_Description();
    p_desc->Set_Path(directory); // FIXME: Post-initialization violates OOP principle of secrecy. `m_path' needs to be moved into cOverworld!
    p_desc->m_user = user_dir; // FIXME: Post-initialization violates OOP principle of secrecy.

    //////// Step 2: Main world file ////////
    cOverworldLoader worldloader;
    worldloader.parse_file(directory / utf8_to_path("world.xml"));
    cOverworld* p_overworld = worldloader.Get_Overworld();

    // Replace the old default description for world_1 with the correct one
    // we loaded previously.
    delete p_overworld->m_description;
    p_overworld->m_description = p_desc;

    //////// Step 3: Layers file ////////
    cOverworldLayerLoader layerloader(p_overworld);
    layerloader.parse_file(directory / utf8_to_path("layer.xml"));

    // Replace the old default layer with the one we just loaded
    delete p_overworld->m_layer;
    p_overworld->m_layer = layerloader.Get_Layer();

    // Set the text that is displayed at the top when this world is shown
    p_overworld->m_hud_world_name->Set_Image(pFont->Render_Text(pFont->m_font_normal, p_overworld->m_description->m_name, yellow), true, true);

    return p_overworld;
}

cOverworld::~cOverworld(void)
{
    Unload();

    delete m_sprite_manager;
    delete m_animation_manager;
    delete m_description;
    delete m_layer;
    delete m_hud_level_name;
    delete m_hud_world_name;
}

void cOverworld::Init()
{
    m_sprite_manager = new cWorld_Sprite_Manager(this);
    m_animation_manager = new cAnimation_Manager();
    m_description = new cOverworld_description();
    m_layer = new cLayer(this);

    m_engine_version = -1;
    m_last_saved = 0;
    m_background_color = Color();
    m_musicfile = "overworld/land_1.ogg";
    m_hud_world_name = new cHudSprite(m_sprite_manager);
    m_hud_world_name->Set_Pos(10, static_cast<float>(game_res_h) - 30);
    m_hud_world_name->Set_Shadow(black, 1.5f);
    m_hud_level_name = new cHudSprite(m_sprite_manager);
    m_hud_level_name->Set_Pos(350, 2);
    m_hud_level_name->Set_Shadow(black, 1.5f);

    m_next_level = 0;

    m_player_start_waypoint = 0;
    m_player_moving_state = STA_STAY;
}

void cOverworld::Replace_Description(cOverworld_description* p_desc)
{
    delete m_description;
    m_description = p_desc;
}

bool cOverworld::New(std::string name)
{
    if (name.empty()) {
        return 0;
    }

    Unload();

    // set path
    m_description->m_path = name;
    // default name is the path
    m_description->m_name = name;

    m_background_color = Color(0.2f, 0.5f, 0.1f);
    m_engine_version = world_engine_version;

    return 1;
}

void cOverworld::Unload(void)
{
    // not loaded
    if (!Is_Loaded()) {
        return;
    }

    // Objects
    m_sprite_manager->Delete_All();
    // Waypoints
    m_waypoints.clear();
    // Layer
    m_layer->Delete_All();
    // animations
    m_animation_manager->Delete_All();

    // no engine version
    m_engine_version = -1;
    m_last_saved = 0;
}

void cOverworld::Save(void)
{
    pAudio->Play_Sound("editor/save.ogg");

    // Ensure we save in the user world dir (the user may take a game world to edit
    // and then save; in that case we want to save in the user dir, because the game
    // dir most likely is not writable at all.
    fs::path save_dir = pPackage_Manager->Get_User_World_Path() / m_description->m_path.filename();

    // Create directory if new world
    if (!Dir_Exists(save_dir)) {
        fs::create_directories(save_dir);
    }

    try {
        Save_To_Directory(save_dir);
    }
    catch (xmlpp::exception& e) {
        cerr << "Error: Could not save overworld '" << path_to_utf8(save_dir) << "': " << e.what() << endl
             << "Is the directory read-only?" << endl;
        pHud_Debug->Set_Text(_("Couldn't save world ") + path_to_utf8(save_dir), speedfactor_fps * 5.0f);
        return;
    }

    // show info
    pHud_Debug->Set_Text(_("World ") + m_description->m_name + _(" saved"));
}

void cOverworld::Save_To_Directory(fs::path path)
{
    // As with loading, saving is a three-step process:
    // main world file, layer file, description file.

    Save_To_File(path / utf8_to_path("world.xml"));
    m_layer->Save_To_File(path / utf8_to_path("layer.xml"));
    m_description->Save(); // FIXME: When m_path is moved to cOverworld, replace with call to cOverworld_description::Save_To_File()
}

void cOverworld::Save_To_File(fs::path path)
{
    xmlpp::Document doc;
    xmlpp::Element* p_root = doc.create_root_node("overworld");
    xmlpp::Element* p_node = NULL;

    // General information
    p_node = p_root->add_child("information");
    Add_Property(p_node, "game_version", int_to_string(TSC_VERSION_MAJOR) + "." + int_to_string(TSC_VERSION_MINOR) + "." + int_to_string(TSC_VERSION_PATCH));
    Add_Property(p_node, "engine_version", world_engine_version);
    Add_Property(p_node, "save_time", static_cast<Uint64>(time(NULL))); // seconds since 1970

    // Settings (currently only music)
    p_node = p_root->add_child("settings");
    Add_Property(p_node, "music", m_musicfile);

    // Background color
    p_node = p_root->add_child("background");
    Add_Property(p_node, "color_red", static_cast<int>(m_background_color.red));
    Add_Property(p_node, "color_green", static_cast<int>(m_background_color.green));
    Add_Property(p_node, "color_blue", static_cast<int>(m_background_color.blue));

    // Player
    p_node = p_root->add_child("player");
    Add_Property(p_node, "waypoint", m_player_start_waypoint);
    Add_Property(p_node, "moving_state", static_cast<int>(m_player_moving_state));

    cSprite_List::const_iterator iter;
    for (iter = m_sprite_manager->objects.begin(); iter != m_sprite_manager->objects.end(); iter++) {
        cSprite* p_sprite = (*iter);

        // Skip spawned and destroyed objects
        if (p_sprite->m_spawned || p_sprite->m_auto_destroy)
            continue;

        // Save below this node
        p_sprite->Save_To_XML_Node(p_root);
    }

    doc.write_to_file_formatted(Glib::filename_from_utf8(path_to_utf8(path)));
    debug_print("Wrote world file '%s'.\n", path_to_utf8(path).c_str());
}

void cOverworld::Enter(const GameMode old_mode /* = MODE_NOTHING */)
{
    if (game_exit) {
        return;
    }

    // if not loaded
    if (!Is_Loaded()) {
        return;
    }

    // set active camera
    pActive_Camera = pOverworld_Manager->m_camera;
    // set active player
    pActive_Player = pOverworld_Player;
    // set animation manager
    pActive_Animation_Manager = m_animation_manager;

    pHud_Manager->Set_Sprite_Manager(m_sprite_manager);
    pMouseCursor->Set_Sprite_Manager(m_sprite_manager);

    // if player start waypoint not set
    if (pOverworld_Player->m_current_waypoint < 0) {
        pOverworld_Player->Reset();
        pOverworld_Player->Set_Waypoint(m_player_start_waypoint, 1);
    }

    // if goto next level
    if (m_next_level) {
        Goto_Next_Level();
    }

    // if on start waypoint
    if (pOverworld_Player->m_current_waypoint == static_cast<int>(m_player_start_waypoint)) {
        // if player state is walk
        if (m_player_moving_state == STA_WALK) {
            // walk to the next Waypoint
            pOverworld_Player->Start_Walk(m_waypoints[pOverworld_Player->m_current_waypoint]->m_direction_forward);
        }
    }

    if (old_mode == MODE_NOTHING || old_mode == MODE_OVERWORLD) {
        // set default camera limit
        pOverworld_Manager->m_camera->Set_Limit_Y(0);
    }

    Update_Camera();

    // play music
    pAudio->Play_Music(m_musicfile, true, 0, 3000);

    // reset custom level mode type
    if (Game_Mode_Type == MODE_TYPE_LEVEL_CUSTOM) {
        Game_Mode_Type = MODE_TYPE_DEFAULT;
    }

    // disable level editor
    pLevel_Editor->Disable();

    // set editor enabled state
    editor_enabled = pWorld_Editor->m_enabled;

    if (pWorld_Editor->m_enabled) {
        if (!pWorld_Editor->m_editor_window->isVisible()) {
            pWorld_Editor->m_editor_window->show();
            pMouseCursor->Set_Active(1);
        }
    }

    // Update Hud Text and position
    pHud_Manager->Update_Text();

    // reset speedfactor
    pFramerate->Reset();
}

void cOverworld::Leave(const GameMode next_mode /* = MODE_NOTHING */)
{
    // if not in world mode
    if (Game_Mode != MODE_OVERWORLD) {
        return;
    }

    // reset mouse
    pMouseCursor->Reset(0);

    // world to world
    if (next_mode == MODE_OVERWORLD) {
        // fade out music
        pAudio->Fadeout_Music(500);
        return;
    }

    if (next_mode == MODE_LEVEL) {
        m_animation_manager->Delete_All();
    }

    // hide editor window if visible
    if (pWorld_Editor->m_enabled) {
        if (pWorld_Editor->m_editor_window->isVisible()) {
            pWorld_Editor->m_editor_window->hide();
        }
    }

    // if new mode is not menu
    if (next_mode != MODE_MENU) {
        // fade out music
        pAudio->Fadeout_Music(1000);

        // clear input
        Clear_Input_Events();
    }
}

void cOverworld::Draw(void)
{
    // Background
    pVideo->Clear_Screen();
    Draw_Layer_1();

    // Player
    pOverworld_Player->Draw();
    // Hud
    Draw_HUD();

    // Editor
    pWorld_Editor->Draw();

    // update performance timer
    pFramerate->m_perf_timer[PERF_DRAW_OVERWORLD]->Update();
}

void cOverworld::Draw_Layer_1(void)
{
    pVideo->Draw_Rect(NULL, 0.0001f, &m_background_color);

    // sprites
    m_sprite_manager->Draw_Items();
    // animations
    m_animation_manager->Draw();
}

void cOverworld::Draw_HUD(void)
{
    // if not editor mode
    if (!editor_world_enabled) {
        // Background
        Color color = Color(static_cast<Uint8>(230), 170, 0, 128);
        pVideo->Draw_Rect(0, 0, static_cast<float>(game_res_w), 30, 0.12f, &color);
        // Line
        color = Color(static_cast<Uint8>(200), 150, 0, 128);
        pVideo->Draw_Rect(0, 30, static_cast<float>(game_res_w), 5, 0.121f, &color);

        // Overworld name and level
        m_hud_world_name->Draw();
        m_hud_level_name->Draw();
    }

    // hud
    pHud_Manager->Draw();
}

void cOverworld::Update(void)
{
    // editor
    pWorld_Editor->Process_Input();

    if (!editor_world_enabled) {
        // Camera
        Update_Camera();
        // Map
        m_sprite_manager->Update_Items();
        // Player
        pOverworld_Player->Update();
        // Animations
        m_animation_manager->Update();
    }
    // if world-editor is enabled
    else {
        // only update particle emitters
        for (cSprite_List::iterator itr = m_sprite_manager->objects.begin(); itr != m_sprite_manager->objects.end(); ++itr) {
            cSprite* obj = (*itr);

            if (obj->m_type == TYPE_PARTICLE_EMITTER) {
                obj->Update();
            }
        }
    }

    // hud
    pHud_Manager->Update();
    // Editor
    pWorld_Editor->Update();

    // update performance timer
    pFramerate->m_perf_timer[PERF_UPDATE_OVERWORLD]->Update();
}

void cOverworld::Update_Camera(void)
{
    if (editor_world_enabled) {
        return;
    }

    // todo : move to a Process_Input function
    if (pOverworld_Manager->m_camera_mode) {
        if (pKeyboard->m_keys[pPreferences->m_key_right] || (pJoystick->m_right && pPreferences->m_joy_enabled)) {
            pOverworld_Manager->m_camera->Move(pFramerate->m_speed_factor * 15, 0);
        }
        else if (pKeyboard->m_keys[pPreferences->m_key_left] || (pJoystick->m_left && pPreferences->m_joy_enabled)) {
            pOverworld_Manager->m_camera->Move(pFramerate->m_speed_factor * -15, 0);
        }
        if (pKeyboard->m_keys[pPreferences->m_key_up] || (pJoystick->m_up && pPreferences->m_joy_enabled)) {
            pOverworld_Manager->m_camera->Move(0, pFramerate->m_speed_factor * -15);
        }
        else if (pKeyboard->m_keys[pPreferences->m_key_down] || (pJoystick->m_down && pPreferences->m_joy_enabled)) {
            pOverworld_Manager->m_camera->Move(0, pFramerate->m_speed_factor * 15);
        }
    }
    // default player camera
    else {
        pOverworld_Manager->m_camera->Update();
    }
}

bool cOverworld::Key_Down(SDLKey key)
{
    if (key == SDLK_LEFT) {
        if (!pOverworld_Manager->m_camera_mode && !editor_world_enabled) {
            pOverworld_Player->Action_Interact(INP_LEFT);
        }
        return 0;
    }
    else if (key == SDLK_RIGHT) {
        if (!pOverworld_Manager->m_camera_mode && !editor_world_enabled) {
            pOverworld_Player->Action_Interact(INP_RIGHT);
        }
        return 0;
    }
    else if (key == SDLK_UP) {
        if (!pOverworld_Manager->m_camera_mode && !editor_world_enabled) {
            pOverworld_Player->Action_Interact(INP_UP);
        }
        return 0;
    }
    else if (key == SDLK_DOWN) {
        if (!pOverworld_Manager->m_camera_mode && !editor_world_enabled) {
            pOverworld_Player->Action_Interact(INP_DOWN);
        }
        return 0;
    }
    else if (key == SDLK_c && !editor_world_enabled) {
        pOverworld_Manager->m_camera_mode = !pOverworld_Manager->m_camera_mode;
    }
    else if (key == SDLK_F8) {
        pWorld_Editor->Toggle();
    }
    else if (key == SDLK_d && pKeyboard->Is_Ctrl_Down()) {
        pOverworld_Manager->m_debug_mode = !pOverworld_Manager->m_debug_mode;
        game_debug = pOverworld_Manager->m_debug_mode;
    }
    else if (key == SDLK_l && pOverworld_Manager->m_debug_mode) {
        // toggle layer drawing
        pOverworld_Manager->m_draw_layer = !pOverworld_Manager->m_draw_layer;
    }
    else if (pKeyboard->m_keys[SDLK_g] && pKeyboard->m_keys[SDLK_o] && pKeyboard->m_keys[SDLK_d]) {
        // all waypoint access
        Set_Progress(m_waypoints.size(), 1);
    }
    else if (key == SDLK_F3 && pOverworld_Manager->m_debug_mode) {
        Goto_Next_Level();
    }
    // Exit
    else if (key == SDLK_ESCAPE || key == SDLK_BACKSPACE) {
        pOverworld_Player->Action_Interact(INP_EXIT);
    }
    // Action
    else if (key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_SPACE) {
        pOverworld_Player->Action_Interact(INP_ACTION);
    }
    // ## editor
    else if (pWorld_Editor->Key_Down(key)) {
        // processed by the editor
        return 1;
    }
    else {
        // not processed
        return 0;
    }

    // key got processed
    return 1;
}

bool cOverworld::Key_Up(SDLKey key)
{
    // nothing yet
    if (0) {
        //
    }
    else {
        // not processed
        return 0;
    }

    // key got processed
    return 1;
}

bool cOverworld::Mouse_Down(Uint8 button)
{
    // ## editor
    if (pWorld_Editor->Mouse_Down(button)) {
        // processed by the editor
        return 1;
    }
    else {
        // not processed
        return 0;
    }

    // button got processed
    return 1;
}

bool cOverworld::Mouse_Up(Uint8 button)
{
    // ## editor
    if (pWorld_Editor->Mouse_Up(button)) {
        // processed by the editor
        return 1;
    }
    else {
        // not processed
        return 0;
    }

    // button got processed
    return 1;
}

bool cOverworld::Joy_Button_Down(Uint8 button)
{
    // Exit
    if (button == pPreferences->m_joy_button_exit) {
        pOverworld_Player->Action_Interact(INP_EXIT);
    }
    // Action
    else if (button == pPreferences->m_joy_button_action) {
        pOverworld_Player->Action_Interact(INP_ACTION);
    }
    else {
        // not processed
        return 0;
    }

    // key got processed
    return 1;
}

bool cOverworld::Joy_Button_Up(Uint8 button)
{
    // nothing yet
    if (0) {
        //
    }
    else {
        // not processed
        return 0;
    }

    // key got processed
    return 1;
}

void cOverworld::Set_Progress(unsigned int normal_level, bool force /* = 1 */)
{
    unsigned int level_num = 0;

    for (WaypointList::iterator itr = m_waypoints.begin(); itr != m_waypoints.end(); ++itr) {
        cWaypoint* obj = (*itr);

        // accessible
        if (normal_level >= level_num) {
            obj->Set_Access(1);
        }
        // force unset
        else if (force) {
            obj->Set_Access(0);
        }

        level_num++;
    }
}

cWaypoint* cOverworld::Get_Waypoint(const std::string& name)
{
    for (WaypointList::iterator itr = m_waypoints.begin(); itr != m_waypoints.end(); ++itr) {
        cWaypoint* obj = (*itr);

        // accessible
        if (obj->m_destination.compare(name) == 0) {
            return obj;
        }
    }

    return NULL;
}

cWaypoint* cOverworld::Get_Waypoint(unsigned int num)
{
    if (num >= m_waypoints.size()) {
        // out of bounds
        return NULL;
    }

    // available
    return m_waypoints[num];
}

int cOverworld::Get_Level_Waypoint_Num(std::string level_name)
{
    // erase file type if set
    if (level_name.rfind(".txt") != std::string::npos || level_name.rfind(".smclvl") != std::string::npos || level_name.rfind(".tsclvl") != std::string::npos) {
        level_name.erase(level_name.rfind("."));
    }

    return Get_Waypoint_Num(level_name);
}

int cOverworld::Get_Waypoint_Num(const std::string& name)
{
    int count = 0;

    // search waypoint
    for (WaypointList::iterator itr = m_waypoints.begin(); itr != m_waypoints.end(); ++itr) {
        cWaypoint* obj = (*itr);

        if (obj->m_destination.compare(name) == 0) {
            // found
            return count;
        }

        count++;
    }

    // not found
    return -1;
}

int cOverworld::Get_Waypoint_Collision(const GL_rect& rect_2)
{
    int count = 0;

    for (WaypointList::iterator itr = m_waypoints.begin(); itr != m_waypoints.end(); ++itr) {
        cWaypoint* obj = (*itr);

        if (rect_2.Intersects(obj->m_rect)) {
            return count;
        }

        count++;
    }

    return -1;
}

int cOverworld::Get_Last_Valid_Waypoint(void)
{
    // no waypoints
    if (m_waypoints.empty()) {
        return -1;
    }

    for (int i = m_waypoints.size() - 1; i > 0; i--) {
        if (m_waypoints[i]->m_access) {
            return i;
        }
    }

    return -1;
}

void cOverworld::Update_Waypoint_text(void)
{
    // get waypoint
    cWaypoint* waypoint = m_waypoints[pOverworld_Player->m_current_waypoint];

    // set color
    Color color = static_cast<Uint8>(0);

    if (waypoint->m_waypoint_type == WAYPOINT_NORMAL) {
        color = lightblue;
    }
    else if (waypoint->m_waypoint_type == WAYPOINT_WORLD_LINK) {
        color = green;
    }

    m_hud_level_name->Set_Image(pFont->Render_Text(pFont->m_font_normal, waypoint->Get_Destination(), color), 1, 1);
}

bool cOverworld::Goto_Next_Level(void)
{
    // if not in overworld only go to the next level on overworld enter
    if (Game_Mode != MODE_OVERWORLD) {
        m_next_level = 1;
        return 0;
    }

    m_next_level = 0;

    cWaypoint* current_waypoint = pOverworld_Player->Get_Waypoint();

    // no Waypoint
    if (!current_waypoint) {
        return 0;
    }

    // Waypoint forward direction is invalid/unset
    if (current_waypoint->m_direction_forward == DIR_UNDEFINED) {
        return 0;
    }

    // Get Layer Line in front
    cLayer_Line_Point_Start* front_line = pOverworld_Player->Get_Front_Line(current_waypoint->m_direction_forward);

    if (!front_line) {
        return 0;
    }

    // Get forward Waypoint
    cWaypoint* next_waypoint = front_line->Get_End_Waypoint();

    // if no next waypoint available
    if (!next_waypoint) {
        return 0;
    }

    // if next waypoint is new
    if (!next_waypoint->m_access) {
        next_waypoint->Set_Access(1);

        // animation
        cParticle_Emitter* anim = new cParticle_Emitter(m_sprite_manager);
        anim->Set_Emitter_Rect(next_waypoint->m_rect.m_x + (next_waypoint->m_rect.m_w * 0.5f), next_waypoint->m_rect.m_y + (next_waypoint->m_rect.m_h * 0.5f), 1.0f, 1.0f);
        anim->Set_Emitter_Time_to_Live(1.5f);
        anim->Set_Emitter_Iteration_Interval(0.05f);
        anim->Set_Quota(1);
        anim->Set_Image(pVideo->Get_Package_Surface("animation/particles/light.png"));
        anim->Set_Pos_Z(0.081f);
        anim->Set_Time_to_Live(1.3f);
        anim->Set_Speed(1.0f, 0.5f);
        anim->Set_Scale(0.5f, 0.2f);
        anim->Set_Const_Rotation_Z(-6, 12);

        // World Waypoint
        if (next_waypoint->m_waypoint_type == WAYPOINT_WORLD_LINK) {
            anim->Set_Color(whitealpha128, Color(static_cast<Uint8>(0), 0, 0, 128));
        }
        else {
            anim->Set_Color(orange, Color(static_cast<Uint8>(6), 60, 20, 0));
        }

        // add animation
        m_animation_manager->Add(anim);
    }

    pOverworld_Player->Start_Walk(current_waypoint->m_direction_forward);

    return  1;
}

void cOverworld::Reset_Waypoints(void)
{
    for (WaypointList::iterator itr = m_waypoints.begin(); itr != m_waypoints.end(); ++itr) {
        cWaypoint* obj = (*itr);

        obj->Set_Access(obj->m_access_default);
    }
}

bool cOverworld::Is_Loaded(void) const
{
    // if not loaded version is -1
    if (m_engine_version >= 0) {
        return 1;
    }

    return 0;
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

cOverworld* pActive_Overworld = NULL;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

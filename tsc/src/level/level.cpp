/***************************************************************************
 * level.cpp  -  level handling class
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

#include "../level/level.hpp"
#include "../level/level_editor.hpp"
#include "level_loader.hpp"
#include "../core/game_core.hpp"
#include "../gui/menu.hpp"
#include "../user/preferences.hpp"
#include "../audio/audio.hpp"
#include "../level/level_player.hpp"
#include "../objects/goldpiece.hpp"
#include "../objects/level_exit.hpp"
#include "../video/font.hpp"
#include "../input/keyboard.hpp"
#include "../input/mouse.hpp"
#include "../input/joystick.hpp"
#include "../user/savegame/savegame.hpp"
#include "../overworld/world_manager.hpp"
#include "../overworld/overworld.hpp"
#include "../enemies/army.hpp"
#include "../enemies/bosses/turtle_boss.hpp"
#include "../enemies/rokko.hpp"
#include "../enemies/krush.hpp"
#include "../enemies/furball.hpp"
#include "../enemies/flyon.hpp"
#include "../enemies/thromp.hpp"
#include "../enemies/eato.hpp"
#include "../enemies/gee.hpp"
#include "../enemies/spika.hpp"
#include "../enemies/static.hpp"
#include "../enemies/spikeball.hpp"
#include "../objects/powerup.hpp"
#include "../objects/star.hpp"
#include "../objects/enemystopper.hpp"
#include "../objects/spinbox.hpp"
#include "../objects/bonusbox.hpp"
#include "../objects/text_box.hpp"
#include "../objects/moving_platform.hpp"
#include "../video/renderer.hpp"
#include "../core/math/utilities.hpp"
#include "../core/i18n.hpp"
#include "../objects/path.hpp"
#include "../core/filesystem/filesystem.hpp"
#include "../core/filesystem/resource_manager.hpp"
#include "../core/filesystem/package_manager.hpp"
#include "../core/filesystem/boost_relative.hpp"
#include "../overworld/world_editor.hpp"
#include "../scripting/events/key_down_event.hpp"
#include "../core/global_basic.hpp"

namespace fs = boost::filesystem;

using namespace std;

/* Function call order on level (un)loading
 * ========================================
 *
 * The following table gives a brief overview over which functions are
 * called when creating a level and which ones when loading or destroying
 * one.
 *
 * New level       | Loaded level
 * ================+================
 * operator new    | operator new
 * New()           | Load()
 * Init()          | Init()
 * - - - - - - - - + - - - - - - - -
 * Unload()        | Unload()
 * operator delete | operator delete
 *
 * The loading functions are called from the main event loop at core/game_core.cpp
 * in Handle_Generic_Game_Events() (events "new_level" and "load_level"). Note
 * that loading of the XML elements happens in Load(), so on adding features
 * to the level startup that happen for both new and loaded levels you want
 * to hook into Init(), otherwise choose the apropriate methods.
 */

namespace TSC {

/* *** *** *** *** *** cLevel *** *** *** *** *** *** *** *** *** *** *** *** */

cLevel::cLevel(void)
{
    // settings
    Reset_Settings();

    m_delayed_unload = 0;

#ifdef ENABLE_MRUBY
    m_mruby = NULL; // Initialized in Init()
    m_mruby_has_been_initialized = false;
#endif

    m_sprite_manager = new cSprite_Manager();
    m_background_manager = new cBackground_Manager();
    m_animation_manager = new cAnimation_Manager();

    // add default gradient layer
    cBackground* gradient_background = new cBackground(m_sprite_manager);
    gradient_background->Set_Type(BG_GR_VER);
    gradient_background->m_pos_z = 0.0001f;
    m_background_manager->Add(gradient_background);
}

cLevel::~cLevel(void)
{
    Unload();

    // delete
    delete m_background_manager;
    delete m_animation_manager;
    delete m_sprite_manager;
}

bool cLevel::New(std::string levelname)
{
    Unload();

    string_trim(levelname, ' ');
    fs::path filename = utf8_to_path(levelname);

    fs::ifstream ifs;

    // if no name is given create name
    if (filename.empty()) {
        unsigned int i = 1;

        // search for a not existing file
        while (1) {
            // set name
            filename = pPackage_Manager->Get_User_Level_Path() / utf8_to_path("new_" + int_to_string(i));
            // try to open the file
            ifs.open(filename, ios::in);

            // found unused name
            if (!ifs.is_open()) {
                break;
            }

            // stop on 99
            if (i > 99) {
                return 0;
            }

            ifs.close();
            i++;
        }
    }
    else {
        // set file type
        if (filename.extension() != fs::path(".tsclvl"))
            filename.replace_extension(".tsclvl");

        // set user directory, creating an absolute path if necessary
        filename = fs::absolute(filename, pPackage_Manager->Get_User_Level_Path());
    }

    // open file
    ifs.open(filename, ios::in);

    // level doesn't exist
    if (!ifs) {
        debug_print("Created new level: %s\n", path_to_utf8(filename).c_str());
        // set filename
        m_level_filename = filename;
        m_engine_version = level_engine_version;

        return 1;
    }

    // level already exists
    ifs.close();

    return 0;
}

cLevel* cLevel::Load_From_File(fs::path filename)
{
    if (filename.empty())
        throw(InvalidLevelError("Empty level filename!"));
    if (!File_Exists(filename)) {
        std::string msg = "Level file not found: " + path_to_utf8(filename);
        throw (InvalidLevelError(msg));
    }

    // This is our loader
    cLevelLoader loader;

    // supported level format
    if (filename.extension() == fs::path(".tsclvl")  || filename.extension() == fs::path(".smclvl")) {
        loader.parse_file(filename);
    }
    else { // old, unsupported level format
        pHud_Debug->Set_Text(_("Unsupported Level format : ") + (const std::string)path_to_utf8(filename));
        return NULL;
    }

    // Our level
    cLevel* p_level = loader.Get_Level();

    // FIXME: Move this into cLevelLoader::on_end_document()
    /* late initialization
     * needed to create links to other objects
    */
    for (cSprite_List::iterator itr = p_level->m_sprite_manager->objects.begin(); itr != p_level->m_sprite_manager->objects.end(); ++itr) {
        cSprite* obj = (*itr);

        obj->Init_Links();
    }

    debug_print("Loaded level: %s\n", path_to_utf8(p_level->m_level_filename).c_str());

    return p_level;
}

void cLevel::Unload(bool delayed /* = 0 */)
{
    if (delayed) {
        m_delayed_unload = 1;
        return;
    }
    else {
        m_delayed_unload = 0;
    }

    // not loaded
    if (!Is_Loaded()) {
        return;
    }

    // delete backgrounds
    m_background_manager->Delete_All();

    // add default gradient layer
    cBackground* gradient_background = new cBackground(m_sprite_manager);
    gradient_background->Set_Type(BG_GR_VER);
    gradient_background->m_pos_z = 0.0001f;
    m_background_manager->Add(gradient_background);

    // reset music
    m_musicfile.clear();
    m_valid_music = 0;

    m_author.clear();
    m_version.clear();

    // no version
    m_engine_version = -1;

    debug_print("Unloaded level: %s\n", path_to_utf8(m_level_filename).c_str());
    m_level_filename.clear();

    Reset_Settings();

#ifdef ENABLE_MRUBY
    /* Shutdown the mruby interpreter. The menu level (the one shown on the
     * startup screen) has not been Init()ialized and hence has no mruby
     * interpreter attached. Therefore we need to check the existance
     * of the mruby interpreter here. */
    if (m_mruby)
        delete m_mruby;
#endif

    /* delete sprites
     * do this at last
    */
    m_sprite_manager->Delete_All();
}

fs::path cLevel::Save_To_File(fs::path filename /* = fs::path() */)
{
    xmlpp::Document doc;
    xmlpp::Element* p_root = doc.create_root_node("level");
    xmlpp::Element* p_node = NULL;

    // <information>
    p_node = p_root->add_child("information");
    Add_Property(p_node, "game_version", int_to_string(TSC_VERSION_MAJOR) + "." + int_to_string(TSC_VERSION_MINOR) + "." + int_to_string(TSC_VERSION_PATCH));
    Add_Property(p_node, "engine_version", level_engine_version);
    Add_Property(p_node, "save_time", static_cast<Uint64>(time(NULL)));
    // </information>

    // <settings>
    p_node = p_root->add_child("settings");
    Add_Property(p_node, "lvl_author", m_author);
    Add_Property(p_node, "lvl_version", m_version);
    Add_Property(p_node, "lvl_music", Get_Music_Filename().generic_string());
    Add_Property(p_node, "lvl_description", m_description);
    Add_Property(p_node, "lvl_difficulty", static_cast<int>(m_difficulty));
    Add_Property(p_node, "lvl_land_type", Get_Level_Land_Type_Name(m_land_type));
    Add_Property(p_node, "cam_limit_x", static_cast<int>(m_camera_limits.m_x));
    Add_Property(p_node, "cam_limit_y", static_cast<int>(m_camera_limits.m_y));
    Add_Property(p_node, "cam_limit_w", static_cast<int>(m_camera_limits.m_w));
    Add_Property(p_node, "cam_limit_h", static_cast<int>(m_camera_limits.m_h));
    Add_Property(p_node, "cam_fixed_hor_vel", m_fixed_camera_hor_vel);
    Add_Property(p_node, "unload_after_exit", m_unload_after_exit ? 1 : 0);
    // </settings>

    // backgrounds
    vector<cBackground*>::iterator iter;
    for (iter=m_background_manager->objects.begin(); iter != m_background_manager->objects.end(); iter++)
        (*iter)->Save_To_XML_Node(p_root);

    // <player>
    p_node = p_root->add_child("player");
    Add_Property(p_node, "posx", static_cast<int>(pLevel_Player->m_start_pos_x));
    Add_Property(p_node, "posy", static_cast<int>(pLevel_Player->m_start_pos_y));
    Add_Property(p_node, "direction", Get_Direction_Name(pLevel_Player->m_start_direction));
    // </player>

    cSprite_List::iterator iter2;
    for (iter2=m_sprite_manager->objects.begin(); iter2 != m_sprite_manager->objects.end(); iter2++) {
        cSprite* p_obj = *iter2;

        // skip spawned and destroyed objects
        if (p_obj->m_spawned || p_obj->m_auto_destroy)
            continue;

        // save to XML node
        p_obj->Save_To_XML_Node(p_root);
    }

    // MRuby script code
    // <script>
    p_node = p_root->add_child("script");
    p_node->add_child_text(m_script);
    // </script>

    // Write to file (raises xmlpp::exception on write error)
    doc.write_to_file_formatted(Glib::filename_from_utf8(path_to_utf8(filename)));
    debug_print("Wrote level file '%s'.\n", path_to_utf8(filename).c_str());

    return filename;
}

// TODO: Merge Save() with Save_To_File() after ENABLE_NEW_LOADER
// is the only variant?
void cLevel::Save(void)
{
    pAudio->Play_Sound("editor/save.ogg");

    // use user level dir
    if (path_to_utf8(m_level_filename).find(path_to_utf8(pPackage_Manager->Get_User_Level_Path())) == std::string::npos) {
        // erase old directory
        m_level_filename = Trim_Filename(m_level_filename, 0, 1);
        // set user directory
        m_level_filename = fs::absolute(m_level_filename, pPackage_Manager->Get_User_Level_Path());
    }

    //Force all levels to save with the .tsclvl extension
    fs::path tsc_level_filename = m_level_filename;
    tsc_level_filename.replace_extension(".tsclvl");

    try {
        Save_To_File(tsc_level_filename);
    }
    catch (xmlpp::exception& e) {
        cerr << "Error: Couldn't save level file: " << e.what() << endl;
        cerr << "Is the file read-only?" << endl;
        pHud_Debug->Set_Text(_("Couldn't save level ") + path_to_utf8(m_level_filename), speedfactor_fps * 5.0f);

        // Abort
        return;
    }

    //If the file originally had .smclvl for the extension and if the .tsclvl save was successful, remove the old
    //.smclvl file.
    if (m_level_filename.extension().string() == ".smclvl") {
        if (fs::exists(m_level_filename) && fs::exists(tsc_level_filename)) {
            fs::remove(m_level_filename);
        }
        m_level_filename.replace_extension(".tsclvl");
    }

    // Display nice completion message
    pHud_Debug->Set_Text(_("Level ") + path_to_utf8(Trim_Filename(m_level_filename, false, false)) + _(" saved"));
}

void cLevel::Delete(void)
{
    fs::remove(m_level_filename);
    Unload();
}

void cLevel::Reset_Settings(void)
{
    // no engine version
    m_engine_version = -1;
    m_last_saved = 0;
    m_author.clear();
    m_version.clear();

    // set default music
    Set_Music(pPackage_Manager->Get_Music_Reading_Path(LEVEL_DEFAULT_MUSIC));

    m_description.clear();
    m_difficulty = 0;
    m_land_type = LLT_UNDEFINED;

    // player
    m_player_start_pos_x = cLevel_Player::m_default_pos_x;
    m_player_start_pos_y = cLevel_Player::m_default_pos_y;
    m_player_start_direction = DIR_RIGHT;

    // camera
    m_camera_limits = cCamera::m_default_limits;
    m_fixed_camera_hor_vel = 0.0f;

    // unload after exit
    m_unload_after_exit = false;

    // MRuby script code
    m_script = std::string();
}

void cLevel::Init(void)
{
    // if not loaded
    if (!Is_Loaded()) {
        return;
    }

    // player position
    pLevel_Player->Set_Pos(m_player_start_pos_x, m_player_start_pos_y, 1);
    // player direction
    pLevel_Player->Set_Direction(m_player_start_direction, 1);
    // player reset
    pLevel_Player->Reset();

    // pre-update animations
    for (cSprite_List::iterator itr = m_sprite_manager->objects.begin(); itr != m_sprite_manager->objects.end(); ++itr) {
        cSprite* obj = (*itr);

        if (obj->m_type == TYPE_PARTICLE_EMITTER) {
            cParticle_Emitter* emitter = static_cast<cParticle_Emitter*>(obj);
            emitter->Pre_Update();
        }
    }

#ifdef ENABLE_MRUBY
    /* For unknown reasons, Init() is public. And for even more unknown reasons,
     * it is called from the outside at some totally unfitting places such as
     * when returning from a sublevel. We do NOT want to reload all the scripting
     * stuff in such a case, but rather continue where we left off.
     * This is a HACK. It should be removed when Init() is made private as it
     * should be with regard to the secrecy principle of OOP. */
    if (!m_mruby_has_been_initialized) {
        Reinitialize_MRuby_Interpreter();
        m_mruby_has_been_initialized = true;
    }
#endif
}

std::string cLevel::Get_Level_Name()
{
    return path_to_utf8(Trim_Filename(m_level_filename, false, false));
}

void cLevel::Set_Sprite_Manager(void)
{
    pHud_Manager->Set_Sprite_Manager(m_sprite_manager);
    pMouseCursor->Set_Sprite_Manager(m_sprite_manager);
    pLevel_Editor->Set_Sprite_Manager(m_sprite_manager);
    pLevel_Editor->Set_Level(this);
    // camera
    pLevel_Manager->m_camera->Set_Sprite_Manager(m_sprite_manager);
    pLevel_Manager->m_camera->Set_Limits(m_camera_limits);
    pLevel_Manager->m_camera->m_fixed_hor_vel = m_fixed_camera_hor_vel;

    pLevel_Player->Set_Sprite_Manager(m_sprite_manager);
}

void cLevel::Enter(const GameMode old_mode /* = MODE_NOTHING */)
{
    // if not loaded
    if (!Is_Loaded()) {
        return;
    }

    Set_Sprite_Manager();
    // set active camera
    pActive_Camera = pLevel_Manager->m_camera;
    // set active player
    pActive_Player = pLevel_Player;
    // set animation manager
    pActive_Animation_Manager = m_animation_manager;

    // disable world editor
    pWorld_Editor->Disable();

    // set editor enabled state
    // FIXME: Duplicates the information in pLevel_Editor->m_enabled (set in cLevel::Enter())
    editor_enabled = pLevel_Editor->m_enabled;

    if (pLevel_Editor->m_enabled) {
        if (!pLevel_Editor->m_editor_window->isVisible()) {
            pLevel_Editor->m_editor_window->show();
            pMouseCursor->Set_Active(1);
        }
    }

    // camera
    if (pLevel_Editor->m_enabled) {
        pActive_Camera->Update_Position();
    }
    else {
        pLevel_Manager->m_camera->Center();
    }

    // play music
    if (m_valid_music) {
        if (pAudio->m_music_filename.compare(m_musicfile) != 0) {
            pAudio->Play_Music(m_musicfile, true, 0, 1000);
        }
    }
    else if (pAudio->m_music_enabled) {
        cerr << "Warning : Music file not found: " << path_to_utf8(pActive_Level->m_musicfile) << endl;
    }

    // Update Hud Text and position
    pHud_Manager->Update_Text();

    // reset speed factor
    pFramerate->Reset();
}

void cLevel::Leave(const GameMode next_mode /* = MODE_NOTHING */)
{
    // if not in level mode
    if (Game_Mode != MODE_LEVEL) {
        return;
    }

    // reset camera limits
    pLevel_Manager->m_camera->Reset_Limits();
    pLevel_Manager->m_camera->m_fixed_hor_vel = 0.0f;

    // reset mouse
    pMouseCursor->Reset(0);

    // level to level
    if (next_mode == MODE_LEVEL) {
        // unload this level if it is marked
        if (m_unload_after_exit)
            Unload();
        return;
    }
    // if new mode: it should play different music
    else if (next_mode != MODE_MENU && next_mode != MODE_LEVEL_SETTINGS) {
        // fade out music
        pAudio->Fadeout_Music(1000);
    }

    pJoystick->Reset_keys();

    // hide editor window if visible
    if (pLevel_Editor->m_enabled) {
        if (pLevel_Editor->m_editor_window->isVisible()) {
            pLevel_Editor->m_editor_window->hide();
        }
    }
}

void cLevel::Update(void)
{
    if (m_delayed_unload) {
        Unload();
        return;
    }

    if (!m_next_level_filename.empty()) {
        // TODO: This should NOT be in cLevel::Update()!
        // Level changing belongs to the game loop or so!
        throw (NotImplementedError("Cannot load next level yet!"));
        //Load( path_to_utf8(m_next_level_filename) );
    }

    // if level-editor is not active
    if (!editor_level_enabled) {
        // backgrounds
        for (vector<cBackground*>::iterator itr = m_background_manager->objects.begin(); itr != m_background_manager->objects.end(); ++itr) {
            (*itr)->Update();
        }

        // objects
        m_sprite_manager->Update_Items();
        // animations
        m_animation_manager->Update();

#ifdef ENABLE_MRUBY
        // Scripted timers (if an MRuby interpreter is there)
        if (m_mruby)
            m_mruby->Evaluate_Timer_Callbacks();
#endif
    }
    // if level-editor enabled
    else {
        // only update particle emitters
        for (cSprite_List::iterator itr = m_sprite_manager->objects.begin(); itr != m_sprite_manager->objects.end(); ++itr) {
            cSprite* obj = (*itr);

            if (obj->m_type == TYPE_PARTICLE_EMITTER) {
                obj->Update();
            }
        }
    }
}

void cLevel::Update_Late(void)
{
    // if leveleditor is not active
    if (!editor_level_enabled) {
        // objects
        m_sprite_manager->Update_Items_Late();
    }
}

void cLevel::Draw_Layer_1(LevelDrawType type /* = LVL_DRAW */)
{
    // with background
    if (type != LVL_DRAW_NO_BG) {
        for (vector<cBackground*>::iterator itr = m_background_manager->objects.begin(); itr != m_background_manager->objects.end(); ++itr) {
            (*itr)->Draw();
        }
    }

    // only background
    if (type == LVL_DRAW_BG) {
        return;
    }

    // Objects
    m_sprite_manager->Draw_Items();
    // Animations
    m_animation_manager->Draw();
}

void cLevel::Draw_Layer_2(LevelDrawType type /* = LVL_DRAW */)
{
    // only background
    if (type == LVL_DRAW_BG) {
        return;
    }

    // ghost
    if (pLevel_Player->m_alex_type == ALEX_GHOST) {
        // create request
        cRect_Request* request = new cRect_Request();

        Color color = Color(0.5f, 0.5f, 0.5f, 0.3f);

        // fade alpha in
        if (pLevel_Player->m_ghost_time > 220) {
            color.alpha = static_cast<Uint8>(color.alpha * ((-pLevel_Player->m_ghost_time + 320) * 0.01f));
        }
        // fade alpha out
        else if (pLevel_Player->m_ghost_time < 100) {
            color.alpha = static_cast<Uint8>(color.alpha * (pLevel_Player->m_ghost_time * 0.01f));
        }

        pVideo->Draw_Rect(0, 0, static_cast<float>(game_res_w), static_cast<float>(game_res_h), 0.12f, &color, request);

        request->m_combine_type = GL_MODULATE;

        request->m_combine_color[0] = 0.9f;
        request->m_combine_color[1] = 0.6f;
        request->m_combine_color[2] = 0.8f;

        // add request
        pRenderer->Add(request);
    }
}

void cLevel::Process_Input(void)
{
    // only non editor
    if (!editor_level_enabled) {
        // none
    }
}

bool cLevel::Key_Down(const SDLKey key)
{
    // debug key F2
    if (key == SDLK_F2 && game_debug && !editor_level_enabled) {
        pLevel_Player->Set_Type(ALEX_CAPE, 0);
    }
    // special key F3
    else if (key == SDLK_F3 && !editor_level_enabled) {
        //pLevel_Player->GotoNextLevel();
        //DrawEffect( HORIZONTAL_VERTICAL_FADE );
        //pLevel_Player->Draw_Animation( ALEX_FIRE );

        cParticle_Emitter* anim = new cParticle_Emitter(m_sprite_manager);
        anim->Set_Emitter_Rect(pLevel_Player->m_pos_x + static_cast<float>(pLevel_Player->m_col_rect.m_w / 2), pLevel_Player->m_pos_y - 100, 10, 10);
        anim->Set_Emitter_Time_to_Live(-1);
        anim->Set_Emitter_Iteration_Interval(5);
        anim->Set_Quota(200);
        anim->Set_Image(pVideo->Get_Package_Surface("animation/particles/star.png"));
        anim->Set_Time_to_Live(3, 3);
        anim->Set_Fading_Alpha(1);
        anim->Set_Speed(1, 4);
        anim->Set_Scale(0.5f);
        anim->Set_Const_Rotation_Z(-5, 10);
        m_animation_manager->Add(anim);
    }
    // special key F4
    else if (key == SDLK_F4) {
        Draw_Effect_Out(EFFECT_OUT_FIXED_COLORBOX);
        Draw_Effect_In();
    }
    // Toggle leveleditor
    else if (key == SDLK_F8) {
        pLevel_Editor->Toggle();
    }
    // ## Game
    // Shoot
    else if (key == pPreferences->m_key_shoot && !editor_enabled) {
        Scripting::cKeyDown_Event evt("shoot");
        evt.Fire(m_mruby, pKeyboard);
        pLevel_Player->Action_Shoot();
    }
    // Jump
    else if (key == pPreferences->m_key_jump && !editor_enabled) {
        Scripting::cKeyDown_Event evt("jump");
        evt.Fire(m_mruby, pKeyboard);
        pLevel_Player->Action_Jump();
    }
    // Action
    else if (key == pPreferences->m_key_action && !editor_enabled) {
        Scripting::cKeyDown_Event evt("action");
        evt.Fire(m_mruby, pKeyboard);
        pLevel_Player->Action_Interact(INP_ACTION);
    }
    // Up
    else if (key == pPreferences->m_key_up && !editor_enabled) {
        Scripting::cKeyDown_Event evt("up");
        evt.Fire(m_mruby, pKeyboard);
        pLevel_Player->Action_Interact(INP_UP);
    }
    // Down
    else if (key == pPreferences->m_key_down && !editor_enabled) {
        Scripting::cKeyDown_Event evt("down");
        evt.Fire(m_mruby, pKeyboard);
        pLevel_Player->Action_Interact(INP_DOWN);
    }
    // Left
    else if (key == pPreferences->m_key_left && !editor_enabled) {
        Scripting::cKeyDown_Event evt("left");
        evt.Fire(m_mruby, pKeyboard);
        pLevel_Player->Action_Interact(INP_LEFT);
    }
    // Right
    else if (key == pPreferences->m_key_right && !editor_enabled) {
        Scripting::cKeyDown_Event evt("right");
        evt.Fire(m_mruby, pKeyboard);
        pLevel_Player->Action_Interact(INP_RIGHT);
    }
    // Request Item
    else if (key == pPreferences->m_key_item && !editor_enabled) {
        Scripting::cKeyDown_Event evt("item");
        evt.Fire(m_mruby, pKeyboard);
        pLevel_Player->Action_Interact(INP_ITEM);
    }
    // God Mode
    else if (pKeyboard->m_keys[SDLK_g] && pKeyboard->m_keys[SDLK_o] && pKeyboard->m_keys[SDLK_d] && !editor_enabled) {
        if (pLevel_Player->m_god_mode) {
            pHud_Debug->Set_Text("Funky God Mode disabled");
        }
        else {
            pHud_Debug->Set_Text("Funky God Mode enabled");
        }

        pLevel_Player->m_god_mode = !pLevel_Player->m_god_mode;
    }
    // Set Small state
    else if (pKeyboard->m_keys[SDLK_k] && pKeyboard->m_keys[SDLK_i] && pKeyboard->m_keys[SDLK_d] && !editor_enabled) {
        pLevel_Player->Set_Type(ALEX_SMALL, 0);
    }
    // Exit
    else if (key == SDLK_ESCAPE) {
        pLevel_Player->Action_Interact(INP_EXIT);
    }
    // ## editor
    else if (pLevel_Editor->Key_Down(key)) {
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

bool cLevel::Key_Up(const SDLKey key)
{
    // only if not in Editor
    if (editor_level_enabled) {
        return 0;
    }

    // Interaction keys
    if (key == pPreferences->m_key_right) {
        pLevel_Player->Action_Stop_Interact(INP_RIGHT);
    }
    else if (key == pPreferences->m_key_left) {
        pLevel_Player->Action_Stop_Interact(INP_LEFT);
    }
    else if (key == pPreferences->m_key_down) {
        pLevel_Player->Action_Stop_Interact(INP_DOWN);
    }
    else if (key == pPreferences->m_key_jump) {
        pLevel_Player->Action_Stop_Interact(INP_JUMP);
    }
    else if (key == pPreferences->m_key_shoot) {
        pLevel_Player->Action_Stop_Interact(INP_SHOOT);
    }
    else if (key == pPreferences->m_key_action) {
        pLevel_Player->Action_Stop_Interact(INP_ACTION);
    }
    else {
        // not processed
        return 0;
    }

    // key got processed
    return 1;
}

bool cLevel::Mouse_Down(Uint8 button)
{
    // ## editor
    if (pLevel_Editor->Mouse_Down(button)) {
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

bool cLevel::Mouse_Up(Uint8 button)
{
    // ## editor
    if (pLevel_Editor->Mouse_Up(button)) {
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

bool cLevel::Joy_Button_Down(Uint8 button)
{
    // Shoot
    if (button == pPreferences->m_joy_button_shoot && !editor_enabled) {
        pLevel_Player->Action_Interact(INP_SHOOT);
    }
    // Jump
    else if (button == pPreferences->m_joy_button_jump && !editor_enabled) {
        pLevel_Player->Action_Interact(INP_JUMP);
    }
    // Interaction keys
    else if (button == pPreferences->m_joy_button_action && !editor_enabled) {
        pLevel_Player->Action_Interact(INP_ACTION);
    }
    // Request Itembox Item
    else if (button == pPreferences->m_joy_button_item && !editor_enabled) {
        pLevel_Player->Action_Interact(INP_ITEM);
    }
    // Enter menu
    else if (button == pPreferences->m_joy_button_exit) {
        pLevel_Player->Action_Interact(INP_EXIT);
    }
    else {
        // not processed
        return 0;
    }

    // key got processed
    return 1;
}

bool cLevel::Joy_Button_Up(Uint8 button)
{
    // only if not in Editor
    if (editor_level_enabled) {
        return 0;
    }

    if (button == pPreferences->m_joy_button_jump) {
        pLevel_Player->Action_Stop_Interact(INP_JUMP);
    }
    else if (button == pPreferences->m_joy_button_shoot) {
        pLevel_Player->Action_Stop_Interact(INP_SHOOT);
    }
    else if (button == pPreferences->m_joy_button_action) {
        pLevel_Player->Action_Stop_Interact(INP_ACTION);
    }
    else {
        // not processed
        return 0;
    }

    // key got processed
    return 1;
}

fs::path cLevel::Get_Music_Filename() const
{
    return pPackage_Manager->Get_Relative_Music_Path(m_musicfile);
}

void cLevel::Set_Music(fs::path filename)
{
    // add music dir
    if (!filename.is_absolute())
        filename = pPackage_Manager->Get_Music_Reading_Path(path_to_utf8(filename));

    // already set
    if (m_musicfile.compare(filename) == 0) {
        return;
    }

    m_musicfile = filename;
    // check if music is available
    m_valid_music = File_Exists(filename);
}

void cLevel::Set_Filename(fs::path filename, bool rename_old /* = true */)
{
    // erase file type and directory
    filename = Trim_Filename(filename, 0, 0);

    // if invalid
    if (path_to_utf8(filename).length() < 2) {
        return;
    }

    // add level file type
    if (filename.extension() != fs::path(".tsclvl"))
        filename.replace_extension(".tsclvl");

    // add level dir if we aren’t absolute yet
    filename = fs::absolute(filename, pPackage_Manager->Get_User_Level_Path());

    // rename file
    if (rename_old) {
        fs::rename(m_level_filename, filename);
    }

    m_level_filename = filename;
}

void cLevel::Set_Author(const std::string& name)
{
    m_author = name;
}

void cLevel::Set_Version(const std::string& level_version)
{
    m_version = level_version;
}

void cLevel::Set_Description(const std::string& level_description)
{
    m_description = level_description;
}

void cLevel::Set_Difficulty(const Uint8 level_difficulty)
{
    m_difficulty = level_difficulty;

    // if invalid
    if (m_difficulty > 100) {
        m_difficulty = 0;
    }
}

void cLevel::Set_Land_Type(const LevelLandType level_land_type)
{
    m_land_type = level_land_type;
}

cLevel_Entry* cLevel::Get_Entry(const std::string& name)
{
    if (name.empty()) {
        return NULL;
    }

    std::vector<cLevel_Entry*> entries;

    // Search for entries matching name
    for (cSprite_List::iterator itr = m_sprite_manager->objects.begin(); itr != m_sprite_manager->objects.end(); ++itr) {
        cSprite* obj = (*itr);

        if (obj->m_type != TYPE_LEVEL_ENTRY || obj->m_auto_destroy) {
            continue;
        }

        cLevel_Entry* level_entry = static_cast<cLevel_Entry*>(obj);

        // found
        if (level_entry->m_entry_name.compare(name) == 0) {
            entries.push_back(level_entry);
        }
    }

    // Return a random entry
    if (!entries.empty()) {
        return entries[rand() % entries.size()];
    }

    return NULL;
}

bool cLevel::Is_Loaded(void) const
{
    // if not loaded version is -1
    if (m_engine_version >= 0) {
        return 1;
    }

    return 0;
}

#ifdef ENABLE_MRUBY
/**
 * This method wipes out the entire current mruby state (just
 * as if the level is finished), and sets up an entirely new
 * state, that also runs the script in `m_script` anew.
 * This method is intended for:
 *
 * 1. Initial level start, where no interpreter exists yet.
 * 2. The level settings to set the script code in the editor.
 *
 * It should NEVER be called outside one of these contexts.
 */
void cLevel::Reinitialize_MRuby_Interpreter()
{
    debug_print("Reinitializing mruby interpreter.\n");

    // Delete any currently existing incarnation of an mruby
    // stack and completely annihilate it.
    if (m_mruby)
        delete m_mruby;

    // Initialize an mruby interpreter for this level. Each level has its own mruby
    // interpreter to prevent unintended object exchange between levels.
    m_mruby = new Scripting::cMRuby_Interpreter(this);

    // Run the mruby code associated with this level (this sets up
    // all the event handlers the user wants to register)
    m_mruby->Run_Code(m_script, "(level script)");
}
#endif

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

/// Currently running level (NULL if no level is active).
cLevel* pActive_Level = NULL;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

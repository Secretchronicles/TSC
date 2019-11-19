/***************************************************************************
 * menu.cpp  -  menu handler
 *
 * Copyright © 2003 - 2011 Florian Richter
 * Copyright © 2012-2017 The TSC Contributors
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
#include "../gui/menu.hpp"
#include "../gui/menu_data.hpp"
#include "../core/game_core.hpp"
#include "../core/framerate.hpp"
#include "../input/mouse.hpp"
#include "../audio/audio.hpp"
#include "../level/level_player.hpp"
#include "../video/gl_surface.hpp"
#include "../level/level.hpp"
#include "../overworld/overworld.hpp"
#include "../user/preferences.hpp"
#include "../input/keyboard.hpp"
#include "../core/filesystem/resource_manager.hpp"
#include "../core/global_basic.hpp"

// Music files to play on the title and credits screens.
#define GAME_TITLE_MUSIC "land/hyper_1.ogg"
#define GAME_CREDITS_MUSIC "land/hyper_1.ogg"

using namespace std;
namespace fs = boost::filesystem;

namespace TSC {

/* *** *** *** *** *** *** cMenuHandler *** *** *** *** *** *** *** *** *** *** *** */

cMenuHandler::cMenuHandler(void)
{
    fs::path lvl_path = menu_level_path();
    if (fs::exists(lvl_path)) {
        m_level = cLevel::Load_From_File(lvl_path);
    }
    else {
        // at least give it something so it doesn't crash
        m_level = new cLevel();
    }

    m_camera = new cCamera(m_level->m_sprite_manager);
    m_player = new cSprite(m_level->m_sprite_manager);
    m_player->Set_Massive_Type(MASS_PASSIVE);
    Reset();

    // TSC logo image
    mp_tsc_logo = new cHudSprite(m_level->m_sprite_manager);
    mp_tsc_logo->Set_Image(pVideo->Get_Surface("game/logo/logo.png"));
    //mp_tsc_logo->Set_Pos(180.0f, 20.0f);
    mp_tsc_logo->Set_Pos(295.0f, 20.0f);
    mp_tsc_logo->Set_Scale(0.8f);
    mp_tsc_logo->Set_Massive_Type(MASS_FRONT_PASSIVE);
    m_level->m_sprite_manager->Add(mp_tsc_logo); // Lets the sprite manager manage the memory
}

cMenuHandler::~cMenuHandler(void)
{
    Reset();

    delete m_camera;
    delete m_level;
    delete m_player;
    mp_tsc_logo = NULL;
}

/**
 * Adds a new text item to the menu. Don't forget to set the menu
 * item you want to set active with Set_Active() when you're done
 * with adding menu items.
 *
 * \param rect
 * Screen area the item occupies. This is used for testing whether the mouse
 * is over the item currently (see Update_Mouse()).
 *
 * \param[in] p_item
 * Object to associate with this menu item. This is what is returned
 * by Get_Active_Item() if the item is active. Set to NULL if you
 * don't need this.
 *
 * \returns
 * The 0-based index number of this menu item in the menu.
 */
int cMenuHandler::Add_Menu_Item(sf::FloatRect rect, void* p_item)
{
    m_items.resize(m_items.size() + 1);
    m_items.back().m_rect = rect;
    m_items.back().mp_item = p_item;

    return m_items.size() - 1;
}

void cMenuHandler::Reset(void)
{
    m_items.clear();

   // nothing is active
    m_active = -1;
    if (pMenuCore) // is NULL if called from pMenuCore's constructor
        if (pMenuCore->m_menu_data) // NULL before first set
            pMenuCore->m_menu_data->Selected_Item_Changed(m_active);
}

void cMenuHandler::Set_Active(int num)
{
    // if not already active and exists
    if (num == m_active || num < 0 || static_cast<unsigned int>(num) >= m_items.size() ) {
        return;
    }

    m_active = num;
    pMenuCore->m_menu_data->Selected_Item_Changed(m_active);
}

void cMenuHandler::Update_Mouse(void)
{
    int found = -1;

    // check
    for (unsigned int i = 0; i < m_items.size(); i++) {
        if (m_items[i].m_rect.contains(static_cast<float>(pMouseCursor->m_x), static_cast<float>(pMouseCursor->m_y))) {
            found = i;
            break;
        }
    }

    // ignore mouse init
    if (found < 0 && input_event.mouseMove.x == pMouseCursor->m_x) {
        return;
    }

    Set_Active(found);
}

void cMenuHandler::Update(void)
{
    // level
    m_level->Update();
    // collision and movement handling
    m_level->m_sprite_manager->Handle_Collision_Items();
}

void cMenuHandler::Draw(bool with_background /* = 1 */)
{
    if (with_background) {
        // draw menu level
        m_level->Draw_Layer_1();
    }
}

/**
 * Returns the pointer associated with the currently active menu item,
 * or, if no item is active, NULL.
 */
void* cMenuHandler::Get_Active_Item(void)
{
    if (m_active < 0 || static_cast<unsigned int>(m_active) >= m_items.size()) {
        return NULL;
    }

    return m_items[m_active].mp_item;
}

/**
 * Returns the rectangle of the active item.
 */
sf::FloatRect cMenuHandler::Get_Active_Item_Rect(void)
{
    if (m_active < 0 || static_cast<unsigned int>(m_active) >= m_items.size()) {
        return sf::FloatRect();
    }

    return m_items[m_active].m_rect;
}

unsigned int cMenuHandler::Get_Size(void) const
{
    return static_cast<unsigned int>(m_items.size());
}

fs::path cMenuHandler::menu_level_path()
{
    // determine level for the menu
    fs::path result;
    std::string level;

    // User specified menu level
    level = pPreferences->m_menu_level;
    if (!level.empty()) {
        level = level + ".tsclvl";

        result = pResource_Manager->Get_User_Level(level);
        if (fs::exists(result))
            return result;

        result = pResource_Manager->Get_Game_Level(level);
        if (fs::exists(result))
            return result;
    }

    // Default menu level
    level = pPreferences->m_menu_level_default + ".tsclvl";

    result = pResource_Manager->Get_User_Level(level);
    if (fs::exists(result))
        return result;

    return pResource_Manager->Get_Game_Level(level);
}

/* *** *** *** *** *** *** *** cMenuCore *** *** *** *** *** *** *** *** *** *** */

cMenuCore::cMenuCore(void)
{
    m_menu_id = MENU_NOTHING;

    m_menu_data = NULL;
    m_handler = new cMenuHandler();

    // particle animation
    m_animation_manager = new cAnimation_Manager();

    // left side
    cParticle_Emitter* anim = new cParticle_Emitter(m_handler->m_level->m_sprite_manager);
    anim->Set_Image_Filename(utf8_to_path("clouds/default_1/1_middle.png"));
    anim->Set_Emitter_Rect(-100, static_cast<float>(-game_res_h), 0, game_res_h * 0.5f);
    anim->Set_Emitter_Time_to_Live(-1);
    anim->Set_Emitter_Iteration_Interval(16);
    anim->Set_Direction_Range(350, 20);
    anim->Set_Time_to_Live(800);
    anim->Set_Fading_Alpha(0);
    anim->Set_Scale(0.2f, 0.2f);
    anim->Set_Color(Color(static_cast<uint8_t>(255), 255, 255, 200), Color(static_cast<uint8_t>(0), 0, 0, 55));
    anim->Set_Speed(0.05f, 0.005f);
    anim->Set_Pos_Z(0.0015f, 0.0004f);

    m_animation_manager->Add(anim);

    // right side
    anim = new cParticle_Emitter(m_handler->m_level->m_sprite_manager);
    anim->Set_Image_Filename(utf8_to_path("clouds/default_1/1_middle.png"));
    anim->Set_Emitter_Rect(static_cast<float>(game_res_w) + 100, static_cast<float>(-game_res_h), 0, static_cast<float>(game_res_h) * 0.5f);
    anim->Set_Emitter_Time_to_Live(-1);
    anim->Set_Emitter_Iteration_Interval(16);
    anim->Set_Direction_Range(170, 20);
    anim->Set_Time_to_Live(800);
    anim->Set_Fading_Alpha(0);
    anim->Set_Scale(0.2f, 0.2f);
    anim->Set_Color(Color(static_cast<uint8_t>(255), 255, 255, 200), Color(static_cast<uint8_t>(0), 0, 0, 55));
    anim->Set_Speed(0.05f, 0.005f);
    anim->Set_Pos_Z(0.0015f, 0.0004f);

    m_animation_manager->Add(anim);
}

cMenuCore::~cMenuCore(void)
{
    Unload();

    delete m_handler;
    delete m_animation_manager;
}

bool cMenuCore::Handle_Event(const sf::Event& evt)
{
    switch (evt.type) {
    case sf::Event::MouseMoved: {
        m_handler->Update_Mouse();
        break;
    }
    // other events
    default: {
        break;
    }
    }

    return 0;
}

bool cMenuCore::Key_Down(const sf::Event& evt)
{
    // Down (todo: detect event for joystick better)
    if (evt.key.code == sf::Keyboard::Down || evt.key.code == pPreferences->m_key_down) {
        if (m_handler->Get_Size() <= static_cast<unsigned int>(m_handler->m_active + 1)) {
            m_handler->Set_Active(0);
        }
        else {
            m_handler->Set_Active(m_handler->m_active + 1);
        }
    }
    // Up (todo: detect event for joystick better)
    else if (evt.key.code == sf::Keyboard::Up || evt.key.code == pPreferences->m_key_up) {
        if (m_handler->m_active <= 0) {
            m_handler->Set_Active(m_handler->Get_Size() - 1);
        }
        else {
            m_handler->Set_Active(m_handler->m_active - 1);
        }
    }
    // Activate Button
    else if (evt.key.code == sf::Keyboard::Return) {
        if (m_menu_data) {
            m_menu_data->Item_Activated(m_handler->m_active);
        }
    }
    // exit
    else if (evt.key.code == sf::Keyboard::Escape) {
        m_menu_data->Exit();
    }
    else {
        // not processed
        return 0;
    }

    // key got processed
    return 1;
}

bool cMenuCore::Key_Up(const sf::Event& evt)
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

bool cMenuCore::Joy_Button_Down(unsigned int button)
{
    // Activate button
    if (button == pPreferences->m_joy_button_action) {
        if (m_menu_data) {
            m_menu_data->Item_Activated(m_handler->m_active);
        }
    }
    // exit
    else if (button == pPreferences->m_joy_button_exit) {
        m_menu_data->Exit();
    }
    else {
        // not processed
        return 0;
    }

    // key got processed
    return 1;
}

bool cMenuCore::Joy_Button_Up(unsigned int button)
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

bool cMenuCore::Mouse_Down(sf::Mouse::Button button)
{
    // nothing yet
    if (button == sf::Mouse::Left) {
        sf::FloatRect itemrect = m_handler->Get_Active_Item_Rect();

        if (itemrect.contains(static_cast<float>(pMouseCursor->m_x), static_cast<float>(pMouseCursor->m_y))) {
            m_menu_data->Item_Activated(m_handler->m_active);
            return 1;
        }
    }
    else {
        // not processed
        return 0;
    }

    // button got processed
    return 1;
}

bool cMenuCore::Mouse_Up(sf::Mouse::Button button)
{
    // not processed
    return 0;
}

void cMenuCore::Load(const MenuID menu /* = MENU_MAIN */, const GameMode exit_gamemode /* = MODE_NOTHING */)
{
    Unload();
    // reset menu handler
    m_handler->Reset();

    // clear mouse active object
    pMouseCursor->Double_Click(0);

    // default background color to white
    glClearColor(1, 1, 1, 1);

    // Set ID
    m_menu_id = menu;

    // ## Create menu class
    // Main
    if (m_menu_id == MENU_MAIN) {
        m_menu_data = static_cast<cMenu_Base*>(new cMenu_Main());
    }
    // Start
    else if (m_menu_id == MENU_START) {
        m_menu_data = static_cast<cMenu_Base*>(new cMenu_Start());
    }
    // Options
    else if (m_menu_id == MENU_OPTIONS) {
        m_menu_data = static_cast<cMenu_Base*>(new cMenu_Options());
    }
    // Load
    else if (m_menu_id == MENU_LOAD) {
        m_menu_data = static_cast<cMenu_Base*>(new cMenu_Savegames(0));
    }
    // Save
    else if (m_menu_id == MENU_SAVE) {
        m_menu_data = static_cast<cMenu_Base*>(new cMenu_Savegames(1));
    }
    // Credits
    else if (m_menu_id == MENU_CREDITS) {
        m_menu_data = static_cast<cMenu_Base*>(new cMenu_Credits(m_handler->Get_TSC_Logo()));
    }

    m_menu_data->Set_Exit_To_Game_Mode(exit_gamemode);
    m_menu_data->Init();
}

void cMenuCore::Enter(const GameMode old_mode /* = MODE_NOTHING */)
{
    // set active camera
    pActive_Camera = m_handler->m_camera;
    // set active player
    pActive_Player = m_handler->m_player;
    // set animation manager
    pActive_Animation_Manager = m_animation_manager;

    editor_enabled = 0;

    pMouseCursor->Set_Sprite_Manager(m_handler->m_level->m_sprite_manager);
    // show mouse
    pMouseCursor->Set_Active(1);
    // camera
    m_handler->m_camera->Reset_Pos();
    // update animation ( for the valid draw state )
    m_animation_manager->Update();
    // if not entering from another menu
    if (old_mode == MODE_NOTHING || old_mode == MODE_LEVEL || old_mode == MODE_OVERWORLD) {
        // set camera start position
        m_handler->m_camera->Reset_Pos();

        // pre-update animations
        for (cSprite_List::iterator itr = m_handler->m_level->m_sprite_manager->objects.begin(); itr != m_handler->m_level->m_sprite_manager->objects.end(); ++itr) {
            cSprite* obj = (*itr);

            if (obj->m_type == TYPE_PARTICLE_EMITTER) {
                cParticle_Emitter* emitter = static_cast<cParticle_Emitter*>(obj);
                emitter->Pre_Update();
            }
        }

        for (cAnimation_Manager::cAnimation_List::iterator itr = m_animation_manager->objects.begin(); itr != m_animation_manager->objects.end(); ++itr) {
            cAnimation* obj = (*itr);

            if (obj->m_type == TYPE_PARTICLE_EMITTER) {
                cParticle_Emitter* emitter = static_cast<cParticle_Emitter*>(obj);
                emitter->Pre_Update();
            }
        }

        pFramerate->Update();
    }

    if (m_menu_data) {
        m_menu_data->Enter(old_mode);
    }

    // Do not play the menu music when the menu is entered from
    // within a level.
    // TODO: Also don’t play it when the menu is entered from
    // an overworld.
    if (!pActive_Level->Is_Loaded()) {
        if (m_menu_id == MENU_CREDITS)
            pAudio->Play_Music(GAME_CREDITS_MUSIC, true, 0, 1500);
        else
            pAudio->Play_Music(GAME_TITLE_MUSIC, true, 0, 1500);
    }
}

void cMenuCore::Leave(const GameMode next_mode /* = MODE_NOTHING */)
{
    // if not in menu mode
    if (Game_Mode != MODE_MENU) {
        return;
    }

    // hide mouse
    if (!editor_enabled) {
        pMouseCursor->Set_Active(0);
    }

    if (m_menu_data) {
        m_menu_data->Leave(next_mode);
    }
}

void cMenuCore::Unload(void)
{
    m_menu_id = MENU_NOTHING;

    if (m_menu_data) {
        delete m_menu_data;
        m_menu_data = NULL;
    }
}

void cMenuCore::Update(void)
{
    if (!m_menu_data) {
        return;
    }

    // if not in a level/world
    if (m_menu_data->m_exit_to_gamemode == MODE_NOTHING) {
        m_handler->Update();
    }

    m_menu_data->Update();

    // update performance timer
    pFramerate->m_perf_timer[PERF_UPDATE_MENU]->Update();
}

void cMenuCore::Draw(void)
{
    if (!m_menu_data) {
        return;
    }

    m_menu_data->Draw();

    // update performance timer
    pFramerate->m_perf_timer[PERF_DRAW_MENU]->Update();
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

cMenuCore* pMenuCore = NULL;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

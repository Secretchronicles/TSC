/***************************************************************************
 * menu.cpp  -  menu handler
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

namespace TSC {

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

cMenu_Item::cMenu_Item(cSprite_Manager* sprite_manager)
    : cHudSprite(sprite_manager)
{
    Set_Scale_Directions(1, 1, 1, 1);
    m_active = 0;
    m_is_quit = 0;

    m_image_default = new cHudSprite(sprite_manager);
    m_image_menu = new cHudSprite(sprite_manager);
}

cMenu_Item::~cMenu_Item(void)
{
    if (m_image_default) {
        delete m_image_default;
    }

    if (m_image_menu) {
        delete m_image_menu;
    }
}

void cMenu_Item::Set_Active(bool active /* = 0 */)
{
    m_active = active;
    m_rot_z = 0;
    Set_Scale(1);

    if (!active) {
        Set_Color_Combine(0, 0, 0, 0);
    }
}

void cMenu_Item::Draw(cSurface_Request* request /* = NULL */)
{
    if (m_active) {
        // rotation is used for the scale state
        if (!m_rot_z) {
            Add_Scale((1.2f / m_image->m_w) * pFramerate->m_speed_factor);
        }
        else {
            Add_Scale(-(1.2f / m_image->m_w) * pFramerate->m_speed_factor);
        }

        if (m_image->m_w * m_scale_x > m_image->m_w + 10.0f) {
            m_rot_z = 0.0001f;
        }
        else if (m_scale_x < 1.0f) {
            m_rot_z = 0.0f;
        }
    }

    cHudSprite::Draw(request);

    if (m_active) {
        float strength = m_image->m_w * (m_scale_x - 1);

        // boost color to yellow
        Set_Color_Combine(strength / 40, strength / 40, 0, GL_ADD);

        m_pos_x = m_start_pos_x;
        m_pos_y = m_start_pos_y;

        m_image_menu->Draw();
    }
}

/* *** *** *** *** *** *** cMenuHandler *** *** *** *** *** *** *** *** *** *** *** */

cMenuHandler::cMenuHandler(void)
{
    m_level = cLevel::Load_From_File(pResource_Manager->Get_Game_Level(pPreferences->m_menu_level + ".smclvl"));
    m_camera = new cCamera(m_level->m_sprite_manager);
    m_player = new cSprite(m_level->m_sprite_manager);
    m_player->Set_Massive_Type(MASS_PASSIVE);
    Reset();

    // TSC logo image
    //cHudSprite* sprite = new cHudSprite(m_level->m_sprite_manager);
    //sprite->Set_Image(pVideo->Get_Surface("game/logo/tsc_big_1.png"));
    //sprite->Set_Pos(180.0f, 20.0f);
    //sprite->Set_Scale(0.8f);
    //sprite->Set_Massive_Type(MASS_FRONT_PASSIVE);
    //m_level->m_sprite_manager->Add(sprite);
}

cMenuHandler::~cMenuHandler(void)
{
    Reset();

    delete m_camera;
    delete m_level;
    delete m_player;
}

void cMenuHandler::Add_Menu_Item(cMenu_Item* item, float shadow_pos /* = 0 */, Color shadow_color /* = static_cast<Uint8>(0) */)
{
    if (!item) {
        printf("Menu item is NULL ( current Menu size : %d )\n", Get_Size());
        return;
    }

    item->Set_Shadow_Pos(shadow_pos);
    item->Set_Shadow_Color(shadow_color);
    item->Set_Image(item->m_image_default->m_image);
    m_items.push_back(item);

    if (m_active == -1 && Get_Size() == 1) {
        Set_Active(0);
    }
}

void cMenuHandler::Reset(void)
{
    for (MenuList::iterator itr = m_items.begin(); itr != m_items.end(); ++itr) {
        delete *itr;
    }

    m_items.clear();

    // nothing is active
    m_active = -1;
}

void cMenuHandler::Set_Active(int num)
{
    // if not already active and exists
    if (num == static_cast<int>(m_active) || num >= static_cast<int>(m_items.size()) || (num >= 0 && !m_items[num])) {
        return;
    }

    if (num >= 0 && static_cast<unsigned int>(num) < m_items.size()) {
        // set last active item un-active
        if (m_active >= 0 && static_cast<unsigned int>(m_active) < m_items.size()) {
            m_items[m_active]->Set_Active(0);
        }
    }
    else if (num == -1) {
        m_items[m_active]->Set_Active(0);
    }

    m_active = num;

    if (m_active >= 0) {
        m_items[m_active]->Set_Active(1);
    }
}

void cMenuHandler::Update_Mouse(void)
{
    int found = -1;

    // check
    for (unsigned int i = 0; i < m_items.size(); i++) {
        if (m_items[i]->m_col_rect.Intersects(static_cast<float>(pMouseCursor->m_x), static_cast<float>(pMouseCursor->m_y))) {
            found = i;
            break;
        }
    }

    // ignore mouse init
    if (found < 0 && input_event.motion.x == pMouseCursor->m_x) {
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

    // menu items
    for (MenuList::iterator itr = m_items.begin(); itr != m_items.end(); ++itr) {
        (*itr)->Draw();
    }
}

cMenu_Item* cMenuHandler::Get_Active_Item(void)
{
    if (m_active < 0 || static_cast<unsigned int>(m_active) > m_items.size()) {
        return NULL;
    }

    return m_items[m_active];
}

unsigned int cMenuHandler::Get_Size(void) const
{
    return static_cast<unsigned int>(m_items.size());
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
    anim->Set_Color(Color(static_cast<Uint8>(255), 255, 255, 200), Color(static_cast<Uint8>(0), 0, 0, 55));
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
    anim->Set_Color(Color(static_cast<Uint8>(255), 255, 255, 200), Color(static_cast<Uint8>(0), 0, 0, 55));
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

bool cMenuCore::Handle_Event(SDL_Event* ev)
{
    switch (ev->type) {
    case SDL_MOUSEMOTION: {
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

bool cMenuCore::Key_Down(SDLKey key)
{
    // Down (todo: detect event for joystick better)
    if (key == SDLK_DOWN || key == pPreferences->m_key_down) {
        if (m_handler->Get_Size() <= static_cast<unsigned int>(m_handler->m_active + 1)) {
            m_handler->Set_Active(0);
        }
        else {
            m_handler->Set_Active(m_handler->m_active + 1);
        }
    }
    // Up (todo: detect event for joystick better)
    else if (key == SDLK_UP || key == pPreferences->m_key_up) {
        if (m_handler->m_active <= 0) {
            m_handler->Set_Active(m_handler->Get_Size() - 1);
        }
        else {
            m_handler->Set_Active(m_handler->m_active - 1);
        }
    }
    // Activate Button
    else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
        if (m_menu_data) {
            m_menu_data->m_action = 1;
        }
    }
    // Fast Debug Level entering
    else if (key == SDLK_x && pKeyboard->Is_Ctrl_Down()) {
        // random level name
        std::string lvl_name;

        if (!CEGUI::WindowManager::getSingleton().isWindowPresent("listbox_levels")) {
            // Create temporary start menu
            cMenu_Start* menu_start = new cMenu_Start();

            menu_start->Init();
            // get levels listbox
            CEGUI::Listbox* listbox_levels = static_cast<CEGUI::Listbox*>(CEGUI::WindowManager::getSingleton().getWindow("listbox_levels"));
            // select random level
            listbox_levels->setItemSelectState(rand() % listbox_levels->getItemCount(), 1);
            // get level name
            lvl_name = listbox_levels->getFirstSelectedItem()->getText().c_str();
            menu_start->Load_Level(lvl_name);
            // destroy menu
            delete menu_start;
        }
        else {
            // Get levels listbox
            CEGUI::Listbox* listbox_levels = static_cast<CEGUI::Listbox*>(CEGUI::WindowManager::getSingleton().getWindow("listbox_levels"));
            // select random level
            listbox_levels->setItemSelectState(rand() % listbox_levels->getItemCount(), 1);
            // get level name
            lvl_name = listbox_levels->getFirstSelectedItem()->getText().c_str();
            static_cast<cMenu_Start*>(pMenuCore->m_menu_data)->Load_Level(lvl_name);
        }
    }
    // exit
    else if (key == SDLK_ESCAPE) {
        m_menu_data->Exit();
    }
    else {
        // not processed
        return 0;
    }

    // key got processed
    return 1;
}

bool cMenuCore::Key_Up(SDLKey key)
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

bool cMenuCore::Joy_Button_Down(Uint8 button)
{
    // Activate button
    if (button == pPreferences->m_joy_button_action) {
        if (m_menu_data) {
            m_menu_data->m_action = 1;
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

bool cMenuCore::Joy_Button_Up(Uint8 button)
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

bool cMenuCore::Mouse_Down(Uint8 button)
{
    // nothing yet
    if (button == SDL_BUTTON_LEFT) {
        cMenu_Item* item = m_handler->Get_Active_Item();

        if (item && item->m_col_rect.Intersects(static_cast<float>(pMouseCursor->m_x), static_cast<float>(pMouseCursor->m_y))) {
            m_menu_data->m_action = 1;
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

bool cMenuCore::Mouse_Up(Uint8 button)
{
    // nothing yet
    if (0) {
        //
    }
    else {
        // not processed
        return 0;
    }

    // button got processed
    return 1;
}

cMenu_Item* cMenuCore::Auto_Menu(std::string imagename, std::string imagefilename_menu, float ypos /* = 0 */, bool is_quit /* = 0 */)
{
    cMenu_Item* temp_item = new cMenu_Item(m_handler->m_level->m_sprite_manager);

    // the menu image
    if (imagefilename_menu.length() > 0) {
        temp_item->m_image_menu->Set_Image(pVideo->Get_Surface(pResource_Manager->Get_Game_Pixmap("menu/items/" + imagefilename_menu)), 1);
    }

    // the active image
    if (imagename.length() > 0) {
        temp_item->m_image_default->Set_Image(pVideo->Get_Surface(pResource_Manager->Get_Game_Pixmap("menu/" + imagename)), 1);
    }

    // position and initialization
    temp_item->Set_Pos((game_res_w * 0.5f) - (temp_item->m_image_default->m_col_rect.m_w * 0.5f), ypos);
    temp_item->m_is_quit = is_quit;

    return temp_item;
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
        m_menu_data = static_cast<cMenu_Base*>(new cMenu_Credits());
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

    pHud_Manager->Set_Sprite_Manager(m_handler->m_level->m_sprite_manager);
    pMouseCursor->Set_Sprite_Manager(m_handler->m_level->m_sprite_manager);
    // show mouse
    pMouseCursor->Set_Active(1);
    // camera
    m_handler->m_camera->Reset_Pos();
    // position HUD
    pHud_Manager->Update_Text();
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
            pAudio->Play_Music("land/hyper_1.ogg", -1, 0, 1500);
        else
            pAudio->Play_Music("game/menu.ogg", -1, 0, 1500);
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

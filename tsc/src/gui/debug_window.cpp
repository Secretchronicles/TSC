/***************************************************************************
 * debug_window.cpp - The debug window shown in debug mode
 *
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

#include "../core/global_basic.hpp"
#include "../core/game_core.hpp"
#include "../core/i18n.hpp"
#include "../core/framerate.hpp"
#include "../core/camera.hpp"
#include "../core/property_helper.hpp"
#include "../level/level.hpp"
#include "../level/level_player.hpp"
#include "../overworld/overworld.hpp"
#include "../objects/bonusbox.hpp"
#include "../scene/scene.hpp"
#include "debug_window.hpp"

// extern
TSC::cDebug_Window* TSC::gp_debug_window = NULL;

using namespace TSC;

cDebug_Window::cDebug_Window(cSprite_Manager* p_sprite_manager)
    : mp_sprite_manager(p_sprite_manager),
      mp_debugwin_root(NULL)
{
    // Load layout file and add it to the root
    mp_debugwin_root = CEGUI::WindowManager::getSingleton().loadLayoutFromFile("debug_window.layout");
    CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->addChild(mp_debugwin_root);
    mp_debugwin_root->hide(); // Do not show for now

    mp_debugwin_root->setText(UTF8_("Debugging Information"));
}

cDebug_Window::~cDebug_Window()
{
    if (mp_debugwin_root) {
        CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->removeChild(mp_debugwin_root);
        CEGUI::WindowManager::getSingleton().destroyWindow(mp_debugwin_root);
        mp_debugwin_root = NULL;
    }
}

void cDebug_Window::Show()
{
    mp_debugwin_root->show();
}

void cDebug_Window::Hide()
{
    mp_debugwin_root->hide();
}

void cDebug_Window::Toggle()
{
    if (mp_debugwin_root->isVisible())
        Hide();
    else
        Show();
}

/* TSC legacy code design strikes back. This method needs to be
 * called anywhere where the global sprite manager is changed.
 * The problem is, there's no global sprite manager. Each major
 * object (level, world, menu) has its own notion and the game
 * engine assumes that from these, only one ever is the active
 * one. Since there's no way to find out which one the currently
 * active one is, each of them sets the sprite manager on all
 * possibly interested objects when it becomes active. Care
 * needs to be taken that Set_Sprite_Manager() really is called
 * each time such a condition happens somewhere in the code. */
void cDebug_Window::Set_Sprite_Manager(cSprite_Manager* p_sprite_manager)
{
    mp_sprite_manager = p_sprite_manager;
}

bool cDebug_Window::IsVisible()
{
    return mp_debugwin_root->isVisible();
}

void cDebug_Window::Update()
{
    if (!IsVisible())
        return;

    char buf[4096];

    snprintf(buf,
             4096,
             // TRANS: Do not translate the part in brackets
             _("[colour='FFFFFF00']FPS: Best: %.02f Worst: %.02f Current: %0.2f"),
             pFramerate->m_fps_best,
             pFramerate->m_fps_worst,
             pFramerate->m_fps);
    mp_debugwin_root->getChild("fps")->setText(reinterpret_cast<const CEGUI::utf8*>(buf));

    snprintf(buf,
             4096,
             _("Camera X: %d Y: %d"),
             static_cast<int>(pActive_Camera->m_x),
             static_cast<int>(pActive_Camera->m_y));
    mp_debugwin_root->getChild("camera")->setText(reinterpret_cast<const CEGUI::utf8*>(buf));

    switch (Game_Mode) {
    case MODE_LEVEL:
    case MODE_LEVEL_SETTINGS: // fall-through
        snprintf(buf,
                 4096,
                 _("Level: %s (return stack size: %ld)"),
                 pActive_Level->Get_Level_Name().c_str(),
                 pLevel_Player->m_return_stack.size());
        break;
    case MODE_OVERWORLD:
        snprintf(buf, 4096, _("Overworld: %s"),
                 pActive_Overworld->m_description->m_name.c_str());
        break;
    case MODE_MENU:
        snprintf(buf, 4096, _("<Menu Active>"));
        break;
    case MODE_SCENE:
        snprintf(buf, 4096, _("Scene: %s"), pActive_Scene->Get_Name().c_str());
        break;
    case MODE_NOTHING:
        snprintf(buf, 4096, "--");
        break;
    } // No default to let the compiler warn about missed values
    mp_debugwin_root->getChild("general")->setText(reinterpret_cast<const CEGUI::utf8*>(buf));

    // Count specific objects
    int halfmassives = 0;
    int bonusboxes = 0;
    int goldboxes = 0;
    int moving_platforms = 0;
    for (cSprite* p_obj: mp_sprite_manager->objects) {
        if (p_obj->m_massive_type == MASS_HALFMASSIVE) {
            halfmassives++;
        }
        if (p_obj->m_type == TYPE_MOVING_PLATFORM) { // not "else if"! A moving platform might be set to halfmassive and has then to be counted twice.
            moving_platforms++;
        }
        else if (p_obj->m_type == TYPE_BONUS_BOX) {
            bonusboxes++;
            if (static_cast<const cBonusBox*>(p_obj)->box_type == TYPE_GOLDPIECE) {
                goldboxes++;
            }
        }
    }
    snprintf(buf,
             4096,
             // TRANS: Abbreviations mean:
             // TRANS: T=Total, P=Passive, M=Massive, E=Enemy, A=Active, H=Halfmassive
             _("T: %lu P: %d M: %d E: %d A: %d H: %d"),
             mp_sprite_manager->size(),
             mp_sprite_manager->Get_Size_Array(ARRAY_PASSIVE),
             mp_sprite_manager->Get_Size_Array(ARRAY_MASSIVE),
             mp_sprite_manager->Get_Size_Array(ARRAY_ENEMY),
             mp_sprite_manager->Get_Size_Array(ARRAY_ACTIVE),
             halfmassives);
    mp_debugwin_root->getChild("objectcount")->setText(reinterpret_cast<const CEGUI::utf8*>(buf));

    snprintf(buf,
             4096,
             // TRANS: Abbreviations mean:
             // TRANS: BBox=Bonus boxes, GBox=Gold boxes, MPlat=Moving platforms
             _("BBox: %d GBox: %d MPlat: %d"),
             bonusboxes - goldboxes,
             goldboxes,
             moving_platforms);
    mp_debugwin_root->getChild("objectcount2")->setText(reinterpret_cast<const CEGUI::utf8*>(buf));

    snprintf(buf,
             4096,
             _("Player X1: %.4f X2: %.4f"),
             pActive_Player->m_pos_x,
             pLevel_Player->m_col_rect.m_x + pLevel_Player->m_col_rect.m_w);
    mp_debugwin_root->getChild("player_info")->setText(reinterpret_cast<const CEGUI::utf8*>(buf));

    snprintf(buf,
             4096,
             _("Player Y1: %.4f Y2: %.4f"),
             pActive_Player->m_pos_y,
             pLevel_Player->m_col_rect.m_y + pLevel_Player->m_col_rect.m_h);
    mp_debugwin_root->getChild("player_info2")->setText(reinterpret_cast<const CEGUI::utf8*>(buf));

    snprintf(buf,
             4096,
             _("Player XVel: %.4f, YVel: %.4f"),
             pLevel_Player->m_velx,
             pLevel_Player->m_vely);
    mp_debugwin_root->getChild("player_info3")->setText(reinterpret_cast<const CEGUI::utf8*>(buf));

    snprintf(buf,
             4096,
             _("Player MState: %d Ground: %d (%s)"),
             static_cast<int>(pLevel_Player->m_state),
             pLevel_Player->m_ground_object ? static_cast<int>(pLevel_Player->m_ground_object->m_massive_type) : -1,
             pLevel_Player->m_ground_object ? Get_Massive_Type_Name(pLevel_Player->m_ground_object->m_massive_type).c_str() : "--");
    mp_debugwin_root->getChild("player_info4")->setText(reinterpret_cast<const CEGUI::utf8*>(buf));

    snprintf(buf,
             4096,
             _("Game Mode: %d"),
             Game_Mode);
    mp_debugwin_root->getChild("game_mode")->setText(reinterpret_cast<const CEGUI::utf8*>(buf));
}

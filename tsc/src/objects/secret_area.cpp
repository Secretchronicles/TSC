/***************************************************************************
 * secret_area.cpp - Where you find bonuses
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

#include "../core/global_game.hpp"
#include "secret_area.hpp"
#include "../level/level_player.hpp"
#include "../core/game_core.hpp"
#include "../core/i18n.hpp"
#include "../core/xml_attributes.hpp"
#include "../core/sprite_manager.hpp"
#include "../level/level.hpp"
#include "../level/level_settings.hpp"
#include "../core/editor/editor.hpp"
#include "../level/level_editor.hpp"
#include "../video/renderer.hpp"
#include "../core/framerate.hpp"
#include "../user/savegame/save_level.hpp"
#include "../scripting/events/activate_event.hpp"

using namespace TSC;

cSecret_Area::cSecret_Area(cSprite_Manager* sprite_manager)
    : cMovingSprite(sprite_manager, "secretarea")
{
    cSecret_Area::Init();
}

cSecret_Area::cSecret_Area(XmlAttributes& attributes, cSprite_Manager* sprite_manager)
    : cMovingSprite(sprite_manager, "secretarea")
{
    cSecret_Area::Init();

    // position and sizing
    GL_rect r;
    r.m_x = string_to_float(attributes["posx"]);
    r.m_y = string_to_float(attributes["posy"]);
    r.m_w = string_to_float(attributes["width"]);
    r.m_h = string_to_float(attributes["height"]);
    Set_Rect(r, true);
}

cSecret_Area::~cSecret_Area(void)
{
    if (mp_msg_window) {
        CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->removeChild(mp_msg_window);
        CEGUI::WindowManager::getSingleton().destroyWindow(mp_msg_window);
        mp_msg_window = NULL;
    }
}

void cSecret_Area::Init()
{
    m_sprite_array = ARRAY_ACTIVE;
    m_type = TYPE_SECRET_AREA;
    m_name = _("Secret Area");
    m_massive_type = MASS_PASSIVE;
    m_editor_pos_z = 0.112f;
    m_camera_range = 1000;

    // size
    Set_Rect(GL_rect(m_rect.m_x, m_rect.m_y, 100, 100), true);

    m_transparency_counter = 0.0f;
    m_move_counter = 0.0f;
    mp_msg_window = NULL;
    m_activated = false;
}

cSecret_Area* cSecret_Area::Copy(void) const
{
    cSecret_Area* secarea = new cSecret_Area(m_sprite_manager);
    secarea->Set_Rect(m_rect);
    secarea->m_activated = m_activated;
    return secarea;
}

/**
 * Set the entire extends of the secret area at once, rather than
 * having to cater for all possible X/Y positions and rectangles
 * involved.
 */
void cSecret_Area::Set_Rect(GL_rect rect, bool new_start_pos /* = false */)
{
    Set_Pos(rect.m_x, rect.m_y, new_start_pos);
    m_rect.m_w       = rect.m_w;
    m_rect.m_h       = rect.m_h;
    m_col_rect.m_w   = m_rect.m_w;
    m_col_rect.m_h   = m_rect.m_h;
    m_start_rect.m_w = m_rect.m_w;
    m_start_rect.m_h = m_rect.m_h;
}

void cSecret_Area::Update(void)
{
    if (mp_msg_window) {
        const static float alpha_max = 1.0f /* max alpha*/ / 100.0f /* max time */;
        if (m_transparency_counter >= 100.0f) {
            CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->removeChild(mp_msg_window);
            CEGUI::WindowManager::getSingleton().destroyWindow(mp_msg_window);
            mp_msg_window = NULL;
        }
        else {
            mp_msg_window->setAlpha(1.0f - m_transparency_counter * alpha_max);
            m_transparency_counter += pFramerate->m_speed_factor;

            if (m_move_counter >= 2.5f) {
                mp_msg_window->setYPosition(mp_msg_window->getYPosition() - CEGUI::UDim::px());
                m_move_counter = 0.0f;
            }
            else {
                m_move_counter += pFramerate->m_speed_factor;
            }
        }
    }

    // Activate it if the player is inside and it's not activated
    if (!m_activated && Is_In_Range()) {
        if (pLevel_Player->m_col_rect.Intersects(m_col_rect)) {
            Activate();
        }
    }
}

void cSecret_Area::Draw(cSurface_Request* request /* = NULL */)
{
    if (!m_valid_draw)
        return;

    // draw rect
    cRect_Request* req = new cRect_Request();
    pVideo->Draw_Rect(m_col_rect.m_x - pActive_Camera->m_x, m_col_rect.m_y - pActive_Camera->m_y, m_col_rect.m_w, m_col_rect.m_h, m_editor_pos_z, &orange, req);
    req->m_filled = 0;
    req->m_line_width = 6.0f;
    pRenderer->Add(req);
}

bool cSecret_Area::Is_Draw_Valid(void)
{
    // if editor not enabled
    if (!editor_enabled) {
        return 0;
    }

    // if not visible on the screen
    if (!m_active || !Is_Visible_On_Screen()) {
        return 0;
    }

    return 1;
}

void cSecret_Area::Activate(void)
{
    if (m_activated)
        return;

    if (mp_msg_window) { // This should never happen, because m_activate is true in that case
        CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->removeChild(mp_msg_window);
        CEGUI::WindowManager::getSingleton().destroyWindow(mp_msg_window);
        mp_msg_window = NULL;
    }

    mp_msg_window = CEGUI::WindowManager::getSingleton().loadLayoutFromFile("secret_area_message.layout");
    mp_msg_window->getChild("message")->setText(UTF8_("You found a secret area!"));
    CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->addChild(mp_msg_window);

    m_transparency_counter = 0.0f;
    m_move_counter = 0.0f;
    m_activated = true;

    Scripting::cActivate_Event evt;
    evt.Fire(pActive_Level->m_mruby, this);
}

void cSecret_Area::Editor_Activate(void)
{
}

void cSecret_Area::Editor_Deactivate(void)
{
    cMovingSprite::Editor_Deactivate();
}

void cSecret_Area::Editor_State_Update(void)
{
}

xmlpp::Element* cSecret_Area::Save_To_XML_Node(xmlpp::Element* p_element)
{
    xmlpp::Element* p_node = cMovingSprite::Save_To_XML_Node(p_element);
    Add_Property(p_node, "width", m_rect.m_w);
    Add_Property(p_node, "height", m_rect.m_h);

    return p_node;
}

bool cSecret_Area::Save_To_Savegame_XML_Node(xmlpp::Element* p_element) const
{
    cMovingSprite::Save_To_Savegame_XML_Node(p_element);

    if (m_activated)
        Add_Property(p_element, "activated", m_activated);

    return true;
}

void cSecret_Area::Load_From_Savegame(cSave_Level_Object* save_object)
{
    cMovingSprite::Load_From_Savegame(save_object);

    if (save_object->exists("activated"))
        m_activated = string_to_bool(save_object->Get_Value("activated"));
}

std::string cSecret_Area::Get_XML_Type_Name()
{
    return "secretarea";
}

void cSecret_Area::Set_Massive_Type(MassiveType type)
{
    // Ignore to prevent "m" toggling in level editor
}

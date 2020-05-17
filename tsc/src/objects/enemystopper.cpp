/***************************************************************************
 * enemystopper.cpp  -  enemystopper class
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

#include "../objects/enemystopper.hpp"
#include "../level/level_settings.hpp"
#include "../core/sprite_manager.hpp"
#include "../level/level_editor.hpp"
#include "../core/game_core.hpp"
#include "../core/i18n.hpp"
#include "../core/xml_attributes.hpp"

namespace TSC {

/* *** *** *** *** *** cEnemyStopper *** *** *** *** *** *** *** *** *** *** *** *** */

cEnemyStopper::cEnemyStopper(cSprite_Manager* sprite_manager)
    : cMovingSprite(sprite_manager, "enemystopper")
{
    cEnemyStopper::Init();
}

cEnemyStopper::cEnemyStopper(XmlAttributes& attributes, cSprite_Manager* sprite_manager)
    : cMovingSprite(sprite_manager, "enemystopper")
{
    cEnemyStopper::Init();
    Set_Pos(string_to_float(attributes["posx"]), string_to_float(attributes["posy"]), true);
}

cEnemyStopper::~cEnemyStopper(void)
{
    //
}

cEnemyStopper* cEnemyStopper::Copy(void) const
{
    cEnemyStopper* enemystopper = new cEnemyStopper(m_sprite_manager);
    enemystopper->Set_Pos(m_start_pos_x, m_start_pos_y, 1);
    return enemystopper;
}

void cEnemyStopper::Init(void)
{
    m_sprite_array = ARRAY_ACTIVE;
    m_type = TYPE_ENEMY_STOPPER;
    m_massive_type = MASS_PASSIVE;
    m_editor_pos_z = 0.11f;

    m_name = _("Enemystopper");

    // size
    m_rect.m_w = 15.0f;
    m_rect.m_h = 15.0f;
    m_col_rect.m_w = m_rect.m_w;
    m_col_rect.m_h = m_rect.m_h;
    m_start_rect.m_w = m_rect.m_w;
    m_start_rect.m_h = m_rect.m_h;

    m_editor_color = Color(static_cast<uint8_t>(0), 0, 255, 128);
}

void cEnemyStopper::Draw(cSurface_Request* request /* = NULL */)
{
    if (!m_valid_draw) {
        return;
    }

    pVideo->Draw_Rect(m_col_rect.m_x - pActive_Camera->m_x, m_col_rect.m_y - pActive_Camera->m_y, m_col_rect.m_w, m_col_rect.m_h, m_editor_pos_z, &m_editor_color);
}

bool cEnemyStopper::Is_Draw_Valid(void)
{
    // if editor not enabled
    if (!editor_enabled) {
        return 0;
    }

    // if not active or not on the screen
    if (!m_active || !Is_Visible_On_Screen()) {
        return 0;
    }

    return 1;
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

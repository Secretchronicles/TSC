/***************************************************************************
 * shell.cpp
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

#include "shell.hpp"
#include "../video/gl_surface.hpp"
#include "../level/level_player.hpp"
#include "../core/errors.hpp"
#include "../core/property_helper.hpp"
#include "../core/i18n.hpp"
#include "../core/xml_attributes.hpp"
#include "../core/sprite_manager.hpp"
#include "../level/level.hpp"
#include "../level/level_settings.hpp"
#include "../core/editor/editor.hpp"
#include "../level/level_editor.hpp"

using namespace TSC;

cShell::cShell(cSprite_Manager* p_sprite_manager)
    : cArmy(p_sprite_manager)
{
    cShell::Init();
}

cShell::cShell(XmlAttributes& attributes, cSprite_Manager* p_sprite_manager)
    : cArmy(attributes, p_sprite_manager)
{
    // Actually this init should be before the cArmy constructor call,
    // (and behind cArmy::Init() there), but C++ limitations make this
    // impossible. Seems to work fine this way, though.
    cShell::Init();

    // Redo army init due to c++ limitations outlined above
    // position
    Set_Pos(string_to_float(attributes["posx"]), string_to_float(attributes["posy"]), true);

    // direction
    Set_Direction(Get_Direction_Id(attributes.fetch("direction", Get_Direction_Name(m_start_direction))), true);

    // color
    Set_Color(static_cast<DefaultColor>(Get_Color_Id(attributes.fetch("color", Get_Color_Name(m_color_type)))));
}

cShell::~cShell()
{
    //
}

void cShell::Init()
{
    cArmy::Init();

    m_type = TYPE_SHELL;
    m_name = "Shell";
    m_gravity_max = 22.0f;

    Set_Army_Moving_State(ARMY_SHELL_STAND);
}

cShell* cShell::Copy() const
{
    cShell* p_shell = new cShell(m_sprite_manager);
    p_shell->Set_Pos(m_start_pos_x, m_start_pos_y);
    p_shell->Set_Direction(m_start_direction, true);
    p_shell->Set_Color(m_color_type);
    return p_shell;
}

std::string cShell::Get_XML_Type_Name()
{
    return "shell";
}

void cShell::Update()
{
    // The loose shell can't stand up as the ordinary
    // army. We ignore how cArmy handles this and
    // implement updating ourselves.
    cEnemy::Update();
    if (!m_valid_update || !Is_In_Range())
        return;

    Update_Animation();

    if (m_army_state == ARMY_SHELL_STAND) {
        // slow down
        if (!Is_Float_Equal(m_velx, 0.0f)) {
            Add_Velocity_X(-m_velx * 0.2f);

            if (m_velx < 0.3f && m_velx > -0.3f) {
                m_velx = 0.0f;
            }
        }
    }
    else if (m_army_state == ARMY_SHELL_RUN)
        Update_Velocity();

    if (m_army_state == ARMY_SHELL_STAND || m_army_state == ARMY_SHELL_RUN) {
        if (!Is_Float_Equal(m_velx, 0.0f))
            Add_Rotation_Z((m_velx / (m_image->m_w * 0.009f)) * pFramerate->m_speed_factor);
    }

    if (m_player_counter > 0.0f) {
        m_player_counter -= pFramerate->m_speed_factor;

        if (m_player_counter <= 0.0f) {
            // do not start collision detection if colliding with alex
            if (pLevel_Player->m_col_rect.Intersects(m_col_rect)) {
                m_player_counter = 5.0f;
            }
            else {
                m_player_counter = 0.0f;
            }
        }
    }
}

void cShell::Stand_Up()
{
    throw (NotImplementedError("Loose shells can't stand up!"));
}

void cShell::Set_Color(DefaultColor col)
{
    if (m_color_type == col)
        return;

    cArmy::Set_Color(col);

    // Fix wrong image set set by cArmy::Set_Color()
    Set_Image_Set("shell", true);
    // Make it show up in the editor properly
    Set_Image_Num(10, true);
}

std::string cShell::Create_Name() const
{
    return m_name + " " + _(Get_Color_Name(m_color_type).c_str()) + " " + _(Get_Direction_Name(m_start_direction).c_str());
}

/***************************************************************************
 * crate.cpp
 *
 * Copyright © 2014 The TSC Contributors
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "crate.hpp"
#include "../core/xml_attributes.hpp"
#include "../core/sprite_manager.hpp"
#include "../core/i18n.hpp"
#include "../core/game_core.hpp"
#include "../audio/audio.hpp"
#include "../level/level_player.hpp"
#include "../user/savegame.hpp"
#include "../enemies/enemy.hpp"

using namespace TSC;

cCrate::cCrate(cSprite_Manager* p_sprite_manager)
    : cMovingSprite(p_sprite_manager, "crate")
{
    Init();
}

cCrate::cCrate(XmlAttributes& attributes, cSprite_Manager* p_sprite_manager)
    : cMovingSprite(p_sprite_manager, "crate")
{
    Init();
    Set_Pos(attributes.fetch<float>("posx", 0), attributes.fetch<float>("posy", 0), true);
}

cCrate* cCrate::Copy() const
{
    cCrate* p_crate = new cCrate(m_sprite_manager);
    p_crate->Set_Pos(m_start_pos_x, m_start_pos_y);
    return p_crate;
}

cCrate::~cCrate()
{
    //
}

void cCrate::Init()
{
    m_type = TYPE_CRATE;
    m_sprite_array = ARRAY_ACTIVE;
    m_massive_type = MASS_MASSIVE;
    m_name = _("Crate");
    m_gravity_max = 22.0f;
    m_crate_state = CRATE_STAND;

    m_can_be_on_ground = true;
    m_can_be_ground = true;
    Set_Scale_Directions(1, 1, 1, 1);

    Add_Image(pVideo->Get_Package_Surface("blocks/extra/box.png"));
    Set_Animation(false);
    Set_Image_Num(0, true);
}

void cCrate::Load_From_Savegame(cSave_Level_Object* p_saveobj)
{
    // state
    if (p_saveobj->exists("state")) {
        m_state = static_cast<Moving_state>(string_to_int(p_saveobj->Get_Value("state")));
    }

    // new position x
    if (p_saveobj->exists("new_posx")) {
        Set_Pos_X(string_to_float(p_saveobj->Get_Value("new_posx")));
    }

    // new position y
    if (p_saveobj->exists("new_posy")) {
        Set_Pos_Y(string_to_float(p_saveobj->Get_Value("new_posy")));
    }

    // velocity x
    if (p_saveobj->exists("velx")) {
        m_velx = string_to_float(p_saveobj->Get_Value("velx"));
    }

    // velocity y
    if (p_saveobj->exists("vely")) {
        m_vely = string_to_float(p_saveobj->Get_Value("vely"));
    }
}

cSave_Level_Object* cCrate::Save_To_Savegame(void)
{
    cSave_Level_Object* p_saveobj = new cSave_Level_Object();

    p_saveobj->m_type = m_type;
    p_saveobj->m_properties.push_back(cSave_Level_Object_Property("posx", int_to_string(static_cast<int>(m_start_pos_x))));
    p_saveobj->m_properties.push_back(cSave_Level_Object_Property("posy", int_to_string(static_cast<int>(m_start_pos_y))));

    p_saveobj->m_properties.push_back(cSave_Level_Object_Property("state", int_to_string(m_state)));

    // new position (only save if needed)
    if (!Is_Float_Equal(m_start_pos_x, m_pos_x) || !(Is_Float_Equal(m_start_pos_y, m_pos_y))) {
        p_saveobj->m_properties.push_back(cSave_Level_Object_Property("new_posx", int_to_string(static_cast<int>(m_pos_x))));
        p_saveobj->m_properties.push_back(cSave_Level_Object_Property("new_posy", int_to_string(static_cast<int>(m_pos_y))));
    }

    p_saveobj->m_properties.push_back(cSave_Level_Object_Property("velx", float_to_string(m_velx)));
    p_saveobj->m_properties.push_back(cSave_Level_Object_Property("vely", float_to_string(m_vely)));

    return p_saveobj;
}

void cCrate::Update()
{
    if (!m_valid_update || !Is_In_Range())
        return;
    if (m_crate_state == CRATE_DEAD)
        return;

    cMovingSprite::Update();

    // Slow down if moving
    if (m_crate_state == CRATE_SLIDE && !Is_Float_Equal(m_velx, 0.0f)) {
        Add_Velocity_X(-m_velx * 0.2f);

        // Stop tolerance
        if (m_velx > -0.3f && m_velx < 0.3f) {
            m_velx = 0.0f;
            m_crate_state = CRATE_STAND;
        }
    }
}

std::string cCrate::Get_XML_Type_Name()
{
    return "crate";
}

xmlpp::Element* cCrate::Save_To_XML_Node(xmlpp::Element* p_element)
{
    xmlpp::Element* p_node = cMovingSprite::Save_To_XML_Node(p_element);

    // No configuration currently

    return p_node;
}

void cCrate::Handle_Collision_Player(cObjectCollision* p_collision)
{
    if (m_crate_state == CRATE_DEAD)
        return;

    if (p_collision->m_direction == DIR_LEFT)
        m_velx = 20.0f;
    else if (p_collision->m_direction == DIR_RIGHT)
        m_velx = -20.0f;
    else if (p_collision->m_direction == DIR_BOTTOM) {
        // If the crate is moving down, it fell onto the player.
        if (m_vely > 0.5f)
            pLevel_Player->DownGrade_Player();
        return;
    }
    else // unsupported direction
        return;

    m_crate_state = CRATE_SLIDE;
    pAudio->Play_Sound("wood_1.ogg");
}

void cCrate::Handle_Collision_Enemy(cObjectCollision* p_collision)
{
    // When a crate falls onto an enemy, it gets killed.
    if (m_vely > 0.05f) {
        cEnemy* p_enemy = static_cast<cEnemy*>(m_sprite_manager->Get_Pointer(p_collision->m_number));
        p_enemy->DownGrade(true);
        Reset_On_Ground(); // The enemy cannot be ground for us, it was just killed
    }
}

void cCrate::Handle_out_of_Level(ObjectDirection dir)
{
    // abyss
    if (dir == DIR_BOTTOM) {
        m_crate_state = CRATE_DEAD;
        m_massive_type = MASS_PASSIVE;
    }

    // Don’t move it outside right/left level edge
    if (dir == DIR_LEFT)
        Set_Pos_X(pActive_Camera->m_limit_rect.m_x - m_col_pos.m_x);
    else if (dir == DIR_RIGHT)
        Set_Pos_X(pActive_Camera->m_limit_rect.m_x + pActive_Camera->m_limit_rect.m_w - m_col_pos.m_x - m_col_rect.m_w - 0.01f);

}

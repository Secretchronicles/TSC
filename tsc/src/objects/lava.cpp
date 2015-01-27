/***************************************************************************
 * lava.cpp  -  class for lava blocks
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

#include "lava.hpp"
#include "../level/level_player.hpp"
#include "../enemies/enemy.hpp"
#include "../core/xml_attributes.hpp"
#include "../core/game_core.hpp"
#include "../core/i18n.hpp"

using namespace TSC;

cLava::cLava(cSprite_Manager* p_sprite_manager)
    : cMovingSprite(p_sprite_manager, "lava")
{
    Init();
}

cLava::cLava(XmlAttributes& attributes, cSprite_Manager* p_sprite_manager)
    : cMovingSprite(p_sprite_manager, "lava")
{
    Init();
    Set_Pos(attributes.fetch<float>("posx", 0), attributes.fetch<float>("posy", 0), true);
}

cLava* cLava::Copy() const
{
    cLava* p_lava = new cLava(m_sprite_manager);
    p_lava->Set_Pos(m_start_pos_x, m_start_pos_y);
    return p_lava;
}

cLava::~cLava()
{
    //
}

void cLava::Init()
{
    m_type = TYPE_ACTIVE_SPRITE;
    m_sprite_array = ARRAY_LAVA;
    m_massive_type = MASS_MASSIVE;
    m_name = _("Lava");

    m_can_be_ground = true;
    Set_Scale_Directions(1, 1, 1, 1);

    Add_Image_Set("main", "lava/red/red.imgset");
    Set_Image_Set("main", true);
}

void cLava::Update()
{
    Update_Animation();
}

void cLava::Draw(cSurface_Request* p_request /* = NULL */)
{
    if (!m_valid_draw)
        return;

    cMovingSprite::Draw(p_request);
}

void cLava::Handle_Collision_Player(cObjectCollision* p_collision)
{
    if (pLevel_Player->m_god_mode)
        pLevel_Player->m_vely = -35.0f; // Same as in cLevel_Player::Handle_out_of_Level()
    else {
        // Lava collision means instant death, even for invincible Alex
        // (i.e. star or hurt short-term invincibility).
        pLevel_Player->DownGrade_Player(true, true, true);
    }
}

void cLava::Handle_Collision_Enemy(cObjectCollision* p_collision)
{
    cEnemy* p_enemy = static_cast<cEnemy*>(p_collision->m_obj);

    // Lava collision means instant death.
    p_enemy->DownGrade(true);
}

xmlpp::Element* cLava::Save_To_XML_Node(xmlpp::Element* p_element)
{
    xmlpp::Element* p_node = cMovingSprite::Save_To_XML_Node(p_element);

    // No configuration currently

    return p_node;
}

std::string cLava::Get_XML_Type_Name()
{
    return "lava";
}

void cLava::Set_Massive_Type(MassiveType type)
{
    // Ignore to prevent "m" toggling in level editor
}

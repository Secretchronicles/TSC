/***************************************************************************
 * crate.cpp
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

#include "crate.hpp"
#include "../core/xml_attributes.hpp"
#include "../core/sprite_manager.hpp"
#include "../core/i18n.hpp"
#include "../core/game_core.hpp"
#include "../audio/audio.hpp"
#include "../video/animation.hpp"
#include "../level/level_player.hpp"
#include "../user/savegame/savegame.hpp"
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

    Clear_Images();
    Add_Image_Set("main", "blocks/extra/crate.imgset");
    Set_Image_Set("main", true);
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

void cCrate::Update()
{
    if (!m_valid_update || !Is_In_Range())
        return;
    if (m_crate_state == CRATE_DEAD)
        return;

    cMovingSprite::Update();

    // cMovingSprite::Update does not call cSprite::Update, so update animation directly
    Update_Animation();

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

void cCrate::Handle_out_of_Level(ObjectDirection dir)
{
    // abyss
    if (dir == DIR_BOTTOM) {
        m_crate_state = CRATE_DEAD;
        m_massive_type = MASS_PASSIVE;
        Set_Active(false);
    }

    // Don’t move it outside right/left level edge
    if (dir == DIR_LEFT)
        Set_Pos_X(pActive_Camera->m_limit_rect.m_x - m_col_pos.m_x);
    else if (dir == DIR_RIGHT)
        Set_Pos_X(pActive_Camera->m_limit_rect.m_x + pActive_Camera->m_limit_rect.m_w - m_col_pos.m_x - m_col_rect.m_w - 0.01f);

}

void cCrate::Smash()
{
    m_crate_state = CRATE_DEAD;
    m_massive_type = MASS_PASSIVE;
    Set_Active(false);
    pAudio->Play_Sound("stomp_1.ogg");
    Smash_Animation();

    // Any objects standing on this crate need to fall down now.
    cObjectCollisionType* col_list = Collision_Check_Relative(0.0f, -1.0, 0.0f, 1.0f, COLLIDE_ONLY_BLOCKING);
    for(cObjectCollision* p_col: col_list->objects) {
        static_cast<cMovingSprite*>(p_col->m_obj)->Reset_On_Ground();
    }
}

void cCrate::Smash_Animation()
{
    cParticle_Emitter* p_em = new cParticle_Emitter(m_sprite_manager);
    p_em->Set_Emitter_Rect(m_col_rect);
    p_em->Set_Quota(4);
    p_em->Set_Pos_Z(cSprite::m_pos_z_front_passive_start + 0.01f);
    p_em->Set_Image(pVideo->Get_Surface("animation/particles/axis.png"));
    p_em->Set_Color(orange);
    p_em->Set_Time_to_Live(10.0f);
    p_em->Set_Scale(0.5f);
    p_em->Set_Const_Rotation_Z(20.0f, 5.0f);
    p_em->Set_Vertical_Gravity(2.5f, 0.5f);
    p_em->Set_Horizontal_Gravity(1.0f, 0.5f);
    p_em->Set_Emitter_Time_to_Live(0.0f); // Only emit exactly...
    p_em->Emit();                         // ...a single time.

    p_em->Set_Horizontal_Gravity(-1.0f, -0.5f);
    p_em->Emit();
    pActive_Animation_Manager->Add(p_em);
}

/***************************************************************************
 * goldpiece.cpp  -  goldpiece class
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

#include "../objects/goldpiece.hpp"
#include "../core/game_core.hpp"
#include "../level/level_player.hpp"
#include "../audio/audio.hpp"
#include "../core/framerate.hpp"
#include "../video/animation.hpp"
#include "../gui/hud.hpp"
#include "../user/savegame/savegame.hpp"
#include "../core/math/utilities.hpp"
#include "../core/i18n.hpp"
#include "../level/level.hpp"
#include "../scripting/events/activate_event.hpp"

namespace TSC {

/* *** *** *** *** *** *** cGoldpiece *** *** *** *** *** *** *** *** *** *** *** */

cGoldpiece::cGoldpiece(cSprite_Manager* sprite_manager, DefaultColor color /* = COL_YELLOW */)
    : cMovingSprite(sprite_manager, "item")
{
    cGoldpiece::Init(color);
}

cGoldpiece::cGoldpiece(XmlAttributes& attributes, cSprite_Manager* sprite_manager)
    : cMovingSprite(sprite_manager, "item")
{
    cGoldpiece::Init();

    // position
    Set_Pos(string_to_float(attributes["posx"]), string_to_float(attributes["posy"]), true);
    // gold color
    Set_Gold_Color(Get_Color_Id(attributes.fetch("color", Get_Color_Name(m_color_type))));
}

cGoldpiece::~cGoldpiece(void)
{
    //
}

void cGoldpiece::Init(DefaultColor color /* = COL_YELLOW */)
{
    m_sprite_array = ARRAY_ACTIVE;
    m_massive_type = MASS_PASSIVE;
    m_type = TYPE_GOLDPIECE;
    m_pos_z = 0.041f;
    m_can_be_on_ground = 0;

    Set_Gold_Color(color);
}

cGoldpiece* cGoldpiece::Copy(void) const
{
    cGoldpiece* goldpiece = new cGoldpiece(m_sprite_manager);
    goldpiece->Set_Pos(m_start_pos_x, m_start_pos_y, 1);
    goldpiece->Set_Gold_Color(m_color_type);
    return goldpiece;
}

std::string cGoldpiece::Get_XML_Type_Name()
{
    return "goldpiece";
}

xmlpp::Element* cGoldpiece::Save_To_XML_Node(xmlpp::Element* p_element)
{
    xmlpp::Element* p_node = cMovingSprite::Save_To_XML_Node(p_element);

    // color
    Add_Property(p_node, "color", Get_Color_Name(m_color_type));

    return p_node;
}

void cGoldpiece::Load_From_Savegame(cSave_Level_Object* save_object)
{
    cMovingSprite::Load_From_Savegame(save_object);
}

void cGoldpiece::Set_Gold_Color(DefaultColor color)
{
    m_color_type = color;

    // clear images
    Clear_Images();

    if (m_type == TYPE_FALLING_GOLDPIECE) {
        if (m_color_type == COL_RED) {
            Add_Image_Set("main", "game/items/goldpiece/red/falling.imgset");

            m_name = _("Red Falling Jewel");
        }
        // default is yellow
        else {
            Add_Image_Set("main", "game/items/goldpiece/yellow/falling.imgset");

            m_name = _("Falling Jewel");
        }
    }
    else {
        if (m_color_type == COL_RED) {
            Add_Image_Set("main", "game/items/goldpiece/red/jewel.imgset");

            m_name = _("Red Jewel");
        }
        // default is yellow
        else {
            Add_Image_Set("main", "game/items/goldpiece/yellow/jewel.imgset");

            m_name = _("Jewel");
        }
    }

    Set_Image_Set("main", 1);
    if (m_type == TYPE_JUMPING_GOLDPIECE || m_type == TYPE_FALLING_GOLDPIECE) {
        Set_Animation_Speed(1.143);
    }
    else {
        Set_Animation_Speed(1.0);
    }
}

void cGoldpiece::Activate(void)
{
    if (!m_active) {
        return;
    }

    // animation
    cAnimation_Goldpiece* anim = new cAnimation_Goldpiece(m_sprite_manager, m_pos_x + (m_col_rect.m_w / 10), m_pos_y + (m_col_rect.m_h / 10));

    // gold
    unsigned int points = 0;

    if (m_color_type == COL_RED) {
        gp_hud->Add_Jewels(5);
        points = 100;

        anim->Set_Scale(1.2f, 1);
    }
    else {
        gp_hud->Add_Jewels(1);
        points = 5;
    }

    pActive_Animation_Manager->Add(anim);

    // if jumping double the points
    if (m_type == TYPE_JUMPING_GOLDPIECE) {
        points *= 2;
    }
    else {
        if (m_color_type == COL_RED) {
            pAudio->Play_Sound("item/jewel_2.ogg");
        }
        else {
            pAudio->Play_Sound("item/jewel_1.ogg");
        }
    }

    gp_hud->Add_Points(points, m_pos_x + m_col_rect.m_w / 2, m_pos_y + 2);

    Scripting::cActivate_Event evt;
    evt.Fire(pActive_Level->m_mruby, this);

    // if spawned destroy
    if (m_spawned) {
        Destroy();
    }
    // hide
    else {
        Set_Active(0);
    }
}

void cGoldpiece::Update(void)
{
    if (!m_valid_update || !Is_Visible_On_Screen()) {
        return;
    }

    Update_Animation();
}

void cGoldpiece::Draw(cSurface_Request* request /* = NULL */)
{
    if (!m_valid_draw) {
        return;
    }

    // don't draw in leveleditor if spawned ingame
    if (editor_level_enabled && m_spawned) {
        return;
    }

    cMovingSprite::Draw(request);
}

bool cGoldpiece::Is_Update_Valid()
{
    // if not visible
    if (!m_active) {
        return 0;
    }

    return 1;
}

void cGoldpiece::Handle_Collision_Player(cObjectCollision* collision)
{
    // invalid
    if (collision->m_direction == DIR_UNDEFINED) {
        return;
    }

    Activate();
}

void cGoldpiece::Handle_Collision_Lava(cObjectCollision* collision)
{
    Set_Active(false);
}

/* *** *** *** *** *** *** cJGoldpiecee *** *** *** *** *** *** *** *** *** *** *** */

cJGoldpiece::cJGoldpiece(cSprite_Manager* sprite_manager, DefaultColor color /* = COL_YELLOW */)
        : cGoldpiece(sprite_manager, color)
{
    m_type = TYPE_JUMPING_GOLDPIECE;
    Set_Spawned(1);

    m_vely = -18.0f;
}

cJGoldpiece::~cJGoldpiece(void)
{
    //
}

void cJGoldpiece::Update(void)
{
    if (!m_active) {
        return;
    }

    Update_Animation();

    // add velocity downwards
    if (m_vely < 8.0f) {
        Add_Velocity_Y_Max(1.62f, 8.0f);
    }
    // finished animation
    else {
        Activate();
    }
}

Col_Valid_Type cJGoldpiece::Validate_Collision(cSprite* obj)
{
    return COL_VTYPE_NOT_VALID;
}

/* *** *** *** *** *** *** cFGoldpiecee *** *** *** *** *** *** *** *** *** *** *** */

cFGoldpiece::cFGoldpiece(cSprite_Manager* sprite_manager, ObjectDirection dir /* = DIR_NOTHING */, DefaultColor color /* = COL_YELLOW */)
        : cGoldpiece(sprite_manager, color)
{
    m_type = TYPE_FALLING_GOLDPIECE;
    m_camera_range = 2000;
    m_gravity_max = 25.0f;
    m_can_be_on_ground = 1;

    // direction set
    if (dir == DIR_LEFT || dir == DIR_RIGHT) {
        m_direction = dir;
    }
    // set a random direction
    else {
        if (rand() % 2 != 1) {
            m_direction = DIR_LEFT;
        }
        else {
            m_direction = DIR_RIGHT;
        }
    }

    if (m_direction == DIR_RIGHT) {
        m_velx = 5.0f;
    }
    else {
        m_velx = -5.0f;
    }
}

cFGoldpiece::~cFGoldpiece(void)
{
    //
}

void cFGoldpiece::Update(void)
{
    if (!m_valid_update || !Is_In_Range()) {
        return;
    }

    Update_Animation();

    // Add Gravitation
    if (!m_ground_object && m_vely < m_gravity_max) {
        Add_Velocity_Y_Max(1.2f, m_gravity_max);
    }
}

Col_Valid_Type cFGoldpiece::Validate_Collision(cSprite* obj)
{
    // basic validation checking
    Col_Valid_Type basic_valid = Validate_Collision_Ghost(obj);

    // found valid collision
    if (basic_valid != COL_VTYPE_NOT_POSSIBLE) {
        return basic_valid;
    }

    switch (obj->m_type) {
    case TYPE_PLAYER: {
        return COL_VTYPE_INTERNAL;
    }
    case TYPE_BALL: {
        return COL_VTYPE_NOT_VALID;
    }
    default: {
        break;
    }
    }

    if (obj->m_massive_type == MASS_MASSIVE) {
        if (obj->m_sprite_array == ARRAY_ENEMY) {
            return COL_VTYPE_NOT_VALID;
        }

        return COL_VTYPE_BLOCKING;
    }
    if (obj->m_massive_type == MASS_HALFMASSIVE) {
        // if moving downwards and the object is on bottom
        if (m_vely >= 0.0f && Is_On_Top(obj)) {
            return COL_VTYPE_BLOCKING;
        }
    }

    return COL_VTYPE_NOT_VALID;
}

void cFGoldpiece::Handle_Collision_Massive(cObjectCollision* collision)
{
    if (collision->m_direction == DIR_RIGHT || collision->m_direction == DIR_LEFT) {
        Turn_Around(collision->m_direction);
    }
    else if (collision->m_direction == DIR_UP) {
        m_vely = -(m_vely * 0.3f);
    }
    else if (collision->m_direction == DIR_DOWN) {
        // minimal value for a jump
        if (m_vely > 0.5f) {
            m_vely = -(m_vely * 0.5f);

            // maximum value for a jump
            if (m_vely > 10.0f) {
                m_vely = 10.0f;
            }
        }
        else {
            m_vely = 0.0f;
        }
    }
}

void cFGoldpiece::Handle_Collision_Box(ObjectDirection cdirection, GL_rect* r2)
{
    // if unsupported collision direction
    if (cdirection != DIR_DOWN && cdirection != DIR_LEFT && cdirection != DIR_RIGHT) {
        return;
    }

    if (cdirection == DIR_DOWN) {
        m_vely = -30.0f;

        // left
        if (m_pos_x > r2->m_x && m_velx < 0.0f) {
            Turn_Around(DIR_LEFT);
        }
        // right
        else if (m_pos_x < r2->m_x && m_velx > 0.0f) {
            Turn_Around(DIR_RIGHT);
        }
    }
    else if (cdirection == DIR_LEFT || cdirection == DIR_RIGHT) {
        m_vely = -13.0f;
        Turn_Around(cdirection);
    }

    Reset_On_Ground();
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

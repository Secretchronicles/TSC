/***************************************************************************
 * spika.cpp  -  spika, spiked waiting enemy
 *
 * Copyright © 2006 - 2011 Florian Richter
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

#include "../enemies/spika.hpp"
#include "../core/game_core.hpp"
#include "../level/level_player.hpp"
#include "../level/level.hpp"
#include "../gui/hud.hpp"
#include "../video/gl_surface.hpp"
#include "../core/sprite_manager.hpp"
#include "../core/i18n.hpp"
#include "../core/xml_attributes.hpp"
#include "../enemies/bosses/turtle_boss.hpp"

namespace TSC {

/* *** *** *** *** *** *** cSpika *** *** *** *** *** *** *** *** *** *** *** */

cSpika::cSpika(cSprite_Manager* sprite_manager)
    : cEnemy(sprite_manager)
{
    cSpika::Init();
}

cSpika::cSpika(XmlAttributes& attributes, cSprite_Manager* sprite_manager)
    : cEnemy(sprite_manager)
{
    cSpika::Init();

    // position
    Set_Pos(string_to_float(attributes["posx"]), string_to_float(attributes["posy"]), true);

    // color
    Set_Color(static_cast<DefaultColor>(Get_Color_Id(attributes.fetch("color", Get_Color_Name(m_color_type)))));
}


cSpika::~cSpika(void)
{
    //
}

void cSpika::Init(void)
{
    m_type = TYPE_SPIKA;
    m_name = "Spika";
    m_pos_z = 0.09f;
    m_gravity_max = 25.0f;

    m_speed = 0.0f;
    m_detection_size = 0.0f;
    m_walk_count = 0.0f;

    m_color_type = COL_DEFAULT;
    Set_Color(COL_ORANGE);
}

cSpika* cSpika::Copy(void) const
{
    cSpika* spika = new cSpika(m_sprite_manager);
    spika->Set_Pos(m_start_pos_x, m_start_pos_y, 1);
    spika->Set_Color(m_color_type);
    return spika;
}

std::string cSpika::Get_XML_Type_Name()
{
    return "spika";
}

xmlpp::Element* cSpika::Save_To_XML_Node(xmlpp::Element* p_element)
{
    xmlpp::Element* p_node = cEnemy::Save_To_XML_Node(p_element);

    Add_Property(p_node, "color", Get_Color_Name(m_color_type));

    return p_node;
}

void cSpika::Set_Color(DefaultColor col)
{
    // already set
    if (m_color_type == col) {
        return;
    }

    // clear old images
    Clear_Images();

    m_color_type = col;

    if (m_color_type == COL_ORANGE) {
        Add_Image(pVideo->Get_Surface("enemy/spika/orange.png"));

        m_speed = 3;
        m_detection_size = 160.0f;
        m_kill_points = 50;

        m_fire_resistant = 0;
        m_ice_resistance = 0.0f;
    }
    else if (m_color_type == COL_GREEN) {
        Add_Image(pVideo->Get_Surface("enemy/spika/green.png"));

        m_speed = 4;
        m_detection_size = 220.0f;
        m_kill_points = 200;

        m_fire_resistant = 0;
        m_ice_resistance = 0.1f;
    }
    else if (m_color_type == COL_GREY) {
        Add_Image(pVideo->Get_Surface("enemy/spika/grey.png"));

        m_speed = 7;
        m_detection_size = 330.0f;
        m_kill_points = 500;

        m_fire_resistant = 1;
        m_ice_resistance = 0.5f;
    }
    else if (m_color_type == COL_RED) {
        Add_Image(pVideo->Get_Surface("enemy/spika/red.png"));

        m_speed = 10;
        m_detection_size = 300.0f;
        m_kill_points = 550;

        m_fire_resistant = 1;
        m_ice_resistance = 1.0f;
    }
    else {
        printf("Error : Unknown Spika Color %d\n", m_color_type);
    }

    Set_Image_Num(0, 1);
}

void cSpika::DownGrade(bool force /* = 0 */)
{
    Set_Dead(1);
    m_massive_type = MASS_PASSIVE;
    m_counter = 0.0f;
    m_velx = 0.0f;
    m_vely = 0.0f;
    Set_Scale_Directions(1, 1, 1, 1);

    // default stomp death
    if (!force) {
        Generate_Hit_Animation();
    }
    // falling death
    else {
        Set_Rotation_Z(180.0f);
    }
}

void cSpika::Update(void)
{
    cEnemy::Update();

    if (!m_valid_update || !Is_In_Range()) {
        return;
    }

    // update rotation
    if (m_velx != 0) {
        Add_Rotation_Z((m_velx / (m_image->m_w * 0.01f)) * pFramerate->m_speed_factor);
    }

    // check for player
    GL_rect player_rect = pLevel_Player->m_col_rect;
    player_rect.m_x += (pLevel_Player->m_col_rect.m_w / 2);
    player_rect.m_w = 1;

    // rect
    GL_rect rect_left = m_col_rect;
    rect_left.m_y -= 10;
    rect_left.m_h += 10;

    GL_rect rect_right = rect_left;
    // left
    rect_left.m_x -= m_detection_size + (m_col_rect.m_w / 2);
    rect_left.m_w += m_detection_size;
    // right
    rect_right.m_x -= m_col_rect.m_w / 2;
    rect_right.m_w += m_detection_size;


    // if player is left
    if (pLevel_Player->m_maryo_type != MARYO_GHOST && player_rect.Intersects(rect_left)) {
        if (m_velx > -m_speed) {
            m_velx -= m_speed * 0.1f * pFramerate->m_speed_factor;

            if (m_velx < -m_speed) {
                m_velx = -m_speed;
            }
        }
    }
    // if player is right
    else if (pLevel_Player->m_maryo_type != MARYO_GHOST && player_rect.Intersects(rect_right)) {
        if (m_velx < m_speed) {
            m_velx += m_speed * 0.1f * pFramerate->m_speed_factor;

            if (m_velx > m_speed) {
                m_velx = m_speed;
            }
        }
    }
    // out of range
    else {
        // slow down
        m_velx -= (m_velx * 0.03f) * pFramerate->m_speed_factor;
    }

    // play walking sound based on speed
    if (m_walk_count < m_rot_z - 30.0f || m_walk_count > m_rot_z + 30.0f) {
        pAudio->Play_Sound("enemy/spika/move.ogg");

        m_walk_count = m_rot_z;
    }
}

Col_Valid_Type cSpika::Validate_Collision(cSprite* obj)
{
    // basic validation checking
    Col_Valid_Type basic_valid = Validate_Collision_Ghost(obj);

    // found valid collision
    if (basic_valid != COL_VTYPE_NOT_POSSIBLE) {
        return basic_valid;
    }

    if (obj->m_massive_type == MASS_MASSIVE) {
        switch (obj->m_type) {
        case TYPE_ROKKO: {
            return COL_VTYPE_NOT_VALID;
        }
        case TYPE_GEE: {
            return COL_VTYPE_NOT_VALID;
        }
        case TYPE_TURTLE_BOSS: {
            return COL_VTYPE_NOT_VALID;
        }
        case TYPE_FURBALL_BOSS: {
            return COL_VTYPE_NOT_VALID;
        }
        case TYPE_STATIC_ENEMY: {
            return COL_VTYPE_NOT_VALID;
        }
        default: {
            break;
        }
        }

        if (obj->m_sprite_array == ARRAY_ENEMY) {
            // collide only if moving
            if (!Is_Float_Equal(m_velx, 0.0f)) {
                // if enemy is spika and more powerful
                if (obj->m_type == TYPE_SPIKA && m_speed < static_cast<cSpika*>(obj)->m_speed) {
                    return COL_VTYPE_BLOCKING;
                }

                return COL_VTYPE_INTERNAL;
            }

            return COL_VTYPE_NOT_VALID;
        }

        return COL_VTYPE_BLOCKING;
    }
    else if (obj->m_massive_type == MASS_HALFMASSIVE) {
        // if moving downwards and the object is on bottom
        if (m_vely >= 0 && Is_On_Top(obj)) {
            return COL_VTYPE_BLOCKING;
        }
    }

    return COL_VTYPE_NOT_VALID;
}

void cSpika::Handle_Collision_Player(cObjectCollision* collision)
{
    pLevel_Player->DownGrade_Player();

    if (collision->m_direction == DIR_LEFT || collision->m_direction == DIR_RIGHT) {
        m_velx = 0.0f;
    }
}

void cSpika::Handle_Collision_Enemy(cObjectCollision* collision)
{
    // invalid
    if (collision->m_number < 0) {
        return;
    }

    cEnemy* enemy = static_cast<cEnemy*>(m_sprite_manager->Get_Pointer(collision->m_number));

    // already dead
    if (enemy->m_dead) {
        return;
    }

    // if spika
    if (enemy->m_type == TYPE_SPIKA) {
        cSpika* spika = static_cast<cSpika*>(enemy);

        // if colliding spika is more powerful
        if (m_speed < spika->m_speed) {
            //enemy->Send_Collision( collision );
            return;
        }
    }

    // state change for turtle boss
    if (enemy->m_type == TYPE_TURTLE_BOSS) {
        // todo : remove hackiness and implement a better generic downgrade handler
        cTurtleBoss* turtle_boss = static_cast<cTurtleBoss*>(enemy);

        // downgrade until state change
        for (int i = turtle_boss->m_hits; i < turtle_boss->m_max_hits; i++) {
            turtle_boss->DownGrade();
        }

        // boss kills spika
        DownGrade(1);
    }
    // state change for furball boss
    else if (enemy->m_type == TYPE_FURBALL_BOSS) {
        enemy->DownGrade();
        // also hits us
        DownGrade(1);
    }
    // enemies that also hit us
    else if ((enemy->m_type == TYPE_TURTLE && enemy->m_state == STA_RUN)) {
        enemy->DownGrade(1);
        // also hits us
        DownGrade(1);
    }
    // enemies that only hit us
    else if (enemy->m_type == TYPE_SPIKEBALL) {
        // hits us
        DownGrade(1);
    }
    // kill enemy
    else {
        // only if moving
        if (Is_Float_Equal(m_velx, 0.0f) && Is_Float_Equal(m_vely, 0.0f)) {
            return;
        }

        pAudio->Play_Sound(enemy->m_kill_sound);
        pHud_Points->Add_Points(enemy->m_kill_points, enemy->m_pos_x, enemy->m_pos_y - 5.0f, "", static_cast<Uint8>(255), 1);
        enemy->DownGrade(1);
    }
}

void cSpika::Handle_Collision_Massive(cObjectCollision* collision)
{
    if (m_state == STA_OBJ_LINKED) {
        return;
    }

    Send_Collision(collision);

    // get colliding object
    cSprite* col_object = m_sprite_manager->Get_Pointer(collision->m_number);

    if (col_object->m_type == TYPE_BALL) {
        return;
    }

    if (collision->m_direction == DIR_TOP) {
        if (m_vely < 0.0f) {
            m_vely = 0.0f;
        }
    }
    else if (collision->m_direction == DIR_BOTTOM) {
        if (m_vely > 0.0f) {
            m_vely = 0.0f;
        }
    }
    else if (collision->m_direction == DIR_RIGHT || collision->m_direction == DIR_LEFT) {
        m_velx = 0.0f;
    }
}

void cSpika::Handle_Collision_Box(ObjectDirection cdirection, GL_rect* r2)
{
    if (cdirection == DIR_DOWN) {
        m_vely = -10.0f;

        // left
        if (m_pos_x > r2->m_x) {
            m_velx += 10.0f;
        }
        // right
        else if (m_pos_x < r2->m_x) {
            m_velx -= 10.0f;
        }

        Reset_On_Ground();
    }
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

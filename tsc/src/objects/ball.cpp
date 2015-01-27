/***************************************************************************
 * ball.cpp  -  ball class
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

#include "../objects/ball.hpp"
#include "../core/game_core.hpp"
#include "../enemies/enemy.hpp"
#include "../objects/goldpiece.hpp"
#include "../enemies/gee.hpp"
#include "../enemies/spika.hpp"
#include "../level/level_player.hpp"
#include "../video/animation.hpp"
#include "../level/level.hpp"
#include "../gui/hud.hpp"
#include "../core/sprite_manager.hpp"
#include "../user/savegame.hpp"
#include "../core/global_basic.hpp"

using namespace std;

namespace TSC {

/* *** *** *** *** *** *** cBall *** *** *** *** *** *** *** *** *** *** *** */

cBall::cBall(cSprite_Manager* sprite_manager)
    : cMovingSprite(sprite_manager, "ball")
{
    cBall::Init();
}

cBall::cBall(XmlAttributes& attributes, cSprite_Manager* sprite_manager)
    : cMovingSprite(sprite_manager, "ball")
{
    cBall::Init();

    // position
    Set_Pos(string_to_float(attributes["posx"]), string_to_float(attributes["posy"]), true);

    // direction
    m_direction = static_cast<ObjectDirection>(string_to_int(attributes["direction"]));

    // origin array and type
    Set_Origin(static_cast<ArrayType>(string_to_int(attributes["origin_array"])),
               static_cast<SpriteType>(string_to_int(attributes["origin_type"])));

    // type
    Set_Ball_Type(static_cast<ball_effect>(string_to_int(attributes["ball_type"])));
}


cBall::~cBall(void)
{
    // always destroy
    if (!m_auto_destroy) {
        cBall::Destroy();
    }
}

void cBall::Init(void)
{
    m_sprite_array = ARRAY_ACTIVE;
    m_type = TYPE_BALL;
    m_pos_z = 0.095f;
    m_gravity_max = 20.0f;

    Set_Spawned(1);
    m_camera_range = 2000;

    m_massive_type = MASS_MASSIVE;

    m_glim_mod = 0.1f;
    m_glim_counter = 0.0f;
    m_fire_counter = 0.0f;

    Set_Origin(ARRAY_UNDEFINED, TYPE_UNDEFINED);
    Set_Ball_Type(FIREBALL_DEFAULT);
}

cBall* cBall::Copy(void) const
{
    cBall* ball = new cBall(m_sprite_manager);
    ball->Set_Pos(m_start_pos_x, m_start_pos_y, 1);
    ball->Set_Origin(m_origin_array, m_origin_type);
    ball->Set_Ball_Type(m_ball_type);
    return ball;
}

std::string cBall::Get_XML_Type_Name()
{
    return "";
}

xmlpp::Element* cBall::Save_To_XML_Node(xmlpp::Element* p_element)
{
    xmlpp::Element* p_node = cMovingSprite::Save_To_XML_Node(p_element);

    // direction
    Add_Property(p_node, "direction", m_direction);
    // origin array and type
    Add_Property(p_node, "origin_array", m_origin_array);
    Add_Property(p_node, "origin_type", m_origin_type);
    // type
    Add_Property(p_node, "ball_type", m_ball_type);

    return p_node;
}

void cBall::Load_From_Savegame(cSave_Level_Object* save_object)
{
    // new position x
    if (save_object->exists("new_posx")) {
        Set_Pos_X(string_to_float(save_object->Get_Value("new_posx")));
    }

    // new position y
    if (save_object->exists("new_posy")) {
        Set_Pos_Y(string_to_float(save_object->Get_Value("new_posy")));
    }

    // direction
    if (save_object->exists("direction")) {
        m_direction = static_cast<ObjectDirection>(string_to_int(save_object->Get_Value("direction")));
    }

    // velocity x
    if (save_object->exists("velx")) {
        m_velx = string_to_float(save_object->Get_Value("velx"));
    }

    // velocity y
    if (save_object->exists("vely")) {
        m_vely = string_to_float(save_object->Get_Value("vely"));
    }
}

cSave_Level_Object* cBall::Save_To_Savegame(void)
{
    cSave_Level_Object* save_object = new cSave_Level_Object();

    // default values
    save_object->m_type = m_type;
    save_object->m_properties.push_back(cSave_Level_Object_Property("posx", int_to_string(static_cast<int>(m_start_pos_x))));
    save_object->m_properties.push_back(cSave_Level_Object_Property("posy", int_to_string(static_cast<int>(m_start_pos_y))));

    // new position
    save_object->m_properties.push_back(cSave_Level_Object_Property("new_posx", int_to_string(static_cast<int>(m_pos_x))));
    save_object->m_properties.push_back(cSave_Level_Object_Property("new_posy", int_to_string(static_cast<int>(m_pos_y))));

    // direction
    save_object->m_properties.push_back(cSave_Level_Object_Property("direction", int_to_string(m_direction)));

    // velocity
    save_object->m_properties.push_back(cSave_Level_Object_Property("velx", float_to_string(m_velx)));
    save_object->m_properties.push_back(cSave_Level_Object_Property("vely", float_to_string(m_vely)));

    return save_object;
}

void cBall::Set_Ball_Type(ball_effect type)
{
    Clear_Images();

    if (type == FIREBALL_DEFAULT || type == FIREBALL_EXPLOSION) {
        Set_Image(pVideo->Get_Package_Surface("animation/fireball/1.png"));
        m_ball_type = FIREBALL_DEFAULT;
    }
    else if (type == ICEBALL_DEFAULT || type == ICEBALL_EXPLOSION) {
        Set_Image(pVideo->Get_Package_Surface("animation/iceball/1.png"));
        m_ball_type = ICEBALL_DEFAULT;
    }
    else {
        cerr << "Warning : Ball unknown type " << type << endl;
        cMovingSprite::Destroy();
    }
}

void cBall::Set_Origin(ArrayType origin_array, SpriteType origin_type)
{
    m_origin_array = origin_array;
    m_origin_type = origin_type;
}

void cBall::Destroy_Ball(bool with_sound /* = 0 */)
{
    if (with_sound) {
        if (m_ball_type == FIREBALL_DEFAULT) {
            pAudio->Play_Sound("item/fireball_explode.wav");
        }
    }

    Destroy();
}

void cBall::Destroy(void)
{
    if (m_auto_destroy) {
        return;
    }

    if (m_ball_type == FIREBALL_DEFAULT) {
        pActive_Animation_Manager->Add(new cAnimation_Fireball(m_sprite_manager, m_pos_x + m_col_rect.m_w / 2, m_pos_y + m_col_rect.m_h / 2));
    }
    else if (m_ball_type == ICEBALL_DEFAULT) {
        // create animation
        cParticle_Emitter* anim = new cParticle_Emitter(m_sprite_manager);
        Generate_Particles(anim);
        anim->Set_Quota(15);
        anim->Emit();
        pActive_Animation_Manager->Add(anim);
    }

    cMovingSprite::Destroy();
}

void cBall::Update(void)
{
    if (!m_valid_update) {
        return;
    }

    // if this is out of range
    if (!Is_In_Range()) {
        Destroy();
    }

    Update_Animation();

    // right
    if (m_velx > 0.0f) {
        m_rot_z += pFramerate->m_speed_factor * 40;
    }
    // left
    else {
        m_rot_z -= pFramerate->m_speed_factor * 40;
    }

    if (m_vely < m_gravity_max) {
        Add_Velocity_Y_Max(1.0f, m_gravity_max);
    }

    // glim animation
    m_glim_counter += pFramerate->m_speed_factor * m_glim_mod;

    // glim animation
    if (m_glim_mod > 0.0f) {
        if (m_glim_mod < 0.05f) {
            m_glim_mod += pFramerate->m_speed_factor * 0.1f;
        }

        if (m_glim_counter > 1.0f) {
            m_glim_counter = 1.0f;
            m_glim_mod = -0.01f;
        }
    }
    else {
        if (m_glim_mod > -0.05f) {
            m_glim_mod -= pFramerate->m_speed_factor * 0.1f;
        }

        if (m_glim_counter < 0.0f) {
            m_glim_counter = 0.0f;
            m_glim_mod = 0.01f;
        }
    }

    // generate fire particle animation
    m_fire_counter += pFramerate->m_speed_factor;
    while (m_fire_counter > 1) {
        Generate_Particles();
        m_fire_counter -= 1;
    }
}

void cBall::Draw(cSurface_Request* request /* = NULL */)
{
    if (!m_valid_draw) {
        return;
    }

    // don't draw if leveleditor mode
    if (editor_level_enabled) {
        return;
    }

    if (m_ball_type == FIREBALL_DEFAULT) {
        Set_Color_Combine(m_glim_counter, m_glim_counter / 1.6f, m_glim_counter / 3, GL_ADD);
    }
    else if (m_ball_type == ICEBALL_DEFAULT) {
        Set_Color_Combine(m_glim_counter / 6.0f, m_glim_counter / 6.0f, m_glim_counter / 6.0f, GL_ADD);
    }

    cMovingSprite::Draw(request);
}

void cBall::Generate_Particles(cParticle_Emitter* anim /* = NULL */) const
{
    bool create_anim = 0;

    if (!anim) {
        create_anim = 1;
        // create animation
        anim = new cParticle_Emitter(m_sprite_manager);
    }

    anim->Set_Emitter_Rect(m_col_rect);
    anim->Set_Pos_Z(m_pos_z + 0.0001f);
    if (m_ball_type == FIREBALL_DEFAULT) {
        unsigned int rand_image = rand() % 3;

        if (rand_image == 0) {
            anim->Set_Image(pVideo->Get_Package_Surface("animation/particles/fire_1.png"));
        }
        else if (rand_image == 1) {
            anim->Set_Image(pVideo->Get_Package_Surface("animation/particles/fire_2.png"));
        }
        else {
            anim->Set_Image(pVideo->Get_Package_Surface("animation/particles/fire_4.png"));
        }

        anim->Set_Time_to_Live(0.2f);
        anim->Set_Color(Color(static_cast<Uint8>(150 + (m_glim_counter * 100)), 50, 50), Color(static_cast<Uint8>(0), 200, 200, 0));
    }
    // ice
    else {
        anim->Set_Image(pVideo->Get_Package_Surface("animation/particles/light.png"));
        anim->Set_Time_to_Live(0.5f);
        anim->Set_Color(Color(static_cast<Uint8>(90), 90, 255));
    }

    anim->Set_Blending(BLEND_ADD);
    anim->Set_Speed(0.35f, 0.3f);
    anim->Set_Scale(0.4f, 0.2f);

    if (create_anim) {
        anim->Emit();
        pActive_Animation_Manager->Add(anim);
    }
}

Col_Valid_Type cBall::Validate_Collision(cSprite* obj)
{
    // basic validation checking
    Col_Valid_Type basic_valid = Validate_Collision_Ghost(obj);

    // found valid collision
    if (basic_valid != COL_VTYPE_NOT_POSSIBLE) {
        return basic_valid;
    }

    switch (obj->m_type) {
    case TYPE_PLAYER: {
        if (m_origin_type != TYPE_PLAYER) {
            return COL_VTYPE_INTERNAL;
        }

        return COL_VTYPE_NOT_VALID;
    }
    case TYPE_BALL: {
        return COL_VTYPE_NOT_VALID;
    }
    default: {
        break;
    }
    }

    if (obj->m_massive_type == MASS_MASSIVE) {
        if (obj->m_sprite_array == ARRAY_ENEMY && m_origin_array == ARRAY_ENEMY) {
            return COL_VTYPE_NOT_VALID;
        }

        return COL_VTYPE_BLOCKING;
    }
    else if (obj->m_massive_type == MASS_HALFMASSIVE) {
        // if moving downwards and the object is on bottom
        if (m_vely >= 0.0f && Is_On_Top(obj)) {
            return COL_VTYPE_BLOCKING;
        }
    }

    return COL_VTYPE_NOT_VALID;
}

void cBall::Handle_Collision(cObjectCollision* collision)
{
    // already destroyed
    if (m_auto_destroy) {
        return;
    }

    cMovingSprite::Handle_Collision(collision);
}

void cBall::Handle_Collision_Player(cObjectCollision* collision)
{
    // velocity hit
    if (collision->m_direction == DIR_LEFT) {
        if (pLevel_Player->m_velx > 0.0f) {
            pLevel_Player->m_velx *= 0.3f;
        }
    }
    else if (collision->m_direction == DIR_RIGHT) {
        if (pLevel_Player->m_velx < 0.0f) {
            pLevel_Player->m_velx *= 0.3f;
        }
    }
    else if (collision->m_direction == DIR_UP) {
        if (pLevel_Player->m_vely > 0.0f) {
            pLevel_Player->m_vely *= 0.2f;
        }
    }
    else if (collision->m_direction == DIR_DOWN) {
        if (pLevel_Player->m_vely < 0.0f) {
            pLevel_Player->m_vely *= 0.4f;
        }
    }

    pAudio->Play_Sound("item/fireball_repelled.wav");
    Destroy();
}

void cBall::Handle_Collision_Enemy(cObjectCollision* collision)
{
    cEnemy* enemy = static_cast<cEnemy*>(m_sprite_manager->Get_Pointer(collision->m_number));

    // if enemy is not vulnerable
    if ((m_ball_type == FIREBALL_DEFAULT && enemy->m_fire_resistant) || (m_ball_type == ICEBALL_DEFAULT && enemy->m_ice_resistance >= 1)) {
        pAudio->Play_Sound("item/fireball_repelled.wav");
    }
    // make enemy handle the ball
    else {
        enemy->Handle_Ball_Hit(*this, collision);
    }

    Destroy();
}

void cBall::Handle_Collision_Massive(cObjectCollision* collision)
{
    if (collision->m_direction == DIR_DOWN) {
        // if directly hitting the ground
        if (m_velx < 0.1f && m_velx > -0.1f) {
            Destroy_Ball(1);
            return;
        }

        if (m_ball_type == FIREBALL_DEFAULT) {
            m_vely = -10.0f;

            // create animation
            cAnimation_Fireball* anim = new cAnimation_Fireball(m_sprite_manager, m_pos_x + m_col_rect.m_w / 2, m_pos_y + m_col_rect.m_h / 2);
            anim->Set_Fading_Speed(3.0f);
            pActive_Animation_Manager->Add(anim);
        }
        else if (m_ball_type == ICEBALL_DEFAULT) {
            m_vely = -5.0f;

            // create animation
            cParticle_Emitter* anim = new cParticle_Emitter(m_sprite_manager);
            anim->Set_Pos(m_pos_x + m_col_rect.m_w / 2, m_pos_y + m_col_rect.m_h / 2, 1);
            anim->Set_Image(pVideo->Get_Package_Surface("animation/particles/cloud.png"));
            anim->Set_Direction_Range(0, 180);
            anim->Set_Quota(3);
            anim->Set_Time_to_Live(0.8f);
            anim->Set_Pos_Z(m_pos_z + 0.0001f);
            anim->Set_Color(Color(static_cast<Uint8>(50), 50, 250));
            anim->Set_Blending(BLEND_ADD);
            anim->Set_Speed(0.5f, 0.4f);
            anim->Set_Scale(0.3f, 0.4f);
            anim->Emit();
            pActive_Animation_Manager->Add(anim);
        }
    }
    // other directions
    else {
        Destroy_Ball(1);
    }
}

void cBall::Handle_Collision_Lava(cObjectCollision* collision)
{
    // Fireballs jump when colliding with lava, ice balls melt.
    if (m_ball_type == FIREBALL_DEFAULT)
        m_vely = -25.0f;
    else
        Destroy_Ball(true);
}

void cBall::Handle_out_of_Level(ObjectDirection dir)
{
    // ignore top
    if (dir == DIR_TOP) {
        return;
    }

    Destroy_Ball(1);
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

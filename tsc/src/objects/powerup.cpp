/***************************************************************************
 * powerup.cpp  -  powerup classes
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

#include "../objects/powerup.hpp"
#include "../core/game_core.hpp"
#include "../level/level_player.hpp"
#include "../gui/hud.hpp"
#include "../core/framerate.hpp"
#include "../video/animation.hpp"
#include "../video/gl_surface.hpp"
#include "../user/savegame/savegame.hpp"
#include "../core/math/utilities.hpp"
#include "../core/i18n.hpp"
#include "../level/level.hpp"
#include "../scripting/events/activate_event.hpp"
#include "../core/global_basic.hpp"

using namespace std;

namespace TSC {

/* *** *** *** *** *** *** cPowerUp *** *** *** *** *** *** *** *** *** *** *** */

cPowerUp::cPowerUp(cSprite_Manager* sprite_manager)
    : cMovingSprite(sprite_manager, "item")
{
    m_sprite_array = ARRAY_ACTIVE;
    m_massive_type = MASS_PASSIVE;
    m_type = TYPE_POWERUP;
    m_pos_z = 0.05f;
    m_gravity_max = 25.0f;

    m_counter = 0;
}

cPowerUp::~cPowerUp(void)
{
    //
}

void cPowerUp::Load_From_Savegame(cSave_Level_Object* save_object)
{
    cMovingSprite::Load_From_Savegame(save_object);
}

void cPowerUp::Draw(cSurface_Request* request /* = NULL */)
{
    if (!m_valid_draw) {
        return;
    }

    // don't draw in editor mode if spawned
    if (m_spawned && editor_level_enabled) {
        return;
    }

    cMovingSprite::Draw(request);
}

bool cPowerUp::Is_Update_Valid(void)
{
    // if not visible
    if (!m_active) {
        return 0;
    }

    return 1;
}

void cPowerUp::Activate(void)
{
    if (!m_active)
        return;

    Scripting::cActivate_Event evt;
    evt.Fire(pActive_Level->m_mruby, this);
}

Col_Valid_Type cPowerUp::Validate_Collision(cSprite* obj)
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

void cPowerUp::Handle_out_of_Level(ObjectDirection dir)
{
    if (dir == DIR_LEFT) {
        m_pos_x = pActive_Camera->m_limit_rect.m_x;
    }
    else if (dir == DIR_RIGHT) {
        m_pos_x = pActive_Camera->m_limit_rect.m_x + pActive_Camera->m_limit_rect.m_w - m_col_pos.m_x - m_col_rect.m_w - 0.01f;
    }

    Turn_Around(dir);
}

void cPowerUp::Handle_Collision_Player(cObjectCollision* collision)
{
    // invalid
    if (collision->m_direction == DIR_UNDEFINED) {
        return;
    }

    Activate();
}

/* *** *** *** *** *** *** cMushroom *** *** *** *** *** *** *** *** *** *** *** */

cMushroom::cMushroom(cSprite_Manager* sprite_manager)
    : cPowerUp(sprite_manager)
{
    cMushroom::Init();
}

cMushroom::cMushroom(XmlAttributes& attributes, cSprite_Manager* sprite_manager)
    : cPowerUp(sprite_manager)
{
    cMushroom::Init();

    // position
    Set_Pos(string_to_float(attributes["posx"]), string_to_float(attributes["posy"]), true);
    // type
    Set_Type(static_cast<SpriteType>(attributes.fetch<int>("mushroom_type", TYPE_MUSHROOM_DEFAULT)));
}


cMushroom::~cMushroom(void)
{
    //
}

void cMushroom::Init(void)
{
    m_velx = 3.0f;
    m_direction = DIR_RIGHT;
    m_camera_range = 5000;

    m_type = TYPE_UNDEFINED;
    Set_Type(TYPE_MUSHROOM_DEFAULT);

    m_glim_mod = 1;
}

cMushroom* cMushroom::Copy(void) const
{
    cMushroom* mushroom = new cMushroom(m_sprite_manager);
    mushroom->Set_Pos(m_start_pos_x, m_start_pos_y, 1);
    mushroom->Set_Type(m_type);
    return mushroom;
}

std::string cMushroom::Get_XML_Type_Name()
{
    return "mushroom";
}

xmlpp::Element* cMushroom::Save_To_XML_Node(xmlpp::Element* p_element)
{
    xmlpp::Element* p_node = cPowerUp::Save_To_XML_Node(p_element);

    Add_Property(p_node, "mushroom_type", m_type);

    return p_node;
}

void cMushroom::Set_Type(SpriteType new_type)
{
    // already set
    if (m_type == new_type) {
        return;
    }

    Set_Color_Combine(0, 0, 0, 0);
    Clear_Images();

    if (new_type == TYPE_MUSHROOM_DEFAULT) {
        Add_Image_Set("main", "game/items/berry_big.imgset");
        m_name = _("Berry");
    }
    else if (new_type == TYPE_MUSHROOM_LIVE_1) {
        Add_Image_Set("main", "game/items/berry_life.imgset");
        m_name = _("Life berry");
    }
    else if (new_type == TYPE_MUSHROOM_POISON) {
        Add_Image_Set("main", "game/items/berry_poison.imgset");
        m_name = _("Poisonous berry");
    }
    else if (new_type == TYPE_MUSHROOM_BLUE) {
        Add_Image_Set("main", "game/items/berry_ice.imgset");
        m_name = _("Ice berry");
    }
    else if (new_type == TYPE_MUSHROOM_GHOST) {
        Add_Image_Set("main", "game/items/berry_ghost.imgset");
        m_name = _("Ghost berry");
    }
    else {
        cerr << "Warning Unknown Mushroom type : " << new_type << endl;
        return;
    }

    m_type = new_type;
    Set_Image_Set("main", 1);
}

void cMushroom::Activate(void)
{
    if (!m_active) {
        return;
    }

    cPowerUp::Activate();

    pLevel_Player->Get_Item(m_type);

    if (m_type == TYPE_MUSHROOM_DEFAULT) {
        gp_hud->Add_Points(500, m_pos_x + (m_rect.m_w * 0.5f), m_pos_y + 3);
    }
    else if (m_type == TYPE_MUSHROOM_LIVE_1) {
        gp_hud->Add_Points(1000, m_pos_x + (m_rect.m_w * 0.5f), m_pos_y + 3);
    }
    else if (m_type == TYPE_MUSHROOM_POISON) {
        // nothing
    }
    else if (m_type == TYPE_MUSHROOM_BLUE) {
        gp_hud->Add_Points(700, m_pos_x + (m_rect.m_w * 0.5f), m_pos_y + 3);
    }
    else if (m_type == TYPE_MUSHROOM_GHOST) {
        gp_hud->Add_Points(800, m_pos_x + (m_rect.m_w * 0.5f), m_pos_y + 3);
    }

    // if spawned destroy
    if (m_spawned) {
        Destroy();
    }
    // hide
    else {
        Set_Active(0);
    }
}

void cMushroom::Update(void)
{
    if (!m_valid_update || !Is_In_Range()) {
        return;
    }
    Update_Animation();

    if (!m_ground_object) {
        if (m_vely < m_gravity_max) {
            Add_Velocity_Y_Max(1.5f, m_gravity_max);
        }
    }
    else {
        m_vely = 0.0f;
    }

    // particles
    if (m_type == TYPE_MUSHROOM_LIVE_1 || m_type == TYPE_MUSHROOM_BLUE) {
        m_counter += pFramerate->m_speed_factor * 0.8f;

        if (m_counter > 1.0f) {
            cParticle_Emitter* anim = new cParticle_Emitter(m_sprite_manager);
            anim->Set_Image(pVideo->Get_Surface("animation/particles/light.png"));
            anim->Set_Emitter_Rect(m_col_rect.m_x, m_col_rect.m_y, m_col_rect.m_w, m_col_rect.m_h);
            anim->Set_Quota(static_cast<int>(m_counter));
            anim->Set_Pos_Z(m_pos_z - m_pos_z_delta);
            anim->Set_Direction_Range(180, 180);
            anim->Set_Scale(0.4f, 0.1f);
            // 1-UP
            if (m_type ==  TYPE_MUSHROOM_LIVE_1) {
                anim->Set_Time_to_Live(0.4f);
                anim->Set_Color(Color(static_cast<uint8_t>(0), 100, 0), Color(static_cast<uint8_t>(30), 150, 30, 0));
                anim->Set_Speed(0.5f, 0.5f);
                anim->Set_Blending(BLEND_ADD);
            }
            // Ice
            else if (m_type == TYPE_MUSHROOM_BLUE) {
                anim->Set_Time_to_Live(0.6f);
                anim->Set_Color(Color(static_cast<uint8_t>(180), 180, 255, 128), Color(static_cast<uint8_t>(50), 50, 0, 0));
                anim->Set_Speed(0.2f, 0.1f);
            }

            anim->Emit();
            pActive_Animation_Manager->Add(anim);

            m_counter -= static_cast<int>(m_counter);
        }

        if (m_type == TYPE_MUSHROOM_BLUE) {
            // glimming
            // TODO: using image sets, we could perhaps do this as separate frames.
            float new_glim = m_combine_color[2];

            if (m_glim_mod) {
                new_glim += pFramerate->m_speed_factor * 0.02f;

                if (new_glim > 0.5f) {
                    new_glim = 1.0f;
                    m_glim_mod = 0;
                }
            }
            else {
                new_glim -= pFramerate->m_speed_factor * 0.02f;

                if (new_glim < 0.0f) {
                    new_glim = 0.0f;
                    m_glim_mod = 1;
                }
            }

            Set_Color_Combine(new_glim * 0.1f, new_glim * 0.1f, new_glim, GL_ADD);
        }
    }
    // Poison
    else if (m_type == TYPE_MUSHROOM_POISON) {
        // animation only if on ground
        if (m_ground_object) {
            m_counter += pFramerate->m_speed_factor * 0.7f;

            if (m_counter > 1.0f) {
                cParticle_Emitter* anim = new cParticle_Emitter(m_sprite_manager);
                anim->Set_Emitter_Rect(m_col_rect.m_x + (m_col_rect.m_w * 0.3f), m_col_rect.m_y + (m_col_rect.m_h * 0.91f), m_col_rect.m_w * 0.4f, 0);
                anim->Set_Quota(static_cast<int>(m_counter));
                anim->Set_Pos_Z(m_pos_z - m_pos_z_delta);
                anim->Set_Time_to_Live(1.4f, 0.4f);
                anim->Set_Scale(0.7f, 0.2f);
                anim->Set_Color(Color(static_cast<uint8_t>(120), 190, 0), Color(static_cast<uint8_t>(40), 60, 10, 0));
                anim->Set_Blending(BLEND_ADD);
                anim->Set_Speed(0.0f, 0.0f);
                anim->Set_Image(pVideo->Get_Surface("animation/particles/slime_1.png"));
                anim->Emit();
                pActive_Animation_Manager->Add(anim);

                m_counter -= static_cast<int>(m_counter);
            }
        }
    }
}

void cMushroom::Handle_Collision_Massive(cObjectCollision* collision)
{
    Turn_Around(collision->m_direction);
}

void cMushroom::Handle_Collision_Box(ObjectDirection cdirection, GL_rect* r2)
{
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
    // unsupported collision direction
    else {
        return;
    }

    Reset_On_Ground();
}

/* *** *** *** *** *** *** cFirePlant *** *** *** *** *** *** *** *** *** *** *** */

cFirePlant::cFirePlant(cSprite_Manager* sprite_manager)
    : cPowerUp(sprite_manager)
{
    cFirePlant::Init();
}

cFirePlant::cFirePlant(XmlAttributes& attributes, cSprite_Manager* sprite_manager)
    : cPowerUp(sprite_manager)
{
    cFirePlant::Init();

    // position
    Set_Pos(string_to_float(attributes["posx"]), string_to_float(attributes["posy"]), true);
}

cFirePlant::~cFirePlant(void)
{
    //
}

void cFirePlant::Init(void)
{
    m_type = TYPE_FIREPLANT;
    m_can_be_on_ground = 0;
    m_pos_z = 0.051f;

    Clear_Images();
    Add_Image_Set("main", "game/items/berry_fire.imgset");
    Set_Image_Set("main", 1);

    m_name = _("Fireberry");

    m_particle_counter = 0.0f;
}

cFirePlant* cFirePlant::Copy(void) const
{
    cFirePlant* fireplant = new cFirePlant(m_sprite_manager);
    fireplant->Set_Pos(m_start_pos_x, m_start_pos_y, 1);
    return fireplant;
}

void cFirePlant::Activate(void)
{
    if (!m_active) {
        return;
    }

    cPowerUp::Activate();

    pLevel_Player->Get_Item(TYPE_FIREPLANT);

    gp_hud->Add_Points(700, m_pos_x + (m_image ? m_image->m_w / 2 : 0), m_pos_y);

    // if spawned destroy
    if (m_spawned) {
        Destroy();
    }
    // hide
    else {
        Set_Active(0);
    }
}

void cFirePlant::Update(void)
{
    if (!m_valid_update || !Is_Visible_On_Screen()) {
        return;
    }
    Update_Animation();

    if (!m_ground_object) {
        if (m_vely < m_gravity_max) {
            Add_Velocity_Y_Max(1.2f, m_gravity_max);
        }
    }
    else {
        m_vely = 0.0f;
    }

    // particle animation
    m_particle_counter += pFramerate->m_speed_factor * 0.8f;

    if (m_particle_counter > 1.0f) {
        cParticle_Emitter* anim = new cParticle_Emitter(m_sprite_manager);
        anim->Set_Image(pVideo->Get_Surface("animation/particles/light.png"));
        anim->Set_Emitter_Rect(m_col_rect.m_x, m_col_rect.m_y + 10, m_col_rect.m_w, m_col_rect.m_h * 0.5f);
        anim->Set_Quota(static_cast<int>(m_particle_counter));
        anim->Set_Pos_Z(m_pos_z + m_pos_z_delta);
        anim->Set_Direction_Range(180.0f, 180.0f);
        anim->Set_Scale(0.4f, 0.1f);
        anim->Set_Time_to_Live(0.4f);
        anim->Set_Color(Color(static_cast<uint8_t>(255), 50, 0), Color(static_cast<uint8_t>(0), 50, 0, 0));
        anim->Set_Speed(0.2f, 0.1f);
        anim->Set_Blending(BLEND_ADD);
        anim->Emit();
        pActive_Animation_Manager->Add(anim);

        m_particle_counter -= static_cast<int>(m_particle_counter);
    }
}

/* *** *** *** *** *** *** *** cMoon *** *** *** *** *** *** *** *** *** *** */

cMoon::cMoon(cSprite_Manager* sprite_manager)
    : cPowerUp(sprite_manager)
{
    cMoon::Init();
}

cMoon::cMoon(XmlAttributes& attributes, cSprite_Manager* sprite_manager)
    : cPowerUp(sprite_manager)
{
    cMoon::Init();

    // Position
    Set_Pos(string_to_float(attributes["posx"]), string_to_float(attributes["posy"]), true);
}

cMoon::~cMoon(void)
{
    //
}

void cMoon::Init(void)
{
    m_type = TYPE_MOON;
    m_can_be_on_ground = 0;
    m_pos_z = 0.052f;

    Clear_Images();
    Add_Image_Set("main", "game/items/cookie.imgset");
    Set_Image_Set("main", 1);

    m_name = _("Cookie (3-UP)");
    m_particle_counter = 0.0f;
}

cMoon* cMoon::Copy(void) const
{
    cMoon* moon = new cMoon(m_sprite_manager);
    moon->Set_Pos(m_start_pos_x, m_start_pos_y);
    return moon;
}

void cMoon::Activate(void)
{
    if (!m_active) {
        return;
    }

    cPowerUp::Activate();

    pLevel_Player->Get_Item(TYPE_MOON);

    gp_hud->Add_Points(4000, m_pos_x + (m_image ? m_image->m_w / 2 : 0), m_pos_y);

    // Particle burst on collection
    cParticle_Emitter* anim = new cParticle_Emitter(m_sprite_manager);
    anim->Set_Image(pVideo->Get_Surface("animation/particles/dirt.png"));
    anim->Set_Emitter_Rect(m_pos_x, m_pos_y, m_rect.m_w, m_rect.m_h);
    anim->Set_Quota(20);
    anim->Set_Pos_Z(m_pos_z - 0.0001f);
    anim->Set_Speed(0.6f, 0.6f);
    anim->Set_Scale(1.0f);
    anim->Set_Time_to_Live(5.0f);
    anim->Set_Const_Rotation_Z(7.0f, 1.0f);
    anim->Emit();
    pActive_Animation_Manager->Add(anim);

    // if spawned destroy
    if (m_spawned) {
        Destroy();
    }
    // hide
    else {
        Set_Active(0);
    }
}

void cMoon::Update(void)
{
    if (!m_valid_update || !Is_Visible_On_Screen()) {
        return;
    }

    m_particle_counter += pFramerate->m_speed_factor * 0.06;

    // particles
    if (m_particle_counter >= 1.0f) {
        cParticle_Emitter* anim = new cParticle_Emitter(m_sprite_manager);
        anim->Set_Image(pVideo->Get_Surface("animation/particles/dirt.png"));
        anim->Set_Emitter_Rect(m_pos_x, m_pos_y, m_rect.m_w, m_rect.m_h);
        anim->Set_Quota(static_cast<int>(m_particle_counter));
        anim->Set_Pos_Z(m_pos_z - 0.0001f);
        anim->Set_Speed(0.2f, 0.2f);
        anim->Set_Scale(1.0f);
        anim->Set_Time_to_Live(2.5f);
        anim->Set_Const_Rotation_Z(5.0f, 1.0f);
        anim->Emit();
        pActive_Animation_Manager->Add(anim);

        m_particle_counter -= static_cast<int>(m_particle_counter);
    }
}


/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

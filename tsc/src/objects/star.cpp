/***************************************************************************
 * star.cpp  -  jumping star class
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

#include "../objects/star.hpp"
#include "../level/level_player.hpp"
#include "../core/framerate.hpp"
#include "../video/animation.hpp"
#include "../core/i18n.hpp"
#include "../core/game_core.hpp"

namespace TSC {

/* *** *** *** *** *** *** cjStar *** *** *** *** *** *** *** *** *** *** *** */

cjStar::cjStar(cSprite_Manager* sprite_manager)
    : cPowerUp(sprite_manager)
{
    cjStar::Init();
}

cjStar::cjStar(XmlAttributes& attributes, cSprite_Manager* sprite_manager)
    : cPowerUp(sprite_manager)
{
    cjStar::Init();

    // Position
    Set_Pos(string_to_float(attributes["posx"]), string_to_float(attributes["posy"]), true);
}

cjStar::~cjStar(void)
{
    //
}

void cjStar::Init(void)
{
    m_type = TYPE_STAR;
    m_pos_z = 0.053f;

    m_direction = DIR_RIGHT;
    m_anim_counter = 0;
    m_glim_mod = 1;
    m_glim_counter = 0;

    m_velx = 5;

    Add_Image(pVideo->Get_Surface("game/items/star.png"));
    Set_Image_Num(0, 1, 0);

    m_name = _("Star");
}

cjStar* cjStar::Copy(void) const
{
    cjStar* star = new cjStar(m_sprite_manager);
    star->Set_Pos(m_start_pos_x, m_start_pos_y, 1);
    return star;
}

void cjStar::Activate(void)
{
    if (!m_active) {
        return;
    }

    cPowerUp::Activate();

    // animation
    Generate_Particles(m_pos_x + m_col_rect.m_w * 0.5f, m_pos_y + m_col_rect.m_h * 0.5f, 1, 20);

    // activate star
    pLevel_Player->Get_Item(TYPE_STAR);

    // if spawned destroy
    if (m_spawned) {
        Destroy();
    }
    // disable
    else {
        Set_Active(0);
    }
}

void cjStar::Update(void)
{
    if (!m_valid_update || !Is_In_Range()) {
        return;
    }

    // Add Gravitation
    if (m_vely < m_gravity_max) {
        Add_Velocity_Y_Max(1.8f, m_gravity_max);
    }

    // rotate
    if (m_vely < 0.0f) {
        Add_Rotation_Z((5 - (m_vely / 2.5f)) * pFramerate->m_speed_factor);
    }
    // rotate back to 0 if falling
    else {
        if (m_rot_z > 5.0f && m_rot_z <= 175.0f) {
            Add_Rotation_Z((5 - (m_vely / 1.2f)) * pFramerate->m_speed_factor);
        }
        else if (m_rot_z < 355 && m_rot_z > 185) {
            Add_Rotation_Z((-5 + (m_vely / 1.2f)) * pFramerate->m_speed_factor);
        }
    }

    // generate small stars
    m_anim_counter += 1.1f * pFramerate->m_speed_factor;

    if (m_anim_counter > 1.0f) {
        Generate_Particles(m_pos_x, m_pos_y, 1, static_cast<int>(m_anim_counter));
        m_anim_counter -= static_cast<int>(m_anim_counter);
    }

    // glim animation
    if (m_glim_mod) {
        m_glim_counter += pFramerate->m_speed_factor * 0.2f;

        if (m_glim_counter > 1.0f) {
            m_glim_counter = 1.0f;
            m_glim_mod = 0;
        }
    }
    else {
        m_glim_counter -= pFramerate->m_speed_factor * 0.2f;

        if (m_glim_counter < 0.0f) {
            m_glim_counter = 0.0f;
            m_glim_mod = 1;
        }
    }
}

void cjStar::Draw(cSurface_Request* request /* = NULL */)
{
    if (!m_valid_draw) {
        return;
    }

    Set_Color_Combine(m_glim_counter / 0.8f, m_glim_counter / 0.9f, m_glim_counter, GL_ADD);

    cPowerUp::Draw();
}

void cjStar::Generate_Particles(float x /* = 0.0f */, float y /* = 0.0f */, bool random /* = 1 */, unsigned int quota /* = 2 */) const
{
    if (Is_Float_Equal(x, 0.0f)) {
        x = m_pos_x;
    }

    if (Is_Float_Equal(y, 0.0f)) {
        y = m_pos_y;
    }

    // set particle color
    Color particle_color = orange;
    particle_color.green += static_cast<Uint8>(m_glim_counter / 5);
    particle_color.blue += static_cast<Uint8>(m_glim_counter / 1.5f);

    // create emitter
    cParticle_Emitter* anim = new cParticle_Emitter(m_sprite_manager);
    anim->Set_Image(pVideo->Get_Surface("animation/particles/star.png"));
    anim->Set_Pos_Z(m_pos_z + 0.0001f);

    if (random) {
        anim->Set_Emitter_Rect(x, y, m_col_rect.m_w * 0.9f, m_col_rect.m_h * 0.9f);
    }
    // no random position
    else {
        anim->Set_Emitter_Rect(x, y);
        // emit upwards
        anim->Set_Direction_Range(180, 180);
    }
    anim->Set_Quota(quota);
    anim->Set_Time_to_Live(0.4f);
    anim->Set_Fading_Alpha(1);
    anim->Set_Fading_Size(1);
    anim->Set_Speed(1.5f, 0.5f);
    anim->Set_Scale(0.15f);
    anim->Set_Color(particle_color);
    anim->Set_Blending(BLEND_ADD);
    anim->Set_Const_Rotation_Z(-5, 10);
    anim->Emit();
    pActive_Animation_Manager->Add(anim);
}

void cjStar::Handle_Collision_Massive(cObjectCollision* collision)
{
    if (collision->m_direction == DIR_RIGHT || collision->m_direction == DIR_LEFT) {
        Turn_Around(collision->m_direction);
    }
    else if (collision->m_direction == DIR_UP) {
        m_vely = -(m_vely * 0.3f);
    }
    else if (collision->m_direction == DIR_DOWN) {
        if (m_ground_object) {
            // clamp x to the ground object position
            const float clamped_x = Clamp(m_rect.m_x + (m_rect.m_w / 2), m_ground_object->m_col_rect.m_x, m_ground_object->m_col_rect.m_x + m_ground_object->m_col_rect.m_w);
            Generate_Particles(clamped_x, m_ground_object->m_col_rect.m_y, 0, 5);
        }

        m_vely = -25.0f;
    }
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

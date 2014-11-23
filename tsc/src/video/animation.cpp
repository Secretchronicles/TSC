/***************************************************************************
 * animation.cpp  -  Animation and Particle classes
 *
 * Copyright © 2003 - 2011 Florian Richter
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

#include "../video/animation.hpp"
#include "../core/framerate.hpp"
#include "../core/game_core.hpp"
#include "../video/gl_surface.hpp"
#include "../video/renderer.hpp"
#include "../core/math/utilities.hpp"
#include "../core/i18n.hpp"
#include "../core/filesystem/filesystem.hpp"
#include "../core/filesystem/resource_manager.hpp"
#include "../core/filesystem/boost_relative.hpp"
#include "../core/xml_attributes.hpp"
#include "../input/mouse.hpp"

namespace fs = boost::filesystem;

namespace TSC {

/* *** *** *** *** *** *** *** Base Animation class *** *** *** *** *** *** *** *** *** *** */

cAnimation::cAnimation(cSprite_Manager* sprite_manager, std::string type_name /* = "sprite" */)
    : cAnimated_Sprite(sprite_manager, type_name)
{
    m_sprite_array = ARRAY_ANIM;
    m_type = TYPE_ACTIVE_SPRITE;
    m_massive_type = MASS_PASSIVE;
    Set_Spawned(1);
    m_can_be_on_ground = 0;

    m_pos_z = 0.07000f;
    m_pos_z_rand = 0.0f;
    m_time_to_live = 0.0f;
    m_time_to_live_rand = 0.0f;

    m_fading_speed = 1.0f;
}

cAnimation::~cAnimation(void)
{
    //
}

void cAnimation::Init_Anim(void)
{
    // virtual
}

void cAnimation::Update(void)
{
    // virtual
}

void cAnimation::Draw(cSurface_Request* request /* = NULL */)
{

}

void cAnimation::Set_Time_to_Live(float time, float time_rand /* = 0.0f */)
{
    m_time_to_live = time;
    m_time_to_live_rand = time_rand;
}

void cAnimation::Set_Fading_Speed(float speed)
{
    m_fading_speed = speed;

    if (m_fading_speed <= 0.0f) {
        m_fading_speed = 0.1f;
    }
}

void cAnimation::Set_Pos_Z(float pos, float pos_rand /* = 0.0f */)
{
    m_pos_z = pos;
    m_pos_z_rand = pos_rand;
}

/* *** *** *** *** *** *** *** cAnimation_Goldpiece *** *** *** *** *** *** *** *** *** *** */

cAnimation_Goldpiece::cAnimation_Goldpiece(cSprite_Manager* sprite_manager, float posx, float posy, float height /* = 40.0f */, float width /* = 20.0f */)
    : cAnimation(sprite_manager)
{
    Add_Image(pVideo->Get_Surface("animation/light_1/1.png"));
    Add_Image(pVideo->Get_Surface("animation/light_1/2.png"));
    Add_Image(pVideo->Get_Surface("animation/light_1/3.png"));

    Set_Pos(posx, posy, 1);
    m_rect.m_w = width;
    m_rect.m_h = height;

    for (unsigned int i = 0; i < 4; i++) {
        cSprite* obj = new cSprite(m_sprite_manager);
        obj->Set_Image(m_images[0].m_image);
        obj->Set_Pos(m_pos_x + Get_Random_Float(0.0f, m_rect.m_w), m_pos_y + Get_Random_Float(0.0f, m_rect.m_h));
        obj->m_pos_z = m_pos_z;
        obj->Set_Scale_X(m_scale_x, 1);
        obj->Set_Scale_Y(m_scale_y, 1);
        obj->Set_Color(m_color);
        obj->Set_Color_Combine(m_combine_color[0], m_combine_color[1], m_combine_color[2], m_combine_type);

        m_objects.push_back(obj);
    }
}

cAnimation_Goldpiece::~cAnimation_Goldpiece(void)
{
    // clear
    for (BlinkPointList::iterator itr = m_objects.begin(); itr != m_objects.end(); ++itr) {
        delete *itr;
    }

    m_objects.clear();
}

void cAnimation_Goldpiece::Update(void)
{
    if (!m_active || editor_enabled) {
        return;
    }

    m_time_to_live += pFramerate->m_speed_factor * m_fading_speed;

    unsigned int count = 0;

    // update the fixed points
    for (BlinkPointList::iterator itr = m_objects.begin(); itr != m_objects.end(); ++itr) {
        // get object
        cSprite* obj = (*itr);

        switch (count) {
        case 0: {
            if (m_time_to_live < 3.0f) {
                obj->Set_Image(m_images[0].m_image);
            }
            else if (m_time_to_live < 6.0f) {
                obj->Set_Image(m_images[0].m_image);
            }
            else if (m_time_to_live < 9.0f) {
                obj->Set_Image(m_images[1].m_image);
            }
            else if (m_time_to_live < 12.0f) {
                obj->Set_Image(m_images[0].m_image);
            }
            break;
        }
        case 1: {
            if (m_time_to_live < 3.0f) {
                obj->Set_Image(m_images[0].m_image);
            }
            else if (m_time_to_live < 6.0f) {
                obj->Set_Image(m_images[1].m_image);
            }
            else if (m_time_to_live < 9.0f) {
                obj->Set_Image(m_images[2].m_image);
            }
            else if (m_time_to_live < 12.0f) {
                obj->Set_Image(m_images[1].m_image);
            }
            break;
        }
        case 2: {
            if (m_time_to_live < 3.0f) {
                obj->Set_Image(m_images[1].m_image);
            }
            else if (m_time_to_live < 6.0f) {
                obj->Set_Image(m_images[2].m_image);
            }
            else if (m_time_to_live < 9.0f) {
                obj->Set_Image(m_images[0].m_image);
            }
            else if (m_time_to_live < 12.0f) {
                obj->Set_Image(m_images[0].m_image);
            }
            break;
        }
        case 3: {
            if (m_time_to_live < 3.0f) {
                obj->Set_Image(m_images[0].m_image);
            }
            else if (m_time_to_live < 6.0f) {
                obj->Set_Image(m_images[1].m_image);
            }
            else if (m_time_to_live < 9.0f) {
                obj->Set_Image(m_images[0].m_image);
            }
            else if (m_time_to_live < 12.0f) {
                obj->Set_Image(m_images[0].m_image);
            }
            break;
        }
        default: {
            break;
        }
        }

        obj->Set_Scale(1.1f - (m_time_to_live / 12));
        count++;
    }
}

void cAnimation_Goldpiece::Draw(cSurface_Request* request /* = NULL */)
{
    if (!m_active) {
        return;
    }

    // draw the fixed points
    for (BlinkPointList::iterator itr = m_objects.begin(); itr != m_objects.end(); ++itr) {
        // get object
        cSprite* obj = (*itr);

        obj->Draw();
    }

    if (m_time_to_live > 11.0f || m_time_to_live < 0.0f) {
        Set_Active(0);
    }
}

/* *** *** *** *** *** *** *** cAnimation_Fireball *** *** *** *** *** *** *** *** *** *** */

cAnimation_Fireball::cAnimation_Fireball(cSprite_Manager* sprite_manager, float posx, float posy, unsigned int power /* = 5 */)
    : cAnimation(sprite_manager)
{
    Set_Pos(posx, posy, 1);

    // create objects
    for (unsigned int i = 0; i < power; i++) {
        cAnimation_Fireball_Item* obj = new cAnimation_Fireball_Item(sprite_manager);

        // images
        obj->Add_Image(pVideo->Get_Surface("animation/particles/fire_4.png"));
        obj->Add_Image(pVideo->Get_Surface("animation/particles/fire_3.png"));
        obj->Add_Image(pVideo->Get_Surface("animation/particles/fire_2.png"));
        obj->Add_Image(pVideo->Get_Surface("animation/particles/fire_1.png"));
        obj->Set_Image_Num(0);

        // velocity
        obj->m_velx = Get_Random_Float(-2.5f, 5);
        obj->m_vely = Get_Random_Float(-2.5f, 5);

        // Z position
        obj->m_pos_z = m_pos_z;
        if (m_pos_z_rand > 0) {
            obj->m_pos_z += Get_Random_Float(0, m_pos_z_rand);
        }

        // lifetime
        obj->m_counter = Get_Random_Float(8, 13);

        m_objects.push_back(obj);
    }
}

cAnimation_Fireball::~cAnimation_Fireball(void)
{
    // clear
    for (FireAnimList::iterator itr = m_objects.begin(); itr != m_objects.end(); ++itr) {
        delete *itr;
    }

    m_objects.clear();
}

void cAnimation_Fireball::Update(void)
{
    if (!m_active || editor_enabled) {
        return;
    }

    m_time_to_live += pFramerate->m_speed_factor * m_fading_speed;

    // update objects
    for (FireAnimList::iterator itr = m_objects.begin(); itr != m_objects.end(); ++itr) {
        cAnimation_Fireball_Item* obj = (*itr);

        if (obj->m_counter > 8) {
            obj->Set_Image_Num(0);
        }
        else if (obj->m_counter > 5) {
            obj->Set_Image_Num(1);
        }
        else if (obj->m_counter > 3) {
            obj->Set_Image_Num(2);
        }
        else {
            obj->Set_Image_Num(3);
        }

        obj->m_counter -= pFramerate->m_speed_factor * m_fading_speed;

        obj->Set_Scale(obj->m_counter / 10);
        obj->Move(obj->m_velx, obj->m_vely);
    }

    if (m_time_to_live > 12.0f || m_time_to_live < 0.0f) {
        Set_Active(0);
    }
}

void cAnimation_Fireball::Draw(cSurface_Request* request /* = NULL */)
{
    if (!m_active) {
        return;
    }

    // draw objects
    for (FireAnimList::iterator itr = m_objects.begin(); itr != m_objects.end(); ++itr) {
        cAnimation_Fireball_Item* obj = (*itr);

        // create request
        cSurface_Request* request = new cSurface_Request();
        obj->m_image->Blit(obj->m_pos_x - (pActive_Camera->m_x - m_pos_x), obj->m_pos_y - (pActive_Camera->m_y - m_pos_y), obj->m_pos_z, request);

        // scale
        request->m_scale_x = obj->m_scale_x;
        request->m_scale_y = obj->m_scale_y;

        // color
        request->m_color = m_color;

        // add request
        pRenderer->Add(request);
    }
}

/* *** *** *** *** *** *** *** cParticle *** *** *** *** *** *** *** *** *** *** */

cParticle::cParticle(cParticle_Emitter* parent)
    : cMovingSprite(parent->m_sprite_manager)
{
    m_can_be_on_ground = 0;
    // scale centered
    Set_Scale_Directions(1, 1, 1, 1);

    m_time_to_live = 0.0f;
    m_const_rot_x = 0.0f;
    m_const_rot_y = 0.0f;
    m_const_rot_z = 0.0f;
    m_gravity_x = 0.0f;
    m_gravity_y = 0.0f;

    m_fade_pos = 1.0f;

    m_parent = parent;
}

cParticle::~cParticle(void)
{

}

void cParticle::Update(void)
{
    // update fade modifier
    m_fade_pos -= ((static_cast<float>(speedfactor_fps) * 0.001f) * pFramerate->m_speed_factor) / m_time_to_live;

    // finished fading
    if (m_fade_pos <= 0.0f) {
        m_fade_pos = 0.0f;
        Set_Active(0);
        return;
    }

    // with size fading
    if (m_parent->m_fade_size) {
        Set_Scale(m_start_scale_x * m_fade_pos);
    }

    // move
    Move(m_velx, m_vely);
    // todo : gravity maximum
    Add_Velocity(m_gravity_x, m_gravity_y);

    // constant rotation
    if (!Is_Float_Equal(m_const_rot_x, 0.0f)) {
        Add_Rotation_X(m_const_rot_x * pFramerate->m_speed_factor);
    }
    if (!Is_Float_Equal(m_const_rot_y, 0.0f)) {
        Add_Rotation_Y(m_const_rot_y * pFramerate->m_speed_factor);
    }
    if (!Is_Float_Equal(m_const_rot_z, 0.0f)) {
        Add_Rotation_Z(m_const_rot_z * pFramerate->m_speed_factor);
    }
}

void cParticle::Draw(cSurface_Request* request /* = NULL */)
{
    if (!m_image) {
        return;
    }

    bool create_request = 0;

    if (!request) {
        create_request = 1;
        // create request
        request = new cSurface_Request();
    }

    Draw_Image_Normal(request);

    // based on emitter position
    if (m_parent->m_particle_based_on_emitter_pos > 0.0f) {
        request->m_pos_x += (m_parent->m_pos_x * m_parent->m_particle_based_on_emitter_pos);
        request->m_pos_y += (m_parent->m_pos_y * m_parent->m_particle_based_on_emitter_pos);
    }

    // blending
    if (m_parent->m_blending == BLEND_ADD) {
        request->m_blend_sfactor = GL_SRC_ALPHA;
        request->m_blend_dfactor = GL_ONE;
    }
    else if (m_parent->m_blending == BLEND_DRIVE) {
        request->m_blend_sfactor = GL_SRC_COLOR;
        request->m_blend_dfactor = GL_DST_ALPHA;
    }

    // color fading
    if (m_parent->m_fade_color) {
        request->m_color.red = static_cast<Uint8>(m_color.red * m_fade_pos);
        request->m_color.green = static_cast<Uint8>(m_color.green * m_fade_pos);
        request->m_color.blue = static_cast<Uint8>(m_color.blue * m_fade_pos);
    }

    // alpha fading
    if (m_parent->m_fade_alpha) {
        request->m_color.alpha = static_cast<Uint8>(request->m_color.alpha * m_fade_pos);
    }

    if (create_request) {
        // add request
        pRenderer->Add(request);
    }
}

void cParticle::Set_Gravity(float x, float y)
{
    m_gravity_x = x;
    m_gravity_y = y;
}

/* *** *** *** *** *** *** *** cParticle_Emitter *** *** *** *** *** *** *** *** *** *** */

cParticle_Emitter::cParticle_Emitter(cSprite_Manager* sprite_manager)
    : cAnimation(sprite_manager, "particle_emitter")
{
    cParticle_Emitter::Init();
}

cParticle_Emitter::cParticle_Emitter(XmlAttributes& attributes, cSprite_Manager* sprite_manager)
    : cAnimation(sprite_manager, "particle_emitter")
{
    cParticle_Emitter::Init();

    // Particle image filename
    Set_Image_Filename(utf8_to_path(attributes["particle_image"]));


    // position z
    Set_Pos_Z(attributes.fetch<float>("pos_z", m_pos_z),
              attributes.fetch<float>("pos_z_rand", m_pos_z_rand));

    // emitter based on camera pos
    Set_Based_On_Camera_Pos(attributes.fetch<bool>("emitter_based_on_camera_pos", m_emitter_based_on_camera_pos));

    // particle based on emitter pos
    Set_Particle_Based_On_Emitter_Pos(attributes.fetch<float>("particle_based_on_emitter_pos", m_particle_based_on_emitter_pos));

    // emitter rect
    Set_Emitter_Rect(static_cast<float>(attributes.fetch<int>("posx", static_cast<int>(m_pos_x))),
                     static_cast<float>(attributes.fetch<int>("posy", static_cast<int>(m_pos_y))),
                     static_cast<float>(attributes.fetch<int>("sizex", static_cast<int>(m_start_rect.m_w))),
                     static_cast<float>(attributes.fetch<int>("sizey", static_cast<int>(m_start_rect.m_h))));

    // emitter time to live
    Set_Emitter_Time_to_Live(attributes.fetch<float>("emitter_time_to_live", m_emitter_time_to_live));

    // emitter interval
    Set_Emitter_Iteration_Interval(attributes.fetch<float>("emitter_interval", m_emitter_iteration_interval));

    // quota/count
    Set_Quota(attributes.fetch<int>("quota", m_emitter_quota));

    // time for particle to live
    Set_Time_to_Live(attributes.fetch<float>("time_to_live", m_time_to_live),
                     attributes.fetch<float>("time_to_live_rand", m_time_to_live_rand));

    // velocity
    Set_Speed(attributes.fetch<float>("vel", m_vel),
              attributes.fetch<float>("vel_rand", m_vel_rand));

    // start rotation
    Set_Rotation(attributes.fetch<float>("rot_x", m_start_rot_x),
                 attributes.fetch<float>("rot_y", m_start_rot_y),
                 attributes.fetch<float>("rot_z", m_start_rot_z),
                 true);

    Set_Start_Rot_Z_Uses_Direction(attributes.fetch<bool>("start_rot_z_uses_direction", m_start_rot_z_uses_direction));

    // constant rotation x
    Set_Const_Rotation_X(attributes.fetch<float>("const_rot_x", m_const_rot_x),
                         attributes.fetch<float>("const_rot_x_rand", m_const_rot_x_rand));

    // constant rotation y
    Set_Const_Rotation_X(attributes.fetch<float>("const_rot_y", m_const_rot_y),
                         attributes.fetch<float>("const_rot_y_rand", m_const_rot_y_rand));

    // constant rotation z
    Set_Const_Rotation_X(attributes.fetch<float>("const_rot_z", m_const_rot_z),
                         attributes.fetch<float>("const_rot_z_rand", m_const_rot_z_rand));

    // angle
    Set_Direction_Range(attributes.fetch<float>("angle_start", m_angle_start),
                        attributes.fetch<float>("angle_range", m_angle_range));

    // scale
    Set_Scale(attributes.fetch<float>("size_scale", m_size_scale),
              attributes.fetch<float>("size_scale_rand", m_size_scale_rand));

    // horizontal gravity
    Set_Horizontal_Gravity(attributes.fetch<float>("gravity_x", m_gravity_x),
                           attributes.fetch<float>("gravity_x_rand", m_gravity_x_rand));

    // vertical gravity
    Set_Vertical_Gravity(attributes.fetch<float>("gravity_y", m_gravity_y),
                         attributes.fetch<float>("gravity_y_rand", m_gravity_y_rand));

    // clip rect
    Set_Clip_Rect(static_cast<float>(attributes.fetch<int>("clip_x", static_cast<int>(m_clip_rect.m_x))),
                  static_cast<float>(attributes.fetch<int>("clip_y", static_cast<int>(m_clip_rect.m_y))),
                  static_cast<float>(attributes.fetch<int>("clip_w", static_cast<int>(m_clip_rect.m_w))),
                  static_cast<float>(attributes.fetch<int>("clip_h", static_cast<int>(m_clip_rect.m_h))));

    // clip mode
    Set_Clip_Mode(static_cast<ParticleClipMode>(attributes.fetch<int>("clip_mode", m_clip_mode)));
}

cParticle_Emitter::~cParticle_Emitter(void)
{
    cParticle_Emitter::Clear();
}

void cParticle_Emitter::Init(void)
{
    m_editor_pos_z = 0.111f;
    m_sprite_array = ARRAY_ACTIVE;
    m_type = TYPE_PARTICLE_EMITTER;
    m_name = "Particle Emitter";

    m_emitter_based_on_camera_pos = 0;
    m_particle_based_on_emitter_pos = 0.0f;
    m_rect.m_w = 0.0f;
    m_rect.m_h = 0.0f;
    m_col_rect.m_w = m_rect.m_w;
    m_col_rect.m_h = m_rect.m_h;
    m_start_rect.m_w = m_rect.m_w;
    m_start_rect.m_h = m_rect.m_h;

    // 0 = 1 emit
    m_emitter_time_to_live = 0.0f;
    m_emitter_iteration_interval = 0.2f;
    m_emitter_quota = 1;

    // velocity
    m_vel = 2.0f;
    m_vel_rand = 2.0f;
    // rotation
    m_start_rot_z_uses_direction = 0;
    m_const_rot_x = 0.0f;
    m_const_rot_y = 0.0f;
    m_const_rot_z = 0.0f;
    m_const_rot_x_rand = 0.0f;
    m_const_rot_y_rand = 0.0f;
    m_const_rot_z_rand = 0.0f;
    // angle
    m_angle_start = 0.0f;
    m_angle_range = 360.0f;
    // scale
    m_size_scale = 1.0f;
    m_size_scale_rand = 0.0f;
    // gravity
    m_gravity_x = 0.0f;
    m_gravity_x_rand = 0.0f;
    m_gravity_y = 0.0f;
    m_gravity_y_rand = 0.0f;

    // color
    m_color_rand = Color(static_cast<Uint8>(0), 0, 0, 0);
    // default 1 second
    m_time_to_live = 1.0f;
    // default fading is alpha
    m_fade_size = 0;
    m_fade_alpha = 1;
    m_fade_color = 0;

    m_blending = BLEND_NONE;

    m_clip_rect = GL_rect();
    m_clip_mode = PCM_MOVE;

    // animation data
    m_emit_counter = 0.0f;
    m_emitter_living_time = 0.0f;
}

cParticle_Emitter* cParticle_Emitter::Copy(void) const
{
    cParticle_Emitter* particle_animation = new cParticle_Emitter(m_sprite_manager);
    particle_animation->Set_Based_On_Camera_Pos(m_emitter_based_on_camera_pos);
    particle_animation->Set_Particle_Based_On_Emitter_Pos(m_particle_based_on_emitter_pos);
    particle_animation->Set_Pos(m_start_pos_x, m_start_pos_y, 1);
    particle_animation->Set_Pos_Z(m_pos_z, m_pos_z_rand);
    particle_animation->Set_Image_Filename(fs::path(m_image_filename));
    particle_animation->Set_Emitter_Rect(m_rect);
    particle_animation->Set_Emitter_Time_to_Live(m_emitter_time_to_live);
    particle_animation->Set_Emitter_Iteration_Interval(m_emitter_iteration_interval);
    particle_animation->Set_Quota(m_emitter_quota);
    particle_animation->Set_Time_to_Live(m_time_to_live, m_time_to_live_rand);
    particle_animation->Set_Speed(m_vel, m_vel_rand);
    particle_animation->Set_Rotation(m_start_rot_x, m_start_rot_y, m_start_rot_z, 1);
    particle_animation->Set_Start_Rot_Z_Uses_Direction(m_start_rot_z_uses_direction);
    particle_animation->Set_Const_Rotation_X(m_const_rot_x, m_const_rot_x_rand);
    particle_animation->Set_Const_Rotation_Y(m_const_rot_y, m_const_rot_y_rand);
    particle_animation->Set_Const_Rotation_Z(m_const_rot_z, m_const_rot_z_rand);
    particle_animation->Set_Direction_Range(m_angle_start, m_angle_range);
    particle_animation->Set_Scale(m_size_scale, m_size_scale_rand);
    particle_animation->Set_Color(m_color, m_color_rand);
    particle_animation->Set_Horizontal_Gravity(m_gravity_x, m_gravity_x_rand);
    particle_animation->Set_Vertical_Gravity(m_gravity_y, m_gravity_y_rand);
    particle_animation->Set_Spawned(m_spawned);
    particle_animation->Set_Clip_Rect(m_clip_rect);
    particle_animation->Set_Clip_Mode(m_clip_mode);
    return particle_animation;
}

std::string cParticle_Emitter::Get_XML_Type_Name()
{
    return "";
}

xmlpp::Element* cParticle_Emitter::Save_To_XML_Node(xmlpp::Element* p_element)
{
    xmlpp::Element* p_node = cAnimation::Save_To_XML_Node(p_element);

    // particle image filename
    Add_Property(p_node, "particle_image", path_to_utf8(m_image_filename));
    // position z
    Add_Property(p_node, "pos_z",      m_pos_z);
    Add_Property(p_node, "pos_z_rand", m_pos_z_rand);
    // emitter based on camera pos
    Add_Property(p_node, "emitter_based_on_camera_pos", m_emitter_based_on_camera_pos);
    // particle based on emitter pos
    Add_Property(p_node, "particle_based_on_emitter_pos", m_particle_based_on_emitter_pos);
    // emitter rect (X and Y positions are saved by cSprite::Save_To_XML_Node())
    Add_Property(p_node, "sizex", static_cast<int>(m_start_rect.m_w));
    Add_Property(p_node, "sizey", static_cast<int>(m_start_rect.m_h));
    // emitter interval
    Add_Property(p_node, "emitter_time_to_live", m_emitter_time_to_live);
    Add_Property(p_node, "emitter_interval",     m_emitter_iteration_interval);
    // quota/count
    Add_Property(p_node, "quota", m_emitter_quota);
    // time to live
    Add_Property(p_node, "time_to_live", m_time_to_live);
    Add_Property(p_node, "time_to_live_rand", m_time_to_live_rand);
    // velocity
    Add_Property(p_node, "vel", m_vel);
    Add_Property(p_node, "vel_rand", m_vel_rand);
    // start rotation
    Add_Property(p_node, "rot_x", m_start_rot_x);
    Add_Property(p_node, "rot_y", m_start_rot_y);
    Add_Property(p_node, "rot_z", m_start_rot_z);
    Add_Property(p_node, "start_rot_z_uses_direction", m_start_rot_z_uses_direction);
    // constant rotation x
    Add_Property(p_node, "const_rot_x", m_const_rot_x);
    Add_Property(p_node, "const_rot_x_rand", m_const_rot_x_rand);
    // constant rotation y
    Add_Property(p_node, "const_rot_y", m_const_rot_y);
    Add_Property(p_node, "const_rot_y_rand", m_const_rot_y_rand);
    // constant rotation z
    Add_Property(p_node, "const_rot_z", m_const_rot_z);
    Add_Property(p_node, "const_rot_z_rand", m_const_rot_z_rand);
    // angle
    Add_Property(p_node, "angle_start", m_angle_start);
    Add_Property(p_node, "angle_range", m_angle_range);
    // scale
    Add_Property(p_node, "size_scale", m_size_scale);
    Add_Property(p_node, "size_scale_rand", m_size_scale_rand);
    // horizontal gravity
    Add_Property(p_node, "gravity_x", m_gravity_x);
    Add_Property(p_node, "gravity_x_rand", m_gravity_x_rand);
    // vertical gravity
    Add_Property(p_node, "gravity_y", m_gravity_y);
    Add_Property(p_node, "gravity_y_rand", m_gravity_y_rand);
    // clip rect
    Add_Property(p_node, "clip_x", static_cast<int>(m_clip_rect.m_x));
    Add_Property(p_node, "clip_y", static_cast<int>(m_clip_rect.m_y));
    Add_Property(p_node, "clip_w", static_cast<int>(m_clip_rect.m_w));
    Add_Property(p_node, "clip_h", static_cast<int>(m_clip_rect.m_h));
    // clip mode
    Add_Property(p_node, "clip_mode", m_clip_mode);

    return p_node;
}

void cParticle_Emitter::Pre_Update(void)
{
    if (!m_image || m_emitter_quota == 0 || Is_Float_Equal(m_time_to_live, 0.0f)) {
        return;
    }

    // remove old particles
    Clear(0);

    // update ahead
    const float old_speedfactor = pFramerate->m_speed_factor;
    pFramerate->m_speed_factor = 1.0f;

    float ttl;

    // todo : estimate this
    if (m_time_to_live < 0.0f) {
        ttl = 2.0f * speedfactor_fps;
    }
    // use time to live as seconds
    else {
        ttl = m_time_to_live * speedfactor_fps;
    }

    for (float i = 0.0f; i < ttl; i++) {
        Update_Particles();
        Update_Position();
    }

    pFramerate->m_speed_factor = old_speedfactor;
}

void cParticle_Emitter::Emit(void)
{
    if (!m_image) {
        return;
    }

    for (unsigned int i = 0; i < m_emitter_quota; i++) {
        cParticle* particle = new cParticle(this);

        // X Position
        float x = m_pos_x - (m_image->m_w * 0.5f);
        if (m_rect.m_w > 0.0f) {
            x += Get_Random_Float(0.0f, m_rect.m_w);
        }
        // Y Position
        float y = m_pos_y - (m_image->m_h * 0.5f);
        if (m_rect.m_h > 0.0f) {
            y += Get_Random_Float(0.0f, m_rect.m_h);
        }
        // Set Position
        particle->Set_Pos(x, y, 1);

        // Image
        particle->Set_Image(m_image, 1);

        // Z position
        particle->m_pos_z = m_pos_z;
        if (m_pos_z_rand > 0.0f) {
            particle->m_pos_z += Get_Random_Float(0.0f, m_pos_z_rand);
        }

        // angle range
        float dir_angle = m_angle_start;
        // start angle
        if (m_angle_range > 0.0f) {
            dir_angle += Get_Random_Float(0.0f, m_angle_range);
        }

        // Velocity
        float speed = m_vel;
        if (m_vel_rand > 0.0f) {
            speed += Get_Random_Float(0.0f, m_vel_rand);
        }
        // Set Velocity
        particle->Set_Velocity_From_Angle(dir_angle, speed, 1);

        // Start rotation
        particle->m_rot_x = m_start_rot_x;
        particle->m_rot_y = m_start_rot_y;
        particle->m_rot_z = m_start_rot_z;

        // Start direction is added to the z rotation
        if (m_start_rot_z_uses_direction) {
            particle->m_rot_z += dir_angle;
        }

        // Constant rotation
        particle->m_const_rot_x = m_const_rot_x;
        particle->m_const_rot_y = m_const_rot_y;
        particle->m_const_rot_z = m_const_rot_z;
        if (m_const_rot_x_rand > 0.0f) {
            particle->m_const_rot_x += Get_Random_Float(0.0f, m_const_rot_x_rand);
        }
        if (m_const_rot_y_rand > 0.0f) {
            particle->m_const_rot_y += Get_Random_Float(0.0f, m_const_rot_y_rand);
        }
        if (m_const_rot_z_rand > 0.0f) {
            particle->m_const_rot_z += Get_Random_Float(0.0f, m_const_rot_z_rand);
        }

        // Scale
        float scale = m_size_scale;
        if (m_size_scale_rand > 0.0f) {
            scale += Get_Random_Float(0.0f, m_size_scale_rand);
        }
        particle->Set_Scale(scale, 1);

        // Gravity
        float grav_x = m_gravity_x;
        if (m_gravity_x_rand > 0.0f) {
            grav_x += Get_Random_Float(0.0f, m_gravity_x_rand);
        }
        float grav_y = m_gravity_y;
        if (m_gravity_y_rand > 0.0f) {
            grav_y += Get_Random_Float(0.0f, m_gravity_y_rand);
        }
        // set Gravity
        particle->Set_Gravity(grav_x, grav_y);

        // Color
        particle->Set_Color(m_color);
        if (m_color_rand.red > 0) {
            particle->m_color.red += rand() % m_color_rand.red;
        }
        if (m_color_rand.green > 0) {
            particle->m_color.green += rand() % m_color_rand.green;
        }
        if (m_color_rand.blue > 0) {
            particle->m_color.blue += rand() % m_color_rand.blue;
        }
        if (m_color_rand.alpha > 0) {
            particle->m_color.alpha += rand() % m_color_rand.alpha;
        }

        // Time to life
        particle->m_time_to_live = m_time_to_live;
        if (m_time_to_live_rand > 0.0f) {
            particle->m_time_to_live += Get_Random_Float(0.0f, m_time_to_live_rand);
        }

        m_objects.push_back(particle);
    }
}

void cParticle_Emitter::Clear(bool reset /* = 1 */)
{
    // clear particles
    for (ParticleList::iterator itr = m_objects.begin(); itr != m_objects.end(); ++itr) {
        delete *itr;
    }

    m_objects.clear();

    // clear animation data
    m_emit_counter = 0.0f;

    if (reset) {
        m_emitter_living_time = 0.0f;
    }
}

void cParticle_Emitter::Update(void)
{
    Update_Valid_Update();

    if (!m_valid_update) {
        return;
    }

    Update_Position();

    m_emitter_living_time += pFramerate->m_speed_factor * (static_cast<float>(speedfactor_fps) * 0.001f);

    Update_Particles();
}

void cParticle_Emitter::Update_Particles(void)
{
    // update objects
    for (ParticleList::iterator itr = m_objects.begin(); itr != m_objects.end();) {
        // get object pointer
        cParticle* obj = (*itr);

        // update
        obj->Update();

        // if finished
        if (!obj->m_active) {
            itr = m_objects.erase(itr);
            delete obj;
        }
        // increment
        else {
            ++itr;
        }
    }

    // if able to emit or endless emitter
    if (m_emitter_living_time < m_emitter_time_to_live || Is_Float_Equal(m_emitter_time_to_live, -1.0f)) {
        // emit
        while (m_emit_counter > m_emitter_iteration_interval) {
            Emit();
            m_emit_counter -= m_emitter_iteration_interval;
        }

        m_emit_counter += pFramerate->m_speed_factor * (static_cast<float>(speedfactor_fps) * 0.001f);
    }
    // no particles are active
    else if (m_objects.empty()) {
        Set_Active(0);
    }
}

void cParticle_Emitter::Update_Position(void)
{
    if (m_emitter_based_on_camera_pos && !editor_enabled) {
        Set_Pos(m_start_pos_x + pActive_Camera->m_x, m_start_pos_y + (pActive_Camera->m_y + game_res_h));
    }

    // if clip rect is set
    if (m_clip_rect.m_w > 0.0f && m_clip_rect.m_h > 0.0f) {
        GL_rect clip_rect_final;

        clip_rect_final.m_x = m_start_pos_x + m_clip_rect.m_x;
        clip_rect_final.m_y = m_start_pos_y + m_clip_rect.m_y;

        if (!editor_enabled) {
            if (m_emitter_based_on_camera_pos) {
                clip_rect_final.m_x += pActive_Camera->m_x;
                clip_rect_final.m_y += pActive_Camera->m_y + game_res_h;
            }

            if (m_particle_based_on_emitter_pos > 0.0f) {
                clip_rect_final.m_x -= m_pos_x * m_particle_based_on_emitter_pos;
                clip_rect_final.m_y -= m_pos_y * m_particle_based_on_emitter_pos;
            }
        }

        clip_rect_final.m_w = m_clip_rect.m_w;
        clip_rect_final.m_h = m_clip_rect.m_h;

        Keep_Particles_In_Rect(clip_rect_final, m_clip_mode);
    }
}

void cParticle_Emitter::Draw(cSurface_Request* request /* = NULL */)
{
    if (!m_valid_draw) {
        return;
    }

    for (ParticleList::iterator itr = m_objects.begin(); itr != m_objects.end(); ++itr) {
        (*itr)->Draw();
    }

    if (editor_enabled) {
        if (!m_spawned) {
            // draw emitter rect
            GL_rect color_rect = GL_rect(m_start_pos_x - pActive_Camera->m_x, m_start_pos_y - pActive_Camera->m_y, m_col_rect.m_w, m_col_rect.m_h);

            cRect_Request* rect_request = new cRect_Request();
            pVideo->Draw_Rect(&color_rect, m_editor_pos_z, &darkgreen, rect_request);
            rect_request->m_filled = 0;
            pRenderer->Add(rect_request);

            // if active mouse object
            if (pMouseCursor->m_active_object == this) {
                // draw clip rect
                if (m_clip_rect.m_w > 0.0f && m_clip_rect.m_h > 0.0f) {
                    color_rect = GL_rect(m_start_pos_x + m_clip_rect.m_x - pActive_Camera->m_x, m_start_pos_y + m_clip_rect.m_y - pActive_Camera->m_y, m_clip_rect.m_w, m_clip_rect.m_h);

                    rect_request = new cRect_Request();
                    pVideo->Draw_Rect(&color_rect, m_editor_pos_z, &lightgrey, rect_request);
                    rect_request->m_filled = 0;
                    rect_request->m_line_width = 2.0f;
                    pRenderer->Add(rect_request);
                }
            }
        }
    }
}

void cParticle_Emitter::Keep_Particles_In_Rect(const GL_rect& clip_rect, ParticleClipMode mode /* = PCM_MOVE */)
{
    // temporary obj rect
    GL_rect obj_rect;

    // find particles that are not visible and move them to the opposite screen side
    for (ParticleList::iterator itr = m_objects.begin(); itr != m_objects.end(); ++itr) {
        // get animation particle pointer
        cParticle* obj = static_cast<cParticle*>(*itr);

        // set rectangle
        if (obj->m_scale_x != 1.0f) {
            obj_rect.m_x = obj->m_pos_x - ((obj->m_image->m_w * 0.5f) * (obj->m_scale_x - 1.0f));
            obj_rect.m_w = obj->m_rect.m_w * obj->m_scale_x;
        }
        else {
            obj_rect.m_x = obj->m_pos_x;
            obj_rect.m_w = obj->m_rect.m_w;
        }

        if (obj->m_scale_y != 1.0f) {
            obj_rect.m_y = obj->m_pos_y - ((obj->m_image->m_h * 0.5f) * (obj->m_scale_y - 1.0f));
            obj_rect.m_h = obj->m_rect.m_h * obj->m_scale_y;
        }
        else {
            obj_rect.m_y = obj->m_pos_y;
            obj_rect.m_h = obj->m_rect.m_h;
        }

        // out in left
        if (obj_rect.m_x + obj_rect.m_w < clip_rect.m_x) {
            // move to right
            if (mode == PCM_MOVE) {
                obj->Move(clip_rect.m_w + obj_rect.m_w - 1.0f, 0, 1);
            }
            else if (mode == PCM_REVERSE) {
                if (obj->m_velx < 0.0f) {
                    obj->Set_Velocity(-obj->m_velx, obj->m_vely);
                }
            }
            else if (mode == PCM_DELETE) {
                obj->Set_Active(0);
            }
        }
        // out in right
        else if (obj_rect.m_x > clip_rect.m_x + clip_rect.m_w) {
            // move to left
            if (mode == PCM_MOVE) {
                obj->Move(-clip_rect.m_w - obj_rect.m_w + 1.0f, 0, 1);
            }
            else if (mode == PCM_REVERSE) {
                if (obj->m_velx > 0.0f) {
                    obj->Set_Velocity(-obj->m_velx, obj->m_vely);
                }
            }
            else if (mode == PCM_DELETE) {
                obj->Set_Active(0);
            }
        }
        // out on top
        else if (obj_rect.m_y + obj_rect.m_h < clip_rect.m_y) {
            // move to bottom
            if (mode == PCM_MOVE) {
                obj->Move(0, clip_rect.m_h + obj_rect.m_h - 1.0f, 1);
            }
            else if (mode == PCM_REVERSE) {
                if (obj->m_vely < 0.0f) {
                    obj->Set_Velocity(obj->m_velx, -obj->m_vely);
                }
            }
            else if (mode == PCM_DELETE) {
                obj->Set_Active(0);
            }
        }
        // out on bottom
        else if (obj_rect.m_y > clip_rect.m_y + clip_rect.m_h) {
            // move to top
            if (mode == PCM_MOVE) {
                obj->Move(0, -clip_rect.m_h - obj_rect.m_h + 1.0f, 1);
            }
            else if (mode == PCM_REVERSE) {
                if (obj->m_vely > 0.0f) {
                    obj->Set_Velocity(obj->m_velx, -obj->m_vely);
                }
            }
            else if (mode == PCM_DELETE) {
                obj->Set_Active(0);
            }
        }
    }
}

bool cParticle_Emitter::Is_Update_Valid()
{
    // if not active
    if (!m_active) {
        return 0;
    }

    // if not in camera range
    if ((!m_emitter_based_on_camera_pos || editor_enabled) && !Is_In_Range()) {
        return 0;
    }

    // if editor enabled
    if (editor_enabled) {
        // if active mouse object
        if (pMouseCursor->m_active_object == this) {
            return 1;
        }

        return 0;
    }

    return 1;
}

bool cParticle_Emitter::Is_Draw_Valid(void)
{
    // if not visible
    if (!m_active) {
        return 0;
    }

    // if not in camera range
    if (!Is_In_Range()) {
        return 0;
    }

    return 1;
}

void cParticle_Emitter::Set_Image(cGL_Surface* img)
{
    if (!img) {
        return;
    }

    m_image = img;
}

void cParticle_Emitter::Set_Image_Filename(const fs::path& filename)
{
    // remember the filename for saving
    m_image_filename = filename;
    if (filename.is_absolute())
        m_image_filename = boost::filesystem::relative(pResource_Manager->Get_Game_Pixmaps_Directory(), filename);

    // set new image
    Set_Image(pVideo->Get_Surface(m_image_filename, 0));
}

void cParticle_Emitter::Set_Spawned(bool enable /* = 0 */)
{
    cAnimation::Set_Spawned(enable);

    if (!m_spawned) {
        // invalid width
        if (m_rect.m_w < 5.0f) {
            m_rect.m_w = 5.0f;
        }
        // invalid height
        if (m_rect.m_h < 5.0f) {
            m_rect.m_h = 5.0f;
        }
    }
}

void cParticle_Emitter::Set_Based_On_Camera_Pos(bool enable)
{
    m_emitter_based_on_camera_pos = enable;
}

void cParticle_Emitter::Set_Particle_Based_On_Emitter_Pos(float val)
{
    m_particle_based_on_emitter_pos = val;
}

void cParticle_Emitter::Set_Emitter_Rect(float x, float y, float w /* = 0 */, float h /* = 0 */)
{
    // hack: don't set x/y to 0 or the next Set_Pos call will overwrite the start position
    if (Is_Float_Equal(x, 0.0f) && Is_Float_Equal(y, 0.0f)) {
        y = -1.0f;
    }

    Set_Pos(x, y, 1);
    m_rect.m_w = w;
    m_rect.m_h = h;

    if (!m_spawned) {
        // invalid width
        if (m_rect.m_w < 5.0f) {
            m_rect.m_w = 5.0f;
        }
        // invalid height
        if (m_rect.m_h < 5.0f) {
            m_rect.m_h = 5.0f;
        }
    }

    m_col_rect.m_w = m_rect.m_w;
    m_col_rect.m_h = m_rect.m_h;
    m_start_rect.m_w = m_rect.m_w;
    m_start_rect.m_h = m_rect.m_h;
}

void cParticle_Emitter::Set_Emitter_Rect(const GL_rect& rect)
{
    Set_Emitter_Rect(rect.m_x, rect.m_y, rect.m_w, rect.m_h);
}

void cParticle_Emitter::Set_Emitter_Time_to_Live(float time)
{
    m_emitter_time_to_live = time;

    // reset time
    m_emitter_living_time = 0.0f;

    if (!m_active) {
        Set_Active(1);
    }
}

void cParticle_Emitter::Set_Emitter_Iteration_Interval(float time)
{
    m_emitter_iteration_interval = time;

    if (m_emitter_iteration_interval < 0.001f) {
        m_emitter_iteration_interval = 0.001f;
    }
}

void cParticle_Emitter::Set_Quota(unsigned int size)
{
    m_emitter_quota = Clamp<unsigned int>(size, 1, 1000);
}

void cParticle_Emitter::Set_Speed(float vel_base, float vel_random /* = 2 */)
{
    m_vel = vel_base;
    m_vel_rand = vel_random;
}

void cParticle_Emitter::Set_Start_Rot_Z_Uses_Direction(bool enable)
{
    m_start_rot_z_uses_direction = enable;
}

void cParticle_Emitter::Set_Const_Rotation_X(float rot, float rot_random /* = 0 */)
{
    m_const_rot_x = rot;
    m_const_rot_x_rand = rot_random;
}

void cParticle_Emitter::Set_Const_Rotation_Y(float rot, float rot_random /* = 0 */)
{
    m_const_rot_y = rot;
    m_const_rot_y_rand = rot_random;
}

void cParticle_Emitter::Set_Const_Rotation_Z(float rot, float rot_random /* = 0 */)
{
    m_const_rot_z = rot;
    m_const_rot_z_rand = rot_random;
}

void cParticle_Emitter::Set_Direction_Range(float start, float range /* = 0 */)
{
    m_angle_start = start;
    m_angle_range = range;
}

void cParticle_Emitter::Set_Scale(float nscale, float scale_random /* = 0 */)
{
    m_size_scale = nscale;
    m_size_scale_rand = scale_random;
}

void cParticle_Emitter::Set_Horizontal_Gravity(float start, float random /* = 0 */)
{
    m_gravity_x = start;
    m_gravity_x_rand = random;
}

void cParticle_Emitter::Set_Vertical_Gravity(float start, float random /* = 0 */)
{
    m_gravity_y = start;
    m_gravity_y_rand = random;
}

void cParticle_Emitter::Set_Color(const Color& col, const Color& col_rand /* = Color( static_cast<Uint8>(0) ) */)
{
    m_color = col;
    m_color_rand = col_rand;

#ifdef _DEBUG
    if (m_color.red + col_rand.red > 255) {
        printf("cParticle_Emitter::Set_Color random color red (%d) is too high for %s\n", col_rand.red, path_to_utf8(m_image_filename).c_str());
    }
    if (m_color.green + col_rand.green > 255) {
        printf("cParticle_Emitter::Set_Color random color green (%d) is too high for %s\n", col_rand.green, path_to_utf8(m_image_filename).c_str());
    }
    if (m_color.blue + col_rand.blue > 255) {
        printf("cParticle_Emitter::Set_Color random color blue (%d) is too high for %s\n", col_rand.blue, path_to_utf8(m_image_filename).c_str());
    }
    if (m_color.alpha + col_rand.alpha > 255) {
        printf("cParticle_Emitter::Set_Color random color alpha (%d) is too high for %s\n", col_rand.alpha, path_to_utf8(m_image_filename).c_str());
    }
#endif
}

void cParticle_Emitter::Set_Fading_Size(bool enable)
{
    m_fade_size = enable;
}

void cParticle_Emitter::Set_Fading_Alpha(bool enable)
{
    m_fade_alpha = enable;
}

void cParticle_Emitter::Set_Fading_Color(bool enable)
{
    m_fade_color = enable;
}

void cParticle_Emitter::Set_Blending(BlendingMode mode)
{
    m_blending = mode;
}

void cParticle_Emitter::Set_Clip_Rect(float x, float y, float w, float h)
{
    Set_Clip_Rect(GL_rect(x, y, w, h));
}

void cParticle_Emitter::Set_Clip_Rect(const GL_rect& rect)
{
    m_clip_rect = rect;

    // invalid width
    if (m_clip_rect.m_w < 0.0f) {
        m_clip_rect.m_w = 0.0f;
    }
    // invalid height
    if (m_clip_rect.m_h < 0.0f) {
        m_clip_rect.m_h = 0.0f;
    }
}

void cParticle_Emitter::Set_Clip_Mode(ParticleClipMode mode)
{
    m_clip_mode = mode;
}

void cParticle_Emitter::Editor_Activate(void)
{
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    // position z
    CEGUI::Editbox* editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "emitter_pos_z_base"));
    Editor_Add(UTF8_("Position z"), UTF8_("Initial depth position. Use values from 0.00011 to 0.12."), editbox, 150);

    editbox->setValidationString("[+]?[0-9]*\\.?[0-9]*");
    editbox->setText(float_to_string(m_pos_z, 6, 0));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cParticle_Emitter::Editor_Pos_Z_Base_Text_Changed, this));

    // position z rand
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "emitter_pos_z_rand"));
    Editor_Add(UTF8_("Random"), UTF8_("Additional random value"), editbox, 150, 28, 0);

    editbox->setValidationString("[+]?[0-9]*\\.?[0-9]*");
    editbox->setText(float_to_string(m_pos_z_rand, 6, 0));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cParticle_Emitter::Editor_Pos_Z_Rand_Text_Changed, this));

    // image filename
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "emitter_image_filename"));
    Editor_Add(UTF8_("Filename"), UTF8_("Image filename"), editbox, 360);

    editbox->setText(path_to_utf8(m_image_filename).c_str());
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cParticle_Emitter::Editor_Filename_Text_Changed, this));

    // emitter position based on camera pos
    CEGUI::Checkbox* checkbox = static_cast<CEGUI::Checkbox*>(wmgr.createWindow("TaharezLook/Checkbox", "emitter_based_on_camera_pos"));
    Editor_Add(UTF8_("Based on camera pos."), UTF8_("The emitter position is based on the camera position"), checkbox, 50);

    checkbox->setSelected(m_emitter_based_on_camera_pos);
    checkbox->subscribeEvent(CEGUI::Checkbox::EventCheckStateChanged, CEGUI::Event::Subscriber(&cParticle_Emitter::Editor_Emitter_Based_On_Camera_Pos_Changed, this));

    // particle position based on emitter pos
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "emitter_particle_based_on_emitter_pos"));
    Editor_Add(UTF8_("Particles Based on Emitter pos."), UTF8_("The particle position is based on the emitter position"), editbox, 150);

    editbox->setValidationString("[+]?[0-9]*\\.?[0-9]*");
    editbox->setText(float_to_string(m_particle_based_on_emitter_pos, 6, 0));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cParticle_Emitter::Editor_Particle_Based_On_Emitter_Pos_Text_Changed, this));

    // emitter width
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "emitter_width"));
    Editor_Add(UTF8_("Emitter width"), UTF8_("Emitter width in which the particles spawn"), editbox, 150);

    editbox->setValidationString("[+]?[0-9]*\\.?[0-9]*");
    editbox->setText(float_to_string(m_rect.m_w, 6, 0));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cParticle_Emitter::Editor_Emitter_Width_Text_Changed, this));

    // emitter height
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "emitter_height"));
    Editor_Add(UTF8_("Height"), UTF8_("Emitter height in which the particles spawn"), editbox, 150, 28, 0);

    editbox->setValidationString("[+]?[0-9]*\\.?[0-9]*");
    editbox->setText(float_to_string(m_rect.m_h, 6, 0));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cParticle_Emitter::Editor_Emitter_Height_Text_Changed, this));

    // emitter time to live
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "emitter_time_to_live"));
    Editor_Add(UTF8_("Emitter TTL"), UTF8_("Emitter time to live. Set -1 for infinite."), editbox, 150);

    editbox->setValidationString("[+-]?[0-9]*\\.?[0-9]*");
    editbox->setText(float_to_string(m_emitter_time_to_live, 6, 0));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cParticle_Emitter::Editor_Emitter_Time_To_Live_Text_Changed, this));

    // emitter interval
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "emitter_interval"));
    Editor_Add(UTF8_("Emitter interval"), UTF8_("Time between spawning particles. Amount is the quota."), editbox, 150);

    editbox->setValidationString("[+]?[0-9]*\\.?[0-9]*");
    editbox->setText(float_to_string(m_emitter_iteration_interval, 6, 0));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cParticle_Emitter::Editor_Emitter_Interval_Text_Changed, this));

    // quota
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "emitter_quota"));
    Editor_Add(UTF8_("Quota"), UTF8_("The amount of particles to spawn for an interval"), editbox, 50, 28, 0);

    editbox->setValidationString("^[+]?\\d*$");
    editbox->setText(int_to_string(m_emitter_quota));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cParticle_Emitter::Editor_Quota_Text_Changed, this));

    // time to live
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "emitter_ttl_base"));
    Editor_Add(UTF8_("TTL"), UTF8_("Particle time to live(TTL)"), editbox, 150);

    editbox->setValidationString("[+]?[0-9]*\\.?[0-9]*");
    editbox->setText(float_to_string(m_time_to_live, 6, 0));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cParticle_Emitter::Editor_TTL_Base_Text_Changed, this));

    // time to live rand
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "emitter_ttl_rand"));
    Editor_Add(UTF8_("Random"), UTF8_("Additional random value"), editbox, 150, 28, 0);

    editbox->setValidationString("[+]?[0-9]*\\.?[0-9]*");
    editbox->setText(float_to_string(m_time_to_live_rand, 6, 0));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cParticle_Emitter::Editor_TTL_Rand_Text_Changed, this));

    // velocity
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "emitter_vel_base"));
    Editor_Add(UTF8_("Velocity"), UTF8_("Initial particle velocity or speed"), editbox, 150);

    editbox->setValidationString("[+]?[0-9]*\\.?[0-9]*");
    editbox->setText(float_to_string(m_vel, 6, 0));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cParticle_Emitter::Editor_Velocity_Base_Text_Changed, this));

    // velocity rand
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "emitter_vel_rand"));
    Editor_Add(UTF8_("Random"), UTF8_("Additional random value"), editbox, 150, 28, 0);

    editbox->setValidationString("[+]?[0-9]*\\.?[0-9]*");
    editbox->setText(float_to_string(m_vel_rand, 6, 0));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cParticle_Emitter::Editor_Velocity_Rand_Text_Changed, this));

    // start rotation x
    /*editbox = static_cast<CEGUI::Editbox *>(wmgr.createWindow( "TaharezLook/Editbox", "emitter_rotation_x_base" ));
    Editor_Add( UTF8_("Rotation x"), UTF8_("Initial rotation x"), editbox, 150 );

    editbox->setValidationString( "[+]?[0-9]*\\.?[0-9]*" );
    editbox->setText( float_to_string( start_rotx, 6, 0 ) );
    editbox->subscribeEvent( CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber( &cParticle_Emitter::Editor_Rotation_X_Base_Text_Changed, this ) );
    */
    // start rotation y
    /*editbox = static_cast<CEGUI::Editbox *>(wmgr.createWindow( "TaharezLook/Editbox", "emitter_rotation_y_base" ));
    Editor_Add( UTF8_("Rotation y"), UTF8_("Initial rotation y"), editbox, 150 );

    editbox->setValidationString( "[+]?[0-9]*\\.?[0-9]*" );
    editbox->setText( float_to_string( start_roty, 6, 0 ) );
    editbox->subscribeEvent( Editbox::EventTextChanged, CEGUI::Event::Subscriber( &cParticle_Emitter::Editor_Rotation_Y_Base_Text_Changed, this ) );
    */

    // start rotation z
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "emitter_rotation_z_base"));
    Editor_Add(UTF8_("Rotation z"), UTF8_("Initial rotation z"), editbox, 150);

    editbox->setValidationString("[+-]?[0-9]*\\.?[0-9]*");
    editbox->setText(float_to_string(m_start_rot_z, 6, 0));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cParticle_Emitter::Editor_Rotation_Z_Base_Text_Changed, this));

    // start direction is added to the z rotation
    checkbox = static_cast<CEGUI::Checkbox*>(wmgr.createWindow("TaharezLook/Checkbox", "emitter_start_rot_z_uses_direction"));
    Editor_Add(UTF8_("Add direction"), UTF8_("Start direction is added to the z rotation"), checkbox, 50, 28, 0);

    checkbox->setSelected(m_start_rot_z_uses_direction);
    checkbox->subscribeEvent(CEGUI::Checkbox::EventCheckStateChanged, CEGUI::Event::Subscriber(&cParticle_Emitter::Editor_Start_Rot_Z_Uses_Direction_Changed, this));

    // constant rotation x
    /*editbox = static_cast<CEGUI::Editbox *>(wmgr.createWindow( "TaharezLook/Editbox", "emitter_const_rotation_x_base" ));
    Editor_Add( UTF8_("Const. rotation x"), UTF8_("Initial constant rotation x"), editbox, 150 );

    editbox->setValidationString( "[+]?[0-9]*\\.?[0-9]*" );
    editbox->setText( float_to_string( m_const_rot_x, 6, 0 ) );
    editbox->subscribeEvent( CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber( &cParticle_Emitter::Editor_Const_Rotation_X_Base_Text_Changed, this ) );
    */
    // constant rotation x rand
    /*editbox = static_cast<CEGUI::Editbox *>(wmgr.createWindow( "TaharezLook/Editbox", "emitter_const_rotation_x_rand" ));
    Editor_Add( UTF8_("Random"), UTF8_("Additional random value"), editbox, 150, 28, 0 );

    editbox->setValidationString( "[+]?[0-9]*\\.?[0-9]*" );
    editbox->setText( float_to_string( m_const_rot_x_rand, 6, 0 ) );
    editbox->subscribeEvent( CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber( &cParticle_Emitter::Editor_Const_Rotation_X_Rand_Text_Changed, this ) );
    */
    // constant rotation y
    /*editbox = static_cast<CEGUI::Editbox *>(wmgr.createWindow( "TaharezLook/Editbox", "emitter_const_rotation_y_base" ));
    Editor_Add( UTF8_("Const. rotation y"), UTF8_("Initial constant rotation y"), editbox, 150 );

    editbox->setValidationString( "[+]?[0-9]*\\.?[0-9]*" );
    editbox->setText( float_to_string( m_const_rot_y, 6, 0 ) );
    editbox->subscribeEvent( CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber( &cParticle_Emitter::Editor_Const_Rotation_Y_Base_Text_Changed, this ) );
    */
    // constant rotation y rand
    /*editbox = static_cast<CEGUI::Editbox *>(wmgr.createWindow( "TaharezLook/Editbox", "emitter_const_rotation_y_rand" ));
    Editor_Add( UTF8_("Random"), UTF8_("Additional random value"), editbox, 150, 28, 0 );

    editbox->setValidationString( "[+]?[0-9]*\\.?[0-9]*" );
    editbox->setText( float_to_string( m_const_rot_y_rand, 6, 0 ) );
    editbox->subscribeEvent( CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber( &cParticle_Emitter::Editor_Const_Rotation_Y_Rand_Text_Changed, this ) );
    */
    // constant rotation z
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "emitter_const_rotation_z_base"));
    Editor_Add(UTF8_("Const. rotation z"), UTF8_("Initial constant rotation z"), editbox, 150);

    editbox->setValidationString("[+-]?[0-9]*\\.?[0-9]*");
    editbox->setText(float_to_string(m_const_rot_z, 6, 0));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cParticle_Emitter::Editor_Const_Rotation_Z_Base_Text_Changed, this));

    // constant rotation z rand
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "emitter_const_rotation_z_rand"));
    Editor_Add(UTF8_("Random"), UTF8_("Additional random value"), editbox, 150, 28, 0);

    editbox->setValidationString("[+]?[0-9]*\\.?[0-9]*");
    editbox->setText(float_to_string(m_const_rot_z_rand, 6, 0));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cParticle_Emitter::Editor_Const_Rotation_Z_Rand_Text_Changed, this));

    // direction
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "emitter_direction_base"));
    Editor_Add(UTF8_("Direction"), UTF8_("Initial direction/angle"), editbox, 150);

    editbox->setValidationString("[+]?[0-9]*\\.?[0-9]*");
    editbox->setText(float_to_string(m_angle_start, 6, 0));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cParticle_Emitter::Editor_Direction_Base_Text_Changed, this));

    // direction rand
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "emitter_direction_rand"));
    Editor_Add(UTF8_("Random"), UTF8_("Additional random value"), editbox, 150, 28, 0);

    editbox->setValidationString("[+]?[0-9]*\\.?[0-9]*");
    editbox->setText(float_to_string(m_angle_range, 6, 0));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cParticle_Emitter::Editor_Direction_Rand_Text_Changed, this));

    // scale
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "emitter_scale_base"));
    Editor_Add(UTF8_("Scale"), UTF8_("Initial size scale"), editbox, 150);

    editbox->setValidationString("[+]?[0-9]*\\.?[0-9]*");
    editbox->setText(float_to_string(m_size_scale, 6, 0));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cParticle_Emitter::Editor_Scale_Base_Text_Changed, this));

    // scale rand
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "emitter_scale_rand"));
    Editor_Add(UTF8_("Random"), UTF8_("Additional random value"), editbox, 150, 28, 0);

    editbox->setValidationString("[+]?[0-9]*\\.?[0-9]*");
    editbox->setText(float_to_string(m_size_scale_rand, 6, 0));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cParticle_Emitter::Editor_Scale_Rand_Text_Changed, this));

    // horizontal gravity
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "emitter_hor_gravity_base"));
    Editor_Add(UTF8_("Hor gravity"), UTF8_("Initial horizontal gravity"), editbox, 150);

    editbox->setValidationString("[+-]?[0-9]*\\.?[0-9]*");
    editbox->setText(float_to_string(m_gravity_x, 6, 0));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cParticle_Emitter::Editor_Horizontal_Gravity_Base_Text_Changed, this));

    // horizontal gravity rand
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "emitter_hor_gravity_rand"));
    Editor_Add(UTF8_("Random"), UTF8_("Additional random value"), editbox, 150, 28, 0);

    editbox->setValidationString("[+]?[0-9]*\\.?[0-9]*");
    editbox->setText(float_to_string(m_gravity_x_rand, 6, 0));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cParticle_Emitter::Editor_Horizontal_Gravity_Rand_Text_Changed, this));

    // vertical gravity
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "emitter_ver_gravity_base"));
    Editor_Add(UTF8_("Ver gravity"), UTF8_("Initial vertical gravity"), editbox, 150);

    editbox->setValidationString("[+-]?[0-9]*\\.?[0-9]*");
    editbox->setText(float_to_string(m_gravity_y, 6, 0));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cParticle_Emitter::Editor_Vertical_Gravity_Base_Text_Changed, this));

    // vertical gravity rand
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "emitter_ver_gravity_rand"));
    Editor_Add(UTF8_("Random"), UTF8_("Additional random value"), editbox, 150, 28, 0);

    editbox->setValidationString("[+]?[0-9]*\\.?[0-9]*");
    editbox->setText(float_to_string(m_gravity_y_rand, 6, 0));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cParticle_Emitter::Editor_Vertical_Gravity_Rand_Text_Changed, this));

    // clip rect x
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "emitter_clip_rect_x"));
    Editor_Add(UTF8_("Clip rect x"), UTF8_("Clipping rectangle position x"), editbox, 150);

    editbox->setValidationString("[+]?[0-9]*\\.?[0-9]*");
    editbox->setText(float_to_string(m_clip_rect.m_x, 6, 0));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cParticle_Emitter::Editor_Clip_Rect_X_Text_Changed, this));

    // clip rect w
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "emitter_clip_rect_w"));
    Editor_Add(UTF8_("Width"), UTF8_("Clipping rectangle width"), editbox, 150, 28, 0);

    editbox->setValidationString("[+]?[0-9]*\\.?[0-9]*");
    editbox->setText(float_to_string(m_clip_rect.m_w, 6, 0));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cParticle_Emitter::Editor_Clip_Rect_W_Text_Changed, this));

    // clip rect y
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "emitter_clip_rect_y"));
    Editor_Add(UTF8_("Clip rect y"), UTF8_("Clipping rectangle position y"), editbox, 150);

    editbox->setValidationString("[+]?[0-9]*\\.?[0-9]*");
    editbox->setText(float_to_string(m_clip_rect.m_y, 6, 0));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cParticle_Emitter::Editor_Clip_Rect_Y_Text_Changed, this));

    // clip rect h
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "emitter_clip_rect_h"));
    Editor_Add(UTF8_("Height"), UTF8_("Clipping rectangle height"), editbox, 150, 28, 0);

    editbox->setValidationString("[+]?[0-9]*\\.?[0-9]*");
    editbox->setText(float_to_string(m_clip_rect.m_h, 6, 0));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cParticle_Emitter::Editor_Clip_Rect_H_Text_Changed, this));

    // clip mode
    CEGUI::Combobox* combobox = static_cast<CEGUI::Combobox*>(wmgr.createWindow("TaharezLook/Combobox", "emitter_clip_mode"));
    Editor_Add(UTF8_("Clip mode"), UTF8_("Clipping mode if particle is out the rectangle"), combobox, 100, 105);

    combobox->addItem(new CEGUI::ListboxTextItem("move"));
    combobox->addItem(new CEGUI::ListboxTextItem("reverse"));
    combobox->addItem(new CEGUI::ListboxTextItem("delete"));

    if (m_clip_mode == PCM_MOVE) {
        combobox->setText("move");
    }
    else if (m_clip_mode == PCM_REVERSE) {
        combobox->setText("reverse");
    }
    else if (m_clip_mode == PCM_DELETE) {
        combobox->setText("delete");
    }

    combobox->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cParticle_Emitter::Editor_Clip_Mode_Select, this));

    // init
    Editor_Init();
}

bool cParticle_Emitter::Editor_Filename_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Image_Filename(utf8_to_path(str_text));
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Pos_Z_Base_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Pos_Z(string_to_float(str_text), m_pos_z_rand);
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Pos_Z_Rand_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Pos_Z(m_pos_z, string_to_float(str_text));
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Emitter_Based_On_Camera_Pos_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    bool enabled = static_cast<CEGUI::Checkbox*>(windowEventArgs.window)->isSelected();

    Set_Based_On_Camera_Pos(enabled);
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Particle_Based_On_Emitter_Pos_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Particle_Based_On_Emitter_Pos(string_to_float(str_text));
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Emitter_Width_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Emitter_Rect(m_start_pos_x, m_start_pos_y, string_to_float(str_text), m_start_rect.m_h);
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Emitter_Height_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Emitter_Rect(m_start_pos_x, m_start_pos_y, m_start_rect.m_w, string_to_float(str_text));
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Emitter_Time_To_Live_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Emitter_Time_to_Live(string_to_float(str_text));
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Emitter_Interval_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Emitter_Iteration_Interval(string_to_float(str_text));
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Quota_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Quota(string_to_int(str_text));
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_TTL_Base_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Time_to_Live(string_to_float(str_text), m_time_to_live_rand);
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_TTL_Rand_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Time_to_Live(m_time_to_live, string_to_float(str_text));
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Velocity_Base_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Speed(string_to_float(str_text), m_vel_rand);
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Velocity_Rand_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Speed(m_vel, string_to_float(str_text));
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Rotation_X_Base_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Rotation_X(string_to_float(str_text), 1);
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Rotation_Y_Base_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Rotation_Y(string_to_float(str_text), 1);
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Start_Rot_Z_Uses_Direction_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    bool enabled = static_cast<CEGUI::Checkbox*>(windowEventArgs.window)->isSelected();

    Set_Start_Rot_Z_Uses_Direction(enabled);
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Rotation_Z_Base_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Rotation_Z(string_to_float(str_text), 1);
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Const_Rotation_X_Base_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Const_Rotation_X(string_to_float(str_text), m_const_rot_z_rand);
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Const_Rotation_X_Rand_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Const_Rotation_X(m_const_rot_z, string_to_float(str_text));
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Const_Rotation_Y_Base_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Const_Rotation_Y(string_to_float(str_text), m_const_rot_z_rand);
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Const_Rotation_Y_Rand_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Const_Rotation_Y(m_const_rot_z, string_to_float(str_text));
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Const_Rotation_Z_Base_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Const_Rotation_Z(string_to_float(str_text), m_const_rot_z_rand);
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Const_Rotation_Z_Rand_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Const_Rotation_Z(m_const_rot_z, string_to_float(str_text));
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Direction_Base_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Direction_Range(string_to_float(str_text), m_angle_range);
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Direction_Rand_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Direction_Range(m_angle_start, string_to_float(str_text));
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Scale_Base_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Scale(string_to_float(str_text), m_size_scale_rand);
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Scale_Rand_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Scale(m_size_scale, string_to_float(str_text));
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Horizontal_Gravity_Base_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Horizontal_Gravity(string_to_float(str_text), m_gravity_x_rand);
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Horizontal_Gravity_Rand_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Horizontal_Gravity(m_gravity_x, string_to_float(str_text));
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Vertical_Gravity_Base_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Vertical_Gravity(string_to_float(str_text), m_gravity_y_rand);
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Vertical_Gravity_Rand_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Vertical_Gravity(m_gravity_y, string_to_float(str_text));
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Clip_Rect_X_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Clip_Rect(string_to_float(str_text), m_clip_rect.m_y, m_clip_rect.m_w, m_clip_rect.m_h);
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Clip_Rect_Y_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Clip_Rect(m_clip_rect.m_x, string_to_float(str_text), m_clip_rect.m_w, m_clip_rect.m_h);
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Clip_Rect_W_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Clip_Rect(m_clip_rect.m_x, m_clip_rect.m_y, string_to_float(str_text), m_clip_rect.m_h);
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Clip_Rect_H_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Clip_Rect(m_clip_rect.m_x, m_clip_rect.m_y, m_clip_rect.m_w, string_to_float(str_text));
    Pre_Update();

    return 1;
}

bool cParticle_Emitter::Editor_Clip_Mode_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();
    std::string str_text = item->getText().c_str();

    if (str_text.compare("move") == 0) {
        Set_Clip_Mode(PCM_MOVE);
    }
    else if (str_text.compare("reverse") == 0) {
        Set_Clip_Mode(PCM_REVERSE);
    }
    else if (str_text.compare("delete") == 0) {
        Set_Clip_Mode(PCM_DELETE);
    }

    Pre_Update();

    return 1;
}

/* *** *** *** *** *** cAnimation_Manager *** *** *** *** *** *** *** *** *** *** *** *** */

cAnimation_Manager::cAnimation_Manager(void)
    : cObject_Manager<cAnimation>()
{

}

cAnimation_Manager::~cAnimation_Manager(void)
{
    cAnimation_Manager::Delete_All();
}

void cAnimation_Manager::Update(void)
{
    for (cAnimation_List::iterator itr = objects.begin(); itr != objects.end();) {
        // get object pointer
        cAnimation* obj = (*itr);

        // update
        obj->Update();

        // delete if finished
        if (!obj->m_active) {
            itr = objects.erase(itr);
            delete obj;
        }
        // increment
        else {
            ++itr;
        }
    }
}

void cAnimation_Manager::Draw(void)
{
    for (cAnimation_List::iterator itr = objects.begin(); itr != objects.end(); ++itr) {
        (*itr)->Draw();
    }
}

void cAnimation_Manager::Add(cAnimation* animation)
{
    if (!animation) {
        return;
    }

    /* todo : particle animations should not set emitter TTL to 0
     * many spawned particle emitters only emit one particle with an emitter which is very slow
    */
    cObject_Manager<cAnimation>::Add(animation);
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

cAnimation_Manager* pActive_Animation_Manager = NULL;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

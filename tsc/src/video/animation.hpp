/***************************************************************************
 * animation.h
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

#ifndef TSC_ANIMATION_HPP
#define TSC_ANIMATION_HPP

#include "../objects/animated_sprite.hpp"
#include "../core/obj_manager.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** Particle blending definitions *** *** *** *** *** *** *** *** *** *** */

    enum BlendingMode {
        BLEND_NONE,
        BLEND_ADD,
        BLEND_DRIVE
        // todo : more
    };

    /* *** *** *** *** *** *** *** Base Animation class *** *** *** *** *** *** *** *** *** *** */

    class cAnimation : public cAnimated_Sprite {
    public:
        cAnimation(cSprite_Manager* sprite_manager, std::string type_name = "sprite");
        virtual ~cAnimation(void);

        // initialize animation
        virtual void Init_Anim(void);
        // update animation
        virtual void Update(void);
        // draw animation
        virtual void Draw(cSurface_Request* request = NULL);

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "Animation"), &Scripting::rtTSC_Scriptable, this));
        }

        // Set time to live for Objects in seconds
        void Set_Time_to_Live(float time, float time_rand = 0.0f);
        /* Set speed of fading out ( 0.01 - 100 )
        * the lower the longer it takes
        * Note : only useful for non particle animation objects
        */
        void Set_Fading_Speed(float speed);
        // set z position
        virtual void Set_Pos_Z(float pos, float pos_rand = 0.0f);

        // z random position
        float m_pos_z_rand;
        // fading out speed
        float m_fading_speed;
        // object time to live
        float m_time_to_live;
        float m_time_to_live_rand;
    };

    /* *** *** *** *** *** *** *** *** Blinking points *** *** *** *** *** *** *** *** *** */

    class cAnimation_Goldpiece : public cAnimation {
    public:
        cAnimation_Goldpiece(cSprite_Manager* sprite_manager, float posx, float posy, float height = 40.0f, float width = 20.0f);
        virtual ~cAnimation_Goldpiece(void);

        // update
        virtual void Update(void);
        // draw
        virtual void Draw(cSurface_Request* request = NULL);

        typedef vector<cSprite*> BlinkPointList;
        BlinkPointList m_objects;
    };

    /* *** *** *** *** *** *** *** Fireball Animation *** *** *** *** *** *** *** *** *** *** */

    class cAnimation_Fireball_Item : public cAnimated_Sprite {
    public:
        cAnimation_Fireball_Item(cSprite_Manager* sprite_manager)
            : cAnimated_Sprite(sprite_manager)
        {
            m_counter = 0.0f;
        }

        virtual ~cAnimation_Fireball_Item(void) {}

        // lifetime
        float m_counter;
    };

    class cAnimation_Fireball : public cAnimation {
    public:
        cAnimation_Fireball(cSprite_Manager* sprite_manager, float posx, float posy, unsigned int power = 5);
        virtual ~cAnimation_Fireball(void);

        // update
        virtual void Update(void);
        // draw
        virtual void Draw(cSurface_Request* request = NULL);

        typedef vector<cAnimation_Fireball_Item*> FireAnimList;
        FireAnimList m_objects;
    };

    /* *** *** *** *** *** *** *** Particle Emitter item *** *** *** *** *** *** *** *** *** *** */

// forward declare
    class cParticle_Emitter;

// Particle Item
    class cParticle : public cMovingSprite {
    public:
        cParticle(cParticle_Emitter* parent);
        virtual ~cParticle(void);

        // update
        virtual void Update(void);
        // draw
        virtual void Draw(cSurface_Request* request = NULL);

        // set gravity
        void Set_Gravity(float x, float y);

        // parent particle emitter
        cParticle_Emitter* m_parent;
        // time to live
        float m_time_to_live;
        // constant rotation
        float m_const_rot_x;
        float m_const_rot_y;
        float m_const_rot_z;
        // particle gravity
        float m_gravity_x;
        float m_gravity_y;

        // fading position value
        float m_fade_pos;
    };

    /* *** *** *** *** *** *** *** Particle Emitter *** *** *** *** *** *** *** *** *** *** */

    enum ParticleClipMode {
        // move to the other side
        PCM_MOVE = 0,
        // reverse the velocity
        PCM_REVERSE = 1,
        PCM_DELETE = 2
    };

    class cParticle_Emitter : public cAnimation {
    public:
        // constructor
        cParticle_Emitter(cSprite_Manager* sprite_manager);
        // create from stream
        cParticle_Emitter(XmlAttributes& attributes, cSprite_Manager* sprite_manager);
        // destructor
        virtual ~cParticle_Emitter(void);

        // Init
        virtual void Init(void);
        // copy
        virtual cParticle_Emitter* Copy(void) const;

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "ParticleEmitter"), &Scripting::rtTSC_Scriptable, this));
        }

        // pre-update animation
        void Pre_Update(void);
        // Emit Particles
        virtual void Emit(void);
        // Clear particles and animation data
        virtual void Clear(bool reset = 1);

        // update
        virtual void Update(void);
        /* update and emit particles
         * does not update emitter living time
        */
        void Update_Particles(void);
        // update position and clipping
        void Update_Position(void);
        // Draw everything
        virtual void Draw(cSurface_Request* request = NULL);

        // keep particles in the given rectangle
        void Keep_Particles_In_Rect(const GL_rect& clip_rect, ParticleClipMode mode = PCM_MOVE);

        // if update is valid for the current state
        virtual bool Is_Update_Valid();
        // if draw is valid for the current state and position
        virtual bool Is_Draw_Valid(void);

        /* Set image
         * does not set the image filename
        */
        virtual void Set_Image(cGL_Surface* img);
        // Set the image with the filename. `filename' should
        // be relative to the pixmaps/ directory (but it’s converted if
        // it’s an absolute path).
        virtual void Set_Image_Filename(const boost::filesystem::path& filename);
        /* set if spawned
         * if set it is not saved in the level/world file
        */
        virtual void Set_Spawned(bool enable = 0);
        // Set if the position is based on the camera position
        void Set_Based_On_Camera_Pos(bool enable);
        // Set if particles position is based on the emitter position
        void Set_Particle_Based_On_Emitter_Pos(float val);
        // Set the Emitter rect
        void Set_Emitter_Rect(float x, float y, float w = 0.0f, float h = 0.0f);
        void Set_Emitter_Rect(const GL_rect& rect);
        /* Set time to live for the Emitter in seconds
         * set -1 for infinite
        */
        void Set_Emitter_Time_to_Live(float time);
        // Set time between Iterations
        void Set_Emitter_Iteration_Interval(float time);
        // Set Particle Count/Quota
        void Set_Quota(unsigned int size);
        // Set speed ( 0 - 100 )
        void Set_Speed(float vel_base, float vel_random = 2.0f);
        // Set start rotation z uses start direction
        void Set_Start_Rot_Z_Uses_Direction(bool enable);
        // Set x constant rotation
        void Set_Const_Rotation_X(float rot, float rot_random = 0.0f);
        // Set y constant rotation
        void Set_Const_Rotation_Y(float rot, float rot_random = 0.0f);
        // Set z constant rotation
        void Set_Const_Rotation_Z(float rot, float rot_random = 0.0f);
        /* Set direction range ( 0 - 360 )
         * 0 : Right, 90 Down, 180 Left, 270 Up
        */
        void Set_Direction_Range(float start, float range = 0.0f);
        // Set image scale ( 0.01 - 100 )
        virtual void Set_Scale(float nscale, float scale_random = 0.0f);
        // Set horizontal gravity
        void Set_Horizontal_Gravity(float start, float random = 0.0f);
        // Set vertical gravity
        void Set_Vertical_Gravity(float start, float random = 0.0f);
        // Set the Color
        virtual void Set_Color(const Color& col, const Color& col_rand = Color(static_cast<Uint8>(0), 0, 0, 0));
        // Set fading type
        void Set_Fading_Size(bool enable);
        void Set_Fading_Alpha(bool enable);
        void Set_Fading_Color(bool enable);
        // Set blending mode
        virtual void Set_Blending(BlendingMode mode);
        // set the clipping rect
        void Set_Clip_Rect(float x, float y, float w, float h);
        void Set_Clip_Rect(const GL_rect& rect);
        // set the clip mode
        void Set_Clip_Mode(ParticleClipMode mode);

        // editor todo : start rotation x/y/z rand, color, color_rand
        // editor activation
        virtual void Editor_Activate(void);
        // editor events
        bool Editor_Pos_Z_Base_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Pos_Z_Rand_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Filename_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Emitter_Based_On_Camera_Pos_Changed(const CEGUI::EventArgs& event);
        bool Editor_Particle_Based_On_Emitter_Pos_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Emitter_Width_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Emitter_Height_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Emitter_Time_To_Live_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Emitter_Interval_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Quota_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_TTL_Base_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_TTL_Rand_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Velocity_Base_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Velocity_Rand_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Rotation_X_Base_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Rotation_Y_Base_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Rotation_Z_Base_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Start_Rot_Z_Uses_Direction_Changed(const CEGUI::EventArgs& event);
        bool Editor_Const_Rotation_X_Base_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Const_Rotation_X_Rand_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Const_Rotation_Y_Base_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Const_Rotation_Y_Rand_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Const_Rotation_Z_Base_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Const_Rotation_Z_Rand_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Direction_Base_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Direction_Rand_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Scale_Base_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Scale_Rand_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Horizontal_Gravity_Base_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Horizontal_Gravity_Rand_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Vertical_Gravity_Base_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Vertical_Gravity_Rand_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Clip_Rect_X_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Clip_Rect_Y_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Clip_Rect_W_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Clip_Rect_H_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Clip_Mode_Select(const CEGUI::EventArgs& event);

        // Particle items
        typedef vector<cParticle*> ParticleList;
        ParticleList m_objects;

        // filename of the particle image
        boost::filesystem::path m_image_filename;
        // if emitter position is based on the camera position
        bool m_emitter_based_on_camera_pos;
        // if particles position is based on the emitter position
        float m_particle_based_on_emitter_pos;
        // emitter time to live
        float m_emitter_time_to_live;
        // emitter iteration interval
        float m_emitter_iteration_interval;
        // emitter object quota
        unsigned int m_emitter_quota;

        // velocity
        float m_vel;
        float m_vel_rand;
        // start rotation z uses start direction
        bool m_start_rot_z_uses_direction;
        // constant rotation
        float m_const_rot_x;
        float m_const_rot_y;
        float m_const_rot_z;
        // constant rotation random modifier
        float m_const_rot_x_rand;
        float m_const_rot_y_rand;
        float m_const_rot_z_rand;
        // direction range
        float m_angle_start;
        float m_angle_range;
        // size scaling
        float m_size_scale;
        float m_size_scale_rand;
        // gravity
        float m_gravity_x;
        float m_gravity_x_rand;
        float m_gravity_y;
        float m_gravity_y_rand;
        // color random
        Color m_color_rand;
        // fading types (todo : dest-color)
        bool m_fade_size;
        bool m_fade_alpha;
        bool m_fade_color;
        // blending mode
        BlendingMode m_blending;

        // keep particles in this rect
        GL_rect m_clip_rect;
        // clip mode
        ParticleClipMode m_clip_mode;

        // Save to XML node
        virtual xmlpp::Element* Save_To_XML_Node(xmlpp::Element* p_element);

    protected:
        virtual std::string Get_XML_Type_Name();

    private:
        // time alive
        float m_emitter_living_time;
        // emit counter
        float m_emit_counter;
    };

    /* *** *** *** *** *** *** *** Animation Manager *** *** *** *** *** *** *** *** *** *** */

    class cAnimation_Manager : public cObject_Manager<cAnimation> {
    public:
        cAnimation_Manager(void);
        virtual ~cAnimation_Manager(void);

        // Add an animation object with the given settings
        virtual void Add(cAnimation* animation);

        // Update the objects
        void Update(void);
        // Draw the objects
        void Draw(void);

        typedef vector<cAnimation*> cAnimation_List;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

// The Animation Manager
    extern cAnimation_Manager* pActive_Animation_Manager;

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

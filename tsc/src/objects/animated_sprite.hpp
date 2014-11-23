/***************************************************************************
 * animated_sprite.h
 *
 * Copyright © 2005 - 2011 Florian Richter
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

#ifndef TSC_ANIMATED_SPRITE_HPP
#define TSC_ANIMATED_SPRITE_HPP

#include "../objects/movingsprite.hpp"
#include "../scripting/objects/sprites/mrb_animated_sprite.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** cAnimation_Surface *** *** *** *** *** *** *** *** *** *** */

    class cAnimation_Surface {
    public:
        cAnimation_Surface(void);
        ~cAnimation_Surface(void);

        // the image
        cGL_Surface* m_image;
        // time to display in milliseconds
        Uint32 m_time;
    };

    /* *** *** *** *** *** *** *** cAnimated_Sprite *** *** *** *** *** *** *** *** *** *** */

    class cAnimated_Sprite : public cMovingSprite {
    public:
        cAnimated_Sprite(cSprite_Manager* sprite_manager, std::string type_name = "sprite");
        virtual ~cAnimated_Sprite(void);

        /* Add an image to the animation
         * NULL image is allowed
         * time: if not set uses the default display time
        */
        void Add_Image(cGL_Surface* image, Uint32 time = 0);
        // Set the animation start and end image
        inline void Set_Animation_Image_Range(const int start, const int end)
        {
            m_anim_img_start = start;
            m_anim_img_end = end;
        };
        /* Set the image using the given array number
         * if new_start_image is set the default start_image will be set to the given image
         * if del_img is set the given image will be deleted
        */
        virtual void Set_Image_Num(const int num, const bool new_startimage = 0, const bool del_img = 0);
        // Get an array image
        cGL_Surface* Get_Image(const unsigned int num) const;
        // Clear the image list
        void Clear_Images(void);

        /* Set if the animation is enabled
         * default : disabled
        */
        inline void Set_Animation(const bool enabled = 0)
        {
            m_anim_enabled = enabled;
        };
        // Reset animation back to the first image
        inline void Reset_Animation(void)
        {
            m_anim_counter = 0;
        };

        // update animation
        void Update_Animation(void);

        // Set default image display time
        inline void Set_Default_Time(const Uint32 time = 1000)
        {
            m_anim_time_default = time;
        };
        /* Set display time for all images
         * default_time: if set also make it the default time
        */
        void Set_Time_All(const Uint32 time, const bool default_time = 0);
        /* Set the animation speed modifier
         * 1.0 is the normal speed
        */
        inline void Set_Animation_Speed(const float anim_mod)
        {
            m_anim_mod = anim_mod;

            if (m_anim_mod < 0.0f) {
                m_anim_mod = 0.0f;
            }
        };

        // Create the MRuby instance for this object.
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "AnimatedSprite"), &Scripting::rtTSC_Scriptable, this));
        }

        // currently set image array number
        int m_curr_img;
        // if animation is enabled
        bool m_anim_enabled;
        // animation start image
        int m_anim_img_start;
        // animation end image
        int m_anim_img_end;
        // default animation time
        Uint32 m_anim_time_default;
        // animation counter
        Uint32 m_anim_counter;
        // animation speed modifier
        float m_anim_mod;

        // Surface list
        typedef vector<cAnimation_Surface> cAnimation_Surface_List;
        cAnimation_Surface_List m_images;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif


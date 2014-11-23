/***************************************************************************
 * camera.h
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

#ifndef TSC_CAMERA_HPP
#define TSC_CAMERA_HPP

#include "../core/global_game.hpp"
#include "../core/math/rect.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** How to move the camera *** *** *** *** *** *** *** *** */
    enum Camera_movement {
        CAMERA_MOVE_NONE = 0,   // No movement
        CAMERA_MOVE_FLY = 1,    // Move the camera to the destination position
        CAMERA_MOVE_BLINK = 2,  // Fade out at the current position and fade back in at the destination position
        CAMERA_MOVE_ALONG_PATH = 3, // Move the camera along a path
        CAMERA_MOVE_ALONG_PATH_BACKWARDS = 4 // Move the camera along a path backwards
    };

    /* *** *** *** *** *** cCamera *** *** *** *** *** *** *** *** *** *** *** *** */

    class cCamera {
    public:
        cCamera(cSprite_Manager* sprite_manager);
        ~cCamera(void);

        // Set the parent sprite manager
        void Set_Sprite_Manager(cSprite_Manager* sprite_manager);

        // set camera position
        void Set_Pos(float x, float y);
        void Set_Pos_X(float x);
        void Set_Pos_Y(float y);
        // set start position
        void Reset_Pos(void);
        // move the camera
        void Move(const float move_x, const float move_y);
        /* moves to the given position with the given frames gradually
         * returns 0 if reached the nearest possible position
        */
        bool Move_to_Position_Gradually(const float pos_x, const float pos_y, const unsigned int frames = 200);
        /* move one step to the given position gradually
         * returns 0 if reached the nearest possible position
        */
        bool Step_to_Position_Gradually(const float pos_x, const float pos_y);

        // update
        void Update(void);

        // center on the player with the given direction ( DIR_HORIZONTAL, DIR_VERTICAL and DIR_ALL )
        void Center(const ObjectDirection direction = DIR_ALL);
        // get centered player position x
        float Get_Center_Pos_X(void) const;
        // get centered player position y
        float Get_Center_Pos_Y(void) const;
        // get camera rect
        GL_rect Get_Rect(void) const;

        // reset limits
        inline void Reset_Limits(void)
        {
            m_limit_rect = m_default_limits;
        };
        // set limits
        void Set_Limits(const GL_rect& rect);
        inline void Set_Limit_X(const float val)
        {
            m_limit_rect.m_x = val;
        };
        inline void Set_Limit_Y(const float val)
        {
            m_limit_rect.m_y = val;
        };
        inline void Set_Limit_W(const float val)
        {
            m_limit_rect.m_w = val;
        };
        inline void Set_Limit_H(const float val)
        {
            m_limit_rect.m_h = val;
        };
        // update limit with the given position
        inline void Update_Limit(float& x, float& y) const
        {
            Update_Limit_X(x);
            Update_Limit_Y(y);
        };
        void Update_Limit_X(float& x) const;
        void Update_Limit_Y(float& y) const;
        // update if position changed
        void Update_Position(void) const;

        // the parent sprite manager
        cSprite_Manager* m_sprite_manager;
        // position
        float m_x, m_y;
        // additional position offset
        float m_x_offset, m_y_offset;
        // position offset speed
        float m_hor_offset_speed, m_ver_offset_speed;
        // limits
        GL_rect m_limit_rect;

        // fixed horizontal scrolling velocity
        float m_fixed_hor_vel;

        // default limits
        static const GL_rect m_default_limits;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

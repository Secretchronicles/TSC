/***************************************************************************
 * collison.cpp  -  internal collision functions
 *
 * Copyright © 2005 - 2011 Florian Richter
 * Copyright (C) 2005        Amir Taaki
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

#include "../core/collision.hpp"
#include "../core/game_core.hpp"
#include "../level/level.hpp"
#include "../level/level_player.hpp"
#include "../video/gl_surface.hpp"
#include "../core/sprite_manager.hpp"

namespace TSC {

/* *** *** *** *** *** *** *** cObjectCollisionType *** *** *** *** *** *** *** *** *** *** */

cObjectCollisionType::cObjectCollisionType(void)
    : cObject_Manager<cObjectCollision>()
{

}

cObjectCollisionType::~cObjectCollisionType(void)
{
    Delete_All();
}

void cObjectCollisionType::Add(cObjectCollision* obj)
{
    if (!obj) {
        return;
    }

    cObject_Manager<cObjectCollision>::Add(obj);
}

// check if sprite
struct check_if_sprite : public std::binary_function<cObjectCollision*, const cSprite*, bool> {
    bool operator()(const cObjectCollision* col, const cSprite* sprite) const
    {
        return col->m_obj == sprite;
    }
};
bool cObjectCollisionType::Is_Included(const cSprite* obj)
{
    return std::find_if(objects.begin(), objects.end(), std::bind2nd(check_if_sprite(), obj)) != objects.end();
}

// check if array type
struct check_if_sprite_array : public std::binary_function<cObjectCollision*, ArrayType, bool> {
    bool operator()(const cObjectCollision* col, ArrayType type) const
    {
        return col->m_obj->m_sprite_array == type;
    }
};
bool cObjectCollisionType::Is_Included(const ArrayType type)
{
    return std::find_if(objects.begin(), objects.end(), std::bind2nd(check_if_sprite_array(), type)) != objects.end();
}

// check if sprite type
struct check_if_sprite_type : public std::binary_function<cObjectCollision*, SpriteType, bool> {
    bool operator()(const cObjectCollision* col, SpriteType type) const
    {
        return col->m_obj->m_type == type;
    }
};
bool cObjectCollisionType::Is_Included(const SpriteType type)
{
    return std::find_if(objects.begin(), objects.end(), std::bind2nd(check_if_sprite_type(), type)) != objects.end();
}

// check if validation type
struct check_if_valid_type : public std::binary_function<cObjectCollision*, Col_Valid_Type, bool> {
    bool operator()(const cObjectCollision* col, Col_Valid_Type type) const
    {
        return col->m_valid_type == type;
    }
};
bool cObjectCollisionType::Is_Included(const Col_Valid_Type type)
{
    return std::find_if(objects.begin(), objects.end(), std::bind2nd(check_if_valid_type(), type)) != objects.end();
}

cObjectCollision* cObjectCollisionType::Find_First(const ArrayType type)
{
    return *std::find_if(objects.begin(), objects.end(), std::bind2nd(check_if_sprite_array(), type));
}

cObjectCollision* cObjectCollisionType::Find_First(const SpriteType type)
{
    return *std::find_if(objects.begin(), objects.end(), std::bind2nd(check_if_sprite_type(), type));
}

/* *** *** *** *** *** *** *** cObjectCollision *** *** *** *** *** *** *** *** *** *** */

cObjectCollision::cObjectCollision(void)
{
    m_valid_type = COL_VTYPE_NOT_VALID;
    m_received = 0;
    m_obj = NULL;
    m_number = 0;
    m_direction = DIR_UNDEFINED;
    m_array = ARRAY_UNDEFINED;
}

cObjectCollision::~cObjectCollision(void)
{
    //
}

void cObjectCollision::Set_Direction(const cSprite* base, const cSprite* col)
{
    m_direction = Get_Collision_Direction(base, col);
}

/* *** *** *** *** *** *** *** functions *** *** *** *** *** *** *** *** *** *** */

ObjectDirection Get_Collision_Direction(const cSprite* base, const cSprite* col)
{
    // if valid moving sprite try the velocity based detection
    if (base->m_sprite_array == ARRAY_ENEMY || base->m_sprite_array == ARRAY_ACTIVE || base->m_sprite_array == ARRAY_PLAYER) {
        const cMovingSprite* moving_base = static_cast<const cMovingSprite*>(base);

        // top
        if (moving_base->m_vely < 0.0f) {
            if (Is_Collision_Top(base->m_col_rect, col->m_col_rect)) {
                return DIR_TOP;
            }

            if (moving_base->m_velx < 0.0f) {
                return DIR_LEFT;
            }
            if (moving_base->m_velx > 0.0f) {
                return DIR_RIGHT;
            }
        }
        // bottom
        else if (moving_base->m_vely > 0.0f) {
            if (Is_Collision_Bottom(base->m_col_rect, col->m_col_rect)) {
                return DIR_BOTTOM;
            }

            if (moving_base->m_velx < 0.0f) {
                return DIR_LEFT;
            }
            if (moving_base->m_velx > 0.0f) {
                return DIR_RIGHT;
            }
        }
        // left
        if (moving_base->m_velx < 0.0f) {
            if (Is_Collision_Left(base->m_col_rect, col->m_col_rect)) {
                return DIR_LEFT;
            }

            if (moving_base->m_vely < 0.0f) {
                return DIR_TOP;
            }
            if (moving_base->m_vely > 0.0f) {
                return DIR_BOTTOM;
            }
        }
        // right
        else if (moving_base->m_velx > 0.0f) {
            if (Is_Collision_Right(base->m_col_rect, col->m_col_rect)) {
                return DIR_RIGHT;
            }

            if (moving_base->m_vely < 0.0f) {
                return DIR_TOP;
            }
            if (moving_base->m_vely > 0.0f) {
                return DIR_BOTTOM;
            }
        }
    }

    // ## detection without velocity

    // top
    if (Is_Collision_Top(base->m_col_rect, col->m_col_rect)) {
        return DIR_TOP;
    }
    // bottom
    if (Is_Collision_Bottom(base->m_col_rect, col->m_col_rect)) {
        return DIR_BOTTOM;
    }
    // left
    if (Is_Collision_Left(base->m_col_rect, col->m_col_rect)) {
        return DIR_LEFT;
    }
    // right
    if (Is_Collision_Right(base->m_col_rect, col->m_col_rect)) {
        return DIR_RIGHT;
    }

    // ## advanced detection ( if base object is partly inside col object )

    // get the middle point
    GL_point point_base = base->m_col_rect.Get_pos_middle();
    GL_point point_col = col->m_col_rect.Get_pos_middle();

    // difference between points
    float diff_x = point_col.m_x - point_base.m_x;
    float diff_y = point_col.m_y - point_base.m_y;

    // more horizontal
    if ((diff_x > 0.0f && (diff_x > diff_y)) || (diff_x < 0.0f && (diff_x < diff_y))) {
        // if col object point left
        if (diff_x > 0.0f) {
            return DIR_RIGHT;
        }
        // if col object point is right
        else {
            return DIR_LEFT;
        }
    }
    // more vertical
    else {
        // if col object point is on top
        if (diff_y > 0.0f) {
            return DIR_DOWN;
        }
        // if col object point is below
        else {
            return DIR_UP;
        }
    }
}

// from SDL_collide ( Copyright (C) 2005 Amir Taaki ) - MIT License
bool Col_Circle(float x1, float y1, float r1, float x2, float y2, float r2, int offset)
{
    float xdiff = x2 - x1;  // x plane difference
    float ydiff = y2 - y1;  // y plane difference

    /* distance between the circles centres squared */
    float dcentre_sq = (ydiff * ydiff) + (xdiff * xdiff);

    /* calculate sum of radiuses squared */
    float r_sum_sq = r1 + r2;   // square on seperate line, so
    r_sum_sq *= r_sum_sq;   // dont recompute r1 + r2

    return (dcentre_sq - r_sum_sq <= (offset * offset));
}

// from SDL_collide ( Copyright (C) 2005 Amir Taaki ) - MIT License
bool Col_Circle(const GL_Circle& c, const GL_rect& r, int offset /* = 1 */)
{
    /* if radius is not specified
    we approximate them using SDL_Surface's
    width and height average and divide by 2*/
    float r2 = ((r.m_w + r.m_h) / 4);     // same as / 2) / 2;

    float x2 = r.m_x + r.m_w / 2;       // offset x and y
    float y2 = r.m_y + r.m_h / 2;       // co-ordinates into
    // centre of image

    return Col_Circle(c.Get_X(), c.Get_Y(), c.Get_Radius(), x2, y2, r2, offset);

}

// from SDL_collide ( Copyright (C) 2005 Amir Taaki ) - MIT License
bool Col_Circle(cGL_Surface* a, float x1, float y1, cGL_Surface* b, float x2, float y2, int offset)
{
    /* if radius is not specified
    we approximate them using SDL_Surface's
    width and height average and divide by 2*/
    float r1 = ((a->m_w + a->m_h) / 4);       // same as / 2) / 2;
    float r2 = ((b->m_w + b->m_h) / 4);

    x1 += a->m_w / 2;       // offset x and y
    y1 += a->m_h / 2;       // co-ordinates into
    // centre of image
    x2 += b->m_w / 2;
    y2 += b->m_h / 2;

    return Col_Circle(x1, y1, r1, x2, y2, r2, offset);
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

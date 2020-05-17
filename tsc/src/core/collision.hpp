/***************************************************************************
 * collision.h
 *
 * Copyright © 2005 - 2011 Florian Richter
 * Copyright © 2012-2020 The TSC Contributors
 * Copyright (C) 2005 Amir Taaki ( Circle Collision tests ) - MIT License
 * Copyright (C) 2005 Magnus Norddahl ( Line Collision tests ) - BSD License
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TSC_COLLISION_HPP
#define TSC_COLLISION_HPP

#include "../core/global_basic.hpp"
#include "../core/global_game.hpp"
#include "../core/obj_manager.hpp"
#include "../core/math/rect.hpp"
#include "math/circle.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** cObjectCollision *** *** *** *** *** *** *** *** *** *** */

// Object collision data
    class cObjectCollision {
    public:
        cObjectCollision(void);
        ~cObjectCollision(void);

        /* Set the collision direction
         * base - the base sprite
         * col - the colliding sprite
        */
        void Set_Direction(const cSprite* base, const cSprite* col);

        // valid type
        Col_Valid_Type m_valid_type;

        /* if true this is a receiving collision
         * and not a normal self detected collision
        */
        bool m_received;

        // the object colliding with (only use it in the same frame for now !)
        cSprite* m_obj;
        // colliding object number
        int m_number;

        // direction
        ObjectDirection m_direction;
        // Colliding object array type
        ArrayType m_array;
    };

    typedef vector<cObjectCollision*> cObjectCollision_List;

    /* *** *** *** *** *** *** *** cObjectCollisionType *** *** *** *** *** *** *** *** *** *** */

// collision type class
    class cObjectCollisionType : public cObject_Manager<cObjectCollision> {
    public:
        cObjectCollisionType(void);
        virtual ~cObjectCollisionType(void);

        // Add an object collision
        virtual void Add(cObjectCollision* obj);

        // returns true if the given object was found in the list
        bool Is_Included(const cSprite* obj);
        // returns true if the given array type was found in the list
        bool Is_Included(const ArrayType type);
        // returns true if the sprite type was found in the list
        bool Is_Included(const SpriteType type);
        // returns true if the validation type was found in the list
        bool Is_Included(const Col_Valid_Type type);

        // returns the first found sprite if the given array type was found
        cObjectCollision* Find_First(const ArrayType type);
        // returns the first found sprite if the given sprite type was found
        cObjectCollision* Find_First(const SpriteType type);
    };

    /* *** *** *** *** *** *** *** functions *** *** *** *** *** *** *** *** *** *** */

    /* Returns the collision direction
     * base - the base sprite
     * col - the colliding sprite
    */
    ObjectDirection Get_Collision_Direction(const cSprite* base, const cSprite* col);

// returns true if a collision is on the top
    inline bool Is_Collision_Top(const GL_rect& base_rect, const GL_rect& col_rect)
    {
        if (base_rect.m_y + 1 >= col_rect.m_y + col_rect.m_h && base_rect.m_x < col_rect.m_x + col_rect.m_w && base_rect.m_x + base_rect.m_w > col_rect.m_x) {
            return 1;
        }

        return 0;
    };
// returns true if a collision is on the bottom
    inline bool Is_Collision_Bottom(const GL_rect& base_rect, const GL_rect& col_rect)
    {
        if (base_rect.m_y + base_rect.m_h - 1 <= col_rect.m_y && base_rect.m_x < col_rect.m_x + col_rect.m_w && base_rect.m_x + base_rect.m_w > col_rect.m_x) {
            return 1;
        }

        return 0;
    };
// returns true if a collision is on the left
    inline bool Is_Collision_Left(const GL_rect& base_rect, const GL_rect& col_rect)
    {
        if (base_rect.m_x + 1 >= col_rect.m_x + col_rect.m_w && base_rect.m_y < col_rect.m_y + col_rect.m_h && base_rect.m_y + base_rect.m_h > col_rect.m_y) {
            return 1;
        }

        return 0;
    };
// returns true if a collision is on the right
    inline bool Is_Collision_Right(const GL_rect& base_rect, const GL_rect& col_rect)
    {
        if (base_rect.m_x + base_rect.m_w - 1 <= col_rect.m_x && base_rect.m_y < col_rect.m_y + col_rect.m_h && base_rect.m_y + base_rect.m_h > col_rect.m_y) {
            return 1;
        }

        return 0;
    };

    /*
     * Bounding box collision test
     * Checks if the first rect intersects with the second
    */
    inline bool Col_Box(const sf::IntRect& a, const GL_rect& b)
    {
        // check if their bounding boxes touch
        if (b.m_x + b.m_w < a.left) {
            return 0;
        }
        if (b.m_x > a.left + a.width) {
            return 0;
        }

        if (b.m_y + b.m_h < a.top) {
            return 0;
        }
        if (b.m_y > a.top + a.height) {
            return 0;
        }

        // bounding boxes intersect
        return 1;
    };

    /*
     * Bounding box collision test (sf::IntRect)
     * Checks if the first rect intersects completely with the second
    */
    inline bool Col_Box_full(const sf::IntRect& a, const sf::IntRect& b)
    {
        if (a.left <= b.left) {
            return 0;
        }
        if (a.left + a.width >= b.left + b.width) {
            return 0;
        }
        if (a.top <= b.top) {
            return 0;
        }
        if (a.top + a.height >= b.top + b.height) {
            return 0;
        }

        return 1;
    };

    /*
     * tests whether 2 circles intersect
     *
     * Parameters:
     * circle1 - center (x1,y1) with radius r1
     * circle2 - center (x2,y2) with radius r2
     * (allow distance between circles of offset)
    */
    bool Col_Circle(float x1, float y1, float r1, float x2, float y2, float r2, int offset = 1);
    /*
     * a circle intersection detection algorithm that will use
     * the position of the center of the surface as the center of
     * the circle and approximate the radius using the width and height
     * of the surface (for example a rect of 4x6 would have r = 2.5).
    */

    bool Col_Circle(cGL_Surface* a, float x1, float y1, cGL_Surface* b, float x2, float y2, int offset);
    bool Col_Circle(const GL_Circle& c, const GL_rect& r, int offset = 1);

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

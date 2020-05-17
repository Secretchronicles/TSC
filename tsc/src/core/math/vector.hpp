/***************************************************************************
 * vector.h
 *
 * Copyright © 2006 - 2011 Florian Richter
 * Copyright (C) 2007 Clanlib Team ( original Vector class )
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

#ifndef TSC_VECTOR_HPP
#define TSC_VECTOR_HPP

#include "../../core/global_game.hpp"
#include "../../core/math/utilities.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** GL_Vector *** *** *** *** *** *** *** *** *** *** */

    /* Vector
     * parts from Clanlib (Magnus Norddahl)
     * BSD style license
    */
    class GL_Vector {
    public:
        GL_Vector(float pos_x = 0, float pos_y = 0, float pos_z = 0, float pos_w = 1);
        // copy from another vector
        GL_Vector(const GL_Vector& v);

        // Returns the (euclid) norm of the vector (in R^3)
        // This function does not use the w coordinate of the vector
        // uses only x,y,z coordinates
        float norm(void) const;

        // Normalizes the vector ( not using the w ordinate )
        void normalize(void);

        // Dot products this vector with an other vector
        float dot(const GL_Vector& vector) const;

        // Calculate the angle between this vector and an other vector
        float angle(const GL_Vector& vector) const;

        // Calculate the cross product between this vector and an other vector
        GL_Vector cross(const GL_Vector& vector) const;

        // Rotate vector around an axis
        // angle: Angle to rotate
        // axis: Rotation axis
        GL_Vector rotate(float angle, const GL_Vector& axis) const;

        // Rounds all components
        void round(void);

        //: Scalar product (vector * scalar)
        inline GL_Vector operator * (float s) const
        {
            return GL_Vector(s * x, s * y, s * z, s * w);
        }

        // Scalar product (scalar * vector)
        inline friend GL_Vector operator * (float s, const GL_Vector& v)
        {
            return GL_Vector(s * v.x, s * v.y, s * v.z, s * v.w);
        }

        // += operator
        inline void operator += (const GL_Vector& v)
        {
            x += v.x;
            y += v.y;
            z += v.z;
            w += v.w;
        }

        // -= operator
        inline void operator -= (const GL_Vector& v)
        {
            x -= v.x;
            y -= v.y;
            z -= v.z;
            w -= v.w;
        }

        // *= operator (scalar multiplication)
        inline void operator *= (float s)
        {
            x *= s;
            y *= s;
            z *= s;
            w *= s;
        }

        // + operator
        inline GL_Vector operator + (const GL_Vector& v) const
        {
            return GL_Vector(x + v.x, y + v.y, z + v.z, w + v.w);
        }

        // - operator
        inline GL_Vector operator - (const GL_Vector& v) const
        {
            return GL_Vector(x - v.x, y - v.y, z - v.z, w - v.w);
        }

        // unary - operator
        inline GL_Vector operator - () const
        {
            return GL_Vector(-x, -y, -z, -w);
        }

        // assignment operator
        inline GL_Vector& operator = (const GL_Vector& v)
        {
            x = v.x;
            y = v.y;
            z = v.z;
            w = v.w;

            return *this;
        }

        // Returns true if current vector equals v
        // v : other vector
        inline bool operator == (const GL_Vector& v) const
        {
            return (Is_Float_Equal(x, v.x) && Is_Float_Equal(y, v.y) && Is_Float_Equal(z, v.z) && Is_Float_Equal(w, v.w));
        }

        // Returns false if current vector equals v
        // v : other vector
        inline bool operator != (const GL_Vector& v) const
        {
            return !(operator == (v));
        }

        // coordinates
        float x;
        float y;
        float z;
        float w;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

/***************************************************************************
 * point.h
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

#ifndef TSC_POINT_HPP
#define TSC_POINT_HPP

#include "../../core/math/utilities.hpp"

namespace TSC {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

    /* *** *** *** *** *** *** *** GL_point *** *** *** *** *** *** *** *** *** *** */

    /* 2D point
     * parts from Clanlib
    */
    class GL_point {
    public:
        GL_point(void)
            : m_x(0), m_y(0) {}

        GL_point(float x, float y)
            : m_x(x), m_y(y) {}

        GL_point(const GL_point& p)
            : m_x(p.m_x), m_y(p.m_y) {}

        // Return a rotated version of this point.
        // hotspot: The point around which to rotate.
        // angle: The amount of degrees to rotate by, clockwise.
        GL_point rotate(const GL_point& hotspot, float angle) const
        {
            //Move the hotspot to 0,0
            GL_point r(m_x - hotspot.m_x, m_y - hotspot.m_y);

            const float c = static_cast<float>(sqrt(r.m_x*r.m_x + r.m_y*r.m_y));
            const float nw = static_cast<float>(atan2(r.m_y, r.m_x) + ((angle + 180) * M_PI / 180));
            r.m_x = static_cast<float>((sin(1.5 * M_PI - nw) * c) /*+ 0.5*/) + hotspot.m_x;
            r.m_y = -static_cast<float>((sin(nw) * c) /*+ 0.5*/) + hotspot.m_y;

            return r;
        }

        // Return a point rotated around the given axis in 3D.
        // angle : Rotation amount
        // a_: Axis to rotate around.
        GL_point rotate3d(float angle, const float& a_x, const float& a_y, const float& a_z)const
        {
            GL_point tmp;

            float s = static_cast<float>(sin(angle));
            float c = static_cast<float>(cos(angle));

            tmp.m_x = m_x*(a_x*a_x*(1-c)+c)     + m_y*(a_x*a_y*(1-c)-a_z*s) + /*z**/(a_x*a_z*(1-c)+a_y*s);
            tmp.m_y = m_x*(a_y*a_x*(1-c)+a_z*s) + m_y*(a_y*a_y*(1-c)+c)     + /*z**/(a_y*a_z*(1-c)-a_x*s);
            //tmp.z = m_x*(a_x*a_z*(1-c)-a_y*s) + m_y*(a_y*a_z*(1-c)+a_x*s) + z*(a_z*a_z*(1-c)+c);

            return tmp;
        }

        // Returns the distance to another point
        inline float distance(const GL_point& p) const
        {
            return sqrt((m_x - p.m_x) * (m_x- p.m_x) + (m_y - p.m_y) * (m_y - p.m_y));
        }

        // Translate point
        inline GL_point& operator += (const GL_point& p)
        {
            m_x += p.m_x;
            m_y += p.m_y;

            return *this;
        }

        // Translate point negatively
        inline GL_point& operator -= (const GL_point& p)
        {
            m_x -= p.m_x;
            m_y -= p.m_y;

            return *this;
        }

        // + operator
        inline GL_point operator + (const GL_point& p) const
        {
            return GL_point(m_x + p.m_x, m_y + p.m_y);
        }

        // - operator
        inline GL_point operator - (const GL_point& p) const
        {
            return GL_point(m_x - p.m_x, m_y - p.m_y);
        }

        // == operator
        inline bool operator == (const GL_point& p) const
        {
            return Is_Float_Equal(m_x, p.m_x) && Is_Float_Equal(m_y, p.m_y);
        }

        // != operator
        inline bool operator != (const GL_point& p) const
        {
            return !Is_Float_Equal(m_x, p.m_x) || !Is_Float_Equal(m_y, p.m_y);
        }

        float m_x, m_y;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

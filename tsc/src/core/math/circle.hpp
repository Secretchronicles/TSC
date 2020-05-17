/***************************************************************************
 * circle.hpp
 *
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

#ifndef TSC_CIRCLE_HPP
#define TSC_CIRCLE_HPP
#include "../../core/global_game.hpp"
#include "point.hpp"
#include "utilities.hpp"

namespace TSC {

    class GL_Circle {
    public:
        GL_Circle()
            : m_x(0), m_y(0), m_radius(0) {}
        GL_Circle(float x, float y, float r)
            : m_x(x), m_y(y), m_radius(r) {}
        GL_Circle(const GL_Circle& c)
            : m_x(c.Get_X()), m_y(c.Get_Y()), m_radius(c.Get_Radius()) {}

        inline float Get_X() const
        {
            return m_x;
        }
        inline float Get_Y() const
        {
            return m_y;
        }
        inline float Get_Radius() const
        {
            return m_radius;
        }

        inline void Set_X(float x)
        {
            m_x = x;
        }
        inline void Set_Y(float y)
        {
            m_y = y;
        }
        inline void Set_Radius(float r)
        {
            m_radius = r;
        }

        inline GL_point Get_Top_Left_Point()
        {
            return GL_point(m_x - m_radius, m_y - m_radius);
        }
        inline GL_rect Get_Outer_Rect()
        {
            return GL_rect(m_x - m_radius, m_y - m_radius, 2 * m_radius, 2 * m_radius);
        }

        inline void Clear()
        {
            m_x = m_y = m_radius = 0;
        }

        bool Intersects(const GL_Circle& c) const;
        bool Intersects(const GL_rect& r) const;
        // bool Intersects(const GL_point& p); // TODO

    private:
        float m_x;
        float m_y;
        float m_radius;
    };

}

#endif

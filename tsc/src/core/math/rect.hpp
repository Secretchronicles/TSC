/***************************************************************************
 * rect.h
 *
 * Copyright © 2003 - 2011 Florian Richter
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

#ifndef TSC_RECT_HPP
#define TSC_RECT_HPP

#include "../../core/global_game.hpp"
#include "../../core/math/point.hpp"
#include "../../core/math/utilities.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** GL_rect *** *** *** *** *** *** *** *** *** *** */

    class GL_rect {
    public:
        GL_rect(void)
            : m_x(0), m_y(0), m_w(0), m_h(0) {}

        GL_rect(const GL_rect* rect)
            : m_x(rect->m_x), m_y(rect->m_y), m_w(rect->m_w), m_h(rect->m_h) {}

        GL_rect(float x, float y, float w, float h)
            : m_x(x), m_y(y), m_w(w), m_h(h) {}

        // returns an sf::Rect
        inline sf::IntRect Get_Rect(void) const
        {
            sf::IntRect rect;
            rect.left   = static_cast<int16_t>(m_x);
            rect.top    = static_cast<int16_t>(m_y);
            rect.width  = static_cast<uint16_t>(m_w);
            rect.height = static_cast<uint16_t>(m_h);

            return rect;
        }

        // returns this as sf::IntRect
        sf::IntRect Get_Rect_pos(float posx, float posy) const
        {
            sf::IntRect rect;
            rect.left   = static_cast<int16_t>(m_x + posx);
            rect.top    = static_cast<int16_t>(m_y + posy);
            rect.width  = static_cast<uint16_t>(m_w);
            rect.height = static_cast<uint16_t>(m_h);

            return rect;
        }

        // returns the point in the middle of the rect
        inline GL_point Get_pos_middle(void) const
        {
            return GL_point(m_x + (m_w / 2), m_y + (m_h / 2));
        }

        // clears the data
        inline void clear(void)
        {
            m_x = 0;
            m_y = 0;
            m_w = 0;
            m_h = 0;
        }

        // returns true if we intersect with the point
        bool Intersects(const float x, const float y) const
        {
            if (x > m_x + m_w) {
                return 0;
            }
            if (x < m_x) {
                return 0;
            }
            if (y > m_y + m_h) {
                return 0;
            }
            if (y < m_y) {
                return 0;
            }

            // they intersect
            return 1;
        }

        // returns true if we intersect with the rect
        bool Intersects(const GL_rect& b) const
        {
            if (b.m_x + b.m_w < m_x) {
                return 0;
            }
            if (b.m_x > m_x + m_w) {
                return 0;
            }

            if (b.m_y + b.m_h < m_y) {
                return 0;
            }
            if (b.m_y > m_y + m_h) {
                return 0;
            }

            // they intersect
            return 1;
        }

        // += operator
        inline void operator += (const GL_rect& r)
        {
            m_x += r.m_x;
            m_y += r.m_y;
            m_w += r.m_w;
            m_h += r.m_h;
        }

        // -= operator
        inline void operator -= (const GL_rect& r)
        {
            m_x -= r.m_x;
            m_y -= r.m_y;
            m_w -= r.m_w;
            m_h -= r.m_h;
        }

        // + operator
        inline GL_rect operator + (const GL_rect& r) const
        {
            return GL_rect(m_x + r.m_x, m_y + r.m_y, m_w + r.m_w, m_h + r.m_h);
        }

        // - operator
        inline GL_rect operator - (const GL_rect& r) const
        {
            return GL_rect(m_x - r.m_x, m_y - r.m_y, m_w - r.m_w, m_h - r.m_h);
        }

        // unary - operator
        inline GL_rect operator - () const
        {
            return GL_rect(-m_x, -m_y, -m_w, -m_h);
        }

        // assignment operator
        inline GL_rect& operator = (const GL_rect& r)
        {
            m_x = r.m_x;
            m_y = r.m_y;
            m_w = r.m_w;
            m_h = r.m_h;

            return *this;
        }

        // compare
        inline bool operator == (const GL_rect& r) const
        {
            return (Is_Float_Equal(m_x, r.m_x) && Is_Float_Equal(m_y, r.m_y) && Is_Float_Equal(m_w, r.m_w) && Is_Float_Equal(m_h, r.m_h));
        }

        inline bool operator != (const GL_rect& r) const
        {
            return !(operator == (r));
        }

        float m_x, m_y;
        float m_w, m_h;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

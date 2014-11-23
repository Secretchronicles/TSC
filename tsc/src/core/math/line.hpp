/***************************************************************************
 * line.h
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

#ifndef TSC_LINE_HPP
#define TSC_LINE_HPP

#include "../../core/math/point.hpp"

namespace TSC {

    template<typename T> inline T pow2(T value)
    {
        return value * value;
    }
    template<typename T> inline T cl_min(T a, T b)
    {
        if (a < b) return a;
        return b;
    }

    /* *** *** *** *** *** *** *** GL_line *** *** *** *** *** *** *** *** *** *** */

// 2D Line
    class GL_line {
    public:
        GL_line(void)
            : m_x1(0), m_y1(0), m_x2(0), m_y2(0) {}

        GL_line(float x1, float y1, float x2, float y2)
            : m_x1(x1), m_y1(y1), m_x2(x2), m_y2(y2) {}

        // returns the intersection point between the lines
        // from ClanLib ( Copyright (C) 2005 Magnus Norddahl ) - BSD License
        GL_point Get_Intersection(GL_line* line2) const
        {
            float denominator = ((m_x2 - m_x1) * (line2->m_y2 - line2->m_y1) - (m_y2 - m_y1) * (line2->m_x2 - line2->m_x1));

            if (denominator == 0) {
                return GL_point(m_x1, m_y1);
            }

            float r = ((m_y1 - line2->m_y1) * (line2->m_x2 - line2->m_x1) - (m_x1 - line2->m_x1) * (line2->m_y2 - line2->m_y1)) / denominator;

            return GL_point(m_x1 + r * (m_x2 - m_x1), m_y1 + r * (m_y2 - m_y1));
        }

        // returns true if the lines intersect
        // from ClanLib ( Copyright (C) 2005 Magnus Norddahl ) - BSD License
        bool Intersects(GL_line* line2, bool collinear_intersect = 0) const
        {
            float denominator = ((m_x2 - m_x1) * (line2->m_y2 - line2->m_y1) - (m_y2 - m_y1) * (line2->m_x2 - line2->m_x1));

            if (denominator == 0) { // parallel
                if ((m_y1 - line2->m_y1) * (line2->m_x2 - line2->m_x1) - (m_x1 - line2->m_x1) * (line2->m_y2 - line2->m_y1) == 0) {        // collinear
                    if (collinear_intersect) {
                        return 1;
                    }
                    else {
                        return 0;
                    }
                }

                return 0;
            }

            float r = ((m_y1 - line2->m_y1) * (line2->m_x2 - line2->m_x1) - (m_x1 - line2->m_x1) * (line2->m_y2 - line2->m_y1)) / denominator;
            float s = ((m_y1 - line2->m_y1) * (m_x2 - m_x1) - (m_x1 - line2->m_x1) * (m_y2 - m_y1)) / denominator;

            if (line2->m_y1 < line2->m_y2) {
                if ((s >= 0.0f  && s < 1.0f) && (r >= 0.0f && r <= 1.0f)) {
                    return 1;
                }
            }
            else {
                if ((s > 0.0f  && s <= 1.0f) && (r >= 0.0f && r <= 1.0f)) {
                    return 1;
                }
            }

            return 0;
        }

        // returns the distance from a point to this line
        // from ClanLib ( Copyright (C) 2005 Magnus Norddahl ) - BSD License
        float Distance_to_Line(float x, float y)
        {
            float l = sqrt(pow2(m_x2 - m_x1) + pow2(m_y2 - m_y1));
            float r = ((x - m_x1) * (m_x2 - m_x1) + (y - m_y1) * (m_y2 - m_y1)) / pow2(l);

            if (r <= 0.0f || r >= 1.0f) {
                GL_point p(x, y);
                GL_point A(m_x1, m_y1);
                GL_point B(m_x2, m_y2);

                return cl_min(p.distance(A), p.distance(B));
            }

            float s = ((m_y1 - y) * (m_x2 - m_x1) - (m_x1 - x) * (m_y2 - m_y1)) / pow2(l);
            return fabs(s) * l;
        }

        float m_x1, m_y1;
        float m_x2, m_y2;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

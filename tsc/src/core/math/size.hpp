/***************************************************************************
 * size.h
 *
 * Copyright © 2008 - 2011 Florian Richter
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

#ifndef TSC_SIZE_HPP
#define TSC_SIZE_HPP

#include "../../core/global_game.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** cSize_Int *** *** *** *** *** *** *** *** *** *** */

// class for width and height in int
    class cSize_Int {
    public:
        cSize_Int(void)
        {
            m_width = 0;
            m_height = 0;
        }

        cSize_Int(int width, int height)
            : m_width(width), m_height(height) {}

        inline bool operator == (const cSize_Int& other) const
        {
            return (m_width == other.m_width) && (m_height == other.m_height);
        }

        inline bool operator != (const cSize_Int& other) const
        {
            return (m_width != other.m_width) || (m_height != other.m_height);
        }

        int m_width, m_height;
    };

    /* *** *** *** *** *** *** *** cSize_Float *** *** *** *** *** *** *** *** *** *** */

// class for width and height in float
    class cSize_Float {
    public:
        cSize_Float(void)
        {
            m_width = 0;
            m_height = 0;
        }

        cSize_Float(float width, float height)
            : m_width(width), m_height(height) {}

        inline bool operator == (const cSize_Float& other) const
        {
            return Is_Float_Equal(m_width, other.m_width) && Is_Float_Equal(m_height, other.m_height);
        }

        inline bool operator != (const cSize_Float& other) const
        {
            return !Is_Float_Equal(m_width, other.m_width) || !Is_Float_Equal(m_height, other.m_height);
        }

        float m_width, m_height;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

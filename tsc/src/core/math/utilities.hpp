/***************************************************************************
 * utilities.h
 *
 * Copyright © 2005 - 2011 Florian Richter
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

#ifndef TSC_UTILITIES_HPP
#define TSC_UTILITIES_HPP

#include "../../core/global_basic.hpp"
#include "../../core/global_game.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** *** Math utility functions *** *** *** *** *** *** *** *** *** */

    template<class T> inline T Clamp(const T& v, const T& min, const T& max)
    {
        if (v > max) {
            return max;
        }
        else if (v < min) {
            return min;
        }

        return v;
    }

// return a random floating point value between the given values
    inline float Get_Random_Float(float min, float max)
    {
        return min + (max - min) * static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    }

// Checks if number is power of 2 and if not returns the next power of two size
    inline unsigned int Get_Power_of_2(unsigned int size)
    {
        unsigned int value = 1;

        while (value < size) {
            value <<= 1;
        }

        return value;
    };

    /* Returns true if the number is valid
     * accept_floating_point: if is set also accept floating point values
    */
    bool Is_Valid_Number(std::string num, bool accept_floating_point = 1);

// Returns true if the floats are equal with the given tolerance
    inline bool Is_Float_Equal(float a, float b, float tolerance = 0.0001f)
    {
        return fabs(b - a) <= tolerance;
    }

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

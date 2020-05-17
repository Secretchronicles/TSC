/***************************************************************************
 * utilities.cpp  -  General math functions
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

#include "../../core/math/utilities.hpp"

/* non digit class
 * returns true if not a number was found
 * used by the valid_number function
*/
class nondigit {
public:
    bool operator()(char c) const
    {
        return !isdigit(c);
    }
};

namespace TSC {

/* *** *** *** *** *** *** *** *** Math utility functions *** *** *** *** *** *** *** *** *** */

bool Is_Valid_Number(std::string num, bool accept_floating_point /* = 1 */)
{
    // accept negative numbers
    if (num.find('-') == 0) {
        num.erase(0, 1);
    }

    // accept numbers with a point if given
    if (accept_floating_point) {
        std::string::size_type pos = num.find('.');

        if (pos != std::string::npos) {
            num.erase(pos, 1);
        }
    }

    if (std::find_if(num.begin(), num.end(), nondigit()) == num.end()) {
        return 1;
    }

    return 0;
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

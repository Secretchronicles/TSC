/***************************************************************************
 * circle.cpp
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

#include "../collision.hpp"
#include "rect.hpp"
#include "circle.hpp"

using namespace TSC;

bool GL_Circle::Intersects(const GL_Circle& c) const
{
    return Col_Circle(m_x, m_y, m_radius, c.Get_X(), c.Get_Y(), c.Get_Radius());
}

bool GL_Circle::Intersects(const GL_rect& r) const
{
    return Col_Circle(*this, r);
}

/*bool GL_Circle::Intersects(const GL_point& p)
{
    // TODO
}
*/

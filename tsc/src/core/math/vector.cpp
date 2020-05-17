/***************************************************************************
 * vector.cpp  -  vector class
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

#include "../../core/global_basic.hpp"
#include "../../core/math/vector.hpp"

namespace TSC {

/* *** *** *** *** *** *** *** GL_Vector *** *** *** *** *** *** *** *** *** *** */

GL_Vector::GL_Vector(float pos_x /* = 0 */, float pos_y /* = 0 */, float pos_z /* = 0 */, float pos_w /* = 1 */)
{
    x = pos_x;
    y = pos_y;
    z = pos_z;
    w = pos_w;
}

GL_Vector::GL_Vector(const GL_Vector& v)
{
    x = v.x;
    y = v.y;
    z = v.z;
    w = v.w;
}

float GL_Vector::norm(void) const
{
    return static_cast<float>(sqrt(x * x + y * y + z * z));
}

void GL_Vector::normalize(void)
{
    float val = norm();

    if (val != 0) {
        x /= val;
        y /= val;
        z /= val;
    }
}

float GL_Vector::dot(const GL_Vector& v) const
{
    return x * v.x + y * v.y + z * v.z;
}

float GL_Vector::angle(const GL_Vector& v) const
{
    return acos(dot(v) / (norm() * v.norm()));
}

GL_Vector GL_Vector::cross(const GL_Vector& v) const
{
    GL_Vector tmp = GL_Vector(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
    return tmp;
}

// quick hack, same as glRotatef(angle, a);
GL_Vector GL_Vector::rotate(float angle, const GL_Vector& a) const
{
    GL_Vector tmp = GL_Vector();

    float s = static_cast<float>(sin(angle));
    float c = static_cast<float>(cos(angle));

    tmp.x = x*(a.x*a.x*(1-c)+c)     + y*(a.x*a.y*(1-c)-a.z*s) + z*(a.x*a.z*(1-c)+a.y*s);
    tmp.y = x*(a.y*a.x*(1-c)+a.z*s) + y*(a.y*a.y*(1-c)+c)     + z*(a.y*a.z*(1-c)-a.x*s);
    tmp.z = x*(a.x*a.z*(1-c)-a.y*s) + y*(a.y*a.z*(1-c)+a.x*s) + z*(a.z*a.z*(1-c)+c);

    return tmp;
}

void GL_Vector::round(void)
{
    x = static_cast<float>(int(x + 0.5f));
    y = static_cast<float>(int(y + 0.5f));
    z = static_cast<float>(int(z + 0.5f));
    w = static_cast<float>(int(w + 0.5f));
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

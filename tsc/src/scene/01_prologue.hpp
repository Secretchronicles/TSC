/***************************************************************************
 * 01_prologue.hpp
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

#ifndef TSC_01_PROLOGUE_HPP
#define TSC_01_PROLOGUE_HPP
#include "scene.hpp"

namespace TSC {

    class cPrologueScene: public cScene {
    public:
        cPrologueScene();
        virtual ~cPrologueScene();
    };

}

#endif /* TSC_01_PROLOGUE_HPP */

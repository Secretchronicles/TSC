/***************************************************************************
 * shell.hpp - loose shells lying around.
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

#ifndef TSC_SHELL_HPP
#define TSC_SHELL_HPP
#include "army.hpp"

namespace TSC {

    class cShell: public cArmy {
    public:
        cShell(cSprite_Manager* p_sprite_manager);
        cShell(XmlAttributes& attributes, cSprite_Manager* p_sprite_manager);
        virtual ~cShell();
        virtual cShell* Copy() const;

        virtual void Update();
        virtual void Stand_Up();
        virtual void Set_Color(DefaultColor col);

        virtual std::string Create_Name() const;
    protected:
        void Init();
        virtual std::string Get_XML_Type_Name();
    };
}

#endif

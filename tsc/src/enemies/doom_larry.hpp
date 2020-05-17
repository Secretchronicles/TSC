

/***************************************************************************
 * doom_larry.hpp - The walking bomb with super explosive power.
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

#ifndef TSC_DOOM_LARRY_HPP
#define TSC_DOOM_LARRY_HPP

#include "larry.hpp"

namespace TSC {
    class cDoomLarry : public cLarry {
    public:
        cDoomLarry(cSprite_Manager* p_sprite_manager);
        cDoomLarry(XmlAttributes& attributes, cSprite_Manager* p_sprite_manager);
        ~cDoomLarry();
        virtual cDoomLarry* Copy() const;
        virtual void Update();

    protected:
        virtual std::string Get_XML_Type_Name();
        virtual void Kill_Objects_in_Explosion_Range();
    private:
        void Init();
    };

}
#endif


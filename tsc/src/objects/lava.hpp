/***************************************************************************
 * lava.hpp  -  class for lava blocks
 *
 * Copyright © 2014 The TSC Contributors
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TSC_LAVA_HPP
#define TSC_LAVA_HPP
#include "movingsprite.hpp"

namespace TSC {

    class cLava: public cMovingSprite {
    public:
        cLava(cSprite_Manager* p_sprite_manager);
        cLava(XmlAttributes& attributes, cSprite_Manager* p_sprite_manager);
        virtual ~cLava();

        virtual cLava* Copy() const;
        virtual void Update();
        virtual void Draw(cSurface_Request* p_request = NULL);

        virtual xmlpp::Element* Save_To_XML_Node(xmlpp::Element* p_element);

        void Set_Massive_Type(MassiveType type);

        virtual void Handle_Collision_Player(cObjectCollision* p_collision);
        virtual void Handle_Collision_Enemy(cObjectCollision* p_collision);

    protected:
        virtual std::string Get_XML_Type_Name();

    private:
        void Init();
    };

}

#endif

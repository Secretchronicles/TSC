/***************************************************************************
 * rescue_item.hpp
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

#ifndef TSC_RESCUE_ITEM_HPP
#define TSC_RESCUE_ITEM_HPP

namespace TSC {

    class cRescueItem: public cMovingSprite {
    public:
        cRescueItem(cSprite_Manager* sprite_manager);
        virtual ~cRescueItem();

        void Set_Item_Type(SpriteType type);

        virtual void Draw(cSurface_Request* request = NULL);
        virtual void Activate(void);

        virtual bool Is_Update_Valid(void);
        virtual Col_Valid_Type Validate_Collision(cSprite* obj);
        virtual void Handle_out_of_Level(ObjectDirection dir);
        virtual void Handle_Collision_Player(cObjectCollision* collision);
    };

}

#endif /* TSC_RESCUE_ITEM_HPP */

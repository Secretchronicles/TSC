/***************************************************************************
 * world_sprite_manager.h
 *
 * Copyright © 2008 - 2011 Florian Richter
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

#ifndef TSC_WORLD_SPRITE_MANAGER_HPP
#define TSC_WORLD_SPRITE_MANAGER_HPP

#include "../core/global_basic.hpp"
#include "../core/sprite_manager.hpp"
#include "../overworld/overworld.hpp"

namespace TSC {

    /* *** *** *** *** *** cWorld_Sprite_Manager *** *** *** *** *** *** *** *** *** *** *** *** */

    class cWorld_Sprite_Manager : public cSprite_Manager {
    public:
        cWorld_Sprite_Manager(cOverworld* overworld);
        virtual ~cWorld_Sprite_Manager(void);

        // Add a sprite
        virtual void Add(cSprite* sprite);

        // parent overworld
        cOverworld* m_overworld;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

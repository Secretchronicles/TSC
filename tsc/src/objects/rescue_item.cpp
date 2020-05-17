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

#include "../objects/powerup.hpp"
#include "../core/game_core.hpp"
#include "../level/level_player.hpp"
#include "../gui/hud.hpp"
#include "../core/framerate.hpp"
#include "../video/animation.hpp"
#include "../video/gl_surface.hpp"
#include "../user/savegame/savegame.hpp"
#include "../core/math/utilities.hpp"
#include "../core/i18n.hpp"
#include "../level/level.hpp"
#include "../scripting/events/activate_event.hpp"
#include "../core/global_basic.hpp"
#include "rescue_item.hpp"

// Place it in front of most stuff
#define RESCUE_ITEM_Z_POS 0.1299f

using namespace TSC;

cRescueItem::cRescueItem(cSprite_Manager* p_sprite_manager)
    : cMovingSprite(p_sprite_manager)
{
    m_sprite_array = ARRAY_ACTIVE;
    m_massive_type = MASS_PASSIVE;
    m_type = TYPE_UNDEFINED;
    m_pos_z = RESCUE_ITEM_Z_POS;
    m_gravity_max = 5.0f;
    m_can_be_on_ground = false;
    m_color.alpha = 150;

    Set_Spawned(true); // These are always spawned
}

cRescueItem::~cRescueItem()
{
    //
}

void cRescueItem::Draw(cSurface_Request* request)
{
    if (!m_valid_draw)
        return;

    if (editor_level_enabled)
        return;

    cMovingSprite::Draw(request);
}

bool cRescueItem::Is_Update_Valid(void)
{
    // if not visible
    if (!m_active) {
        return 0;
    }

    return 1;
}

void cRescueItem::Activate()
{
    pLevel_Player->Get_Item(m_type);
    Destroy();
}

/**
 * A RescueItem can only collide with the player, nothing else.
 */
Col_Valid_Type cRescueItem::Validate_Collision(cSprite* obj)
{
    // basic validation checking
    Col_Valid_Type basic_valid = Validate_Collision_Ghost(obj);

    // found valid collision
    if (basic_valid != COL_VTYPE_NOT_POSSIBLE) {
        return basic_valid;
    }

    // Can only collide with the player.
    if (obj->m_type == TYPE_PLAYER)
        return COL_VTYPE_INTERNAL;
    else
        return COL_VTYPE_NOT_VALID;

}

void cRescueItem::Handle_out_of_Level(ObjectDirection dir)
{
    Destroy();
}

void cRescueItem::Handle_Collision_Player(cObjectCollision* collision)
{
    Activate();
}

void cRescueItem::Set_Item_Type(SpriteType type)
{
    Set_Color_Combine(0, 0, 0, 0);
    Clear_Images();

    switch (type)
    {
    case TYPE_MUSHROOM_DEFAULT:
        Add_Image_Set("main", "game/items/berry_big.imgset");
        break;
    case TYPE_MUSHROOM_BLUE:
        Add_Image_Set("main", "game/items/berry_ice.imgset");
        break;
    case TYPE_FIREPLANT:
        Add_Image_Set("main", "game/items/berry_fire.imgset");
        break;
    default:
        std::cerr << "Warning: Invalid rescue item type set, using normal berry instead." << std::endl;
        type = TYPE_MUSHROOM_DEFAULT;
        Add_Image_Set("main", "game/items/berry_big.imgset");
    }

    m_type = type;
    Set_Image_Set("main", 1);
}

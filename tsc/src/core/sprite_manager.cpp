/***************************************************************************
 * sprite_manager.cpp  -  Sprite Manager
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

#include "../core/sprite_manager.hpp"
#include "../core/game_core.hpp"
#include "../level/level_player.hpp"
#include "../input/mouse.hpp"
#include "../overworld/world_player.hpp"
#include "../enemies/enemy.hpp"
#include "../core/global_basic.hpp"

using namespace std;

namespace TSC {

/* *** *** *** *** *** *** cSprite_Manager *** *** *** *** *** *** *** *** *** *** *** */

cSprite_Manager::cSprite_Manager(unsigned int reserve_items /* = 2000 */, unsigned int zpos_items /* = 100 */)
    : cObject_Manager<cSprite>()
{
    objects.reserve(reserve_items);

    m_max_uid_mark = 1; // UID 0 is reserved for the player
    m_z_pos_data.assign(zpos_items, 0.0f);
    m_z_pos_data_editor.assign(zpos_items,0.0f);
}

cSprite_Manager::~cSprite_Manager(void)
{
    Delete_All();
}

void cSprite_Manager::Add(cSprite* sprite)
{
    // empty object
    if (!sprite) {
        return;
    }

    // Ensure sprites of the same layer get slightly different Z
    // coordinates. See method docs in sprite_manager.hpp for more
    //information.
    Ensure_Different_Z(sprite);

    /* If the sprite already has a UID set, we accept it as-is. This is
     * usually the case when loading a level from the XML file. Otherwise
     * we generate a unique id. */
    if (sprite->m_uid <= 0) { // No UID set
        sprite->m_uid = Generate_UID();
    }
    else { // UID set
        // Ensure the pool knows about new maximum UIDs
        if (sprite->m_uid >= m_max_uid_mark) {
            // Allocate till our new mark
            Allocate_UIDs(sprite->m_uid + 1);
        }

//#ifdef _DEBUG
//        // This slows down performance, so only check this in debug mode
//        if (Is_UID_In_Use(sprite->m_uid))
//            std::cerr << "Warning : UID collision : UID " << sprite->m_uid << " is already in use." << std::endl;
//#endif

        // Mark the sprite’s UID as taken
        m_uid_pool.erase(sprite->m_uid);
    }

    // Check if an destroyed object can be replaced
    for (cSprite_List::iterator itr = objects.begin(); itr != objects.end(); ++itr) {
        // get object pointer
        cSprite* obj = (*itr);

        // if destroy is set
        if (obj->m_auto_destroy) {
            // set new object
            *itr = sprite;

            // Release old sprite’s UID by putting it back into the UID pool
            m_uid_pool.insert(obj->m_uid);

            // delete old
            delete obj;

            return;
        }
    }

    cObject_Manager<cSprite>::Add(sprite);
}

cSprite* cSprite_Manager::Copy(unsigned int identifier)
{
    if (identifier >= objects.size()) {
        return NULL;
    }

    return objects[identifier]->Copy();
}

void cSprite_Manager::Ensure_Different_Z(cSprite* sprite)
{
    /*The following sprites should never be placed over one another,
     * so they are excluded from the Z correction any may well have
     * equal Z coordinates. As the Z correction focuses on the
     * massivity, we would otherwise e.g. have the enemies (which are
     * mostly massive) given Z coordinates from the line of normal
     * massive tiles, causing for example flyons to appear over their
     * containing pipe (given that the flyon sprite was added after
     * the pipe sprite). */
    if (dynamic_cast<cEnemy*>(sprite) ||
        sprite->m_type == TYPE_ANIMATION ||
        sprite->m_type == TYPE_PARTICLE_EMITTER ||
        sprite->m_type == TYPE_OW_WAYPOINT ||
        sprite->m_type == TYPE_OW_LINE_START ||
        sprite->m_type == TYPE_OW_LINE_END)
        return;
    // TODO: Replace that with dynamic_cast<> alltogether? See issue #44.

    // set new Z position if not higher than a prior Z of
    // the same massivity.
    if (sprite->m_pos_z <= m_z_pos_data[sprite->m_massive_type]) {
        sprite->m_pos_z = m_z_pos_data[sprite->m_massive_type] + cSprite::m_pos_z_delta;
    }
    // Same for editor
    if (sprite->m_editor_pos_z > 0.0f) {
        if (sprite->m_editor_pos_z <= m_z_pos_data_editor[sprite->m_massive_type]) {
            sprite->m_editor_pos_z = m_z_pos_data_editor[sprite->m_massive_type] + cSprite::m_pos_z_delta;
        }
    }


    // Update our Z memory for the next call of this method
    // (so this Z is not given out again!)
    if (sprite->m_pos_z > m_z_pos_data[sprite->m_massive_type]) {
        m_z_pos_data[sprite->m_massive_type] = sprite->m_pos_z;
    }

    // Same for editor Z memory
    if (sprite->m_editor_pos_z > 0.0f) {
        if (sprite->m_editor_pos_z > m_z_pos_data_editor[sprite->m_massive_type]) {
            m_z_pos_data_editor[sprite->m_massive_type] = sprite->m_editor_pos_z;
        }
    }
}

void cSprite_Manager::Move_To_Front(cSprite* sprite)
{
    // not needed
    if (objects.size() <= 1) {
        return;
    }

    cSprite* first = objects.front();

    // if already in front
    if (sprite == first) {
        return;
    }

    // get iterator
    cSprite_List::iterator itr = std::find(objects.begin(), objects.end(), sprite);

    // not available
    if (itr == objects.end()) {
        // fixme : should not happen but it does
        return;
    }

    objects.erase(itr);
    objects.front() = sprite;
    objects.insert(objects.begin() + 1, first);

    // make it the first z position
    sprite->m_pos_z = Get_First(sprite->m_type)->m_pos_z - cSprite::m_pos_z_delta;
}

void cSprite_Manager::Move_To_Back(cSprite* sprite)
{
    // not needed
    if (objects.size() <= 1) {
        return;
    }

    cSprite* last = objects.back();

    // if already in back
    if (sprite == last) {
        return;
    }

    // get iterator
    cSprite_List::iterator itr = std::find(objects.begin(), objects.end(), sprite);

    // not available
    if (itr == objects.end()) {
        // fixme : should not happen but it does
        return;
    }

    objects.erase(itr);
    objects.back() = sprite;
    objects.insert(objects.end() - 1, last);

    // make it the last z position
    Ensure_Different_Z(sprite);
}

void cSprite_Manager::Delete_All(bool delayed /* = 0 */)
{
    // delayed
    if (delayed) {
        for (cSprite_List::iterator itr = objects.begin(); itr != objects.end(); ++itr) {
            // get object pointer
            cSprite* obj = (*itr);

            obj->Destroy(); // Marks for autodestroy in cSprite_Manager::Add()
        }
    }
    // instant
    else {
        // remove objects that can not be auto-deleted
        for (cSprite_List::iterator itr = objects.begin(); itr != objects.end();) {
            // get object pointer
            cSprite* obj = (*itr);

            if (obj->m_disallow_managed_delete) {
                itr = objects.erase(itr);
            }
            // increment
            else {
                ++itr;
            }
        }

        cObject_Manager<cSprite>::Delete_All();
    }

    // Empty the UID pool, we have no sprites anymore
    m_uid_pool.clear();

    // clear z position data
    std::fill(m_z_pos_data.begin(), m_z_pos_data.end(), 0.0f);
    std::fill(m_z_pos_data_editor.begin(), m_z_pos_data_editor.end(), 0.0f);
}

cSprite* cSprite_Manager::Get_First(const SpriteType type) const
{
    cSprite* first = NULL;

    for (cSprite_List::const_iterator itr = objects.begin(); itr != objects.end(); ++itr) {
        // get object pointer
        cSprite* obj = (*itr);

        if (obj->m_type == type && (!first || obj->m_pos_z < first->m_pos_z)) {
            first = obj;
        }
    }

    // return result
    return first;
}

cSprite* cSprite_Manager::Get_Last(const SpriteType type) const
{
    cSprite* last = NULL;

    for (cSprite_List::const_iterator itr = objects.begin(); itr != objects.end(); ++itr) {
        // get object pointer
        cSprite* obj = (*itr);

        if (obj->m_type == type && (!last || obj->m_pos_z > last->m_pos_z)) {
            last = obj;
        }
    }

    // return result
    return last;
}

cSprite* cSprite_Manager::Get_from_Position(int start_pos_x, int start_pos_y, const SpriteType type /* = TYPE_UNDEFINED */, bool check_pos /* = false */) const
{
    for (cSprite_List::const_iterator itr = objects.begin(); itr != objects.end(); ++itr) {
        // get object pointer
        cSprite* obj = (*itr);

        if (static_cast<int>(obj->m_start_pos_x) != start_pos_x || static_cast<int>(obj->m_start_pos_y) != start_pos_y) {
            continue;
        }

        if (check_pos && (static_cast<int>(obj->m_pos_x) != start_pos_x || static_cast<int>(obj->m_pos_y) != start_pos_y)) {
            continue;
        }

        // if type is given
        if (type != TYPE_UNDEFINED) {
            // skip invalid type
            if (obj->m_type != type) {
                continue;
            }
        }

        // found
        return obj;
    }

    return NULL;
}

cSprite* cSprite_Manager::Get_by_UID(int uid) const
{
    for (cSprite_List::const_iterator itr = objects.begin(); itr != objects.end(); ++itr) {
        if ((*itr)->m_uid == uid)
            return *itr;
    }
    return NULL;
}

void cSprite_Manager::Get_Objects_sorted(cSprite_List& new_objects, bool editor_sort /* = 0 */, bool with_player /* = 0 */) const
{
    new_objects = objects;

    if (with_player) {
        new_objects.push_back(pActive_Player);
    }

    // z position sort
    if (!editor_sort) {
        // default
        std::sort(new_objects.begin(), new_objects.end(), zpos_sort());
    }
    else {
        // editor
        std::sort(new_objects.begin(), new_objects.end(), editor_zpos_sort());
    }
}

void cSprite_Manager::Get_Colliding_Objects(cSprite_List& col_objects, const GL_rect& rect, bool with_player /* = 0 */, const cSprite* exclude_sprite /* = NULL */) const
{
    // Check objects
    for (cSprite_List::const_iterator itr = objects.begin(); itr != objects.end(); ++itr) {
        // get object pointer
        cSprite* obj = (*itr);

        // if destroyed object
        if (obj == exclude_sprite || obj->m_auto_destroy) {
            continue;
        }

        // if rects don't touch
        if (!rect.Intersects(obj->m_col_rect)) {
            continue;
        }

        col_objects.push_back(obj);
    }

    if (with_player && pActive_Player != exclude_sprite) {
        if (rect.Intersects(pActive_Player->m_col_rect)) {
            col_objects.push_back(pActive_Player);
        }
    }
}

void cSprite_Manager::Get_Colliding_Objects(cSprite_List& col_objects, const GL_Circle& circle, bool with_player /* = 0 */, const cSprite* exclude_sprite /* = NULL */) const
{
    // Check objects
    for (cSprite_List::const_iterator itr = objects.begin(); itr != objects.end(); ++itr) {
        // get object pointer
        cSprite* obj = (*itr);

        // if destroyed object
        if (obj == exclude_sprite || obj->m_auto_destroy) {
            continue;
        }

        // if circles don't touch
        if (!circle.Intersects(obj->m_col_rect)) {
            continue;
        }

        col_objects.push_back(obj);
    }

    if (with_player && pActive_Player != exclude_sprite) {
        if (circle.Intersects(pActive_Player->m_col_rect)) {
            col_objects.push_back(pActive_Player);
        }
    }
}

void cSprite_Manager::Handle_Collision_Items(void)
{
    for (cSprite_List::iterator itr = objects.begin(); itr != objects.end(); ++itr) {
        cSprite* obj = (*itr);

        // invalid
        if (obj->m_auto_destroy) {
            if (obj->m_collisions.size()) {
                debug_print("Collision with a destroyed object (%s)\n", obj->Create_Name().c_str());
                obj->Clear_Collisions();
            }

            continue;
        }

        // collision and movement handling
        obj->Collide_Move();
        // handle found collisions
        obj->Handle_Collisions();
    }
}

unsigned int cSprite_Manager::Get_Size_Array(const ArrayType sprite_array)
{
    unsigned int count = 0;

    for (cSprite_List::const_iterator itr = objects.begin(); itr != objects.end(); ++itr) {
        if ((*itr)->m_sprite_array == sprite_array) {
            count++;
        }
    }

    return count;
}

/* The member m_uid_pool contains a list of all those UIDs that
 * are *not* currently in use, sorted from the smallest to
 * the greatest (not necessarily without gaps, as destroyed
 * sprites give their UID back into the pool). This allows
 * use to quickly find the next free UID without much searching
 * by just picking the very first element from m_uid_pool,
 * which, as said, is always sorted and stays so.
 *
 * However, at the level start this would mean that m_uid_pool
 * must contain infinitely many numbers reaching from 1 to ∞. Well,
 * OK, not ∞, because CEGUI’s XML handler can only handle the
 * bare `int' type and is hence limited to INT_MAX. As we don’t want
 * to allocate space for all those possible UIDs which we will likely
 * never need right from the start on, we instead just allocate the
 * next ten UIDs for the pool if it is empty, remembering the new
 * highest possible UID in m_max_uid_mark. */
int cSprite_Manager::Generate_UID()
{
    // Allocate 10 new UIDs if the pool is empty
    if (m_uid_pool.empty())
        Allocate_UIDs(m_max_uid_mark + 10);

    // Pool is not empty, return the first available UID.
    std::set<int>::const_iterator iter = m_uid_pool.begin();
    int id = *iter;
    m_uid_pool.erase(iter);
    return id;
}

// We need `long', because we must check an `int' overflow (see below)
void cSprite_Manager::Allocate_UIDs(long new_max_uid_mark)
{
    // The pool can’t be shrinked
    if (new_max_uid_mark <= m_max_uid_mark)
        return;

    // int is the only type CEGUI’s XML handler can handle. Therefore, we
    // must refuse the generation of new UIDs beyond the maximum of what an
    // int can hold.
    if (new_max_uid_mark >= INT_MAX)
        throw(std::range_error("Too many sprites, unable to generate further UIDs!"));

    // Actually allocate the numbers for the UID pool
    for (int i = m_max_uid_mark; i < static_cast<int>(new_max_uid_mark); i++) // new_max_uid_mark is guaranteed to be < INT_MAX
        m_uid_pool.insert(i);

    // Remember the new maximum. Note that by checking INT_MAX, we have
    // ensured the values fits into an int.
    m_max_uid_mark = static_cast<int>(new_max_uid_mark);
}

bool cSprite_Manager::Is_UID_In_Use(int uid)
{
    // The "invalid UID" always is in use
    if (uid == 0)
        return true;

    // If the UID is in the pool, it is free. If it is greater or equal to
    // the pool border marker, it is free. Otherwise, it is in use.
    if (uid >= m_max_uid_mark)
        return false;

    if (m_uid_pool.find(uid) == m_uid_pool.end())
        return true;

    return false;
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

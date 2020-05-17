/***************************************************************************
 * sprite_manager.h
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

#ifndef TSC_SPRITE_MANAGER_HPP
#define TSC_SPRITE_MANAGER_HPP

#include "../core/global_game.hpp"
#include "../core/obj_manager.hpp"
#include "../objects/movingsprite.hpp"

namespace TSC {

    /* *** *** *** *** *** cSprite_Manager *** *** *** *** *** *** *** *** *** *** *** *** */

    class cSprite_Manager : public cObject_Manager<cSprite> {
    public:
        cSprite_Manager(unsigned int reserve_items = 2000, unsigned int zpos_items = 100);
        virtual ~cSprite_Manager(void);

        /* Add a sprite
         * If the sprite has m_uid set to something greater than 0,
         * it will not be touched, otherwise it is assigned a free UID.
         */
        virtual void Add(cSprite* sprite);

        // Return a sprite copy
        cSprite* Copy(unsigned int identifier);

        /* Move the sprite to the front of the array
         * the sprite is then behind other sprites on the screen
         * this also sets the z position
        */
        void Move_To_Front(cSprite* sprite);
        /* Move the sprite to the back of the array
         * the sprite is then in front of other sprites on the screen
         * this also sets the z position
        */
        void Move_To_Back(cSprite* sprite);

        /* Delete all objects
         * if delayed is set deletion will only occur if replaced
         */
        virtual void Delete_All(bool delayed = 0);

        // Return the first z position object from the given type
        cSprite* Get_First(const SpriteType type) const;
        // Return the last z position object from the given type
        cSprite* Get_Last(const SpriteType type) const;
        /* Return the matching object from the given start position
         * type : if not set to zero only returns the object with the given type
         * check_pos : if set to true the normal position must be equal to the start position as well
        */
        cSprite* Get_from_Position(int start_pos_x, int start_pos_y, const SpriteType type = TYPE_UNDEFINED, bool check_pos = false) const;
        /* Return the object assigned the given UID. Returns NULL
         * if no object has this UID.
         */
        cSprite* Get_by_UID(int uid) const;

        /* Get a sorted Objects Array
         * editor_sort : if set sorts from editor z pos
         * with_player : include player
        */
        void Get_Objects_sorted(cSprite_List& new_objects, bool editor_sort = 0, bool with_player = 0) const;
        /* Get objects colliding with the given rectangle/circle
         * with_player : include player in check
         * exclude_sprite : exclude the given sprite from check
        */
        void Get_Colliding_Objects(cSprite_List& col_objects, const GL_rect& rect, bool with_player = 0, const cSprite* exclude_sprite = NULL) const;
        void Get_Colliding_Objects(cSprite_List& col_objects, const GL_Circle& circle, bool with_player = 0, const cSprite* exclude_sprite = NULL) const;

        // Update items drawing validation
        inline void Update_Items_Valid_Draw(void)
        {
            for (cSprite_List::iterator itr = objects.begin(); itr != objects.end(); ++itr) {
                (*itr)->Update_Valid_Draw();
            }
        }
        // Update items
        inline void Update_Items(void)
        {
            for (cSprite_List::iterator itr = objects.begin(); itr != objects.end(); ++itr) {
                (*itr)->Update();
            }
        }
        // Update_Late items
        inline void Update_Items_Late(void)
        {
            for (cSprite_List::iterator itr = objects.begin(); itr != objects.end(); ++itr) {
                (*itr)->Update_Late();
            }
        }
        // Draw items
        inline void Draw_Items(void)
        {
            for (cSprite_List::iterator itr = objects.begin(); itr != objects.end(); ++itr) {
                (*itr)->Draw();
            }
        }

        // Create Collision data and Handle the collisions
        void Handle_Collision_Items(void);


        /* Return the current size
         * of the specified sprite array
         */
        unsigned int Get_Size_Array(const ArrayType sprite_array);

        // Return object pointer if found
        cSprite* operator [](unsigned int identifier)
        {
            return Get_Pointer(identifier);
        }

        // Generate a new and unused sprite ID. Throws std::range_error if
        // no IDs can be generated anymore (more than INT_MAX objects are
        // requested).
        int Generate_UID();
        // Returns true if the given UID already exists, false otherwise.
        bool Is_UID_In_Use(int uid);
        // Allocate new UIDs in the pool of available UIDs. The new maximum
        // available uid is `new_max_uid_mark - 1'. This method does nothing
        // if `new_max_uid_mark' is smaller than the current max mark.
        void Allocate_UIDs(long new_max_uid_mark);

        typedef vector<float> ZposList;
        // biggest type z position
        ZposList m_z_pos_data;
        // biggest editor type z position
        ZposList m_z_pos_data_editor;
        // This set holds the not-yet-used UIDs so we can easily
        // find the next free one.
        std::set<int> m_uid_pool;
        // The UID pool is filled as needed. This is always the first
        // non-yet allocated UID.
        int m_max_uid_mark;

        // Z position sort
        struct zpos_sort {
            bool operator()(const cSprite* a, const cSprite* b) const
            {
                return a->m_pos_z > b->m_pos_z;
            }
        };

        // Editor Z position sort
        struct editor_zpos_sort {
            bool operator()(const cSprite* a, const cSprite* b) const
            {
                // check if the editor zpos is available
                if (!a->m_editor_pos_z) {
                    if (!b->m_editor_pos_z) {
                        return a->m_pos_z < b->m_pos_z;
                    }

                    return a->m_pos_z < b->m_editor_pos_z;
                }
                if (!b->m_editor_pos_z) {
                    if (!a->m_editor_pos_z) {
                        return a->m_pos_z < b->m_pos_z;
                    }

                    return a->m_editor_pos_z < b->m_pos_z;
                }

                // both objects have an editor z pos
                return a->m_editor_pos_z < b->m_editor_pos_z;
            }
        };

    private:
        /* When multiple sprites of the same massivity are placed
         * on the same place (think two hills before one another,
         * where one may be higher than the other), they would
         * have the same Z coordinate by default, resulting in
         * a floating-point precision race which one gets rendered
         * in front and which one behind. This method is called from
         * Add() and adds a very tiny amount of Z to every new sprite
         * of a given massivity so that sprites that are added later
         * are ensured to be placed in front of older ones.
         */
        void Ensure_Different_Z(cSprite* sprite);
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

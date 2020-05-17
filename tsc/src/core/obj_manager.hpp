/***************************************************************************
 * obj_manager.h  -  Generic object manager
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

#ifndef TSC_OBJ_MANAGER_HPP
#define TSC_OBJ_MANAGER_HPP

#include "../core/global_basic.hpp"

namespace TSC {

    /* *** *** *** *** *** cObject_Manager *** *** *** *** *** *** *** *** *** *** *** *** */

    template<class T> class cObject_Manager {
    public:
        cObject_Manager(void) {};
        virtual ~cObject_Manager(void) {};

        //  Add the given object
        virtual void Add(T* obj)
        {
            objects.push_back(obj);
        }

        // Delete the object from given array number
        virtual bool Delete(size_t array_num, bool delete_data = 1)
        {
            // if in vector
            if (array_num < objects.size()) {
                objects.erase(objects.begin() + array_num);
            }

            if (delete_data) {
                delete objects[array_num];
            }

            return 1;
        }

        // Delete the given object
        virtual bool Delete(T* obj, bool delete_data = 1)
        {
            // empty object
            if (!obj) {
                return 0;
            }

            // get iterator
            typename vector<T*>::iterator itr = std::find(objects.begin(), objects.end(), obj);

            // available in vector
            if (itr != objects.end()) {
                // erase
                objects.erase(itr);
            }

            if (delete_data) {
                delete obj;
            }

            return 1;
        }

        // Delete all objects
        virtual void Delete_All(void)
        {
            for (typename vector<T*>::iterator itr = objects.begin(); itr != objects.end(); ++itr) {
                delete *itr;
            }

            objects.clear();
        }

        /* Return the object pointer
         * if not found returns NULL
        */
        virtual T* Get_Pointer(size_t identifier)
        {
            if (identifier >= objects.size()) {
                // out of array
                return NULL;
            }

            // available
            return objects[identifier];
        }

        // Switch objects array position
        bool Switch_Array_Num(T* obj1, T* obj2)
        {
            // empty object
            if (!obj1 || !obj2) {
                return 0;
            }

            // if the same
            if (obj1 == obj2) {
                return 0;
            }

            typename vector<T*>::iterator itr1 = std::find(objects.begin(), objects.end(), obj1);

            // not in vector
            if (itr1 == objects.end()) {
                return 0;
            }

            typename vector<T*>::iterator itr2 = std::find(objects.begin(), objects.end(), obj2);

            // not in vector
            if (itr2 == objects.end()) {
                return 0;
            }

            // todo: does that work ?
            itr1 = obj2;
            itr2 = obj1;

            return 1;
        }

        /* Return the object array number
         * if not found returns -1
        */
        int Get_Array_Num(T* obj) const
        {
            // invalid
            if (!obj) {
                return -1;
            }

            typename vector<T*>::const_iterator itr_obj = std::find(objects.begin(), objects.end(), obj);

            // not in vector
            if (itr_obj == objects.end()) {
                return -1;
            }

            return std::distance(objects.begin(), itr_obj);
        }

        // Return the object count
        size_t size(void) const
        {
            return objects.size();
        }

        // Return true if no objects
        bool empty(void) const
        {
            return objects.empty();
        }

        vector<T*> objects;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

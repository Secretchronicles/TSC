/***************************************************************************
 * krush.h
 *
 * Copyright © 2004 - 2011 Florian Richter
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

#ifndef TSC_KRUSH_HPP
#define TSC_KRUSH_HPP

#include "../enemies/enemy.hpp"
#include "../scripting/objects/enemies/mrb_krush.hpp"

namespace TSC {

    /* *** *** *** *** *** cKrush *** *** *** *** *** *** *** *** *** *** *** *** */
    /* Walking all Day and doesn't stop even if you hit him the first time :O
     * but the second time he's out.
     * Colors : Red
     */
    class cKrush : public cEnemy {
    public:
        // constructor
        cKrush(cSprite_Manager* sprite_manager);
        // create from stream
        cKrush(XmlAttributes& attributes, cSprite_Manager* sprite_manager);
        // destructor
        virtual ~cKrush(void);

        // init defaults
        void Init(void);
        // copy
        virtual cKrush* Copy(void) const;

        // load from savegame
        virtual void Load_From_Savegame(cSave_Level_Object* save_object);

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "Krush"), &Scripting::rtTSC_Scriptable, this));
        }

        // Set Direction
        virtual void Set_Direction(const ObjectDirection dir);

        /* Move into the opposite Direction
         * if col_dir is given only turns around if the collision direction is in front
         */
        virtual void Turn_Around(ObjectDirection col_dir = DIR_UNDEFINED);

        /* downgrade state ( if already weakest state : dies )
         * force : usually dies or a complete downgrade
        */
        virtual void DownGrade(bool force = 0);

        // set the moving state
        void Set_Moving_State(Moving_state new_state);

        // update
        virtual void Update(void);

        // update maximum velocity values
        void Update_Velocity_Max(void);

        /* Validate the given collision object
         * returns 0 if not valid
         * returns 1 if an internal collision with this object is valid
         * returns 2 if the given object collides with this object (blocking)
        */
        virtual Col_Valid_Type Validate_Collision(cSprite* obj);
        // collision from player
        virtual void Handle_Collision_Player(cObjectCollision* collision);
        // collision from an enemy
        virtual void Handle_Collision_Enemy(cObjectCollision* collision);
        // collision with massive
        virtual void Handle_Collision_Massive(cObjectCollision* collision);
        virtual void Handle_Collision_Box( ObjectDirection cdirection, GL_rect *r2 );

        // editor activation
        virtual void Editor_Activate(void);
        // editor direction option selected event
        bool Editor_Direction_Select(const CEGUI::EventArgs& event);

        // Save to XML node
        virtual xmlpp::Element* Save_To_XML_Node(xmlpp::Element* p_element);

    protected:

        virtual std::string Get_XML_Type_Name();

        // image indexes
        int m_big_end;
        int m_small_start;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

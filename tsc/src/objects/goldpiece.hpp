/***************************************************************************
 * goldpiece.h
 *
 * Copyright © 2003 - 2011 Florian Richter
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

#ifndef TSC_GOLDPIECE_HPP
#define TSC_GOLDPIECE_HPP

#include "../core/global_basic.hpp"
#include "../core/xml_attributes.hpp"
#include "../objects/movingsprite.hpp"
#include "../scripting/objects/specials/mrb_jewel.hpp"
#include "../scripting/objects/specials/mrb_jumping_jewel.hpp"
#include "../scripting/objects/specials/mrb_falling_jewel.hpp"

namespace TSC {

    /* *** *** *** *** *** cGoldpiece *** *** *** *** *** *** *** *** *** *** *** *** */

    class cGoldpiece : public cMovingSprite {
    public:
        // constructor
        cGoldpiece(cSprite_Manager* sprite_manager, DefaultColor gold_color = COL_YELLOW);
        // create from stream
        cGoldpiece(XmlAttributes& attributes, cSprite_Manager* sprite_manager);
        // destructor
        virtual ~cGoldpiece(void);

        // init defaults
        void Init(DefaultColor color = COL_YELLOW);

        // copy
        virtual cGoldpiece* Copy(void) const;

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "Jewel"), &Scripting::rtTSC_Scriptable, this));
        }

        // load from savegame
        virtual void Load_From_Savegame(cSave_Level_Object* save_object);

        // Set the gold color
        virtual void Set_Gold_Color(DefaultColor color);

        // activate the goldpiece
        void Activate(void);

        // update
        virtual void Update(void);
        // draw
        virtual void Draw(cSurface_Request* request = NULL);

        // if update is valid for the current state
        virtual bool Is_Update_Valid();

        // collision from player
        virtual void Handle_Collision_Player(cObjectCollision* collision);
        // collision with lava
        virtual void Handle_Collision_Lava(cObjectCollision* collision);

        // gold color
        DefaultColor m_color_type;

        // Save to node
        virtual xmlpp::Element* Save_To_XML_Node(xmlpp::Element* p_element);

    protected:
        // save to stream
        virtual std::string Get_XML_Type_Name();
    };

    /* *** *** *** *** *** cJGoldpiece *** *** *** *** *** *** *** *** *** *** *** *** */

    /* Jumping Goldpiece
     * used by Goldbox
    */
    class cJGoldpiece : public cGoldpiece {
    public:
        cJGoldpiece(cSprite_Manager* sprite_manager, DefaultColor color = COL_YELLOW);
        virtual ~cJGoldpiece(void);

        // update
        virtual void Update(void);

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "JumpingJewel"), &Scripting::rtTSC_Scriptable, this));
        }

        /* Validate the given collision object
         * returns 0 if not valid
         * returns 1 if an internal collision with this object is valid
         * returns 2 if the given object collides with this object (blocking)
        */
        virtual Col_Valid_Type Validate_Collision(cSprite* obj);
    };

    /* *** *** *** *** *** cFGoldpiece *** *** *** *** *** *** *** *** *** *** *** *** */

// Falling Goldpiece
    class cFGoldpiece : public cGoldpiece {
    public:
        // if direction is undefined it moves into a random direction
        cFGoldpiece(cSprite_Manager* sprite_manager, ObjectDirection dir = DIR_UNDEFINED, DefaultColor color = COL_YELLOW);
        virtual ~cFGoldpiece(void);

        // update
        virtual void Update(void);

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "FallingJewel"), &Scripting::rtTSC_Scriptable, this));
        }

        /* Validate the given collision object
         * returns 0 if not valid
         * returns 1 if an internal collision with this object is valid
         * returns 2 if the given object collides with this object (blocking)
        */
        virtual Col_Valid_Type Validate_Collision(cSprite* obj);
        // collision with massive
        virtual void Handle_Collision_Massive(cObjectCollision* collision);
        // collision from a box
        virtual void Handle_Collision_Box(ObjectDirection cdirection, GL_rect* r2);
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

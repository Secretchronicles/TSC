/***************************************************************************
 * box.h
 *
 * Copyright © 2003 - 2011 Florian Richter
 * Copyright © 2013 - 2014 The TSC Contributors
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TSC_BOX_HPP
#define TSC_BOX_HPP

#include "../core/global_basic.hpp"
#include "../core/xml_attributes.hpp"
#include "../objects/animated_sprite.hpp"
#include "../scripting/objects/boxes/mrb_box.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** *** Box invisible types *** *** *** *** *** *** *** *** *** */

    enum Box_Invisible_Type {
        // always visible
        BOX_VISIBLE = 0,
        // visible after activation
        BOX_INVISIBLE_MASSIVE = 1,
        // only visible in ghost mode
        BOX_GHOST = 2,
        // visible after activation and only touchable in the activation direction
        BOX_INVISIBLE_SEMI_MASSIVE = 3
    };

    /* *** *** *** *** *** *** *** *** cBaseBox *** *** *** *** *** *** *** *** *** */

    class cBaseBox : public cAnimated_Sprite {
    public:
        cBaseBox(cSprite_Manager* sprite_manager);
        virtual ~cBaseBox(void);

        // load from stream
        virtual void Load_From_XML(XmlAttributes& attributes);

        // load from savegame
        virtual void Load_From_Savegame(cSave_Level_Object* save_object);
        // save to savegame
        virtual cSave_Level_Object* Save_To_Savegame(void);

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "Box"), &Scripting::rtTSC_Scriptable, this));
        }

        /* Set the Animation Type
         * new_anim_type can be : Bonus, Default or Power
        */
        void Set_Animation_Type(const std::string& new_anim_type);
        // sets the count this object can be activated
        virtual void Set_Useable_Count(int count, bool new_startcount = 0);
        // Set invisible type
        void Set_Invisible(Box_Invisible_Type type);

        virtual void Set_Massive_Type(MassiveType type);

        // activates collision movement
        void Activate_Collision(ObjectDirection col_direction);
        // updates the collision movement
        void Update_Collision(void);

        // check for collision with objects
        void Check_Collision(ObjectDirection col_direction);
        // collision with the given enemy
        void Col_Enemy(cSprite* obj);

        // activate
        virtual void Activate(void);

        // update
        virtual void Update(void);
        // draw
        virtual void Draw(cSurface_Request* request = NULL);

        // Generate activation Particles
        void Generate_Activation_Particles(void);

        // if update is valid for the current state
        virtual bool Is_Update_Valid();
        // if draw is valid for the current state and position
        virtual bool Is_Draw_Valid(void);

        /* Validate the given collision object
         * returns 0 if not valid
         * returns 1 if an internal collision with this object is valid
         * returns 2 if the given object collides with this object (blocking)
        */
        virtual Col_Valid_Type Validate_Collision(cSprite* obj);
        // handle the basic box player collision
        virtual void Handle_Collision_Player(cObjectCollision* collision);
        // handle the basic box enemy collision
        virtual void Handle_Collision_Enemy(cObjectCollision* collision);

        // editor activation
        virtual void Editor_Activate(void);
        // editor useable count text changed event
        bool Editor_Useable_Count_Text_Changed(const CEGUI::EventArgs& event);
        // editor invisible option selected event
        bool Editor_Invisible_Select(const CEGUI::EventArgs& event);

        // animation type
        std::string m_anim_type;
        // box type
        SpriteType box_type;

        // leveleditor item image
        cGL_Surface* m_item_image;

        // moving direction when activated
        ObjectDirection m_move_col_dir;
        // current moving counter ( if activated )
        float m_move_counter;
        // if object is moving back to the original position
        bool m_move_back;
        /* number of times this box object can be activated
         * the box is visible if useable_count != start_useable_count
         * if set to -1 it is infinite (and is set to -2 if activated)
        */
        int m_start_useable_count;
        int m_useable_count;

        // box invisible type
        Box_Invisible_Type m_box_invisible;

        // active particle animation counter
        float m_particle_counter_active;

        // Save to XML node
        virtual xmlpp::Element* Save_To_XML_Node(xmlpp::Element* p_element);
        virtual std::string Create_Name(void) const;

    protected:
        virtual std::string Get_XML_Type_Name();
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

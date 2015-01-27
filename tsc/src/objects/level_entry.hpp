/***************************************************************************
 * level_entry.h
 *
 * Copyright © 2007 - 2011 Florian Richter
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

#ifndef TSC_LEVEL_ENTRY_HPP
#define TSC_LEVEL_ENTRY_HPP

#include "../core/global_basic.hpp"
#include "../objects/movingsprite.hpp"
#include "../scripting/objects/specials/mrb_level_entry.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** Level Entry types *** *** *** *** *** *** *** *** *** *** */

    enum Level_Entry_type {
        LEVEL_ENTRY_BEAM        = 0,    // no animation ( f.e. a door or hole )
        LEVEL_ENTRY_WARP        = 1     // rotated player moves slowly into the destination direction
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

    /* Level Entry
     *
    */
    class cLevel_Entry : public cMovingSprite {
    public:
        // constructor
        cLevel_Entry(cSprite_Manager* sprite_manager);
        // create from stream
        cLevel_Entry(XmlAttributes& attributes, cSprite_Manager* sprite_manager);
        // destructor
        virtual ~cLevel_Entry(void);

        // init defaults
        void Init(void);
        // copy this sprite
        virtual cLevel_Entry* Copy(void) const;

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "LevelEntry"), &Scripting::rtTSC_Scriptable, this));
        }

        // Set direction
        void Set_Direction(const ObjectDirection dir);

        virtual void Set_Massive_Type(MassiveType type);

        // draw
        virtual void Draw(cSurface_Request* request = NULL);

        // Get centered player position y
        float Get_Player_Pos_X(void) const;
        // Get centered player position y
        float Get_Player_Pos_Y(void) const;

        // Activate
        void Activate(void);

        // Set the type
        void Set_Type(Level_Entry_type new_type);
        // Set the name
        void Set_Name(const std::string& str_name);

        // if draw is valid for the current state and position
        virtual bool Is_Draw_Valid(void);

        // editor activation
        virtual void Editor_Activate(void);
        // editor direction option selected event
        bool Editor_Direction_Select(const CEGUI::EventArgs& event);
        // editor name text changed event
        bool Editor_Name_Text_Changed(const CEGUI::EventArgs& event);

        // level entry type
        Level_Entry_type m_entry_type;
        // identification name
        std::string m_entry_name;

        // editor type color
        Color m_editor_color;
        // editor entry name text
        cGL_Surface* m_editor_entry_name;

        // Save to node
        virtual xmlpp::Element* Save_To_XML_Node(xmlpp::Element* p_element);
        virtual std::string  Create_Name(void) const;

    protected:
        virtual std::string Get_XML_Type_Name();
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

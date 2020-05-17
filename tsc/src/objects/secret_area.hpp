/***************************************************************************
 * secret_area.hpp
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
#ifndef TSC_SECRET_AREA_HPP
#define TSC_SECRET_AREA_HPP

#include "../core/global_basic.hpp"
#include "../objects/movingsprite.hpp"

namespace TSC {

    class cSecret_Area: public cMovingSprite {
    public:
        // constructor
        cSecret_Area(cSprite_Manager* sprite_manager);
        // create from stream
        cSecret_Area(XmlAttributes& attributes, cSprite_Manager* sprite_manager);
        // destructor
        virtual ~cSecret_Area(void);

        // init defaults
        void Init();
        // copy this sprite
        virtual cSecret_Area* Copy(void) const;

        virtual void Update(void);
        virtual void Draw(cSurface_Request* request = NULL);
        virtual void Set_Massive_Type(MassiveType type);

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "SecretArea"), &Scripting::rtTSC_Scriptable, this));
        }

        // if draw is valid for the current state and position
        virtual bool Is_Draw_Valid(void);

        void Activate(void);
        void Set_Rect(GL_rect rect, bool new_start_pos = false);

        virtual void Editor_Activate(void);
        virtual void Editor_Deactivate(void);
        virtual void Editor_State_Update(void);

        virtual xmlpp::Element* Save_To_XML_Node(xmlpp::Element* p_element);
        virtual bool Save_To_Savegame_XML_Node(xmlpp::Element* p_element) const;
        virtual void Load_From_Savegame(cSave_Level_Object* save_object);

        CEGUI::Window* mp_msg_window;
        float m_transparency_counter;
        float m_move_counter;
        bool m_activated;
    protected:
        virtual std::string Get_XML_Type_Name();
    };
}

#endif /* TSC_SECRET_AREA_HPP */

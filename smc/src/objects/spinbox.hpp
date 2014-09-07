/***************************************************************************
 * spinbox.h
 *
 * Copyright © 2003 - 2011 The SMC Contributors
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SMC_SPINBOX_HPP
#define SMC_SPINBOX_HPP

#include "../core/global_basic.hpp"
#include "../objects/box.hpp"
#include "../scripting/objects/boxes/mrb_spinbox.hpp"

namespace SMC {

    /* *** *** *** *** *** *** *** *** cSpinBox *** *** *** *** *** *** *** *** *** */

    class cSpinBox : public cBaseBox {
    public:
        // constructor
        cSpinBox(cSprite_Manager* sprite_manager);
        // create from stream
        cSpinBox(XmlAttributes& attributes, cSprite_Manager* sprite_manager);
        // destructor
        virtual ~cSpinBox(void);

        // init defaults
        void Init(void);

        // copy
        virtual cSpinBox* Copy(void) const;

        // load from stream
        virtual void Load_From_XML(XmlAttributes& attributes);

        // load from savegame
        virtual void Load_From_Savegame(cSave_Level_Object* save_object);
        // save to savegame
        virtual cSave_Level_Object* Save_To_Savegame(void);

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "SpinBox"), &Scripting::rtSMC_Scriptable, this));
        }

        // Activate the Spinning
        virtual void Activate(void);
        // Stop the Spinning
        void Stop(void);
        // update
        virtual void Update(void);

        // if update is valid for the current state
        virtual bool Is_Update_Valid(void);

        // spin counter
        float m_spin_counter;

        // if spinning
        bool m_spin;

        // Save below given XML node
        virtual xmlpp::Element* Save_To_XML_Node(xmlpp::Element* p_element);

    protected:
        // typename inherited
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace SMC

#endif

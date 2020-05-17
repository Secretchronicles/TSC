/***************************************************************************
 * keyboard.h
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

#ifndef TSC_KEYBOARD_HPP
#define TSC_KEYBOARD_HPP

#include "../core/global_basic.hpp"
#include "../core/global_game.hpp"
#include "../scripting/scriptable_object.hpp"
#include "../scripting/objects/misc/mrb_input.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** *** cKeyboard *** *** *** *** *** *** *** *** *** */

    class cKeyboard: public Scripting::cScriptable_Object {
    public:
        cKeyboard(void);
        ~cKeyboard(void);

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            // Someone needs to hold the event table, hence I decided
            // pKeyboard should do this.
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "InputClass"), &Scripting::rtTSC_Scriptable, this));
        }

        // Check the state of the Shift and Ctrl keys.
        inline bool Is_Shift_Down(){ return sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift); }
        inline bool Is_Ctrl_Down(){ return sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::RControl); }

        /* CEGUI Key Up handler
         * returns true if CEGUI processed the given key up event
        */
        bool CEGUI_Handle_Key_Up(sf::Keyboard::Key key) const;

        /* Key Up Handler
         * returns true if the event was processed
        */
        bool Key_Up(const sf::Event& evt);

        /* CEGUI Key Down handler
         * returns true if CEGUI processed the given key down event
        */
        bool CEGUI_Handle_Key_Down(sf::Keyboard::Key key) const;

        /* Key Down handler
         * returns true if the event was processed
        */
        bool Key_Down(const sf::Event& evt);

        /* CEGUI Text Entry handler
         * returns true if CEGUI processed the given text entry event
        */
        bool CEGUI_Handle_Text_Entered(uint32_t character);

        /* Text Entry handler
         * returns true if the event was processed
        */
        bool Text_Entered(const sf::Event& evt);

        // Translate a SFMLKey to the proper CEGUI::Key
        CEGUI::Key::Scan SFMLKey_to_CEGUIKey(const sf::Keyboard::Key key) const;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

// global Keyboard pointer
    extern cKeyboard* pKeyboard;

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

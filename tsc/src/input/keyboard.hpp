/***************************************************************************
 * keyboard.h
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

        // Reset all keys
        void Reset_Keys(void);

        /* CEGUI Key Up handler
         * returns true if CEGUI processed the given key up event
        */
        bool CEGUI_Handle_Key_Up(SDLKey key) const;

        /* Key Up Handler
         * returns true if the event was processed
        */
        bool Key_Up(SDLKey key);

        /* CEGUI Key Down handler
         * returns true if CEGUI processed the given key down event
        */
        bool CEGUI_Handle_Key_Down(SDLKey key) const;

        /* Key Down handler
         * returns true if the event was processed
        */
        bool Key_Down(SDLKey key);

        // Is the CTRL key pressed
        inline bool Is_Ctrl_Down(void) const
        {
            return m_keys[SDLK_RCTRL] || m_keys[SDLK_LCTRL];
        };
        // Is the SHIFT key pressed
        inline bool Is_Shift_Down(void) const
        {
            return m_keys[SDLK_RSHIFT] || m_keys[SDLK_LSHIFT];
        };
        // Is the ALT key pressed
        inline bool Is_Alt_Down(void) const
        {
            return m_keys[SDLK_RALT] || m_keys[SDLK_LALT];
        };

        // Translate a SDLKey to the proper CEGUI::Key
        unsigned int SDLKey_to_CEGUIKey(const SDLKey key) const;

        // Pressed keys
        Uint8 m_keys[SDLK_LAST];
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

// global Keyboard pointer
    extern cKeyboard* pKeyboard;

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

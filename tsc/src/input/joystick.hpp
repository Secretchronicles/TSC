/***************************************************************************
 * joystick.h
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

#ifndef TSC_JOYSTICK_HPP
#define TSC_JOYSTICK_HPP

#include "../core/global_basic.hpp"

namespace TSC {

    /* *** *** *** *** *** *** cJoystick *** *** *** *** *** *** *** *** *** *** *** */

    class cJoystick {
    public:
        cJoystick(void);
        ~cJoystick(void);

        // Initializes the Joystick system
        int Init(void);
        // Closes the current Joystick
        void Close(void);

        // Opens the specified Joystick
        bool Stick_Open(unsigned int index);
        // Closes the Stick
        void Stick_Close(void);

        // Resets all Buttons and modifiers
        void Reset_keys(void);

        // Handle the Hat
        void Handle_Hat(SDL_Event* ev);
        // Handles the Joystick motion
        void Handle_Motion(SDL_Event* ev);
        // Handle Joystick Button down event
        bool Handle_Button_Down_Event(SDL_Event* ev);
        // Handle Joystick Button up event
        bool Handle_Button_Up_Event(SDL_Event* ev);

        // Returns the current Joystick name
        std::string Get_Name(void) const;
        // Returns all available Joystick names
        vector<std::string> Get_Names(void) const;

        // Sets the given button state
        void Set_Button(Uint8 button, bool pressed);

        // check if the analog direction is pressed
        bool Left(void) const;
        bool Right(void) const;
        bool Up(void) const;
        bool Down(void) const;
        // check if the given button is pushed
        bool Button(Uint8 button);

        // current joystick pointer
        SDL_Joystick* m_joystick;

        // button state array
        typedef vector<bool> ButtonList;
        ButtonList m_buttons;

        // analog directions
        bool m_left;
        bool m_right;
        bool m_up;
        bool m_down;

        // current opened joystick
        int m_current_joystick;
        // if true the current joystick is available/loaded
        bool m_joystick_open;

        // available buttons
        unsigned int m_num_buttons;
        // available axes
        unsigned int m_num_axes;
        // available balls
        unsigned int m_num_balls;

        // if true print debug output
        bool m_debug;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

// global Joystick pointer
    extern cJoystick* pJoystick;

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

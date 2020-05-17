/***************************************************************************
 * joystick.h
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

#ifndef TSC_JOYSTICK_HPP
#define TSC_JOYSTICK_HPP

#include "../core/global_basic.hpp"
#include "../user/preferences.hpp"

namespace TSC {

    /* *** *** *** *** *** *** cJoystick *** *** *** *** *** *** *** *** *** *** *** */

    class cJoystick {
    private:
        /* These booleans represent the currently recorded state for each direction
         * for each axis. */
        bool m_is_axis_left[cPreferences::NUM_JOYSTICK_AXIS_TYPES];
        bool m_is_axis_right[cPreferences::NUM_JOYSTICK_AXIS_TYPES];
        bool m_is_axis_down[cPreferences::NUM_JOYSTICK_AXIS_TYPES];
        bool m_is_axis_up[cPreferences::NUM_JOYSTICK_AXIS_TYPES];

        // analog / directional pad directions
        bool m_left;
        bool m_right;
        bool m_up;
        bool m_down;

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

        // Handles the Joystick motion
        void Handle_Motion(const sf::Event& evt);
        // Handle Joystick Button down event
        bool Handle_Button_Down_Event(const sf::Event& evt);
        // Handle Joystick Button up event
        bool Handle_Button_Up_Event(const sf::Event& evt);

        // Returns the current Joystick name
        std::string Get_Name(void) const;
        // Returns all available Joystick names
        vector<std::string> Get_Names(void) const;

        // Check if analog or directional pad left is pressed
        bool Left(void) const
        {
            return pPreferences->m_joy_enabled && m_left;
        }

        // Check if analog or directional pad right is pressed
        bool Right(void) const
        {
            return pPreferences->m_joy_enabled && m_right;
        }

        // Check if analog or directional pad up is pressed
        bool Up(void) const
        {
            return pPreferences->m_joy_enabled && m_up;
        }

        // Check if analog or directional pad down is pressed
        bool Down(void) const
        {
            return pPreferences->m_joy_enabled && m_down;
        }

        // check if the given button is pushed
        bool Button(unsigned int button);

        // SFML current opened joystick
        unsigned int m_current_joystick;

        // available joysticks
        unsigned int m_num_joysticks;
        // available buttons
        unsigned int m_num_buttons;

        // if true print debug output
        bool m_debug;

        /* If the absolute value of the joystick position is below this number,
         * it will be considered neutral. */
        static const float m_joystick_neutral_bound;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

// global Joystick pointer
    extern cJoystick* pJoystick;

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

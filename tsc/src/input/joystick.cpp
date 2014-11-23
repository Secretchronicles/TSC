/***************************************************************************
 * joystick.cpp  -  Joystick handling class
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

#include "../core/global_basic.hpp"
#include "../input/keyboard.hpp"
#include "../input/joystick.hpp"
#include "../user/preferences.hpp"
#include "../core/game_core.hpp"
#include "../level/level_player.hpp"
#include "../gui/hud.hpp"
#include "../gui/menu.hpp"
#include "../level/level.hpp"
#include "../overworld/overworld.hpp"

namespace TSC {

/* *** *** *** *** *** *** cJoystick *** *** *** *** *** *** *** *** *** *** *** */

cJoystick::cJoystick(void)
{
    m_joystick = NULL;

    m_joystick_open = 0;

    m_current_joystick = 0;
    m_num_buttons = 0;
    m_num_axes = 0;
    m_num_balls = 0;

    m_debug = 0;

    Reset_keys();

    Init();
}

cJoystick::~cJoystick(void)
{
    Close();
}

int cJoystick::Init(void)
{
    // if not enabled
    if (!pPreferences->m_joy_enabled) {
        return 0;
    }

    int joy_count = SDL_NumJoysticks();

    // no joystick available
    if (joy_count <= 0) {
        printf("No joysticks available\n");
        pPreferences->m_joy_enabled = 0;
        return 0;
    }

    if (m_debug) {
        printf("Joysticks found : %d\n\n", joy_count);
    }

    unsigned int default_joy = 0;

    // if default joy name is given
    if (!pPreferences->m_joy_name.empty()) {
        vector<std::string> joy_names = Get_Names();

        for (unsigned int i = 0; i < joy_names.size(); i++) {
            // found default joy
            if (joy_names[i].compare(pPreferences->m_joy_name) == 0) {
                default_joy = i;
                break;
            }
        }
    }

    // setup
    SDL_JoystickEventState(SDL_ENABLE);
    Stick_Open(default_joy);

    if (m_debug) {
        printf("Joypad System Initialized\n");
    }

    return 1;
}

void cJoystick::Close(void)
{
    Stick_Close();
}

bool cJoystick::Stick_Open(unsigned int index)
{
    // if a joystick is already opened close it first
    if (m_joystick_open) {
        Stick_Close();
    }

    m_joystick = SDL_JoystickOpen(index);

    if (!m_joystick) {
        printf("Couldn't open joystick %d\n", index);
        m_joystick_open = 0;
        return 0;
    }

    m_current_joystick = index;

    m_num_buttons = SDL_JoystickNumButtons(m_joystick);
    m_num_axes = SDL_JoystickNumAxes(m_joystick);
    m_num_balls = SDL_JoystickNumBalls(m_joystick);

    // setup available buttons
    m_buttons.assign(m_num_buttons, 0);

    if (m_debug) {
        printf("Opened Joystick %d\n", m_current_joystick);
        printf("Name: %s\n", Get_Name().c_str());
        printf("Number of Buttons: %d\n", m_num_buttons);
        printf("Number of Axes: %d\n", m_num_axes);
        printf("Number of Balls: %d\n\n", m_num_balls);
    }

    m_joystick_open = 1;
    return 1;
}

void cJoystick::Stick_Close(void)
{
    // not available
    if (!m_joystick) {
        return;
    }

    SDL_JoystickClose(m_joystick);
    m_joystick = NULL;

    Reset_keys();

    m_num_buttons = 0;
    m_num_axes = 0;
    m_num_balls = 0;

    m_buttons.clear();
    m_joystick_open = 0;

    if (m_debug) {
        printf("Joystick %d closed\n", m_current_joystick);
    }

    m_current_joystick = 0;
}

void cJoystick::Reset_keys(void)
{
    // clear buttons
    std::fill(m_buttons.begin(), m_buttons.end(), static_cast<bool>(0));

    m_left = 0;
    m_right = 0;
    m_up = 0;
    m_down = 0;
}

void cJoystick::Handle_Hat(SDL_Event* ev)
{
    // up
    if (ev->jhat.value == SDL_HAT_UP) {
        if (!m_up) {
            pKeyboard->Key_Down(pPreferences->m_key_up);
            m_up = 1;
        }

        if (m_down) {
            pKeyboard->Key_Up(pPreferences->m_key_down);
            m_down = 0;
        }
    }
    else {
        if (m_up) {
            pKeyboard->Key_Up(pPreferences->m_key_up);
            m_up = 0;
        }
    }

    // down
    if (ev->jhat.value == SDL_HAT_DOWN) {
        if (!m_down) {
            pKeyboard->Key_Down(pPreferences->m_key_down);
            m_down = 1;
        }

        if (m_up) {
            pKeyboard->Key_Up(pPreferences->m_key_up);
            m_up = 0;
        }
    }
    else {
        if (m_down) {
            pKeyboard->Key_Up(pPreferences->m_key_down);
            m_down = 0;
        }
    }

    // left
    if (ev->jhat.value == SDL_HAT_LEFT) {
        if (!m_left) {
            pKeyboard->Key_Down(pPreferences->m_key_left);
            m_left = 1;
        }

        if (m_right) {
            pKeyboard->Key_Up(pPreferences->m_key_right);
            m_right = 0;
        }
    }
    else {
        if (m_left) {
            pKeyboard->Key_Up(pPreferences->m_key_left);
            m_left = 0;
        }
    }

    // right
    if (ev->jhat.value == SDL_HAT_RIGHT) {
        if (!m_right) {
            pKeyboard->Key_Down(pPreferences->m_key_right);
            m_right = 1;
        }

        if (m_left) {
            pKeyboard->Key_Up(pPreferences->m_key_left);
            m_left = 0;
        }
    }
    else {
        if (m_right) {
            pKeyboard->Key_Up(pPreferences->m_key_right);
            m_right = 0;
        }
    }
}

void cJoystick::Handle_Motion(SDL_Event* ev)
{
    // Vertical Axis
    if (ev->jaxis.axis == pPreferences->m_joy_axis_ver) {
        // Up
        if (ev->jaxis.value < -pPreferences->m_joy_axis_threshold) {
            if (m_debug) {
                printf("Joystick %d : Up Button pressed\n", m_current_joystick);
            }

            if (!m_up) {
                pKeyboard->Key_Down(pPreferences->m_key_up);
                m_up = 1;
            }

            if (m_down) {
                pKeyboard->Key_Up(pPreferences->m_key_down);
                m_down = 0;
            }
        }
        // Down
        else if (ev->jaxis.value > pPreferences->m_joy_axis_threshold) {
            if (m_debug) {
                printf("Joystick %d : Down Button pressed\n", m_current_joystick);
            }

            if (!m_down) {
                pKeyboard->Key_Down(pPreferences->m_key_down);
                m_down = 1;
            }

            if (m_up) {
                pKeyboard->Key_Up(pPreferences->m_key_up);
                m_up = 0;
            }
        }
        // No Down/Left
        else {
            if (m_down) {
                pKeyboard->Key_Up(pPreferences->m_key_down);
                m_down = 0;
            }

            if (m_up) {
                pKeyboard->Key_Up(pPreferences->m_key_up);
                m_up = 0;
            }
        }
    }
    // Horizontal Axis
    else if (ev->jaxis.axis == pPreferences->m_joy_axis_hor) {
        // Left
        if (ev->jaxis.value < -pPreferences->m_joy_axis_threshold) {
            if (m_debug) {
                printf("Joystick %d : Left Button pressed\n", m_current_joystick);
            }

            if (!m_left) {
                pKeyboard->Key_Down(pPreferences->m_key_left);
                m_left = 1;
            }

            if (m_right) {
                pKeyboard->Key_Up(pPreferences->m_key_right);
                m_right = 0;
            }
        }
        // Right
        else if (ev->jaxis.value > pPreferences->m_joy_axis_threshold) {
            if (m_debug) {
                printf("Joystick %d : Right Button pressed\n", m_current_joystick);
            }

            if (!m_right) {
                pKeyboard->Key_Down(pPreferences->m_key_right);
                m_right = 1;
            }

            if (m_left) {
                pKeyboard->Key_Up(pPreferences->m_key_left);
                m_left = 0;
            }
        }
        // No Left/Right
        else {
            if (m_left) {
                pKeyboard->Key_Up(pPreferences->m_key_left);
                m_left = 0;
            }

            if (m_right) {
                pKeyboard->Key_Up(pPreferences->m_key_right);
                m_right = 0;
            }
        }
    }
}

bool cJoystick::Handle_Button_Down_Event(SDL_Event* ev)
{
    // not enabled or opened
    if (!pPreferences->m_joy_enabled || !m_joystick_open) {
        return 0;
    }

    Set_Button(ev->jbutton.button, 1);

    // handle button in the current mode
    if (Game_Mode == MODE_LEVEL) {
        // processed by the level
        if (pActive_Level->Joy_Button_Down(ev->jbutton.button)) {
            return 1;
        }
    }
    else if (Game_Mode == MODE_OVERWORLD) {
        // processed by the overworld
        if (pActive_Overworld->Joy_Button_Down(ev->jbutton.button)) {
            return 1;
        }
    }
    else if (Game_Mode == MODE_MENU) {
        // processed by the menu
        if (pMenuCore->Joy_Button_Down(ev->jbutton.button)) {
            return 1;
        }
    }

    if (ev->jbutton.button < m_buttons.size()) {
        // Jump
        if (ev->jbutton.button == pPreferences->m_joy_button_jump) {
            //
        }
        // Shoot
        else if (ev->jbutton.button == pPreferences->m_joy_button_shoot) {
            pKeyboard->Key_Down(pPreferences->m_key_shoot);
            return 1;
        }
        // Request Itembox Item
        else if (ev->jbutton.button == pPreferences->m_joy_button_item) {
            // not handled
            return 1;
        }
        // Interaction
        else if (ev->jbutton.button == pPreferences->m_joy_button_action) {
            pKeyboard->Key_Down(pPreferences->m_key_action);
            return 1;
        }
        // Exit
        else if (ev->jbutton.button == pPreferences->m_joy_button_exit) {
            pKeyboard->Key_Down(SDLK_ESCAPE);
            return 1;
        }
        // Pause
        else if (ev->jbutton.button == 9) {
            pKeyboard->Key_Down(SDLK_PAUSE);
            return 1;
        }
    }

    return 0;
}

bool cJoystick::Handle_Button_Up_Event(SDL_Event* ev)
{
    // not enabled or opened
    if (!pPreferences->m_joy_enabled || !m_joystick_open) {
        return 0;
    }

    Set_Button(ev->jbutton.button, 0);

    // handle button in the current mode
    if (Game_Mode == MODE_LEVEL) {
        // processed by the level
        if (pActive_Level->Joy_Button_Up(ev->jbutton.button)) {
            return 1;
        }
    }
    else if (Game_Mode == MODE_OVERWORLD) {
        // processed by the overworld
        if (pActive_Overworld->Joy_Button_Up(ev->jbutton.button)) {
            return 1;
        }
    }
    else if (Game_Mode == MODE_MENU) {
        // processed by the menu
        if (pMenuCore->Joy_Button_Up(ev->jbutton.button)) {
            return 1;
        }
    }

    if (ev->jbutton.button < m_buttons.size()) {
        if (ev->jbutton.button == pPreferences->m_joy_button_jump) {
            pKeyboard->Key_Up(pPreferences->m_key_jump);
            return 1;
        }
        else if (ev->jbutton.button == pPreferences->m_joy_button_shoot) {
            pKeyboard->Key_Up(pPreferences->m_key_shoot);
            return 1;
        }
        else if (ev->jbutton.button == pPreferences->m_joy_button_item) {
            // not handled
        }
        else if (ev->jbutton.button == pPreferences->m_joy_button_action) {
            pKeyboard->Key_Up(pPreferences->m_key_action);
            return 1;
        }
        else if (ev->jbutton.button == pPreferences->m_joy_button_exit) {
            pKeyboard->Key_Up(SDLK_ESCAPE);
            return 1;
        }
    }

    return 0;
}

std::string cJoystick::Get_Name(void) const
{
    return SDL_JoystickName(m_current_joystick);
}

vector<std::string> cJoystick::Get_Names(void) const
{
    vector<std::string> names;
    // get joy count
    int joy_count = SDL_NumJoysticks();

    // joystick names
    for (int i = 0; i < joy_count; i++) {
        names.push_back(SDL_JoystickName(i));
    }

    return names;
}

void cJoystick::Set_Button(Uint8 num, bool pressed)
{
    // not available
    if (num >= m_buttons.size()) {
        return;
    }

    if (m_debug) {
        if (pressed) {
            printf("Joystick %d : Joy Button %d pressed\n", m_current_joystick, num);
        }
        else {
            printf("Joystick %d : Joy Button %d released\n", m_current_joystick, num);
        }
    }

    m_buttons[num] = pressed;
}

bool cJoystick::Left(void) const
{
    if (pPreferences->m_joy_enabled && input_event.type == SDL_JOYAXISMOTION && input_event.jaxis.value < -pPreferences->m_joy_axis_threshold &&
            input_event.jaxis.axis == pPreferences->m_joy_axis_hor) {
        return 1;
    }

    return 0;
}

bool cJoystick::Right(void) const
{
    if (pPreferences->m_joy_enabled && input_event.type == SDL_JOYAXISMOTION && input_event.jaxis.value > pPreferences->m_joy_axis_threshold &&
            input_event.jaxis.axis == pPreferences->m_joy_axis_hor) {
        return 1;
    }

    return 0;
}

bool cJoystick::Up(void) const
{
    if (pPreferences->m_joy_enabled && input_event.type == SDL_JOYAXISMOTION && input_event.jaxis.value < -pPreferences->m_joy_axis_threshold &&
            input_event.jaxis.axis == pPreferences->m_joy_axis_ver) {
        return 1;
    }

    return 0;
}

bool cJoystick::Down(void) const
{
    if (pPreferences->m_joy_enabled && input_event.type == SDL_JOYAXISMOTION && input_event.jaxis.value > pPreferences->m_joy_axis_threshold &&
            input_event.jaxis.axis == pPreferences->m_joy_axis_ver) {
        return 1;
    }

    return 0;
}

bool cJoystick::Button(Uint8 num)
{
    // if available and pressed
    if (num < m_buttons.size() && m_buttons[num]) {
        return 1;
    }

    return 0;
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

cJoystick* pJoystick = NULL;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

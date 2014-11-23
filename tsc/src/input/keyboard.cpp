/***************************************************************************
 * keyboard.cpp  -  keyboard handling class
 *
 * Copyright © 2006 - 2011 Florian Richter
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

#include "../core/game_core.hpp"
#include "../gui/generic.hpp"
#include "../input/keyboard.hpp"
#include "../input/mouse.hpp"
#include "../input/joystick.hpp"
#include "../level/level_player.hpp"
#include "../gui/menu.hpp"
#include "../overworld/overworld.hpp"
#include "../core/framerate.hpp"
#include "../audio/audio.hpp"
#include "../level/level.hpp"
#include "../user/preferences.hpp"
#include "../level/level_editor.hpp"
#include "../overworld/world_editor.hpp"

namespace TSC {

/* *** *** *** *** *** *** *** *** cKeyboard *** *** *** *** *** *** *** *** *** */

cKeyboard::cKeyboard(void)
{
    Reset_Keys();
}

cKeyboard::~cKeyboard(void)
{

}

void cKeyboard::Reset_Keys(void)
{
    // set all keys to 0
    memset(m_keys, 0, sizeof(m_keys));
}

bool cKeyboard::CEGUI_Handle_Key_Up(SDLKey key) const
{
    // inject the scancode directly
    if (pGuiSystem->injectKeyUp(SDLKey_to_CEGUIKey(key))) {
        // input was processed by the gui system
        return 1;
    }

    return 0;
}

bool cKeyboard::Key_Up(SDLKey key)
{
    // set key to 0
    m_keys[key] = 0;

    // input was processed by the gui system
    if (CEGUI_Handle_Key_Up(key)) {
        return 1;
    }

    // handle key in the current mode
    if (Game_Mode == MODE_LEVEL) {
        // got processed
        if (pActive_Level->Key_Up(key)) {
            return 1;
        }
    }
    else if (Game_Mode == MODE_MENU) {
        // got processed
        if (pMenuCore->Key_Up(key)) {
            return 1;
        }
    }

    return 0;
}

bool cKeyboard::CEGUI_Handle_Key_Down(SDLKey key) const
{
    // inject the scancode
    if (pGuiSystem->injectKeyDown(SDLKey_to_CEGUIKey(key)) == 1) {
        // input got processed by the gui system
        return 1;
    }

    // use for translated unicode value
    if (input_event.key.keysym.unicode != 0) {
        if (pGuiSystem->injectChar(input_event.key.keysym.unicode)) {
            // input got processed by the gui system
            return 1;
        }
    }

    return 0;
}

bool cKeyboard::Key_Down(SDLKey key)
{
    // input was processed by the gui system
    if (CEGUI_Handle_Key_Down(key)) {
        return 1;
    }

    // set key to 1
    m_keys[key] = 1;

    // ## first the internal keys

    // game exit
    if (key == SDLK_F4 && pKeyboard->Is_Alt_Down()) {
        game_exit = 1;
        return 1;
    }
    // fullscreen toggle
    else if (key == SDLK_RETURN && pKeyboard->Is_Alt_Down()) {
        pVideo->Toggle_Fullscreen();
        return 1;
    }
    // GUI copy
    else if (key == SDLK_c && pKeyboard->Is_Ctrl_Down()) {
        if (GUI_Copy_To_Clipboard()) {
            return 1;
        }
    }
    // GUI cut
    else if (key == SDLK_x && pKeyboard->Is_Ctrl_Down()) {
        if (GUI_Copy_To_Clipboard(1)) {
            return 1;
        }
    }
    // GUI paste
    else if (key == SDLK_v && pKeyboard->Is_Ctrl_Down()) {
        if (GUI_Paste_From_Clipboard()) {
            return 1;
        }
    }

    // handle key in the current mode
    if (Game_Mode == MODE_LEVEL) {
        // processed by the level
        if (pActive_Level->Key_Down(key)) {
            return 1;
        }
    }
    else if (Game_Mode == MODE_OVERWORLD) {
        // processed by the overworld
        if (pActive_Overworld->Key_Down(key)) {
            return 1;
        }
    }
    else if (Game_Mode == MODE_MENU) {
        // processed by the menu
        if (pMenuCore->Key_Down(key)) {
            return 1;
        }
    }
    else if (Game_Mode == MODE_LEVEL_SETTINGS) {
        // processed by the level settings
        if (pLevel_Editor->m_settings_screen->Key_Down(key)) {
            return 1;
        }
    }

    // set fixed speed factor mode
    if (key == SDLK_F6) {
        float fixed_speedfactor = string_to_float(Box_Text_Input(float_to_string(pFramerate->m_force_speed_factor, 2), "Set Fixed Speedfactor", 1));

        // disable
        if (Is_Float_Equal(fixed_speedfactor, 0.0f)) {
            pFramerate->Set_Fixed_Speedfacor(0.0f);
            pHud_Debug->Set_Text("Fixed speed factor disabled");
        }
        // below minimum
        else if (fixed_speedfactor <= 0.04f) {
            pHud_Debug->Set_Text("Fixed speed factor must be greater than 0.04");
        }
        // enable
        else {
            pFramerate->Set_Fixed_Speedfacor(fixed_speedfactor);
            pHud_Debug->Set_Text("Fixed speed factor enabled");
        }
    }
    // take a screenshot
    else if (key == pPreferences->m_key_screenshot) {
        pVideo->Save_Screenshot();
    }
    // pause the game
    else if (key == SDLK_PAUSE) {
        Draw_Static_Text("Pause", &yellow, &lightgreyalpha64);
    }
    // load a level
    else if (key == SDLK_l && pKeyboard->Is_Ctrl_Down() && !(Game_Mode == MODE_OVERWORLD && pOverworld_Manager->m_debug_mode) && Game_Mode != MODE_LEVEL_SETTINGS) {
        pLevel_Editor->Function_Load();
    }
    // load an overworld
    else if (key == SDLK_w && pKeyboard->Is_Ctrl_Down() && !(Game_Mode == MODE_OVERWORLD && pOverworld_Manager->m_debug_mode) && Game_Mode != MODE_LEVEL_SETTINGS) {
        pWorld_Editor->Function_Load();
    }
    // sound toggle
    else if (key == SDLK_F10) {
        pAudio->Toggle_Sounds();

        if (!pAudio->m_sound_enabled) {
            pHud_Debug->Set_Text("Sound disabled");
        }
        else {
            pHud_Debug->Set_Text("Sound enabled");
        }
    }
    // music toggle
    else if (key == SDLK_F11) {
        pAudio->Toggle_Music();

        if (!pAudio->m_music_enabled) {
            pHud_Debug->Set_Text("Music disabled");
        }
        else {
            pHud_Debug->Set_Text("Music enabled");
        }
    }
    // debug mode
    else if (key == SDLK_d && pKeyboard->Is_Ctrl_Down()) {
        if (game_debug) {
            pHud_Debug->Set_Text("Debug mode disabled");
        }
        else {
            pFramerate->m_fps_worst = 100000;
            pFramerate->m_fps_best = 0;
            pHud_Debug->Set_Text("Debug mode enabled");
        }

        game_debug = !game_debug;
    }
    // performance mode
    else if (key == SDLK_p && pKeyboard->Is_Ctrl_Down()) {
        if (game_debug_performance) {
            pHud_Debug->Set_Text("Performance debug mode disabled");
        }
        else {
            pFramerate->m_fps_worst = 100000;
            pFramerate->m_fps_best = 0;
            pHud_Debug->Set_Text("Performance debug mode enabled");
        }

        game_debug_performance = !game_debug_performance;
    }

    return 0;
}

unsigned int cKeyboard::SDLKey_to_CEGUIKey(const SDLKey key) const
{
    switch (key) {
    case SDLK_BACKSPACE:
        return CEGUI::Key::Backspace;
    case SDLK_TAB:
        return CEGUI::Key::Tab;
    case SDLK_RETURN:
        return CEGUI::Key::Return;
    case SDLK_PAUSE:
        return CEGUI::Key::Pause;
    case SDLK_ESCAPE:
        return CEGUI::Key::Escape;
    case SDLK_SPACE:
        return CEGUI::Key::Space;
    case SDLK_COMMA:
        return CEGUI::Key::Comma;
    case SDLK_MINUS:
        return CEGUI::Key::Minus;
    case SDLK_PERIOD:
        return CEGUI::Key::Period;
    case SDLK_SLASH:
        return CEGUI::Key::Slash;
    case SDLK_0:
        return CEGUI::Key::Zero;
    case SDLK_1:
        return CEGUI::Key::One;
    case SDLK_2:
        return CEGUI::Key::Two;
    case SDLK_3:
        return CEGUI::Key::Three;
    case SDLK_4:
        return CEGUI::Key::Four;
    case SDLK_5:
        return CEGUI::Key::Five;
    case SDLK_6:
        return CEGUI::Key::Six;
    case SDLK_7:
        return CEGUI::Key::Seven;
    case SDLK_8:
        return CEGUI::Key::Eight;
    case SDLK_9:
        return CEGUI::Key::Nine;
    case SDLK_COLON:
        return CEGUI::Key::Colon;
    case SDLK_SEMICOLON:
        return CEGUI::Key::Semicolon;
    case SDLK_EQUALS:
        return CEGUI::Key::Equals;
    case SDLK_LEFTBRACKET:
        return CEGUI::Key::LeftBracket;
    case SDLK_BACKSLASH:
        return CEGUI::Key::Backslash;
    case SDLK_RIGHTBRACKET:
        return CEGUI::Key::RightBracket;
    case SDLK_a:
        return CEGUI::Key::A;
    case SDLK_b:
        return CEGUI::Key::B;
    case SDLK_c:
        return CEGUI::Key::C;
    case SDLK_d:
        return CEGUI::Key::D;
    case SDLK_e:
        return CEGUI::Key::E;
    case SDLK_f:
        return CEGUI::Key::F;
    case SDLK_g:
        return CEGUI::Key::G;
    case SDLK_h:
        return CEGUI::Key::H;
    case SDLK_i:
        return CEGUI::Key::I;
    case SDLK_j:
        return CEGUI::Key::J;
    case SDLK_k:
        return CEGUI::Key::K;
    case SDLK_l:
        return CEGUI::Key::L;
    case SDLK_m:
        return CEGUI::Key::M;
    case SDLK_n:
        return CEGUI::Key::N;
    case SDLK_o:
        return CEGUI::Key::O;
    case SDLK_p:
        return CEGUI::Key::P;
    case SDLK_q:
        return CEGUI::Key::Q;
    case SDLK_r:
        return CEGUI::Key::R;
    case SDLK_s:
        return CEGUI::Key::S;
    case SDLK_t:
        return CEGUI::Key::T;
    case SDLK_u:
        return CEGUI::Key::U;
    case SDLK_v:
        return CEGUI::Key::V;
    case SDLK_w:
        return CEGUI::Key::W;
    case SDLK_x:
        return CEGUI::Key::X;
    case SDLK_y:
        return CEGUI::Key::Y;
    case SDLK_z:
        return CEGUI::Key::Z;
    case SDLK_DELETE:
        return CEGUI::Key::Delete;
    case SDLK_KP0:
        return CEGUI::Key::Numpad0;
    case SDLK_KP1:
        return CEGUI::Key::Numpad1;
    case SDLK_KP2:
        return CEGUI::Key::Numpad2;
    case SDLK_KP3:
        return CEGUI::Key::Numpad3;
    case SDLK_KP4:
        return CEGUI::Key::Numpad4;
    case SDLK_KP5:
        return CEGUI::Key::Numpad5;
    case SDLK_KP6:
        return CEGUI::Key::Numpad6;
    case SDLK_KP7:
        return CEGUI::Key::Numpad7;
    case SDLK_KP8:
        return CEGUI::Key::Numpad8;
    case SDLK_KP9:
        return CEGUI::Key::Numpad9;
    case SDLK_KP_PERIOD:
        return CEGUI::Key::Decimal;
    case SDLK_KP_DIVIDE:
        return CEGUI::Key::Divide;
    case SDLK_KP_MULTIPLY:
        return CEGUI::Key::Multiply;
    case SDLK_KP_MINUS:
        return CEGUI::Key::Subtract;
    case SDLK_KP_PLUS:
        return CEGUI::Key::Add;
    case SDLK_KP_ENTER:
        return CEGUI::Key::NumpadEnter;
    case SDLK_KP_EQUALS:
        return CEGUI::Key::NumpadEquals;
    case SDLK_UP:
        return CEGUI::Key::ArrowUp;
    case SDLK_DOWN:
        return CEGUI::Key::ArrowDown;
    case SDLK_RIGHT:
        return CEGUI::Key::ArrowRight;
    case SDLK_LEFT:
        return CEGUI::Key::ArrowLeft;
    case SDLK_INSERT:
        return CEGUI::Key::Insert;
    case SDLK_HOME:
        return CEGUI::Key::Home;
    case SDLK_END:
        return CEGUI::Key::End;
    case SDLK_PAGEUP:
        return CEGUI::Key::PageUp;
    case SDLK_PAGEDOWN:
        return CEGUI::Key::PageDown;
    case SDLK_F1:
        return CEGUI::Key::F1;
    case SDLK_F2:
        return CEGUI::Key::F2;
    case SDLK_F3:
        return CEGUI::Key::F3;
    case SDLK_F4:
        return CEGUI::Key::F4;
    case SDLK_F5:
        return CEGUI::Key::F5;
    case SDLK_F6:
        return CEGUI::Key::F6;
    case SDLK_F7:
        return CEGUI::Key::F7;
    case SDLK_F8:
        return CEGUI::Key::F8;
    case SDLK_F9:
        return CEGUI::Key::F9;
    case SDLK_F10:
        return CEGUI::Key::F10;
    case SDLK_F11:
        return CEGUI::Key::F11;
    case SDLK_F12:
        return CEGUI::Key::F12;
    case SDLK_F13:
        return CEGUI::Key::F13;
    case SDLK_F14:
        return CEGUI::Key::F14;
    case SDLK_F15:
        return CEGUI::Key::F15;
    case SDLK_NUMLOCK:
        return CEGUI::Key::NumLock;
    case SDLK_SCROLLOCK:
        return CEGUI::Key::ScrollLock;
    case SDLK_RSHIFT:
        return CEGUI::Key::RightShift;
    case SDLK_LSHIFT:
        return CEGUI::Key::LeftShift;
    case SDLK_RCTRL:
        return CEGUI::Key::RightControl;
    case SDLK_LCTRL:
        return CEGUI::Key::LeftControl;
    case SDLK_RALT:
        return CEGUI::Key::RightAlt;
    case SDLK_LALT:
        return CEGUI::Key::LeftAlt;
    case SDLK_LSUPER:
        return CEGUI::Key::LeftWindows;
    case SDLK_RSUPER:
        return CEGUI::Key::RightWindows;
    case SDLK_SYSREQ:
        return CEGUI::Key::SysRq;
    case SDLK_MENU:
        return CEGUI::Key::AppMenu;
    case SDLK_POWER:
        return CEGUI::Key::Power;
    default:
        return 0;
    }
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

cKeyboard* pKeyboard = NULL;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

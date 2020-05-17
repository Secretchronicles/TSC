/***************************************************************************
 * keyboard.cpp  -  keyboard handling class
 *
 * Copyright © 2006 - 2011 Florian Richter
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

#include "../core/game_core.hpp"
#include "../gui/generic.hpp"
#include "../input/keyboard.hpp"
#include "../input/mouse.hpp"
#include "../input/joystick.hpp"
#include "../level/level_player.hpp"
#include "../scene/scene.hpp"
#include "../gui/menu.hpp"
#include "../overworld/overworld.hpp"
#include "../core/framerate.hpp"
#include "../audio/audio.hpp"
#include "../level/level.hpp"
#include "../user/preferences.hpp"
#include "../level/level_settings.hpp"
#include "../level/level_editor.hpp"
#include "../core/i18n.hpp"
#include "../overworld/world_editor.hpp"
#include "../gui/game_console.hpp"
#include "../gui/debug_window.hpp"

namespace TSC {

/* *** *** *** *** *** *** *** *** cKeyboard *** *** *** *** *** *** *** *** *** */

cKeyboard::cKeyboard(void)
{

}

cKeyboard::~cKeyboard(void)
{

}

bool cKeyboard::CEGUI_Handle_Key_Up(sf::Keyboard::Key key) const
{
    // inject the scancode directly
    if (CEGUI::System::getSingleton().getDefaultGUIContext().injectKeyUp(SFMLKey_to_CEGUIKey(key))) {
        // input was processed by the gui system
        return 1;
    }

    return 0;
}

bool cKeyboard::Key_Up(const sf::Event& evt)
{
    // input was processed by the gui system
    if (CEGUI_Handle_Key_Up(evt.key.code)) {
        return 1;
    }

    // handle key in the current mode
    if (Game_Mode == MODE_LEVEL) {
        // got processed
        if (pActive_Level->Key_Up(evt)) {
            return 1;
        }
    }
    else if (Game_Mode == MODE_MENU) {
        // got processed
        if (pMenuCore->Key_Up(evt)) {
            return 1;
        }
    }
    else if (Game_Mode == MODE_SCENE) {
        // got processed
        if (pActive_Scene->Key_Up(evt)) {
            return 1;
        }
    }

    return 0;
}

bool cKeyboard::CEGUI_Handle_Key_Down(sf::Keyboard::Key key) const
{
    // inject the scancode
    if (CEGUI::System::getSingleton().getDefaultGUIContext().injectKeyDown(SFMLKey_to_CEGUIKey(key))) {
        // input got processed by the gui system
        return true;
    }

    return false;
}

bool cKeyboard::Key_Down(const sf::Event& evt)
{
    // input was processed by the gui system
    if (CEGUI_Handle_Key_Down(evt.key.code)) {
        return 1;
    }

    /* Do not forward keyboard input if the game console is open
     * so that user input does not accidentally cause gameplay
     * (e.g., jumping). It's not clear why CEGUI_Handle_Key_Down()
     * does not return true if the console input window accepts the
     * keyboard input, which makes this filter necessary (otherwise
     * program flow would never get here).
     *
     * [F7] and [ESC] keys are let through so the user can close the
     * game console again. */
    if (gp_game_console->IsVisible() &&
        evt.key.code != sf::Keyboard::F7 &&
        evt.key.code != sf::Keyboard::Escape) {
        return true;
    }

    // Do not take screenshots and such things while the editor
    // config panel is open and the user may be typing into the widgets
    // (typing "p" would take a screenshot).
    if ((Game_Mode == MODE_LEVEL && pLevel_Editor->m_object_config_pane_shown) ||
        (Game_Mode == MODE_OVERWORLD && pWorld_Editor->m_object_config_pane_shown)) {
        return true;
    }

    // ## first the internal keys

    // game exit
    if (evt.key.code == sf::Keyboard::F4 && evt.key.alt) {
        game_exit = 1;
        return 1;
    }
    // fullscreen toggle
    else if (evt.key.code == sf::Keyboard::Return && evt.key.alt) {
        pVideo->Toggle_Fullscreen();
        return 1;
    }
    // GUI copy
    else if (evt.key.code == sf::Keyboard::C && evt.key.control) {
        if (GUI_Copy_To_Clipboard()) {
            return 1;
        }
    }
    // GUI cut
    else if (evt.key.code == sf::Keyboard::X && evt.key.control) {
        if (GUI_Copy_To_Clipboard(1)) {
            return 1;
        }
    }
    // GUI paste
    else if (evt.key.code == sf::Keyboard::V && evt.key.control) {
        if (GUI_Paste_From_Clipboard()) {
            return 1;
        }
    }

    // ## then handle key in the current mode
    if (Game_Mode == MODE_LEVEL) {
        // processed by the level
        if (pActive_Level->Key_Down(evt)) {
            return 1;
        }
    }
    else if (Game_Mode == MODE_OVERWORLD) {
        // processed by the overworld
        if (pActive_Overworld->Key_Down(evt)) {
            return 1;
        }
    }
    else if (Game_Mode == MODE_MENU) {
        // processed by the menu
        if (pMenuCore->Key_Down(evt)) {
            return 1;
        }
    }
    else if (Game_Mode == MODE_LEVEL_SETTINGS) {
        // processed by the level settings
        if (pLevel_Editor->m_settings_screen.Key_Down(evt)) {
            return 1;
        }
    }
    else if (Game_Mode == MODE_SCENE) {
        // processed by the scene
        if (pActive_Scene->Key_Down(evt)) {
            return 1;
        }
    }

    // set fixed speed factor mode
    if (evt.key.code == sf::Keyboard::F6) {
        float fixed_speedfactor = string_to_float(Box_Text_Input(float_to_string(pFramerate->m_force_speed_factor, 2), "Set Fixed Speedfactor", 1));

        // disable
        if (Is_Float_Equal(fixed_speedfactor, 0.0f)) {
            pFramerate->Set_Fixed_Speedfacor(0.0f);
            gp_hud->Set_Text("Fixed speed factor disabled");
        }
        // below minimum
        else if (fixed_speedfactor <= 0.04f) {
            gp_hud->Set_Text("Fixed speed factor must be greater than 0.04");
        }
        // enable
        else {
            pFramerate->Set_Fixed_Speedfacor(fixed_speedfactor);
            gp_hud->Set_Text("Fixed speed factor enabled");
        }
    }
    // take a screenshot
    else if (evt.key.code == pPreferences->m_key_screenshot) {
        pVideo->Save_Screenshot();
    }
    // pause the game
    else if (evt.key.code == sf::Keyboard::Pause) {
        Draw_Static_Text("Pause", &yellow, &lightgreyalpha64);
    }
    // load a level
    else if (evt.key.code == sf::Keyboard::L && evt.key.control && !(Game_Mode == MODE_OVERWORLD && pOverworld_Manager->m_debug_mode) && Game_Mode != MODE_LEVEL_SETTINGS) {
        pLevel_Editor->Function_Load();
    }
    // load an overworld
    else if (evt.key.code == sf::Keyboard::W && evt.key.control && !(Game_Mode == MODE_OVERWORLD && pOverworld_Manager->m_debug_mode) && Game_Mode != MODE_LEVEL_SETTINGS) {
        pWorld_Editor->Function_Load();
    }
    // sound toggle
    else if (evt.key.code == sf::Keyboard::F10) {
        pAudio->Toggle_Sounds();

        if (!pAudio->m_sound_enabled) {
            gp_hud->Set_Text("Sound disabled");
        }
        else {
            gp_hud->Set_Text("Sound enabled");
        }
    }
    // music toggle
    else if (evt.key.code == sf::Keyboard::F11) {
        pAudio->Toggle_Music();

        if (!pAudio->m_music_enabled) {
            gp_hud->Set_Text("Music disabled");
        }
        else {
            gp_hud->Set_Text("Music enabled");
        }
    }
    // debug mode
    else if (evt.key.code == sf::Keyboard::D && evt.key.control) {
        if (game_debug) {
            gp_hud->Set_Text(_("Debug mode disabled"));
            gp_debug_window->Hide();
        }
        else {
            pFramerate->m_fps_worst = 100000;
            pFramerate->m_fps_best = 0;
            gp_hud->Set_Text(_("Debug mode enabled"));
            gp_debug_window->Show();
        }

        game_debug = !game_debug;
    }
    // performance mode
    else if (evt.key.code == sf::Keyboard::P && evt.key.control) {
        if (game_debug_performance) {
            gp_hud->Set_Text("Performance debug mode disabled");
        }
        else {
            pFramerate->m_fps_worst = 100000;
            pFramerate->m_fps_best = 0;
            gp_hud->Set_Text("Performance debug mode enabled");
        }

        game_debug_performance = !game_debug_performance;
    }

    return 0;
}

bool cKeyboard::CEGUI_Handle_Text_Entered(uint32_t character)
{
    if (CEGUI::System::getSingleton().getDefaultGUIContext().injectChar(character)) {
        // input got processed by the gui system
        return 1;
    }

    return 0;
}

bool cKeyboard::Text_Entered(const sf::Event& evt)
{
    if (CEGUI_Handle_Text_Entered(evt.text.unicode)) {
        // input got processed by the gui system
        return 1;
    }

    return 0;
}

CEGUI::Key::Scan cKeyboard::SFMLKey_to_CEGUIKey(const sf::Keyboard::Key key) const
{
    switch (key) {
    case sf::Keyboard::BackSpace:
        return CEGUI::Key::Backspace;
    case sf::Keyboard::Tab:
        return CEGUI::Key::Tab;
    case sf::Keyboard::Return:
        return CEGUI::Key::Return;
    case sf::Keyboard::Pause:
        return CEGUI::Key::Pause;
    case sf::Keyboard::Escape:
        return CEGUI::Key::Escape;
    case sf::Keyboard::Space:
        return CEGUI::Key::Space;
    case sf::Keyboard::Comma:
        return CEGUI::Key::Comma;
    case sf::Keyboard::Period:
        return CEGUI::Key::Period;
    case sf::Keyboard::Slash:
        return CEGUI::Key::Slash;
    case sf::Keyboard::Num0:
        return CEGUI::Key::Zero;
    case sf::Keyboard::Num1:
        return CEGUI::Key::One;
    case sf::Keyboard::Num2:
        return CEGUI::Key::Two;
    case sf::Keyboard::Num3:
        return CEGUI::Key::Three;
    case sf::Keyboard::Num4:
        return CEGUI::Key::Four;
    case sf::Keyboard::Num5:
        return CEGUI::Key::Five;
    case sf::Keyboard::Num6:
        return CEGUI::Key::Six;
    case sf::Keyboard::Num7:
        return CEGUI::Key::Seven;
    case sf::Keyboard::Num8:
        return CEGUI::Key::Eight;
    case sf::Keyboard::Num9:
        return CEGUI::Key::Nine;
        //case sf::Keyboard::Colon: // no Colon in SFML?
        //return CEGUI::Key::Colon;
    case sf::Keyboard::SemiColon:
        return CEGUI::Key::Semicolon;
    case sf::Keyboard::LBracket:
        return CEGUI::Key::LeftBracket;
    case sf::Keyboard::RBracket:
        return CEGUI::Key::RightBracket;
    case sf::Keyboard::A:
        return CEGUI::Key::A;
    case sf::Keyboard::B:
        return CEGUI::Key::B;
    case sf::Keyboard::C:
        return CEGUI::Key::C;
    case sf::Keyboard::D:
        return CEGUI::Key::D;
    case sf::Keyboard::E:
        return CEGUI::Key::E;
    case sf::Keyboard::F:
        return CEGUI::Key::F;
    case sf::Keyboard::G:
        return CEGUI::Key::G;
    case sf::Keyboard::H:
        return CEGUI::Key::H;
    case sf::Keyboard::I:
        return CEGUI::Key::I;
    case sf::Keyboard::J:
        return CEGUI::Key::J;
    case sf::Keyboard::K:
        return CEGUI::Key::K;
    case sf::Keyboard::L:
        return CEGUI::Key::L;
    case sf::Keyboard::M:
        return CEGUI::Key::M;
    case sf::Keyboard::N:
        return CEGUI::Key::N;
    case sf::Keyboard::O:
        return CEGUI::Key::O;
    case sf::Keyboard::P:
        return CEGUI::Key::P;
    case sf::Keyboard::Q:
        return CEGUI::Key::Q;
    case sf::Keyboard::R:
        return CEGUI::Key::R;
    case sf::Keyboard::S:
        return CEGUI::Key::S;
    case sf::Keyboard::T:
        return CEGUI::Key::T;
    case sf::Keyboard::U:
        return CEGUI::Key::U;
    case sf::Keyboard::V:
        return CEGUI::Key::V;
    case sf::Keyboard::W:
        return CEGUI::Key::W;
    case sf::Keyboard::X:
        return CEGUI::Key::X;
    case sf::Keyboard::Y:
        return CEGUI::Key::Y;
    case sf::Keyboard::Z:
        return CEGUI::Key::Z;
    case sf::Keyboard::Delete:
        return CEGUI::Key::Delete;
    case sf::Keyboard::Numpad0:
        return CEGUI::Key::Numpad0;
    case sf::Keyboard::Numpad1:
        return CEGUI::Key::Numpad1;
    case sf::Keyboard::Numpad2:
        return CEGUI::Key::Numpad2;
    case sf::Keyboard::Numpad3:
        return CEGUI::Key::Numpad3;
    case sf::Keyboard::Numpad4:
        return CEGUI::Key::Numpad4;
    case sf::Keyboard::Numpad5:
        return CEGUI::Key::Numpad5;
    case sf::Keyboard::Numpad6:
        return CEGUI::Key::Numpad6;
    case sf::Keyboard::Numpad7:
        return CEGUI::Key::Numpad7;
    case sf::Keyboard::Numpad8:
        return CEGUI::Key::Numpad8;
    case sf::Keyboard::Numpad9:
        return CEGUI::Key::Numpad9;
    case sf::Keyboard::Divide:
        return CEGUI::Key::Divide;
    case sf::Keyboard::Multiply:
        return CEGUI::Key::Multiply;
    case sf::Keyboard::Subtract:
        return CEGUI::Key::Subtract;
    case sf::Keyboard::Add:
        return CEGUI::Key::Add;
    case sf::Keyboard::Up:
        return CEGUI::Key::ArrowUp;
    case sf::Keyboard::Right:
        return CEGUI::Key::ArrowRight;
    case sf::Keyboard::Left:
        return CEGUI::Key::ArrowLeft;
    case sf::Keyboard::Down:
        return CEGUI::Key::ArrowDown;
    case sf::Keyboard::Insert:
        return CEGUI::Key::Insert;
    case sf::Keyboard::Home:
        return CEGUI::Key::Home;
    case sf::Keyboard::End:
        return CEGUI::Key::End;
    case sf::Keyboard::PageUp:
        return CEGUI::Key::PageUp;
    case sf::Keyboard::PageDown:
        return CEGUI::Key::PageDown;
    case sf::Keyboard::F1:
        return CEGUI::Key::F1;
    case sf::Keyboard::F2:
        return CEGUI::Key::F2;
    case sf::Keyboard::F3:
        return CEGUI::Key::F3;
    case sf::Keyboard::F4:
        return CEGUI::Key::F4;
    case sf::Keyboard::F5:
        return CEGUI::Key::F5;
    case sf::Keyboard::F6:
        return CEGUI::Key::F6;
    case sf::Keyboard::F7:
        return CEGUI::Key::F7;
    case sf::Keyboard::F8:
        return CEGUI::Key::F8;
    case sf::Keyboard::F9:
        return CEGUI::Key::F9;
    case sf::Keyboard::F10:
        return CEGUI::Key::F10;
    case sf::Keyboard::F11:
        return CEGUI::Key::F11;
    case sf::Keyboard::F12:
        return CEGUI::Key::F12;
    case sf::Keyboard::F13:
        return CEGUI::Key::F13;
    case sf::Keyboard::F14:
        return CEGUI::Key::F14;
    case sf::Keyboard::F15:
        return CEGUI::Key::F15;
    default:
        //std::cerr << "Warning: Unknown key received, treating as CEGUI::Key::Unknown." << std::endl;
        return CEGUI::Key::Unknown;
    }
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

cKeyboard* pKeyboard = NULL;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

/***************************************************************************
 * debug_window.hpp
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

#ifndef TSC_DEBUG_WINDOW_HPP
#define TSC_DEBUG_WINDOW_HPP

namespace TSC {

    class cDebug_Window
    {
    public:
        cDebug_Window(cSprite_Manager* p_sprite_manager);
        ~cDebug_Window();
        void Show();
        void Hide();
        void Toggle();
        bool IsVisible();

        void Set_Sprite_Manager(cSprite_Manager* p_sprite_manager);
        void Update();
    private:
        cSprite_Manager* mp_sprite_manager;
        CEGUI::Window* mp_debugwin_root;
    };

    extern cDebug_Window* gp_debug_window;
}

#endif /* TSC_DEBUG_WINDOW_HPP */

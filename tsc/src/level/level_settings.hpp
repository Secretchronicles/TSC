/***************************************************************************
 * level_settings.h
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

#ifndef TSC_LEVEL_SETTINGS_HPP
#define TSC_LEVEL_SETTINGS_HPP

#include "../core/global_basic.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** Level Settings *** *** *** *** *** *** *** *** *** *** */
    class cLevel_Settings {
    public:
        cLevel_Settings();
        ~cLevel_Settings(void);

        void Init(void);
        void Exit(void);
        // Enter game mode
        void Enter(void);
        // Leave game mode (saves settings)
        void Leave(void);
        void Unload(void);

        void Update(void);
        void Draw(void);

        /* handle key down event
         * returns true if the key was processed
        */
        bool Key_Down(const sf::Event& evt);

        // Set the parent level
        void Set_Level(cLevel* level);

        // Adds a Background image
        bool Add_Background_Image(const CEGUI::EventArgs& event);
        // Delete the currently selected Background image
        bool Delete_Background_Image(const CEGUI::EventArgs& event);
        // Set the current Background image
        bool Set_Background_Image(const CEGUI::EventArgs& event);

        // Save the settings and exit
        bool Button_Apply(const CEGUI::EventArgs& event);

        // Updates the Colors
        bool Update_BG_Colors(const CEGUI::EventArgs& event);
        // Loads the Background image List
        void Load_BG_Image_List(void);

        // Updates the Background image
        bool Update_BG_Image(const CEGUI::EventArgs& event);
        // Clears the background Layer fields
        void Clear_Layer_Field(void);

        // cegui events
        bool Spinner_Difficulty_Changed(const CEGUI::EventArgs& event);
        bool Slider_Difficulty_Changed(const CEGUI::EventArgs& event);

        // true if menu is active
        bool m_active;

        // parent level
        cLevel* m_level;
        // settings camera
        cCamera* m_camera;
        // GUI
        CEGUI::Window* m_gui_window;
        CEGUI::TabControl* m_tabcontrol;
        CEGUI::Spinner* m_spinner_difficulty;
        CEGUI::Slider* m_slider_difficulty;
        CEGUI::Window* m_text_difficulty_name;

        // Background colors
        Color m_bg_color_1;
        Color m_bg_color_2;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

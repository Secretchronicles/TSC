/***************************************************************************
 * game_console.hpp
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

#ifndef TSC_GAME_CONSOLE_HPP
#define TSC_GAME_CONSOLE_HPP

namespace TSC {

    class cGame_Console
    {
    public:
        cGame_Console();
        ~cGame_Console();

        void Show();
        void Hide();
        void Toggle();
        bool IsVisible() const;
        void Update();

        void Reset();
        void Append_Text(CEGUI::String text);
        void Append_Text(std::string text);

        void Display_Exception(mrb_state* p_state);

        void History_Back();
        void History_Forward();
    private:
        CEGUI::Window* mp_console_root;
        CEGUI::Editbox* mp_input_edit;
        CEGUI::MultiLineEditbox* mp_output_edit;
        CEGUI::Window* mp_lino_text;
        std::vector<CEGUI::String> m_history;
        CEGUI::String m_last_edit;
        size_t m_history_idx;
        boost::filesystem::ofstream m_logfile;

        void print_preamble();
        bool on_input_accepted(const CEGUI::EventArgs& evt);
        bool on_key_up(const CEGUI::EventArgs& evt);
    };

    extern cGame_Console* gp_game_console;
}

#endif /* TSC_GAME_CONSOLE_HPP */

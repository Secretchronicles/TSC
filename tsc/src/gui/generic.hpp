/***************************************************************************
 * generic.h
 *
 * Copyright © 2005 - 2011 Florian Richter
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

#ifndef TSC_GENERIC_HPP
#define TSC_GENERIC_HPP

#include "../core/global_basic.hpp"
#include "../video/video.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** *** DialogBox *** *** *** *** *** *** *** *** *** */

    class cDialogBox {
    public:
        cDialogBox(void);
        virtual ~cDialogBox(void);

        // initialize
        void Init(void);
        // exit
        void Exit(void);

        // draw
        virtual void Draw(void);
        // update
        virtual void Update(void);

        // if finished
        bool finished;
        // base window
        CEGUI::Window* window;
        // layout filename
        std::string layout_file;

        // hide mouse on exit
        bool mouse_hide;
    };

// CEGUI EditBox with header
    class cDialogBox_Text : public cDialogBox {
    public:
        cDialogBox_Text(void);
        virtual ~cDialogBox_Text(void);

        // initialize
        void Init(std::string title_text);

        // enter
        std::string Enter(std::string default_text, std::string title_text, bool auto_no_text = 1);

        // CEGUI events
        // window quit button clicked event
        bool Button_window_quit_clicked(const CEGUI::EventArgs& event);

        // editbox
        CEGUI::Editbox* box_editbox;
    };

// Button Question Box
    class cDialogBox_Question : public cDialogBox {
    public:
        cDialogBox_Question(void);
        virtual ~cDialogBox_Question(void);

        // initialize
        void Init(bool with_cancel);

        // enter
        int Enter(std::string text, bool with_cancel = 0);

        // CEGUI events
        // yes button clicked event
        bool Button_yes_clicked(const CEGUI::EventArgs& event);
        // no button clicked event
        bool Button_no_clicked(const CEGUI::EventArgs& event);
        // cancel button clicked event
        bool Button_cancel_clicked(const CEGUI::EventArgs& event);

        // box window
        CEGUI::FrameWindow* box_window;

        // return value
        int return_value;
    };

    /* *** *** *** *** *** *** *** Functions *** *** *** *** *** *** *** *** *** *** */

// Update The GUI time handler
    void Gui_Handle_Time(void);

    /* Draw a Statictext
     * if wait_for_input is given draws the text until the user pressed a key
     */
    void Draw_Static_Text(const std::string& text, const Color* color_text = &white, const Color* color_bg = NULL, bool wait_for_input = 1);

    /* CEGUI EditBox with header
     *
     * Parameters
     * default_text : default text in the EditBox
     * title_text : box header text
     * auto_no_text : if true and any key is pressed the default text is cleared
     */
    std::string Box_Text_Input(const std::string& default_text, const std::string& title_text, bool auto_no_text = 1);

    /* Button Question Box
     * returns 1 for Yes, 0 for No and -1 if canceled
     *
     * text : box question text
     * with_cancel : add the cancel button
    */
    int Box_Question(const std::string& text, bool with_cancel = 0);

// Return the clipboard content
    std::string Get_Clipboard_Content(void);
// Set the clipboard content
    void Set_Clipboard_Content(std::string str);
    /* Copy selected GUI text to the Clipboard
     * cut: if set cut the text
    */
    bool GUI_Copy_To_Clipboard(bool cut = 0);
// Paste text from the clipboard to the GUI
    bool GUI_Paste_From_Clipboard(void);

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif


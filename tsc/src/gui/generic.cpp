/***************************************************************************
 * generic.cpp  -  generic gui stuff
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

#include "../gui/generic.hpp"
#include "../core/game_core.hpp"
#include "../core/main.hpp"
#include "../core/framerate.hpp"
#include "../input/mouse.hpp"
#include "../input/keyboard.hpp"
#include "../video/renderer.hpp"
#include "../user/preferences.hpp"

namespace TSC {

/* *** *** *** *** *** *** *** cDialogBox *** *** *** *** *** *** *** *** *** *** */

cDialogBox::cDialogBox(void)
{
    finished = 0;
    window = NULL;
    mouse_hide = 0;
}

cDialogBox::~cDialogBox(void)
{

}

void cDialogBox::Init(void)
{
    // load layout
    window = CEGUI::WindowManager::getSingleton().loadWindowLayout(layout_file);
    pGuiSystem->getGUISheet()->addChildWindow(window);

    // hide mouse on exit
    if (!pMouseCursor->m_active) {
        mouse_hide = 1;
        pMouseCursor->Set_Active(1);
    }
}

void cDialogBox::Exit(void)
{
    // hide mouse
    if (mouse_hide) {
        pMouseCursor->Set_Active(0);
    }

    pGuiSystem->getGUISheet()->removeChildWindow(window);
    CEGUI::WindowManager::getSingleton().destroyWindow(window);
}

void cDialogBox::Draw(void)
{
    Draw_Game();
    pVideo->Draw_Gradient(NULL, 0.905f, &whitealpha128, &black, DIR_VERTICAL);

    pVideo->Render();

    pMouseCursor->Update_Position();
}

void cDialogBox::Update(void)
{
    pFramerate->Update();
    Gui_Handle_Time();
}

/* *** *** *** *** *** *** *** cDialogBox_Text *** *** *** *** *** *** *** *** *** *** */

cDialogBox_Text::cDialogBox_Text(void)
    : cDialogBox()
{
    layout_file = "box_text.layout";
}

cDialogBox_Text::~cDialogBox_Text(void)
{

}

void cDialogBox_Text::Init(std::string title_text)
{
    cDialogBox::Init();

    // get window
    CEGUI::FrameWindow* box_window = static_cast<CEGUI::FrameWindow*>(CEGUI::WindowManager::getSingleton().getWindow("box_text_window"));
    box_window->setText(reinterpret_cast<const CEGUI::utf8*>(title_text.c_str()));
    box_window->setSizingEnabled(0);
    box_window->getCloseButton()->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&cDialogBox_Text::Button_window_quit_clicked, this));

    // get editbox
    box_editbox = static_cast<CEGUI::Editbox*>(CEGUI::WindowManager::getSingleton().getWindow("box_text_editbox"));
}

std::string cDialogBox_Text::Enter(std::string default_text, std::string title_text, bool auto_no_text /* = 1 */)
{
    pVideo->Render_Finish();
    Init(title_text);

    box_editbox->setText(reinterpret_cast<const CEGUI::utf8*>(default_text.c_str()));
    box_editbox->setTooltipText(reinterpret_cast<const CEGUI::utf8*>(title_text.c_str()));
    box_editbox->activate();
    box_editbox->setCaratIndex(default_text.length());

    finished = 0;

    while (!finished) {
        while (SDL_PollEvent(&input_event)) {
            if (input_event.type == SDL_KEYDOWN) {
                if (auto_no_text && default_text.compare(box_editbox->getText().c_str()) == 0) {
                    box_editbox->setText("");
                    // only the first time
                    auto_no_text = 0;
                }

                if (input_event.key.keysym.sym == SDLK_ESCAPE) {
                    box_editbox->setText("");
                    finished = 1;
                }
                else if (input_event.key.keysym.sym == SDLK_RETURN || input_event.key.keysym.sym == SDLK_KP_ENTER) {
                    finished = 1;
                }
                else {
                    pKeyboard->CEGUI_Handle_Key_Down(input_event.key.keysym.sym);
                }
            }
            else if (input_event.type == SDL_KEYUP) {
                pKeyboard->CEGUI_Handle_Key_Up(input_event.key.keysym.sym);
            }
            else {
                pMouseCursor->Handle_Event(&input_event);
            }
        }

        Update();
        Draw();
    }

    std::string return_string = box_editbox->getText().c_str();
    Exit();
    return return_string;
}

bool cDialogBox_Text::Button_window_quit_clicked(const CEGUI::EventArgs& event)
{
    box_editbox->setText("");
    finished = 1;
    return 1;
}

/* *** *** *** *** *** *** *** cDialogBox_Question *** *** *** *** *** *** *** *** *** *** */

cDialogBox_Question::cDialogBox_Question(void)
    : cDialogBox()
{
    layout_file = "box_question.layout";
    box_window = NULL;
    return_value = -1;
}

cDialogBox_Question::~cDialogBox_Question(void)
{

}

void cDialogBox_Question::Init(bool with_cancel)
{
    pVideo->Render_Finish();
    cDialogBox::Init();

    // get window
    box_window = static_cast<CEGUI::FrameWindow*>(CEGUI::WindowManager::getSingleton().getWindow("box_question_window"));
    box_window->activate();

    // subscribe close button
    if (with_cancel) {
        box_window->getCloseButton()->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&cDialogBox_Question::Button_cancel_clicked, this));
    }
    else {
        box_window->getCloseButton()->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&cDialogBox_Question::Button_no_clicked, this));
    }
}

int cDialogBox_Question::Enter(std::string text, bool with_cancel /* = 0 */)
{
    Init(with_cancel);

    // get text
    CEGUI::Editbox* box_text = static_cast<CEGUI::Editbox*>(CEGUI::WindowManager::getSingleton().getWindow("box_question_text"));
    box_text->setText(reinterpret_cast<const CEGUI::utf8*>(text.c_str()));


    // align text
    CEGUI::Font* font = &CEGUI::FontManager::getSingleton().get("bluebold_medium");
    // fixme : Can't handle multiple lines of text
    float text_width = font->getTextExtent(text) * global_downscalex;

    if (text_width > 250) {
        box_window->setWidth(CEGUI::UDim(0, (text_width + 15) * global_upscalex));
        box_window->setXPosition(CEGUI::UDim(0, (game_res_w * 0.5f - text_width * 0.5f) * global_upscalex));
    }

    // Button Yes
    CEGUI::PushButton* button_yes = static_cast<CEGUI::PushButton*>(CEGUI::WindowManager::getSingleton().getWindow("box_question_button_yes"));
    // Button No
    CEGUI::PushButton* button_no = static_cast<CEGUI::PushButton*>(CEGUI::WindowManager::getSingleton().getWindow("box_question_button_no"));
    // Button Cancel
    CEGUI::PushButton* button_cancel = static_cast<CEGUI::PushButton*>(CEGUI::WindowManager::getSingleton().getWindow("box_question_button_cancel"));

    // if without cancel
    if (!with_cancel) {
        button_cancel->hide();
    }

    // events
    button_yes->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&cDialogBox_Question::Button_yes_clicked, this));
    button_no->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&cDialogBox_Question::Button_no_clicked, this));
    button_cancel->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&cDialogBox_Question::Button_cancel_clicked, this));

    finished = 0;

    while (!finished) {
        Draw();

        while (SDL_PollEvent(&input_event)) {
            if (input_event.type == SDL_KEYDOWN) {
                if (input_event.key.keysym.sym == SDLK_ESCAPE) {
                    if (with_cancel) {
                        return_value = -1;
                    }
                    else {
                        return_value = 0;
                    }

                    finished = 1;
                }
                else if (input_event.key.keysym.sym == SDLK_RETURN || input_event.key.keysym.sym == SDLK_KP_ENTER) {
                    return_value = 1;
                    finished = 1;
                }
                else {
                    pKeyboard->CEGUI_Handle_Key_Down(input_event.key.keysym.sym);
                }
            }
            else if (input_event.type == SDL_KEYUP) {
                pKeyboard->CEGUI_Handle_Key_Up(input_event.key.keysym.sym);
            }
            else {
                pMouseCursor->Handle_Event(&input_event);
            }
        }

        Update();
    }

    Exit();

    return return_value;
}

bool cDialogBox_Question::Button_yes_clicked(const CEGUI::EventArgs& event)
{
    return_value = 1;
    finished = 1;
    return 1;
}

bool cDialogBox_Question::Button_no_clicked(const CEGUI::EventArgs& event)
{
    return_value = 0;
    finished = 1;
    return 1;
}

bool cDialogBox_Question::Button_cancel_clicked(const CEGUI::EventArgs& event)
{
    return_value = -1;
    finished = 1;
    return 1;
}

/* *** *** *** *** *** *** *** Functions *** *** *** *** *** *** *** *** *** *** */

void Gui_Handle_Time(void)
{
    static float last_time_pulse = 0;

    // get current "run-time" in seconds
    float t = 0.001f * SDL_GetTicks();

    // inject the time that passed since the last call
    CEGUI::System::getSingleton().injectTimePulse(t - last_time_pulse);

    // store the new time as the last time
    last_time_pulse = t;
}

void Draw_Static_Text(const std::string& text, const Color* color_text /* = &white */, const Color* color_bg /* = NULL */, bool wait_for_input /* = 1 */)
{
    // fixme : Can't handle multiple lines of text. Change to MultiLineEditbox or use HorzFormatting=WordWrapLeftAligned property.
    bool draw = 1;

    // Statictext window
    CEGUI::Window* window_statictext = CEGUI::WindowManager::getSingleton().loadWindowLayout("statictext.layout");
    pGuiSystem->getGUISheet()->addChildWindow(window_statictext);
    // get default text
    CEGUI::Window* text_default = static_cast<CEGUI::Window*>(CEGUI::WindowManager::getSingleton().getWindow("text_default"));

    // set text
    text_default->setProperty("TextColours", CEGUI::PropertyHelper::colourToString(CEGUI::colour(static_cast<float>(color_text->red) / 255, static_cast<float>(color_text->green) / 255, static_cast<float>(color_text->blue) / 255, 1)));
    CEGUI::String gui_text = reinterpret_cast<const CEGUI::utf8*>(text.c_str());
    text_default->setText(gui_text);

    // align text
    CEGUI::Font* font = &CEGUI::FontManager::getSingleton().get("bluebold_medium");
    float text_width = font->getTextExtent(gui_text) * global_downscalex;

    text_default->setWidth(CEGUI::UDim(0, (text_width + 15) * global_upscalex));
    text_default->setXPosition(CEGUI::UDim(0, (game_res_w * 0.5f - text_width * 0.5f) * global_upscalex));
    text_default->moveToFront();

    float text_height = font->getLineSpacing();
    text_height *= 1 + std::count(text.begin(), text.end(), '\n');
    // set window height
    text_default->setHeight(CEGUI::UDim(0, text_height + (12 * global_upscaley)));

    while (draw) {
        Draw_Game();

        // draw background
        if (color_bg) {
            // create request
            cRect_Request* request = new cRect_Request();

            pVideo->Draw_Rect(NULL, 0.9f, color_bg, request);
            request->m_render_count = wait_for_input ? 4 : 1;

            // add request
            pRenderer->Add(request);
        }

        pVideo->Render();

        if (wait_for_input) {
            while (SDL_PollEvent(&input_event)) {
                if (input_event.type == SDL_KEYDOWN || input_event.type == SDL_JOYBUTTONDOWN || input_event.type == SDL_MOUSEBUTTONDOWN) {
                    draw = 0;
                }
            }

            // if vsync is disabled then limit the fps to reduce the CPU usage
            if (!pPreferences->m_video_vsync) {
                Correct_Frame_Time(100);
            }
        }
        else {
            draw = 0;
        }

        pFramerate->Update();
    }

    // Clear possible active input
    if (wait_for_input) {
        Clear_Input_Events();
    }

    pGuiSystem->getGUISheet()->removeChildWindow(window_statictext);
    CEGUI::WindowManager::getSingleton().destroyWindow(window_statictext);
}

std::string Box_Text_Input(const std::string& default_text, const std::string& title_text, bool auto_no_text /* = 1 */)
{
    cDialogBox_Text* box = new cDialogBox_Text();
    std::string return_value = box->Enter(default_text, title_text, auto_no_text);
    delete box;

    return return_value;
}

int Box_Question(const std::string& text, bool with_cancel /* = 0 */)
{
    cDialogBox_Question* box = new cDialogBox_Question();
    int return_value = box->Enter(text, with_cancel);
    delete box;

    return return_value;
}

std::string Get_Clipboard_Content(void)
{
    std::string content;
#ifdef _WIN32
    if (OpenClipboard(NULL)) {
        bool ucs2 = 0;
        if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
            ucs2 = 1;
        }

        HANDLE h;

        // Both have line ends as CR-LF and a null character at the end of the data.
        if (ucs2) {
            // CF_UNICODETEXT is UCS-2
            h = GetClipboardData(CF_UNICODETEXT);
        }
        else {
            // CF_TEXT is Windows-1252
            h = GetClipboardData(CF_TEXT);
        }

        // no handle
        if (!h) {
            printf("Could not get clipboard data\n");
            CloseClipboard();
            return content;
        }

        // get content
        if (ucs2) {
            content = ucs2_to_utf8(static_cast<wchar_t*>(GlobalLock(h)));
        }
        else {
            content = static_cast<char*>(GlobalLock(h));
        }

        // clean up
        GlobalUnlock(h);
        CloseClipboard();
    }
#elif __APPLE__
    // not tested
    ScrapRef scrap;
    if (::GetCurrentScrap(&scrap) != noErr) {
        return false;
    }

    Size bytecount = 0;
    OSStatus status = ::GetScrapFlavorSize(scrap, kScrapFlavorTypeText, &bytecount);
    if (status != noErr) {
        return false;
    }

    char* buffer = new char[bytecount];
    if (::GetScrapFlavorData(scrap, kScrapFlavorTypeText, &bytecount, buffer) == noErr) {
        content = static_cast<char*>(buffer);
    }

    delete[] buffer;
#elif __unix__
    // only works with the cut-buffer method (xterm) and not with the more recent selections method
    SDL_SysWMinfo sdlinfo;
    SDL_VERSION(&sdlinfo.version);
    if (SDL_GetWMInfo(&sdlinfo)) {
        sdlinfo.info.x11.lock_func();
        Display* display = sdlinfo.info.x11.display;
        int count = 0;
        char* msg = XFetchBytes(display, &count);
        if (msg) {
            if (count > 0) {
                content.append(msg, count);
            }

            XFree(msg);
        }

        sdlinfo.info.x11.unlock_func();
    }
#endif
    return content;
}

void Set_Clipboard_Content(std::string str)
{
#ifdef _WIN32
    if (OpenClipboard(NULL)) {
        if (!EmptyClipboard()) {
            printf("Failed to empty clipboard\n");
            return;
        }

        unsigned int length = (str.length() + 1) * sizeof(std::string::allocator_type);
        HANDLE h = GlobalAlloc((GMEM_MOVEABLE|GMEM_DDESHARE|GMEM_ZEROINIT), length);

        if (!h) {
            printf("Could not allocate clipboard memory\n");
            return;
        }

        void* data = GlobalLock(h);

        if (!data) {
            GlobalFree(h);
            CloseClipboard();
            printf("Could not lock clipboard memory\n");
            return;
        }

        memcpy(data, str.c_str(), length);
        GlobalUnlock(h);

        HANDLE data_result = SetClipboardData(CF_TEXT, h);

        if (!data_result) {
            GlobalFree(h);
            CloseClipboard();
            printf("Could not set clipboard data\n");
        }

        CloseClipboard();
    }
#elif __APPLE__
    // not implemented
#elif __unix__
    SDL_SysWMinfo sdlinfo;
    SDL_VERSION(&sdlinfo.version);
    if (SDL_GetWMInfo(&sdlinfo)) {
        sdlinfo.info.x11.lock_func();
        Display* display = sdlinfo.info.x11.display;
        Window window = sdlinfo.info.x11.window;

        XChangeProperty(display, DefaultRootWindow(display), XA_CUT_BUFFER0, XA_STRING, 8, PropModeReplace, static_cast<const unsigned char*>(static_cast<const void*>(str.c_str())), str.length());

        if (XGetSelectionOwner(display, XA_PRIMARY) != window) {
            XSetSelectionOwner(display, XA_PRIMARY, window, CurrentTime);
        }

        sdlinfo.info.x11.unlock_func();
    }
#endif
}

bool GUI_Copy_To_Clipboard(bool cut)
{
    CEGUI::Window* sheet = CEGUI::System::getSingleton().getGUISheet();

    // no sheet
    if (!sheet) {
        return 0;
    }

    CEGUI::Window* window_active = sheet->getActiveChild();

    // no active window
    if (!window_active) {
        return 0;
    }

    CEGUI::String sel_text;
    const CEGUI::String& type = window_active->getType();

    // MultiLineEditbox
    if (type.find("/MultiLineEditbox") != CEGUI::String::npos) {
        CEGUI::MultiLineEditbox* editbox = static_cast<CEGUI::MultiLineEditbox*>(window_active);
        CEGUI::String::size_type beg = editbox->getSelectionStartIndex();
        CEGUI::String::size_type len = editbox->getSelectionLength();
        sel_text = editbox->getText().substr(beg, len).c_str();

        // if cutting
        if (cut) {
            if (editbox->isReadOnly()) {
                return 0;
            }

            CEGUI::String new_text = editbox->getText();
            editbox->setText(new_text.erase(beg, len));
        }
    }
    // Editbox
    else if (type.find("/Editbox") != CEGUI::String::npos) {
        CEGUI::Editbox* editbox = static_cast<CEGUI::Editbox*>(window_active);
        CEGUI::String::size_type beg = editbox->getSelectionStartIndex();
        CEGUI::String::size_type len = editbox->getSelectionLength();
        sel_text = editbox->getText().substr(beg, len).c_str();

        // if cutting
        if (cut) {
            if (editbox->isReadOnly()) {
                return 0;
            }

            CEGUI::String new_text = editbox->getText();
            editbox->setText(new_text.erase(beg, len));
        }
    }
    else {
        return 0;
    }

    Set_Clipboard_Content(sel_text.c_str());
    return 1;
}

bool GUI_Paste_From_Clipboard(void)
{
    CEGUI::Window* sheet = CEGUI::System::getSingleton().getGUISheet();

    // no sheet
    if (!sheet) {
        return 0;
    }

    CEGUI::Window* window_active = sheet->getActiveChild();

    // no active window
    if (!window_active) {
        return 0;
    }

    const CEGUI::String& type = window_active->getType();

    // MultiLineEditbox
    if (type.find("/MultiLineEditbox") != CEGUI::String::npos) {
        CEGUI::MultiLineEditbox* editbox = static_cast<CEGUI::MultiLineEditbox*>(window_active);

        if (editbox->isReadOnly()) {
            return 0;
        }

        CEGUI::String::size_type beg = editbox->getSelectionStartIndex();
        CEGUI::String::size_type len = editbox->getSelectionLength();

        CEGUI::String new_text = editbox->getText();
        // erase selected text
        new_text.erase(beg, len);

        // get clipboard text
        CEGUI::String clipboard_text = reinterpret_cast<const CEGUI::utf8*>(Get_Clipboard_Content().c_str());
        // set new text
        editbox->setText(new_text.insert(beg, clipboard_text));
        // set new carat index
        editbox->setCaratIndex(editbox->getCaratIndex() + clipboard_text.length());
    }
    // Editbox
    else if (type.find("/Editbox") != CEGUI::String::npos) {
        CEGUI::Editbox* editbox = static_cast<CEGUI::Editbox*>(window_active);

        if (editbox->isReadOnly()) {
            return 0;
        }

        CEGUI::String::size_type beg = editbox->getSelectionStartIndex();
        CEGUI::String::size_type len = editbox->getSelectionLength();

        CEGUI::String new_text = editbox->getText();
        // erase selected text
        new_text.erase(beg, len);

        // get clipboard text
        CEGUI::String clipboard_text = reinterpret_cast<const CEGUI::utf8*>(Get_Clipboard_Content().c_str());
        // set new text
        editbox->setText(new_text.insert(beg, clipboard_text));
        // set new carat index
        editbox->setCaratIndex(editbox->getCaratIndex() + clipboard_text.length());
    }
    else {
        return 0;
    }

    return 1;
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

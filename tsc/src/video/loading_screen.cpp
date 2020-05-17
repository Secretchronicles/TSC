/***************************************************************************
 * loading_screen.cpp  -  global routines for drawing the loading screen
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

#include "../core/global_basic.hpp"
#include "../core/i18n.hpp"
#include "../core/framerate.hpp"
#include "../video/video.hpp"
#include "../video/renderer.hpp"
#include "loading_screen.hpp"

#define LOADING_ROOT_NAME "loadingroot"
#define GPL_TEXT_NAME "text_gpl"
#define LOADING_TEXT_NAME "text_loading"
#define PROGRESSBAR_NAME "progress_bar"

void TSC::Loading_Screen_Init(void)
{
    CEGUI::GUIContext& guicontext = CEGUI::System::getSingleton().getDefaultGUIContext();

    // Create new window
    CEGUI::WindowManager& winmanager = CEGUI::WindowManager::getSingleton();
    CEGUI::Window* p_loadingscreen   = winmanager.createWindow("DefaultWindow", LOADING_ROOT_NAME);

    // Use this as our slate for the loading screen so that we
    // can easily destroy all its child widgets at once later.
    guicontext.getRootWindow()->addChild(p_loadingscreen);

    CEGUI::ProgressBar* p_progress_bar =
        static_cast<CEGUI::ProgressBar*>(winmanager.createWindow("TSCLook256/ProgressBar", PROGRESSBAR_NAME));
    CEGUI::Window* p_progress_text =
        winmanager.createWindow("TSCLook256/StaticText", LOADING_TEXT_NAME);
    CEGUI::Window* p_license_text =
        winmanager.createWindow("TSCLook256/StaticText", GPL_TEXT_NAME);

    p_progress_bar->setStepSize(0.01f);
    p_progress_bar->setSize(CEGUI::USize(CEGUI::UDim(0.7, 0), CEGUI::UDim(0, 58)));
    p_progress_bar->setPosition(CEGUI::UVector2(CEGUI::UDim(0.15, 0), CEGUI::UDim(0.5, -29)));
    p_loadingscreen->addChild(p_progress_bar);

    p_progress_text->setSize(CEGUI::USize(CEGUI::UDim(0.7, 0), CEGUI::UDim(0, 55)));
    p_progress_text->setPosition(CEGUI::UVector2(CEGUI::UDim(0.15, 0), CEGUI::UDim(0.5, 29 + 10)));
    p_loadingscreen->addChild(p_progress_text);

    p_license_text->setSize(CEGUI::USize(CEGUI::UDim(0.7, 0), CEGUI::UDim(0, 55)));
    p_license_text->setPosition(CEGUI::UVector2(CEGUI::UDim(0.15, 0), CEGUI::UDim(0.8, 0)));
    p_loadingscreen->addChild(p_license_text);

    // Set license info as translatable string
    // TRANS: Be careful with the length of this line, if
    // TRANS: it is much longer than the English version,
    // TRANS: it will be cut off.
    p_license_text->setText(UTF8_("This program is distributed under the terms of the GPLv3"));

    // set info text
    p_progress_text->setText(_("Loading"));
}

void TSC::Loading_Screen_Draw_Text(const std::string& str_info /* = "Loading" */)
{
    // Retrieve the text widget from the root window
    CEGUI::GUIContext& gui_context = CEGUI::System::getSingleton().getDefaultGUIContext();
    CEGUI::Window* p_rootwindow = gui_context.getRootWindow();
    CEGUI::Window* p_text = p_rootwindow->getChild(LOADING_ROOT_NAME "/" LOADING_TEXT_NAME);

    // set info text
    if (!p_text) {
        throw(std::runtime_error("Loading Screen not initialized."));
    }
    p_text->setText(reinterpret_cast<const CEGUI::utf8*>(str_info.c_str()));

    Loading_Screen_Draw();
}

void TSC::Loading_Screen_Set_Progress(float progress)
{
    // Retrieve the progressbar widget from the loading screen
    CEGUI::GUIContext& gui_context = CEGUI::System::getSingleton().getDefaultGUIContext();
    CEGUI::Window* p_rootwindow = gui_context.getRootWindow();
    CEGUI::ProgressBar* p_progress = static_cast<CEGUI::ProgressBar*>(p_rootwindow->getChild(LOADING_ROOT_NAME "/" PROGRESSBAR_NAME));

    // set info text
    if (!p_progress) {
        throw(std::runtime_error("Loading Screen not initialized."));
    }
    p_progress->setProgress(progress);
}

void TSC::Loading_Screen_Draw(void)
{
    // limit fps or vsync will slow down the loading
    if (!Is_Frame_Time(60)) {
        pRenderer->Fake_Render();
        return;
    }

    // clear screen
    pVideo->Clear_Screen();
    pVideo->Draw_Rect(NULL, 0.00001f, &black);

    // Render
    pRenderer->Render();
    CEGUI::System::getSingleton().renderAllGUIContexts();
    pVideo->mp_window->display();
}

void TSC::Loading_Screen_Exit(void)
{
    CEGUI::GUIContext& gui_context = CEGUI::System::getSingleton().getDefaultGUIContext();
    CEGUI::Window* p_rootwindow = gui_context.getRootWindow();

    // loading window is present
    if (p_rootwindow) {
        // Destroy the loading screen's widgets
        CEGUI::Window* p_loadingscreen = p_rootwindow->getChild(LOADING_ROOT_NAME);
        p_rootwindow->removeChild(p_rootwindow);
        CEGUI::WindowManager::getSingleton().destroyWindow(p_loadingscreen);
    }
    else {
        throw(std::runtime_error("Can't exit from loading screen if no loading screen exists!"));
    }
}

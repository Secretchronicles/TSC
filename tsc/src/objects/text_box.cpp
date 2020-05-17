/***************************************************************************
 * text_box.cpp  -  box speaking to you
 *
 * Copyright © 2007 - 2011 Florian Richter
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

#include "../objects/text_box.hpp"
#include "../core/framerate.hpp"
#include "../core/game_core.hpp"
#include "../core/i18n.hpp"
#include "../user/preferences.hpp"
#include "../input/joystick.hpp"
#include "../core/main.hpp"
#include "../input/keyboard.hpp"
#include "../core/i18n.hpp"
#include "../audio/audio.hpp"
#include "../level/level.hpp"
#include "../core/sprite_manager.hpp"
#include "../level/level.hpp"
#include "../level/level_settings.hpp"
#include "../core/editor/editor.hpp"
#include "../level/level_editor.hpp"

namespace TSC {

/* *** *** *** *** *** *** *** *** cText_Box *** *** *** *** *** *** *** *** *** */

static unsigned int text_box_window_width = 300;
static unsigned int text_box_window_height = 200;

cText_Box::cText_Box(cSprite_Manager* sprite_manager)
    : cBaseBox(sprite_manager)
{
    cText_Box::Init();
}

cText_Box::cText_Box(XmlAttributes& attributes, cSprite_Manager* sprite_manager)
    : cBaseBox(sprite_manager)
{
    cText_Box::Init();
    cText_Box::Load_From_XML(attributes);
}

cText_Box::~cText_Box(void)
{

}

void cText_Box::Init(void)
{
    m_type = TYPE_TEXT_BOX;
    box_type = m_type;
    m_can_be_on_ground = 0;
    m_name = _("Text Box");

    // default is infinite times activate-able
    Set_Useable_Count(-1, 1);
    // Spinbox Animation
    Set_Animation_Type("Default");

    // todo : editor image needed
    //item_image = NULL;
}

cText_Box* cText_Box::Copy(void) const
{
    cText_Box* text_box = new cText_Box(m_sprite_manager);
    text_box->Set_Pos(m_start_pos_x, m_start_pos_y);
    text_box->Set_Text(m_text);
    text_box->Set_Invisible(m_box_invisible);
    return text_box;
}

void cText_Box::Load_From_XML(XmlAttributes& attributes)
{
    cBaseBox::Load_From_XML(attributes);

    // text
    Set_Text(xml_string_to_string(attributes["text"]));
}

xmlpp::Element* cText_Box::Save_To_XML_Node(xmlpp::Element* p_element)
{
    xmlpp::Element* p_node = cBaseBox::Save_To_XML_Node(p_element);

    // text
    Add_Property(p_node, "text", m_text);

    return p_node;
}

void cText_Box::Activate(void)
{
    cBaseBox::Activate();

    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();
    CEGUI::MultiLineEditbox* editbox = static_cast<CEGUI::MultiLineEditbox*>(wmgr.createWindow("TSCLook256/MultiLineEditbox", "text_box_text"));

    // add to main window
    CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->addChild(editbox);

    // set on top
    editbox->setAlwaysOnTop(1);
    // set position
    float text_pos_x = m_pos_x - (text_box_window_width * 0.5f) + (m_rect.m_w * 0.5f);
    float text_pos_y = m_pos_y - 5 - text_box_window_height;

    // if not on screen on the left side
    if (text_pos_x < 0) {
        // put it on screen
        text_pos_x = 0;
    }
    // if not on screen on the right side
    if (text_pos_x > pActive_Camera->m_limit_rect.m_x + pActive_Camera->m_limit_rect.m_w - text_box_window_width) {
        // put it on screen
        text_pos_x = pActive_Camera->m_limit_rect.m_x + pActive_Camera->m_limit_rect.m_w - text_box_window_width;
    }
    // if not on screen on the top side
    if (text_pos_y < pActive_Camera->m_y) {
        // put it on screen
        text_pos_y = pActive_Camera->m_y;
    }

    editbox->setXPosition(CEGUI::UDim(0, (text_pos_x - pActive_Camera->m_x) * global_upscalex));
    editbox->setYPosition(CEGUI::UDim(0, (text_pos_y - pActive_Camera->m_y) * global_upscaley));
    // set size
    editbox->setWidth(CEGUI::UDim(0, text_box_window_width * global_upscalex));
    editbox->setHeight(CEGUI::UDim(0, text_box_window_height * global_upscaley));

    // set text
    editbox->setText(reinterpret_cast<const CEGUI::utf8*>(m_text.c_str()));
    // always hide horizontal scrollbar
    editbox->getHorzScrollbar()->hide();

    bool display = 1;

    while (display) {
        while (pVideo->PollEvent(input_event)) {
            if (input_event.type == sf::Event::KeyPressed) {

                // exit keys
                if (input_event.key.code == pPreferences->m_key_action || input_event.key.code == sf::Keyboard::Escape || input_event.key.code == sf::Keyboard::Return || input_event.key.code == sf::Keyboard::Space) {
                    display = 0;
                    break;
                }
                // handled keys
                else if (input_event.key.code == pPreferences->m_key_right || input_event.key.code == pPreferences->m_key_left) {
                    pKeyboard->Key_Down(input_event);
                }
            }
            else if (input_event.type == sf::Event::KeyReleased) {

                // handled keys
                if (input_event.key.code == pPreferences->m_key_right || input_event.key.code == pPreferences->m_key_left) {
                    pKeyboard->Key_Up(input_event);
                }
            }
            else if (input_event.type == sf::Event::JoystickButtonPressed) {
                if (input_event.joystickButton.button == pPreferences->m_joy_button_action || input_event.joystickButton.button == pPreferences->m_joy_button_exit) {
                    display = 0;
                    break;
                }
            }
            else if (input_event.type == sf::Event::JoystickMoved) {
                pJoystick->Handle_Motion(input_event);
                break;
            }
        }

        // down
        if (sf::Keyboard::isKeyPressed(pPreferences->m_key_down) || pJoystick->Down()) {
            editbox->getVertScrollbar()->setScrollPosition(editbox->getVertScrollbar()->getScrollPosition() + (editbox->getVertScrollbar()->getStepSize() * 0.25f * pFramerate->m_speed_factor));
        }
        // up
        if (sf::Keyboard::isKeyPressed(pPreferences->m_key_up) || pJoystick->Up()) {
            editbox->getVertScrollbar()->setScrollPosition(editbox->getVertScrollbar()->getScrollPosition() - (editbox->getVertScrollbar()->getStepSize() * 0.25f * pFramerate->m_speed_factor));
        }

        // move camera because text could not be completely visible
        if (pActive_Camera->m_y_offset > 0) {
            pActive_Camera->m_y_offset -= 2;
            // set position
            pActive_Camera->Center();

            // set position
            editbox->setXPosition(CEGUI::UDim(0, (text_pos_x - pActive_Camera->m_x) * global_upscalex));
            editbox->setYPosition(CEGUI::UDim(0, (text_pos_y - pActive_Camera->m_y) * global_upscaley));
        }

        // update animation
        Update();

        // update audio
        pAudio->Update();
        // draw
        Draw_Game();
        // render
        pVideo->Render();
        pFramerate->Update();
    }

    wmgr.destroyWindow(editbox);
}

void cText_Box::Update(void)
{
    if (!m_valid_update || !Is_In_Range()) {
        return;
    }

    cBaseBox::Update();
}

void cText_Box::Set_Text(const std::string& str_text)
{
    m_text = str_text;
}

void cText_Box::Editor_Activate(void)
{
    // BaseBox Settings first
    cBaseBox::Editor_Activate();

    // get window manager
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    // text
    CEGUI::MultiLineEditbox* editbox = static_cast<CEGUI::MultiLineEditbox*>(wmgr.createWindow("TSCLook256/MultiLineEditbox", "text_box_text"));
    pLevel_Editor->Add_Config_Widget(UTF8_("Text"), UTF8_("Text to display when activated"), editbox, static_cast<float>(text_box_window_height));

    editbox->setWordWrapping(false);
    editbox->setText(reinterpret_cast<const CEGUI::utf8*>(m_text.c_str()));
    editbox->subscribeEvent(CEGUI::MultiLineEditbox::EventTextChanged, CEGUI::Event::Subscriber(&cText_Box::Editor_Text_Text_Changed, this));

    // init
    Editor_Init();
}

bool cText_Box::Editor_Text_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::MultiLineEditbox*>(windowEventArgs.window)->getText().c_str();

    Set_Text(str_text);

    return 1;
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

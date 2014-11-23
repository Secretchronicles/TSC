/***************************************************************************
 * editor.cpp  -  class for the basic editor
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

#include "editor.hpp"
#include "editor_items_loader.hpp"
#include "../game_core.hpp"
#include "../../gui/generic.hpp"
#include "../framerate.hpp"
#include "../../audio/audio.hpp"
#include "../../video/font.hpp"
#include "../../video/animation.hpp"
#include "../../input/keyboard.hpp"
#include "../../input/mouse.hpp"
#include "../../input/joystick.hpp"
#include "../../user/preferences.hpp"
#include "../../level/level.hpp"
#include "../../level/level_player.hpp"
#include "../../overworld/world_manager.hpp"
#include "../../video/renderer.hpp"
#include "../sprite_manager.hpp"
#include "../../overworld/overworld.hpp"
#include "../i18n.hpp"
#include "../filesystem/filesystem.hpp"
#include "../filesystem/resource_manager.hpp"
#include "../errors.hpp"

namespace fs = boost::filesystem;

namespace TSC {

/* *** *** *** *** *** *** *** cEditor_Object_Settings_Item *** *** *** *** *** *** *** *** *** *** */

cEditor_Object_Settings_Item::cEditor_Object_Settings_Item(void)
{
    window_name = NULL;
    window_setting = NULL;
    advance_row = 1;
}

cEditor_Object_Settings_Item::~cEditor_Object_Settings_Item(void)
{
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    wmgr.destroyWindow(window_name);
    wmgr.destroyWindow(window_setting);
}

/* *** *** *** *** *** *** *** *** cEditor_CEGUI_Texture *** *** *** *** *** *** *** *** *** */

cEditor_CEGUI_Texture::cEditor_CEGUI_Texture(CEGUI::OpenGLRenderer& owner, GLuint tex, const CEGUI::Size& size)
    : CEGUI::OpenGLTexture(owner, tex, size)
{

}

cEditor_CEGUI_Texture::~cEditor_CEGUI_Texture(void)
{
    cleanupOpenGLTexture();
}

void cEditor_CEGUI_Texture::cleanupOpenGLTexture(void)
{
    // from cegui
    if (d_grabBuffer) {
        delete[] d_grabBuffer;
        d_grabBuffer = 0;
    }
    // do not delete it
    else {
        d_ogltexture = 0;
    }
}

/* *** *** *** *** *** *** *** *** cEditor_Item_Object *** *** *** *** *** *** *** *** *** */

cEditor_Item_Object::cEditor_Item_Object(const std::string& text, const CEGUI::Listbox* parent)
    : CEGUI::ListboxItem("")
{
    m_parent = parent;
    list_text = new CEGUI::ListboxTextItem(reinterpret_cast<const CEGUI::utf8*>(text.c_str()));
    list_text->setSelectionColours(CEGUI::colour(0.33f, 0.33f, 0.33f));
    list_text->setSelectionBrushImage("TaharezLook", "ListboxSelectionBrush");

    m_image = NULL;
    sprite_obj = NULL;
    preview_scale = 1;
}

cEditor_Item_Object::~cEditor_Item_Object(void)
{
    delete list_text;

    if (m_image) {
        delete m_image->getTexture();
        CEGUI::ImagesetManager::getSingleton().destroy(*m_image);
        m_image = NULL;
    }

    if (sprite_obj) {
        delete sprite_obj;
    }
}

void cEditor_Item_Object::Init(cSprite* sprite)
{
    if (m_image) {
        printf("cEditor_Item_Object::Init: Warning: Image is already set\n");
        return;
    }

    /* Don't set sprite settings which could get copied
     * into the level/world like shadow and z position
    */
    sprite_obj = sprite;

    // CEGUI settings
    list_text->setTextColours(Get_Massive_Type_Color(sprite_obj->m_massive_type).Get_cegui_Color());

    if (!sprite_obj->m_start_image || !pPreferences->m_editor_show_item_images) {
        return;
    }

    // get scale
    preview_scale = pVideo->Get_Scale(sprite_obj->m_start_image, static_cast<float>(pPreferences->m_editor_item_image_size) * 2.0f, static_cast<float>(pPreferences->m_editor_item_image_size));

    // create CEGUI link
    cEditor_CEGUI_Texture* texture = new cEditor_CEGUI_Texture(*pGuiRenderer, sprite_obj->m_start_image->m_image, CEGUI::Size(sprite_obj->m_start_image->m_tex_w, sprite_obj->m_start_image->m_tex_h));
    CEGUI::String imageset_name = "editor_item " + list_text->getText() + " " + CEGUI::PropertyHelper::uintToString(m_parent->getItemCount());
    m_image = &CEGUI::ImagesetManager::getSingleton().create(imageset_name, *texture);
    m_image->defineImage("default", CEGUI::Point(0, 0), texture->getSize(), CEGUI::Point(0, 0));
}

CEGUI::Size cEditor_Item_Object::getPixelSize(void) const
{
    CEGUI::Size tmp = list_text->getPixelSize();

    if (pPreferences->m_editor_show_item_images) {
        tmp.d_height += (pPreferences->m_editor_item_image_size + 10) * global_upscaley;
    }

    return tmp;
}

void cEditor_Item_Object::draw(CEGUI::GeometryBuffer& buffer, const CEGUI::Rect& targetRect, float alpha, const CEGUI::Rect* clipper) const
{
    // image
    if (m_image && pPreferences->m_editor_show_item_images) {
        m_image->draw(buffer, CEGUI::Rect(CEGUI::Point(0, 0), m_image->getTexture()->getSize()), CEGUI::Rect(targetRect.d_left + 15, targetRect.d_top + 22, targetRect.d_left + 15 + (sprite_obj->m_start_image->m_start_w * preview_scale * global_upscalex), targetRect.d_top + 22 + (sprite_obj->m_start_image->m_start_h * preview_scale * global_upscaley)), clipper, CEGUI::ColourRect(CEGUI::colour(1.0f, 1.0f, 1.0f, alpha)), CEGUI::TopLeftToBottomRight);
    }
    // name text
    list_text->draw(buffer, targetRect, alpha, clipper);
}

/* *** *** *** *** *** *** *** *** cEditor_Menu_Object *** *** *** *** *** *** *** *** *** */

cEditor_Menu_Object::cEditor_Menu_Object(const std::string& text)
    : ListboxTextItem(text.c_str())
{
    bfunction = 0;
    header = 0;
}

cEditor_Menu_Object::~cEditor_Menu_Object(void)
{

}

void cEditor_Menu_Object::Init(void)
{
    setSelectionColours(CEGUI::colour(0.33f, 0.33f, 0.33f));
    setSelectionBrushImage("TaharezLook", "ListboxSelectionBrush");
}

/* *** *** *** *** *** *** *** cEditor *** *** *** *** *** *** *** *** *** *** */

cEditor::cEditor(cSprite_Manager* sprite_manager)
{
    m_sprite_manager = sprite_manager;
    m_enabled = 0;

    m_camera_speed = 35;
    m_menu_timer = 0;
    m_editor_window = NULL;
    m_listbox_menu = NULL;
    m_listbox_items = NULL;
    m_tabcontrol_menu = NULL;
}

cEditor::~cEditor(void)
{
    cEditor::Unload();
}

void cEditor::Init(void)
{
    // already loaded
    if (m_editor_window) {
        return;
    }

    // Create Editor CEGUI Window
    m_editor_window = CEGUI::WindowManager::getSingleton().loadWindowLayout("editor.layout");
    pGuiSystem->getGUISheet()->addChildWindow(m_editor_window);

    // Get TabControl
    m_tabcontrol_menu = static_cast<CEGUI::TabControl*>(CEGUI::WindowManager::getSingleton().getWindow("tabcontrol_editor"));
    // fixme : CEGUI does not detect the mouse enter event if in the listbox or any other window in it
    // TabControl Menu Tab Events
    m_tabcontrol_menu->getTabContents("editor_tab_menu")->subscribeEvent(CEGUI::Window::EventMouseEnters, CEGUI::Event::Subscriber(&cEditor::Editor_Mouse_Enter, this));
    // TabControl Items Tab Events
    m_tabcontrol_menu->getTabContents("editor_tab_items")->subscribeEvent(CEGUI::Window::EventMouseEnters, CEGUI::Event::Subscriber(&cEditor::Editor_Mouse_Enter, this));

    // Get Menu Listbox
    m_listbox_menu = static_cast<CEGUI::Listbox*>(CEGUI::WindowManager::getSingleton().getWindow("editor_menu"));
    // Menu Listbox events
    m_listbox_menu->subscribeEvent(CEGUI::Window::EventMouseEnters, CEGUI::Event::Subscriber(&cEditor::Editor_Mouse_Enter, this));
    m_listbox_menu->subscribeEvent(CEGUI::Listbox::EventSelectionChanged, CEGUI::Event::Subscriber(&cEditor::Menu_Select, this));
    // Get Items Listbox
    m_listbox_items = static_cast<CEGUI::Listbox*>(CEGUI::WindowManager::getSingleton().getWindow("editor_items"));
    // Items Listbox events
    m_listbox_items->subscribeEvent(CEGUI::Window::EventMouseEnters, CEGUI::Event::Subscriber(&cEditor::Editor_Mouse_Enter, this));
    m_listbox_items->subscribeEvent(CEGUI::Listbox::EventSelectionChanged, CEGUI::Event::Subscriber(&cEditor::Item_Select, this));

    // Get Items
    if (!File_Exists(m_items_filename)) {
        std::cerr << "Error : Editor Loading : No Item file found: " << path_to_utf8(m_items_filename) << std::endl;
        return;
    }

    Parse_Items_File(path_to_utf8(m_items_filename));
    Load_Image_Items(pResource_Manager->Get_Game_Pixmaps_Directory());
    Parse_Menu_File(path_to_utf8(m_menu_filename));
}

void cEditor::Unload(void)
{
    // Unload Items
    Unload_Item_Menu();

    // close help window
    if (CEGUI::WindowManager::getSingleton().isWindowPresent("editor_help_window")) {
        Window_Help_Exit_Clicked(CEGUI::EventArgs());
    }

    // if editor window is loaded
    if (m_editor_window) {
        pGuiSystem->getGUISheet()->removeChildWindow(m_editor_window);
        CEGUI::WindowManager::getSingleton().destroyWindow(m_editor_window);
        m_editor_window = NULL;
        m_listbox_menu = NULL;
        m_listbox_items = NULL;
        m_tabcontrol_menu = NULL;
    }

    // Tagged Items
    for (TaggedItemObjectsList::iterator itr = m_tagged_item_objects.begin(); itr != m_tagged_item_objects.end(); ++itr) {
        delete *itr;
    }

    m_tagged_item_objects.clear();

    // Tagged Image Settings
    for (TaggedItemImageSettingsList::iterator itr = m_tagged_item_images.begin(); itr != m_tagged_item_images.end(); ++itr) {
        delete *itr;
    }

    m_tagged_item_images.clear();
}

void cEditor::Toggle(void)
{
    // enable
    if (!m_enabled) {
        Enable();
    }
    // disable
    else {
        Disable();
    }
}

void cEditor::Enable(void)
{
    // already enabled
    if (m_enabled) {
        return;
    }

    // TRANS: displayed to the user when opening the editor
    Draw_Static_Text(_("Loading"), &orange, NULL, 0);

    // Basic Initialize
    Init();

    pAudio->Play_Sound("editor/enter.ogg");
    pHud_Debug->Set_Text(_("Editor enabled"));
    pActive_Animation_Manager->Delete_All();

    pMouseCursor->Set_Active(1);

    // update player position rect
    pActive_Player->Update_Position_Rect();
    // update sprite manager position rect
    for (cSprite_List::iterator itr = m_sprite_manager->objects.begin(); itr != m_sprite_manager->objects.end(); ++itr) {
        (*itr)->Update_Position_Rect();
    }

    pActive_Camera->Update_Position();
    m_enabled = 1;
}

void cEditor::Disable(bool native_mode /* = 1 */)
{
    // already disabled
    if (!m_enabled) {
        return;
    }

    Unload();

    m_enabled = 0;

    if (native_mode) {
        pAudio->Play_Sound("editor/leave.ogg");

        // player
        pActive_Player->Update_Position_Rect();
        // sprite manager
        for (cSprite_List::iterator itr = m_sprite_manager->objects.begin(); itr != m_sprite_manager->objects.end(); ++itr) {
            (*itr)->Update_Position_Rect();
        }

        pActive_Camera->Center();
        pMouseCursor->Reset(0);

        // ask if editor should save
        Function_Save(1);

        pMouseCursor->Set_Active(0);
    }
}

void cEditor::Update(void)
{
    if (!m_enabled) {
        return;
    }

    // if visible
    if (m_listbox_menu->isVisible(1)) {
        // if timed out
        if (m_menu_timer >= speedfactor_fps * 2) {
            // fade out
            float new_alpha = m_editor_window->getAlpha() - (pFramerate->m_speed_factor * 0.05f);

            if (new_alpha <= 0.0f) {
                new_alpha = 1.0f;

                /* move editor window out
                 * fixme: this could fire an mouse enter event if mouse is over it
                */
                m_editor_window->setXPosition(CEGUI::UDim(-0.19f, 0.0f));
                // Hide Listbox
                m_listbox_menu->hide();
                m_listbox_items->hide();
                m_menu_timer = 0.0f;
            }

            m_editor_window->setAlpha(new_alpha);
        }
        // if active
        else {
            // fade in
            if (m_editor_window->getAlpha() < 1.0f) {
                float new_alpha = m_editor_window->getAlpha() + pFramerate->m_speed_factor * 0.1f;

                if (new_alpha > 1.0f) {
                    new_alpha = 1.0f;
                }

                m_editor_window->setAlpha(new_alpha);
            }
            // inactive counter
            else if (Is_Float_Equal(m_editor_window->getXPosition().asRelative(1), 0.0f)) {
                // if mouse is over the window
                if (m_tabcontrol_menu->isHit(CEGUI::MouseCursor::getSingletonPtr()->getPosition())) {
                    m_menu_timer = 0.0f;
                }
                // inactive
                else {
                    m_menu_timer += pFramerate->m_speed_factor;
                }
            }
        }
    }

    pMouseCursor->Editor_Update();
}

void cEditor::Draw(void)
{
    if (!m_enabled) {
        return;
    }

    const float camera_top = pActive_Camera->m_y;
    const float camera_bottom = pActive_Camera->m_y + game_res_h;
    const float camera_left = pActive_Camera->m_x;
    const float camera_right = pActive_Camera->m_x + game_res_w;

    Color color;

    // Camera limit bottom line
    if (camera_bottom > pActive_Camera->m_limit_rect.m_y && camera_top < pActive_Camera->m_limit_rect.m_y) {
        float start_x = 0.0f;

        if (pActive_Camera->m_x < 0.0f) {
            start_x = -pActive_Camera->m_x;
        }

        color = Color(static_cast<Uint8>(0), 0, 100, 192);
        pVideo->Draw_Line(start_x, -pActive_Camera->m_y, static_cast<float>(game_res_w), -pActive_Camera->m_y, 0.124f, &color);
    }
    // Camera limit top line
    if (camera_bottom > pActive_Camera->m_limit_rect.m_y + pActive_Camera->m_limit_rect.m_h && camera_top < pActive_Camera->m_limit_rect.m_y + pActive_Camera->m_limit_rect.m_h) {
        float start_x = 0.0f;

        if (pActive_Camera->m_x < pActive_Camera->m_limit_rect.m_x) {
            start_x = -pActive_Camera->m_x;
        }

        color = Color(static_cast<Uint8>(20), 20, 150, 192);
        pVideo->Draw_Line(start_x, pActive_Camera->m_limit_rect.m_y + pActive_Camera->m_limit_rect.m_h - pActive_Camera->m_y, static_cast<float>(game_res_w), pActive_Camera->m_limit_rect.m_y + pActive_Camera->m_limit_rect.m_h - pActive_Camera->m_y, 0.124f, &color);
    }

    // Camera limit left line
    if (camera_left < pActive_Camera->m_limit_rect.m_x && camera_right > pActive_Camera->m_limit_rect.m_x) {
        float start_y = static_cast<float>(game_res_h);

        if (pActive_Camera->m_y < game_res_h) {
            start_y = game_res_h - pActive_Camera->m_y;
        }

        color = Color(static_cast<Uint8>(0), 100, 0, 192);
        pVideo->Draw_Line(pActive_Camera->m_limit_rect.m_x - pActive_Camera->m_x, start_y, -pActive_Camera->m_x, 0, 0.124f, &color);
    }
    // Camera limit right line
    if (camera_left < pActive_Camera->m_limit_rect.m_x + pActive_Camera->m_limit_rect.m_w && camera_right > pActive_Camera->m_limit_rect.m_x + pActive_Camera->m_limit_rect.m_w) {
        float start_y = static_cast<float>(game_res_h);

        if (pActive_Camera->m_y < game_res_h) {
            start_y = game_res_h - pActive_Camera->m_y;
        }

        color = Color(static_cast<Uint8>(20), 150, 20, 192);
        pVideo->Draw_Line(pActive_Camera->m_limit_rect.m_x + pActive_Camera->m_limit_rect.m_w - pActive_Camera->m_x, start_y, pActive_Camera->m_limit_rect.m_x + pActive_Camera->m_limit_rect.m_w - pActive_Camera->m_x, 0, 0.124f, &color);
    }
}

void cEditor::Process_Input(void)
{
    if (!m_enabled) {
        return;
    }

    // Drag Delete
    if (pKeyboard->Is_Ctrl_Down() && pMouseCursor->m_right) {
        cObjectCollision* col = pMouseCursor->Get_First_Editor_Collsion();

        if (col) {
            pMouseCursor->Delete(col->m_obj);
            delete col;
        }
    }

    // Camera Movement
    if (pKeyboard->m_keys[SDLK_RIGHT] || pJoystick->m_right) {
        if (pKeyboard->Is_Shift_Down()) {
            pActive_Camera->Move(m_camera_speed * pFramerate->m_speed_factor * 3 * pPreferences->m_scroll_speed, 0.0f);
        }
        else {
            pActive_Camera->Move(m_camera_speed * pFramerate->m_speed_factor * pPreferences->m_scroll_speed, 0.0f);
        }
    }
    else if (pKeyboard->m_keys[SDLK_LEFT] || pJoystick->m_left) {
        if (pKeyboard->Is_Shift_Down()) {
            pActive_Camera->Move(-(m_camera_speed * pFramerate->m_speed_factor * 3 * pPreferences->m_scroll_speed), 0.0f);
        }
        else {
            pActive_Camera->Move(-(m_camera_speed * pFramerate->m_speed_factor * pPreferences->m_scroll_speed), 0.0f);
        }
    }
    if (pKeyboard->m_keys[SDLK_UP] || pJoystick->m_up) {
        if (pKeyboard->Is_Shift_Down()) {
            pActive_Camera->Move(0.0f, -(m_camera_speed * pFramerate->m_speed_factor * 3 * pPreferences->m_scroll_speed));
        }
        else {
            pActive_Camera->Move(0.0f, -(m_camera_speed * pFramerate->m_speed_factor * pPreferences->m_scroll_speed));
        }
    }
    else if (pKeyboard->m_keys[SDLK_DOWN] || pJoystick->m_down) {
        if (pKeyboard->Is_Shift_Down()) {
            pActive_Camera->Move(0.0f, m_camera_speed * pFramerate->m_speed_factor * 3 * pPreferences->m_scroll_speed);
        }
        else {
            pActive_Camera->Move(0.0f, m_camera_speed * pFramerate->m_speed_factor * pPreferences->m_scroll_speed);
        }
    }
}

bool cEditor::Handle_Event(SDL_Event* ev)
{
    if (!m_enabled) {
        return 0;
    }

    switch (ev->type) {
    case SDL_MOUSEMOTION: {
        if (pMouseCursor->m_mover_mode) {
            pMouseCursor->Mover_Update(ev->motion.xrel, ev->motion.yrel);
        }

        break;
    }
    default: { // other events
        break;
    }
    }

    return 0;
}

bool cEditor::Key_Down(SDLKey key)
{
    if (!m_enabled) {
        return 0;
    }

    // New level
    if (key == SDLK_n && pKeyboard->Is_Ctrl_Down()) {
        Function_New();
    }
    // Save
    else if (key == SDLK_s && pKeyboard->Is_Ctrl_Down()) {
        Function_Save();
    }
    // Save as
    else if (key == SDLK_s && pKeyboard->Is_Ctrl_Down() && pKeyboard->Is_Shift_Down()) {
        Function_Save_as();
    }
    // help
    else if (key == SDLK_F1) {
        if (CEGUI::WindowManager::getSingleton().isWindowPresent("editor_help_window")) {
            Window_Help_Exit_Clicked(CEGUI::EventArgs());
        }
        else {
            CEGUI::FrameWindow* window_help = static_cast<CEGUI::FrameWindow*>(CEGUI::WindowManager::getSingleton().createWindow("TaharezLook/FrameWindow", "editor_help_window"));
            window_help->setPosition(CEGUI::UVector2(CEGUI::UDim(0, (game_res_w * 0.1f) * global_upscalex), CEGUI::UDim(0, (game_res_h * 0.1f) * global_upscalex)));
            window_help->setSize(CEGUI::UVector2(CEGUI::UDim(0, (game_res_w * 0.8f) * global_upscalex), CEGUI::UDim(0, (game_res_h * 0.8f) * global_upscalex)));
            window_help->getCloseButton()->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&cEditor::Window_Help_Exit_Clicked, this));
            // TRANS: Title of the help window
            window_help->setText(UTF8_("Editor Help"));

            CEGUI::Window* text_help = CEGUI::WindowManager::getSingleton().createWindow("TaharezLook/StaticText", "editor_help_text");
            text_help->setPosition(CEGUI::UVector2(CEGUI::UDim(0.00f, 0.0f), CEGUI::UDim(0.00f, 0.0f)));
            text_help->setSize(CEGUI::UVector2(CEGUI::UDim(1, 0.0f), CEGUI::UDim(1, 0.0f)));
            text_help->setProperty("VertScrollbar", "True");
            text_help->setProperty("FrameEnabled", "False");
            text_help->setProperty("BackgroundEnabled", "False");
            text_help->setProperty("VertFormatting", "TopAligned");

            // TRANS: This is the help window in the editor. Do not translate
            // TRANS: the color codes in the string.
            text_help->setText(UTF8_(" \n"
                               "----- [colour='FFFFCF5F']General[colour='FFFFFFFF'] -----\n"
                               " \n"
                               "F1 - Toggle this Help Window\n"
                               "F8 - Open/Close the Editor\n"
                               "F10 - Toggle sound effects\n"
                               "F11 - Toggle music play\n"
                               "Home - Focus level start\n"
                               "End - Focus last level exit\n"
                               "Ctrl + G - Goto Camera position\n"
                               "N - Step one screen to the right ( Next Screen )\n"
                               "P - Step one screen to the left ( Previous Screen )\n"
                               "Ctrl + N - Create a new Level\n"
                               "Ctrl + L - Load a Level\n"
                               "Ctrl + W - Load an Overworld\n"
                               "Ctrl + S - Save the current Level/World\n"
                               "Ctrl + Shift + S - Save the current Level/World under a new name\n"
                               "Ctrl + D - Toggle debug mode\n"
                               "Ctrl + P - Toggle performance mode\n"
                               " \n"
                               "----- [colour='FFFFCF5F']Objects[colour='FFFFFFFF'] -----\n"
                               " \n"
                               "M - Cycle selected object(s) through massive types\n"
                               "Massive types with their color:\n"
                               "   [colour='FFFF2F0F']Massive[colour='FFFFFFFF']->[colour='FFFFAF2F']Halfmassive[colour='FFFFFFFF']->[colour='FFDF00FF']Climbable[colour='FFFFFFFF']->[colour='FF1FFF1F']Passive[colour='FFFFFFFF']->[colour='FF1FFF1F']Front Passive[colour='FFFFFFFF']\n"
                               "O - Enable snap to object mode\n"
                               "Ctrl + A - Select all objects\n"
                               "Ctrl + X - Cut currently selected objects\n"
                               "Ctrl + C - Copy currently selected objects\n"
                               "Ctrl + V or Insert - Paste current copied / cutted objects\n"
                               "Ctrl + R - Replace the selected basic sprite(s) image with another one\n"
                               "Del - If Mouse is over an object: Delete current object\n"
                               "Del - If Mouse has nothing selected: Delete selected objects\n"
                               "Numpad:\n"
                               " + - Bring object to front\n"
                               " - - Send object to back\n"
                               " 2/4/6/8 - Move selected object 1 pixel into the direction\n"
                               "Mouse:\n"
                               " Left (Hold) - Drag objects\n"
                               " Left (Click) - With shift to select / deselect single objects\n"
                               " Right - Delete intersecting Object\n"
                               " Middle - Toggle Mover Mode\n"
                               " Ctrl + Shift + Left (Click) - Select objects with the same type\n"
                               "Arrow keys:\n"
                               " Use arrow keys to move around. Press shift for faster movement\n"
                               " \n")
                              );

            CEGUI::Window* guisheet = pGuiSystem->getGUISheet();
            window_help->addChildWindow(text_help);
            guisheet->addChildWindow(window_help);
        }
    }
    // focus level start
    else if (key == SDLK_HOME) {
        pActive_Camera->Reset_Pos();
    }
    // move camera one screen to the right
    else if (key == SDLK_n) {
        pActive_Camera->Move(static_cast<float>(game_res_w), 0);
    }
    // move camera one screen to the left
    else if (key == SDLK_p) {
        pActive_Camera->Move(-static_cast<float>(game_res_w), 0);
    }
    // move camera to position
    else if (key == SDLK_g && pKeyboard->Is_Ctrl_Down()) {
        int pos_x = string_to_int(Box_Text_Input(int_to_string(static_cast<int>(pActive_Camera->m_x)), "Position X", 1));
        int pos_y = string_to_int(Box_Text_Input(int_to_string(static_cast<int>(pActive_Camera->m_y)), "Position Y", 1));

        pActive_Camera->Set_Pos(static_cast<float>(pos_x), static_cast<float>(pos_y));
    }
    // push selected objects into the front
    else if (key == SDLK_KP_PLUS) {
        for (SelectedObjectList::iterator itr = pMouseCursor->m_selected_objects.begin(); itr != pMouseCursor->m_selected_objects.end(); ++itr) {
            cSelectedObject* sel_obj = (*itr);

            if (!sel_obj->m_obj->Is_Sprite_Managed()) {
                continue;
            }

            // last object is in front of others
            m_sprite_manager->Move_To_Back(sel_obj->m_obj);
        }
    }
    // push selected objects into the back
    else if (key == SDLK_KP_MINUS) {
        for (SelectedObjectList::iterator itr = pMouseCursor->m_selected_objects.begin(); itr != pMouseCursor->m_selected_objects.end(); ++itr) {
            cSelectedObject* sel_obj = (*itr);

            if (!sel_obj->m_obj->Is_Sprite_Managed()) {
                continue;
            }

            // first object is behind others
            m_sprite_manager->Move_To_Front(sel_obj->m_obj);
        }
    }
    // copy into direction
    else if ((key == pPreferences->m_key_editor_fast_copy_up || key == pPreferences->m_key_editor_fast_copy_down || key == pPreferences->m_key_editor_fast_copy_left || key == pPreferences->m_key_editor_fast_copy_right) && pMouseCursor->m_hovering_object->m_obj && pMouseCursor->m_fastcopy_mode) {
        ObjectDirection dir = DIR_UNDEFINED;

        if (key == pPreferences->m_key_editor_fast_copy_up) {
            dir = DIR_UP;
        }
        else if (key == pPreferences->m_key_editor_fast_copy_down) {
            dir = DIR_DOWN;
        }
        else if (key == pPreferences->m_key_editor_fast_copy_left) {
            dir = DIR_LEFT;
        }
        else if (key == pPreferences->m_key_editor_fast_copy_right) {
            dir = DIR_RIGHT;
        }

        // get currently selected objects
        cSprite_List objects = pMouseCursor->Get_Selected_Objects();
        // copy objects
        cSprite_List new_objects = Copy_Direction(objects, dir);

        // add new objects
        for (cSprite_List::iterator itr = new_objects.begin(); itr != new_objects.end(); ++itr) {
            cSprite* obj = (*itr);

            pMouseCursor->Add_Selected_Object(obj, 1);
        }

        // deselect old objects
        for (cSprite_List::const_iterator itr = objects.begin(); itr != objects.end(); ++itr) {
            const cSprite* obj = (*itr);

            pMouseCursor->Remove_Selected_Object(obj);
        }
    }
    // Precise Pixel-Positioning
    else if ((key == pPreferences->m_key_editor_pixel_move_up || key == pPreferences->m_key_editor_pixel_move_down || key == pPreferences->m_key_editor_pixel_move_left || key == pPreferences->m_key_editor_pixel_move_right) && pMouseCursor->m_hovering_object->m_obj) {
        if (key == pPreferences->m_key_editor_pixel_move_up) {
            pActive_Camera->Move(0, -1);
        }
        else if (key == pPreferences->m_key_editor_pixel_move_down) {
            pActive_Camera->Move(0, 1);
        }
        else if (key == pPreferences->m_key_editor_pixel_move_left) {
            pActive_Camera->Move(-1, 0);
        }
        else if (key == pPreferences->m_key_editor_pixel_move_right) {
            pActive_Camera->Move(1, 0);
        }
    }
    // deselect everything
    else if (key == SDLK_a && pKeyboard->Is_Ctrl_Down() && pKeyboard->Is_Shift_Down()) {
        pMouseCursor->Clear_Selected_Objects();
    }
    // select everything
    else if (key == SDLK_a && pKeyboard->Is_Ctrl_Down()) {
        pMouseCursor->Clear_Selected_Objects();

        // player
        pMouseCursor->Add_Selected_Object(pActive_Player, 1);
        // sprite manager
        for (cSprite_List::iterator itr = m_sprite_manager->objects.begin(); itr != m_sprite_manager->objects.end(); ++itr) {
            cSprite* obj = (*itr);

            pMouseCursor->Add_Selected_Object(obj, 1);
        }
    }
    // Paste copy buffer objects
    else if ((key == SDLK_INSERT || key == SDLK_v) && pKeyboard->Is_Ctrl_Down()) {
        pMouseCursor->Paste_Copy_Objects(static_cast<float>(static_cast<int>(pMouseCursor->m_pos_x)), static_cast<float>(static_cast<int>(pMouseCursor->m_pos_y)));
    }
    // Cut selected Sprites to the copy buffer
    else if (key == SDLK_x && pKeyboard->Is_Ctrl_Down()) {
        pMouseCursor->Clear_Copy_Objects();

        for (SelectedObjectList::iterator itr = pMouseCursor->m_selected_objects.begin(); itr != pMouseCursor->m_selected_objects.end(); ++itr) {
            cSelectedObject* sel_obj = (*itr);

            pMouseCursor->Add_Copy_Object(sel_obj->m_obj);
        }

        pMouseCursor->Delete_Selected_Objects();
    }
    // Add selected Sprites to the copy buffer
    else if (key == SDLK_c && pKeyboard->Is_Ctrl_Down()) {
        pMouseCursor->Clear_Copy_Objects();

        for (SelectedObjectList::iterator itr = pMouseCursor->m_selected_objects.begin(); itr != pMouseCursor->m_selected_objects.end(); ++itr) {
            cSelectedObject* sel_obj = (*itr);

            pMouseCursor->Add_Copy_Object(sel_obj->m_obj);
        }
    }
    // Replace sprites
    else if (key == SDLK_r && pKeyboard->Is_Ctrl_Down()) {
        Replace_Sprites();
    }
    // Delete mouse object
    else if (key == SDLK_DELETE && pMouseCursor->m_hovering_object->m_obj) {
        pMouseCursor->Delete(pMouseCursor->m_hovering_object->m_obj);
    }
    // if shift got pressed remove mouse object for possible mouse selection
    else if (pKeyboard->Is_Shift_Down() && pMouseCursor->m_hovering_object->m_obj) {
        pMouseCursor->Clear_Hovered_Object();
    }
    // Delete selected objects
    else if (key == SDLK_DELETE) {
        pMouseCursor->Delete_Selected_Objects();
    }
    // Snap to objects mode
    else if (key == SDLK_o) {
        pMouseCursor->Toggle_Snap_Mode();
    }
    else {
        // not processed
        return 0;
    }

    // key got processed
    return 1;
}

bool cEditor::Mouse_Down(Uint8 button)
{
    if (!m_enabled) {
        return 0;
    }

    // left
    if (button == SDL_BUTTON_LEFT) {
        pMouseCursor->Left_Click_Down();

        // auto hide if enabled
        if (pMouseCursor->m_hovering_object->m_obj && pPreferences->m_editor_mouse_auto_hide) {
            pMouseCursor->Set_Active(0);
        }
    }
    // middle
    else if (button == SDL_BUTTON_MIDDLE) {
        // Activate fast copy mode
        if (pMouseCursor->m_hovering_object->m_obj) {
            pMouseCursor->m_fastcopy_mode = 1;
            return 1;
        }
        // Mover mode
        else {
            pMouseCursor->Toggle_Mover_Mode();
            return 1;
        }
    }
    // right
    else if (button == SDL_BUTTON_RIGHT) {
        if (!pMouseCursor->m_left) {
            pMouseCursor->Delete(pMouseCursor->m_hovering_object->m_obj);
            return 1;
        }
    }
    else {
        // not processed
        return 0;
    }

    // button got processed
    return 1;
}

bool cEditor::Mouse_Up(Uint8 button)
{
    if (!m_enabled) {
        return 0;
    }

    // left
    if (button == SDL_BUTTON_LEFT) {
        // unhide
        if (pPreferences->m_editor_mouse_auto_hide) {
            pMouseCursor->Set_Active(1);
        }

        pMouseCursor->End_Selection();

        if (pMouseCursor->m_hovering_object->m_obj) {
            for (SelectedObjectList::iterator itr = pMouseCursor->m_selected_objects.begin(); itr != pMouseCursor->m_selected_objects.end(); ++itr) {
                cSelectedObject* object = (*itr);

                // pre-update to keep particles on the correct position
                if (object->m_obj->m_type == TYPE_PARTICLE_EMITTER) {
                    cParticle_Emitter* emitter = static_cast<cParticle_Emitter*>(object->m_obj);
                    emitter->Pre_Update();
                }
            }
        }
    }
    // middle
    else if (button == SDL_BUTTON_MIDDLE) {
        pMouseCursor->m_fastcopy_mode = 0;
    }
    else {
        // not processed
        return 0;
    }

    // button got processed
    return 1;
}

void cEditor::Set_Sprite_Manager(cSprite_Manager* sprite_manager)
{
    m_sprite_manager = sprite_manager;
}

void cEditor::Add_Menu_Object(const std::string& name, std::string tags, CEGUI::colour normal_color /* = CEGUI::colour( 1, 1, 1 ) */)
{
    // Create Menu Object
    cEditor_Menu_Object* new_menu = new cEditor_Menu_Object(name);

    // if function
    if (tags.find("function:") == 0) {
        new_menu->bfunction = 1;
        // cut out the function identifier
        tags.erase(0, 9);
    }

    // if header
    if (tags.find("header") == 0) {
        new_menu->header = 1;
        // cut out the function identifier
        tags.erase(0, 6);

        // header color rect
        new_menu->setTextColours(normal_color, normal_color, CEGUI::colour(0.5f, 0.5f, 0.5f), CEGUI::colour(0.5f, 0.5f, 0.5f));
        // not selectable
        new_menu->setDisabled(1);
        // set tooltip
        new_menu->setTooltipText("Header " + name);
    }
    // if not a header
    else {
        new_menu->setTextColours(normal_color);
    }

    // if default items menu
    if (!new_menu->bfunction && !new_menu->header) {
        // set tooltip
        new_menu->setTooltipText("Tags used " + tags);
    }

    new_menu->tags = tags;
    new_menu->Init();

    // Add Listbox item
    m_listbox_menu->addItem(new_menu);
}

void cEditor::Activate_Menu_Item(cEditor_Menu_Object* entry)
{
    // Function
    if (entry->bfunction) {
        if (entry->tags.compare("exit") == 0) {
            Function_Exit();
        }
        else {
            printf("Unknown Function %s\n", entry->tags.c_str());
        }
    }
    // Header
    else if (entry->header) {
        return;
    }
    // Item Menu
    else {
        if (Load_Item_Menu(entry->tags)) {
            // Select Items Tab
            m_tabcontrol_menu->setSelectedTab("editor_tab_items");
        }
        // failed
        else {
            printf("Unknown Menu Type %s\n", entry->tags.c_str());
        }
    }
}

bool cEditor::Load_Item_Menu(std::string item_tags)
{
    if (item_tags.empty()) {
        return 0;
    }

    Unload_Item_Menu();

    // Convert to Array Tags
    vector<std::string> array_tags;

    // Convert
    while (item_tags.length()) {
        std::string::size_type pos = item_tags.find(";");

        // last item
        if (pos == std::string::npos) {
            array_tags.push_back(item_tags);
            item_tags.clear();
            break;
        }

        // add tag
        array_tags.push_back(item_tags.substr(0, pos));
        // remove tag
        item_tags.erase(0, pos + 1);
    }

    unsigned int tag_pos = 0;

    // Get all Images with the Tags
    for (TaggedItemImageSettingsList::const_iterator itr = m_tagged_item_images.begin(); itr != m_tagged_item_images.end(); ++itr) {
        const cImage_Settings_Data* settings = (*itr);

        // search
        while (Is_Tag_Available(settings->m_editor_tags, array_tags[tag_pos])) {
            tag_pos++;

            // found all tags
            if (tag_pos >= array_tags.size()) {
                cGL_Surface* image = pVideo->Get_Surface(settings->m_base);

                if (!image) {
                    printf("Warning : Could not load editor sprite image base : %s\n", settings->m_base.c_str());
                    break;
                }

                // Create sprite
                cSprite* new_sprite = new cSprite(m_sprite_manager);
                new_sprite->Set_Image(image);
                // default massivetype
                new_sprite->Set_Massive_Type(static_cast<MassiveType>(image->m_massive_type));
                // Add new Sprite
                Add_Item_Object(new_sprite);

                break;
            }
        }

        tag_pos = 0;
    }

    // Get all Objects with the Tags
    for (TaggedItemObjectsList::const_iterator itr = m_tagged_item_objects.begin(); itr != m_tagged_item_objects.end(); ++itr) {
        cSprite* object = (*itr);

        // search
        while (Is_Tag_Available(object->m_editor_tags, array_tags[tag_pos])) {
            tag_pos++;

            // found all tags
            if (tag_pos >= array_tags.size()) {
                // Add Objects
                Add_Item_Object(object->Copy());

                break;
            }
        }

        tag_pos = 0;
    }

    return 1;
}

void cEditor::Unload_Item_Menu(void)
{
    // already unloaded
    if (!CEGUI::WindowManager::getSingleton().isWindowPresent("editor_items")) {
        return;
    }

    // Clear Listbox
    if (m_listbox_items) {
        m_listbox_items->resetList();
    }
}


void cEditor::Add_Item_Object(cSprite* sprite, std::string new_name /* = "" */, cGL_Surface* image /* = NULL */)
{
    // if invalid
    if (!sprite) {
        printf("Warning : Invalid Editor Item\n");
        return;
    }

    // set correct array if not given
    if (sprite->m_sprite_array == ARRAY_UNDEFINED) {
        std::cerr << "Warning: Editor sprite '" << sprite->Create_Name() << "' array not set" << std::endl;

        if (sprite->m_massive_type == MASS_PASSIVE) {
            sprite->m_sprite_array = ARRAY_PASSIVE;
        }
        else if (sprite->m_massive_type == MASS_MASSIVE) {
            sprite->m_sprite_array = ARRAY_MASSIVE;
        }
        else if (sprite->m_massive_type == MASS_HALFMASSIVE) {
            sprite->m_sprite_array = ARRAY_ACTIVE;
        }
    }

    // warn if correct type is not given
    if (sprite->m_type == TYPE_UNDEFINED) {
        std::cerr << "Warning: Editor sprite '" << sprite->Create_Name() << "' type not set" << std::endl;
    }

    // if no image is given use the sprite start image
    if (!image) {
        // special object
        if (sprite->m_type == TYPE_ENEMY_STOPPER || sprite->m_type == TYPE_LEVEL_EXIT || sprite->m_type == TYPE_LEVEL_ENTRY || sprite->m_type == TYPE_SOUND || sprite->m_type == TYPE_ANIMATION || sprite->m_type == TYPE_PATH) {
            sprite->m_image = pVideo->Get_Surface("game/editor/special.png");
            sprite->m_start_image = sprite->m_image;
        }

        image = sprite->m_start_image;
    }

    // set object name
    std::string obj_name;

    if (new_name.length()) {
        obj_name = new_name;
    }
    else if (sprite->Create_Name().length()) {
        obj_name = sprite->Create_Name();
    }
    // no object name available
    else {
        if (image) {
            fs::path imgpath = image->Get_Path().filename();
            imgpath.replace_extension();
            obj_name = path_to_utf8(imgpath);
        }

        // Warn if using filename
        std::cerr << "Warning : editor object '" << obj_name << "' with no name given" << std::endl;
    }

    cEditor_Item_Object* new_item = new cEditor_Item_Object(obj_name, m_listbox_items);
    // Initialize
    new_item->Init(sprite);

    // Add Item
    m_listbox_items->addItem(new_item);
}

void cEditor::Load_Image_Items(fs::path dir)
{
    vector<fs::path> image_files = Get_Directory_Files(dir, ".settings");

    // load all available objects
    for (vector<fs::path>::const_iterator itr = image_files.begin(); itr != image_files.end(); ++itr) {
        // get filename
        fs::path filename = (*itr);

        // load settings
        cImage_Settings_Data* settings = pSettingsParser->Get(filename);

        // if settings are available
        if (settings) {
            // if required editor tag is available
            if (settings->m_editor_tags.find(m_editor_item_tag) != std::string::npos) {
                // set base to the filename
                settings->m_base = filename;
                // add real image
                m_tagged_item_images.push_back(settings);
            }
        }
    }
}

void cEditor::Activate_Item(cEditor_Item_Object* entry)
{
    // invalid
    if (!entry)
        throw(EditorError("Invalid Editor Item!"));

    // create copy from editor item
    cSprite* new_sprite = entry->sprite_obj->Copy();

    // if copying failed
    if (!new_sprite)
        throw(EditorSpriteCopyFailedError(entry->sprite_obj));

    new_sprite->Set_Sprite_Manager(m_sprite_manager);
    new_sprite->Set_Pos(pMouseCursor->m_pos_x, pMouseCursor->m_pos_y, 1);

    // hide editor window
    m_editor_window->setXPosition(CEGUI::UDim(-0.19f, 0));
    // Hide Listbox
    m_listbox_menu->hide();
    m_listbox_items->hide();

    // add item
    m_sprite_manager->Add(new_sprite);

    // Set mouse objects
    pMouseCursor->m_left = 1;
    pMouseCursor->m_hovering_object->m_mouse_offset_y = static_cast<int>(new_sprite->m_col_rect.m_h / 2);
    pMouseCursor->m_hovering_object->m_mouse_offset_x = static_cast<int>(new_sprite->m_col_rect.m_w / 2);
    pMouseCursor->Set_Hovered_Object(new_sprite);
}

cSprite_List cEditor::Copy_Direction(const cSprite_List& objects, const ObjectDirection dir) const
{
    // additional direction objects offset
    unsigned int offset = 0;

    // get the objects difference offset
    if (dir == DIR_LEFT || dir == DIR_RIGHT) {
        // first object
        const cSprite* first = objects[0];

        for (unsigned int i = 1; i < objects.size(); i++) {
            if (objects[i]->m_start_pos_x < first->m_start_pos_x) {
                first = objects[i];
            }
        }

        // last object
        const cSprite* last = objects[0];

        for (unsigned int i = 1; i < objects.size(); i++) {
            if (objects[i]->m_start_pos_x + objects[i]->m_start_rect.m_w > last->m_start_pos_x + last->m_start_rect.m_w) {
                last = objects[i];
            }
        }

        // Set X offset
        offset = static_cast<int>(last->m_start_pos_x - first->m_start_pos_x + last->m_start_rect.m_w);
    }
    else if (dir == DIR_UP || dir == DIR_DOWN) {
        // first object
        const cSprite* first = objects[0];

        for (unsigned int i = 1; i < objects.size(); i++) {
            if (objects[i]->m_start_pos_y < first->m_start_pos_y) {
                first = objects[i];
            }
        }

        // last object
        const cSprite* last = objects[0];

        for (unsigned int i = 1; i < objects.size(); i++) {
            if (objects[i]->m_start_pos_y + objects[i]->m_start_rect.m_h > last->m_start_pos_y + last->m_start_rect.m_h) {
                last = objects[i];
            }
        }

        // Set Y offset
        offset = static_cast<int>(last->m_start_pos_y - first->m_start_pos_y + last->m_start_rect.m_h);
    }

    // new copied objects
    cSprite_List new_objects;

    for (cSprite_List::const_iterator itr = objects.begin(); itr != objects.end(); ++itr) {
        const cSprite* obj = (*itr);

        new_objects.push_back(Copy_Direction(obj, dir, offset));
    }

    // return only new objects
    return new_objects;
}

cSprite* cEditor::Copy_Direction(const cSprite* obj, const ObjectDirection dir, int offset /* = 0 */) const
{
    float w = 0.0f;
    float h = 0.0f;

    if (dir == DIR_LEFT) {
        if (offset) {
            w = -static_cast<float>(offset);
        }
        else {
            w = -obj->m_start_rect.m_w;
        }
    }
    else if (dir == DIR_RIGHT) {
        if (offset) {
            w = static_cast<float>(offset);
        }
        else {
            w = obj->m_start_rect.m_w;
        }
    }
    else if (dir == DIR_UP) {
        if (offset) {
            h = -static_cast<float>(offset);
        }
        else {
            h = -obj->m_start_rect.m_h;
        }
    }
    else if (dir == DIR_DOWN) {
        if (offset) {
            h = static_cast<float>(offset);
        }
        else {
            h = obj->m_start_rect.m_h;
        }
    }

    // only move camera if obj is the mouse object
    if (pMouseCursor->m_hovering_object->m_obj == obj) {
        pActive_Camera->Move(w, h);
    }

    return pMouseCursor->Copy(obj, obj->m_start_pos_x + w, obj->m_start_pos_y + h);
}

void cEditor::Select_Same_Object_Types(const cSprite* obj)
{
    if (!obj) {
        return;
    }

    bool is_basic_sprite = 0;

    if (obj->Is_Basic_Sprite()) {
        if (!obj->m_start_image) {
            return;
        }

        is_basic_sprite = 1;
    }

    // sprite manager
    for (cSprite_List::iterator itr = m_sprite_manager->objects.begin(); itr != m_sprite_manager->objects.end(); ++itr) {
        cSprite* game_obj = (*itr);

        if (game_obj->m_type != obj->m_type) {
            continue;
        }

        if (is_basic_sprite && game_obj->m_start_image != obj->m_start_image) {
            continue;
        }

        pMouseCursor->Add_Selected_Object(game_obj, 1);
    }
}

bool cEditor::Editor_Mouse_Enter(const CEGUI::EventArgs& event)
{
    // ignore if a button is pressed
    if (pMouseCursor->m_left || pMouseCursor->m_middle || pMouseCursor->m_right) {
        return 1;
    }

    // if not visible
    if (!m_listbox_items->isVisible(1)) {
        // fade in
        m_editor_window->setXPosition(CEGUI::UDim(0, 0));
        m_editor_window->setAlpha(0.0f);

        // Show Listbox
        m_listbox_menu->show();
        m_listbox_items->show();
    }

    m_menu_timer = 0.0f;

    return 1;
}

bool cEditor::Menu_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Listbox*>(windowEventArgs.window)->getFirstSelectedItem();

    // set item
    if (item) {
        Activate_Menu_Item(static_cast<cEditor_Menu_Object*>(item));
    }
    // clear ?
    else {
        // todo : clear
    }

    return 1;
}

bool cEditor::Item_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Listbox*>(windowEventArgs.window)->getFirstSelectedItem();

    // activate item
    if (item) {
        Activate_Item(static_cast<cEditor_Item_Object*>(item));
    }

    return 1;
}

void cEditor::Function_Exit(void)
{
    pKeyboard->Key_Down(SDLK_F8);
}

void cEditor::Replace_Sprites(void)
{
    if (pMouseCursor->Get_Selected_Object_Size() == 0) {
        return;
    }

    if (!pMouseCursor->m_selected_objects[0]->m_obj->m_start_image) {
        return;
    }

    char i18nstr[256];
    sprintf(i18nstr, PL_("Change selected sprite's image", "Change selected %d sprites' images", pMouseCursor->Get_Selected_Object_Size()), pMouseCursor->Get_Selected_Object_Size());

    std::string image_filename = Box_Text_Input(path_to_utf8(pMouseCursor->m_selected_objects[0]->m_obj->m_start_image->m_path), i18nstr, 0);

    // aborted/invalid
    if (image_filename.empty()) {
        return;
    }

    cGL_Surface* image = pVideo->Get_Surface(image_filename);

    if (!image) {
        return;
    }

    for (SelectedObjectList::iterator itr = pMouseCursor->m_selected_objects.begin(); itr != pMouseCursor->m_selected_objects.end(); ++itr) {
        cSelectedObject* sel_obj = (*itr);

        // only sprites
        if (!sel_obj->m_obj->Is_Basic_Sprite()) {
            continue;
        }

        sel_obj->m_obj->Set_Image(image, 1);
    }
}

bool cEditor::Is_Tag_Available(const std::string& str, const std::string& tag, unsigned int search_pos /* = 0 */)
{
    // found tag position
    std::string::size_type pos = str.find(tag, search_pos);

    // not found
    if (pos == std::string::npos) {
        return 0;
    }

    // tag end position
    std::string::size_type end_pos = pos + tag.length();

    // if tag starting position is valid
    if (pos == 0 || str.substr(pos - 1, 1).compare(";") == 0) {
        // if tag ending position is valid
        if (end_pos == str.length() || str.substr(end_pos, 1).compare(";") == 0) {
            return 1;
        }
    }

    // not valid - continue search
    return Is_Tag_Available(str, tag, end_pos);
}

bool cEditor::Window_Help_Exit_Clicked(const CEGUI::EventArgs& event)
{
    CEGUI::Window* window_help = CEGUI::WindowManager::getSingleton().getWindow("editor_help_window");
    pGuiSystem->getGUISheet()->removeChildWindow(window_help);
    CEGUI::WindowManager::getSingleton().destroyWindow(window_help);

    return 1;
}

// virtual
// This function must set m_tagged_item_objects in the subclasses!
// Instanciate cEditorItemsLoader for parsing the items file.
void cEditor::Parse_Items_File(fs::path filename)
{
    throw (NotImplementedError("Parse_Items_File() must be overridden in a subclass."));
    //cEditorItemsLoader parser;
    //parser.parse_file(filename, cLevelLoader::Create_Level_Objects_From_XML_Tag);
    //m_tagged_item_objects = parser.get_tagged_sprites();
}

void cEditor::Parse_Menu_File(fs::path filename)
{
    // The menu XML file is so dead simple that a SAX parser would
    // simply be overkill. Leightweight XPath queries are enough.
    xmlpp::DomParser parser;
    parser.parse_file(path_to_utf8(filename));

    xmlpp::Element* p_root = parser.get_document()->get_root_node();
    xmlpp::NodeSet items = p_root->find("item");

    xmlpp::NodeSet::const_iterator iter;
    for (iter=items.begin(); iter != items.end(); iter++) {
        xmlpp::Element* p_node = dynamic_cast<xmlpp::Element*>(*iter);

        std::string name = dynamic_cast<xmlpp::Element*>(p_node->find("property[@name='name']")[0])->get_attribute("value")->get_value();
        std::string tags = dynamic_cast<xmlpp::Element*>(p_node->find("property[@name='tags']")[0])->get_attribute("value")->get_value();

        // Set color if available (---header--- elements have no color property)
        std::string color = "FFFFFFFF";
        xmlpp::NodeSet results = p_node->find("property[@name='color']");
        if (!results.empty())
            color = dynamic_cast<xmlpp::Element*>(results[0])->get_attribute("value")->get_value();

        Add_Menu_Object(name, tags, CEGUI::PropertyHelper::stringToColour(color));
    }
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

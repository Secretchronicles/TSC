#include "../global_basic.hpp"
#include "../game_core.hpp"
#include "../../gui/generic.hpp"
#include "../framerate.hpp"
#include "../../audio/audio.hpp"
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
#include "../filesystem/relative.hpp"
#include "../filesystem/resource_manager.hpp"
#include "../../video/img_settings.hpp"
#include "../../objects/level_exit.hpp"
#include "../../objects/level_entry.hpp"
#include "../errors.hpp"
#include "editor.hpp"

#define TABPANE_OUT_OF_SIGHT_X -0.19f
#define CONFIGPANE_OUT_OF_SIGHT_X 0.99f

using namespace TSC;

cEditor::cEditor()
{
    mp_editor_tabpane = NULL;
    mp_menu_listbox = NULL;
    mp_object_config_pane = NULL;
    mp_status_bar = NULL;
    m_enabled = false;
    m_rested = false;
    m_visibility_timer = 0.0f;
    m_mouse_inside = false;
    m_menu_filename = boost::filesystem::path(path_to_utf8("Needs to be set by subclasses"));
    m_editor_item_tag = "Must be set by subclass";
    m_help_window_visible = false;
    m_object_config_pane_shown = false;
    mp_edited_sprite_manager = NULL;
}

cEditor::~cEditor()
{
    Unload();
}

/**
 * Load the CEGUI layout from disk and attach it to the root window.
 * Does not show it, use Enable() for that.
 *
 * Override in subclasses to fill the editor pane with your custom
 * items. Be sure to call this parent method before doing so, though.
 */
void cEditor::Init(void)
{
    mp_editor_root = CEGUI::WindowManager::getSingleton().loadLayoutFromFile("editor.layout");
    mp_editor_tabpane = static_cast<CEGUI::TabControl*>(mp_editor_root->getChild("editor_tabpane"));
    mp_object_config_pane = mp_editor_root->getChild("object_config_pane");
    mp_status_bar = mp_editor_root->getChild("status_bar");
    m_tabpane_target_x_position = mp_editor_tabpane->getXPosition();
    m_object_config_pane_target_x_position = mp_object_config_pane->getXPosition();
    mp_editor_root->hide(); // Do not show for now

    // Object settings window only pops up when selecting an object.
    mp_object_config_pane->setXPosition(CEGUI::UDim(CONFIGPANE_OUT_OF_SIGHT_X, 0.0f));
    m_object_config_pane_shown = false;

    // Status bar
    mp_status_bar->setText("");

    // Ensure multiple editors (level and world) can coexist. CEGUI requires unique
    // window names in that case; m_editor_item_tag must be set by subclasses, so
    // it is a good fit for making a unique name.
    mp_editor_root->setName(m_editor_item_tag + "_editor_root");

    // Add it to the GUI root window
    CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->addChild(mp_editor_root);

    mp_menu_listbox = static_cast<CEGUI::Listbox*>(mp_editor_tabpane->getChild("editor_tab_menu/editor_menu"));
    mp_selector_radio_selection = static_cast<CEGUI::RadioButton*>(mp_editor_tabpane->getChild("editor_tab_selector/selection"));
    mp_selector_radio_secretarea = static_cast<CEGUI::RadioButton*>(mp_editor_tabpane->getChild("editor_tab_selector/secretarea"));
    mp_selector_radio_selection->setSelected(true);

    // Create the menu hierarchy
    parse_menu_file();
    populate_menu();

    /* Fill in the creatable objects for each menu entry; first the
     * image items (= static sprites with a .settings file), then the
     * special items (= everything else, such as enemies).  The
     * function menu entries are handled in the
     * on_menu_selection_changed() event handler function. */
    load_image_items();
    load_special_items();

    mp_editor_tabpane->subscribeEvent(CEGUI::Window::EventMouseEntersArea, CEGUI::Event::Subscriber(&cEditor::on_mouse_enter, this));
    mp_editor_tabpane->subscribeEvent(CEGUI::Window::EventMouseLeavesArea, CEGUI::Event::Subscriber(&cEditor::on_mouse_leave, this));

    mp_menu_listbox->subscribeEvent(CEGUI::Listbox::EventSelectionChanged, CEGUI::Event::Subscriber(&cEditor::on_menu_selection_changed, this));
    mp_selector_radio_selection->subscribeEvent(CEGUI::RadioButton::EventSelectStateChanged, CEGUI::Event::Subscriber(&cEditor::on_selector_radio_changed, this));
    mp_selector_radio_secretarea->subscribeEvent(CEGUI::RadioButton::EventSelectStateChanged, CEGUI::Event::Subscriber(&cEditor::on_selector_radio_changed, this));
}

/**
 * Empties the editor panel, detaches it from the CEGUI root window
 * and destroys it. After calling this you need to call Init()
 * again to use the editor.
 */
void cEditor::Unload(void)
{
    std::vector<cEditor_Menu_Entry*>::iterator iter;
    for(iter=m_menu_entries.begin(); iter != m_menu_entries.end(); iter++)
        delete *iter;

    if (mp_editor_root) {
        CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->removeChild(mp_editor_root);
        CEGUI::WindowManager::getSingleton().destroyWindow(mp_editor_root); // destroys child windows
        mp_editor_root = NULL;
        mp_editor_tabpane = NULL;
        mp_menu_listbox = NULL;
        mp_object_config_pane = NULL;
        mp_status_bar = NULL;
    }
}

void cEditor::Toggle(cSprite_Manager* p_sprite_manager)
{
    if (m_enabled)
        Disable();
    else
        Enable(p_sprite_manager);
}

void cEditor::Enable(cSprite_Manager* p_edited_sprite_manager)
{
    if (m_enabled)
        return;

    // TRANS: displayed to the user when opening the editor
    Draw_Static_Text(_("Loading"), &orange, NULL, 0);

    pAudio->Play_Sound("editor/enter.ogg");
    pMouseCursor->Set_Active(true);

    pActive_Animation_Manager->Delete_All(); // Stop all animations

    gp_hud->Hide();
    mp_editor_root->show();
    m_enabled = true;
    editor_enabled = true;
    mp_edited_sprite_manager = p_edited_sprite_manager;

    /* Update the position rects of all objects so they are positioned
     * properly in the editor. This must come after setting
     * editor_enabled so that Update_Position_Rect() can execute the
     * editor-specific update on the initial start position. */
    cSprite_List::iterator iter;
    for(iter=mp_edited_sprite_manager->objects.begin(); iter != mp_edited_sprite_manager->objects.end(); iter++) {
        cSprite*  p_sprite = *iter;
        p_sprite->Update_Position_Rect();
    }
}

void cEditor::Disable(void)
{
    if (!m_enabled)
        return;

    pAudio->Play_Sound("editor/leave.ogg");
    pMouseCursor->Reset(0);
    pMouseCursor->Set_Active(false);

    Hide_Config_Panel();
    gp_hud->Show();
    mp_editor_root->hide();
    m_enabled = false;
    editor_enabled = false;
    mp_edited_sprite_manager = NULL;
}

/**
 * Adds a widget to the editor's object configuration panel (that one on the right side).
 * If you pass a CEGUI::Combobox as a widget, ensure that all items are added to it
 * *before* you call this method, because this method uses the item count to calculate
 * the height of the combobox dropdown. If there are no items in the box, the dropdown
 * height will be zero, making the entire combobox invisible.
 *
 * Also, you should probably subscribe to the widget's change event handler so you
 * get noticed if the user interacts with the widget.
 *
 * The widget will be added together with a label to its left side whose text
 * is determined from the `name` parameter.
 *
 * \param[in] name
 * The text to display on the left-side label widget.
 *
 * \param[in] tooltip
 * A tooltip to display when hovering over the label.
 *
 * \param[in] p_settings_widget
 * The settings widget to add.
 *
 * \param obj_height
 * Height of the widget, defaulting to 28 pixels. There is usually no need to
 * specify this unless you have an unusually large widget. For the special
 * case of comboboxes, see the description above.
 */
void cEditor::Add_Config_Widget(const CEGUI::String& name, const CEGUI::String& tooltip, CEGUI::Window* p_settings_widget, float obj_height /* = 28.0f */)
{
    static const float labelheight = 28.0f;

    // The settings widget must at least have the height of the label widget.
    if (obj_height < labelheight) {
        obj_height = labelheight;
    }

    // Get scrollarea
    CEGUI::ScrollablePane* p_pane = static_cast<CEGUI::ScrollablePane*>(mp_object_config_pane->getChild("tab_config/object_config_scrollarea"));
    // get window manager
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    CEGUI::Window* p_label = wmgr.createWindow("TSCLook256/StaticText");
    p_label->setText(name);
    p_label->setTooltipText(tooltip);

    // set size
    p_label->setWidth(CEGUI::UDim(0.5f, 0));
    p_label->setHeight(CEGUI::UDim(0, labelheight * global_upscaley));
    p_settings_widget->setWidth(CEGUI::UDim(0.5f, 0));
    p_settings_widget->setHeight(CEGUI::UDim(0, obj_height * global_upscaley));

    /* Combo boxes treat their height differently than other
     * widgets. They use it for the height of the dropdown list, which
     * means that we cannot simply set them to the uniform height of
     * all other widgets, but must make them larger so that the
     * dropdown list actually appears. We set the height in
     * correspondance with the number of items in the list, assuming
     * one line is `labelheight' pixels (times `global_upscaley' as
     * always) high plus some guessing (* 3). */
    CEGUI::Combobox* p_combo = NULL;
    if ((p_combo = dynamic_cast<CEGUI::Combobox*>(p_settings_widget))) {
        p_combo->setHeight(CEGUI::UDim(0, p_combo->getItemCount() * 3 * labelheight * global_upscaley));
        //p_combo->setAutoSizeListHeightToContent(true); // While this would ease things, it does not work...
    }

    // Calculate position on the scrollarea
    if (p_pane->getContentPane()->getChildCount() > 0) { // Place below existing rows
        const CEGUI::Window* p_last_window = p_pane->getContentPane()->getChildAtIdx(p_pane->getContentPane()->getChildCount() - 1);
        CEGUI::UDim new_y = p_last_window->getYPosition();
        new_y += CEGUI::UDim(0, labelheight * global_upscaley);

        p_label->setPosition(CEGUI::UVector2(CEGUI::UDim(0, 0), new_y));
        p_settings_widget->setPosition(CEGUI::UVector2(CEGUI::UDim(0.5f, 0), new_y));
    }
    else { // First entry, starting at top of container
        p_label->setPosition(CEGUI::UVector2(CEGUI::UDim(0, 0), CEGUI::UDim(0, 0)));
        p_settings_widget->setPosition(CEGUI::UVector2(CEGUI::UDim(0.5, 0), CEGUI::UDim(0, 0)));
    }

    /* Add to scrollarea. The settings window must come last as it may
     * be higher then the label window and its height is used in the
     * position calculation above. */
    p_pane->addChild(p_label);
    p_pane->addChild(p_settings_widget);
}

void cEditor::Show_Config_Panel()
{
    // Already shown
    if (m_object_config_pane_shown)
        return;

    mp_object_config_pane->setXPosition(m_object_config_pane_target_x_position);
    m_object_config_pane_shown = true;

}

void cEditor::Hide_Config_Panel()
{
    // Already hidden
    if (!m_object_config_pane_shown)
        return;

    // Destroy all config widgets in the scrollarea
    CEGUI::ScrollablePane* p_pane = static_cast<CEGUI::ScrollablePane*>(mp_object_config_pane->getChild("tab_config/object_config_scrollarea"));
    while (p_pane->getContentPane()->getChildCount() > 0) {
        CEGUI::Window* p_window = p_pane->getContentPane()->getChildAtIdx(0);
        p_pane->removeChild(p_window);
        CEGUI::WindowManager::getSingleton().destroyWindow(p_window);
    }

    // Hide config panel
    mp_object_config_pane->setXPosition(CEGUI::UDim(CONFIGPANE_OUT_OF_SIGHT_X, 0.0f));
    m_object_config_pane_shown = false;
}

void cEditor::Update(void)
{
    if (!m_enabled)
        return;

    // If the mouse is in the editor panel, do not fade it out.
    if (!m_mouse_inside) {
        // Otherwise, slowly fade the panel out until it is invisible.
        // When it reaches transparency, set to fully visible again
        // and place it on the side.
        if (!m_rested) {
            float timeout = speedfactor_fps * 2;
            if (m_visibility_timer >= timeout) {
                mp_editor_tabpane->setXPosition(CEGUI::UDim(TABPANE_OUT_OF_SIGHT_X, 0.0f));
                mp_editor_tabpane->setAlpha(1.0f);

                m_rested = true;
                m_visibility_timer = 0.0f;
            }
            else {
                m_visibility_timer += pFramerate->m_speed_factor;
                float alpha_max = 1.0f;
                mp_editor_tabpane->setAlpha(alpha_max - ((alpha_max * m_visibility_timer) / timeout));
            }
        }
    }

    pMouseCursor->Editor_Update();
    Process_Input();
    update_status_bar();
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

        color = Color(static_cast<uint8_t>(0), 0, 100, 192);
        pVideo->Draw_Line(start_x, -pActive_Camera->m_y, static_cast<float>(game_res_w), -pActive_Camera->m_y, 0.124f, &color);
    }
    // Camera limit top line
    if (camera_bottom > pActive_Camera->m_limit_rect.m_y + pActive_Camera->m_limit_rect.m_h && camera_top < pActive_Camera->m_limit_rect.m_y + pActive_Camera->m_limit_rect.m_h) {
        float start_x = 0.0f;

        if (pActive_Camera->m_x < pActive_Camera->m_limit_rect.m_x) {
            start_x = -pActive_Camera->m_x;
        }

        color = Color(static_cast<uint8_t>(20), 20, 150, 192);
        pVideo->Draw_Line(start_x, pActive_Camera->m_limit_rect.m_y + pActive_Camera->m_limit_rect.m_h - pActive_Camera->m_y, static_cast<float>(game_res_w), pActive_Camera->m_limit_rect.m_y + pActive_Camera->m_limit_rect.m_h - pActive_Camera->m_y, 0.124f, &color);
    }

    // Camera limit left line
    if (camera_left < pActive_Camera->m_limit_rect.m_x && camera_right > pActive_Camera->m_limit_rect.m_x) {
        float start_y = static_cast<float>(game_res_h);

        if (pActive_Camera->m_y < game_res_h) {
            start_y = game_res_h - pActive_Camera->m_y;
        }

        color = Color(static_cast<uint8_t>(0), 100, 0, 192);
        pVideo->Draw_Line(pActive_Camera->m_limit_rect.m_x - pActive_Camera->m_x, start_y, -pActive_Camera->m_x, 0, 0.124f, &color);
    }
    // Camera limit right line
    if (camera_left < pActive_Camera->m_limit_rect.m_x + pActive_Camera->m_limit_rect.m_w && camera_right > pActive_Camera->m_limit_rect.m_x + pActive_Camera->m_limit_rect.m_w) {
        float start_y = static_cast<float>(game_res_h);

        if (pActive_Camera->m_y < game_res_h) {
            start_y = game_res_h - pActive_Camera->m_y;
        }

        color = Color(static_cast<uint8_t>(20), 150, 20, 192);
        pVideo->Draw_Line(pActive_Camera->m_limit_rect.m_x + pActive_Camera->m_limit_rect.m_w - pActive_Camera->m_x, start_y, pActive_Camera->m_limit_rect.m_x + pActive_Camera->m_limit_rect.m_w - pActive_Camera->m_x, 0, 0.124f, &color);
    }
}

bool cEditor::Key_Down(const sf::Event& evt)
{
    if (!m_enabled)
        return false;

    // Ignore key presses if the editor config pane is open
    if (m_object_config_pane_shown)
        return false;

    // New level
    if (evt.key.code == sf::Keyboard::N && evt.key.control) {
        Function_New();
    }
    // Save
    else if (evt.key.code == sf::Keyboard::S && evt.key.control) {
        Function_Save();
    }
    // Save as
    else if (evt.key.code == sf::Keyboard::S && evt.key.control && evt.key.shift) {
        Function_Save_as();
    }
    else if (evt.key.code == sf::Keyboard::F1) {
        if (m_help_window_visible) {
            on_help_window_exit_clicked(CEGUI::EventArgs());
        }
        else {
            CEGUI::FrameWindow* p_helpframe = static_cast<CEGUI::FrameWindow*>(CEGUI::WindowManager::getSingleton().createWindow("TSCLook256/FrameWindow", "editor_help"));
            p_helpframe->setPosition(CEGUI::UVector2(CEGUI::UDim(0, (game_res_w * 0.1f) * global_upscalex), CEGUI::UDim(0, (game_res_h * 0.1f) * global_upscalex)));
            p_helpframe->setSize(CEGUI::USize(CEGUI::UDim(0, (game_res_w * 0.8f) * global_upscalex), CEGUI::UDim(0, (game_res_h * 0.8f) * global_upscalex)));
            // TRANS: Title of the editor help window
            p_helpframe->setText(UTF8_("Editor Help"));

            p_helpframe->getCloseButton()->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&cEditor::on_help_window_exit_clicked, this));

            CEGUI::Window* p_helptext = CEGUI::WindowManager::getSingleton().createWindow("TSCLook256/StaticText", "editor_help_text");
            p_helptext->setPosition(CEGUI::UVector2(CEGUI::UDim(0.00f, 0.0f), CEGUI::UDim(0.00f, 0.0f)));
            p_helptext->setSize(CEGUI::USize(CEGUI::UDim(1, 0.0f), CEGUI::UDim(1, 0.0f)));
            p_helptext->setProperty("VertScrollbar", "True");
            p_helptext->setProperty("FrameEnabled", "False");
            p_helptext->setProperty("BackgroundEnabled", "False");
            p_helptext->setProperty("VertFormatting", "TopAligned");

            // TRANS: This is the help window in the editor. Do not translate
            // TRANS: the color codes in the string.
            p_helptext->setText(UTF8_(" \n"
                                      "----- [colour='FFFFCF5F']General[colour='FFFFFFFF'] -----\n"
                                      " \n"
                                      "F1 - Toggle this Help Window\n"
                                      "F8 - Open/Close the Editor\n"
                                      "F10 - Toggle sound effects\n"
                                      "F11 - Toggle music play\n"
                                      "Home - Focus level start\n"
                                      "End - Focus last level exit\n"
                                      "Ctrl + G - Goto Camera position\n"
                                      "L - In world editor: toggle walk line visibility\n"
                                      "N - Step one screen to the right ( Next Screen )\n"
                                      "P - Step one screen to the left ( Previous Screen )\n"
                                      "Ctrl + N - Create a new Level\n"
                                      "Ctrl + L - Load a Level\n"
                                      "Ctrl + W - Load an Overworld\n"
                                      "Ctrl + S - Save the current Level/World\n"
                                      "Ctrl + Shift + S - Save the current Level/World under a new name\n"
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

            CEGUI::Window* p_root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
            p_helpframe->addChild(p_helptext);
            p_root->addChild(p_helpframe);

            m_help_window_visible = true;
        }
    }
    // focus level start
    else if (evt.key.code == sf::Keyboard::Home) {
        pActive_Camera->Reset_Pos();
    }
    // move camera one screen to the right
    else if (evt.key.code == sf::Keyboard::N) {
        pActive_Camera->Move(static_cast<float>(game_res_w), 0);
    }
    // move camera one screen to the left
    else if (evt.key.code == sf::Keyboard::P) {
        pActive_Camera->Move(-static_cast<float>(game_res_w), 0);
    }
    // move camera to position
    else if (evt.key.code == sf::Keyboard::G && evt.key.control) {
        int pos_x = string_to_int(Box_Text_Input(int_to_string(static_cast<int>(pActive_Camera->m_x)), "Position X", 1));
        int pos_y = string_to_int(Box_Text_Input(int_to_string(static_cast<int>(pActive_Camera->m_y)), "Position Y", 1));

        pActive_Camera->Set_Pos(static_cast<float>(pos_x), static_cast<float>(pos_y));
    }
    // push selected objects into the front
    else if (evt.key.code == sf::Keyboard::Add) {
        for (SelectedObjectList::iterator itr = pMouseCursor->m_selected_objects.begin(); itr != pMouseCursor->m_selected_objects.end(); ++itr) {
            cSelectedObject* sel_obj = (*itr);

            if (!sel_obj->m_obj->Is_Sprite_Managed()) {
                continue;
            }

            // last object is in front of others
            mp_edited_sprite_manager->Move_To_Back(sel_obj->m_obj);
        }
    }
    // push selected objects into the back
    else if (evt.key.code == sf::Keyboard::Subtract) {
        for (SelectedObjectList::iterator itr = pMouseCursor->m_selected_objects.begin(); itr != pMouseCursor->m_selected_objects.end(); ++itr) {
            cSelectedObject* sel_obj = (*itr);

            if (!sel_obj->m_obj->Is_Sprite_Managed()) {
                continue;
            }

            // first object is behind others
            mp_edited_sprite_manager->Move_To_Front(sel_obj->m_obj);
        }
    }
    // copy into direction
    else if ((evt.key.code == pPreferences->m_key_editor_fast_copy_up || evt.key.code == pPreferences->m_key_editor_fast_copy_down || evt.key.code == pPreferences->m_key_editor_fast_copy_left || evt.key.code == pPreferences->m_key_editor_fast_copy_right) && pMouseCursor->m_hovering_object->m_obj && pMouseCursor->m_fastcopy_mode) {
        ObjectDirection dir = DIR_UNDEFINED;

        if (evt.key.code == pPreferences->m_key_editor_fast_copy_up) {
            dir = DIR_UP;
        }
        else if (evt.key.code == pPreferences->m_key_editor_fast_copy_down) {
            dir = DIR_DOWN;
        }
        else if (evt.key.code == pPreferences->m_key_editor_fast_copy_left) {
            dir = DIR_LEFT;
        }
        else if (evt.key.code == pPreferences->m_key_editor_fast_copy_right) {
            dir = DIR_RIGHT;
        }

        // get currently selected objects
        cSprite_List objects = pMouseCursor->Get_Selected_Objects();
        // copy objects
        cSprite_List new_objects = copy_direction(objects, dir);

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
    else if ((evt.key.code == pPreferences->m_key_editor_pixel_move_up || evt.key.code == pPreferences->m_key_editor_pixel_move_down || evt.key.code == pPreferences->m_key_editor_pixel_move_left || evt.key.code == pPreferences->m_key_editor_pixel_move_right)) {
        int x_offset = 0;
        int y_offset = 0;

        if (evt.key.code == pPreferences->m_key_editor_pixel_move_up) {
            y_offset = -1;
        }
        else if (evt.key.code == pPreferences->m_key_editor_pixel_move_down) {
            y_offset = 1;
        }
        else if (evt.key.code == pPreferences->m_key_editor_pixel_move_left) {
            x_offset = -1;
        }
        else if (evt.key.code == pPreferences->m_key_editor_pixel_move_right) {
            x_offset = 1;
        }

        for (SelectedObjectList::iterator itr = pMouseCursor->m_selected_objects.begin(); itr != pMouseCursor->m_selected_objects.end(); ++itr) {
            cSelectedObject* sel_obj = (*itr);
            cSprite* obj = sel_obj->m_obj;

            obj->Set_Pos(obj->m_pos_x + x_offset, obj->m_pos_y + y_offset, true);
        }
    }
    // deselect everything
    else if (evt.key.code == sf::Keyboard::A && evt.key.control && evt.key.shift) {
        pMouseCursor->Clear_Selected_Objects();
    }
    // select everything
    else if (evt.key.code == sf::Keyboard::A && evt.key.control) {
        pMouseCursor->Clear_Selected_Objects();

        // player
        pMouseCursor->Add_Selected_Object(pActive_Player, 1);
        // sprite manager
        for (cSprite_List::iterator itr = mp_edited_sprite_manager->objects.begin(); itr != mp_edited_sprite_manager->objects.end(); ++itr) {
            cSprite* obj = (*itr);

            pMouseCursor->Add_Selected_Object(obj, 1);
        }
    }
    // Paste copy buffer objects
    else if ((evt.key.code == sf::Keyboard::Insert || evt.key.code == sf::Keyboard::V) && evt.key.control) {
        pMouseCursor->Paste_Copy_Objects(static_cast<float>(static_cast<int>(pMouseCursor->m_pos_x)), static_cast<float>(static_cast<int>(pMouseCursor->m_pos_y)));
    }
    // Cut selected Sprites to the copy buffer
    else if (evt.key.code == sf::Keyboard::X && evt.key.control) {
        pMouseCursor->Clear_Copy_Objects();

        for (SelectedObjectList::iterator itr = pMouseCursor->m_selected_objects.begin(); itr != pMouseCursor->m_selected_objects.end(); ++itr) {
            cSelectedObject* sel_obj = (*itr);

            pMouseCursor->Add_Copy_Object(sel_obj->m_obj);
        }

        pMouseCursor->Delete_Selected_Objects();
    }
    // Add selected Sprites to the copy buffer
    else if (evt.key.code == sf::Keyboard::C && evt.key.control) {
        pMouseCursor->Clear_Copy_Objects();

        for (SelectedObjectList::iterator itr = pMouseCursor->m_selected_objects.begin(); itr != pMouseCursor->m_selected_objects.end(); ++itr) {
            cSelectedObject* sel_obj = (*itr);

            pMouseCursor->Add_Copy_Object(sel_obj->m_obj);
        }
    }
    // Replace sprites
    else if (evt.key.code == sf::Keyboard::R && evt.key.control) {
        replace_sprites();
    }
    // Delete mouse object
    else if (evt.key.code == sf::Keyboard::Delete && pMouseCursor->m_hovering_object->m_obj) {
        pMouseCursor->Delete(pMouseCursor->m_hovering_object->m_obj);
    }
    // if shift got pressed remove mouse object for possible mouse selection
    else if (evt.key.shift && pMouseCursor->m_hovering_object->m_obj) {
        pMouseCursor->Clear_Hovered_Object();
    }
    // Delete selected objects
    else if (evt.key.code == sf::Keyboard::Delete) {
        pMouseCursor->Delete_Selected_Objects();
    }
    // Snap to objects mode
    else if (evt.key.code == sf::Keyboard::O) {
        pMouseCursor->Toggle_Snap_Mode();
    }
    else {
        // not processed
        return false;
    }

    // key got processed
    return true;

}

bool cEditor::Mouse_Down(sf::Mouse::Button button)
{
    if (!m_enabled) {
        return 0;
    }

    // left
    if (button == sf::Mouse::Left) {
        pMouseCursor->Left_Click_Down();

        // auto hide if enabled
        if (pMouseCursor->m_hovering_object->m_obj && pPreferences->m_editor_mouse_auto_hide) {
            pMouseCursor->Set_Active(0);
        }
    }
    // middle
    else if (button == sf::Mouse::Middle) {
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
    else if (button == sf::Mouse::Right) {
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

bool cEditor::Mouse_Up(sf::Mouse::Button button)
{
    if (!m_enabled) {
        return 0;
    }

    // left
    if (button == sf::Mouse::Left) {
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
    else if (button == sf::Mouse::Middle) {
        pMouseCursor->m_fastcopy_mode = 0;
    }
    else {
        // not processed
        return 0;
    }

    // button got processed
    return 1;
}

bool cEditor::Mouse_Move(const sf::Event& evt)
{
    if (!m_enabled)
        return false;

    if (pMouseCursor->m_mover_mode) {
        sf::Vector2i pos = sf::Mouse::getPosition(*pVideo->mp_window);
        pMouseCursor->Mover_Update(pos.x, pos.y);
    }

    return false; // Bubble up
}

/**
 * Adds a graphic for a static .settings-file based object to the
 * editor menu. The settings file of this graphic will be parsed and
 * it will be placed in the menu accordingly (subclasses have to set
 * the `m_editor_item_tag` member variable the the master tag required
 * for graphics to show up in this editor; these are "level" and
 * "world" for the level and world editor subclasses,
 * respectively. That is, a graphic tagged with "world" will never
 * appear in the level editor, and vice-versa.).
 *
 * \param settings_path
 * Absolute path that refers to the settings file
 * of the graphic to add.
 *
 * \returns false if the item was not added because the master tag
 * was missing, true otherwise.
 */
bool cEditor::Try_Add_Image_Item(boost::filesystem::path settings_path)
{
    // Parse the image's settings file
    cImage_Settings_Parser parser;
    cImage_Settings_Data* p_settings = parser.Get(settings_path);
    std::vector<std::string> available_tags = string_split(p_settings->m_editor_tags, ";");

    // If the master tag is not in the tag list, do not add this graphic to the
    // editor.
    if (std::find(available_tags.begin(), available_tags.end(), m_editor_item_tag) == available_tags.end())
        return false;

    // Find the menu entries that contain the tags this graphic has set.
    std::vector<cEditor_Menu_Entry*> target_menu_entries = find_target_menu_entries_for(available_tags);
    std::vector<cEditor_Menu_Entry*>::iterator iter;

    // Find the PNG of this settings file. If an equally named .png exists,
    // assume that file, otherwise check the settings 'base' property. If
    // that also doesn't exist, that's an error.
    boost::filesystem::path pixmap_path(settings_path); // Copy
    pixmap_path.replace_extension(utf8_to_path(".png"));
    if (!boost::filesystem::exists(pixmap_path)) {
        if (p_settings->m_base.empty()) { // Error
            std::cerr << "PNG file for settings file '" << path_to_utf8(settings_path) << "' not found (no .png found and no 'base' setting)." << std::endl;
            std::cerr << "Using dummy image instead." << std::endl;
            pixmap_path = pResource_Manager->Get_Game_Pixmap("game/image_not_found.png");
        }
        else {
            pixmap_path = pixmap_path.parent_path() / p_settings->m_base;
            if (!boost::filesystem::exists(pixmap_path)) {
                std::cerr << "PNG base file not found at '" << path_to_utf8(pixmap_path) << "'." << std::endl;
                std::cerr << "Using dummy image instead." << std::endl;
                pixmap_path = pResource_Manager->Get_Game_Pixmap("game/image_not_found.png");
            }
        }
    }

    // Create the template sprite that will be copied each time the
    // user wants to add this object.
    // Cf. cSprite::cSprite(XmlAtributes) constructor on how to create
    // a sprite correctly.
    cSprite* p_template_sprite = new cSprite(&m_sprite_manager);
    p_template_sprite->Set_Image(pVideo->Get_Surface(settings_path), 1); // FIXME: handle .imgset files?
    p_template_sprite->Set_Massive_Type(p_settings->m_massive_type);
    m_sprite_manager.Add(p_template_sprite); // Memory-manage it

    // Add the graphics to the respective menu entries' GUI panels.
    for(iter=target_menu_entries.begin(); iter != target_menu_entries.end(); iter++) {
        (*iter)->Add_Item(
            p_template_sprite,
            load_cegui_image(pixmap_path),
            p_settings->m_name,
            CEGUI::Quaternion::eulerAnglesDegrees(
                p_settings->m_rotation_x,
                p_settings->m_rotation_y,
                p_settings->m_rotation_z
                )
            );
    }

    delete p_settings;
    return true;
}

/**
 * Similar to Try_Add_Image_Item(), but this method instead adds
 * a special object that is not based on a .settings file (e.g. enemies)
 * to the editor menu. Regarding tags, the function works exactly the same
 * way as Try_Add_Image_Item(), except the tags for each object are not
 * specified in a .settings file (as they don't have one), but in the
 * editor/level_items.xml or editor/world_items.xml file.
 */
bool cEditor::Try_Add_Special_Item(cSprite* p_sprite)
{
    // Get the list of tags attached to this graphic.
    std::vector<std::string> available_tags = string_split(p_sprite->m_editor_tags, ";");

    // If the master tag is not in the tag list, do not add this graphic to the
    // editor.
    if (std::find(available_tags.begin(), available_tags.end(), m_editor_item_tag) == available_tags.end())
        return false;

    // Find the menu entries that contain the tags this graphic has set.
    std::vector<cEditor_Menu_Entry*> target_menu_entries = find_target_menu_entries_for(available_tags);
    std::vector<cEditor_Menu_Entry*>::iterator iter;

    // Some objects (like pathes) have no image at all. Others are
    // marked as "special", which means they intentionally have no
    // image. The rest is an error and needs to have the dummy image.
    boost::filesystem::path image_path;
    if (p_sprite->Get_Image(0))
        image_path = p_sprite->Get_Image(0)->Get_Real_PNG_Path();
    else if (std::find(available_tags.begin(), available_tags.end(), "special") != available_tags.end())
        image_path = pResource_Manager->Get_Game_Pixmap("game/editor/special.png");
    else
        image_path = pResource_Manager->Get_Game_Pixmap("game/image_not_found.png");

    // Add the graphics to the respective menu entries' GUI panels.
    for(iter=target_menu_entries.begin(); iter != target_menu_entries.end(); iter++) {
        (*iter)->Add_Item(
            p_sprite,
            load_cegui_image(image_path),
            p_sprite->Create_Name(),
            CEGUI::Quaternion::eulerAnglesDegrees(
                p_sprite->m_start_rot_x,
                p_sprite->m_start_rot_y,
                p_sprite->m_start_rot_z
                )
            );
    }

    return true;
}

void cEditor::parse_menu_file()
{
    std::string menu_file = path_to_utf8(m_menu_filename);

    // The menu XML file is so dead simple that a SAX parser would
    // simply be overkill. Leightweight XPath queries are enough.
    xmlpp::DomParser parser;
    parser.parse_file(menu_file);

#ifdef USE_LIBXMLPP3
    xmlpp::Element* p_root = parser.get_document()->get_root_node();
    xmlpp::Node::NodeSet items = p_root->find("item");

    xmlpp::Node::NodeSet::const_iterator iter;
#else
    xmlpp::Element* p_root = parser.get_document()->get_root_node();
    xmlpp::NodeSet items = p_root->find("item");

    xmlpp::NodeSet::const_iterator iter;
#endif
    for (iter=items.begin(); iter != items.end(); iter++) {
        xmlpp::Element* p_node = dynamic_cast<xmlpp::Element*>(*iter);

        std::string name   = dynamic_cast<xmlpp::Element*>(p_node->find("property[@name='name']")[0])->get_attribute("value")->get_value();
        std::string tagstr = dynamic_cast<xmlpp::Element*>(p_node->find("property[@name='tags']")[0])->get_attribute("value")->get_value();

        // Set color if available (---header--- elements have no color property)
#ifdef USE_LIBXMLPP3
        std::string colorstr = "FFFFFFFF";
        xmlpp::Node::NodeSet results = p_node->find("property[@name='color']");
#else
        std::string colorstr = "FFFFFFFF";
        xmlpp::NodeSet results = p_node->find("property[@name='color']");
#endif
        if (!results.empty())
            colorstr = dynamic_cast<xmlpp::Element*>(results[0])->get_attribute("value")->get_value();

        // Burst the tag list into its elements
        std::vector<std::string> tags = string_split(tagstr, ";");

        // Prepare informational menu object
        cEditor_Menu_Entry* p_entry = new cEditor_Menu_Entry(name);
        p_entry->Set_Color(Color(colorstr));
        p_entry->Set_Required_Tags(tags);

        // Mark as header element if the tag "header" is encountered.
        std::vector<std::string>::iterator iter;
        iter = std::find(tags.begin(), tags.end(), std::string("header"));
        p_entry->Set_Header((iter != tags.end()));

        // Mark as a function element (= does something special if clicked)
        // if the tag "function" is encountered.
        iter = std::find(tags.begin(), tags.end(), std::string("function"));
        p_entry->Set_Function((iter != tags.end()));

        // Store
        m_menu_entries.push_back(p_entry);
    }
}

void cEditor::populate_menu()
{
    std::vector<cEditor_Menu_Entry*>::iterator iter;

    for(iter = m_menu_entries.begin(); iter != m_menu_entries.end(); iter++) {
        CEGUI::ListboxTextItem* p_item = new CEGUI::ListboxTextItem((*iter)->Get_Name());
        p_item->setTextColours(CEGUI::ColourRect((*iter)->Get_Color().Get_cegui_Color()));
        p_item->setUserData(*iter);
        mp_menu_listbox->addItem(p_item);
    }
}

/// Load the static .settings-file based objects into the editor menu.
void cEditor::load_image_items()
{
    std::vector<boost::filesystem::path> image_files = Get_Directory_Files(pResource_Manager->Get_Game_Pixmaps_Directory(), ".settings");
    std::vector<boost::filesystem::path>::iterator iter;

    for(iter=image_files.begin(); iter != image_files.end(); iter++) {
        Try_Add_Image_Item(*iter);
    }
}

/// Load the special objects into the editor menu (e.g. enemies).
void cEditor::load_special_items()
{
    std::vector<cSprite*> tagged_sprites = Parse_Items_File();
    std::vector<cSprite*>::iterator iter;

    for(iter=tagged_sprites.begin(); iter != tagged_sprites.end(); iter++) {
        Try_Add_Special_Item(*iter);
    }
}

cEditor_Menu_Entry* cEditor::get_menu_entry(const std::string& name)
{
    std::vector<cEditor_Menu_Entry*>::iterator iter;
    for(iter=m_menu_entries.begin(); iter != m_menu_entries.end(); iter++) {
        if ((*iter)->Get_Name() == name) {
            return (*iter);
        }
    }

    throw(std::runtime_error(std::string("Element '") + name + "' not in editor menu list!"));
}

/**
 * Iterates the list of menu entries and returns a list of those whose
 * requirements match the given taglist. That is, a menu entry declares
 * a set of required tags, and if the passed graphic has all of these
 * tags set, the menu entry is included in the return list. If more
 * than the required tags are set, the entry is returned nevertheless.
 *
 * \param[in] available_tags
 * A list of tags the target graphics has set. This is what you
 * find in a .settings file of a graphic or for special items
 * in the editor/level_items.xml resp. editor/world_items.xml file.
 */
std::vector<cEditor_Menu_Entry*> cEditor::find_target_menu_entries_for(const std::vector<std::string>& available_tags)
{
    std::vector<cEditor_Menu_Entry*> results;

    /* To show up in the editor, a graphic needs to have all tags set
     * on it that are required by a menu entry (except for the master
     * editor tag, which is checked elsewhere). */
    std::vector<cEditor_Menu_Entry*>::iterator iter;
    for(iter=m_menu_entries.begin(); iter != m_menu_entries.end(); iter++) {
        cEditor_Menu_Entry* p_menu_entry = *iter;
        std::vector<std::string>::iterator tag_iterator;

        // Check if all of this menu's tags are on the graphic.
        bool all_tags_available = true;
        for(tag_iterator=p_menu_entry->Get_Required_Tags().begin(); tag_iterator != p_menu_entry->Get_Required_Tags().end(); tag_iterator++) {
            if (std::find(available_tags.begin(), available_tags.end(), *tag_iterator) == available_tags.end()) {
                // Tag is missing.
                all_tags_available = false;
                break;
            }
        }

        if (all_tags_available) {
            results.push_back(p_menu_entry);
        }
    }

    return results;
}

bool cEditor::on_mouse_enter(const CEGUI::EventArgs& event)
{
    m_mouse_inside = true;
    m_visibility_timer = 0.0f;
    m_rested = false;

    mp_editor_tabpane->setAlpha(1.0f);
    mp_editor_tabpane->setXPosition(m_tabpane_target_x_position);
    return true;
}

bool cEditor::on_mouse_leave(const CEGUI::EventArgs& event)
{
    m_mouse_inside = false;
    return true;
}

bool cEditor::on_menu_selection_changed(const CEGUI::EventArgs& event)
{
    CEGUI::ListboxItem* p_current_item = mp_menu_listbox->getFirstSelectedItem();

    // Ignore if no selection
    if (!p_current_item)
        return false;

    cEditor_Menu_Entry* p_menu_entry = static_cast<cEditor_Menu_Entry*>(p_current_item->getUserData());

    // Ignore clicks on headings
    if (p_menu_entry->Is_Header())
        return false;

    // Treat clicks on function elements specifically
    if (p_menu_entry->Is_Function()) {
        Activate_Function_Entry(p_menu_entry);
        return true;
    }

    // Ordinary menu item (i.e. submenu with game objects).
    p_menu_entry->Activate(mp_editor_tabpane);

    return true;
}

/**
 * Takes a function menu entry and executes the Function_*() function it
 * belongs to. These functions are intended to be overridden in subclasses.
 */
void cEditor::Activate_Function_Entry(cEditor_Menu_Entry* p_function_entry)
{
    std::vector<std::string>& tags = p_function_entry->Get_Required_Tags();
    std::vector<std::string>::const_iterator iter;

    if (std::find(tags.begin(), tags.end(), std::string("new")) != tags.end())
        Function_New();
    else if (std::find(tags.begin(), tags.end(), std::string("load")) != tags.end())
        Function_Load();
    else if (std::find(tags.begin(), tags.end(), std::string("save")) != tags.end())
        Function_Save(); // TODO: optional argument?
    else if (std::find(tags.begin(), tags.end(), std::string("save_as")) != tags.end())
        Function_Save_as();
    else if (std::find(tags.begin(), tags.end(), std::string("delete")) != tags.end())
        Function_Delete();
    else if (std::find(tags.begin(), tags.end(), std::string("settings")) != tags.end())
        Function_Settings();
    else
        throw(std::runtime_error("Invalid function menu item!"));
}

cEditor_Menu_Entry::cEditor_Menu_Entry(std::string name)
{
    m_name = name;
    m_element_y = 0;

    // Prepare the CEGUI items window. This will be shown whenever
    // this menu entry is clicked.
    mp_tab_pane = static_cast<CEGUI::ScrollablePane*>(CEGUI::WindowManager::getSingleton().createWindow("TSCLook256/ScrollablePane", std::string("editor_items_") + name));
    mp_tab_pane->setPosition(CEGUI::UVector2(CEGUI::UDim(0, 0), CEGUI::UDim(0.01, 0)));
    mp_tab_pane->setSize(CEGUI::USize(CEGUI::UDim(0.99, 0), CEGUI::UDim(0.95, 0)));
    mp_tab_pane->setContentPaneAutoSized(true);
}

cEditor_Menu_Entry::~cEditor_Menu_Entry()
{
    // TODO: Detach if still attached and destroy
    // Nonattached windows will not get destroyed, obviously
    // CEGUI::Window* p_editor_tabpane = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("tabcontrol_editor/editor_tab_items")
}

/**
 * Looks up if the given file is available in the CEGUI image manager, and
 * if so, returns the CEGUI image reference string to add (you can pass
 * this into cEditor_Menu_Entry::Add_Item()). If not, loads the file into
 * the image manager and returns the same; further calls will return the
 * reference to the stored image. I.e., the image gets cached.
 *
 * \param path
 * Absolute path to the PNG file to load.
 *
 * \returns CEGUI image identifier that can be used everywhere CEGUI
 * expects an image.
 */
std::string cEditor::load_cegui_image(boost::filesystem::path path)
{
    std::string escaped_path(path_to_utf8(path));
    string_replace_all(escaped_path, "/", "+"); // CEGUI doesn't like / in ImageManager image names

    /* CEGUI only knows about image sets, not about single images.
     * Thus we effectively add one-image imagesets here to have CEGUI
     * display our image. Note CEGUI also caches these images and will
     * throw a CEGUI::AlreadyExistsException in addFromImageFile() if
     * an image is added multiple times (like multiple loads of the
     * editor or just the same item being added to multiple menus).
     * Thus we have to check if the item graphic has been cached before,
     * and only if that isn't the case, load the file from disk in the
     * way described. */
    if (!CEGUI::ImageManager::getSingleton().isDefined(escaped_path)) {
        std::string string_path(path_to_utf8(path));
        std::string target_group;
        boost::filesystem::path reltarget;

        // Find out whether we need to specify a path relative to the cache
        // or to the game pixmaps directory. CEGUI does not accept absolute
        // pathes as we receive them here.
        if (string_path.find(path_to_utf8(pResource_Manager->Get_User_Pixmaps_Directory())) != string::npos) {
            target_group = "cache-images";
            reltarget = pResource_Manager->Get_User_Pixmaps_Directory();
        }
        else {
            target_group = "ingame-images";
            reltarget = pResource_Manager->Get_Game_Pixmaps_Directory();
        }

        try {
            boost::filesystem::path relative_path = fs_relative(reltarget, path);
            CEGUI::ImageManager::getSingleton().addFromImageFile(escaped_path, path_to_utf8(relative_path), target_group);
        }
        catch(CEGUI::RendererException& e) {
            std::cerr << "Warning: Failed to load as editor item image: " << path << std::endl;
            std::cerr << "Using dummy image instead." << std::endl;
            return load_cegui_image(pResource_Manager->Get_Game_Pixmap("game/image_not_found.png"));
        }
    }

    return escaped_path;
}

void cEditor_Menu_Entry::Add_Item(cSprite* p_template_sprite, std::string cegui_img_ident, std::string name, CEGUI::Quaternion rotation)
{
    static const int labelheight = 24;
    static const int imageheight = 48; /* Also image width (square) */
    static const int yskip = 24;

    CEGUI::Window* p_label = CEGUI::WindowManager::getSingleton().createWindow("TSCLook256/StaticText"/* , std::string("label-of-") + name */);
    p_label->setText(name);
    p_label->setSize(CEGUI::USize(CEGUI::UDim(1, 0), CEGUI::UDim(0, labelheight)));
    p_label->setPosition(CEGUI::UVector2(CEGUI::UDim(0, 0), CEGUI::UDim(0, m_element_y)));
    p_label->setProperty("FrameEnabled", "False");

    CEGUI::Window* p_image = CEGUI::WindowManager::getSingleton().createWindow("TSCLook256/StaticImage"/* , std::string("image-of-") + name */);
    p_image->setProperty("Image", cegui_img_ident);
    p_image->setSize(CEGUI::USize(CEGUI::UDim(0, imageheight), CEGUI::UDim(0, imageheight)));
    p_image->setPosition(CEGUI::UVector2(CEGUI::UDim(0.5, -imageheight/2) /* center on X */, CEGUI::UDim(0, m_element_y + labelheight)));
    p_image->setProperty("FrameEnabled", "False");
    p_image->setUserData(p_template_sprite);
    p_image->subscribeEvent(CEGUI::Window::EventMouseButtonDown, CEGUI::Event::Subscriber(&cEditor_Menu_Entry::on_image_mouse_down, this));

    // Apply rotation
    p_image->setRotation(rotation);

    mp_tab_pane->addChild(p_label);
    mp_tab_pane->addChild(p_image);

    // Remember where we stopped for the next call.
    m_element_y += labelheight + imageheight + yskip;
}

/// Activate this entry's panel in the handed tabbook.
void cEditor_Menu_Entry::Activate(CEGUI::TabControl* p_tabcontrol)
{
    CEGUI::Window* p_container = p_tabcontrol->getTabContents("editor_tab_items");

    // Detach the current scrollable pane.
    CEGUI::ScrollablePane* p_current_pane = static_cast<CEGUI::ScrollablePane*>(p_container->getChildElementAtIdx(0));
    p_container->removeChild(p_current_pane);

    // Attach our content instead.
    p_container->addChild(mp_tab_pane);

    // Switch to the items page
    p_tabcontrol->setSelectedTab("editor_tab_items");
}

bool cEditor_Menu_Entry::on_image_mouse_down(const CEGUI::EventArgs& ev)
{
    // EventMouseButtonDown event handler receives a CEGUI::MouseEventArgs in reality,
    // see http://static.cegui.org.uk/docs/0.8.7/classCEGUI_1_1Window.html#a073c0f8e07cad39c21dce04cc2e49b3c.
    const CEGUI::MouseEventArgs& event = static_cast<const CEGUI::MouseEventArgs&>(ev);

    const cSprite* p_template_sprite = static_cast<const cSprite*>(event.window->getUserData());

    // Create the new sprite
    cSprite* p_new_sprite = p_template_sprite->Copy();
    p_new_sprite->Set_Pos(pMouseCursor->m_pos_x, pMouseCursor->m_pos_y, 1);

    // Move the editor tabpane to the left
    mp_tab_pane->getParent()->getParent()->getParent()->setXPosition(CEGUI::UDim(TABPANE_OUT_OF_SIGHT_X, 0));

    // Add the new sprite to the level / world and attach it to the mouse cursor,
    // ensuring the cursor is centered on the object.
    if (Game_Mode == MODE_LEVEL) {
        p_new_sprite->Set_Sprite_Manager(pActive_Level->m_sprite_manager);
        pActive_Level->m_sprite_manager->Add(p_new_sprite);
    }
    else {
        p_new_sprite->Set_Sprite_Manager(pActive_Overworld->m_sprite_manager);
        pActive_Overworld->m_sprite_manager->Add(p_new_sprite);
    }

    pMouseCursor->Set_Hovered_Object(p_new_sprite);
    pMouseCursor->m_left = 1;
    pMouseCursor->m_hovering_object->m_mouse_offset_y = static_cast<int>(p_new_sprite->m_col_rect.m_h / 2);
    pMouseCursor->m_hovering_object->m_mouse_offset_x = static_cast<int>(p_new_sprite->m_col_rect.m_w / 2);

    return true;
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
    for (cSprite_List::iterator itr = mp_edited_sprite_manager->objects.begin(); itr != mp_edited_sprite_manager->objects.end(); ++itr) {
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
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) || pJoystick->Right()) {
        if (pKeyboard->Is_Shift_Down()) {
            pActive_Camera->Move(CAMERA_SPEED * pFramerate->m_speed_factor * 3 * pPreferences->m_scroll_speed, 0.0f);
        }
        else {
            pActive_Camera->Move(CAMERA_SPEED * pFramerate->m_speed_factor * pPreferences->m_scroll_speed, 0.0f);
        }
    }
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) || pJoystick->Left()) {
        if (pKeyboard->Is_Shift_Down()) {
            pActive_Camera->Move(-(CAMERA_SPEED * pFramerate->m_speed_factor * 3 * pPreferences->m_scroll_speed), 0.0f);
        }
        else {
            pActive_Camera->Move(-(CAMERA_SPEED * pFramerate->m_speed_factor * pPreferences->m_scroll_speed), 0.0f);
        }
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) || pJoystick->Up()) {
        if (pKeyboard->Is_Shift_Down()) {
            pActive_Camera->Move(0.0f, -(CAMERA_SPEED * pFramerate->m_speed_factor * 3 * pPreferences->m_scroll_speed));
        }
        else {
            pActive_Camera->Move(0.0f, -(CAMERA_SPEED * pFramerate->m_speed_factor * pPreferences->m_scroll_speed));
        }
    }
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) || pJoystick->Down()) {
        if (pKeyboard->Is_Shift_Down()) {
            pActive_Camera->Move(0.0f, CAMERA_SPEED * pFramerate->m_speed_factor * 3 * pPreferences->m_scroll_speed);
        }
        else {
            pActive_Camera->Move(0.0f, CAMERA_SPEED * pFramerate->m_speed_factor * pPreferences->m_scroll_speed);
        }
    }
}

void cEditor::Function_Exit(void)
{
    sf::Event newevt;
    newevt.type = sf::Event::KeyPressed;
    newevt.key.code = sf::Keyboard::F8;
    pKeyboard->Key_Down(newevt);
}

cSprite_List cEditor::copy_direction(const cSprite_List& objects, const ObjectDirection dir) const
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

        new_objects.push_back(copy_direction(obj, dir, offset));
    }

    // return only new objects
    return new_objects;
}

cSprite* cEditor::copy_direction(const cSprite* obj, const ObjectDirection dir, int offset /* = 0 */) const
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

void cEditor::replace_sprites(void)
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

void cEditor::update_status_bar()
{
    char status_text[512] = {'\0'};
    char mousepos[32] = {'\0'};
    char startpos[32] = {'\0'};

    std::string display_name;
    std::string target_level;
    std::string levele;
    std::string massivity;
    int uid = -1;
    Color ltarget_color = white;
    Color mass_color = white;

    // Current mouse coordinates
    snprintf(mousepos, 32, "(%d,%d)", static_cast<int>(pMouseCursor->m_pos_x), static_cast<int>(pMouseCursor->m_pos_y));

    if (pMouseCursor->m_hovering_object->m_obj) { // If the cursor is hovering about something
        // Display name
        display_name = pMouseCursor->m_hovering_object->m_obj->Create_Name();

        // UID
        uid = pMouseCursor->m_hovering_object->m_obj->m_uid;

        // Object Start position
        snprintf(startpos, 32, "(%d,%d)",
                 static_cast<int>(pMouseCursor->m_hovering_object->m_obj->m_start_pos_x),
                 static_cast<int>(pMouseCursor->m_hovering_object->m_obj->m_start_pos_y));

        // Special info about level exits/entries
        if (pMouseCursor->m_hovering_object->m_obj->m_type == TYPE_LEVEL_EXIT) {
            cLevel_Exit* p_exit = static_cast<cLevel_Exit*>(pMouseCursor->m_hovering_object->m_obj);
            ltarget_color       = p_exit->m_editor_color;
            ltarget_color.alpha = 255;

            if (p_exit->m_dest_level.empty() && p_exit->m_dest_entry.empty()) { // Level finish
                // TRANS: Displayed when hovering over the level finish object in the editor.
                target_level = _("FINISH");
            }
            else if (!p_exit->m_dest_level.empty() && !p_exit->m_dest_entry.empty()) { // entry in sublevel
                target_level = p_exit->m_dest_level;
                levele = std::string("→ ") + p_exit->m_dest_entry;
            }
            else if (p_exit->m_dest_level.empty()) { // entry in the active level
                // TRANS: Displayed when hovering over a level exit without a target level name.
                // TRANS: Do not translate the "<" and ">".
                target_level = _("<This level>");
                levele = std::string("→ ") + p_exit->m_dest_entry;
            }
            else { // sublevel without entry specification
                target_level = p_exit->m_dest_level;
            }
        }
        else if (pMouseCursor->m_hovering_object->m_obj->m_type == TYPE_LEVEL_ENTRY) {
            cLevel_Entry* p_entry = static_cast<cLevel_Entry*>(pMouseCursor->m_hovering_object->m_obj);
            ltarget_color         = p_entry->m_editor_color;
            ltarget_color.alpha   = 255;
            levele                = p_entry->m_entry_name;
        }

        // Massivity
        mass_color = Get_Massive_Type_Color(pMouseCursor->m_hovering_object->m_obj->m_massive_type);
        switch (pMouseCursor->m_hovering_object->m_obj->m_massive_type) {
        case MASS_PASSIVE:
            massivity = _("Passive");
            break;
        case MASS_MASSIVE:
            massivity = _("Massive");
            break;
        case MASS_HALFMASSIVE:
            massivity = _("Halfmassive");
            break;
        case MASS_CLIMBABLE:
            massivity = _("Climbable");
            break;
        case MASS_FRONT_PASSIVE:
            massivity = _("Front Passive");
            break;
        } // No default clause -- let compiler warn about missing values
    }

    CEGUI::String ltarget_cegui_color = CEGUI::PropertyHelper<CEGUI::Colour>::toString(ltarget_color.Get_cegui_Color());
    CEGUI::String mass_cegui_color = CEGUI::PropertyHelper<CEGUI::Colour>::toString(mass_color.Get_cegui_Color());

    if (game_debug) { // Provide additional info in status bar if debug mode
        char currpos[32] = {'\0'};
        std::string zpos;

        if (pMouseCursor->m_hovering_object->m_obj) {
            snprintf(currpos, 32, "(%d,%d)",
                     static_cast<int>(pMouseCursor->m_hovering_object->m_obj->m_pos_x),
                     static_cast<int>(pMouseCursor->m_hovering_object->m_obj->m_pos_y));

            zpos = std::string("Z: ") + float_to_string(pMouseCursor->m_hovering_object->m_obj->m_pos_z, 6);
            // if also got editor z position
            if (!Is_Float_Equal(pMouseCursor->m_hovering_object->m_obj->m_editor_pos_z, 0.0f)) {
                zpos += ", Ed.Z: " + float_to_string(pMouseCursor->m_hovering_object->m_obj->m_editor_pos_z, 6);
            }

        }

        snprintf(status_text, 512, "%-13s │ %-30s │ %-13s │ [colour='%s']%-20s[colour='FFFFFFFF'] │ %s\n"
                                   "[colour='FF00FFFF']%-13s[colour='FFFFFFFF'] │ [colour='%s']%-30s[colour='FFFFFFFF'] │ UID: %-8d │ [colour='%s']%-20s[colour='FFFFFFFF'] │ %s\n",
                 mousepos, string_shorten(display_name, 30).c_str(),
                 startpos, ltarget_cegui_color.c_str(),
                 string_shorten(target_level, 20).c_str(), currpos,
                 string_shorten(Status_Bar_Ident(), 13).c_str(),
                 mass_cegui_color.c_str(), massivity.c_str(), uid,
                 ltarget_cegui_color.c_str(), string_shorten(levele, 20).c_str(),
                 zpos.c_str());
    }
    else {
        snprintf(status_text, 512, "%-13s │ %-30s │ %-13s │ [colour='%s']%s[colour='FFFFFFFF']\n"
                                   "[colour='FF00FFFF']%-13s[colour='FFFFFFFF'] │ [colour='%s']%-30s[colour='FFFFFFFF'] │ UID: %-8d │ [colour='%s']%s[colour='FFFFFFFF']\n",
                 mousepos, string_shorten(display_name, 30).c_str(),
                 startpos, ltarget_cegui_color.c_str(), target_level.c_str(),
                 string_shorten(Status_Bar_Ident(), 13).c_str(),
                 mass_cegui_color.c_str(), massivity.c_str(), uid,
                 ltarget_cegui_color.c_str(), levele.c_str());
    }

    mp_status_bar->setText(reinterpret_cast<CEGUI::utf8*>(status_text));
}

bool cEditor::on_help_window_exit_clicked(const CEGUI::EventArgs& args)
{
    CEGUI::Window* p_root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
    CEGUI::FrameWindow* p_helpframe = static_cast<CEGUI::FrameWindow*>(p_root->getChild("editor_help"));

    p_root->removeChild(p_helpframe);
    CEGUI::WindowManager::getSingleton().destroyWindow(p_helpframe);
    m_help_window_visible = false;

    return true;
}

bool cEditor::on_selector_radio_changed(const CEGUI::EventArgs& args)
{
    CEGUI::RadioButton* p_selected_radio = mp_selector_radio_selection->getSelectedButtonInGroup();

    if (Game_Mode == MODE_LEVEL) {
        // The names compared to must match the widget names in the
        // layout XML file.
        if (p_selected_radio->getName() == "selection")
            pMouseCursor->Set_Drag_Mode(DRAGMODE_SELECTION);
        else if (p_selected_radio->getName() == "secretarea")
            pMouseCursor->Set_Drag_Mode(DRAGMODE_SECRETAREA);
        else {
            std::cerr << "Warning: Invalid selector radio selected" << std::endl;
        }
    }
    else if (Game_Mode == MODE_OVERWORLD) {
        // Nothing for now (no secret areas in overworlds).
        mp_selector_radio_selection->setSelected(true);
        pMouseCursor->Set_Drag_Mode(DRAGMODE_SELECTION);
    }

    return true;
}

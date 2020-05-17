/***************************************************************************
 * level_settings.cpp  - level editor settings class
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

#include "../core/global_basic.hpp"
#include "../core/game_core.hpp"
#include "../level/level_settings.hpp"
#include "../input/mouse.hpp"
#include "../level/level.hpp"
#include "../video/renderer.hpp"
#include "../core/filesystem/filesystem.hpp"
#include "../core/filesystem/resource_manager.hpp"
#include "../core/framerate.hpp"
#include "../audio/audio.hpp"
#include "../gui/generic.hpp"
#include "../core/i18n.hpp"
#include "../core/filesystem/relative.hpp"

namespace fs = boost::filesystem;

namespace TSC {

/* *** *** *** *** *** cLevel_Settings *** *** *** *** *** *** *** *** *** *** *** *** */

cLevel_Settings::cLevel_Settings()
{
    m_active = 0;

    m_level = NULL;
    m_camera = NULL;
    m_gui_window = NULL;
    m_tabcontrol = NULL;
}

cLevel_Settings::~cLevel_Settings(void)
{
    Unload();

    if (m_camera)
        delete m_camera;
}

void cLevel_Settings::Init(void)
{
    if (m_gui_window) {
        Unload();
    }

    // get window manager
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();
    CEGUI::Window* p_root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();

    // GUI
    m_gui_window = wmgr.loadLayoutFromFile("level_settings/main.layout");
    p_root->addChild(m_gui_window);

    // Tab Control
    m_tabcontrol = static_cast<CEGUI::TabControl*>(p_root->getChild("level_settings/tabcontrol_main"));
    m_tabcontrol->addTab(wmgr.loadLayoutFromFile("level_settings/tab_main.layout"));
    m_tabcontrol->addTab(wmgr.loadLayoutFromFile("level_settings/tab_background.layout"));
    m_tabcontrol->addTab(wmgr.loadLayoutFromFile("level_settings/tab_global_effect.layout"));
    m_tabcontrol->addTab(wmgr.loadLayoutFromFile("level_settings/tab_script.layout"));

    // apply button
    CEGUI::PushButton* button_apply = static_cast<CEGUI::PushButton*>(m_gui_window->getChild("button_apply"));
    button_apply->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&cLevel_Settings::Button_Apply, this));

    //////////////////// Main ////////////////////
    // level filename
    CEGUI::Editbox* editbox_level_filename = static_cast<CEGUI::Editbox*>(m_tabcontrol->getChild("level_settings_tab_main/editbox_level_filename"));
    editbox_level_filename->setText(m_level->Get_Level_Name().c_str());
    // author
    CEGUI::Editbox* editbox_author = static_cast<CEGUI::Editbox*>(m_tabcontrol->getChild("level_settings_tab_main/editbox_author"));
    editbox_author->setText(reinterpret_cast<const CEGUI::utf8*>(m_level->m_author.c_str()));
    // version
    CEGUI::Editbox* editbox_version = static_cast<CEGUI::Editbox*>(m_tabcontrol->getChild("level_settings_tab_main/editbox_version"));
    editbox_version->setText(m_level->m_version.c_str());
    // difficulty
    m_spinner_difficulty = static_cast<CEGUI::Spinner*>(m_tabcontrol->getChild("level_settings_tab_main/spinner_difficulty"));
    m_spinner_difficulty->setCurrentValue(static_cast<float>(m_level->m_difficulty));
    m_spinner_difficulty->subscribeEvent(CEGUI::Spinner::EventValueChanged, CEGUI::Event::Subscriber(&cLevel_Settings::Spinner_Difficulty_Changed, this));

    m_slider_difficulty = static_cast<CEGUI::Slider*>(m_tabcontrol->getChild("level_settings_tab_main/slider_difficulty"));
    m_slider_difficulty->setCurrentValue(static_cast<float>(m_level->m_difficulty));
    m_slider_difficulty->subscribeEvent(CEGUI::Slider::EventValueChanged, CEGUI::Event::Subscriber(&cLevel_Settings::Slider_Difficulty_Changed, this));

    m_text_difficulty_name = m_tabcontrol->getChild("level_settings_tab_main/text_difficulty_name");
    m_text_difficulty_name->setText(Get_Difficulty_Name(m_level->m_difficulty));

    // music filename
    CEGUI::Combobox* combo_music_filename = static_cast<CEGUI::Combobox*>(m_tabcontrol->getChild("level_settings_tab_main/combo_music_filename"));
    std::vector<fs::path> filenames = Get_Directory_Files(pResource_Manager->Get_Game_Music_Directory(), ".ogg");
    std::sort(filenames.begin(), filenames.end());
    for (fs::path path: filenames) {
        fs::path filename = fs_relative(pResource_Manager->Get_Game_Music_Directory(), path);
        combo_music_filename->addItem(new CEGUI::ListboxTextItem(reinterpret_cast<const CEGUI::utf8*>(path_to_utf8(filename).c_str())));
    }
    combo_music_filename->setText(reinterpret_cast<const CEGUI::utf8*>(path_to_utf8(m_level->Get_Music_Filename()).c_str()));
    combo_music_filename->setReadOnly(true);

    // land type
    CEGUI::Combobox* combo_land_type = static_cast<CEGUI::Combobox*>(m_tabcontrol->getChild("level_settings_tab_main/combo_land_type"));
    // add all types
    for (unsigned int i = 0; i < LLT_LAST; i++) {
        combo_land_type->addItem(new CEGUI::ListboxTextItem(Get_Level_Land_Type_Name(static_cast<LevelLandType>(i)), i));
    }
    combo_land_type->setText(Get_Level_Land_Type_Name(m_level->m_land_type).c_str());
    // description
    CEGUI::MultiLineEditbox* multieditbox_description = static_cast<CEGUI::MultiLineEditbox*>(m_tabcontrol->getChild("level_settings_tab_main/multieditbox_description"));
    multieditbox_description->setText(reinterpret_cast<const CEGUI::utf8*>(m_level->m_description.c_str()));

    // camera limits
    CEGUI::Spinner* spinner = static_cast<CEGUI::Spinner*>(m_tabcontrol->getChild("level_settings_tab_main/spinner_camera_limit_w"));
    spinner->setCurrentValue(m_level->m_camera_limits.m_w);
    spinner = static_cast<CEGUI::Spinner*>(m_tabcontrol->getChild("level_settings_tab_main/spinner_camera_limit_h"));
    spinner->setCurrentValue(m_level->m_camera_limits.m_h);
    // fixed camera horizontal velocity
    spinner = static_cast<CEGUI::Spinner*>(m_tabcontrol->getChild("level_settings_tab_main/spinner_camera_hor_vel"));
    spinner->setCurrentValue(m_level->m_fixed_camera_hor_vel);
    // last save time
    CEGUI::Editbox* editbox_save_time = static_cast<CEGUI::Editbox*>(m_tabcontrol->getChild("level_settings_tab_main/editbox_save_time"));
    editbox_save_time->setText(Time_to_String(m_level->m_last_saved, "%Y-%m-%d  %H:%M:%S"));

    // unload after exit
    CEGUI::Combobox* combo_unload_after_exit = static_cast<CEGUI::Combobox*>(m_tabcontrol->getChild("level_settings_tab_main/unload_after_exit"));
    combo_unload_after_exit->addItem(new CEGUI::ListboxTextItem("Do not unload", 0));
    combo_unload_after_exit->addItem(new CEGUI::ListboxTextItem("Unload", 1));
    combo_unload_after_exit->setText(m_level->m_unload_after_exit ? "Unload" : "Do not unload");

    //////////////////// Background ////////////////////

    // add background image button
    CEGUI::PushButton* button_add_background_image = static_cast<CEGUI::PushButton*>(m_tabcontrol->getChild("level_settings_tab_background/button_add_background_image"));
    button_add_background_image->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&cLevel_Settings::Add_Background_Image, this));
    // delete background image button
    CEGUI::PushButton* button_delete_background_image = static_cast<CEGUI::PushButton*>(m_tabcontrol->getChild("level_settings_tab_background/button_delete_background_image"));
    button_delete_background_image->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&cLevel_Settings::Delete_Background_Image, this));

    // listbox
    CEGUI::Listbox* listbox = static_cast<CEGUI::Listbox*>(m_tabcontrol->getChild("level_settings_tab_background/listbox_backgrounds"));
    listbox->setSortingEnabled(1);
    listbox->subscribeEvent(CEGUI::Listbox::EventSelectionChanged, CEGUI::Event::Subscriber(&cLevel_Settings::Set_Background_Image, this));
    // type
    CEGUI::Combobox* combobox = static_cast<CEGUI::Combobox*>(m_tabcontrol->getChild("level_settings_tab_background/combo_bg_image_type"));
    combobox->addItem(new CEGUI::ListboxTextItem(cBackground::Get_Type_Name(BG_NONE)));
    combobox->addItem(new CEGUI::ListboxTextItem(cBackground::Get_Type_Name(BG_IMG_TOP)));
    combobox->addItem(new CEGUI::ListboxTextItem(cBackground::Get_Type_Name(BG_IMG_BOTTOM)));
    combobox->addItem(new CEGUI::ListboxTextItem(cBackground::Get_Type_Name(BG_IMG_ALL)));
    combobox->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cLevel_Settings::Update_BG_Image, this));
    // speed
    spinner = static_cast<CEGUI::Spinner*>(m_tabcontrol->getChild("level_settings_tab_background/spinner_bg_image_speed_x"));
    spinner->subscribeEvent(CEGUI::Spinner::EventKeyUp, CEGUI::Event::Subscriber(&cLevel_Settings::Update_BG_Image, this));
    spinner = static_cast<CEGUI::Spinner*>(m_tabcontrol->getChild("level_settings_tab_background/spinner_bg_image_speed_y"));
    spinner->subscribeEvent(CEGUI::Spinner::EventKeyUp, CEGUI::Event::Subscriber(&cLevel_Settings::Update_BG_Image, this));
    // position
    CEGUI::Editbox* editbox = static_cast<CEGUI::Editbox*>(m_tabcontrol->getChild("level_settings_tab_background/editbox_bg_image_posx"));
    editbox->subscribeEvent(CEGUI::Editbox::EventKeyUp, CEGUI::Event::Subscriber(&cLevel_Settings::Update_BG_Image, this));
    editbox = static_cast<CEGUI::Editbox*>(m_tabcontrol->getChild("level_settings_tab_background/editbox_bg_image_posy"));
    editbox->subscribeEvent(CEGUI::Editbox::EventKeyUp, CEGUI::Event::Subscriber(&cLevel_Settings::Update_BG_Image, this));
    editbox = static_cast<CEGUI::Editbox*>(m_tabcontrol->getChild("level_settings_tab_background/editbox_bg_image_posz"));
    editbox->subscribeEvent(CEGUI::Editbox::EventKeyUp, CEGUI::Event::Subscriber(&cLevel_Settings::Update_BG_Image, this));
    // constant velocity
    spinner = static_cast<CEGUI::Spinner*>(m_tabcontrol->getChild("level_settings_tab_background/spinner_bg_image_const_vel_x"));
    spinner->subscribeEvent(CEGUI::Spinner::EventKeyUp, CEGUI::Event::Subscriber(&cLevel_Settings::Update_BG_Image, this));
    spinner = static_cast<CEGUI::Spinner*>(m_tabcontrol->getChild("level_settings_tab_background/spinner_bg_image_const_vel_y"));
    spinner->subscribeEvent(CEGUI::Spinner::EventKeyUp, CEGUI::Event::Subscriber(&cLevel_Settings::Update_BG_Image, this));
    // Gradient colors
    m_bg_color_1 = Color(m_level->m_background_manager->Get_Pointer(0)->m_color_1.red, m_level->m_background_manager->Get_Pointer(0)->m_color_1.green, m_level->m_background_manager->Get_Pointer(0)->m_color_1.blue, 255);
    m_bg_color_2 = Color(m_level->m_background_manager->Get_Pointer(0)->m_color_2.red, m_level->m_background_manager->Get_Pointer(0)->m_color_2.green, m_level->m_background_manager->Get_Pointer(0)->m_color_2.blue, 255);

    // filename
    combobox = static_cast<CEGUI::Combobox*>(m_tabcontrol->getChild("level_settings_tab_background/combo_bg_image_name"));
    fs::path bgdir = pResource_Manager->Get_Game_Pixmaps_Directory() / utf8_to_path("game") / utf8_to_path("background");
    filenames = Get_Directory_Files(bgdir, ".png");
    std::sort(filenames.begin(), filenames.end());
    for (fs::path path: filenames) {
        fs::path filename = fs_relative(bgdir.parent_path().parent_path(), path);
        combobox->addItem(new CEGUI::ListboxTextItem(reinterpret_cast<const CEGUI::utf8*>(path_to_utf8(filename).c_str())));
    }
    combobox->addItem(new CEGUI::ListboxTextItem(UTF8_("(None)"))); // Item for no background image
    combobox->setText(UTF8_("None"));
    combobox->setReadOnly(true);
    combobox->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cLevel_Settings::Update_BG_Image, this));

    // color 1
    editbox = static_cast<CEGUI::Editbox*>(m_tabcontrol->getChild("level_settings_tab_background/editbox_bg_color_start_red"));
    editbox->subscribeEvent(CEGUI::Editbox::EventKeyUp, CEGUI::Event::Subscriber(&cLevel_Settings::Update_BG_Colors, this));
    editbox->setText(int_to_string(m_bg_color_1.red).c_str());
    editbox = static_cast<CEGUI::Editbox*>(m_tabcontrol->getChild("level_settings_tab_background/editbox_bg_color_start_green"));
    editbox->subscribeEvent(CEGUI::Editbox::EventKeyUp, CEGUI::Event::Subscriber(&cLevel_Settings::Update_BG_Colors, this));
    editbox->setText(int_to_string(m_bg_color_1.green).c_str());
    editbox = static_cast<CEGUI::Editbox*>(m_tabcontrol->getChild("level_settings_tab_background/editbox_bg_color_start_blue"));
    editbox->subscribeEvent(CEGUI::Editbox::EventKeyUp, CEGUI::Event::Subscriber(&cLevel_Settings::Update_BG_Colors, this));
    editbox->setText(int_to_string(m_bg_color_1.blue).c_str());
    // color 2
    editbox = static_cast<CEGUI::Editbox*>(m_tabcontrol->getChild("level_settings_tab_background/editbox_bg_color_end_red"));
    editbox->subscribeEvent(CEGUI::Editbox::EventKeyUp, CEGUI::Event::Subscriber(&cLevel_Settings::Update_BG_Colors, this));
    editbox->setText(int_to_string(m_bg_color_2.red).c_str());
    editbox = static_cast<CEGUI::Editbox*>(m_tabcontrol->getChild("level_settings_tab_background/editbox_bg_color_end_green"));
    editbox->subscribeEvent(CEGUI::Editbox::EventKeyUp, CEGUI::Event::Subscriber(&cLevel_Settings::Update_BG_Colors, this));
    editbox->setText(int_to_string(m_bg_color_2.green).c_str());
    editbox = static_cast<CEGUI::Editbox*>(m_tabcontrol->getChild("level_settings_tab_background/editbox_bg_color_end_blue"));
    editbox->subscribeEvent(CEGUI::Editbox::EventKeyUp, CEGUI::Event::Subscriber(&cLevel_Settings::Update_BG_Colors, this));
    editbox->setText(int_to_string(m_bg_color_2.blue).c_str());

    //////////////////// Script ////////////////////
    // script edit field
    CEGUI::MultiLineEditbox* multieditbox_script = static_cast<CEGUI::MultiLineEditbox*>(m_tabcontrol->getChild("level_settings_tab_script/multieditbox_script"));
    multieditbox_script->setText(reinterpret_cast<const CEGUI::utf8*>(m_level->m_script.c_str()));

    CEGUI::Font& monofont = CEGUI::FontManager::getSingleton().get("DejaVuSansMono");
    multieditbox_script->setFont(&monofont);

    Update_BG_Colors(CEGUI::EventArgs());
    Clear_Layer_Field();
    Load_BG_Image_List();
}

void cLevel_Settings::Exit(void)
{
    // back to level
    Game_Action = GA_ENTER_LEVEL;
    Game_Action_Data_Start.add("screen_fadeout", int_to_string(EFFECT_OUT_BLACK));
    Game_Action_Data_Start.add("screen_fadeout_speed", "3");
    Game_Action_Data_End.add("screen_fadein", int_to_string(EFFECT_IN_BLACK));
    Game_Action_Data_End.add("screen_fadein_speed", "3");
    Game_Action_Data_End.add("activate_editor", "1");

    pMouseCursor->Set_Active(0);
}

void cLevel_Settings::Enter(void)
{
    if (!m_level)
        throw(std::runtime_error("No level set with cLevel_Settings::Set_Level()!"));

    // set active camera
    pActive_Camera = m_camera;

    editor_enabled = 0;

    if (pMouseCursor->m_active_object) {
        pMouseCursor->Clear_Active_Object();
    }

    // Initialize level data
    Init();
    // set active
    m_active = 1;

    // update camera
    m_camera->Update_Position();

    pMouseCursor->Set_Active(1);
}

void cLevel_Settings::Leave(void)
{
    // # Main Tab
    // filename
    fs::path level_filename = utf8_to_path(m_tabcontrol->getChild("level_settings_tab_main/editbox_level_filename")->getText().c_str());
    if (!level_filename.empty() && Trim_Filename(m_level->m_level_filename, false, false).compare(level_filename) != 0) {
        m_level->Set_Filename(level_filename);
        if (Box_Question(_("Save ") + path_to_utf8(Trim_Filename(level_filename, false, false)) + " ?")) {
            m_level->Save();
        }
    }
    // music (relative to music/ directory)
    fs::path new_music = utf8_to_path(m_tabcontrol->getChild("level_settings_tab_main/combo_music_filename")->getText().c_str());
    // if the music is new
    if (pAudio->Is_Music_Playing() && new_music.compare(m_level->Get_Music_Filename()) != 0) {
        m_level->Set_Music(new_music);
        pAudio->Fadeout_Music(1000);
    }
    // author
    m_level->Set_Author(m_tabcontrol->getChild("level_settings_tab_main/editbox_author")->getText().c_str());
    // version
    m_level->Set_Version(m_tabcontrol->getChild("level_settings_tab_main/editbox_version")->getText().c_str());
    // difficulty
    m_level->Set_Difficulty(static_cast<CEGUI::Spinner*>(m_tabcontrol->getChild("level_settings_tab_main/spinner_difficulty"))->getCurrentValue());
    // land type
    m_level->Set_Land_Type(Get_Level_Land_Type_Id(static_cast<CEGUI::Combobox*>(m_tabcontrol->getChild("level_settings_tab_main/combo_land_type"))->getText().c_str()));
    // description
    m_level->Set_Description(static_cast<CEGUI::MultiLineEditbox*>(m_tabcontrol->getChild("level_settings_tab_main/multieditbox_description"))->getText().c_str());

    // Camera Limits
    pLevel_Manager->m_camera->Set_Limit_W((static_cast<CEGUI::Spinner*>(m_tabcontrol->getChild("level_settings_tab_main/spinner_camera_limit_w")))->getCurrentValue());
    pLevel_Manager->m_camera->Set_Limit_H((static_cast<CEGUI::Spinner*>(m_tabcontrol->getChild("level_settings_tab_main/spinner_camera_limit_h")))->getCurrentValue());
    m_level->m_camera_limits.m_w = pLevel_Manager->m_camera->m_limit_rect.m_w;
    m_level->m_camera_limits.m_h = pLevel_Manager->m_camera->m_limit_rect.m_h;
    // fixed camera horizontal velocity
    pLevel_Manager->m_camera->m_fixed_hor_vel = (static_cast<CEGUI::Spinner*>(m_tabcontrol->getChild("level_settings_tab_main/spinner_camera_hor_vel")))->getCurrentValue();
    m_level->m_fixed_camera_hor_vel = pLevel_Manager->m_camera->m_fixed_hor_vel;

    // Unload after exit
    m_level->m_unload_after_exit = (static_cast<CEGUI::Combobox*>(m_tabcontrol->getChild("level_settings_tab_main/unload_after_exit")))->isItemSelected(1);

    // # Background Tab
    // Gradient
    m_level->m_background_manager->Get_Pointer(0)->Set_Color_1(m_bg_color_1);
    m_level->m_background_manager->Get_Pointer(0)->Set_Color_2(m_bg_color_2);

    // # Script tab
    std::string new_code = static_cast<CEGUI::MultiLineEditbox*>(m_tabcontrol->getChild("level_settings_tab_script/multieditbox_script"))->getText().c_str();

    // Only force re-init when new code has been specified
    if (m_level->m_script != new_code) {
        m_level->m_script = new_code;
        /* Wipe out the existing mruby context. This is necessary, because
         * if we don’t do this, we either have to ignore that the script code
         * has changed (requiring the user to reload the level) or we would
         * end up running unchanged code again (with unpredictable side effects
         * including event handlers to be registered twice). */
        m_level->Reinitialize_MRuby_Interpreter();
    }

    Unload();
}

void cLevel_Settings::Unload(void)
{
    if (!m_gui_window) {
        return;
    }

    // destroy CEGUI window
    CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->removeChild(m_gui_window);
    CEGUI::WindowManager::getSingleton().destroyWindow(m_gui_window);
    m_gui_window = NULL;
    m_tabcontrol = NULL;

    m_active = 0;
}

void cLevel_Settings::Update(void)
{
    // uhm...

    // update performance timer
    pFramerate->m_perf_timer[PERF_UPDATE_LEVEL_SETTINGS]->Update();
}

void cLevel_Settings::Draw(void)
{
    pVideo->Clear_Screen();
    pVideo->Draw_Rect(NULL, 0.00001f, &black);

    // update performance timer
    pFramerate->m_perf_timer[PERF_DRAW_LEVEL_SETTINGS]->Update();
}

bool cLevel_Settings::Key_Down(const sf::Event& evt)
{
    if (!m_active) {
        return 0;
    }

    if (evt.key.code == sf::Keyboard::Escape) {
        Exit();
    }
    else {
        // not processed
        return 0;
    }

    // key got processed
    return 1;
}

void cLevel_Settings::Set_Level(cLevel* level)
{
    m_level = level;

    if (m_camera)
        delete m_camera;

    m_camera = new cCamera(level->m_sprite_manager);
}

bool cLevel_Settings::Add_Background_Image(const CEGUI::EventArgs& event)
{
    cBackground* background = new cBackground(m_level->m_sprite_manager);
    background->Set_Type(BG_IMG_BOTTOM);
    background->Set_Image(LEVEL_DEFAULT_BACKGROUND);

    m_level->m_background_manager->Add(background);

    Load_BG_Image_List();

    return 1;
}

bool cLevel_Settings::Delete_Background_Image(const CEGUI::EventArgs& event)
{
    CEGUI::Listbox* listbox = static_cast<CEGUI::Listbox*>(m_tabcontrol->getChild("level_settings_tab_background/listbox_backgrounds"));
    CEGUI::ListboxItem* item = listbox->getFirstSelectedItem();

    if (!item) {
        return 1;
    }

    // get background
    cBackground* background = static_cast<cBackground*>(item->getUserData());
    // delete it
    m_level->m_background_manager->Delete(background);

    // update list
    Load_BG_Image_List();
    // clear background image gui
    Update_BG_Image(CEGUI::EventArgs());

    return 1;
}

// This event handler method updates the GUI controls to reflect the
// background the user has selected in the list box.
bool cLevel_Settings::Set_Background_Image(const CEGUI::EventArgs& event)
{
    CEGUI::Listbox* listbox = static_cast<CEGUI::Listbox*>(m_tabcontrol->getChild("level_settings_tab_background/listbox_backgrounds"));
    CEGUI::ListboxItem* item = listbox->getFirstSelectedItem();

    // selected
    if (item) {
        // get background
        cBackground* background = static_cast<cBackground*>(item->getUserData());
        fs::path background_filename = background->m_image_1_filename;

        // type
        CEGUI::Editbox* editbox = static_cast<CEGUI::Editbox*>(m_tabcontrol->getChild("level_settings_tab_background/combo_bg_image_type"));
        editbox->setText(reinterpret_cast<const CEGUI::utf8*>(background->Get_Type_Name().c_str()));
        // filename
        CEGUI::Combobox* combobox = static_cast<CEGUI::Combobox*>(m_tabcontrol->getChild("level_settings_tab_background/combo_bg_image_name"));
        if (background_filename == fs::path())
            combobox->setText(UTF8_("(None)"));
        else
            combobox->setText(reinterpret_cast<const CEGUI::utf8*>(path_to_utf8(background_filename).c_str()));
        // position
        editbox = static_cast<CEGUI::Editbox*>(m_tabcontrol->getChild("level_settings_tab_background/editbox_bg_image_posx"));
        editbox->setText(float_to_string(background->m_start_pos_x, 6, 0).c_str());
        editbox = static_cast<CEGUI::Editbox*>(m_tabcontrol->getChild("level_settings_tab_background/editbox_bg_image_posy"));
        editbox->setText(float_to_string(background->m_start_pos_y, 6, 0).c_str());
        editbox = static_cast<CEGUI::Editbox*>(m_tabcontrol->getChild("level_settings_tab_background/editbox_bg_image_posz"));
        editbox->setText(float_to_string(background->m_pos_z, 6, 0).c_str());
        // speed
        CEGUI::Spinner* spinner = static_cast<CEGUI::Spinner*>(m_tabcontrol->getChild("level_settings_tab_background/spinner_bg_image_speed_x"));
        spinner->setCurrentValue(background->m_speed_x);
        spinner = static_cast<CEGUI::Spinner*>(m_tabcontrol->getChild("level_settings_tab_background/spinner_bg_image_speed_y"));
        spinner->setCurrentValue(background->m_speed_y);
        // constant velocity
        spinner = static_cast<CEGUI::Spinner*>(m_tabcontrol->getChild("level_settings_tab_background/spinner_bg_image_const_vel_x"));
        spinner->setCurrentValue(background->m_const_vel_x);
        spinner = static_cast<CEGUI::Spinner*>(m_tabcontrol->getChild("level_settings_tab_background/spinner_bg_image_const_vel_y"));
        spinner->setCurrentValue(background->m_const_vel_y);
    }
    // deselected
    else {
        Clear_Layer_Field();
    }

    return 1;
}

bool cLevel_Settings::Button_Apply(const CEGUI::EventArgs& event)
{
    Exit();
    return 1;
}

bool cLevel_Settings::Update_BG_Colors(const CEGUI::EventArgs& event)
{
    CEGUI::Editbox* color_start_red = static_cast<CEGUI::Editbox*>(m_tabcontrol->getChild("level_settings_tab_background/editbox_bg_color_start_red"));
    CEGUI::Editbox* color_start_green = static_cast<CEGUI::Editbox*>(m_tabcontrol->getChild("level_settings_tab_background/editbox_bg_color_start_green"));
    CEGUI::Editbox* color_start_blue = static_cast<CEGUI::Editbox*>(m_tabcontrol->getChild("level_settings_tab_background/editbox_bg_color_start_blue"));
    CEGUI::Editbox* color_end_red = static_cast<CEGUI::Editbox*>(m_tabcontrol->getChild("level_settings_tab_background/editbox_bg_color_end_red"));
    CEGUI::Editbox* color_end_green = static_cast<CEGUI::Editbox*>(m_tabcontrol->getChild("level_settings_tab_background/editbox_bg_color_end_green"));
    CEGUI::Editbox* color_end_blue = static_cast<CEGUI::Editbox*>(m_tabcontrol->getChild("level_settings_tab_background/editbox_bg_color_end_blue"));

    m_bg_color_1.red = string_to_int(color_start_red->getText().c_str());
    m_bg_color_1.green = string_to_int(color_start_green->getText().c_str());
    m_bg_color_1.blue = string_to_int(color_start_blue->getText().c_str());
    m_bg_color_2.red = string_to_int(color_end_red->getText().c_str());
    m_bg_color_2.green = string_to_int(color_end_green->getText().c_str());
    m_bg_color_2.blue = string_to_int(color_end_blue->getText().c_str());


    // color start
    color_start_red->setProperty("NormalTextColour", CEGUI::PropertyHelper<CEGUI::Colour>::toString(CEGUI::Colour(1, 1 - (static_cast<float>(m_bg_color_1.red) / 255), 1 - (static_cast<float>(m_bg_color_1.red) / 255), 1)));
    color_start_green->setProperty("NormalTextColour", CEGUI::PropertyHelper<CEGUI::Colour>::toString(CEGUI::Colour(1 - (static_cast<float>(m_bg_color_1.green) / 255), 1, 1 - (static_cast<float>(m_bg_color_1.green) / 255), 1)));
    color_start_blue->setProperty("NormalTextColour", CEGUI::PropertyHelper<CEGUI::Colour>::toString(CEGUI::Colour(1 - (static_cast<float>(m_bg_color_1.blue) / 255), 1 - (static_cast<float>(m_bg_color_1.blue) / 255), 1, 1)));
    // color end
    color_end_red->setProperty("NormalTextColour", CEGUI::PropertyHelper<CEGUI::Colour>::toString(CEGUI::Colour(1, 1 - (static_cast<float>(m_bg_color_2.red) / 255), 1 - (static_cast<float>(m_bg_color_2.red) / 255), 1)));
    color_end_green->setProperty("NormalTextColour", CEGUI::PropertyHelper<CEGUI::Colour>::toString(CEGUI::Colour(1 - (static_cast<float>(m_bg_color_2.green) / 255), 1, 1 - (static_cast<float>(m_bg_color_2.green) / 255), 1)));
    color_end_blue->setProperty("NormalTextColour", CEGUI::PropertyHelper<CEGUI::Colour>::toString(CEGUI::Colour(1 - (static_cast<float>(m_bg_color_2.blue) / 255), 1 - (static_cast<float>(m_bg_color_2.blue) / 255), 1, 1)));

    CEGUI::Window* statictext = m_tabcontrol->getChild("level_settings_tab_background/text_color_start");
    statictext->setProperty("TextColours", CEGUI::PropertyHelper<CEGUI::Colour>::toString(m_bg_color_1.Get_cegui_Color()));
    statictext = m_tabcontrol->getChild("level_settings_tab_background/text_color_end");
    statictext->setProperty("TextColours", CEGUI::PropertyHelper<CEGUI::Colour>::toString(m_bg_color_2.Get_cegui_Color()));

    CEGUI::Window* gradient_image = m_tabcontrol->getChild("level_settings_tab_background/window_background_preview_gradient");
    gradient_image->setProperty("ImageColours",
                                std::string("tl:") + m_bg_color_1.Get_cegui_Colorstr() +
                                "tr:" + m_bg_color_1.Get_cegui_Colorstr() +
                                "bl:" + m_bg_color_2.Get_cegui_Colorstr() +
                                "br:" + m_bg_color_2.Get_cegui_Colorstr());

    return 1;
}

void cLevel_Settings::Load_BG_Image_List(void)
{
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    // Clear list
    CEGUI::Listbox* listbox = static_cast<CEGUI::Listbox*>(m_tabcontrol->getChild("level_settings_tab_background/listbox_backgrounds"));
    listbox->resetList();

    // Clear preview images
    CEGUI::Window* img_container = m_tabcontrol->getChild("level_settings_tab_background/window_background_preview_images");
    while (img_container->getChildCount() > 0)
        img_container->removeChild(img_container->getChildAtIdx(0));

    std::vector<std::pair<std::string, float>> backgrounds;

    for (vector<cBackground*>::iterator itr = m_level->m_background_manager->objects.begin(); itr != m_level->m_background_manager->objects.end(); ++itr) {
        cBackground* background = (*itr);

        // skip gradients
        if (background->m_type == BG_GR_HOR || background->m_type == BG_GR_VER) {
            continue;
        }

        // Add list entry for this background
        CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(float_to_string(background->m_pos_z).c_str(), 0, background);
        item->setSelectionColours(CEGUI::Colour(0.33f, 0.33f, 0.33f));
        item->setSelectionBrushImage("TSCLook256/ListboxSelectionBrush");
        listbox->addItem(item);

        // Add bgname-Z pair for this background
        if (!background->m_image_1_filename.empty()) {
            // Replicate CEGUI background name as set in cEditor_Level::load_background_images_into_cegui().
            std::string cegui_bg_image = path_to_utf8(background->m_image_1_filename.filename());
            size_t pos = cegui_bg_image.find(".png");
            cegui_bg_image.replace(pos, 4, "");

            backgrounds.push_back(std::make_pair(cegui_bg_image, background->m_pos_z));
        }
    }

    // Add the preview images, respecting the Z order
    std::sort(
        backgrounds.begin(),
        backgrounds.end(),
        [](const std::pair<std::string, float>& a,
           const std::pair<std::string, float>& b) {
            return std::get<1>(a) < std::get<1>(b);
        });
    for(std::pair<std::string, float> p: backgrounds) {
        // Add preview image for this background
        CEGUI::Window* preview_image = wmgr.createWindow("TSCLook256/StaticImage");
        preview_image->setPosition(CEGUI::UVector2(CEGUI::UDim(0, 0), CEGUI::UDim(0, 0)));
        preview_image->setSize(CEGUI::USize(CEGUI::UDim(1, 0), CEGUI::UDim(1, 0)));
        preview_image->setProperty("FrameEnabled", "False");
        preview_image->setProperty("BackgroundEnabled", "False");
        preview_image->setProperty("Image", std::get<0>(p));
        img_container->addChild(preview_image);
    }

    CEGUI::PushButton* button_add = static_cast<CEGUI::PushButton*>(m_tabcontrol->getChild("level_settings_tab_background/button_add_background_image"));

    // 9 layers + default background is the maximum
    if (m_level->m_background_manager->size() >= 10) {
        button_add->disable();
    }
    else {
        button_add->enable();
    }
}

bool cLevel_Settings::Update_BG_Image(const CEGUI::EventArgs& event)
{
    CEGUI::Listbox* listbox = static_cast<CEGUI::Listbox*>(m_tabcontrol->getChild("level_settings_tab_background/listbox_backgrounds"));
    CEGUI::ListboxItem* item = listbox->getFirstSelectedItem();

    // clear
    if (!item) {
        Clear_Layer_Field();
        return 1;
    }

    std::string bg_type = m_tabcontrol->getChild("level_settings_tab_background/combo_bg_image_type")->getText().c_str();
    std::string bg_name = m_tabcontrol->getChild("level_settings_tab_background/combo_bg_image_name")->getText().c_str();
    float posx = string_to_float((m_tabcontrol->getChild("level_settings_tab_background/editbox_bg_image_posx"))->getText().c_str());
    float posy = string_to_float((m_tabcontrol->getChild("level_settings_tab_background/editbox_bg_image_posy"))->getText().c_str());
    float posz = string_to_float((m_tabcontrol->getChild("level_settings_tab_background/editbox_bg_image_posz"))->getText().c_str());
    float speed_x = (static_cast<CEGUI::Spinner*>(m_tabcontrol->getChild("level_settings_tab_background/spinner_bg_image_speed_x")))->getCurrentValue();
    float speed_y = (static_cast<CEGUI::Spinner*>(m_tabcontrol->getChild("level_settings_tab_background/spinner_bg_image_speed_y")))->getCurrentValue();
    float const_vel_x = (static_cast<CEGUI::Spinner*>(m_tabcontrol->getChild("level_settings_tab_background/spinner_bg_image_const_vel_x")))->getCurrentValue();
    float const_vel_y = (static_cast<CEGUI::Spinner*>(m_tabcontrol->getChild("level_settings_tab_background/spinner_bg_image_const_vel_y")))->getCurrentValue();

    // get background
    cBackground* background = static_cast<cBackground*>(item->getUserData());
    // set type
    background->Set_Type(bg_type);
    // set position
    background->Set_Start_Pos(posx, posy);
    // set position z
    background->Set_Pos_Z(posz);
    // set scroll speed
    background->Set_Scroll_Speed(speed_x, speed_y);
    // set constant velocity
    background->Set_Const_Velocity_X(const_vel_x);
    background->Set_Const_Velocity_Y(const_vel_y);

    // full filename for validation
    fs::path bg_filename = pResource_Manager->Get_Game_Pixmap(bg_name);

    // invalid file
    if (!File_Exists(bg_filename)) {
        // clear image
        bg_filename.clear();
    }

    // set image
    background->Set_Image(bg_filename);
    // set new item name
    item->setText(float_to_string(posz).c_str());
    // fixme : should update sorting because of the new name
    listbox->handleUpdatedItemData();

    return 1;
}

void cLevel_Settings::Clear_Layer_Field(void)
{
    m_tabcontrol->getChild("level_settings_tab_background/combo_bg_image_type")->setText("Disabled");
    m_tabcontrol->getChild("level_settings_tab_background/combo_bg_image_name")->setText(UTF8_("(None)"));
    m_tabcontrol->getChild("level_settings_tab_background/editbox_bg_image_posx")->setText("");
    m_tabcontrol->getChild("level_settings_tab_background/editbox_bg_image_posy")->setText("");
    m_tabcontrol->getChild("level_settings_tab_background/editbox_bg_image_posz")->setText("");
    m_tabcontrol->getChild("level_settings_tab_background/spinner_bg_image_speed_x")->setText("");
    m_tabcontrol->getChild("level_settings_tab_background/spinner_bg_image_speed_y")->setText("");
    m_tabcontrol->getChild("level_settings_tab_background/spinner_bg_image_const_vel_x")->setText("");
    m_tabcontrol->getChild("level_settings_tab_background/spinner_bg_image_const_vel_y")->setText("");
}

bool cLevel_Settings::Spinner_Difficulty_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::Spinner* spinner = static_cast<CEGUI::Spinner*>(windowEventArgs.window);
    float val = spinner->getCurrentValue();

    m_slider_difficulty->setCurrentValue(val);
    m_text_difficulty_name->setText(Get_Difficulty_Name(val));

    return 1;
}

bool cLevel_Settings::Slider_Difficulty_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    float val = static_cast<CEGUI::Slider*>(windowEventArgs.window)->getCurrentValue();

    m_spinner_difficulty->setCurrentValue(val);
    m_text_difficulty_name->setText(Get_Difficulty_Name(val));

    return 1;
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

/***************************************************************************
 * menu_data.cpp  -  menu data and handling classes
 *
 * Copyright © 2004 - 2011 The TSC Contributors
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
#include "../gui/menu_data.hpp"
#include "../audio/audio.hpp"
#include "../core/game_core.hpp"
#include "../gui/generic.hpp"
#include "../video/font.hpp"
#include "../overworld/overworld.hpp"
#include "../campaign/campaign_manager.hpp"
#include "../user/preferences.hpp"
#include "../input/joystick.hpp"
#include "../input/mouse.hpp"
#include "../core/framerate.hpp"
#include "../core/errors.hpp"
#include "../user/savegame.hpp"
#include "../video/renderer.hpp"
#include "../level/level.hpp"
#include "../input/keyboard.hpp"
#include "../level/level_editor.hpp"
#include "../core/math/utilities.hpp"
#include "../core/i18n.hpp"
#include "../core/math/size.hpp"
#include "../core/filesystem/filesystem.hpp"
#include "../core/filesystem/resource_manager.hpp"
#include "../core/main.hpp"

namespace fs = boost::filesystem;

namespace TSC {

/* *** *** *** *** *** *** *** *** cMenu_Base *** *** *** *** *** *** *** *** *** */

cMenu_Base::cMenu_Base(void)
{
    m_gui_window = NULL;
    m_action = 0;
    m_menu_pos_y = 140.0f;
    m_text_color = Color(static_cast<Uint8>(255), 251, 98);
    m_text_color_value = Color(static_cast<Uint8>(255), 190, 30);

    m_exit_to_gamemode = MODE_NOTHING;
}

cMenu_Base::~cMenu_Base(void)
{
    if (m_gui_window) {
        pGuiSystem->getGUISheet()->removeChildWindow(m_gui_window);
        CEGUI::WindowManager::getSingleton().destroyWindow(m_gui_window);
    }

    for (HudSpriteList::iterator itr = m_draw_list.begin(); itr != m_draw_list.end(); ++itr) {
        delete *itr;
    }

    m_draw_list.clear();
}

void cMenu_Base::Init(void)
{
    m_layout_file = "";
}

void cMenu_Base::Init_GUI(void)
{
    if (m_layout_file.empty()) {
        return;
    }

    m_gui_window = CEGUI::WindowManager::getSingleton().loadWindowLayout(m_layout_file.c_str());
    pGuiSystem->getGUISheet()->addChildWindow(m_gui_window);
}

void cMenu_Base::Enter(const GameMode old_mode /* = MODE_NOTHING */)
{
    // virtual
}

void cMenu_Base::Leave(const GameMode next_mode /* = MODE_NOTHING */)
{
    // virtual
}

void cMenu_Base::Exit(void)
{
    // virtual
}

void cMenu_Base::Update(void)
{
    if (m_exit_to_gamemode != MODE_LEVEL && m_exit_to_gamemode != MODE_OVERWORLD) {
        // animation
        pMenuCore->m_animation_manager->Update();
    }

    // hud
    pHud_Manager->Update();
}

void cMenu_Base::Draw(void)
{
    pVideo->Clear_Screen();

    if (m_exit_to_gamemode == MODE_LEVEL) {
        pActive_Level->m_sprite_manager->Update_Items_Valid_Draw();
        // draw level layer 1
        pActive_Level->Draw_Layer_1();
        // draw alpha rect
        pVideo->Draw_Rect(NULL, 0.125f, &blackalpha128);

        // gui
        pMenuCore->m_handler->Draw(0);
    }
    else if (m_exit_to_gamemode == MODE_OVERWORLD) {
        pActive_Overworld->m_sprite_manager->Update_Items_Valid_Draw();
        // draw world layer 1
        pActive_Overworld->Draw_Layer_1();
        // draw alpha rect
        pVideo->Draw_Rect(NULL, 0.125f, &blackalpha128);

        // gui
        pMenuCore->m_handler->Draw(0);
    }
    else {
        // animation
        pMenuCore->m_animation_manager->Draw();
        // gui
        pMenuCore->m_handler->Draw();
    }

    // menu items
    for (HudSpriteList::iterator itr = m_draw_list.begin(); itr != m_draw_list.end(); ++itr) {
        (*itr)->Draw();
    }
}

void cMenu_Base::Draw_End(void)
{
    // hud
    pHud_Manager->Draw();
}

void cMenu_Base::Set_Exit_To_Game_Mode(GameMode gamemode)
{
    m_exit_to_gamemode = gamemode;
}

/* *** *** *** *** *** *** *** *** cMenu_Main *** *** *** *** *** *** *** *** *** */

cMenu_Main::cMenu_Main(void)
    : cMenu_Base()
{

}

cMenu_Main::~cMenu_Main(void)
{

}

void cMenu_Main::Init(void)
{
    cMenu_Base::Init();

    cMenu_Item* temp_item = NULL;

    m_layout_file = "menu/main.layout";

    // Start
    temp_item = pMenuCore->Auto_Menu("start.png", "start.png", m_menu_pos_y);
    temp_item->m_image_menu->Set_Pos(temp_item->m_pos_x + (temp_item->m_image_default->m_col_rect.m_w + 16), temp_item->m_pos_y);
    pMenuCore->m_handler->Add_Menu_Item(temp_item);
    // Options
    m_menu_pos_y += 60;
    temp_item = pMenuCore->Auto_Menu("options.png", "options.png", m_menu_pos_y);
    temp_item->m_image_menu->Set_Pos(temp_item->m_pos_x - temp_item->m_image_menu->m_col_rect.m_w - 16, temp_item->m_pos_y);
    pMenuCore->m_handler->Add_Menu_Item(temp_item);
    // Load
    m_menu_pos_y += 60;
    temp_item = pMenuCore->Auto_Menu("load.png", "load.png", m_menu_pos_y);
    temp_item->m_image_menu->Set_Pos(temp_item->m_pos_x + (temp_item->m_image_default->m_col_rect.m_w + 16), temp_item->m_pos_y);
    pMenuCore->m_handler->Add_Menu_Item(temp_item);
    // Save
    m_menu_pos_y += 60;
    temp_item = pMenuCore->Auto_Menu("save.png", "save.png", m_menu_pos_y);
    temp_item->m_image_menu->Set_Pos(temp_item->m_pos_x - temp_item->m_image_menu->m_col_rect.m_w - 16, temp_item->m_pos_y);
    pMenuCore->m_handler->Add_Menu_Item(temp_item);
    // Quit
    m_menu_pos_y += 60;
    temp_item = pMenuCore->Auto_Menu("quit.png", "", m_menu_pos_y, 1);
    temp_item->m_image_menu->Set_Pos(temp_item->m_pos_x + temp_item->m_col_rect.m_w + 16, temp_item->m_pos_y);
    pMenuCore->m_handler->Add_Menu_Item(temp_item);

    if (m_exit_to_gamemode == MODE_NOTHING) {
        // Credits
        cGL_Surface* credits = pFont->Render_Text(pFont->m_font_normal, _("Credits"), yellow);
        temp_item = new cMenu_Item(pMenuCore->m_handler->m_level->m_sprite_manager);
        temp_item->m_image_default->Set_Image(credits);
        temp_item->Set_Pos(static_cast<float>(game_res_w) * 0.45f, static_cast<float>(game_res_h) - 30.0f);
        pMenuCore->m_handler->Add_Menu_Item(temp_item, 1.5f, grey);

        cHudSprite* hud_sprite = new cHudSprite(pMenuCore->m_handler->m_level->m_sprite_manager);
        hud_sprite->Set_Image(credits, 0, 1);
        hud_sprite->Set_Pos(-200, 0);
        m_draw_list.push_back(hud_sprite);
        // SDL logo
        hud_sprite = new cHudSprite(pMenuCore->m_handler->m_level->m_sprite_manager);
        hud_sprite->Set_Image(pVideo->Get_Surface("menu/logo_sdl.png"));
        hud_sprite->Set_Pos(static_cast<float>(game_res_w) * 0.04f, static_cast<float>(game_res_h) * 0.935f);
        m_draw_list.push_back(hud_sprite);
    }

    Init_GUI();
}

void cMenu_Main::Init_GUI(void)
{
    cMenu_Base::Init_GUI();

    CEGUI::Window* text_version = CEGUI::WindowManager::getSingleton().getWindow("text_version");

#ifdef TSC_VERSION_POSTFIX
    text_version->setProperty("Text", CEGUI::PropertyHelper::intToString(TSC_VERSION_MAJOR) + "." + CEGUI::PropertyHelper::intToString(TSC_VERSION_MINOR) + "." + CEGUI::PropertyHelper::intToString(TSC_VERSION_PATCH) + "-" + TSC_VERSION_POSTFIX);
    text_version->setProperty("TextColours", "tl:FFFF0000 tr:FFFF0000 bl:FFFF0000 br:FFFF0000");
#else
    text_version->setProperty("Text", UTF8_("Version ") + CEGUI::PropertyHelper::intToString(TSC_VERSION_MAJOR) + "." + CEGUI::PropertyHelper::intToString(TSC_VERSION_MINOR) + "." + CEGUI::PropertyHelper::intToString(TSC_VERSION_PATCH));
#endif

    // if in a level/world
    if (m_exit_to_gamemode != MODE_NOTHING) {
        CEGUI::Window* text_website = CEGUI::WindowManager::getSingleton().getWindow("text_website");
        text_website->hide();
    }
}

void cMenu_Main::Exit(void)
{
    if (m_exit_to_gamemode == MODE_LEVEL) {
        Game_Action = GA_ENTER_LEVEL;
        Game_Action_Data_Middle.add("unload_menu", "1");
    }
    else if (m_exit_to_gamemode == MODE_OVERWORLD) {
        Game_Action = GA_ENTER_WORLD;
        Game_Action_Data_Middle.add("unload_menu", "1");
    }
}

void cMenu_Main::Update(void)
{
    cMenu_Base::Update();

    if (!m_action) {
        return;
    }

    m_action = 0;

    // Start
    if (pMenuCore->m_handler->m_active == 0) {
        Game_Action = GA_ENTER_MENU;
        Game_Action_Data_Middle.add("load_menu", int_to_string(MENU_START));
    }
    // Options
    else if (pMenuCore->m_handler->m_active == 1) {
        Game_Action = GA_ENTER_MENU;
        Game_Action_Data_Middle.add("load_menu", int_to_string(MENU_OPTIONS));
    }
    // Load
    else if (pMenuCore->m_handler->m_active == 2) {
        Game_Action = GA_ENTER_MENU;
        Game_Action_Data_Middle.add("load_menu", int_to_string(MENU_LOAD));
    }
    // Save
    else if (pMenuCore->m_handler->m_active == 3) {
        Game_Action = GA_ENTER_MENU;
        Game_Action_Data_Middle.add("load_menu", int_to_string(MENU_SAVE));
    }
    // Quit
    else if (pMenuCore->m_handler->m_active == 4) {
        game_exit = 1;
    }
    // Credits
    else if (pMenuCore->m_handler->m_active == 5) {
        Game_Action = GA_ENTER_MENU;
        Game_Action_Data_Middle.add("load_menu", int_to_string(MENU_CREDITS));
        Game_Action_Data_Start.add("music_fadeout", "500");
    }

    if (m_exit_to_gamemode != MODE_NOTHING) {
        Game_Action_Data_Middle.add("menu_exit_back_to", int_to_string(m_exit_to_gamemode));
    }
}

void cMenu_Main::Draw(void)
{
    cMenu_Base::Draw();
    Draw_End();
}

/* *** *** *** *** *** *** *** *** cMenu_Start *** *** *** *** *** *** *** *** *** */

cMenu_Start::cMenu_Start(void)
    : cMenu_Base()
{

}

cMenu_Start::~cMenu_Start(void)
{

}

void cMenu_Start::Init(void)
{
    m_listbox_search_buffer_counter = 0.0f;

    cMenu_Base::Init();

    m_layout_file = "menu/start.layout";

    cHudSprite* hud_sprite = new cHudSprite(pMenuCore->m_handler->m_level->m_sprite_manager);
    hud_sprite->Set_Image(pVideo->Get_Surface("menu/start.png"));
    hud_sprite->Set_Pos(static_cast<float>(game_res_w) * 0.02f, 140);
    m_draw_list.push_back(hud_sprite);
    hud_sprite = new cHudSprite(pMenuCore->m_handler->m_level->m_sprite_manager);
    hud_sprite->Set_Image(pVideo->Get_Surface("menu/items/overworld.png"));
    hud_sprite->Set_Pos(static_cast<float>(game_res_w) / 20, 210);
    m_draw_list.push_back(hud_sprite);

    Init_GUI();
}

void cMenu_Start::Init_GUI(void)
{
    cMenu_Base::Init_GUI();

    // Tab Control
    CEGUI::TabControl* tabcontrol = static_cast<CEGUI::TabControl*>(CEGUI::WindowManager::getSingleton().getWindow("tabcontrol_main"));
    tabcontrol->activate();

    // events
    tabcontrol->subscribeEvent(CEGUI::TabControl::EventSelectionChanged, CEGUI::Event::Subscriber(&cMenu_Start::TabControl_Selection_Changed, this));
    tabcontrol->subscribeEvent(CEGUI::Window::EventKeyDown, CEGUI::Event::Subscriber(&cMenu_Start::TabControl_Keydown, this));

    // ### Campaign ###
    CEGUI::Listbox* listbox_campaigns = static_cast<CEGUI::Listbox*>(CEGUI::WindowManager::getSingleton().getWindow("listbox_campaigns"));

    // campaign names
    for (vector<cCampaign*>::const_iterator itr = pCampaign_Manager->objects.begin(); itr != pCampaign_Manager->objects.end(); ++itr) {
        const cCampaign* campaign = (*itr);

        CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(reinterpret_cast<const CEGUI::utf8*>(campaign->m_name.c_str()));
        // is in game dir
        if (campaign->m_user == 0) {
            item->setTextColours(CEGUI::colour(1, 0.8f, 0.6f));
        }
        // is in user dir
        else if (campaign->m_user == 1) {
            item->setTextColours(CEGUI::colour(0.8f, 1, 0.6f));
        }
        // is in both
        else if (campaign->m_user == 2) {
            // mix colors
            item->setTextColours(CEGUI::colour(0.8f, 1, 0.6f), CEGUI::colour(0.8f, 1, 0.6f), CEGUI::colour(1, 0.8f, 0.6f), CEGUI::colour(1, 0.8f, 0.6f));
        }

        item->setSelectionColours(CEGUI::colour(0.33f, 0.33f, 0.33f));
        item->setSelectionBrushImage("TaharezLook", "ListboxSelectionBrush");
        listbox_campaigns->addItem(item);
    }

    // events
    listbox_campaigns->subscribeEvent(CEGUI::Window::EventKeyDown, CEGUI::Event::Subscriber(&cMenu_Start::Listbox_Keydown, this));
    listbox_campaigns->subscribeEvent(CEGUI::Window::EventCharacterKey, CEGUI::Event::Subscriber(&cMenu_Start::Listbox_Character_Key, this));
    listbox_campaigns->subscribeEvent(CEGUI::Listbox::EventSelectionChanged, CEGUI::Event::Subscriber(&cMenu_Start::Campaign_Select, this));
    listbox_campaigns->subscribeEvent(CEGUI::Listbox::EventMouseDoubleClick, CEGUI::Event::Subscriber(&cMenu_Start::Campaign_Select_final_list, this));

    // select first item
    if (listbox_campaigns->getItemCount()) {
        listbox_campaigns->setItemSelectState(static_cast<size_t>(0), 1);
    }

    // ### World ###
    CEGUI::Listbox* listbox_worlds = static_cast<CEGUI::Listbox*>(CEGUI::WindowManager::getSingleton().getWindow("listbox_worlds"));

    // overworld names
    for (vector<cOverworld*>::const_iterator itr = pOverworld_Manager->objects.begin(); itr != pOverworld_Manager->objects.end(); ++itr) {
        const cOverworld_description* world = (*itr)->m_description;

// show all worlds in debug builds
#ifndef _DEBUG
        if (!world->m_visible) {
            continue;
        }
#else
        std::cout << "Showing invisible world '" << world->m_name << "' because this is a debug build." << std::endl;
#endif

        CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(reinterpret_cast<const CEGUI::utf8*>(world->m_name.c_str()));
        // is in game dir
        if (world->m_user == 0) {
            item->setTextColours(CEGUI::colour(1, 0.8f, 0.6f));
        }
        // is in user dir
        else if (world->m_user == 1) {
            item->setTextColours(CEGUI::colour(0.8f, 1, 0.6f));
        }
        // is in both
        else if (world->m_user == 2) {
            // mix colors
            item->setTextColours(CEGUI::colour(0.8f, 1, 0.6f), CEGUI::colour(0.8f, 1, 0.6f), CEGUI::colour(1, 0.8f, 0.6f), CEGUI::colour(1, 0.8f, 0.6f));
        }

        item->setSelectionColours(CEGUI::colour(0.33f, 0.33f, 0.33f));
        item->setSelectionBrushImage("TaharezLook", "ListboxSelectionBrush");
        listbox_worlds->addItem(item);
    }

    // events
    listbox_worlds->subscribeEvent(CEGUI::Window::EventKeyDown, CEGUI::Event::Subscriber(&cMenu_Start::Listbox_Keydown, this));
    listbox_worlds->subscribeEvent(CEGUI::Window::EventCharacterKey, CEGUI::Event::Subscriber(&cMenu_Start::Listbox_Character_Key, this));
    listbox_worlds->subscribeEvent(CEGUI::Listbox::EventSelectionChanged, CEGUI::Event::Subscriber(&cMenu_Start::World_Select, this));
    listbox_worlds->subscribeEvent(CEGUI::Listbox::EventMouseDoubleClick, CEGUI::Event::Subscriber(&cMenu_Start::World_Select_final_list, this));

    // select first item
    if (listbox_worlds->getItemCount()) {
        listbox_worlds->setItemSelectState(static_cast<size_t>(0), 1);
    }

    // ### Level ###
    CEGUI::Listbox* listbox_levels = static_cast<CEGUI::Listbox*>(CEGUI::WindowManager::getSingleton().getWindow("listbox_levels"));
    listbox_levels->setSortingEnabled(1);

    // get game level
    Get_Levels(pResource_Manager->Get_Game_Level_Directory(), CEGUI::colour(1, 0.8f, 0.6f));
    // get user level
    Get_Levels(pResource_Manager->Get_User_Level_Directory(), CEGUI::colour(0.8f, 1, 0.6f));

    // events
    listbox_levels->subscribeEvent(CEGUI::Listbox::EventSelectionChanged, CEGUI::Event::Subscriber(&cMenu_Start::Level_Select, this));
    listbox_levels->subscribeEvent(CEGUI::Listbox::EventMouseDoubleClick, CEGUI::Event::Subscriber(&cMenu_Start::Level_Select_Final_List, this));
    listbox_levels->subscribeEvent(CEGUI::Window::EventKeyDown, CEGUI::Event::Subscriber(&cMenu_Start::Listbox_Keydown, this));
    listbox_levels->subscribeEvent(CEGUI::Window::EventCharacterKey, CEGUI::Event::Subscriber(&cMenu_Start::Listbox_Character_Key, this));

    // Level Buttons
    CEGUI::PushButton* button_new = static_cast<CEGUI::PushButton*>(CEGUI::WindowManager::getSingleton().getWindow("button_level_new"));
    button_new->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&cMenu_Start::Button_Level_New_Clicked, this));
    CEGUI::PushButton* button_edit = static_cast<CEGUI::PushButton*>(CEGUI::WindowManager::getSingleton().getWindow("button_level_edit"));
    button_edit->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&cMenu_Start::Button_Level_Edit_Clicked, this));
    CEGUI::PushButton* button_delete = static_cast<CEGUI::PushButton*>(CEGUI::WindowManager::getSingleton().getWindow("button_level_delete"));
    button_delete->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&cMenu_Start::Button_Level_Delete_Clicked, this));

    // Button Enter
    CEGUI::PushButton* button_enter = static_cast<CEGUI::PushButton*>(CEGUI::WindowManager::getSingleton().getWindow("button_enter"));
    // Button back
    CEGUI::PushButton* button_back = static_cast<CEGUI::PushButton*>(CEGUI::WindowManager::getSingleton().getWindow("button_back"));

    // events
    button_enter->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&cMenu_Start::Button_Enter_Clicked, this));
    button_back->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&cMenu_Start::Button_Back_Clicked, this));

    // ### I18n ###
    // buttons
    button_new->setText(UTF8_("New"));
    button_edit->setText(UTF8_("Edit"));
    button_delete->setText(UTF8_("Delete"));
    button_enter->setText(UTF8_("Enter"));
    button_back->setText(UTF8_("Back"));

    // several texts
    CEGUI::Window* text = static_cast<CEGUI::Window*>(CEGUI::WindowManager::getSingleton().getWindow("text_level_select"));
    text->setText(UTF8_("Select Level"));
    text = static_cast<CEGUI::Window*>(CEGUI::WindowManager::getSingleton().getWindow("text_campaign_select"));
    text->setText(UTF8_("Select Campaign"));
    text = static_cast<CEGUI::Window*>(CEGUI::WindowManager::getSingleton().getWindow("text_world_select"));
    text->setText(UTF8_("Select Overworld"));
    text = static_cast<CEGUI::Window*>(CEGUI::WindowManager::getSingleton().getWindow("text_campaign_description"));
    text->setText(UTF8_("Description"));
    text = static_cast<CEGUI::Window*>(CEGUI::WindowManager::getSingleton().getWindow("text_world_description"));
    text->setText(UTF8_("Description"));

    text = static_cast<CEGUI::Window*>(CEGUI::WindowManager::getSingleton().getWindow("text_level_info"));
    // TRANS: The colour names refer to the colours the level names can
    // TRANS: be in. "Game" means the level is shipped by the game,
    // TRANS: "user" means the level has been created by the user.
    // TRANS: If the user edited a system level, it gets copied to his
    // TRANS: personal level directory and is coloured mixedly to indicate
    // TRANS: that. "Deprecated" are levels from very old versions
    // TRANS: of the game.
    text->setText(UTF8_("- Level Colors -\n"
                      "\n"
                      "Orange: Game\n"
                      "Green: User\n"
                      "Grey: Deprecated\n"
                      "Mixed: See the colors"));

    // Set focus
    listbox_worlds->activate();
}

void cMenu_Start::Exit(void)
{
    Game_Action = GA_ENTER_MENU;
    Game_Action_Data_Middle.add("load_menu", int_to_string(MENU_MAIN));
    if (m_exit_to_gamemode != MODE_NOTHING) {
        Game_Action_Data_Middle.add("menu_exit_back_to", int_to_string(m_exit_to_gamemode));
    }
}

void cMenu_Start::Update(void)
{
    // if search buffer is active
    if (m_listbox_search_buffer_counter > 0.0f) {
        m_listbox_search_buffer_counter -= pFramerate->m_speed_factor;

        // if time limit reached search buffer is abandoned
        if (m_listbox_search_buffer_counter <= 0.0f) {
            m_listbox_search_buffer_counter = 0.0f;
            m_listbox_search_buffer.clear();
        }
    }

    cMenu_Base::Update();

    if (!m_action) {
        return;
    }

    m_action = 0;

    // enter
    Load_Selected();
}

void cMenu_Start::Draw(void)
{
    cMenu_Base::Draw();
    Draw_End();
}

void cMenu_Start::Get_Levels(fs::path dir, CEGUI::colour color)
{
    // Level Listbox
    CEGUI::Listbox* listbox_levels = static_cast<CEGUI::Listbox*>(CEGUI::WindowManager::getSingleton().getWindow("listbox_levels"));

    // get all files
    vector<fs::path> lvl_files = Get_Directory_Files(dir, ".smclvl", false, false);

    // list all available levels
    for (vector<fs::path>::iterator itr = lvl_files.begin(); itr != lvl_files.end(); ++itr) {
        // get filename without base directory
        fs::path lvl_path = (*itr).filename();

        // erase file extension only if smclvl
        if (lvl_path.extension() == fs::path(".smclvl"))
            lvl_path = lvl_path.stem();

        // create listbox item
        std::string lvl_name = path_to_utf8(lvl_path);
        CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(reinterpret_cast<const CEGUI::utf8*>(lvl_name.c_str()));
        item->setTextColours(color);

        // check if item with the same name already exists
        CEGUI::ListboxTextItem* item_old = static_cast<CEGUI::ListboxTextItem*>(listbox_levels->findItemWithText(lvl_name, NULL));

        if (item_old) {
            // mix colors
            item->setTextColours(item->getTextColours().d_top_left, item->getTextColours().d_top_right, item_old->getTextColours().d_bottom_left, item_old->getTextColours().d_bottom_right);
            // remove old item
            listbox_levels->removeItem(item_old);
        }


        item->setSelectionColours(CEGUI::colour(0.33f, 0.33f, 0.33f));
        item->setSelectionBrushImage("TaharezLook", "ListboxSelectionBrush");
        listbox_levels->addItem(item);
    }
}

bool cMenu_Start::Highlight_Level(std::string lvl_name)
{
    if (lvl_name.empty()) {
        return 0;
    }

    // get tab control
    CEGUI::TabControl* tabcontrol = static_cast<CEGUI::TabControl*>(CEGUI::WindowManager::getSingleton().getWindow("tabcontrol_main"));
    // select level tab
    tabcontrol->setSelectedTab("tab_level");

    // get levels listbox
    CEGUI::Listbox* listbox_levels = static_cast<CEGUI::Listbox*>(CEGUI::WindowManager::getSingleton().getWindow("listbox_levels"));
    // get item
    CEGUI::ListboxItem* list_item = listbox_levels->findItemWithText(lvl_name, NULL);
    // select level
    if (list_item) {
        listbox_levels->setItemSelectState(list_item, 1);
        listbox_levels->ensureItemIsVisible(list_item);
    }
    else {
        return 0;
    }

    return 1;
}

void cMenu_Start::Load_Selected(void)
{
    // Get Tab Control
    CEGUI::TabControl* tabcontrol = static_cast<CEGUI::TabControl*>(CEGUI::WindowManager::getSingleton().getWindow("tabcontrol_main"));

    // Campaign
    if (tabcontrol->getSelectedTabIndex() == 0) {
        CEGUI::ListboxItem* item = (static_cast<CEGUI::Listbox*>(CEGUI::WindowManager::getSingleton().getWindow("listbox_campaigns")))->getFirstSelectedItem();

        if (item) {
            Load_Campaign(item->getText().c_str());
        }
    }
    // World
    else if (tabcontrol->getSelectedTabIndex() == 1) {
        CEGUI::ListboxItem* item = (static_cast<CEGUI::Listbox*>(CEGUI::WindowManager::getSingleton().getWindow("listbox_worlds")))->getFirstSelectedItem();

        if (item) {
            Load_World(item->getText().c_str());
        }
    }
    // Level
    else {
        CEGUI::ListboxItem* item = (static_cast<CEGUI::Listbox*>(CEGUI::WindowManager::getSingleton().getWindow("listbox_levels")))->getFirstSelectedItem();

        if (item) {
            Load_Level(item->getText().c_str());
        }
    }
}

void cMenu_Start::Load_Campaign(std::string name)
{
    if (pLevel_Player->m_points > 0 && !Box_Question(_("This will reset your current progress.\nContinue ?"))) {
        return;
    }

    cCampaign* new_campaign = pCampaign_Manager->Get_from_Name(name);

    // if not available
    if (!new_campaign) {
        pHud_Debug->Set_Text(_("Couldn't load campaign ") + name, static_cast<float>(speedfactor_fps));
    }
    else {
        // enter level
        if (new_campaign->m_is_target_level) {
            Game_Action = GA_ENTER_LEVEL;
            Game_Mode_Type = MODE_TYPE_LEVEL_CUSTOM;
            Game_Action_Data_Middle.add("load_level", new_campaign->m_target.c_str());
        }
        // enter world
        else {
            Game_Action = GA_ENTER_WORLD;
            Game_Action_Data_Middle.add("enter_world", new_campaign->m_target.c_str());
        }

        Game_Action_Data_Start.add("music_fadeout", "1000");
        Game_Action_Data_Start.add("screen_fadeout", CEGUI::PropertyHelper::intToString(EFFECT_OUT_BLACK));
        Game_Action_Data_Start.add("screen_fadeout_speed", "3");
        Game_Action_Data_Middle.add("unload_menu", "1");
        Game_Action_Data_Middle.add("reset_save", "1");
        Game_Action_Data_End.add("screen_fadein", CEGUI::PropertyHelper::intToString(EFFECT_IN_RANDOM));
        Game_Action_Data_End.add("screen_fadein_speed", "3");
    }
}

void cMenu_Start::Load_World(std::string name)
{
    if (pLevel_Player->m_points > 0 && !Box_Question(_("This will reset your current progress.\nContinue ?"))) {
        return;
    }

    cOverworld* new_world = pOverworld_Manager->Get_from_Name(name);

    // if not available
    if (!new_world) {
        pHud_Debug->Set_Text(_("Couldn't load overworld ") + name, static_cast<float>(speedfactor_fps));
    }
    else {
        // enter world
        Game_Action = GA_ENTER_WORLD;
        Game_Action_Data_Start.add("music_fadeout", "1000");
        Game_Action_Data_Start.add("screen_fadeout", CEGUI::PropertyHelper::intToString(EFFECT_OUT_BLACK));
        Game_Action_Data_Start.add("screen_fadeout_speed", "3");
        Game_Action_Data_Middle.add("enter_world", name.c_str());
        Game_Action_Data_Middle.add("unload_menu", "1");
        Game_Action_Data_Middle.add("reset_save", "1");
        Game_Action_Data_End.add("screen_fadein", CEGUI::PropertyHelper::intToString(EFFECT_IN_RANDOM));
        Game_Action_Data_End.add("screen_fadein_speed", "3");
    }
}

bool cMenu_Start::Load_Level(std::string level_name)
{
    if (pLevel_Player->m_points > 0 && !Box_Question(_("This will reset your current progress.\nContinue ?"))) {
        return 0;
    }

    // if not available
    boost::filesystem::path level_path = pLevel_Manager->Get_Path(level_name);
    if (level_path.empty()) {
        pAudio->Play_Sound("error.ogg");
        pHud_Debug->Set_Text(_("Couldn't load level ") + level_name, static_cast<float>(speedfactor_fps));
        return 0;
    }

    // Disable editor (does nothing if already disabled)
    pLevel_Editor->Disable();

    // enter level
    Game_Action = GA_ENTER_LEVEL;
    Game_Mode_Type = MODE_TYPE_LEVEL_CUSTOM;
    Game_Action_Data_Start.add("music_fadeout", "1000");
    Game_Action_Data_Start.add("screen_fadeout", CEGUI::PropertyHelper::intToString(EFFECT_OUT_BLACK));
    Game_Action_Data_Start.add("screen_fadeout_speed", "3");
    Game_Action_Data_Middle.add("load_level", level_name.c_str());
    Game_Action_Data_Middle.add("unload_menu", "1");
    Game_Action_Data_Middle.add("reset_save", "1");
    Game_Action_Data_End.add("screen_fadein", CEGUI::PropertyHelper::intToString(EFFECT_IN_RANDOM));
    Game_Action_Data_End.add("screen_fadein_speed", "3");

    return 1;
}

bool cMenu_Start::TabControl_Selection_Changed(const CEGUI::EventArgs& e)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(e);
    CEGUI::TabControl* tabcontrol = static_cast<CEGUI::TabControl*>(windowEventArgs.window);

    if (tabcontrol->getSelectedTabIndex() == 0) {
        static_cast<CEGUI::Listbox*>(CEGUI::WindowManager::getSingleton().getWindow("listbox_campaigns"))->activate();
    }
    else if (tabcontrol->getSelectedTabIndex() == 1) {
        static_cast<CEGUI::Listbox*>(CEGUI::WindowManager::getSingleton().getWindow("listbox_worlds"))->activate();
    }
    else if (tabcontrol->getSelectedTabIndex() == 2) {
        static_cast<CEGUI::Listbox*>(CEGUI::WindowManager::getSingleton().getWindow("listbox_levels"))->activate();
    }

    return 1;
}

bool cMenu_Start::TabControl_Keydown(const CEGUI::EventArgs& e)
{
    const CEGUI::KeyEventArgs& ke = static_cast<const CEGUI::KeyEventArgs&>(e);

    // Return
    if (ke.scancode == CEGUI::Key::Return || ke.scancode == CEGUI::Key::NumpadEnter) {
        Load_Selected();
        return 1;
    }
    // Left (todo: only for joystick when CEGUI supports these events)
    else if (ke.scancode == pKeyboard->SDLKey_to_CEGUIKey(pPreferences->m_key_left)) {
        // Get Tab Control
        CEGUI::TabControl* tabcontrol = static_cast<CEGUI::TabControl*>(CEGUI::WindowManager::getSingleton().getWindow("tabcontrol_main"));

        // if not first tab
        if (tabcontrol->getSelectedTabIndex() != 0) {
            tabcontrol->setSelectedTabAtIndex(tabcontrol->getSelectedTabIndex() - 1);
        }

        return 1;
    }
    // Right (todo: only for joystick when CEGUI supports these events)
    else if (ke.scancode == pKeyboard->SDLKey_to_CEGUIKey(pPreferences->m_key_right)) {
        // Get Tab Control
        CEGUI::TabControl* tabcontrol = static_cast<CEGUI::TabControl*>(CEGUI::WindowManager::getSingleton().getWindow("tabcontrol_main"));

        // if not last tab
        if (tabcontrol->getSelectedTabIndex() + 1 != tabcontrol->getTabCount()) {
            tabcontrol->setSelectedTabAtIndex(tabcontrol->getSelectedTabIndex() + 1);
        }

        return 1;
    }
    // Shift Tab
    else if (pKeyboard->Is_Shift_Down() && ke.scancode == CEGUI::Key::Tab) {
        // Get Tab Control
        CEGUI::TabControl* tabcontrol = static_cast<CEGUI::TabControl*>(CEGUI::WindowManager::getSingleton().getWindow("tabcontrol_main"));

        // if last tab
        if (tabcontrol->getSelectedTabIndex() + 1 == tabcontrol->getTabCount()) {
            tabcontrol->setSelectedTabAtIndex(0);
        }
        // select next tab
        else {
            tabcontrol->setSelectedTabAtIndex(tabcontrol->getSelectedTabIndex() + 1);
        }

        return 1;
    }

    return 0;
}

bool cMenu_Start::Listbox_Keydown(const CEGUI::EventArgs& e)
{
    const CEGUI::KeyEventArgs& ke = static_cast<const CEGUI::KeyEventArgs&>(e);

    // Get the Listbox
    CEGUI::Listbox* listbox = static_cast<CEGUI::Listbox*>(ke.window);

    // Down/Up (todo: detect event for joystick properly when CEGUI supports these events)
    if (ke.scancode == CEGUI::Key::ArrowDown || ke.scancode == CEGUI::Key::ArrowUp || ke.scancode == CEGUI::Key::PageDown || ke.scancode == CEGUI::Key::PageUp ||
            ke.scancode == CEGUI::Key::Home || ke.scancode == CEGUI::Key::End ||
            ke.scancode == pKeyboard->SDLKey_to_CEGUIKey(pPreferences->m_key_up) || ke.scancode == pKeyboard->SDLKey_to_CEGUIKey(pPreferences->m_key_down)) {
        int new_selected = 0;
        int last_selected = 0;

        // get selected item
        CEGUI::ListboxItem* last_selected_item = listbox->getFirstSelectedItem();

        // if something is selected
        if (last_selected_item != NULL) {
            last_selected = listbox->getItemIndex(last_selected_item);
        }

        // down (todo: detect event for joystick properly when CEGUI supports these events)
        if (ke.scancode == CEGUI::Key::ArrowDown || ke.scancode == pKeyboard->SDLKey_to_CEGUIKey(pPreferences->m_key_down)) {
            new_selected = last_selected + 1;
        }
        // up (todo: detect event for joystick properly when CEGUI supports these events)
        else if (ke.scancode == CEGUI::Key::ArrowUp || ke.scancode == pKeyboard->SDLKey_to_CEGUIKey(pPreferences->m_key_up)) {
            new_selected = last_selected - 1;
        }
        // page down
        else if (ke.scancode == CEGUI::Key::PageDown) {
            // todo : should skip all visible items
            new_selected = last_selected + 10;
        }
        // page up
        else if (ke.scancode == CEGUI::Key::PageUp) {
            // todo : should skip all visible items
            new_selected = last_selected - 10;
        }
        // home
        else if (ke.scancode == CEGUI::Key::Home) {
            new_selected = 0;
        }
        // end
        else if (ke.scancode == CEGUI::Key::End) {
            new_selected = listbox->getItemCount() - 1;
        }

        // if after last item
        if (new_selected >= static_cast<int>(listbox->getItemCount())) {
            // select first
            if (last_selected == static_cast<int>(listbox->getItemCount()) - 1) {
                new_selected = 0;
            }
            // select last
            else {
                new_selected = listbox->getItemCount() - 1;
            }
        }
        // if before first item
        else if (new_selected < 0) {
            // select last
            if (last_selected == 0) {
                new_selected = listbox->getItemCount() - 1;
            }
            // select first
            else {
                new_selected = 0;
            }
        }

        listbox->setItemSelectState(new_selected, 1);
        listbox->ensureItemIsVisible(new_selected);

        return 1;
    }

    return 0;
}

bool cMenu_Start::Listbox_Character_Key(const CEGUI::EventArgs& e)
{
    const CEGUI::KeyEventArgs& ke = static_cast<const CEGUI::KeyEventArgs&>(e);

    // Get the Listbox
    CEGUI::Listbox* listbox = static_cast<CEGUI::Listbox*>(ke.window);

    if (listbox->getFont()->isCodepointAvailable(ke.codepoint)) {
        m_listbox_search_buffer_counter = speedfactor_fps;
        m_listbox_search_buffer.insert(m_listbox_search_buffer.end(), 1, ke.codepoint);

        // new selected if found
        CEGUI::ListboxItem* new_selected = NULL;

        // search the list
        size_t index = 0;

        while (index < listbox->getItemCount()) {
            CEGUI::ListboxItem* item = listbox->getListboxItemFromIndex(index);

            // found
            if (item->getText().substr(0, m_listbox_search_buffer.length()).compare(m_listbox_search_buffer) == 0) {
                new_selected = item;
                break;
            }
            // no match
            else {
                index++;
            }
        }

        // set new item selected
        if (new_selected) {
            listbox->setItemSelectState(new_selected, 1);
            listbox->ensureItemIsVisible(new_selected);
        }
    }

    return 0;
}

bool cMenu_Start::Campaign_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Listbox*>(windowEventArgs.window)->getFirstSelectedItem();

    // description
    CEGUI::Editbox* editbox_campaign_description = static_cast<CEGUI::Editbox*>(CEGUI::WindowManager::getSingleton().getWindow("editbox_campaign_description"));

    // set description
    if (item) {
        // todo : should be from the filename not name (more unique)
        editbox_campaign_description->setText(reinterpret_cast<const CEGUI::utf8*>(pCampaign_Manager->Get_from_Name(item->getText().c_str())->m_description.c_str()));
    }
    // clear
    else {
        editbox_campaign_description->setText("");
    }

    return 1;
}

bool cMenu_Start::Campaign_Select_final_list(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Listbox*>(windowEventArgs.window)->getFirstSelectedItem();

    // load campaign
    if (item) {
        Load_Campaign(item->getText().c_str());
    }

    return 1;
}

bool cMenu_Start::World_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Listbox*>(windowEventArgs.window)->getFirstSelectedItem();

    // description
    CEGUI::Editbox* editbox_world_description = static_cast<CEGUI::Editbox*>(CEGUI::WindowManager::getSingleton().getWindow("editbox_world_description"));

    // set description
    if (item) {
        // todo : should be from the path not name (more unique)
        editbox_world_description->setText(reinterpret_cast<const CEGUI::utf8*>(pOverworld_Manager->Get_from_Name(item->getText().c_str())->m_description->m_comment.c_str()));
    }
    // clear
    else {
        editbox_world_description->setText("");
    }

    return 1;
}

bool cMenu_Start::World_Select_final_list(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Listbox*>(windowEventArgs.window)->getFirstSelectedItem();

    // load world
    if (item) {
        Load_World(item->getText().c_str());
    }

    return 1;
}

bool cMenu_Start::Level_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Listbox*>(windowEventArgs.window)->getFirstSelectedItem();

    // set level something
    if (item) {
        // todo : needs level manager
    }
    // clear
    else {
        // todo : needs level manager
    }

    return 1;
}

bool cMenu_Start::Level_Select_Final_List(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Listbox*>(windowEventArgs.window)->getFirstSelectedItem();

    // load level
    if (item) {
        Load_Level(item->getText().c_str());
    }

    return 1;
}


bool cMenu_Start::Button_Level_New_Clicked(const CEGUI::EventArgs& event)
{
    if (!pLevel_Editor->Function_New()) {
        // aborted/failed
        return 0;
    }

    Game_Mode_Type = MODE_TYPE_LEVEL_CUSTOM;
    Game_Action_Data_Middle.add("unload_menu", "1");
    Game_Action_Data_Middle.add("reset_save", "1");
    Game_Action_Data_End.add("activate_editor", "1");

    return 1;
}

bool cMenu_Start::Button_Level_Edit_Clicked(const CEGUI::EventArgs& event)
{
    // Get Selected Level
    CEGUI::Listbox* listbox_levels = static_cast<CEGUI::Listbox*>(CEGUI::WindowManager::getSingleton().getWindow("listbox_levels"));
    CEGUI::ListboxItem* item = listbox_levels->getFirstSelectedItem();

    // load level
    if (item && Load_Level(item->getText().c_str())) {
        Game_Action_Data_End.add("activate_editor", "1");
    }

    return 1;
}

bool cMenu_Start::Button_Level_Delete_Clicked(const CEGUI::EventArgs& event)
{
    // Get Selected Level
    CEGUI::Listbox* listbox_levels = static_cast<CEGUI::Listbox*>(CEGUI::WindowManager::getSingleton().getWindow("listbox_levels"));
    CEGUI::ListboxItem* item = listbox_levels->getFirstSelectedItem();

    // load level
    if (item) {
        std::string filename = item->getText().c_str();

        // if denied
        if (!Box_Question(_("Delete ") + filename + " ?")) {
            return 1;
        }

        // only user directory
        fs::path filepath = pLevel_Manager->Get_Path(filename, true);
        if (!filepath.empty()) {
            fs::remove(filename);
            listbox_levels->removeItem(item);
        }
    }

    return 1;
}

bool cMenu_Start::Button_Enter_Clicked(const CEGUI::EventArgs& event)
{
    Load_Selected();
    return 1;
}

bool cMenu_Start::Button_Back_Clicked(const CEGUI::EventArgs& event)
{
    Exit();
    return 1;
}

/* *** *** *** *** *** *** *** *** cMenu_Options *** *** *** *** *** *** *** *** *** */

cMenu_Options::cMenu_Options(void)
    : cMenu_Base()
{

}

cMenu_Options::~cMenu_Options(void)
{

}

void cMenu_Options::Init(void)
{
    cMenu_Base::Init();
    m_layout_file = "menu/options_main.layout";

    // video settings
    m_vid_w = pPreferences->m_video_screen_w;
    m_vid_h = pPreferences->m_video_screen_h;
    m_vid_bpp = pPreferences->m_video_screen_bpp;
    m_vid_fullscreen = pPreferences->m_video_fullscreen;
    m_vid_vsync = pPreferences->m_video_vsync;
    m_vid_geometry_detail = pVideo->m_geometry_quality;
    m_vid_texture_detail = pVideo->m_texture_quality;

    cMenu_Item* temp_item = NULL;

    // options image
    cHudSprite* hud_sprite = new cHudSprite(pMenuCore->m_handler->m_level->m_sprite_manager);
    hud_sprite->Set_Image(pVideo->Get_Surface("menu/options.png"));
    hud_sprite->Set_Pos(game_res_w * 0.01f, 100);
    m_draw_list.push_back(hud_sprite);

    Init_GUI();
}

void cMenu_Options::Init_GUI(void)
{
    cMenu_Base::Init_GUI();

    // get the CEGUI window manager
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    // back button
    CEGUI::PushButton* button_back = static_cast<CEGUI::PushButton*>(wmgr.getWindow("button_back"));
    button_back->setText(UTF8_("Back"));
    button_back->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&cMenu_Options::Button_Back_Click, this));

    // Tab Control
    m_tabcontrol = static_cast<CEGUI::TabControl*>(wmgr.getWindow("tabcontrol_main"));
    // tab game
    CEGUI::Window* tabwindow = wmgr.loadWindowLayout("menu/tab_game.layout");
    m_tabcontrol->addTab(tabwindow);
    // tab video
    tabwindow = wmgr.loadWindowLayout("menu/tab_video.layout");
    m_tabcontrol->addTab(tabwindow);
    // tab audio
    tabwindow = wmgr.loadWindowLayout("menu/tab_audio.layout");
    m_tabcontrol->addTab(tabwindow);
    // tab keyboard
    tabwindow = wmgr.loadWindowLayout("menu/tab_keyboard.layout");
    m_tabcontrol->addTab(tabwindow);
    // tab joystick
    tabwindow = wmgr.loadWindowLayout("menu/tab_joystick.layout");
    m_tabcontrol->addTab(tabwindow);
    // tab editor
    tabwindow = wmgr.loadWindowLayout("menu/tab_editor.layout");
    m_tabcontrol->addTab(tabwindow);

    Init_GUI_Game();
    Init_GUI_Video();
    Init_GUI_Audio();
    Init_GUI_Keyboard();
    Init_GUI_Joystick();
    Init_GUI_Editor();
}

void cMenu_Options::Init_GUI_Game(void)
{
    // get the CEGUI window manager
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    // always run
    CEGUI::Window* text_always_run = static_cast<CEGUI::Window*>(CEGUI::WindowManager::getSingleton().getWindow("game_text_always_run"));
    text_always_run->setText(UTF8_("Always Run"));

    m_game_combo_always_run = static_cast<CEGUI::Combobox*>(wmgr.getWindow("game_combo_always_run"));

    CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(UTF8_("On"));
    item->setTextColours(CEGUI::colour(0, 1, 0));
    m_game_combo_always_run->addItem(item);
    item = new CEGUI::ListboxTextItem(UTF8_("Off"));
    item->setTextColours(CEGUI::colour(0, 0, 1));
    m_game_combo_always_run->addItem(item);

    if (pPreferences->m_always_run) {
        m_game_combo_always_run->setText(UTF8_("On"));
    }
    else {
        m_game_combo_always_run->setText(UTF8_("Off"));
    }

    m_game_combo_always_run->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cMenu_Options::Game_Always_Run_Select, this));

    // Camera Horizontal Speed
    CEGUI::Window* text_camera_hor_speed = static_cast<CEGUI::Window*>(CEGUI::WindowManager::getSingleton().getWindow("game_text_camera_hor_speed"));
    text_camera_hor_speed->setText(UTF8_("Camera Hor Speed"));

    m_game_spinner_camera_hor_speed = static_cast<CEGUI::Spinner*>(wmgr.getWindow("game_spinner_camera_hor_speed"));
    m_game_spinner_camera_hor_speed->setCurrentValue(pLevel_Manager->m_camera->m_hor_offset_speed);

    m_game_spinner_camera_hor_speed->subscribeEvent(CEGUI::Spinner::EventValueChanged, CEGUI::Event::Subscriber(&cMenu_Options::Game_Camera_Hor_Select, this));

    // Camera Vertical Speed
    CEGUI::Window* text_camera_ver_speed = static_cast<CEGUI::Window*>(CEGUI::WindowManager::getSingleton().getWindow("game_text_camera_ver_speed"));
    text_camera_ver_speed->setText(UTF8_("Camera Ver Speed"));

    m_game_spinner_camera_ver_speed = static_cast<CEGUI::Spinner*>(wmgr.getWindow("game_spinner_camera_ver_speed"));
    m_game_spinner_camera_ver_speed->setCurrentValue(pLevel_Manager->m_camera->m_ver_offset_speed);

    m_game_spinner_camera_ver_speed->subscribeEvent(CEGUI::Spinner::EventValueChanged, CEGUI::Event::Subscriber(&cMenu_Options::Game_Camera_Ver_Select, this));

    // language
    CEGUI::Window* text_language = static_cast<CEGUI::Window*>(CEGUI::WindowManager::getSingleton().getWindow("game_text_language"));
    text_language->setText(UTF8_("Language"));

    m_game_combo_language = static_cast<CEGUI::Combobox*>(wmgr.getWindow("game_combo_language"));

    item = new CEGUI::ListboxTextItem(UTF8_("default"));
    item->setTextColours(CEGUI::colour(0, 1, 0));
    m_game_combo_language->addItem(item);

    // get available languages
    vector<fs::path> language_files = Get_Directory_Files(pResource_Manager->Get_Game_Translation_Directory(), ".none", true, false);
    // add english as it is the base language and not in the translation directory
    language_files.push_back(pResource_Manager->Get_Game_Translation("en"));

    for (vector<fs::path>::iterator itr = language_files.begin(); itr != language_files.end(); ++itr) {
        // get filename
        fs::path this_path = (*itr);

        // if not directory
        if (!fs::is_directory(this_path))
            continue;

        item = new CEGUI::ListboxTextItem(path_to_utf8(this_path.filename()));
        item->setTextColours(CEGUI::colour(0, 0, 1));
        m_game_combo_language->addItem(item);
    }

    if (pPreferences->m_language.empty()) {
        m_game_combo_language->setText(UTF8_("default"));
    }
    else {
        m_game_combo_language->setText(pPreferences->m_language);
    }

    m_game_combo_language->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cMenu_Options::Game_Language_Select, this));

    // menu level
    CEGUI::Window* text_menu_level = static_cast<CEGUI::Window*>(CEGUI::WindowManager::getSingleton().getWindow("game_text_menu_level"));
    text_menu_level->setText(UTF8_("Menu Level"));

    m_game_combo_menu_level = static_cast<CEGUI::Combobox*>(wmgr.getWindow("game_combo_menu_level"));

    m_game_combo_menu_level->addItem(new CEGUI::ListboxTextItem("menu_green_1"));
    m_game_combo_menu_level->addItem(new CEGUI::ListboxTextItem("menu_blue_1"));
    m_game_combo_menu_level->addItem(new CEGUI::ListboxTextItem("menu_brown_1"));

    m_game_combo_menu_level->setText(pPreferences->m_menu_level);

    m_game_combo_menu_level->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cMenu_Options::Game_Menu_Level_Select, this));
    m_game_combo_menu_level->getEditbox()->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cMenu_Options::Game_Menu_Level_Text_Changed, this));

    // Reset Game
    CEGUI::PushButton* button_reset_game = static_cast<CEGUI::PushButton*>(CEGUI::WindowManager::getSingleton().getWindow("game_button_reset"));
    button_reset_game->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&cMenu_Options::Game_Button_Reset_Game_Clicked, this));
    button_reset_game->setText(UTF8_("Reset"));
}

void cMenu_Options::Init_GUI_Video(void)
{
    // get the CEGUI window manager
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    // Resolution
    CEGUI::Window* text_resolution = static_cast<CEGUI::Window*>(wmgr.getWindow("video_text_resolution"));
    text_resolution->setText(UTF8_("Resolution"));

    m_video_combo_resolution = static_cast<CEGUI::Combobox*>(wmgr.getWindow("video_combo_resolution"));

    vector<cSize_Int> valid_resolutions = pVideo->Get_Supported_Resolutions();
    CEGUI::ListboxTextItem* item;

    // add to listbox
    for (vector<cSize_Int>::iterator itr = valid_resolutions.begin(); itr != valid_resolutions.end(); ++itr) {
        // get resolution
        cSize_Int res = (*itr);

        if (res.m_width <= 0 || res.m_height <= 0) {
            continue;
        }

        // calculate aspect ratio
        float ar = static_cast<float>(res.m_width) / static_cast<float>(res.m_height);

        item = new CEGUI::ListboxTextItem(int_to_string(res.m_width) + "x" + int_to_string(res.m_height));
        CEGUI::colour color(0, 0, 0);
        // if a badly stretched resolution, display it in red
        if (ar < 1.1f || ar > 1.5f) {
            color.setGreen(0);
            color.setRed(1);
        }
        // good resolution
        else {
            // calculate difference from a default 1.333 resolution
            float diff_from_default;

            if (ar > 1.333f) {
                diff_from_default = (ar - 1.333f) * 4;
            }
            else {
                diff_from_default = -(ar - 1.333f) * 4;
            }

            color.setGreen(1 - diff_from_default);
            color.setRed(diff_from_default);
        }
        item->setTextColours(color);
        m_video_combo_resolution->addItem(item);
    }

    std::string temp = int_to_string(pPreferences->m_video_screen_w) + "x" + int_to_string(pPreferences->m_video_screen_h);
    m_video_combo_resolution->setText(temp.c_str());

    m_video_combo_resolution->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cMenu_Options::Video_Resolution_Select, this));

    // Bpp
    CEGUI::Window* text_bpp = static_cast<CEGUI::Window*>(wmgr.getWindow("video_text_bpp"));
    text_bpp->setText(UTF8_("Bpp"));

    m_video_combo_bpp = static_cast<CEGUI::Combobox*>(wmgr.getWindow("video_combo_bpp"));

    item = new CEGUI::ListboxTextItem("16");
    item->setTextColours(CEGUI::colour(1, 0.6f, 0.3f));
    m_video_combo_bpp->addItem(item);
    item = new CEGUI::ListboxTextItem("32");
    item->setTextColours(CEGUI::colour(0, 1, 0));
    m_video_combo_bpp->addItem(item);

    m_video_combo_bpp->setText(int_to_string(pPreferences->m_video_screen_bpp));

    m_video_combo_bpp->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cMenu_Options::Video_Bpp_Select, this));

    // Fullscreen
    CEGUI::Window* text_fullscreen = static_cast<CEGUI::Window*>(wmgr.getWindow("video_text_fullscreen"));
    text_fullscreen->setText(UTF8_("Fullscreen"));

    m_video_combo_fullscreen = static_cast<CEGUI::Combobox*>(wmgr.getWindow("video_combo_fullscreen"));

    item = new CEGUI::ListboxTextItem(UTF8_("On"));
    item->setTextColours(CEGUI::colour(0, 1, 0));
    m_video_combo_fullscreen->addItem(item);
    item = new CEGUI::ListboxTextItem(UTF8_("Off"));
    item->setTextColours(CEGUI::colour(0, 0, 1));
    m_video_combo_fullscreen->addItem(item);

    if (pPreferences->m_video_fullscreen) {
        m_video_combo_fullscreen->setText(UTF8_("On"));
    }
    else {
        m_video_combo_fullscreen->setText(UTF8_("Off"));
    }

    m_video_combo_fullscreen->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cMenu_Options::Video_Fullscreen_Select, this));

    // VSync
    CEGUI::Window* text_vsync = static_cast<CEGUI::Window*>(wmgr.getWindow("video_text_vsync"));
    text_vsync->setText(UTF8_("VSync"));

    m_video_combo_vsync = static_cast<CEGUI::Combobox*>(wmgr.getWindow("video_combo_vsync"));

    item = new CEGUI::ListboxTextItem(UTF8_("On"));
    item->setTextColours(CEGUI::colour(0, 1, 0));
    m_video_combo_vsync->addItem(item);
    item = new CEGUI::ListboxTextItem(UTF8_("Off"));
    item->setTextColours(CEGUI::colour(0, 0, 1));
    m_video_combo_vsync->addItem(item);

    if (pPreferences->m_video_vsync) {
        m_video_combo_vsync->setText(UTF8_("On"));
    }
    else {
        m_video_combo_vsync->setText(UTF8_("Off"));
    }

    m_video_combo_vsync->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cMenu_Options::Video_Vsync_Select, this));

    // FPS Limit
    CEGUI::Window* text_fps_limit = static_cast<CEGUI::Window*>(wmgr.getWindow("video_text_fps_limit"));
    text_fps_limit->setText(UTF8_("FPS Limit"));

    m_video_spinner_fps_limit = static_cast<CEGUI::Spinner*>(wmgr.getWindow("video_spinner_fps_limit"));
    m_video_spinner_fps_limit->setCurrentValue(pPreferences->m_video_fps_limit);

    m_video_spinner_fps_limit->subscribeEvent(CEGUI::Spinner::EventValueChanged, CEGUI::Event::Subscriber(&cMenu_Options::Video_FPS_Limit_Select, this));

    // Geometry quality
    CEGUI::Window* text_geometry_quality = static_cast<CEGUI::Window*>(wmgr.getWindow("video_text_geometry_quality"));
    text_geometry_quality->setText(UTF8_("Geometry Quality"));

    m_video_slider_geometry_quality = static_cast<CEGUI::Slider*>(wmgr.getWindow("video_slider_geometry_quality"));
    m_video_slider_geometry_quality->setCurrentValue(pVideo->m_geometry_quality);
    m_video_slider_geometry_quality->subscribeEvent(CEGUI::Slider::EventValueChanged, CEGUI::Event::Subscriber(&cMenu_Options::Video_Slider_Geometry_Quality_Changed, this));

    // Texture quality
    CEGUI::Window* text_texture_quality = static_cast<CEGUI::Window*>(wmgr.getWindow("video_text_texture_quality"));
    text_texture_quality->setText(UTF8_("Texture Quality"));

    m_video_slider_texture_quality = static_cast<CEGUI::Slider*>(wmgr.getWindow("video_slider_texture_quality"));
    m_video_slider_texture_quality->setCurrentValue(pVideo->m_texture_quality);
    m_video_slider_texture_quality->subscribeEvent(CEGUI::Slider::EventValueChanged, CEGUI::Event::Subscriber(&cMenu_Options::Video_Slider_Texture_Quality_Changed, this));

    // Reset
    CEGUI::PushButton* button_reset = static_cast<CEGUI::PushButton*>(wmgr.getWindow("video_button_reset"));
    button_reset->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&cMenu_Options::Video_Button_Reset_Clicked, this));
    button_reset->setText(UTF8_("Reset"));

    // Apply
    CEGUI::PushButton* button_apply = static_cast<CEGUI::PushButton*>(wmgr.getWindow("video_button_apply"));
    button_apply->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&cMenu_Options::Video_Button_Apply_Clicked, this));
    button_apply->setText(UTF8_("Apply"));

    // Recreate Cache
    CEGUI::PushButton* button_recreate_cache = static_cast<CEGUI::PushButton*>(wmgr.getWindow("video_button_recreate_cache"));
    button_recreate_cache->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&cMenu_Options::Video_Button_Recreate_Cache_Clicked, this));
    button_recreate_cache->setText(UTF8_("Recreate Cache"));
}

void cMenu_Options::Init_GUI_Audio(void)
{
    // get the CEGUI window manager
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    // Audio Hz
    CEGUI::Window* text_hz = static_cast<CEGUI::Window*>(wmgr.getWindow("audio_text_hz"));
    text_hz->setText(UTF8_("Hertz (Hz)"));
    text_hz->setTooltipText(UTF8_("You should only change the value if the audio is scratchy."));

    m_audio_combo_hz = static_cast<CEGUI::Combobox*>(wmgr.getWindow("audio_combo_hz"));

    CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem("22050");
    item->setTextColours(CEGUI::colour(1, 0, 0));
    m_audio_combo_hz->addItem(item);
    item = new CEGUI::ListboxTextItem("44100");
    item->setTextColours(CEGUI::colour(0, 1, 0));
    m_audio_combo_hz->addItem(item);
    item = new CEGUI::ListboxTextItem("48000");
    item->setTextColours(CEGUI::colour(0, 0, 1));
    m_audio_combo_hz->addItem(item);

    // Set current value
    m_audio_combo_hz->setText(int_to_string(pPreferences->m_audio_hz));

    m_audio_combo_hz->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cMenu_Options::Audio_Hz_Select, this));


    // Music
    CEGUI::Window* text_music = static_cast<CEGUI::Window*>(wmgr.getWindow("audio_text_music"));
    text_music->setText(UTF8_("Music"));
    text_music->setTooltipText(UTF8_("Enable to play music. You need to have the Music Addon installed."));

    m_audio_combo_music = static_cast<CEGUI::Combobox*>(wmgr.getWindow("audio_combo_music"));

    item = new CEGUI::ListboxTextItem(UTF8_("On"));
    item->setTextColours(CEGUI::colour(0, 1, 0));
    m_audio_combo_music->addItem(item);
    item = new CEGUI::ListboxTextItem(UTF8_("Off"));
    item->setTextColours(CEGUI::colour(0, 0, 1));
    m_audio_combo_music->addItem(item);

    if (pAudio->m_music_enabled) {
        m_audio_combo_music->setText(UTF8_("On"));
    }
    else {
        m_audio_combo_music->setText(UTF8_("Off"));
    }

    m_audio_combo_music->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cMenu_Options::Audio_Music_Select, this));

    // music volume slider
    m_audio_slider_music = static_cast<CEGUI::Slider*>(wmgr.getWindow("audio_slider_music_volume"));
    m_audio_slider_music->setTooltipText(UTF8_("Set the Music Volume."));

    m_audio_slider_music->setCurrentValue(static_cast<float>(pAudio->m_music_volume));
    m_audio_slider_music->subscribeEvent(CEGUI::Slider::EventValueChanged, CEGUI::Event::Subscriber(&cMenu_Options::Audio_Music_Volume_Changed, this));


    // Sounds
    CEGUI::Window* text_sound = static_cast<CEGUI::Window*>(wmgr.getWindow("audio_text_sound"));
    text_sound->setText(UTF8_("Sound"));
    text_sound->setTooltipText(UTF8_("Enable to play Sounds.")) ;

    m_audio_combo_sounds = static_cast<CEGUI::Combobox*>(wmgr.getWindow("audio_combo_sounds"));

    item = new CEGUI::ListboxTextItem(UTF8_("On"));
    item->setTextColours(CEGUI::colour(0, 1, 0));
    m_audio_combo_sounds->addItem(item);
    item = new CEGUI::ListboxTextItem(UTF8_("Off"));
    item->setTextColours(CEGUI::colour(1, 0.6f, 0.3f));
    m_audio_combo_sounds->addItem(item);

    if (pAudio->m_sound_enabled) {
        m_audio_combo_sounds->setText(UTF8_("On"));
    }
    else {
        m_audio_combo_sounds->setText(UTF8_("Off"));
    }

    m_audio_combo_sounds->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cMenu_Options::Audio_Sound_Select, this));

    // sound volume slider
    m_audio_slider_sound = static_cast<CEGUI::Slider*>(wmgr.getWindow("audio_slider_sound_volume"));
    m_audio_slider_sound->setTooltipText(UTF8_("Set the Sound Volume."));

    m_audio_slider_sound->setCurrentValue(static_cast<float>(pAudio->m_sound_volume));
    m_audio_slider_sound->subscribeEvent(CEGUI::Slider::EventValueChanged, CEGUI::Event::Subscriber(&cMenu_Options::Audio_Sound_Volume_Changed, this));

    // Reset
    CEGUI::PushButton* button_reset = static_cast<CEGUI::PushButton*>(wmgr.getWindow("audio_button_reset"));
    button_reset->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&cMenu_Options::Audio_Button_Reset_Clicked, this));
    button_reset->setText(UTF8_("Reset"));
}

void cMenu_Options::Init_GUI_Keyboard(void)
{
    // get the CEGUI window manager
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    // Keyboard listbox
    CEGUI::Window* text_keyboard_shortcuts = wmgr.getWindow("keyboard_text_shortcuts");
    text_keyboard_shortcuts->setText(UTF8_("Shortcuts"));

    CEGUI::MultiColumnList* listbox_keyboard = static_cast<CEGUI::MultiColumnList*>(wmgr.getWindow("keyboard_listbox"));

    // TRANS: Name of an action that can be bound to a key
    listbox_keyboard->addColumn(reinterpret_cast<const CEGUI::utf8*>(C_("action", "Name")), 0, CEGUI::UDim(0.47f, 0));
    // TRANS: Key on a keyboard
    listbox_keyboard->addColumn(UTF8_("Key"), 1, CEGUI::UDim(0.47f, 0));
    Build_Shortcut_List();

    listbox_keyboard->subscribeEvent(CEGUI::MultiColumnList::EventMouseDoubleClick, CEGUI::Event::Subscriber(&cMenu_Options::Keyboard_List_Double_Click, this));

    // Keyboard scroll speed
    CEGUI::Window* text_keyboard_scroll_speed = wmgr.getWindow("keyboard_text_scroll_speed");
    text_keyboard_scroll_speed->setText(UTF8_("Scroll Speed"));

    CEGUI::Slider* slider_scoll_speed = static_cast<CEGUI::Slider*>(wmgr.getWindow("keyboard_slider_scroll_speed"));
    slider_scoll_speed->setCurrentValue(pPreferences->m_scroll_speed);
    slider_scoll_speed->subscribeEvent(CEGUI::Slider::EventValueChanged, CEGUI::Event::Subscriber(&cMenu_Options::Keyboard_Slider_Scroll_Speed_Changed, this));

    // Reset Keyboard
    CEGUI::PushButton* button_reset_keyboard = static_cast<CEGUI::PushButton*>(wmgr.getWindow("keyboard_button_reset"));
    button_reset_keyboard->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&cMenu_Options::Keyboard_Button_Reset_Clicked, this));
    button_reset_keyboard->setText(UTF8_("Reset"));
}

void cMenu_Options::Init_GUI_Joystick(void)
{
    // get the CEGUI window manager
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    // Joystick sensitivity text
    CEGUI::Window* text_joystick_sensitivity = wmgr.getWindow("joystick_text_sensitivity");
    text_joystick_sensitivity->setText(UTF8_("Sensitivity"));

    // Joystick analog jump text
    CEGUI::Window* text_joystick_analog_jump = wmgr.getWindow("joystick_text_analog_jump");
    text_joystick_analog_jump->setText(UTF8_("Analog Jump"));

    // Joystick name
    CEGUI::Window* text_joystick_name = wmgr.getWindow("joystick_text_name");
    text_joystick_name->setText(UTF8_("Joystick"));

    text_joystick_name->subscribeEvent(CEGUI::Window::EventMouseClick, CEGUI::Event::Subscriber(&cMenu_Options::Joystick_Name_Click, this));

    CEGUI::Combobox* combo_joy = static_cast<CEGUI::Combobox*>(wmgr.getWindow("joystick_combo"));

    // Add None
    CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(UTF8_("None"));
    item->setTextColours(CEGUI::colour(0, 0, 1));
    combo_joy->addItem(item);

    // Add all Joy names
    vector<std::string> joy_names = pJoystick->Get_Names();

    for (unsigned int i = 0; i < joy_names.size(); i++) {
        item = new CEGUI::ListboxTextItem(joy_names[i]);
        item->setTextColours(CEGUI::colour(0.3f, 1, 0.3f));
        combo_joy->addItem(item);
    }

    // Selected Item
    CEGUI::ListboxTextItem* selected_item = NULL;

    // Set current Joy name
    if (pPreferences->m_joy_enabled) {
        selected_item = static_cast<CEGUI::ListboxTextItem*>(combo_joy->findItemWithText(pJoystick->Get_Name(), NULL));
    }
    // disabled
    else {
        selected_item = static_cast<CEGUI::ListboxTextItem*>(combo_joy->getListboxItemFromIndex(0));
    }
    // set Item
    combo_joy->setText(selected_item->getText());

    combo_joy->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cMenu_Options::Joystick_Name_Select, this));

    // Joystick Sensitivity
    CEGUI::Slider* slider_joy_sensitivity = static_cast<CEGUI::Slider*>(wmgr.getWindow("joystick_slider_sensitivity"));
    slider_joy_sensitivity->setCurrentValue(pPreferences->m_joy_axis_threshold);
    slider_joy_sensitivity->subscribeEvent(CEGUI::Slider::EventValueChanged, CEGUI::Event::Subscriber(&cMenu_Options::Joystick_Sensitivity_Changed, this));

    // Joystick analog jump
    CEGUI::Combobox* combo_joy_analog_jump = static_cast<CEGUI::Combobox*>(wmgr.getWindow("joystick_combo_analog_jump"));

    item = new CEGUI::ListboxTextItem(UTF8_("On"));
    item->setTextColours(CEGUI::colour(0, 0, 1));
    combo_joy_analog_jump->addItem(item);
    item = new CEGUI::ListboxTextItem(UTF8_("Off"));
    item->setTextColours(CEGUI::colour(0, 1, 0));
    combo_joy_analog_jump->addItem(item);

    if (pPreferences->m_joy_analog_jump) {
        combo_joy_analog_jump->setText(UTF8_("On"));
    }
    else {
        combo_joy_analog_jump->setText(UTF8_("Off"));
    }

    combo_joy_analog_jump->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cMenu_Options::Joystick_Analog_Jump_Select, this));

    // Joystick axis horizontal
    CEGUI::Window* text_joystick_axis_hor = wmgr.getWindow("joystick_text_axis_hor");
    text_joystick_axis_hor->setText(UTF8_("Axis Hor"));

    CEGUI::Spinner* spinner_joystick_axis_hor = static_cast<CEGUI::Spinner*>(wmgr.getWindow("joystick_spinner_axis_hor"));
    spinner_joystick_axis_hor->setCurrentValue(static_cast<float>(pPreferences->m_joy_axis_hor));
    spinner_joystick_axis_hor->subscribeEvent(CEGUI::Spinner::EventValueChanged, CEGUI::Event::Subscriber(&cMenu_Options::Joystick_Spinner_Axis_Hor_Changed, this));

    // Joystick axis vertical
    CEGUI::Window* text_joystick_axis_ver = wmgr.getWindow("joystick_text_axis_ver");
    text_joystick_axis_ver->setText(UTF8_("Ver"));

    CEGUI::Spinner* spinner_joystick_axis_ver = static_cast<CEGUI::Spinner*>(wmgr.getWindow("joystick_spinner_axis_ver"));
    spinner_joystick_axis_ver->setCurrentValue(static_cast<float>(pPreferences->m_joy_axis_ver));
    spinner_joystick_axis_ver->subscribeEvent(CEGUI::Spinner::EventValueChanged, CEGUI::Event::Subscriber(&cMenu_Options::Joystick_Spinner_Axis_Ver_Changed, this));

    // Joystick shortcut listbox
    CEGUI::Window* text_joystick_shortcuts = wmgr.getWindow("joystick_text_shortcuts");
    text_joystick_shortcuts->setText(UTF8_("Shortcuts"));

    CEGUI::MultiColumnList* listbox_joystick = static_cast<CEGUI::MultiColumnList*>(wmgr.getWindow("joystick_listbox"));

    listbox_joystick->addColumn(reinterpret_cast<const CEGUI::utf8*>(C_("action", "Name")), 0, CEGUI::UDim(0.47f, 0));
    listbox_joystick->addColumn(UTF8_("Button"), 1, CEGUI::UDim(0.47f, 0));
    Build_Shortcut_List(1);

    listbox_joystick->subscribeEvent(CEGUI::MultiColumnList::EventMouseDoubleClick, CEGUI::Event::Subscriber(&cMenu_Options::Joystick_List_Double_Click, this));

    // Reset Joystick
    CEGUI::PushButton* button_reset_joystick = static_cast<CEGUI::PushButton*>(wmgr.getWindow("joystick_button_reset"));
    button_reset_joystick->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&cMenu_Options::Joystick_Button_Reset_Clicked, this));
    button_reset_joystick->setText(UTF8_("Reset"));
}

void cMenu_Options::Init_GUI_Editor(void)
{
    // get the CEGUI window manager
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    // show item images
    CEGUI::Window* text_editor_show_item_images = static_cast<CEGUI::Window*>(wmgr.getWindow("editor_text_show_item_images"));
    text_editor_show_item_images->setText(UTF8_("Show images"));

    m_game_combo_editor_show_item_images = static_cast<CEGUI::Combobox*>(wmgr.getWindow("editor_combo_show_item_images"));

    CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(UTF8_("On"));
    item->setTextColours(CEGUI::colour(0, 1, 0));
    m_game_combo_editor_show_item_images->addItem(item);
    item = new CEGUI::ListboxTextItem(UTF8_("Off"));
    item->setTextColours(CEGUI::colour(0, 0, 1));
    m_game_combo_editor_show_item_images->addItem(item);

    if (pPreferences->m_editor_show_item_images) {
        m_game_combo_editor_show_item_images->setText(UTF8_("On"));
    }
    else {
        m_game_combo_editor_show_item_images->setText(UTF8_("Off"));
    }

    m_game_combo_editor_show_item_images->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cMenu_Options::Game_Editor_Show_Item_Images_Select, this));

    // item image size
    CEGUI::Window* text_editor_item_image_size = static_cast<CEGUI::Window*>(wmgr.getWindow("editor_text_item_image_size"));
    text_editor_item_image_size->setText(UTF8_("Item image size"));

    m_game_spinner_editor_item_image_size = static_cast<CEGUI::Spinner*>(wmgr.getWindow("editor_spinner_item_image_size"));
    m_game_spinner_editor_item_image_size->setCurrentValue(static_cast<float>(pPreferences->m_editor_item_image_size));

    m_game_spinner_editor_item_image_size->subscribeEvent(CEGUI::Spinner::EventValueChanged, CEGUI::Event::Subscriber(&cMenu_Options::Game_Editor_Item_Image_Size_Select, this));

    // editor mouse auto hide
    CEGUI::Window* text_editor_mouse_auto_hide = static_cast<CEGUI::Window*>(wmgr.getWindow("editor_text_mouse_auto_hide"));
    text_editor_mouse_auto_hide->setText(UTF8_("Auto-Hide Mouse"));

    m_game_combo_editor_mouse_auto_hide = static_cast<CEGUI::Combobox*>(wmgr.getWindow("editor_combo_mouse_auto_hide"));

    item = new CEGUI::ListboxTextItem(UTF8_("On"));
    item->setTextColours(CEGUI::colour(0, 1, 0));
    m_game_combo_editor_mouse_auto_hide->addItem(item);
    item = new CEGUI::ListboxTextItem(UTF8_("Off"));
    item->setTextColours(CEGUI::colour(0, 0, 1));
    m_game_combo_editor_mouse_auto_hide->addItem(item);

    if (pPreferences->m_editor_mouse_auto_hide) {
        m_game_combo_editor_mouse_auto_hide->setText(UTF8_("On"));
    }
    else {
        m_game_combo_editor_mouse_auto_hide->setText(UTF8_("Off"));
    }

    m_game_combo_editor_mouse_auto_hide->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cMenu_Options::Game_Editor_Auto_Hide_Mouse_Select, this));

    // Reset Editor
    CEGUI::PushButton* button_reset_editor = static_cast<CEGUI::PushButton*>(wmgr.getWindow("editor_button_reset"));
    button_reset_editor->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&cMenu_Options::Game_Button_Reset_Editor_Clicked, this));
    button_reset_editor->setText(UTF8_("Reset"));
}

void cMenu_Options::Exit(void)
{
    pPreferences->Save();
    Game_Action = GA_ENTER_MENU;
    Game_Action_Data_Middle.add("load_menu", int_to_string(MENU_MAIN));
    if (m_exit_to_gamemode != MODE_NOTHING) {
        Game_Action_Data_Middle.add("menu_exit_back_to", int_to_string(m_exit_to_gamemode));
    }
}

void cMenu_Options::Update(void)
{
    cMenu_Base::Update();

    if (!m_action) {
        return;
    }

    m_action = 0;

    // only menu actions
    if (pMenuCore->m_handler->m_active > 0) {
        return;
    }

    // todo : use this functionality again
    Change_Game_Setting(pMenuCore->m_handler->m_active);
    Change_Video_Setting(pMenuCore->m_handler->m_active);
    Change_Audio_Setting(pMenuCore->m_handler->m_active);
    Change_Keyboard_Setting(pMenuCore->m_handler->m_active);
    Change_Joystick_Setting(pMenuCore->m_handler->m_active);
    Change_Editor_Setting(pMenuCore->m_handler->m_active);
}

void cMenu_Options::Change_Game_Setting(int setting)
{
    // always run
    if (pMenuCore->m_handler->m_active == 5) {
        pPreferences->m_always_run = !pPreferences->m_always_run;

        if (pPreferences->m_always_run) {
            m_game_combo_always_run->setText(UTF8_("On"));
        }
        else {
            m_game_combo_always_run->setText(UTF8_("Off"));
        }
    }
    // Camera Horizontal
    else if (pMenuCore->m_handler->m_active == 6) {
        // nothing
    }
    // Camera Vertical
    else if (pMenuCore->m_handler->m_active == 7) {
        // nothing
    }
    // language
    else if (pMenuCore->m_handler->m_active == 8) {
        unsigned int selected = m_game_combo_language->getItemIndex(m_game_combo_language->findItemWithText(m_game_combo_language->getText(), NULL));

        CEGUI::ListboxItem* new_selected = NULL;

        // last item selected
        if (selected == m_game_combo_language->getItemCount() - 1) {
            new_selected = m_game_combo_language->getListboxItemFromIndex(0);
        }
        // select next item
        else {
            new_selected = m_game_combo_language->getListboxItemFromIndex(selected + 1);
        }

        m_game_combo_language->setText(new_selected->getText());
        m_game_combo_language->setItemSelectState(new_selected, 1);
        Game_Language_Select(CEGUI::WindowEventArgs(m_game_combo_language));
    }
    // menu level
    else if (pMenuCore->m_handler->m_active == 9) {
        unsigned int selected = m_game_combo_menu_level->getItemIndex(m_game_combo_menu_level->findItemWithText(m_game_combo_menu_level->getText(), NULL));

        CEGUI::ListboxItem* new_selected = NULL;

        // last item selected
        if (selected == m_game_combo_menu_level->getItemCount() - 1) {
            new_selected = m_game_combo_menu_level->getListboxItemFromIndex(0);
        }
        // select next item
        else {
            new_selected = m_game_combo_menu_level->getListboxItemFromIndex(selected + 1);
        }

        m_game_combo_menu_level->setText(new_selected->getText());
        m_game_combo_menu_level->setItemSelectState(new_selected, 1);
        Game_Menu_Level_Select(CEGUI::WindowEventArgs(m_game_combo_menu_level));
    }
}

void cMenu_Options::Change_Video_Setting(int setting)
{
    // Resolution
    if (pMenuCore->m_handler->m_active == 5) {
        // Resolution
        unsigned int selected = m_video_combo_resolution->getItemIndex(m_video_combo_resolution->findItemWithText(m_video_combo_resolution->getText(), NULL));

        CEGUI::ListboxItem* new_selected = NULL;

        // last item selected
        if (selected == m_video_combo_resolution->getItemCount() - 1) {
            new_selected = m_video_combo_resolution->getListboxItemFromIndex(0);
        }
        // select next item
        else {
            new_selected = m_video_combo_resolution->getListboxItemFromIndex(selected + 1);
        }

        m_video_combo_resolution->setText(new_selected->getText());
        m_video_combo_resolution->setItemSelectState(new_selected, 1);
        Video_Resolution_Select(CEGUI::WindowEventArgs(m_video_combo_resolution));
    }
    // BPP
    else if (pMenuCore->m_handler->m_active == 6) {
        if (m_vid_bpp == 16) {
            m_vid_bpp = 32;
        }
        else if (m_vid_bpp == 32) {
            m_vid_bpp = 16;
        }

        m_video_combo_bpp->setText(int_to_string(m_vid_bpp).c_str());
    }
    // Fullscreen
    else if (pMenuCore->m_handler->m_active == 7) {
        m_vid_fullscreen = !m_vid_fullscreen;

        if (m_vid_fullscreen) {
            m_video_combo_fullscreen->setText(UTF8_("On"));
        }
        else {
            m_video_combo_fullscreen->setText(UTF8_("Off"));
        }
    }
    // VSync
    else if (pMenuCore->m_handler->m_active == 8) {
        m_vid_vsync = !m_vid_vsync;

        if (m_vid_vsync) {
            m_video_combo_vsync->setText(UTF8_("On"));
        }
        else {
            m_video_combo_vsync->setText(UTF8_("Off"));
        }
    }
    // FPS Limit
    else if (pMenuCore->m_handler->m_active == 8) {
        // nothing
    }
}

void cMenu_Options::Change_Audio_Setting(int setting)
{
    // Hz
    if (pMenuCore->m_handler->m_active == 5) {
        unsigned int selected = m_audio_combo_hz->getItemIndex(m_audio_combo_hz->findItemWithText(m_audio_combo_hz->getText(), NULL));

        CEGUI::ListboxItem* new_selected = NULL;

        // last item selected
        if (selected == m_audio_combo_hz->getItemCount() - 1) {
            new_selected = m_audio_combo_hz->getListboxItemFromIndex(0);
        }
        // select next item
        else {
            new_selected = m_audio_combo_hz->getListboxItemFromIndex(selected + 1);
        }

        m_audio_combo_hz->setText(new_selected->getText());
        m_audio_combo_hz->setItemSelectState(new_selected, 1);
        Audio_Hz_Select(CEGUI::WindowEventArgs(m_audio_combo_hz));
    }
    // Music
    else if (pMenuCore->m_handler->m_active == 6) {
        pAudio->Toggle_Music();

        if (pAudio->m_music_enabled) {
            m_audio_combo_music->setText(UTF8_("On"));
        }
        else {
            m_audio_combo_music->setText(UTF8_("Off"));
        }
    }
    // Sounds
    else if (pMenuCore->m_handler->m_active == 7) {
        pAudio->Toggle_Sounds();

        if (pAudio->m_sound_enabled) {
            m_audio_combo_sounds->setText(UTF8_("On"));
        }
        else {
            m_audio_combo_sounds->setText(UTF8_("Off"));
        }
    }
}

void cMenu_Options::Change_Keyboard_Setting(int setting)
{
    // todo
}

void cMenu_Options::Change_Joystick_Setting(int setting)
{
    // todo
}

void cMenu_Options::Change_Editor_Setting(int setting)
{
    // editor show item images
    if (pMenuCore->m_handler->m_active == 10) {
        pPreferences->m_editor_show_item_images = !pPreferences->m_editor_show_item_images;

        if (pPreferences->m_editor_show_item_images) {
            m_game_combo_editor_show_item_images->setText(UTF8_("On"));
        }
        else {
            m_game_combo_editor_show_item_images->setText(UTF8_("Off"));
        }
    }
    // editor item image size
    else if (pMenuCore->m_handler->m_active == 11) {
        // nothing
    }
    // editor auto mouse hide
    else if (pMenuCore->m_handler->m_active == 12) {
        pPreferences->m_editor_mouse_auto_hide = !pPreferences->m_editor_mouse_auto_hide;

        if (pPreferences->m_editor_mouse_auto_hide) {
            m_game_combo_editor_show_item_images->setText(UTF8_("On"));
        }
        else {
            m_game_combo_editor_show_item_images->setText(UTF8_("Off"));
        }
    }
}

void cMenu_Options::Draw(void)
{
    cMenu_Base::Draw();
    Draw_End();
}

void cMenu_Options::Build_Shortcut_List(bool joystick /* = 0 */)
{
    // Get Listbox
    CEGUI::MultiColumnList* listbox = NULL;

    // Keyboard
    if (!joystick) {
        listbox = static_cast<CEGUI::MultiColumnList*>(CEGUI::WindowManager::getSingleton().getWindow("keyboard_listbox"));
    }
    // Joystick
    else {
        listbox = static_cast<CEGUI::MultiColumnList*>(CEGUI::WindowManager::getSingleton().getWindow("joystick_listbox"));
    }

    listbox->resetList();

    // build shortcut list
    vector<cShortcut_item> shortcuts;

    // Keyboard
    if (!joystick) {
        shortcuts.push_back(cShortcut_item(UTF8_("Up"), &pPreferences->m_key_up, &pPreferences->m_key_up_default));
        shortcuts.push_back(cShortcut_item(UTF8_("Down"), &pPreferences->m_key_down, &pPreferences->m_key_down_default));
        shortcuts.push_back(cShortcut_item(UTF8_("Left"), &pPreferences->m_key_left, &pPreferences->m_key_left_default));
        shortcuts.push_back(cShortcut_item(UTF8_("Right"), &pPreferences->m_key_right, &pPreferences->m_key_right_default));
        shortcuts.push_back(cShortcut_item(UTF8_("Jump"), &pPreferences->m_key_jump, &pPreferences->m_key_jump_default));
        shortcuts.push_back(cShortcut_item(UTF8_("Shoot"), &pPreferences->m_key_shoot, &pPreferences->m_key_shoot_default));
        shortcuts.push_back(cShortcut_item(UTF8_("Item"), &pPreferences->m_key_item, &pPreferences->m_key_item_default));
        shortcuts.push_back(cShortcut_item(UTF8_("Action"), &pPreferences->m_key_action, &pPreferences->m_key_action_default));
        shortcuts.push_back(cShortcut_item(UTF8_("Screenshot"), &pPreferences->m_key_screenshot, &pPreferences->m_key_screenshot_default));
        shortcuts.push_back(cShortcut_item(UTF8_("Editor fast copy up"), &pPreferences->m_key_editor_fast_copy_up, &pPreferences->m_key_editor_fast_copy_up_default));
        shortcuts.push_back(cShortcut_item(UTF8_("Editor fast copy down"), &pPreferences->m_key_editor_fast_copy_down, &pPreferences->m_key_editor_fast_copy_down_default));
        shortcuts.push_back(cShortcut_item(UTF8_("Editor fast copy left"), &pPreferences->m_key_editor_fast_copy_left, &pPreferences->m_key_editor_fast_copy_left_default));
        shortcuts.push_back(cShortcut_item(UTF8_("Editor fast copy right"), &pPreferences->m_key_editor_fast_copy_right, &pPreferences->m_key_editor_fast_copy_right_default));
        shortcuts.push_back(cShortcut_item(UTF8_("Editor pixel move up"), &pPreferences->m_key_editor_pixel_move_up, &pPreferences->m_key_editor_pixel_move_up_default));
        shortcuts.push_back(cShortcut_item(UTF8_("Editor pixel move down"), &pPreferences->m_key_editor_pixel_move_down, &pPreferences->m_key_editor_pixel_move_down_default));
        shortcuts.push_back(cShortcut_item(UTF8_("Editor pixel move left"), &pPreferences->m_key_editor_pixel_move_left, &pPreferences->m_key_editor_pixel_move_left_default));
        shortcuts.push_back(cShortcut_item(UTF8_("Editor pixel move right"), &pPreferences->m_key_editor_pixel_move_right, &pPreferences->m_key_editor_pixel_move_right_default));
    }
    // Joystick
    else {
        shortcuts.push_back(cShortcut_item(UTF8_("Jump"), &pPreferences->m_joy_button_jump, &pPreferences->m_joy_button_jump_default));
        shortcuts.push_back(cShortcut_item(UTF8_("Shoot"), &pPreferences->m_joy_button_shoot, &pPreferences->m_joy_button_shoot_default));
        shortcuts.push_back(cShortcut_item(UTF8_("Action"), &pPreferences->m_joy_button_action, &pPreferences->m_joy_button_action_default));
        shortcuts.push_back(cShortcut_item(UTF8_("Item"), &pPreferences->m_joy_button_item, &pPreferences->m_joy_button_item_default));
        shortcuts.push_back(cShortcut_item(UTF8_("Exit"), &pPreferences->m_joy_button_exit, &pPreferences->m_joy_button_exit_default));
    }


    // add all available shortcuts
    for (vector<cShortcut_item>::iterator itr = shortcuts.begin(); itr != shortcuts.end(); ++itr) {
        cShortcut_item shortcut_item = (*itr);

        CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(shortcut_item.m_name, 0, shortcut_item.m_key);
        item->setSelectionColours(CEGUI::colour(0.33f, 0.33f, 0.33f));
        item->setSelectionBrushImage("TaharezLook", "ListboxSelectionBrush");
        unsigned int row_id = listbox->addRow(item, 0);

        // Get shortcut key name
        std::string shortcut_key;
        bool shortcut_not_the_default = 0;

        // Keyboard
        if (!joystick) {
            SDLKey* key = static_cast<SDLKey*>(shortcut_item.m_key);
            const SDLKey* key_default = static_cast<const SDLKey*>(shortcut_item.m_key_default);
            shortcut_key = SDL_GetKeyName(*key);

            if (*key != *key_default) {
                shortcut_not_the_default = 1;
            }
        }
        // Joystick
        else {
            Uint8* button = static_cast<Uint8*>(shortcut_item.m_key);
            const Uint8* button_default = static_cast<const Uint8*>(shortcut_item.m_key_default);
            shortcut_key = int_to_string(*button);

            if (*button != *button_default) {
                shortcut_not_the_default = 1;
            }
        }

        // CEGUI eats [] if not escaped
        string_replace_all(shortcut_key, "[", "\\[");

        item = new CEGUI::ListboxTextItem(shortcut_key);
        // if not default
        if (shortcut_not_the_default) {
            item->setTextColours(CEGUI::colour(0.9f, 0.6f, 0.0f));
        }
        item->setSelectionColours(CEGUI::colour(0.33f, 0.33f, 0.33f));
        item->setSelectionBrushImage("TaharezLook", "ListboxSelectionBrush");
        listbox->setItem(item, 1, row_id);
    }
}

void cMenu_Options::Set_Shortcut(std::string name, void* data, bool joystick /* = 0 */)
{
    std::string info_text;

    if (!joystick) {
        info_text += _("Press a key");
    }
    else {
        info_text += _("Press a button");
    }

    Draw_Static_Text(info_text + _(" for ") + name + _(". Press ESC to cancel."), &orange, NULL, 0);

    bool sub_done = 0;

    while (!sub_done) {
        // no event
        if (!SDL_PollEvent(&input_event)) {
            continue;
        }

        if (input_event.key.keysym.sym == SDLK_ESCAPE || input_event.key.keysym.sym == SDLK_BACKSPACE) {
            sub_done = 1;
            break;
        }

        if (!joystick && input_event.type != SDL_KEYDOWN) {
            continue;
        }
        else if (joystick && input_event.type != SDL_JOYBUTTONDOWN) {
            continue;
        }

        // Keyboard
        if (!joystick) {
            SDLKey* key = static_cast<SDLKey*>(data);
            *key = input_event.key.keysym.sym;
        }
        // Joystick
        else {
            Uint8* button = static_cast<Uint8*>(data);
            *button = input_event.jbutton.button;
        }

        sub_done = 1;
    }

    Build_Shortcut_List(joystick);
}

void cMenu_Options::Joy_Default(unsigned int index)
{
    pPreferences->m_joy_enabled = 1;
    pPreferences->m_joy_name = SDL_JoystickName(index);

    // initialize and if no joystick available disable
    pJoystick->Init();
}

void cMenu_Options::Joy_Disable(void)
{
    pPreferences->m_joy_enabled = 0;
    pPreferences->m_joy_name.clear();

    // close the joystick
    pJoystick->Stick_Close();
}

bool cMenu_Options::Button_Back_Click(const CEGUI::EventArgs& event)
{
    Exit();
    return 1;
}

bool cMenu_Options::Game_Always_Run_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    bool always_run = 0;

    if (item->getText().compare(UTF8_("On")) == 0) {
        always_run = 1;
    }

    pPreferences->m_always_run = always_run;

    return 1;
}

bool cMenu_Options::Game_Camera_Hor_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::Spinner* spinner_camera_hor = static_cast<CEGUI::Spinner*>(windowEventArgs.window);

    pLevel_Manager->m_camera->m_hor_offset_speed = spinner_camera_hor->getCurrentValue();
    pPreferences->m_camera_hor_speed = spinner_camera_hor->getCurrentValue();

    return 1;
}

bool cMenu_Options::Game_Camera_Ver_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::Spinner* spinner_camera_ver = static_cast<CEGUI::Spinner*>(windowEventArgs.window);

    pLevel_Manager->m_camera->m_ver_offset_speed = spinner_camera_ver->getCurrentValue();
    pPreferences->m_camera_ver_speed = spinner_camera_ver->getCurrentValue();

    return 1;
}

bool cMenu_Options::Game_Language_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    // default
    if (item->getText().compare(UTF8_("default")) == 0) {
        pPreferences->m_language = "";
    }
    // force
    else {
        pPreferences->m_language = item->getText().c_str();
    }

    return 1;
}

bool cMenu_Options::Game_Menu_Level_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    pPreferences->m_menu_level = item->getText().c_str();

    return 1;
}

bool cMenu_Options::Game_Menu_Level_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    pPreferences->m_menu_level = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    return 1;
}

bool cMenu_Options::Game_Button_Reset_Game_Clicked(const CEGUI::EventArgs& event)
{
    pPreferences->Reset_Game();

    // clear
    Game_Action = GA_ENTER_MENU;
    Game_Action_Data_Middle.add("load_menu", int_to_string(MENU_OPTIONS));
    if (m_exit_to_gamemode != MODE_NOTHING) {
        Game_Action_Data_Middle.add("menu_exit_back_to", int_to_string(m_exit_to_gamemode));
    }

    return 1;
}

bool cMenu_Options::Video_Resolution_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    std::string temp = item->getText().c_str();

    // get end of height value if text is after resolution string
    std::string::size_type height_end = temp.find(" ");

    if (height_end == std::string::npos) {
        height_end = temp.length();
    }

    // get resolution
    unsigned int w = string_to_int(temp.substr(0, temp.find("x")));
    unsigned int h = string_to_int(temp.substr(temp.find("x") + 1, height_end));

    // is it supported
    if (!pVideo->Test_Video(w, h, m_vid_bpp)) {
        return 0;
    }

    // set new selected resolution
    m_vid_w = w;
    m_vid_h = h;

    return 1;
}

bool cMenu_Options::Video_Bpp_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    unsigned int bpp = string_to_int(item->getText().c_str());

    if (!pVideo->Test_Video(m_vid_w, m_vid_h, bpp)) {
        return 0;
    }

    m_vid_bpp = bpp;

    return 1;
}

bool cMenu_Options::Video_Fullscreen_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    bool bfullscreen = 0;

    if (item->getText().compare(UTF8_("On")) == 0) {
        bfullscreen = 1;
    }

    m_vid_fullscreen = bfullscreen;

    return 1;
}

bool cMenu_Options::Video_Vsync_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    bool bvsync = 0;

    if (item->getText().compare(UTF8_("On")) == 0) {
        bvsync = 1;
    }

    m_vid_vsync = bvsync;

    return 1;
}

bool cMenu_Options::Video_FPS_Limit_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::Spinner* spinner_fps_limit = static_cast<CEGUI::Spinner*>(windowEventArgs.window);

    pPreferences->m_video_fps_limit = spinner_fps_limit->getCurrentValue();

    return 1;
}

bool cMenu_Options::Video_Slider_Geometry_Quality_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    // set new value
    m_vid_geometry_detail = static_cast<CEGUI::Slider*>(windowEventArgs.window)->getCurrentValue();

    return 1;
}

bool cMenu_Options::Video_Slider_Texture_Quality_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    // set new value
    m_vid_texture_detail = static_cast<CEGUI::Slider*>(windowEventArgs.window)->getCurrentValue();

    return 1;
}

bool cMenu_Options::Video_Button_Reset_Clicked(const CEGUI::EventArgs& event)
{
    CEGUI::ListboxItem* list_item = m_video_combo_resolution->findItemWithText(int_to_string(cPreferences::m_video_screen_w_default) + "x" + int_to_string(cPreferences::m_video_screen_h_default), NULL);
    if (list_item) {
        m_video_combo_resolution->setItemSelectState(list_item, 1);

        m_vid_w = cPreferences::m_video_screen_w_default;
        m_vid_h = cPreferences::m_video_screen_h_default;
    }

    list_item = m_video_combo_bpp->findItemWithText(int_to_string(cPreferences::m_video_screen_bpp_default), NULL);
    if (list_item) {
        m_video_combo_bpp->setItemSelectState(list_item, 1);
        m_vid_bpp = cPreferences::m_video_screen_bpp_default;
    }

    if (cPreferences::m_video_fullscreen_default) {
        m_video_combo_fullscreen->setText(UTF8_("On"));
    }
    else {
        m_video_combo_fullscreen->setText(UTF8_("Off"));
    }
    m_vid_fullscreen = cPreferences::m_video_fullscreen_default;

    if (cPreferences::m_video_vsync_default) {
        m_video_combo_vsync->setText(UTF8_("On"));
    }
    else {
        m_video_combo_vsync->setText(UTF8_("Off"));
    }
    m_vid_vsync = cPreferences::m_video_vsync_default;

    m_video_spinner_fps_limit->setCurrentValue(cPreferences::m_video_fps_limit_default);
    pPreferences->m_video_fps_limit = cPreferences::m_video_fps_limit_default;

    m_video_slider_geometry_quality->setCurrentValue(cPreferences::m_geometry_quality_default);
    m_vid_geometry_detail = cPreferences::m_geometry_quality_default;

    m_video_slider_texture_quality->setCurrentValue(cPreferences::m_texture_quality_default);
    m_vid_texture_detail = cPreferences::m_texture_quality_default;

    return 1;
}

bool cMenu_Options::Video_Button_Apply_Clicked(const CEGUI::EventArgs& event)
{
    // draw reinitialization text
    Draw_Static_Text(_("Reinitialization"), &green, NULL, 0);

    pGuiSystem->renderGUI();
    pRenderer->Render();
    SDL_GL_SwapBuffers();

    // apply new settings
    pPreferences->Apply_Video(m_vid_w, m_vid_h, m_vid_bpp, m_vid_fullscreen, m_vid_vsync, m_vid_geometry_detail, m_vid_texture_detail);

    // clear
    Game_Action = GA_ENTER_MENU;
    Game_Action_Data_Middle.add("load_menu", int_to_string(MENU_OPTIONS));
    if (m_exit_to_gamemode != MODE_NOTHING) {
        Game_Action_Data_Middle.add("menu_exit_back_to", int_to_string(m_exit_to_gamemode));
    }

    return 1;
}

bool cMenu_Options::Video_Button_Recreate_Cache_Clicked(const CEGUI::EventArgs& event)
{
    Loading_Screen_Init();

    // save textures for reloading from file
    pImage_Manager->Grab_Textures(1, 1);

    // recreate cache
    pVideo->Init_Image_Cache(1, 1);

    // restore textures
    pImage_Manager->Restore_Textures(1);

    Loading_Screen_Exit();

    return 1;
}

bool cMenu_Options::Audio_Hz_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    pPreferences->m_audio_hz = string_to_int(item->getText().c_str());

    // draw reloading text
    Draw_Static_Text(_("Reloading"), &green, NULL, 0);
    // reload
    pAudio->Close();
    pSound_Manager->Delete_All();
    pAudio->Init();
    // todo : add sound manager function to reload sounds and music when needed
    Preload_Sounds();

    return 1;
}

bool cMenu_Options::Audio_Music_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    bool music_enabled = 0;

    if (item->getText().compare(UTF8_("On")) == 0) {
        music_enabled = 1;
    }

    if (pAudio->m_music_enabled != music_enabled) {
        pAudio->Toggle_Music();

        // Warning if no music pack is installed and music got enabled
        if (pAudio->m_music_enabled && !File_Exists(pResource_Manager->Get_Game_Music("game/menu.ogg")) && !File_Exists(pResource_Manager->Get_Game_Music("land/land_1.ogg"))) {
            Draw_Static_Text(_("Music addon not detected.\nYou can download it from the Website."), &orange);
        }
    }

    return 1;
}

bool cMenu_Options::Audio_Music_Volume_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    Uint8 val = static_cast<Uint8>(static_cast<CEGUI::Slider*>(windowEventArgs.window)->getCurrentValue());

    pAudio->Set_Music_Volume(val);
    // save volume
    pAudio->m_music_volume = val;

    return 1;
}

bool cMenu_Options::Audio_Sound_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    bool sound_enabled = 0;

    if (item->getText().compare(UTF8_("On")) == 0) {
        sound_enabled = 1;
    }

    if (pAudio->m_sound_enabled != sound_enabled) {
        pAudio->Toggle_Sounds();
    }

    return 1;
}

bool cMenu_Options::Audio_Sound_Volume_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    Uint8 val = static_cast<Uint8>(static_cast<CEGUI::Slider*>(windowEventArgs.window)->getCurrentValue());

    pAudio->Set_Sound_Volume(val);
    // save volume
    pAudio->m_sound_volume = val;

    return 1;
}

bool cMenu_Options::Audio_Button_Reset_Clicked(const CEGUI::EventArgs& event)
{
    pPreferences->Reset_Audio();

    // clear
    Game_Action = GA_ENTER_MENU;
    Game_Action_Data_Middle.add("load_menu", int_to_string(MENU_OPTIONS));
    if (m_exit_to_gamemode != MODE_NOTHING) {
        Game_Action_Data_Middle.add("menu_exit_back_to", int_to_string(m_exit_to_gamemode));
    }

    return 1;
}

bool cMenu_Options::Keyboard_List_Double_Click(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::MultiColumnList*>(windowEventArgs.window)->getFirstSelectedItem();

    // set shortcut
    if (item) {
        Set_Shortcut(item->getText().c_str(), item->getUserData());
    }

    return 1;
}

bool cMenu_Options::Keyboard_Slider_Scroll_Speed_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    // set new value
    pPreferences->m_scroll_speed = static_cast<CEGUI::Slider*>(windowEventArgs.window)->getCurrentValue();

    return 1;
}

bool cMenu_Options::Keyboard_Button_Reset_Clicked(const CEGUI::EventArgs& event)
{
    pPreferences->Reset_Keyboard();

    // clear
    Game_Action = GA_ENTER_MENU;
    Game_Action_Data_Middle.add("load_menu", int_to_string(MENU_OPTIONS));
    if (m_exit_to_gamemode != MODE_NOTHING) {
        Game_Action_Data_Middle.add("menu_exit_back_to", int_to_string(m_exit_to_gamemode));
    }

    return 1;
}

bool cMenu_Options::Joystick_Name_Click(const CEGUI::EventArgs& event)
{
    // Get Joystick Combo
    CEGUI::Combobox* combo_joy = static_cast<CEGUI::Combobox*>(CEGUI::WindowManager::getSingleton().getWindow("combo_joy"));

    // selected item id
    int selected_item = 0;
    // current list item
    CEGUI::ListboxTextItem* list_item = static_cast<CEGUI::ListboxTextItem*>(combo_joy->findItemWithText(combo_joy->getText(), NULL));

    // if selected
    if (list_item) {
        selected_item = combo_joy->getItemIndex(list_item);
    }

    // select first
    if (selected_item >= SDL_NumJoysticks()) {
        selected_item = 0;
    }
    // select next item
    else {
        selected_item++;
    }

    // Disable Joy
    if (selected_item == 0) {
        Joy_Disable();
    }
    // Select Joy
    else {
        Joy_Default(selected_item - 1);
    }

    // check if initialization succeeded
    if (selected_item) {
        // initialized
        if (pPreferences->m_joy_enabled) {
            Draw_Static_Text(_("Enabled : ") + pJoystick->Get_Name(), &yellow);
        }
        // failed
        else {
            selected_item = 0;
        }
    }

    combo_joy->setText(combo_joy->getListboxItemFromIndex(selected_item)->getText());

    return 1;
}

bool cMenu_Options::Joystick_Name_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::Combobox* combo = static_cast<CEGUI::Combobox*>(windowEventArgs.window);
    CEGUI::ListboxItem* item = combo->getSelectedItem();

    if (item->getText().compare(_("None")) == 0) {
        Joy_Disable();
    }
    else {
        Joy_Default(combo->getItemIndex(item) - 1);
    }

    return 1;
}

bool cMenu_Options::Joystick_Sensitivity_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    // set new value
    pPreferences->m_joy_axis_threshold = static_cast<Sint16>(static_cast<CEGUI::Slider*>(windowEventArgs.window)->getCurrentValue());

    return 1;
}

bool cMenu_Options::Joystick_Analog_Jump_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    bool analog_jump = 0;

    if (item->getText().compare(_("On")) == 0) {
        analog_jump = 1;
    }

    pPreferences->m_joy_analog_jump = analog_jump;

    return 1;
}

bool cMenu_Options::Joystick_Spinner_Axis_Hor_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    // set new value
    pPreferences->m_joy_axis_hor = static_cast<int>(static_cast<CEGUI::Spinner*>(windowEventArgs.window)->getCurrentValue());

    return 1;
}

bool cMenu_Options::Joystick_Spinner_Axis_Ver_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    // set new value
    pPreferences->m_joy_axis_ver = static_cast<int>(static_cast<CEGUI::Spinner*>(windowEventArgs.window)->getCurrentValue());

    return 1;
}

bool cMenu_Options::Joystick_List_Double_Click(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::MultiColumnList*>(windowEventArgs.window)->getFirstSelectedItem();

    // set shortcut
    if (item) {
        Set_Shortcut(item->getText().c_str(), item->getUserData(), 1);
    }

    return 1;
}

bool cMenu_Options::Joystick_Button_Reset_Clicked(const CEGUI::EventArgs& event)
{
    pPreferences->Reset_Joystick();

    // clear
    Game_Action = GA_ENTER_MENU;
    Game_Action_Data_Middle.add("load_menu", int_to_string(MENU_OPTIONS));
    if (m_exit_to_gamemode != MODE_NOTHING) {
        Game_Action_Data_Middle.add("menu_exit_back_to", int_to_string(m_exit_to_gamemode));
    }

    return 1;
}

bool cMenu_Options::Game_Editor_Show_Item_Images_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    bool show_item_images = 0;

    if (item->getText().compare(UTF8_("On")) == 0) {
        show_item_images = 1;
    }

    pPreferences->m_editor_show_item_images = show_item_images;

    return 1;
}

bool cMenu_Options::Game_Editor_Item_Image_Size_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::Spinner* spinner_item_image_size = static_cast<CEGUI::Spinner*>(windowEventArgs.window);

    pPreferences->m_editor_item_image_size = static_cast<unsigned int>(spinner_item_image_size->getCurrentValue());

    return 1;
}

bool cMenu_Options::Game_Editor_Auto_Hide_Mouse_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    bool auto_hide_mouse = 0;

    if (item->getText().compare(UTF8_("On")) == 0) {
        auto_hide_mouse = 1;
    }

    pPreferences->m_editor_mouse_auto_hide = auto_hide_mouse;

    return 1;
}

bool cMenu_Options::Game_Button_Reset_Editor_Clicked(const CEGUI::EventArgs& event)
{
    pPreferences->Reset_Editor();

    // clear
    Game_Action = GA_ENTER_MENU;
    Game_Action_Data_Middle.add("load_menu", int_to_string(MENU_OPTIONS));
    if (m_exit_to_gamemode != MODE_NOTHING) {
        Game_Action_Data_Middle.add("menu_exit_back_to", int_to_string(m_exit_to_gamemode));
    }

    return 1;
}

/* *** *** *** *** *** *** *** *** cMenu_Savegames *** *** *** *** *** *** *** *** *** */

cMenu_Savegames::cMenu_Savegames(bool type)
    : cMenu_Base()
{
    m_type_save = type;

    for (unsigned int i = 0; i < 9; i++) {
        m_savegame_temp.push_back(new cHudSprite(pMenuCore->m_handler->m_level->m_sprite_manager));
    }
}

cMenu_Savegames::~cMenu_Savegames(void)
{
    for (HudSpriteList::iterator itr = m_savegame_temp.begin(); itr != m_savegame_temp.end(); ++itr) {
        delete *itr;
    }

    m_savegame_temp.clear();
}

void cMenu_Savegames::Init(void)
{
    cMenu_Base::Init();
    Update_Saved_Games_Text();

    cMenu_Item* temp_item = NULL;

    // savegame descriptions
    for (HudSpriteList::iterator itr = m_savegame_temp.begin(); itr != m_savegame_temp.end(); ++itr) {
        temp_item = new cMenu_Item(pMenuCore->m_handler->m_level->m_sprite_manager);
        temp_item->m_image_default->Set_Image((*itr)->m_image);
        temp_item->Set_Pos(static_cast<float>(game_res_w) / 5, m_menu_pos_y);
        pMenuCore->m_handler->Add_Menu_Item(temp_item, 1.5f, grey);

        m_menu_pos_y += temp_item->m_image_default->m_col_rect.m_h;
    }

    // back
    cGL_Surface* back1 = pFont->Render_Text(pFont->m_font_normal, _("Back"), m_text_color);
    temp_item = new cMenu_Item(pMenuCore->m_handler->m_level->m_sprite_manager);
    temp_item->m_image_default->Set_Image(back1);
    temp_item->Set_Pos(static_cast<float>(game_res_w) / 18, 450);
    temp_item->m_is_quit = 1;
    pMenuCore->m_handler->Add_Menu_Item(temp_item, 1.5f, grey);

    if (m_type_save) {
        cHudSprite* hud_sprite = new cHudSprite(pMenuCore->m_handler->m_level->m_sprite_manager);
        hud_sprite->Set_Image(pVideo->Get_Surface("menu/save.png"));
        hud_sprite->Set_Pos(game_res_w * 0.2f, game_res_h * 0.15f);
        m_draw_list.push_back(hud_sprite);
        hud_sprite = new cHudSprite(pMenuCore->m_handler->m_level->m_sprite_manager);
        hud_sprite->Set_Image(pVideo->Get_Surface("menu/items/save.png"));
        hud_sprite->Set_Pos(game_res_w * 0.07f, game_res_h * 0.24f);
        m_draw_list.push_back(hud_sprite);
    }
    else {
        cHudSprite* hud_sprite = new cHudSprite(pMenuCore->m_handler->m_level->m_sprite_manager);
        hud_sprite->Set_Image(pVideo->Get_Surface("menu/load.png"));
        hud_sprite->Set_Pos(game_res_w * 0.2f, game_res_h * 0.15f);
        m_draw_list.push_back(hud_sprite);
        hud_sprite = new cHudSprite(pMenuCore->m_handler->m_level->m_sprite_manager);
        hud_sprite->Set_Image(pVideo->Get_Surface("menu/items/load.png"));
        hud_sprite->Set_Pos(game_res_w * 0.07f, game_res_h * 0.24f);
        m_draw_list.push_back(hud_sprite);
    }

    cHudSprite* hud_sprite = new cHudSprite(pMenuCore->m_handler->m_level->m_sprite_manager);
    hud_sprite->Set_Image(back1, 0, 1);
    hud_sprite->Set_Pos(-200, 0);
    m_draw_list.push_back(hud_sprite);

    Init_GUI();
}

void cMenu_Savegames::Init_GUI(void)
{
    cMenu_Base::Init_GUI();
}

void cMenu_Savegames::Exit(void)
{
    Game_Action = GA_ENTER_MENU;
    Game_Action_Data_Middle.add("load_menu", int_to_string(MENU_MAIN));
    if (m_exit_to_gamemode != MODE_NOTHING) {
        Game_Action_Data_Middle.add("menu_exit_back_to", int_to_string(m_exit_to_gamemode));
    }
}

void cMenu_Savegames::Update(void)
{
    cMenu_Base::Update();

    if (!m_action) {
        return;
    }

    m_action = 0;

    // back
    if (pMenuCore->m_handler->m_active == 9) {
        Exit();
        return;
    }

    if (!m_type_save) {
        Update_Load();
    }
    else {
        Update_Save();
    }
}

void cMenu_Savegames::Draw(void)
{
    cMenu_Base::Draw();
    Draw_End();
}

void cMenu_Savegames::Update_Load(void)
{
    int save_num = pMenuCore->m_handler->m_active + 1;

    // not valid
    if (!pSavegame->Is_Valid(save_num)) {
        return;
    }

    // reset before loading the level to keep the level in the manager
    pLevel_Player->Reset_Save();

    cSave* savegame = NULL;
    try {
        savegame = pSavegame->Load(save_num);
    }
    catch(xmlpp::parse_error& err) {
        pAudio->Play_Sound("error.ogg");
        std::cerr << "Error: Failed to load savegame '" << save_num << "' (parsing error). xmlpp parsing exception: " << err.what() << std::endl;
        return;
    }
    catch(InvalidSavegameError& err) {
        pAudio->Play_Sound("error.ogg");
        std::cerr << "Error: Failed to load savegame '" << save_num << "' (invalid savegame). TSC exception: " << err.what() << std::endl;
        return;
    }

    pAudio->Play_Sound("savegame_load.ogg");
    std::string level_name = savegame->Get_Active_Level();
    delete savegame;

    if (!level_name.empty()) {
        Game_Action = GA_ENTER_LEVEL;
        cLevel* level = pLevel_Manager->Load(level_name);
        // only fade-out music if different
        if (pActive_Level->Get_Music_Filename().compare(level->Get_Music_Filename()) != 0) {
            Game_Action_Data_Start.add("music_fadeout", "1000");
        }
    }
    else {
        Game_Action = GA_ENTER_WORLD;
        Game_Action_Data_Start.add("music_fadeout", "1000");
    }

    Game_Action_Data_Start.add("screen_fadeout", CEGUI::PropertyHelper::intToString(EFFECT_OUT_BLACK));
    Game_Action_Data_Start.add("screen_fadeout_speed", "3");
    Game_Action_Data_Middle.add("unload_menu", "1");
    Game_Action_Data_Middle.add("load_savegame", int_to_string(save_num));
    Game_Action_Data_End.add("screen_fadein", CEGUI::PropertyHelper::intToString(EFFECT_IN_BLACK));
    Game_Action_Data_End.add("screen_fadein_speed", "3");
}

void cMenu_Savegames::Update_Save(void)
{
    // not valid
    if (pOverworld_Player->m_current_waypoint < 0 && !pActive_Level->Is_Loaded()) {
        return;
    }

    std::string descripion = Set_Save_Description(pMenuCore->m_handler->m_active + 1);

    pFramerate->Reset();

    if (descripion.compare("Not enough Points") == 0) {
        Game_Action = GA_ENTER_MENU;
        Game_Action_Data_Middle.add("load_menu", int_to_string(MENU_MAIN));
        if (m_exit_to_gamemode != MODE_NOTHING) {
            Game_Action_Data_Middle.add("menu_exit_back_to", int_to_string(m_exit_to_gamemode));
        }
        return;
    }

    if (descripion.empty()) {
        return;
    }

    pAudio->Play_Sound("savegame_save.ogg");

    // no costs in debug builds
#ifndef _DEBUG
    if (pActive_Level->Is_Loaded()) {
        pHud_Points->Set_Points(pLevel_Player->m_points - 3000);
    }
#endif
    // save
    pSavegame->Save_Game(pMenuCore->m_handler->m_active + 1, descripion);

    Game_Action = GA_ENTER_MENU;
    Game_Action_Data_Middle.add("load_menu", int_to_string(MENU_MAIN));
    if (m_exit_to_gamemode != MODE_NOTHING) {
        Game_Action_Data_Middle.add("menu_exit_back_to", int_to_string(m_exit_to_gamemode));
    }
}

std::string cMenu_Savegames::Set_Save_Description(unsigned int save_slot)
{
    if (save_slot == 0 || save_slot > 9) {
        return "";
    }
// save always in debug builds
#ifndef _DEBUG
    if (pActive_Level->Is_Loaded() && pLevel_Player->m_points < 3000) {
        Clear_Input_Events();
        Draw_Static_Text(_("3000 Points needed for saving in a level.\nSaving on the Overworld is free."));

        return "Not enough Points";
    }
#endif
    std::string save_description;

    bool auto_erase_description = 0;

    // if Savegame exists use old description
    if (pSavegame->Is_Valid(save_slot)) {
        save_description.clear();
        // get only the description
        save_description = pSavegame->Get_Description(save_slot, 1);
    }
    else {
        // use default description
        save_description = _("No Description");
        auto_erase_description = 1;
    }

    return Box_Text_Input(save_description, _("Enter Description"), auto_erase_description);
}

void cMenu_Savegames::Update_Saved_Games_Text(void)
{
    unsigned int save_slot = 0;

    for (HudSpriteList::iterator itr = m_savegame_temp.begin(); itr != m_savegame_temp.end(); ++itr) {
        save_slot++;

        std::string text;
        try {
            text = pSavegame->Get_Description(save_slot);
        }
        catch(xmlpp::parse_error& err) {
            std::cerr << "Error: Failed to load savegame '" << save_slot << "' (parsing error). xmlpp parsing exception: " << err.what() << std::endl;
            (*itr)->Set_Image(pFont->Render_Text(pFont->m_font_normal,  _("Savegame loading failed"), red), true, true);
            continue;
        }
        catch(InvalidSavegameError& err) {
            std::cerr << "Error: Failed to load savegame '" << save_slot << "' (invalid savegame). TSC exception: " << err.what() << std::endl;
            (*itr)->Set_Image(pFont->Render_Text(pFont->m_font_normal,  _("Savegame loading failed"), red), true, true);
            continue;
        }

        (*itr)->Set_Image(pFont->Render_Text(pFont->m_font_normal, text, m_text_color_value), true, true);
    }
}

/* *** *** *** *** *** *** *** *** cMenu_Credits *** *** *** *** *** *** *** *** *** */

cMenu_Credits::cMenu_Credits(void)
    : cMenu_Base()
{

}

cMenu_Credits::~cMenu_Credits(void)
{

}

void cMenu_Credits::Init(void)
{
    cMenu_Base::Init();

    // clear credits
    m_draw_list.clear();

    // Load all our credits from the g_credits global string.
    size_t position = 0;
    size_t last_position = 0;
    while (true) {
        position = g_credits.find("\n", last_position + 1);

        if (position == std::string::npos)
            break;

        if (position != last_position + 1) { // Not empty line
            std::string line = g_credits.substr(last_position, position - last_position);

            // Strip leading newline if any (first line has none)
            if (line[0] == '\n')
                line = line.substr(1, std::string::npos);

            if (line[0] == ' ') // Continuation line
                Add_Credits_Line(line, 0, -3, white);
            else if (line[0] == '-') // Separator line
                Add_Credits_Line(line, 0, 20, red);
            else {
                float r =  rand() % 80 / 100.0; // Don’t conflict with separator line color
                float g = rand() % 255 / 100.0;
                float b = rand() % 255 / 100.0;
                Add_Credits_Line(line, 0, 20, Color(r, g, b));
            }
        }

        last_position = position;
    }

    m_menu_pos_y = game_res_h * 1.1f;

    // set credits position
    for (HudSpriteList::iterator itr = m_draw_list.begin(); itr != m_draw_list.end(); ++itr) {
        // get object
        cHudSprite* obj = (*itr);

        // set shadow if not set
        if (obj->m_shadow_pos == 0) {
            obj->Set_Shadow(grey, 1);
        }
        // set position
        obj->m_pos_x += static_cast<float>(game_res_w) / 5;
        obj->m_pos_y += m_menu_pos_y;
        // set posz behind front passive
        obj->m_pos_z = 0.096f;
        // set color combine
        obj->Set_Color_Combine(0, 0, 0, GL_MODULATE);
        obj->m_color.alpha = 0;
        obj->m_shadow_color.alpha = 0;

        m_menu_pos_y = obj->m_pos_y + obj->m_col_rect.m_h;
    }

    cMenu_Item* temp_item = NULL;

    // back
    cGL_Surface* back1 = pFont->Render_Text(pFont->m_font_normal, _("Back"), m_text_color);
    temp_item = new cMenu_Item(pMenuCore->m_handler->m_level->m_sprite_manager);
    temp_item->m_image_default->Set_Image(back1);
    temp_item->Set_Pos(static_cast<float>(game_res_w) / 18, 250);
    temp_item->m_is_quit = 1;
    pMenuCore->m_handler->Add_Menu_Item(temp_item, 2, grey);

    cHudSprite* hud_sprite = new cHudSprite(pMenuCore->m_handler->m_level->m_sprite_manager);
    hud_sprite->Set_Image(back1, 0, 1);
    hud_sprite->Set_Pos(-200, 0);
    m_draw_list.push_back(hud_sprite);

    Init_GUI();
}

void cMenu_Credits::Init_GUI(void)
{
    cMenu_Base::Init_GUI();
}

void cMenu_Credits::Enter(const GameMode old_mode /* = MODE_NOTHING */)
{
    // black background because of fade alpha
    glClearColor(0, 0, 0, 1);

    if (old_mode == MODE_MENU) {
        // fade in
        Menu_Fade();
    }
}

void cMenu_Credits::Leave(const GameMode next_mode /* = MODE_NOTHING */)
{
    if (m_exit_to_gamemode == MODE_NOTHING || m_exit_to_gamemode == MODE_MENU) {
        // fade out
        Menu_Fade(0);

        // white background
        glClearColor(1, 1, 1, 1);
    }

    // set menu gradient colors back
    pMenuCore->m_handler->m_level->m_background_manager->Get_Pointer(0)->m_color_1.alpha = 255;
    pMenuCore->m_handler->m_level->m_background_manager->Get_Pointer(0)->m_color_2.alpha = 255;
}

void cMenu_Credits::Exit(void)
{
    Game_Action = GA_ENTER_MENU;
    Game_Action_Data_Start.add("music_fadeout", "500");
    Game_Action_Data_Middle.add("load_menu", int_to_string(MENU_MAIN));
    if (m_exit_to_gamemode != MODE_NOTHING) {
        Game_Action_Data_Middle.add("menu_exit_back_to", int_to_string(m_exit_to_gamemode));
    }
}

void cMenu_Credits::Update(void)
{
    static int s_credits_done = 0;
    cMenu_Base::Update();

    for (HudSpriteList::iterator itr = m_draw_list.begin(); itr != m_draw_list.end(); ++itr) {
        cHudSprite* obj = (*itr);

        // When the respective line is long out of sight, remove it from the
        // list of lines to draw.
        if (obj->m_pos_y < -300) {
            if (obj->m_active) {
                obj->Set_Active(false); // FIXME: Does nothing it appears?
                s_credits_done++;
            }
            // FIXME: Lines just scroll into negative infinity? Overflow
            // shouldn’t happen, though, as the credits are automatically
            // ended anyway.
        }
        // fading out
        else if (obj->m_pos_y < game_res_h * 0.3f) {
            float new_value = obj->m_combine_color[0] - (pFramerate->m_speed_factor * 0.01f);

            if (new_value < 0) {
                new_value = 0;
            }

            obj->Set_Color_Combine(new_value, new_value, new_value, GL_MODULATE);
            obj->m_color.alpha = static_cast<Uint8>(new_value * 255);
            obj->m_shadow_color.alpha = obj->m_color.alpha;
        }
        // fading in
        else if (obj->m_pos_y < game_res_h * 0.9f) {
            float new_value = obj->m_combine_color[0] + (pFramerate->m_speed_factor * 0.01f);

            if (new_value > 1.0f) {
                new_value = 1.0f;

                // add particles
                if (obj->m_combine_color[0] < 1.0f) {
                    cParticle_Emitter* anim = new cParticle_Emitter(pMenuCore->m_handler->m_level->m_sprite_manager);
                    anim->Set_Emitter_Rect(Get_Random_Float(game_res_w * 0.1f, game_res_w * 0.8f), -Get_Random_Float(game_res_h * 0.8f, game_res_h * 0.9f), Get_Random_Float(0.0f, 5.0f), Get_Random_Float(0.0f, 5.0f));
                    unsigned int quota = 4;

                    // multi-explosion
                    if (rand() % 2) {
                        anim->Set_Image_Filename(utf8_to_path("animation/particles/fire_2.png"));
                        anim->Set_Emitter_Time_to_Live(0.4f);
                        anim->Set_Emitter_Iteration_Interval(0.05f);
                        anim->Set_Direction_Range(0, 360);
                        anim->Set_Scale(0.3f, 0.2f);
                        anim->Set_Blending(BLEND_ADD);
                        anim->Set_Time_to_Live(1.8f, 1.2f);
                        anim->Set_Speed(2.1f, 0.5f);
                    }
                    // star explosion
                    else {
                        quota += rand() % 25;
                        anim->Set_Image_Filename(utf8_to_path("animation/particles/fire_3.png"));
                        anim->Set_Direction_Range(0, 360);
                        anim->Set_Scale(0.2f, 0.1f);

                        if (quota < 10) {
                            anim->Set_Time_to_Live(2.8f, 0.5f);
                            anim->Set_Speed(0.8f, 0.3f);
                        }
                        else {
                            anim->Set_Time_to_Live(1.4f, 0.5f);
                            anim->Set_Fading_Size(1);
                            anim->Set_Speed(1.6f, 0.5f);
                        }
                    }

                    anim->Set_Quota(quota);
                    anim->Set_Color(Color(static_cast<Uint8>(100 + (rand() % 155)), 100 + (rand() % 155), 100 + (rand() % 155)));
                    anim->Set_Const_Rotation_Z(-5, 10);
                    anim->Set_Vertical_Gravity(0.02f);
                    anim->Set_Pos_Z(0.16f);
                    anim->Emit();
                    pMenuCore->m_animation_manager->Add(anim);
                }
            }

            obj->Set_Color_Combine(new_value, new_value, new_value, GL_MODULATE);
            obj->m_color.alpha = static_cast<Uint8>(new_value * 255);
            obj->m_shadow_color.alpha = obj->m_color.alpha;
        }

        // default upwards scroll
        obj->Move(0, -1.1f);
    }

    if (s_credits_done >= m_draw_list.size()) {
        Exit();
    }

    if (!m_action) {
        return;
    }

    m_action = false;

    // back
    if (pMenuCore->m_handler->m_active == 0) {
        Exit();
    }
}

void cMenu_Credits::Draw(void)
{
    // do not draw if exiting
    if (Game_Action != GA_NONE) {
        return;
    }

    cMenu_Base::Draw();

    // darken background
    cRect_Request* request = new cRect_Request();
    pVideo->Draw_Rect(NULL, 0.095f, &pMenuCore->m_handler->m_level->m_background_manager->Get_Pointer(0)->m_color_2, request);
    request->m_color.red = static_cast<Uint8>(request->m_color.red * 0.1f);
    request->m_color.green = static_cast<Uint8>(request->m_color.green * 0.1f);
    request->m_color.blue = static_cast<Uint8>(request->m_color.blue * 0.1f);
    request->m_color.alpha = 195;
    pRenderer->Add(request);

    Draw_End();
}

void cMenu_Credits::Add_Credits_Line(const std::string& text, float posx, float posy, const Color& color /* = black */, float shadow_pos /* = 0.0f */, const Color& shadow_color /* = black */)
{
    cHudSprite* temp = new cHudSprite(pMenuCore->m_handler->m_level->m_sprite_manager);
    temp->Set_Image(pFont->Render_Text(pFont->m_font_normal, text, color), 1, 1);
    temp->Set_Pos(posx, posy);
    if (!Is_Float_Equal(shadow_pos, 0.0f)) {
        temp->Set_Shadow(shadow_color, shadow_pos);
    }
    m_draw_list.push_back(temp);
}

void cMenu_Credits::Menu_Fade(bool fade_in /* = 1 */)
{
    // logo position y
    int logo_pos_y = 0;
    // fade counter
    float counter;
    // move speed
    float move_speed;

    if (fade_in) {
        logo_pos_y = 20;
        counter = 255.0f;
        move_speed = -2.0f;
    }
    else {
        logo_pos_y = -200;
        counter = 60.0f;
        move_speed = 2.0f;
    }

    // get logo
    // FIXME: We shouldn’t have to search this in the sprite manager. It
    // should be referenced at some central place.
    cSprite* logo = pMenuCore->m_handler->m_level->m_sprite_manager->Get_from_Position(180, logo_pos_y, TYPE_UNDEFINED, 2);

    // fade out
    while (1) {
        // # Update

        if (fade_in) {
            counter -= 4.5f * pFramerate->m_speed_factor;
            move_speed -= 1.0f * pFramerate->m_speed_factor;

            if (counter < 60.0f) {
                break;
            }

            // move logo out
            if (logo && logo->m_pos_y > -200.0f) {
                logo->Move(0.0f, move_speed);

                if (logo->m_pos_y < -200.0f) {
                    logo->Set_Pos_Y(-200.0f);
                }
            }
        }
        else {
            counter += 5.0f * pFramerate->m_speed_factor;
            move_speed += 1.0f * pFramerate->m_speed_factor;

            if (counter > 255.0f) {
                break;
            }

            // move logo in
            if (logo && logo->m_pos_y < 20.0f) {
                logo->Move(0.0f, move_speed);

                if (logo->m_pos_y > 20.0f) {
                    logo->Set_Pos_Y(20.0f);
                }
            }
        }

        // set menu gradient colors
        pMenuCore->m_handler->m_level->m_background_manager->Get_Pointer(0)->m_color_1.alpha = static_cast<Uint8>(counter);
        pMenuCore->m_handler->m_level->m_background_manager->Get_Pointer(0)->m_color_2.alpha = static_cast<Uint8>(counter);

        // # Draw

        // clear
        pVideo->Clear_Screen();

        // draw menu
        pMenuCore->m_handler->Draw();
        pMenuCore->m_animation_manager->Draw();

        // create request
        cRect_Request* request = new cRect_Request();
        pVideo->Draw_Rect(NULL, 0.095f, &pMenuCore->m_handler->m_level->m_background_manager->Get_Pointer(0)->m_color_2, request);
        request->m_color.red = static_cast<Uint8>(request->m_color.red * 0.1f);
        request->m_color.green = static_cast<Uint8>(request->m_color.green * 0.1f);
        request->m_color.blue = static_cast<Uint8>(request->m_color.blue * 0.1f);
        request->m_color.alpha = 255 - static_cast<Uint8>(counter);
        // add request
        pRenderer->Add(request);

        pVideo->Render();

        // # framerate
        pFramerate->Update();
        // if vsync is disabled then limit the fps to reduce the CPU usage
        if (!pPreferences->m_video_vsync) {
            Correct_Frame_Time(100);
        }
    }
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

/***************************************************************************
 * menu_data.h
 *
 * Copyright © 2004 - 2011 Florian Richter
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

#ifndef TSC_MENU_DATA_HPP
#define TSC_MENU_DATA_HPP

#include "../core/global_basic.hpp"
#include "../gui/menu.hpp"
#include "../gui/hud.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** cMenu_Base *** *** *** *** *** *** *** *** *** *** */

    class cMenu_Base {
    public:
        cMenu_Base(void);
        virtual ~cMenu_Base(void);

        virtual void Init(void);
        virtual void Init_GUI(void);
        // Enter mode
        virtual void Enter(const GameMode old_mode = MODE_NOTHING);
        // Leave mode
        virtual void Leave(const GameMode next_mode = MODE_NOTHING);
        // Exit menu
        virtual void Exit(void);
        virtual void Update(void);
        virtual void Draw(void);
        void Draw_End(void);

        // Set the game mode to return on exit
        void Set_Exit_To_Game_Mode(GameMode gamemode);

        // gui layout filename
        std::string m_layout_file;
        // CEGUI window
        CEGUI::Window* m_gui_window;

        // if button/key action
        bool m_action;

        // menu position
        float m_menu_pos_y;
        // default text color
        Color m_text_color;
        // value text color
        Color m_text_color_value;
        // return to this game mode on exit
        GameMode m_exit_to_gamemode;

        // current menu sprites
        typedef vector<cHudSprite*> HudSpriteList;
        HudSpriteList m_draw_list;
    };

    /* *** *** *** *** *** *** *** cMenu_Main *** *** *** *** *** *** *** *** *** *** */

    class cMenu_Main : public cMenu_Base {
    public:
        cMenu_Main(void);
        virtual ~cMenu_Main(void);

        virtual void Init(void);
        virtual void Init_GUI(void);
        virtual void Exit(void);
        virtual void Update(void);
        virtual void Draw(void);
    };

    /* *** *** *** *** *** *** *** cMenu_Start *** *** *** *** *** *** *** *** *** *** */

    class cMenu_Start : public cMenu_Base {
    public:
        cMenu_Start(void);
        virtual ~cMenu_Start(void);

        virtual void Init(void);
        virtual void Init_GUI(void);
        virtual void Exit(void);
        virtual void Update(void);
        virtual void Draw(void);

        // Get all levels from the given directory
        void Get_Levels(boost::filesystem::path dir, CEGUI::colour color);

        /* Highlight the given level
         * and activates level tab if needed
        */
        bool Highlight_Level(std::string lvl_name);
        /* Load the Selected Listbox item
         * and exit if successful
        */
        void Load_Selected(void);

        /* Load the Campaign
         * and exit if successful
        */
        void Load_Campaign(std::string name);
        /* Load the World
         * and exit if successful
        */
        void Load_World(std::string name);
        /* Load the Level
         * and exit if successful
        */
        bool Load_Level(std::string name);

        // tabcontrol selection changed event
        bool TabControl_Selection_Changed(const CEGUI::EventArgs& event);
        // key down event
        bool TabControl_Keydown(const CEGUI::EventArgs& event);
        // listbox level/world key down event
        bool Listbox_Keydown(const CEGUI::EventArgs& event);
        // listbox level/world character key event
        bool Listbox_Character_Key(const CEGUI::EventArgs& event);

        // campaign selected event
        bool Campaign_Select(const CEGUI::EventArgs& event);
        // campaign selected for entering event
        bool Campaign_Select_final_list(const CEGUI::EventArgs& event);

        // world selected event
        bool World_Select(const CEGUI::EventArgs& event);
        // world selected for entering event
        bool World_Select_final_list(const CEGUI::EventArgs& event);

        // level selected event
        bool Level_Select(const CEGUI::EventArgs& event);
        // level selected for entering event
        bool Level_Select_Final_List(const CEGUI::EventArgs& event);

        // level new button event
        bool Button_Level_New_Clicked(const CEGUI::EventArgs& event);
        // level edit button event
        bool Button_Level_Edit_Clicked(const CEGUI::EventArgs& event);
        // level delete button event
        bool Button_Level_Delete_Clicked(const CEGUI::EventArgs& event);
        // enter button event
        bool Button_Enter_Clicked(const CEGUI::EventArgs& event);
        // back button event
        bool Button_Back_Clicked(const CEGUI::EventArgs& event);

        // buffer if user types characters in the listbox
        CEGUI::String m_listbox_search_buffer;
        // counter until buffer is cleared
        float m_listbox_search_buffer_counter;
    };

    /* *** *** *** *** *** *** *** cMenu_Options *** *** *** *** *** *** *** *** *** *** */

    class cMenu_Options : public cMenu_Base {
    public:
        cMenu_Options(void);
        virtual ~cMenu_Options(void);

        virtual void Init(void);
        virtual void Init_GUI(void);
        void Init_GUI_Game(void);
        void Init_GUI_Video(void);
        void Init_GUI_Audio(void);
        void Init_GUI_Keyboard(void);
        void Init_GUI_Joystick(void);
        void Init_GUI_Editor(void);
        virtual void Exit(void);
        virtual void Update(void);
        void Change_Game_Setting(int setting);
        void Change_Video_Setting(int setting);
        void Change_Audio_Setting(int setting);
        void Change_Keyboard_Setting(int setting);
        void Change_Joystick_Setting(int setting);
        void Change_Editor_Setting(int setting);
        virtual void Draw(void);

        /* Build the shortcut list
         * joystick : if true build it for joystick
        */
        void Build_Shortcut_List(bool joystick = 0);
        /* Set the given Shortcut
         * and exit if successful
         * joystick : if true set it for joystick
        */
        void Set_Shortcut(std::string name, void* data, bool joystick = 0);

        // Select given Joystick
        void Joy_Default(unsigned int index);
        // Disable Joystick
        void Joy_Disable(void);

        bool Button_Back_Click(const CEGUI::EventArgs& event);
        // game
        bool Game_Always_Run_Select(const CEGUI::EventArgs& event);
        bool Game_Camera_Hor_Select(const CEGUI::EventArgs& event);
        bool Game_Camera_Ver_Select(const CEGUI::EventArgs& event);
        bool Game_Language_Select(const CEGUI::EventArgs& event);
        bool Game_Menu_Level_Select(const CEGUI::EventArgs& event);
        bool Game_Menu_Level_Text_Changed(const CEGUI::EventArgs& event);
        bool Game_Button_Reset_Game_Clicked(const CEGUI::EventArgs& event);
        // video
        bool Video_Resolution_Select(const CEGUI::EventArgs& event);
        bool Video_Bpp_Select(const CEGUI::EventArgs& event);
        bool Video_Fullscreen_Select(const CEGUI::EventArgs& event);
        bool Video_Vsync_Select(const CEGUI::EventArgs& event);
        bool Video_FPS_Limit_Select(const CEGUI::EventArgs& event);
        bool Video_Slider_Geometry_Quality_Changed(const CEGUI::EventArgs& event);
        bool Video_Slider_Texture_Quality_Changed(const CEGUI::EventArgs& event);
        bool Video_Button_Reset_Clicked(const CEGUI::EventArgs& event);
        bool Video_Button_Apply_Clicked(const CEGUI::EventArgs& event);
        bool Video_Button_Recreate_Cache_Clicked(const CEGUI::EventArgs& event);
        // audio
        bool Audio_Hz_Select(const CEGUI::EventArgs& event);
        bool Audio_Music_Select(const CEGUI::EventArgs& event);
        bool Audio_Music_Volume_Changed(const CEGUI::EventArgs& event);
        bool Audio_Sound_Select(const CEGUI::EventArgs& event);
        bool Audio_Sound_Volume_Changed(const CEGUI::EventArgs& event);
        bool Audio_Button_Reset_Clicked(const CEGUI::EventArgs& event);
        bool Keyboard_List_Double_Click(const CEGUI::EventArgs& event);
        // keyboard
        bool Keyboard_Slider_Scroll_Speed_Changed(const CEGUI::EventArgs& event);
        bool Keyboard_Button_Reset_Clicked(const CEGUI::EventArgs& event);
        // joystick
        bool Joystick_Name_Click(const CEGUI::EventArgs& event);
        bool Joystick_Name_Select(const CEGUI::EventArgs& event);
        bool Joystick_Sensitivity_Changed(const CEGUI::EventArgs& event);
        bool Joystick_Analog_Jump_Select(const CEGUI::EventArgs& event);
        bool Joystick_Spinner_Axis_Hor_Changed(const CEGUI::EventArgs& event);
        bool Joystick_Spinner_Axis_Ver_Changed(const CEGUI::EventArgs& event);
        bool Joystick_List_Double_Click(const CEGUI::EventArgs& event);
        bool Joystick_Button_Reset_Clicked(const CEGUI::EventArgs& event);
        // editor
        bool Game_Editor_Show_Item_Images_Select(const CEGUI::EventArgs& event);
        bool Game_Editor_Item_Image_Size_Select(const CEGUI::EventArgs& event);
        bool Game_Editor_Auto_Hide_Mouse_Select(const CEGUI::EventArgs& event);
        bool Game_Button_Reset_Editor_Clicked(const CEGUI::EventArgs& event);

        CEGUI::TabControl* m_tabcontrol;
        // game
        CEGUI::Combobox* m_game_combo_always_run;
        CEGUI::Spinner* m_game_spinner_camera_hor_speed;
        CEGUI::Spinner* m_game_spinner_camera_ver_speed;
        CEGUI::Combobox* m_game_combo_language;
        CEGUI::Combobox* m_game_combo_menu_level;
        // game editor
        CEGUI::Combobox* m_game_combo_editor_show_item_images;
        CEGUI::Spinner* m_game_spinner_editor_item_image_size;
        CEGUI::Combobox* m_game_combo_editor_mouse_auto_hide;
        // video
        CEGUI::Combobox* m_video_combo_resolution;
        CEGUI::Combobox* m_video_combo_bpp;
        CEGUI::Combobox* m_video_combo_fullscreen;
        CEGUI::Combobox* m_video_combo_vsync;
        CEGUI::Spinner* m_video_spinner_fps_limit;
        CEGUI::Slider* m_video_slider_geometry_quality;
        CEGUI::Slider* m_video_slider_texture_quality;
        // video settings
        unsigned int m_vid_w;
        unsigned int m_vid_h;
        unsigned int m_vid_bpp;
        bool m_vid_fullscreen;
        bool m_vid_vsync;
        float m_vid_geometry_detail;
        float m_vid_texture_detail;
        // audio
        CEGUI::Combobox* m_audio_combo_hz;
        CEGUI::Combobox* m_audio_combo_music;
        CEGUI::Slider* m_audio_slider_music;
        CEGUI::Combobox* m_audio_combo_sounds;
        CEGUI::Slider* m_audio_slider_sound;

        // Shortcut item
        class cShortcut_item {
        public:
            cShortcut_item(const CEGUI::String& name, void* key, const void* key_default)
            {
                m_name = name;
                m_key = key;
                m_key_default = key_default;
            }

            CEGUI::String m_name;
            void* m_key;
            const void* m_key_default;
        };
    };

    /* *** *** *** *** *** *** *** cMenu_Savegames *** *** *** *** *** *** *** *** *** *** */

    class cMenu_Savegames : public cMenu_Base {
    public:
        cMenu_Savegames(bool type);
        virtual ~cMenu_Savegames(void);

        virtual void Init(void);
        virtual void Init_GUI(void);
        virtual void Exit(void);
        virtual void Update(void);
        virtual void Draw(void);

        void Update_Load(void);
        void Update_Save(void);

        // Set Savegame Description
        std::string Set_Save_Description(unsigned int save_slot);
        // Update Savegame Descriptions
        void Update_Saved_Games_Text(void);

        // Savegame images
        HudSpriteList m_savegame_temp;

        // if save menu
        bool m_type_save;
    };

    /* *** *** *** *** *** *** *** cMenu_Credits *** *** *** *** *** *** *** *** *** *** */

    class cMenu_Credits : public cMenu_Base {
    public:
        cMenu_Credits(void);
        virtual ~cMenu_Credits(void);

        virtual void Init(void);
        virtual void Init_GUI(void);
        virtual void Enter(const GameMode old_mode = MODE_NOTHING);
        virtual void Leave(const GameMode next_mode = MODE_NOTHING);
        virtual void Exit(void);
        virtual void Update(void);
        virtual void Draw(void);


        // Add a line to the credits text
        void Add_Credits_Line(const std::string& text, float posx, float posy, const Color& color = black, float shadow_pos = 0.0f, const Color& shadow_color = black);
        /* fade from the normal menu to the the credits menu
         * fade_in : if set fade in instead of fade out
        */
        void Menu_Fade(bool fade_in = 1);
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

/***************************************************************************
 * preferences.h
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

#ifndef TSC_PREFERENCES_HPP
#define TSC_PREFERENCES_HPP

#include "../core/global_basic.hpp"

namespace TSC {

    /* *** *** *** *** *** cPreferences *** *** *** *** *** *** *** *** *** *** *** *** */

    class cPreferences {
    public:
        cPreferences(void);
        virtual ~cPreferences(void);

        // Load the preferences from a file
        bool Load(const boost::filesystem::path& filename = boost::filesystem::path());
        // Save the preferences to a file
        void Save(void);

        // Load the preferences from a file, probably the one returned
        // by cResource_Manager::Get_Preferences_File().
        // The returned instance must be freed by you.
        static cPreferences* Load_From_File(boost::filesystem::path filename);

        // Reset the settings
        void Reset_All(void);
        void Reset_Game(void);
        void Reset_Video(void);
        void Reset_Audio(void);
        void Reset_Keyboard(void);
        void Reset_Joystick(void);
        void Reset_Editor(void);
        // Get settings from current game settings
        void Update(void);
        // Set settings to current game settings
        void Apply(void);
        // Set new video settings
        void Apply_Video(uint16_t screen_w, uint16_t screen_h, uint8_t screen_bpp, bool fullscreen, bool vsync, float geometry_detail, float texture_detail);
        // Set new audio settings
        void Apply_Audio(bool sound, bool music);

        /* *** *** *** Settings *** *** *** *** */

        // Game
        // last version of tsc which saved the preferences file
        unsigned int m_game_version;
        // default language
        std::string m_language;
        // player always runs
        bool m_always_run;
        // menu level name to load
        std::string m_menu_level;
        // smart camera speed
        float m_camera_hor_speed;
        float m_camera_ver_speed;

        // Audio
        bool m_audio_music;
        bool m_audio_sound;
        unsigned int m_audio_hz;

        // Video
        bool m_video_fullscreen;
        uint16_t m_video_screen_w;
        uint16_t m_video_screen_h;
        uint8_t m_video_screen_bpp;
        bool m_video_vsync;
        uint16_t m_video_fps_limit;

        // Keyboard
        // key definitions
        sf::Keyboard::Key m_key_up;
        sf::Keyboard::Key m_key_down;
        sf::Keyboard::Key m_key_left;
        sf::Keyboard::Key m_key_right;
        sf::Keyboard::Key m_key_jump;
        sf::Keyboard::Key m_key_shoot;
        sf::Keyboard::Key m_key_item;
        sf::Keyboard::Key m_key_action;
        sf::Keyboard::Key m_key_screenshot;
        sf::Keyboard::Key m_key_editor_fast_copy_up;
        sf::Keyboard::Key m_key_editor_fast_copy_down;
        sf::Keyboard::Key m_key_editor_fast_copy_left;
        sf::Keyboard::Key m_key_editor_fast_copy_right;
        sf::Keyboard::Key m_key_editor_pixel_move_up;
        sf::Keyboard::Key m_key_editor_pixel_move_down;
        sf::Keyboard::Key m_key_editor_pixel_move_left;
        sf::Keyboard::Key m_key_editor_pixel_move_right;
        // scroll speed
        float m_scroll_speed;
        // Joystick
        bool m_joy_enabled;
        // active joy name
        std::string m_joy_name;
        // hor/ver axes used
        static const int NUM_JOYSTICK_AXIS_TYPES = 2;
        sf::Joystick::Axis m_joy_axis_hor [NUM_JOYSTICK_AXIS_TYPES];
        sf::Joystick::Axis m_joy_axis_ver [NUM_JOYSTICK_AXIS_TYPES];
        // axis threshold
        float m_joy_axis_threshold;
        // button definitions
        uint8_t m_joy_button_jump;
        uint8_t m_joy_button_shoot;
        uint8_t m_joy_button_item;
        uint8_t m_joy_button_action;
        uint8_t m_joy_button_exit;

        // Editor
        // hide mouse if clicked on an object
        bool m_editor_mouse_auto_hide;
        // show item images
        bool m_editor_show_item_images;
        // size of the item images
        unsigned int m_editor_item_image_size;

        // Special
        // level background images enabled
        bool m_level_background_images;
        // image cache enabled
        bool m_image_cache_enabled;

        /* *** *** *** *** *** *** *** */

        // configuration filename
        boost::filesystem::path m_config_filename;

        /* *** *** *** defaults *** *** *** *** */

        // Default preferences path
        // TODO: All constants should be uppercase. Certainly
        // they are not simple instance members as denoted by "m_"!
        static const boost::filesystem::path DEFAULT_PREFERENCES_FILENAME;
        // Game
        static const bool m_always_run_default;
        static const std::string m_menu_level_default;
        static const float m_camera_hor_speed_default;
        static const float m_camera_ver_speed_default;
        // Audio
        static const bool m_audio_music_default;
        static const bool m_audio_sound_default;
        static const unsigned int m_audio_hz_default;
        static const uint8_t m_sound_volume_default;
        static const uint8_t m_music_volume_default;
        // Video
        static const bool m_video_fullscreen_default;
        static const uint16_t m_video_screen_w_default;
        static const uint16_t m_video_screen_h_default;
        static const uint8_t m_video_screen_bpp_default;
        static const bool m_video_vsync_default;
        static const uint16_t m_video_fps_limit_default;
        static const float m_geometry_quality_default;
        static const float m_texture_quality_default;
        // Keyboard
        static const sf::Keyboard::Key m_key_up_default;
        static const sf::Keyboard::Key m_key_down_default;
        static const sf::Keyboard::Key m_key_left_default;
        static const sf::Keyboard::Key m_key_right_default;
        static const sf::Keyboard::Key m_key_jump_default;
        static const sf::Keyboard::Key m_key_shoot_default;
        static const sf::Keyboard::Key m_key_item_default;
        static const sf::Keyboard::Key m_key_action_default;
        static const sf::Keyboard::Key m_key_screenshot_default;
        static const sf::Keyboard::Key m_key_editor_fast_copy_up_default;
        static const sf::Keyboard::Key m_key_editor_fast_copy_down_default;
        static const sf::Keyboard::Key m_key_editor_fast_copy_left_default;
        static const sf::Keyboard::Key m_key_editor_fast_copy_right_default;
        static const sf::Keyboard::Key m_key_editor_pixel_move_up_default;
        static const sf::Keyboard::Key m_key_editor_pixel_move_down_default;
        static const sf::Keyboard::Key m_key_editor_pixel_move_left_default;
        static const sf::Keyboard::Key m_key_editor_pixel_move_right_default;
        static const float m_scroll_speed_default;
        // Joystick
        static const bool m_joy_enabled_default;
        static const sf::Joystick::Axis m_joy_axis_hor_default[NUM_JOYSTICK_AXIS_TYPES];
        static const sf::Joystick::Axis m_joy_axis_ver_default[NUM_JOYSTICK_AXIS_TYPES];
        static const float m_joy_axis_threshold_default;
        static const uint8_t m_joy_button_jump_default;
        static const uint8_t m_joy_button_shoot_default;
        static const uint8_t m_joy_button_item_default;
        static const uint8_t m_joy_button_action_default;
        static const uint8_t m_joy_button_exit_default;
        // Editor
        static const bool m_editor_mouse_auto_hide_default;
        static const bool m_editor_show_item_images_default;
        static const unsigned int m_editor_item_image_size_default;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

// The Preferences
    extern cPreferences* pPreferences;

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC


#endif

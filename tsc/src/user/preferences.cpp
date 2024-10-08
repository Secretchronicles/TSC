/***************************************************************************
 * preferences.cpp  -  Game settings handler
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

#include "../user/preferences.hpp"
#include "../audio/audio.hpp"
#include "../video/video.hpp"
#include "../core/game_core.hpp"
#include "../input/joystick.hpp"
#include "../level/level_manager.hpp"
#include "../core/i18n.hpp"
#include "../core/filesystem/resource_manager.hpp"
#include "../core/filesystem/filesystem.hpp"
#include "preferences_loader.hpp"
#include "../core/global_basic.hpp"
#include "../gui/hud.hpp"

namespace fs = boost::filesystem;

using namespace std;

namespace TSC {

/* *** *** *** *** *** *** *** cPreferences *** *** *** *** *** *** *** *** *** *** */

// Game
const bool cPreferences::m_always_run_default = 0;
const std::string cPreferences::m_menu_level_default = "menu_brown_1";
const float cPreferences::m_camera_hor_speed_default = 0.3f;
const float cPreferences::m_camera_ver_speed_default = 0.2f;
// Video
const bool cPreferences::m_video_fullscreen_default = 0;
const uint16_t cPreferences::m_video_screen_w_default = 1024;
const uint16_t cPreferences::m_video_screen_h_default = 768;
const uint8_t cPreferences::m_video_screen_bpp_default = 32;
/* disable by default because of possible bad drivers
 * which can't handle visual sync
*/
const bool cPreferences::m_video_vsync_default = 0;
const uint16_t cPreferences::m_video_fps_limit_default = 240;
// default geometry detail is medium
const float cPreferences::m_geometry_quality_default = 0.5f;
// default texture detail is high
const float cPreferences::m_texture_quality_default = 0.75f;
// Audio
const bool cPreferences::m_audio_music_default = 1;
const bool cPreferences::m_audio_sound_default = 1;
const unsigned int cPreferences::m_audio_hz_default = 44100;
const uint8_t cPreferences::m_sound_volume_default = 100;
const uint8_t cPreferences::m_music_volume_default = 80;
// Keyboard
const sf::Keyboard::Key cPreferences::m_key_up_default = sf::Keyboard::Up;
const sf::Keyboard::Key cPreferences::m_key_down_default = sf::Keyboard::Down;
const sf::Keyboard::Key cPreferences::m_key_left_default = sf::Keyboard::Left;
const sf::Keyboard::Key cPreferences::m_key_right_default = sf::Keyboard::Right;
const sf::Keyboard::Key cPreferences::m_key_jump_default = sf::Keyboard::S;
const sf::Keyboard::Key cPreferences::m_key_shoot_default = sf::Keyboard::Space;
const sf::Keyboard::Key cPreferences::m_key_item_default = sf::Keyboard::Return;
const sf::Keyboard::Key cPreferences::m_key_action_default = sf::Keyboard::A;
const sf::Keyboard::Key cPreferences::m_key_screenshot_default = sf::Keyboard::F9;
const sf::Keyboard::Key cPreferences::m_key_editor_fast_copy_up_default = sf::Keyboard::Numpad8;
const sf::Keyboard::Key cPreferences::m_key_editor_fast_copy_down_default = sf::Keyboard::Numpad2;
const sf::Keyboard::Key cPreferences::m_key_editor_fast_copy_left_default = sf::Keyboard::Numpad4;
const sf::Keyboard::Key cPreferences::m_key_editor_fast_copy_right_default = sf::Keyboard::Numpad6;
const sf::Keyboard::Key cPreferences::m_key_editor_pixel_move_up_default = sf::Keyboard::Numpad8;
const sf::Keyboard::Key cPreferences::m_key_editor_pixel_move_down_default = sf::Keyboard::Numpad2;
const sf::Keyboard::Key cPreferences::m_key_editor_pixel_move_left_default = sf::Keyboard::Numpad4;
const sf::Keyboard::Key cPreferences::m_key_editor_pixel_move_right_default = sf::Keyboard::Numpad6;
const float cPreferences::m_scroll_speed_default = 1.0f;
// Joystick
const bool cPreferences::m_joy_enabled_default = 1;
const sf::Joystick::Axis cPreferences::m_joy_axis_hor_default[2] = {sf::Joystick::Axis::X, sf::Joystick::Axis::PovX};
const sf::Joystick::Axis cPreferences::m_joy_axis_ver_default[2] = {sf::Joystick::Axis::Y, sf::Joystick::Axis::PovY};
const float cPreferences::m_joy_axis_threshold_default = 0.1f;
const uint8_t cPreferences::m_joy_button_jump_default = 0;
const uint8_t cPreferences::m_joy_button_shoot_default = 1;
const uint8_t cPreferences::m_joy_button_item_default = 3;
const uint8_t cPreferences::m_joy_button_action_default = 2;
const uint8_t cPreferences::m_joy_button_exit_default = 4;
// Editor
const bool cPreferences::m_editor_mouse_auto_hide_default = 0;
const bool cPreferences::m_editor_show_item_images_default = 1;
const unsigned int cPreferences::m_editor_item_image_size_default = 50;

cPreferences::cPreferences(void)
{
    Reset_All();
}

cPreferences::~cPreferences(void)
{
    //
}

cPreferences* cPreferences::Load_From_File(fs::path filename)
{
    // If the preferences file doesn’t exist, use default values.
    if (!File_Exists(filename)) {
        cerr << "Warning: Preferences file '" << path_to_utf8(filename) << "' does not exist. Using default values." << endl;
        cPreferences* p_pref = new cPreferences();
        p_pref->m_config_filename = filename;
        return p_pref;
    }

    cPreferencesLoader loader;
    loader.parse_file(filename);

    // FIXME: Merge these settings with the other ordinary settings
    // in a cPreferences instance! The following lines set global
    // variables, which from the outside is totally unexpected.
    // This must be done in main.cpp instead, where the preferences
    // are loaded!
    pVideo->m_geometry_quality = loader.Get_Video_Geometry_Detail();
    pVideo->m_texture_quality  = loader.Get_Video_Texture_Detail();
    pAudio->m_music_volume     = loader.Get_Audio_Music_Volume();
    pAudio->m_sound_volume     = loader.Get_Audio_Sound_Volume();

    return loader.Get_Preferences();
}

void cPreferences::Save(void)
{
    Update();

    xmlpp::Document doc;
    xmlpp::Element* p_root = doc.create_root_node("config");

    // Game
    Add_Property(p_root, "game_version", int_to_string(TSC_VERSION_MAJOR) + "." + int_to_string(TSC_VERSION_MINOR) + "." + int_to_string(TSC_VERSION_PATCH));
    Add_Property(p_root, "game_language", m_language);
    Add_Property(p_root, "game_always_run", m_always_run);
    Add_Property(p_root, "game_menu_level", m_menu_level);
    Add_Property(p_root, "game_camera_hor_speed", m_camera_hor_speed);
    Add_Property(p_root, "game_camera_ver_speed", m_camera_ver_speed);
    // Video
    Add_Property(p_root, "video_fullscreen", m_video_fullscreen);
    Add_Property(p_root, "video_screen_w", m_video_screen_w);
    Add_Property(p_root, "video_screen_h", m_video_screen_h);
    Add_Property(p_root, "video_screen_bpp", static_cast<int>(m_video_screen_bpp));
    Add_Property(p_root, "video_vsync", m_video_vsync);
    Add_Property(p_root, "video_fps_limit", m_video_fps_limit);
    Add_Property(p_root, "video_geometry_quality", pVideo->m_geometry_quality);
    Add_Property(p_root, "video_texture_quality", pVideo->m_texture_quality);
    // Audio
    Add_Property(p_root, "audio_music", m_audio_music);
    Add_Property(p_root, "audio_sound", m_audio_sound);
    Add_Property(p_root, "audio_sound_volume", static_cast<int>(pAudio->m_sound_volume));
    Add_Property(p_root, "audio_music_volume", static_cast<int>(pAudio->m_music_volume));
    Add_Property(p_root, "audio_hz", m_audio_hz);
    // Keyboard
    Add_Property(p_root, "keyboard_key_up", m_key_up);
    Add_Property(p_root, "keyboard_key_down", m_key_down);
    Add_Property(p_root, "keyboard_key_left", m_key_left);
    Add_Property(p_root, "keyboard_key_right", m_key_right);
    Add_Property(p_root, "keyboard_key_jump", m_key_jump);
    Add_Property(p_root, "keyboard_key_shoot", m_key_shoot);
    Add_Property(p_root, "keyboard_key_item", m_key_item);
    Add_Property(p_root, "keyboard_key_action", m_key_action);
    Add_Property(p_root, "keyboard_scroll_speed", m_scroll_speed);
    Add_Property(p_root, "keyboard_key_screenshot", m_key_screenshot);
    Add_Property(p_root, "keyboard_key_editor_fast_copy_up", m_key_editor_fast_copy_up);
    Add_Property(p_root, "keyboard_key_editor_fast_copy_down", m_key_editor_fast_copy_down);
    Add_Property(p_root, "keyboard_key_editor_fast_copy_left", m_key_editor_fast_copy_left);
    Add_Property(p_root, "keyboard_key_editor_fast_copy_right", m_key_editor_fast_copy_right);
    Add_Property(p_root, "keyboard_key_editor_pixel_move_up", m_key_editor_pixel_move_up);
    Add_Property(p_root, "keyboard_key_editor_pixel_move_down", m_key_editor_pixel_move_down);
    Add_Property(p_root, "keyboard_key_editor_pixel_move_left", m_key_editor_pixel_move_left);
    Add_Property(p_root, "keyboard_key_editor_pixel_move_right", m_key_editor_pixel_move_right);
    // Joystick/Gamepad
    Add_Property(p_root, "joy_enabled", m_joy_enabled);
    Add_Property(p_root, "joy_name", m_joy_name);
    Add_Property(p_root, "joy_axis_hor", m_joy_axis_hor[0]);
    Add_Property(p_root, "joy_axis_ver", m_joy_axis_ver[0]);
    Add_Property(p_root, "joy_axis_hor2", m_joy_axis_hor[1]);
    Add_Property(p_root, "joy_axis_ver2", m_joy_axis_ver[1]);
    Add_Property(p_root, "joy_axis_threshold", (float)m_joy_axis_threshold);
    Add_Property(p_root, "joy_button_jump", static_cast<int>(m_joy_button_jump));
    Add_Property(p_root, "joy_button_item", static_cast<int>(m_joy_button_item));
    Add_Property(p_root, "joy_button_shoot", static_cast<int>(m_joy_button_shoot));
    Add_Property(p_root, "joy_button_action", static_cast<int>(m_joy_button_action));
    Add_Property(p_root, "joy_button_exit", static_cast<int>(m_joy_button_exit));
    // Special
    Add_Property(p_root, "level_background_images", m_level_background_images);
    Add_Property(p_root, "image_cache_enabled", m_image_cache_enabled);
    // Editor
    Add_Property(p_root, "editor_mouse_auto_hide", m_editor_mouse_auto_hide);
    Add_Property(p_root, "editor_show_item_images", m_editor_show_item_images);
    Add_Property(p_root, "editor_item_image_size", m_editor_item_image_size);

    doc.write_to_file_formatted(Glib::filename_from_utf8(path_to_utf8(m_config_filename)));
    debug_print("Wrote preferences file '%s'.\n", path_to_utf8(m_config_filename).c_str());
}

void cPreferences::Reset_All(void)
{
    // Game
    m_game_version = tsc_version;

    Reset_Game();
    Reset_Video();
    Reset_Audio();
    Reset_Keyboard();
    Reset_Joystick();
    Reset_Editor();

    // Special
    m_level_background_images = 1;
    m_image_cache_enabled = 1;
}

void cPreferences::Reset_Game(void)
{
    m_language = "";
    m_always_run = m_always_run_default;
    m_menu_level = m_menu_level_default;
    m_camera_hor_speed = m_camera_hor_speed_default;
    m_camera_ver_speed = m_camera_ver_speed_default;
}

void cPreferences::Reset_Video(void)
{
    // Video
    m_video_screen_w = m_video_screen_w_default;
    m_video_screen_h = m_video_screen_h_default;
    m_video_screen_bpp = m_video_screen_bpp_default;
    m_video_vsync = m_video_vsync_default;
    m_video_fps_limit = m_video_fps_limit_default;
    m_video_fullscreen = m_video_fullscreen_default;
    pVideo->m_geometry_quality = m_geometry_quality_default;
    pVideo->m_texture_quality = m_texture_quality_default;
}

void cPreferences::Reset_Audio(void)
{
    // Audio
    m_audio_music = m_audio_music_default;
    m_audio_sound = m_audio_sound_default;
    m_audio_hz = m_audio_hz_default;
    pAudio->m_sound_volume = m_sound_volume_default;
    pAudio->m_music_volume = m_music_volume_default;
}

void cPreferences::Reset_Keyboard(void)
{
    m_key_up = m_key_up_default;
    m_key_down = m_key_down_default;
    m_key_left = m_key_left_default;
    m_key_right = m_key_right_default;
    m_key_jump = m_key_jump_default;
    m_key_shoot = m_key_shoot_default;
    m_key_item = m_key_item_default;
    m_key_action = m_key_action_default;
    m_scroll_speed = m_scroll_speed_default;
    m_key_screenshot = m_key_screenshot_default;
    m_key_editor_fast_copy_up = m_key_editor_fast_copy_up_default;
    m_key_editor_fast_copy_down = m_key_editor_fast_copy_down_default;
    m_key_editor_fast_copy_left = m_key_editor_fast_copy_left_default;
    m_key_editor_fast_copy_right = m_key_editor_fast_copy_right_default;
    m_key_editor_pixel_move_up = m_key_editor_pixel_move_up_default;
    m_key_editor_pixel_move_down = m_key_editor_pixel_move_down_default;
    m_key_editor_pixel_move_left = m_key_editor_pixel_move_left_default;
    m_key_editor_pixel_move_right = m_key_editor_pixel_move_right_default;
}

void cPreferences::Reset_Joystick(void)
{
    m_joy_enabled = m_joy_enabled_default;
    m_joy_name.clear();
    // axes
    for (int i = 0; i < cPreferences::NUM_JOYSTICK_AXIS_TYPES; i++)
    {
        m_joy_axis_hor[i] = m_joy_axis_hor_default[i];
        m_joy_axis_ver[i] = m_joy_axis_ver_default[i];
    }
    // axis threshold
    m_joy_axis_threshold = m_joy_axis_threshold_default;
    // buttons
    m_joy_button_jump = m_joy_button_jump_default;
    m_joy_button_shoot = m_joy_button_shoot_default;
    m_joy_button_item = m_joy_button_item_default;
    m_joy_button_action = m_joy_button_action_default;
    m_joy_button_exit = m_joy_button_exit_default;
}

void cPreferences::Reset_Editor(void)
{
    m_editor_mouse_auto_hide = m_editor_mouse_auto_hide_default;
    m_editor_show_item_images = m_editor_show_item_images_default;
    m_editor_item_image_size = m_editor_item_image_size_default;
}

void cPreferences::Update(void)
{
    m_camera_hor_speed = pLevel_Manager->m_camera->m_hor_offset_speed;
    m_camera_ver_speed = pLevel_Manager->m_camera->m_ver_offset_speed;

    m_audio_music = pAudio->m_music_enabled;
    m_audio_sound = pAudio->m_sound_enabled;

    // if not default joy used
    if (m_joy_enabled && pJoystick->m_current_joystick != 999) {
        m_joy_name = pJoystick->Get_Name();
    }
    // using default joy
    else {
        m_joy_name.clear();
    }
}

void cPreferences::Apply(void)
{
    pLevel_Manager->m_camera->m_hor_offset_speed = m_camera_hor_speed;
    pLevel_Manager->m_camera->m_ver_offset_speed = m_camera_ver_speed;

    // disable joystick if the joystick initialization failed
    if (pVideo->m_joy_init_failed) {
        m_joy_enabled = 0;
    }
}

void cPreferences::Apply_Video(uint16_t screen_w, uint16_t screen_h, uint8_t screen_bpp, bool fullscreen, bool vsync, float geometry_detail, float texture_detail)
{
    /* if resolution, bpp, vsync or texture detail changed
     * a texture reload is necessary
    */
    if (m_video_screen_w != screen_w || m_video_screen_h != screen_h || m_video_screen_bpp != screen_bpp || m_video_vsync != vsync || !Is_Float_Equal(pVideo->m_texture_quality, texture_detail)) {
        // new settings
        m_video_screen_w = screen_w;
        m_video_screen_h = screen_h;
        m_video_screen_bpp = screen_bpp;
        m_video_vsync = vsync;
        m_video_fullscreen = fullscreen;
        pVideo->m_texture_quality = texture_detail;
        pVideo->m_geometry_quality = geometry_detail;

        // reinitialize video and reload textures from file
        pVideo->Init_Video(1);
    }
    // no texture reload necessary
    else {
        // geometry detail changed
        if (!Is_Float_Equal(pVideo->m_geometry_quality, geometry_detail)) {
            pVideo->m_geometry_quality = geometry_detail;
            pVideo->Init_Geometry();
        }

        // fullscreen changed
        if (m_video_fullscreen != fullscreen) {
            // toggle fullscreen and switches video_fullscreen itself
            pVideo->Toggle_Fullscreen();
        }
    }
}

void cPreferences::Apply_Audio(bool sound, bool music)
{
    // disable sound and music if the audio initialization failed
    if (pVideo->m_audio_init_failed) {
        m_audio_sound = 0;
        m_audio_music = 0;
        return;
    }

    m_audio_sound = sound;
    m_audio_music = music;

    // init audio settings
    pAudio->Init();
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

/// User preferences, i.e. the content from the TSC configuration file.
cPreferences* pPreferences = NULL;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

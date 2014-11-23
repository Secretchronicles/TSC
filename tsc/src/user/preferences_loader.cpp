/***************************************************************************
 * preferences_loader.cpp - Loading preferences XML
 *
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

#include "preferences_loader.hpp"

namespace fs = boost::filesystem;
using namespace TSC;

cPreferencesLoader::cPreferencesLoader()
    : xmlpp::SaxParser()
{
    mp_preferences = NULL;
    m_video_geometry_detail = -1.0;
    m_video_texture_detail = -1.0;
    m_audio_music_volume = -1;
    m_audio_sound_volume = -1;
}

cPreferencesLoader::~cPreferencesLoader()
{
    // Do not delete the cPreferences instance — it is
    // used by the caller and deleted by him.
    mp_preferences = NULL;
}

cPreferences* cPreferencesLoader::Get_Preferences()
{
    return mp_preferences;
}

float cPreferencesLoader::Get_Video_Geometry_Detail()
{
    return m_video_geometry_detail;
}

float cPreferencesLoader::Get_Video_Texture_Detail()
{
    return m_video_texture_detail;
}

int cPreferencesLoader::Get_Audio_Music_Volume()
{
    return m_audio_music_volume;
}

int cPreferencesLoader::Get_Audio_Sound_Volume()
{
    return m_audio_sound_volume;
}

/***************************************
 * SAX parser callbacks
 ***************************************/

void cPreferencesLoader::parse_file(fs::path filename)
{
    m_preferences_file = filename;

    xmlpp::SaxParser::parse_file(path_to_utf8(filename));
}

void cPreferencesLoader::on_start_document()
{
    if (mp_preferences)
        throw("Restarted XML parser after already starting it."); // FIXME: Proper exception

    mp_preferences = new cPreferences();
    mp_preferences->m_config_filename = m_preferences_file;
}

void cPreferencesLoader::on_end_document()
{
    //
}

void cPreferencesLoader::on_start_element(const Glib::ustring& name, const xmlpp::SaxParser::AttributeList& properties)
{
    if (name != "property" && name != "Item")
        return;

    std::string key;
    std::string value;

    xmlpp::SaxParser::AttributeList::const_iterator iter;
    for (iter=properties.begin(); iter != properties.end(); iter++) {
        xmlpp::SaxParser::Attribute attr = *iter;

        if (attr.name == "name" || attr.name == "Name")
            key = attr.value;
        else if (attr.name == "value" || attr.name == "Value")
            value = attr.value;
    }

    handle_property(key, value);
}

void cPreferencesLoader::handle_property(const std::string& name, const std::string& value)
{
    int val = 0;

    //////////////////// Game ////////////////////
    if (name == "game_version")
        mp_preferences->m_game_version = string_to_version_number(value);
    else if (name == "game_language")
        mp_preferences->m_language = value;
    else if (name == "game_always_run" || name == "always_run")
        mp_preferences->m_always_run = string_to_bool(value);
    else if (name == "game_menu_level")
        mp_preferences->m_menu_level = value;
    else if (name == "game_camera_hor_speed" || name == "camera_hor_speed")
        mp_preferences->m_camera_hor_speed = string_to_float(value);
    else if (name == "game_camera_ver_speed" || name == "camera_ver_speed")
        mp_preferences->m_camera_ver_speed = string_to_float(value);
    //////////////////// Video ////////////////////
    else if (name == "video_screen_h") {
        val = string_to_int(value);
        if (val < 200)
            val = 200;
        else if (val > 2560)
            val = 2560;

        mp_preferences->m_video_screen_h = val;
    }
    else if (name == "video_screen_w") {
        val = string_to_int(value);
        if (val < 200)
            val = 200;
        else if (val > 2560)
            val = 2560;

        mp_preferences->m_video_screen_w = val;
    }
    else if (name == "video_screen_bpp") {
        val = string_to_int(value);
        if (val < 8)
            val = 8;
        else if (val > 32)
            val = 32;

        mp_preferences->m_video_screen_bpp = val;
    }
    else if (name == "video_vsync")
        mp_preferences->m_video_vsync = string_to_bool(value);
    else if (name == "video_fps_limit")
        mp_preferences->m_video_fps_limit = string_to_int(value);
    else if (name == "video_fullscreen")
        mp_preferences->m_video_fullscreen = string_to_bool(value);
    else if (name == "video_geometry_detail" || name == "video_geometry_quality")
        m_video_geometry_detail = string_to_float(value);
    else if (name == "video_texture_detail" || name == "video_texture_quality")
        m_video_texture_detail = string_to_float(value);
    //////////////////// Audio ////////////////////
    else if (name == "audio_music")
        mp_preferences->m_audio_music = string_to_bool(value);
    else if (name == "audio_sound")
        mp_preferences->m_audio_sound = string_to_bool(value);
    else if (name == "audio_music_volume") {
        val = string_to_int(value);
        if (val >= 0 && val <= MIX_MAX_VOLUME)
            m_audio_music_volume = val;
    }
    else if (name == "audio_sound_volume") {
        val = string_to_int(value);
        if (val >= 0 && val <= MIX_MAX_VOLUME)
            m_audio_sound_volume = val;
    }
    else if (name == "audio_hz") {
        val = string_to_int(value);
        if (val >= 0 && val <= 96000)
            mp_preferences->m_audio_hz = val;
    }
    //////////////////// Keyboard ////////////////////
    else if (name == "keyboard_key_up") {
        val = string_to_int(value);
        if (val >= 0 && val <= SDLK_LAST)
            mp_preferences->m_key_up = static_cast<SDLKey>(val);
    }
    else if (name == "keyboard_key_down") {
        val = string_to_int(value);
        if (val >= 0 && val <= SDLK_LAST)
            mp_preferences->m_key_down = static_cast<SDLKey>(val);
    }
    else if (name == "keyboard_key_left") {
        val = string_to_int(value);
        if (val >= 0 && val <= SDLK_LAST)
            mp_preferences->m_key_left = static_cast<SDLKey>(val);
    }
    else if (name == "keyboard_key_right") {
        val = string_to_int(value);
        if (val >= 0 && val <= SDLK_LAST)
            mp_preferences->m_key_right = static_cast<SDLKey>(val);
    }
    else if (name == "keyboard_key_jump") {
        val = string_to_int(value);
        if (val >= 0 && val <= SDLK_LAST)
            mp_preferences->m_key_jump = static_cast<SDLKey>(val);
    }
    else if (name == "keyboard_key_shoot") {
        val = string_to_int(value);
        if (val >= 0 && val <= SDLK_LAST)
            mp_preferences->m_key_shoot = static_cast<SDLKey>(val);
    }
    else if (name == "keyboard_key_item") {
        val = string_to_int(value);
        if (val >= 0 && val <= SDLK_LAST)
            mp_preferences->m_key_item = static_cast<SDLKey>(val);
    }
    else if (name == "keyboard_key_action") {
        val = string_to_int(value);
        if (val >= 0 && val <= SDLK_LAST)
            mp_preferences->m_key_action = static_cast<SDLKey>(val);
    }
    else if (name == "keyboard_scroll_speed")
        mp_preferences->m_scroll_speed = string_to_float(value);
    else if (name == "keyboard_key_screenshot") {
        val = string_to_int(value);
        if (val >= 0 && val <= SDLK_LAST)
            mp_preferences->m_key_screenshot = static_cast<SDLKey>(val);
    }
    else if (name == "keyboard_key_editor_fast_copy_up") {
        val = string_to_int(value);
        if (val >= 0 && val <= SDLK_LAST)
            mp_preferences->m_key_editor_fast_copy_up = static_cast<SDLKey>(val);
    }
    else if (name == "keyboard_key_editor_fast_copy_down") {
        val = string_to_int(value);
        if (val >= 0 && val <= SDLK_LAST)
            mp_preferences->m_key_editor_fast_copy_down = static_cast<SDLKey>(val);
    }
    else if (name == "keyboard_key_editor_fast_copy_left") {
        val = string_to_int(value);
        if (val >= 0 && val <= SDLK_LAST)
            mp_preferences->m_key_editor_fast_copy_left = static_cast<SDLKey>(val);
    }
    else if (name == "keyboard_key_editor_fast_copy_right") {
        val = string_to_int(value);
        if (val >= 0 && val <= SDLK_LAST)
            mp_preferences->m_key_editor_fast_copy_right = static_cast<SDLKey>(val);
    }
    else if (name == "keyboard_key_editor_pixel_move_up") {
        val = string_to_int(value);
        if (val >= 0 && val <= SDLK_LAST)
            mp_preferences->m_key_editor_pixel_move_up = static_cast<SDLKey>(val);
    }
    else if (name == "keyboard_key_editor_pixel_move_down") {
        val = string_to_int(value);
        if (val >= 0 && val <= SDLK_LAST)
            mp_preferences->m_key_editor_pixel_move_down = static_cast<SDLKey>(val);
    }
    else if (name == "keyboard_key_editor_pixel_move_left") {
        val = string_to_int(value);
        if (val >= 0 && val <= SDLK_LAST)
            mp_preferences->m_key_editor_pixel_move_left = static_cast<SDLKey>(val);
    }
    else if (name == "keyboard_key_editor_pixel_move_right") {
        val = string_to_int(value);
        if (val >= 0 && val <= SDLK_LAST)
            mp_preferences->m_key_editor_pixel_move_right = static_cast<SDLKey>(val);
    }
    //////////////////// Joypad ////////////////////
    else if (name == "joy_enabled")
        mp_preferences->m_joy_enabled = string_to_bool(value);
    else if (name == "joy_name")
        mp_preferences->m_joy_name = value;
    else if (name == "joy_analog_jump")
        mp_preferences->m_joy_analog_jump = string_to_bool(value);
    else if (name == "joy_axis_hor") {
        val = string_to_int(value);
        if (val >= 0 && val <= 256)
            mp_preferences->m_joy_axis_hor = val;
    }
    else if (name == "joy_axis_ver") {
        val = string_to_int(value);
        if (val >= 0 && val <= 256)
            mp_preferences->m_joy_axis_ver = val;
    }
    else if (name == "joy_axis_threshold") {
        val = string_to_int(value);
        if (val >= 0 && val <= 32767)
            mp_preferences->m_joy_axis_threshold = val;
    }
    else if (name == "joy_button_jump") {
        val = string_to_int(value);
        if (val >= 0 && val <= 256)
            mp_preferences->m_joy_button_jump = val;
    }
    else if (name == "joy_button_item") {
        val = string_to_int(value);
        if (val >= 0 && val <= 256)
            mp_preferences->m_joy_button_item = val;
    }
    else if (name == "joy_button_shoot") {
        val = string_to_int(value);
        if (val >= 0 && val <= 256)
            mp_preferences->m_joy_button_shoot = val;
    }
    else if (name == "joy_button_action") {
        val = string_to_int(value);
        if (val >= 0 && val <= 256)
            mp_preferences->m_joy_button_action = val;
    }
    else if (name == "joy_button_exit") {
        val = string_to_int(value);
        if (val >= 0 && val <= 256)
            mp_preferences->m_joy_button_exit = val;
    }
    //////////////////// Special ////////////////////
    else if (name == "level_background_images")
        mp_preferences->m_level_background_images = string_to_bool(value);
    else if (name == "image_cache_enabled")
        mp_preferences->m_image_cache_enabled = string_to_bool(value);
    //////////////////// Editor ////////////////////
    else if (name == "editor_mouse_auto_hide")
        mp_preferences->m_editor_mouse_auto_hide = string_to_bool(value);
    else if (name == "editor_show_item_images")
        mp_preferences->m_editor_show_item_images = string_to_bool(value);
    else if (name == "editor_item_image_size")
        mp_preferences->m_editor_item_image_size = string_to_int(value);
    else {
        std::cerr << "Warning: Unknown config option '" << name << "'. Ignoring." << std::endl;
    }
}

/***************************************************************************
 * preferences_loader.hpp - Loading preferences XML
 *
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

#ifndef TSC_PREFERENCES_LOADER_HPP
#define TSC_PREFERENCES_LOADER_HPP
#include "../core/global_game.hpp"
#include "../core/xml_attributes.hpp"
#include "preferences.hpp"

namespace TSC {

    /**
     * XML parser for the preferences file. You should not use
     * this class directly, use cPreferences::Load_From_File() instead.
     */
    class cPreferencesLoader: public xmlpp::SaxParser {
    public:
        cPreferencesLoader();
        virtual ~cPreferencesLoader();

        // Parse the file given by path.
        virtual void parse_file(boost::filesystem::path filename);

        cPreferences* Get_Preferences();
        float Get_Video_Geometry_Detail();
        float Get_Video_Texture_Detail();
        int Get_Audio_Music_Volume();
        int Get_Audio_Sound_Volume();

    protected:
        // SAX parser callbacks
        virtual void on_start_document();
        virtual void on_end_document();
        virtual void on_start_element(const Glib::ustring& name, const xmlpp::SaxParser::AttributeList& properties);
        //virtual void on_end_element(const Glib::ustring& name);

        void handle_property(const std::string& name, const std::string& value);

        // The preferences we’re building
        cPreferences* mp_preferences;
        // The file we’re parsing
        boost::filesystem::path m_preferences_file;
        // The <property> results we found before the current tag
        XmlAttributes m_current_properties;

        // Special preferences
        float m_video_geometry_detail;
        float m_video_texture_detail;
        int m_audio_music_volume;
        int m_audio_sound_volume;
    };

}

#endif

/***************************************************************************
 * img_settings.h
 *
 * Copyright © 2005 - 2011 Florian Richter
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

#ifndef TSC_IMG_SETTINGS_HPP
#define TSC_IMG_SETTINGS_HPP

#include "../core/global_basic.hpp"
#include "../core/file_parser.hpp"
#include "../video/gl_surface.hpp"
#include "../core/math/rect.hpp"

namespace TSC {

    /* *** *** *** *** *** *** cImage_Settings_Data *** *** *** *** *** *** *** *** *** *** *** */

    class cImage_Settings_Data {
    public:
        cImage_Settings_Data(void);
        ~cImage_Settings_Data(void);

        // returns the best surface size for the current resolution
        cSize_Int Get_Surface_Size(const sf::Image* p_sf_image) const;
        // Apply settings to an image
        void Apply(cGL_Surface* image) const;
        // Apply base settings
        void Apply_Base(const cImage_Settings_Data* base_settings_data);

        // base image/settings path
        boost::filesystem::path m_base;
        // inherit base settings
        bool m_base_settings;

        // internal drawing offset
        int m_int_x;
        int m_int_y;
        // collision rect
        GL_rect m_col_rect;
        // width
        int m_width;
        // height
        int m_height;
        // rotation
        int m_rotation_x, m_rotation_y, m_rotation_z;
        // texture mipmapping
        bool m_mipmap;

        // editor tags
        std::string m_editor_tags;
        // name
        std::string m_name;
        // sprite massivity
        MassiveType m_massive_type;
        // ground type
        GroundType m_ground_type;
        // author
        std::string m_author;
        // obsolete
        bool m_obsolete;
    };

    /* *** *** *** *** *** *** cImage_Settings_Parser *** *** *** *** *** *** *** *** *** *** *** */

    class cImage_Settings_Parser : public cFile_parser {
    public:
        cImage_Settings_Parser(void);
        virtual ~cImage_Settings_Parser(void);

        /* Returns the settings from the given file
         * load_base_settings : if set will overwrite settings with all base settings if available
         * The returned settings data should be deleted if not used anymore
        */
        cImage_Settings_Data* Get(const boost::filesystem::path& filename, bool load_base_settings = 1);

        // Handle one tokenized line
        virtual bool HandleMessage(const std::string* parts, unsigned int count, unsigned int line);

        // temp settings used for loading
        cImage_Settings_Data* m_settings_temp;
        // load base settings
        bool m_load_base;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

// Image settings parser
    extern cImage_Settings_Parser* pSettingsParser;

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

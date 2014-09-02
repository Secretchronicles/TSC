/***************************************************************************
 * img_settings.cpp  -  Image settings
 *
 * Copyright © 2005 - 2011 The SMC Contributors
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../video/img_settings.hpp"
#include "../core/game_core.hpp"
#include "../core/math/utilities.hpp"
#include "../core/math/size.hpp"
#include "../core/filesystem/filesystem.hpp"
#include "../core/filesystem/vfs.hpp"
#include "../core/global_basic.hpp"

using namespace std;

namespace fs = boost::filesystem;

namespace SMC {

/* *** *** *** *** *** *** cImage_Settings_Data *** *** *** *** *** *** *** *** *** *** *** */

cImage_Settings_Data :: cImage_Settings_Data(void)
{
    m_base_settings = 0;

    m_int_x = 0;
    m_int_y = 0;
    m_col_rect.clear();
    m_width = 0;
    m_height = 0;
    m_rotation_x = 0;
    m_rotation_y = 0;
    m_rotation_z = 0;
    m_mipmap = 0;

    m_massive_type = MASS_PASSIVE;
    m_ground_type = GROUND_NORMAL;
    m_obsolete = 0;
}

cImage_Settings_Data :: ~cImage_Settings_Data(void)
{

}

cSize_Int cImage_Settings_Data :: Get_Surface_Size(const SDL_Surface* sdl_surface) const
{
    if (!sdl_surface) {
        return cSize_Int();
    }

    // check if texture needs to get downscaled
    float new_w = static_cast<float>(Get_Power_of_2(sdl_surface->w));
    float new_h = static_cast<float>(Get_Power_of_2(sdl_surface->h));

    // if image settings dimension
    if (m_width > 0 && m_height > 0) {
        /* downscale to fit best for the current resolution
         * uses the first dimension which is not smaller as the default scale
        */
        while (new_w * 0.5f > m_width * global_upscalex && new_h * 0.5f > m_height * global_upscaley) {
            new_w *= 0.5f;
            new_h *= 0.5f;
        }

        // if texture detail below low
        if (pVideo->m_texture_quality < 0.25f) {
            // half size only if texture size is high
            if (new_w * 0.8f > m_width * global_upscalex && new_h * 0.8f > m_height * global_upscaley) {
                new_w *= 0.5f;
                new_h *= 0.5f;
            }
        }
    }

    return cSize_Int(new_w, new_h);
}

void cImage_Settings_Data :: Apply(cGL_Surface* image) const
{
    // empty image
    if (!image) {
        cerr << "Error : surface for base " << path_to_utf8(m_base) << " does not exist" << endl;
        return;
    }

    image->m_int_x = static_cast<float>(m_int_x);
    image->m_int_y = static_cast<float>(m_int_y);

    if (m_width > 0) {
        image->m_start_w = static_cast<float>(m_width);
        image->m_w = image->m_start_w;
        image->m_col_w = image->m_w;
    }

    if (m_height > 0) {
        image->m_start_h = static_cast<float>(m_height);
        image->m_h = image->m_start_h;
        image->m_col_h = image->m_h;
    }

    if (m_col_rect.m_w > 0.0f && m_col_rect.m_h > 0.0f) {
        // position
        image->m_col_pos.m_x = m_col_rect.m_x;
        image->m_col_pos.m_y = m_col_rect.m_y;
        // dimension
        image->m_col_w = m_col_rect.m_w;
        image->m_col_h = m_col_rect.m_h;
    }

    if (m_rotation_x != 0) {
        image->m_base_rot_x = static_cast<float>(m_rotation_x);

        //image->m_col_pos = image->m_col_pos.rotate3d( image->base_rotx, 1, 0, 0 );
        // mirror
        if (image->m_base_rot_x == 180.0f) {
            image->m_col_pos.m_y = image->m_h - (image->m_col_h + image->m_col_pos.m_y);
        }
    }

    if (m_rotation_y != 0) {
        image->m_base_rot_y = static_cast<float>(m_rotation_y);

        //image->m_col_pos = image->m_col_pos.rotate3d( image->base_roty, 0, 1, 0 );
        // mirror
        if (image->m_base_rot_y == 180.0f) {
            image->m_col_pos.m_x = image->m_w - (image->m_col_w + image->m_col_pos.m_x);
        }
    }

    if (m_rotation_z != 0) {
        image->m_base_rot_z = static_cast<float>(m_rotation_z);

        //image->m_col_pos = image->m_col_pos.rotate3d( image->m_base_rot_z, 0, 0, 1 );

        if (image->m_base_rot_z == 90.0f) {
            // rotate position
            GL_point pos(image->m_int_x, image->m_int_y);
            pos = pos.rotate(GL_point(image->m_w * 0.5f, image->m_h * 0.5f), image->m_base_rot_z);
            image->m_int_x = pos.m_y;
            image->m_int_y = pos.m_x - image->m_h;
            // rotate collision position
            float orig_x = image->m_col_pos.m_x;
            image->m_col_pos.m_x = image->m_h - (image->m_col_h + image->m_col_pos.m_y);
            image->m_col_pos.m_y = orig_x;

            // switch width and height
            float orig_w = image->m_w;
            image->m_w = image->m_h;
            image->m_h = orig_w;
            // switch collision width and height
            float orig_col_w = image->m_col_w;
            image->m_col_w = image->m_col_h;
            image->m_col_h = orig_col_w;
        }
        // mirror
        else if (image->m_base_rot_z == 180.0f) {
            image->m_col_pos.m_x = image->m_w - (image->m_col_w + image->m_col_pos.m_x);
            image->m_col_pos.m_y = image->m_h - (image->m_col_h + image->m_col_pos.m_y);
        }
        else if (image->m_base_rot_z == 270.0f) {
            // rotate position
            GL_point pos(image->m_int_x, image->m_int_y);
            pos = pos.rotate(GL_point(image->m_w * 0.5f, image->m_h * 0.5f), image->m_base_rot_z);
            image->m_int_x = pos.m_y - image->m_w;
            image->m_int_y = pos.m_x;
            // rotate collision position
            float orig_x = image->m_col_pos.m_x;
            image->m_col_pos.m_x = image->m_col_pos.m_y;
            image->m_col_pos.m_y = orig_x;

            // switch width and height
            float orig_w = image->m_w;
            image->m_w = image->m_h;
            image->m_h = orig_w;
            // switch collision width and height
            float orig_col_w = image->m_col_w;
            image->m_col_w = image->m_col_h;
            image->m_col_h = orig_col_w;
        }
    }

    if (!m_editor_tags.empty()) {
        image->m_editor_tags = m_editor_tags;
    }

    if (!m_name.empty()) {
        image->m_name = m_name;
        // replace "_" with " "
        string_replace_all(image->m_name, "_", " ");
    }

    if (m_massive_type > 0) {
        image->m_massive_type = m_massive_type;
    }

    image->m_ground_type = m_ground_type;
    image->m_obsolete = m_obsolete;
}

void cImage_Settings_Data :: Apply_Base(const cImage_Settings_Data* base_settings_data)
{
    if (!base_settings_data->m_base.empty()) {
        m_base = base_settings_data->m_base;
        m_base_settings = base_settings_data->m_base_settings;
    }

    m_int_x = base_settings_data->m_int_x;
    m_int_y = base_settings_data->m_int_y;
    m_col_rect = base_settings_data->m_col_rect;
    m_width = base_settings_data->m_width;
    m_height = base_settings_data->m_height;
    m_rotation_x = base_settings_data->m_rotation_x;
    m_rotation_y = base_settings_data->m_rotation_y;
    m_rotation_z = base_settings_data->m_rotation_z;
    m_mipmap = base_settings_data->m_mipmap;
    m_editor_tags = base_settings_data->m_editor_tags;
    m_name = base_settings_data->m_name;
    m_massive_type = base_settings_data->m_massive_type;
    m_ground_type = base_settings_data->m_ground_type;
    m_author = base_settings_data->m_author;

    // only set if this isn't obsolete
    if (!m_obsolete && base_settings_data->m_obsolete) {
        m_obsolete = 1;
    }
}

/* *** *** *** *** *** *** cImage_Settings_Parser *** *** *** *** *** *** *** *** *** *** *** */

cImage_Settings_Parser :: cImage_Settings_Parser(void)
    : cFile_parser()
{
    m_settings_temp = NULL;
    m_load_base = 1;
}

cImage_Settings_Parser :: ~cImage_Settings_Parser(void)
{
    //
}

cImage_Settings_Data* cImage_Settings_Parser :: Get(const boost::filesystem::path& filename, bool load_base_settings /* = 1 */)
{
    m_load_base = load_base_settings;
    m_settings_temp = new cImage_Settings_Data();

    Parse(filename);
    cImage_Settings_Data* settings = m_settings_temp;
    m_settings_temp = NULL;
    return settings;
}

bool cImage_Settings_Parser :: HandleMessage(const std::string* parts, unsigned int count, unsigned int line)
{
    if (parts[0].compare("base") == 0) {
        if (count < 2 || count > 3) {
            cerr << path_to_utf8(Trim_Filename(data_file, 0, 0)) << " : line " << line << " Error :" << endl;
            cerr << "Error : " << parts[0] << " needs 2-3 parameters" << endl;
            return 0;
        }

        if (!Is_Valid_Number(parts[2])) {
            cerr << path_to_utf8(Trim_Filename(data_file, 0, 0)) << " : line " << line << " Error : ";
            cerr << parts[1] << " is not a valid integer value" << endl;
            return 0;
        }

        m_settings_temp->m_base = utf8_to_path(parts[1]);
        //m_settings_temp->m_base = data_file.substr( 0, data_file.rfind( "/" ) + 1 ) + parts[1];

        // with settings option
        if (count == 3 && string_to_int(parts[2])) {
            m_settings_temp->m_base_settings = 1;

            if (m_load_base) {
                fs::path settings_file = data_file.parent_path() / m_settings_temp->m_base;

                // if settings file exists
                while (!settings_file.empty()) {
                    // if not already image settings based
                    if (settings_file.extension() != utf8_to_path(".settings"))
                        settings_file.replace_extension(".settings");

                    // not found
                    if (!pVfs->File_Exists(settings_file)) {
                        break;
                    }

                    // create new temporary parser
                    cImage_Settings_Parser* temp_parser = new cImage_Settings_Parser();
                    cImage_Settings_Data* base_settings = temp_parser->Get(settings_file);
                    // finished loading base settings
                    delete temp_parser;
                    settings_file.clear();

                    // handle
                    if (base_settings) {
                        // todo : apply settings in reverse order ( deepest settings should override first )
                        m_settings_temp->Apply_Base(base_settings);

                        // if also based on settings
                        if (!base_settings->m_base.empty() && base_settings->m_base_settings) {
                            settings_file = base_settings->m_base;
                        }

                        delete base_settings;
                    }
                }
            }
        }
    }
    else if (parts[0].compare("int_x") == 0) {
        if (count != 2) {
            cerr << path_to_utf8(Trim_Filename(data_file, 0, 0)) << " : line " << line << " Error :" << endl;
            cerr << "Error : " << parts[0] << " needs 2 parameters" << endl;
            return 0;
        }

        if (!Is_Valid_Number(parts[1])) {
            cerr << path_to_utf8(Trim_Filename(data_file, 0, 0)) << " : line " << line << " Error : ";
            cerr << parts[1] << " is not a valid integer value" << endl;
            return 0;
        }

        m_settings_temp->m_int_x = string_to_int(parts[1]);
    }
    else if (parts[0].compare("int_y") == 0) {
        if (count != 2) {
            cerr << path_to_utf8(Trim_Filename(data_file, 0, 0)) << " : line " << line << " Error :" << endl;
            cerr << "Error : " << parts[0] << " needs 2 parameters"  << endl;
            return 0;
        }

        if (!Is_Valid_Number(parts[1])) {
            cerr << path_to_utf8(Trim_Filename(data_file, 0, 0)) << " : line " << line << " Error : ";
            cerr << parts[1] << " is not a valid integer value" << endl;
            return 0;
        }

        m_settings_temp->m_int_y = string_to_int(parts[1]);
    }
    else if (parts[0].compare("col_rect") == 0) {
        if (count != 5) {
            cerr << path_to_utf8(Trim_Filename(data_file, 0, 0)) << " : line " << line << " Error :" << endl;
            cerr << "Error : " << parts[0] << " needs 5 parameters" << endl;
            return 0;
        }

        for (unsigned int i = 1; i < 5; i++) {
            if (!Is_Valid_Number(parts[i])) {
                cerr << path_to_utf8(Trim_Filename(data_file, 0, 0)) << " : line " << line << " Error : ";
                cerr << parts[1] << " is not a valid integer value" << endl;
                return 0;
            }
        }

        // position and dimension
        m_settings_temp->m_col_rect = GL_rect(static_cast<float>(string_to_int(parts[1])), static_cast<float>(string_to_int(parts[2])), static_cast<float>(string_to_int(parts[3])), static_cast<float>(string_to_int(parts[4])));
    }
    else if (parts[0].compare("width") == 0) {
        if (count != 2) {
            cerr << path_to_utf8(Trim_Filename(data_file, 0, 0)) << " : line " << line << " Error :" << endl;
            cerr << "Error : " << parts[0] << " needs 2 parameters" << endl;
            return 0;
        }

        if (!Is_Valid_Number(parts[1])) {
            cerr << path_to_utf8(Trim_Filename(data_file, 0, 0)) << " : line " << line << " Error : ";
            cerr << parts[1] << " is not a valid integer value" << endl;
            return 0;
        }

        m_settings_temp->m_width = string_to_int(parts[1]);
    }
    else if (parts[0].compare("height") == 0) {
        if (count != 2) {
            cerr << path_to_utf8(Trim_Filename(data_file, 0, 0)) << " : line " << line << " Error :" << endl;
            cerr << "Error : " << parts[0] << " needs 2 parameters" << endl;
            return 0;
        }

        if (!Is_Valid_Number(parts[1])) {
            cerr << path_to_utf8(Trim_Filename(data_file, 0, 0)) << " : line " << line << " Error : ";
            cerr << parts[1] << " is not a valid integer value" << endl;
            return 0;
        }

        m_settings_temp->m_height = string_to_int(parts[1]);
    }
    else if (parts[0].compare("rotation") == 0) {
        if (count < 2 || count > 5) {
            cerr << path_to_utf8(Trim_Filename(data_file, 0, 0)) << " : line " << line << " Error :" << endl;
            cerr << "Error : " << parts[0] << " needs 2-5 parameters" << endl;
            return 0;
        }

        if (!Is_Valid_Number(parts[1])) {
            cerr << path_to_utf8(Trim_Filename(data_file, 0, 0)) << " : line " << line << " Error : ";
            cerr << parts[1] << " is not a valid integer value" << endl;
            return 0;
        }

        // x
        m_settings_temp->m_rotation_x = string_to_int(parts[1]);

        // y
        if (count > 2) {
            if (!Is_Valid_Number(parts[2])) {
                cerr << path_to_utf8(Trim_Filename(data_file, 0, 0)) << " : line " << line << " Error : ";
                cerr << parts[2] << " is not a valid integer value" << endl;
                return 0; // error
            }

            m_settings_temp->m_rotation_y = string_to_int(parts[2]);
        }
        // z
        if (count > 3) {
            if (!Is_Valid_Number(parts[3])) {
                cerr << path_to_utf8(Trim_Filename(data_file, 0, 0)) << " : line " << line << " Error : ";
                cerr << parts[3] << " is not a valid integer value" << endl;
                return 0;
            }

            m_settings_temp->m_rotation_z = string_to_int(parts[3]);
        }
    }
    else if (parts[0].compare("mipmap") == 0) {
        if (count != 2) {
            cerr << path_to_utf8(Trim_Filename(data_file, 0, 0)) << " : line " << line << " Error :" << endl;
            cerr << "Error : " << parts[0] << " needs 2 parameters" << endl;
            return 0;
        }

        if (!Is_Valid_Number(parts[1])) {
            cerr << path_to_utf8(Trim_Filename(data_file, 0, 0)) << " : line " << line << " Error : ";
            cerr << parts[1] << " is not a valid integer value" << endl;
            return 0;
        }

        // if mipmaps enabled
        if (string_to_int(parts[1])) {
            m_settings_temp->m_mipmap = 1;
        }
    }
    else if (parts[0].compare("editor_tags") == 0) {
        if (count != 2) {
            cerr << path_to_utf8(Trim_Filename(data_file, 0, 0)) << " : line " << line << " Error :" << endl;
            cerr << "Error : " << parts[0] << " needs 2 parameters" << endl;
            return 0;
        }

        m_settings_temp->m_editor_tags = parts[1];
    }
    else if (parts[0].compare("name") == 0) {
        if (count != 2) {
            cerr << path_to_utf8(Trim_Filename(data_file, 0, 0)) << " : line " << line << " Error :" << endl;
            cerr << "Error : " << parts[0] << " needs 2 parameters" << endl;
            return 0;
        }

        m_settings_temp->m_name = parts[1];
    }
    else if (parts[0].compare("type") == 0) {
        if (count != 2) {
            cerr << path_to_utf8(Trim_Filename(data_file, 0, 0)) << " : line " << line << " Error :" << endl;
            cerr << "Error : " << parts[0] << " needs 2 parameters" << endl;
            return 0;
        }

        m_settings_temp->m_massive_type = Get_Massive_Type_Id(parts[1]);
    }
    else if (parts[0].compare("ground_type") == 0) {
        if (count != 2) {
            cerr << path_to_utf8(Trim_Filename(data_file, 0, 0)) << " : line " << line << " Error :" << endl;
            cerr << "Error : " << parts[0] << " needs 2 parameters" << endl;
            return 0;
        }

        m_settings_temp->m_ground_type = Get_Ground_Type_Id(parts[1]);
    }
    else if (parts[0].compare("author") == 0) {
        if (count != 2) {
            cerr << path_to_utf8(Trim_Filename(data_file, 0, 0)) << " : line " << line << " Error :" << endl;
            cerr << "Error : " << parts[0] << " needs 2 parameters" << endl;
            return 0;
        }

        m_settings_temp->m_author = parts[1];
    }
    else if (parts[0].compare("obsolete") == 0) {
        if (count != 2) {
            cerr << path_to_utf8(Trim_Filename(data_file, 0, 0)) << " : line " << line << " Error :" << endl;
            cerr << "Error : " << parts[0] << " needs 2 parameters" << endl;
            return 0;
        }

        if (!Is_Valid_Number(parts[1])) {
            cerr << path_to_utf8(Trim_Filename(data_file, 0, 0)) << " : line " << line << " Error : ";
            cerr << parts[1] << " is not a valid integer value" << endl;
            return 0; // error
        }

        // if tagged obsolete
        if (string_to_int(parts[1])) {
            m_settings_temp->m_obsolete = 1;
        }
    }
    else {
        cerr << path_to_utf8(Trim_Filename(data_file, 0, 0)) << " : line " << line << " Error : ";
        cerr << "Unknown Command : " << parts[0] << endl;
        return 0;
    }

    return 1;
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

cImage_Settings_Parser* pSettingsParser = NULL;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace SMC

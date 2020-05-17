/***************************************************************************
 * level_background.cpp  -  level background image and color handling class
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

#include "../level/level_background.hpp"
#include "../user/preferences.hpp"
#include "../core/game_core.hpp"
#include "../video/gl_surface.hpp"
#include "../core/framerate.hpp"
#include "../core/filesystem/resource_manager.hpp"
#include "../core/filesystem/relative.hpp"
#include "../core/xml_attributes.hpp"

#include "../core/global_basic.hpp"

using namespace std;

namespace fs = boost::filesystem;

namespace TSC {

/* *** *** *** *** *** *** *** cBackground *** *** *** *** *** *** *** *** *** *** */

cBackground::cBackground(cSprite_Manager* sprite_manager)
{
    cBackground::Init();
    cBackground::Set_Sprite_Manager(sprite_manager);
}

cBackground::cBackground(XmlAttributes& attributes, cSprite_Manager* sprite_manager)
{
    cBackground::Init();
    cBackground::Set_Sprite_Manager(sprite_manager);
    cBackground::Load_From_Attributes(attributes);
}

cBackground::~cBackground(void)
{
    //
}

void cBackground::Init(void)
{
    m_sprite_manager = NULL;
    m_type = BG_NONE;

    m_pos_x = 0.0f;
    m_pos_y = 0.0f;
    m_start_pos_x = 0.0f;
    m_start_pos_y = 0.0f;
    m_pos_z = 0.00011f;

    m_color_1 = static_cast<uint8_t>(0);
    m_color_2 = static_cast<uint8_t>(0);

    m_image_1 = NULL;

    m_speed_x = 0.5f;
    m_speed_y = 0.5f;
    m_const_vel_x = 0.0f;
    m_const_vel_y = 0.0f;
}

void cBackground::Load_From_Attributes(XmlAttributes& attributes)
{
    Set_Type(static_cast<BackgroundType>(string_to_int(attributes["type"])));

    if (m_type == BG_GR_HOR || m_type == BG_GR_VER) {
        int r, g, b;

        r = string_to_int(attributes["bg_color_1_red"]);
        g = string_to_int(attributes["bg_color_1_green"]);
        b = string_to_int(attributes["bg_color_1_blue"]);
        Set_Color_1(Color(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b)));

        r = string_to_int(attributes["bg_color_2_red"]);
        g = string_to_int(attributes["bg_color_2_green"]);
        b = string_to_int(attributes["bg_color_2_blue"]);
        Set_Color_2(Color(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b)));
    }
    else if (m_type == BG_IMG_BOTTOM || m_type == BG_IMG_TOP || m_type == BG_IMG_ALL) {
        Set_Start_Pos(string_to_float(attributes["posx"]), string_to_float(attributes["posy"]));
        Set_Pos_Z(string_to_float(attributes["posz"]));

        Set_Image(utf8_to_path(attributes["image"]));
        Set_Scroll_Speed(string_to_float(attributes["speedx"]), string_to_float(attributes["speedy"]));
        Set_Const_Velocity_X(string_to_float(attributes["const_velx"]));
        Set_Const_Velocity_Y(string_to_float(attributes["const_vely"]));
    }
}

void cBackground::Save_To_XML_Node(xmlpp::Element* p_parent)
{
    if (m_type == BG_NONE)
        return;

    // <background>
#ifdef USE_LIBXMLPP3
    xmlpp::Element* p_node = p_parent->add_child_element("background");
    Add_Property(p_node, "type", m_type);
#else
    xmlpp::Element* p_node = p_parent->add_child("background");
    Add_Property(p_node, "type", m_type);
#endif

    // gradient
    if (m_type == BG_GR_HOR || m_type == BG_GR_VER) {
        // background color 1
        Add_Property(p_node, "bg_color_1_red", static_cast<int>(m_color_1.red));
        Add_Property(p_node, "bg_color_1_green", static_cast<int>(m_color_1.green));
        Add_Property(p_node, "bg_color_1_blue", static_cast<int>(m_color_1.blue));
        // background color 2
        Add_Property(p_node, "bg_color_2_red", static_cast<int>(m_color_2.red));
        Add_Property(p_node, "bg_color_2_green", static_cast<int>(m_color_2.green));
        Add_Property(p_node, "bg_color_2_blue", static_cast<int>(m_color_2.blue));
    }
    // image
    else if (m_type == BG_IMG_BOTTOM || m_type == BG_IMG_TOP || m_type == BG_IMG_ALL) {
        // position
        Add_Property(p_node, "posx", m_start_pos_x);
        Add_Property(p_node, "posy", m_start_pos_y);
        Add_Property(p_node, "posz", m_pos_z);

        // image filename
        Add_Property(p_node, "image", path_to_utf8(m_image_1_filename));
        // speed
        Add_Property(p_node, "speedx", m_speed_x);
        Add_Property(p_node, "speedy", m_speed_y);
        // constant velocity
        Add_Property(p_node, "const_velx", m_const_vel_x);
        Add_Property(p_node, "const_vely", m_const_vel_y);
    }
    else
        cerr << "Warning: Detected unknown background type '" << m_type << "' on saving." << endl;
    // </background>
}

void cBackground::Set_Sprite_Manager(cSprite_Manager* sprite_manager)
{
    m_sprite_manager = sprite_manager;
}

void cBackground::Set_Type(const BackgroundType type)
{
    m_type = type;
}

void cBackground::Set_Type(const std::string& type)
{
    if (type.compare("Disabled") == 0) {
        m_type = BG_NONE;
    }
    else if (type.compare("Top") == 0) {
        m_type = BG_IMG_TOP;
    }
    else if (type.compare("Bottom") == 0) {
        m_type = BG_IMG_BOTTOM;
    }
    else if (type.compare("All") == 0) {
        m_type = BG_IMG_ALL;
    }
    else if (type.compare("Gradient Vertical") == 0) {
        m_type = BG_GR_VER;
    }
    else if (type.compare("Gradient Horizontal") == 0) {
        m_type = BG_GR_HOR;
    }
    else {
        cerr << "Warning : Unknown Background type " << type << endl;
    }
}

void cBackground::Set_Color_1(const Color& color)
{
    m_color_1 = color;
}

void cBackground::Set_Color_2(const Color& color)
{
    m_color_2 = color;
}

void cBackground::Set_Image(const fs::path& img_file_1)
{
    m_image_1_filename = img_file_1;

    // empty
    if (m_image_1_filename.empty()) {
        Clear_Images();
        m_image_1 = NULL;
        return;
    }

    // Make the path relative to pixmaps/ if it isn’t yet
    if (m_image_1_filename.is_absolute())
        m_image_1_filename = fs::relative(m_image_1_filename, pResource_Manager->Get_Game_Pixmaps_Directory());

    Clear_Images();
    Add_Image_Set("main", m_image_1_filename);
    Set_Image_Set("main", true);
}

void cBackground::Set_Scroll_Speed(const float x /* = 1.0f */, const float y /* = 1.0f */)
{
    m_speed_x = x;
    m_speed_y = y;
}

void cBackground::Set_Start_Pos(const float x, const float y)
{
    m_start_pos_x = x;
    m_start_pos_y = y;
    // reset current position
    m_pos_x = m_start_pos_x;
    m_pos_y = m_start_pos_y;
}

void cBackground::Set_Pos_Z(const float val)
{
    m_pos_z = val;
}

void cBackground::Set_Const_Velocity_X(const float vel)
{
    m_const_vel_x = vel;
    // reset current position
    m_pos_x = m_start_pos_x;
}

void cBackground::Set_Const_Velocity_Y(const float vel)
{
    m_const_vel_y = vel;
    // reset current position
    m_pos_y = m_start_pos_y;
}

void cBackground::Update(void)
{
    Update_Animation();

    if (!Is_Float_Equal(m_const_vel_x, 0.0f)) {
        m_pos_x += (m_const_vel_x * 2) * pFramerate->m_speed_factor;
    }

    if (!Is_Float_Equal(m_const_vel_y, 0.0f)) {
        m_pos_y += (m_const_vel_y * 2) * pFramerate->m_speed_factor;
    }
}

void cBackground::Draw(void)
{
    // gradient
    if (m_type == BG_GR_VER || m_type == BG_GR_HOR) {
        Draw_Gradient();
    }
    // image
    else if (m_type == BG_IMG_BOTTOM || m_type == BG_IMG_TOP || m_type == BG_IMG_ALL) {
        // if background images are disabled or no image
        if (!pPreferences->m_level_background_images || !m_image_1) {
            return;
        }

        // get position
        float posx_final = m_pos_x - ((pActive_Camera->m_x * 0.2f) * m_speed_x);
        float posy_final = m_pos_y - (((pActive_Camera->m_y + game_res_h) * 0.3f) * m_speed_y);

        if (m_type == BG_IMG_BOTTOM || m_type == BG_IMG_ALL) {
            posy_final += game_res_h - m_image_1->m_h;
        }

        // align start position x
        // to left
        while (posx_final > 0.0f) {
            posx_final -= m_image_1->m_w;
        }
        // to right
        while (posx_final < -m_image_1->m_w) {
            posx_final += m_image_1->m_w;
        }
        // align start position y
        if (m_type == BG_IMG_ALL) {
            // to top
            while (posy_final > 0.0f) {
                posy_final -= m_image_1->m_h;
            }
            // to bottom
            while (posy_final < -m_image_1->m_h) {
                posy_final += m_image_1->m_h;
            }
        }

        // draw until width is filled
        while (posx_final < game_res_w) {
            // draw horizontal
            m_image_1->Blit(posx_final, posy_final, m_pos_z);

            // draw vertical
            if (m_type == BG_IMG_ALL) {
                float posy_temp = posy_final;

                // draw until height is filled
                while (posy_temp < game_res_h - m_image_1->m_h) {
                    // change position first as this position y is already drawn
                    posy_temp += m_image_1->m_h;

                    m_image_1->Blit(posx_final, posy_temp, m_pos_z);
                }
            }

            posx_final += m_image_1->m_w;
        }
    }
}

void cBackground::Draw_Gradient(void)
{
    // no need to draw a gradient if both colors are the same
    if (m_color_1 == m_color_2) {
        pVideo->Draw_Rect(NULL, m_pos_z, &m_color_1);
    }
    else {
        // gradient start color
        Color color_start = m_color_1;
        // get camera bottom position
        const float camera_bottom = pActive_Camera->m_y + game_res_h;

        if (camera_bottom < -0.1f) {
            float power = -camera_bottom / 10000;

            if (power > 1.0f) {
                power = 1.0f;
            }

            color_start.red += static_cast<uint8_t>(static_cast<float>(m_color_2.red - m_color_1.red) * power);
            color_start.green += static_cast<uint8_t>(static_cast<float>(m_color_2.green - m_color_1.green) * power);
            color_start.blue += static_cast<uint8_t>(static_cast<float>(m_color_2.blue - m_color_1.blue) * power);
        }

        if (m_type == BG_GR_VER) {
            pVideo->Draw_Gradient(NULL, m_pos_z, &color_start, &m_color_2, DIR_VERTICAL);
        }
        else if (m_type == BG_GR_HOR) {
            pVideo->Draw_Gradient(NULL, m_pos_z, &color_start, &m_color_2, DIR_HORIZONTAL);
        }
    }
}

std::string cBackground::Get_Type_Name(void) const
{
    return Get_Type_Name(m_type);
}

std::string cBackground::Get_Type_Name(const BackgroundType type)
{
    switch (type) {
    case BG_NONE:
        return "Disabled";
    case BG_IMG_TOP:
        return "Top";
    case BG_IMG_BOTTOM:
        return "Bottom";
    case BG_IMG_ALL:
        return "All";
    case BG_GR_VER:
        return "Gradient Vertical";
    case BG_GR_HOR:
        return "Gradient Horizontal";
    default:
        break;
    }

    return "Unknown";
}

/* *** *** *** *** *** *** cBackground_Manager *** *** *** *** *** *** *** *** *** *** *** */

cBackground_Manager::cBackground_Manager(void)
    : cObject_Manager<cBackground>()
{
    //
}

cBackground_Manager::~cBackground_Manager(void)
{
    cBackground_Manager::Delete_All();
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

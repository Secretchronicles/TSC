/***************************************************************************
 * static.cpp  -  static enemy
 *
 * Copyright © 2007 - 2011 Florian Richter
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

#include "../enemies/static.hpp"
#include "../core/game_core.hpp"
#include "../level/level_player.hpp"
#include "../level/level.hpp"
#include "../gui/hud.hpp"
#include "../video/gl_surface.hpp"
#include "../core/sprite_manager.hpp"
#include "../core/i18n.hpp"
#include "../objects/path.hpp"
#include "../core/filesystem/filesystem.hpp"
#include "../core/filesystem/resource_manager.hpp"
#include "../core/filesystem/package_manager.hpp"
#include "../core/filesystem/boost_relative.hpp"
#include "../core/xml_attributes.hpp"

namespace fs = boost::filesystem;

namespace TSC {

/* *** *** *** *** *** *** cStaticEnemy *** *** *** *** *** *** *** *** *** *** *** */

cStaticEnemy::cStaticEnemy(cSprite_Manager* sprite_manager)
    : cEnemy(sprite_manager), m_path_state(sprite_manager)
{
    cStaticEnemy::Init();
}


cStaticEnemy::cStaticEnemy(XmlAttributes& attributes, cSprite_Manager* sprite_manager)
    : cEnemy(sprite_manager), m_path_state(sprite_manager)
{
    cStaticEnemy::Init();

    // position
    Set_Pos(string_to_float(attributes["posx"]), string_to_float(attributes["posy"]), true);

    // rotation speed
    Set_Rotation_Speed(string_to_float(attributes.fetch("rotation_speed", "-7.5")));

    // image
    m_image_filename = attributes.fetch("image", m_image_filename); // Init sets m_image_filename default
    Clear_Images(true);
    Add_Image_Set("main", utf8_to_path(m_image_filename));
    Set_Image_Set("main", true);

    // path
    Set_Path_Identifier(attributes["path"]);

    // movement speed
    Set_Speed(string_to_float(attributes.fetch("speed", float_to_string(m_speed))));

    // fire resistant
    m_fire_resistant = string_to_bool(attributes.fetch("fire_resistant", bool_to_string(m_fire_resistant)));

    // ice resistance
    m_ice_resistance = string_to_float(attributes.fetch("ice_resistance", float_to_string(m_ice_resistance)));
}

cStaticEnemy::~cStaticEnemy(void)
{
    //
}

void cStaticEnemy::Init(void)
{
    m_type = TYPE_STATIC_ENEMY;
    m_name = "Static Enemy";
    m_pos_z = 0.094f;
    m_can_be_on_ground = 0;
    m_can_be_hit_from_shell = 0;

    Set_Rotation_Speed(0.0f);
    Set_Speed(0.0f);

    m_image_filename = "enemy/static/blocks/spike_1/2_grey.png";
    Clear_Images(true);
    Add_Image_Set("main", utf8_to_path(m_image_filename));
    Set_Image_Set("main", true);
}

void cStaticEnemy::Init_Links(void)
{
    // link to parent path
    m_path_state.Set_Path_Identifier(m_path_state.m_path_identifier);
}

cStaticEnemy* cStaticEnemy::Copy(void) const
{
    cStaticEnemy* static_enemy = new cStaticEnemy(m_sprite_manager);
    static_enemy->Set_Pos(m_start_pos_x, m_start_pos_y, 1);
    static_enemy->Clear_Images(true);
    static_enemy->m_image_filename = m_image_filename;
    static_enemy->Add_Image_Set("main", utf8_to_path(m_image_filename));
    static_enemy->Set_Image_Set("main", true);
    static_enemy->Set_Rotation_Speed(m_rotation_speed);
    static_enemy->Set_Path_Identifier(m_path_state.m_path_identifier);
    static_enemy->Set_Speed(m_speed);
    static_enemy->m_fire_resistant = m_fire_resistant;
    static_enemy->m_ice_resistance = m_ice_resistance;
    return static_enemy;
}

std::string cStaticEnemy::Get_XML_Type_Name()
{
    return "static";
}

xmlpp::Element* cStaticEnemy::Save_To_XML_Node(xmlpp::Element* p_element)
{
    xmlpp::Element* p_node = cEnemy::Save_To_XML_Node(p_element);

    Replace_Property(p_node, "image", m_image_filename);
    Add_Property(p_node, "rotation_speed", m_rotation_speed);
    Add_Property(p_node, "path", m_path_state.m_path_identifier);
    Add_Property(p_node, "speed", m_speed);
    Add_Property(p_node, "fire_resistant", m_fire_resistant); // sic! fire_resistant!
    Add_Property(p_node, "ice_resistance", m_ice_resistance);

    return p_node;
}


void cStaticEnemy::Set_Sprite_Manager(cSprite_Manager* sprite_manager)
{
    cSprite::Set_Sprite_Manager(sprite_manager);
    m_path_state.Set_Sprite_Manager(sprite_manager);
}

void cStaticEnemy::Load_From_Savegame(cSave_Level_Object* save_object)
{
    cEnemy::Load_From_Savegame(save_object);
    m_path_state.Load_From_Savegame(save_object);
}

bool cStaticEnemy::Save_To_Savegame_XML_Node(xmlpp::Element* p_element) const
{
    cEnemy::Save_To_Savegame_XML_Node(p_element);

    m_path_state.Save_To_Savegame_XML_Node(p_element);

    return true;
}

void cStaticEnemy::Set_Rotation_Speed(float speed)
{
    m_rotation_speed = speed;
}

void cStaticEnemy::Set_Path_Identifier(const std::string& path)
{
    m_path_state.Set_Path_Identifier(path);
    Set_Velocity(0.0f, 0.0f);
}

void cStaticEnemy::Set_Speed(float speed)
{
    m_speed = speed;
}

void cStaticEnemy::DownGrade(bool force /* = 0 */)
{
    Set_Dead(1);
    m_massive_type = MASS_PASSIVE;
    m_counter = 0.0f;
    m_velx = 0.0f;
    m_vely = 0.0f;
    Set_Scale_Directions(1, 1, 1, 1);

    // falling death
    Set_Rotation_Z(180.0f);
}

void cStaticEnemy::Update(void)
{
    cEnemy::Update();
    if (!m_freeze_counter) {
        Update_Animation();
    }

    if (!m_valid_update || !Is_In_Range()) {
        return;
    }

    if (m_rotation_speed) {
        // update rotation
        Add_Rotation_Z(m_rotation_speed * pFramerate->m_speed_factor);
    }

    if (m_path_state.m_path) {
        // move along path
        if (m_path_state.Path_Move(m_speed * pFramerate->m_speed_factor) == 0) {
            if (!m_path_state.m_path->m_rewind) {
                // if we can not move further along the path, reverse the direction
                m_path_state.Move_Toggle();
            }
        }

        // get difference
        float diff_x = (m_path_state.m_path->m_start_pos_x + m_path_state.m_pos_x) - m_pos_x;
        float diff_y = (m_path_state.m_path->m_start_pos_y + m_path_state.m_pos_y) - m_pos_y;

        // move to position
        Set_Velocity(diff_x, diff_y);
    }

}

void cStaticEnemy::Draw(cSurface_Request* request /* = NULL */)
{
    if (!m_valid_draw) {
        return;
    }

    // current path state position
    if (game_debug) {
        m_path_state.Draw();
    }

    // enemy
    cEnemy::Draw(request);
}

Col_Valid_Type cStaticEnemy::Validate_Collision(cSprite* obj)
{
    if (obj->m_massive_type == MASS_MASSIVE) {
        switch (obj->m_type) {
        case TYPE_ROKKO: {
            return COL_VTYPE_NOT_VALID;
        }
        case TYPE_GEE: {
            return COL_VTYPE_NOT_VALID;
        }
        case TYPE_TURTLE_BOSS: {
            return COL_VTYPE_NOT_VALID;
        }
        case TYPE_FURBALL_BOSS: {
            return COL_VTYPE_NOT_VALID;
        }
        case TYPE_SPIKEBALL: {
            return COL_VTYPE_NOT_VALID;
        }
        case TYPE_PLAYER: {
            return COL_VTYPE_INTERNAL;
        }
        default: {
            break;
        }
        }

        if (obj->m_sprite_array == ARRAY_ENEMY) {
            return COL_VTYPE_INTERNAL;
        }

        return COL_VTYPE_NOT_VALID;
    }

    return COL_VTYPE_NOT_VALID;
}

void cStaticEnemy::Handle_Collision_Player(cObjectCollision* collision)
{
    pLevel_Player->DownGrade_Player();
}

void cStaticEnemy::Handle_Collision_Enemy(cObjectCollision* collision)
{
    // invalid
    if (collision->m_number < 0) {
        return;
    }

    cEnemy* enemy = static_cast<cEnemy*>(m_sprite_manager->Get_Pointer(collision->m_number));

    // already dead
    if (enemy->m_dead) {
        return;
    }

    // kill enemy
    pAudio->Play_Sound(enemy->m_kill_sound);
    pHud_Points->Add_Points(enemy->m_kill_points, m_pos_x, m_pos_y - 5.0f, "", static_cast<Uint8>(255), 1);
    enemy->DownGrade(1);
}

void cStaticEnemy::Editor_Activate(void)
{
    // get window manager
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    // image
    CEGUI::Editbox* editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "editor_static_enemy_image"));
    Editor_Add(UTF8_("Image"), UTF8_("Image filename"), editbox, 200);

    editbox->setText(m_image_filename.c_str());
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cStaticEnemy::Editor_Image_Text_Changed, this));

    // rotation speed
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "editor_static_enemy_rotation_speed"));
    Editor_Add(UTF8_("Rotation Speed"), UTF8_("Rotation Speed"), editbox, 120);

    editbox->setValidationString("[-+]?[0-9]*\\.?[0-9]*");
    editbox->setText(float_to_string(m_rotation_speed, 6, 0));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cStaticEnemy::Editor_Rotation_Speed_Text_Changed, this));

    // path
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "editor_static_enemy_path_identifier"));
    Editor_Add(UTF8_("Path Identifier"), UTF8_("Name of the Path to move along."), editbox, 120);

    editbox->setText(m_path_state.m_path_identifier.c_str());
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cStaticEnemy::Editor_Path_Identifier_Text_Changed, this));

    // speed
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "editor_static_enemy_speed"));
    Editor_Add(UTF8_("Speed"), UTF8_("Maximum speed"), editbox, 120);

    editbox->setValidationString("[-+]?[0-9]*\\.?[0-9]*");
    editbox->setText(float_to_string(m_speed, 6, 0));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cStaticEnemy::Editor_Speed_Text_Changed, this));

    // fire resistant
    CEGUI::Combobox* combobox = static_cast<CEGUI::Combobox*>(wmgr.createWindow("TaharezLook/Combobox", "editor_static_enemy_fire_resistant"));
    Editor_Add(UTF8_("Fire Resistant"), UTF8_("If it is resistant against fire"), combobox, 120, 80);

    combobox->addItem(new CEGUI::ListboxTextItem(UTF8_("Enabled")));
    combobox->addItem(new CEGUI::ListboxTextItem(UTF8_("Disabled")));

    if (m_fire_resistant) {
        combobox->setText(UTF8_("Enabled"));
    }
    else {
        combobox->setText(UTF8_("Disabled"));
    }

    combobox->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cStaticEnemy::Editor_Fire_Resistant_Select, this));

    // ice resistance
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "editor_static_enemy_ice_resistance"));
    Editor_Add(UTF8_("Ice Resistance"), UTF8_("Resistance against Ice (0.0-1.0)"), editbox, 120);

    editbox->setValidationString("[+]?[0-9]*\\.?[0-9]*");
    editbox->setText(float_to_string(m_ice_resistance, 6, 0));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cStaticEnemy::Editor_Ice_Resistance_Text_Changed, this));

    // init
    Editor_Init();
}

bool cStaticEnemy::Editor_Image_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Clear_Images(true);
    m_image_filename = str_text;
    Add_Image_Set("main", utf8_to_path(m_image_filename));
    Set_Image_Set("main", true);

    return 1;
}

bool cStaticEnemy::Editor_Rotation_Speed_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Rotation_Speed(string_to_float(str_text));

    return 1;
}

bool cStaticEnemy::Editor_Path_Identifier_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Path_Identifier(str_text);

    return 1;
}

bool cStaticEnemy::Editor_Speed_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Speed(string_to_float(str_text));

    return 1;
}

bool cStaticEnemy::Editor_Fire_Resistant_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    if (item->getText().compare(UTF8_("Enabled")) == 0) {
        m_fire_resistant = 1;
    }
    else {
        m_fire_resistant = 0;
    }

    return 1;
}

bool cStaticEnemy::Editor_Ice_Resistance_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    m_ice_resistance = string_to_float(str_text);

    if (m_ice_resistance > 1.0f) {
        m_ice_resistance = 1.0f;
    }

    return 1;
}

std::string cStaticEnemy::Create_Name(void) const
{
    std::string name = m_name; // dup

    if (m_start_image && !m_start_image->m_name.empty()) {
        name += " " + m_start_image->m_name;
    }

    return name;
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

/***************************************************************************
 * beetle.cpp - Small enemy that flies around and dies.
 *
 * Copyright © 2014 The TSC Contributors
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "beetle.hpp"
#include "../core/errors.hpp"
#include "../core/xml_attributes.hpp"
#include "../core/game_core.hpp"
#include "../core/math/circle.hpp"
#include "../core/global_game.hpp"
#include "../core/i18n.hpp"
#include "../level/level_player.hpp"
#include "../gui/hud.hpp"

using namespace TSC;

cBeetle::cBeetle(cSprite_Manager* p_sprite_manager)
    : cEnemy(p_sprite_manager)
{
    Init();
}

cBeetle::cBeetle(XmlAttributes& attributes, cSprite_Manager* p_sprite_manager)
    : cEnemy(p_sprite_manager)
{
    Init();

    // position
    Set_Pos(attributes.fetch<float>("posx", 0), attributes.fetch<float>("posy", 0), true);
    // color
    Set_Color(Get_Color_Id(attributes.fetch<std::string>("color", "blue")));
    // direction
    Set_Direction(Get_Direction_Id(attributes.fetch<std::string>("direction", "left")), true);

    Update_Rotation_Hor();
}

cBeetle::~cBeetle()
{
    //
}

void cBeetle::Init()
{
    m_type = TYPE_BEETLE;
    m_pos_z = 0.092f; // Ensure this is behind cBeetleBarrage
    m_gravity_max = 0.0f;
    m_editor_pos_z = 0.089f;
    m_name = "Beetle";
    m_velx = -2.5;
    m_rest_living_time = Get_Random_Float(150.0f, 250.0f);
    m_start_direction = m_direction = DIR_LEFT;
    m_generation_max_y = 0.0f;
    m_generation_in_progress = false;

    // Select random color
    DefaultColor ary[] = {COL_RED, COL_YELLOW, COL_GREEN, COL_BLUE, COL_VIOLET};
    Set_Color(ary[rand() % 5]);

    m_state = STA_FLY;
    Set_Direction(DIR_LEFT);

    // TODO: Own die sound
    m_kill_sound = "enemy/gee/die.ogg";
    m_kill_points = 100;
}

cBeetle* cBeetle::Copy() const
{
    cBeetle* p_beetle = new cBeetle(m_sprite_manager);
    p_beetle->Set_Pos(m_start_pos_x, m_start_pos_y);
    p_beetle->Set_Direction(m_start_direction);
    p_beetle->Set_Color(m_color);
    return p_beetle;
}

std::string cBeetle::Get_XML_Type_Name()
{
    return "beetle";
}

xmlpp::Element* cBeetle::Save_To_XML_Node(xmlpp::Element* p_element)
{
    xmlpp::Element* p_node = cEnemy::Save_To_XML_Node(p_element);

    Add_Property(p_node, "direction", Get_Direction_Name(m_start_direction));
    Add_Property(p_node, "color", Get_Color_Name(m_color));

    return p_node;
}

void cBeetle::DownGrade(bool force /* = false */)
{
    if (force) // Falling/lava death
        Set_Rotation_Z(180.0f);

    Set_Dead(true);
    m_massive_type = MASS_PASSIVE;
    m_velx = 0.0f;
    m_vely = 0.0f;
}

void cBeetle::Update()
{
    cEnemy::Update();

    if (!m_valid_update || !Is_In_Range())
        return;

    // This will be true if the beetle is generated from a beetle barrage
    if (m_generation_in_progress) {
        // Let our beetle fly upwards until the requested position is reached.
        if (m_pos_y <= m_generation_max_y) {
            m_generation_in_progress = false;
            m_generation_max_y = 0.0f;
            m_vely = 0.0f;
            m_pos_z = 0.094f; // Ensure this is now in front of cBeetleBarrage -- looks weird if they fly behind the plant
        }
    }
    else {
        // When living time is up, die.
        m_rest_living_time -= pFramerate->m_speed_factor;
        if (m_rest_living_time <= 0) {
            m_rest_living_time = 0;
            DownGrade(true);
        }

        // Make it go into random directions on random occasions
        if (rand() % 10 > 6) {
            m_velx = Get_Random_Float(-10.0f, 10.0f);
            m_vely = Get_Random_Float(-10.0f, 10.0f);

            // Can’t use Set_Direction, because we set m_velx ourselves.
            // Therefore we also need to call Update_Rotation_Hor() ourselves.
            if (m_velx < 0)
                m_direction = DIR_LEFT;
            else
                m_direction = DIR_RIGHT;

            Update_Rotation_Hor();
        }
    }

    // This enemy is immune to both gravity and air resistance.
    // We don’t update the velocity or gravity therefore.
    Update_Animation();
}

void cBeetle::Draw(cSurface_Request* p_request /* = NULL */)
{
    if (!m_valid_draw)
        return;

    cEnemy::Draw(p_request);
}

Col_Valid_Type cBeetle::Validate_Collision(cSprite* p_obj)
{
    if (p_obj->m_massive_type == MASS_MASSIVE) {
        switch (p_obj->m_type) {
        case TYPE_PLAYER:
        case TYPE_BALL: // fallthru
            return COL_VTYPE_INTERNAL;
        default:
            break;
        }
        return COL_VTYPE_NOT_VALID;
    }
    else
        return COL_VTYPE_NOT_VALID;
}

void cBeetle::Handle_Collision_Player(cObjectCollision* p_collision)
{
    // invalid
    if (p_collision->m_direction == DIR_UNDEFINED)
        return;

    // We will die only when hit from the top
    if (p_collision->m_direction == DIR_TOP && pLevel_Player->m_state != STA_FLY) {
        pAudio->Play_Sound(m_kill_sound);
        DownGrade();
        pLevel_Player->Action_Jump(true);
        pHud_Points->Add_Points(m_kill_points, m_pos_x, m_pos_y - 5.0f, "", static_cast<Uint8>(255), 1);
        pLevel_Player->Add_Kill_Multiplier();
    }
    else if (!pLevel_Player->m_invincible) {
        if (p_collision->m_direction == DIR_LEFT) {
            pLevel_Player->m_velx = -7.0f;
            Turn_Around();
        }
        else if (p_collision->m_direction == DIR_RIGHT) {
            pLevel_Player->m_velx = 7.0f;
            Turn_Around();
        }

        pLevel_Player->DownGrade_Player();
    }
}

void cBeetle::Handle_Collision_Massive(cObjectCollision* p_collision)
{
    Send_Collision(p_collision);
}

float cBeetle::Get_Rest_Living_Time()
{
    return m_rest_living_time;
}

void cBeetle::Set_Rest_Living_Time(float time)
{
    m_rest_living_time = time;
}

void cBeetle::Set_Color(DefaultColor color)
{
    Clear_Images();
    Add_Image_Set("fly", utf8_to_path("enemy/beetle/" + Get_Color_Name(color) + "/fly.imgset"));
    Set_Image_Set("fly", true);

    m_color = color;
}

DefaultColor cBeetle::Get_Color()
{
    return m_color;
}

void cBeetle::Editor_Activate()
{
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    // direction
    CEGUI::Combobox* p_combo = static_cast<CEGUI::Combobox*>(wmgr.createWindow("TaharezLook/Combobox", "editor_beetle_direction"));
    Editor_Add(UTF8_("Direction"), UTF8_("Starting direction."), p_combo, 100, 75);
    p_combo->addItem(new CEGUI::ListboxTextItem("left"));
    p_combo->addItem(new CEGUI::ListboxTextItem("right"));
    p_combo->setText(Get_Direction_Name(m_start_direction));
    p_combo->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cBeetle::Editor_Direction_Select, this));

    // color
    p_combo = static_cast<CEGUI::Combobox*>(wmgr.createWindow("TaharezLook/Combobox", "editor_beetle_color"));
    Editor_Add(UTF8_("Color"), UTF8_("Color."), p_combo, 100, 75);
    p_combo->addItem(new CEGUI::ListboxTextItem("blue"));
    p_combo->addItem(new CEGUI::ListboxTextItem("green"));
    p_combo->addItem(new CEGUI::ListboxTextItem("red"));
    p_combo->addItem(new CEGUI::ListboxTextItem("violet"));
    p_combo->addItem(new CEGUI::ListboxTextItem("yellow"));
    p_combo->setText(Get_Color_Name(m_color));
    p_combo->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cBeetle::Editor_Color_Select, this));

    Editor_Init();
}

bool cBeetle::Editor_Direction_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& args = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* p_item = static_cast<CEGUI::Combobox*>(args.window)->getSelectedItem();
    Set_Direction(Get_Direction_Id(p_item->getText().c_str()), true);

    Update_Rotation_Hor();
    return true;
}

bool cBeetle::Editor_Color_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& args = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* p_item = static_cast<CEGUI::Combobox*>(args.window)->getSelectedItem();
    Set_Color(Get_Color_Id(p_item->getText().c_str()));

    return true;
}

bool cBeetle::Is_Doing_Beetle_Barrage_Generation()
{
    return m_generation_in_progress;
}

void cBeetle::Do_Beetle_Barrage_Generation(float distance)
{
    m_generation_max_y = m_pos_y - distance;
    m_generation_in_progress = true;
    // Slowly move up
    m_vely = -2.0f;
    m_velx = 0.0f;
}

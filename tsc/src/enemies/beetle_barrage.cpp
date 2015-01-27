/***************************************************************************
 * beetle_barrage.cpp - It spits out beetles!
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

#include "beetle_barrage.hpp"
#include "../core/errors.hpp"
#include "../core/xml_attributes.hpp"
#include "../core/game_core.hpp"
#include "../core/math/circle.hpp"
#include "../core/i18n.hpp"
#include "../core/sprite_manager.hpp"
#include "../level/level_player.hpp"
#include "../level/level.hpp"
#include "beetle.hpp"
#include "../scripting/events/spit_event.hpp"

using namespace TSC;

cBeetleBarrage::cBeetleBarrage(cSprite_Manager* p_sprite_manager)
    : cEnemy(p_sprite_manager)
{
    Init();
}

cBeetleBarrage::cBeetleBarrage(XmlAttributes& attributes, cSprite_Manager* p_sprite_manager)
    : cEnemy(p_sprite_manager)
{
    Init();

    // position
    Set_Pos(attributes.fetch<float>("posx", 0), attributes.fetch<float>("posy", 0), true);

    Set_Active_Range(attributes.fetch<float>("active_range", 0.0f));
    Set_Beetle_Interval(attributes.fetch<float>("interval", 100.0f));
    Set_Beetle_Spit_Count(attributes.fetch<int>("spit_count", 1));
    Set_Beetle_Fly_Distance(attributes.fetch<float>("fly_distance", 0.0f));
}

cBeetleBarrage::~cBeetleBarrage()
{
    //
}

void cBeetleBarrage::Init()
{
    m_type = TYPE_BEETLE_BARRAGE;
    m_pos_z = 0.093f; // Ensure this is in front of cBeetle
    m_gravity_max = 24.0f;
    m_editor_pos_z = 0.089f;
    m_name = "Beetle Barrage";
    m_beetle_interval = 100.0f;
    m_beetle_interval_counter = 0.0f;
    m_is_spitting_out_beetles = false;
    m_beetle_fly_distance = 35.0f;
    m_beetle_spit_count = 5;

    Add_Image_Set("still", utf8_to_path("enemy/beetle_barrage/still.imgset"));
    Add_Image_Set("active", utf8_to_path("enemy/beetle_barrage/active.imgset"));
    Set_Image_Set("still");

    Set_Active_Range(200);
    Set_Moving_State(STA_STAY);
    Set_Direction(DIR_UP);

    // TODO: Own die sound
    m_kill_sound = "enemy/eato/die.ogg";
    m_kill_points = 100;
}

cBeetleBarrage* cBeetleBarrage::Copy() const
{
    cBeetleBarrage* p_bb = new cBeetleBarrage(m_sprite_manager);
    p_bb->Set_Pos(m_start_pos_x, m_start_pos_y);
    p_bb->Set_Direction(m_start_direction);
    return p_bb;
}

std::string cBeetleBarrage::Get_XML_Type_Name()
{
    return "beetle_barrage";
}

xmlpp::Element* cBeetleBarrage::Save_To_XML_Node(xmlpp::Element* p_element)
{
    xmlpp::Element* p_node = cEnemy::Save_To_XML_Node(p_element);

    Add_Property(p_node, "direction", Get_Direction_Name(m_start_direction));
    Add_Property(p_node, "active_range", m_active_range);
    Add_Property(p_node, "interval", m_beetle_interval);
    Add_Property(p_node, "spit_count", m_beetle_spit_count);
    Add_Property(p_node, "fly_distance", m_beetle_fly_distance);

    return p_node;
}

void cBeetleBarrage::DownGrade(bool force /* = false */)
{
    if (force) // Falling/lava death
        Set_Rotation_Z(180.0f);

    Set_Dead(true);
    m_massive_type = MASS_PASSIVE;
    m_velx = 0.0f;
    m_vely = 0.0f;
}

void cBeetleBarrage::Update()
{
    cEnemy::Update();

    if (!m_valid_update || !Is_In_Range())
        return;

    // Check if the player is in our range
    Calculate_Active_Area(m_pos_x, m_pos_y);
    if (m_active_area.Intersects(pLevel_Player->m_col_rect)) {
        // Step through the action interval
        m_beetle_interval_counter += pFramerate->m_speed_factor;

        // When the counter reaches the target, spit out beetles.
        if (m_beetle_interval_counter >= m_beetle_interval) {
            m_beetle_interval_counter = 0.0f;
            m_spitting_beetles_counter = 20.0f;

            m_is_spitting_out_beetles = true;
            Set_Image_Set("active");
        }
    }

    if (m_is_spitting_out_beetles) {
        if (m_spitting_beetles_counter <= 0.0f) {
            m_spitting_beetles_counter = 0.0f;
            Set_Image_Set("still");

            Generate_Beetles();

            m_is_spitting_out_beetles = false;
        }
        else
            m_spitting_beetles_counter -= pFramerate->m_speed_factor;
    }

    Update_Velocity();
    Update_Animation();
}

void cBeetleBarrage::Draw(cSurface_Request* p_request /* = NULL */)
{
    if (!m_valid_draw)
        return;

    if (editor_level_enabled) {
        // Draw the action area
        Calculate_Active_Area(m_start_pos_x, m_start_pos_y);
        pVideo->Draw_Circle(m_active_area.Get_X() - pActive_Camera->m_x,
                            m_active_area.Get_Y() - pActive_Camera->m_y,
                            m_active_area.Get_Radius(),
                            m_editor_pos_z - 0.000001f,
                            &whitealpha128);


        // Draw the distance the beetles fly before distributing
        cBeetle temp_beetle(m_sprite_manager); // Only for rect width!
        float x, y;
        Calculate_Fly_Start(&temp_beetle, x, y);
        pVideo->Draw_Rect(x + temp_beetle.m_rect.m_w / 2.0 - 5.0f - pActive_Camera->m_x,
                          y + temp_beetle.m_rect.m_h - pActive_Camera->m_y,
                          10.0f,
                          -m_beetle_fly_distance,
                          m_editor_pos_z - 0.000002f,
                          &whitealpha128);
    }

    cEnemy::Draw(p_request);
}

Col_Valid_Type cBeetleBarrage::Validate_Collision(cSprite* p_obj)
{
    Col_Valid_Type basic_valid = Validate_Collision_Ghost(p_obj);

    // found valid collision
    if (basic_valid != COL_VTYPE_NOT_POSSIBLE)
        return basic_valid;

    if (p_obj->m_massive_type == MASS_MASSIVE) {
        switch (p_obj->m_type) {
        case TYPE_FLYON:
        case TYPE_ROKKO: // fallthru
        case TYPE_GEE:   // fallthru
            return COL_VTYPE_NOT_VALID;
        default:
            return COL_VTYPE_BLOCKING;
        }
    }
    else if (p_obj->m_massive_type == MASS_HALFMASSIVE) {
        // If moving downwards and the object is on bottom
        if (m_vely >= 0.0f && Is_On_Top(p_obj))
            return COL_VTYPE_BLOCKING;
    }
    else if (p_obj->m_massive_type == MASS_PASSIVE) {
        if (p_obj->m_type == TYPE_ENEMY_STOPPER)
            return COL_VTYPE_BLOCKING;
    }

    return COL_VTYPE_NOT_VALID;
}

void cBeetleBarrage::Handle_Collision_Player(cObjectCollision* p_collision)
{
    // invalid
    if (p_collision->m_direction == DIR_UNDEFINED)
        return;

    // only if not invincable
    if (pLevel_Player->m_invincible <= 0.0f) {
        // If player is big and does not come from the bottom
        if (pLevel_Player->m_alex_type != ALEX_SMALL && p_collision->m_direction != DIR_BOTTOM)
            pLevel_Player->Action_Jump(true);

        pLevel_Player->DownGrade_Player();
    }
}

void cBeetleBarrage::Handle_Collision_Enemy(cObjectCollision* p_collision)
{
    Send_Collision(p_collision);
}

void cBeetleBarrage::Handle_Collision_Massive(cObjectCollision* p_collision)
{
    if (m_state == STA_OBJ_LINKED)
        return;

    // TODO
}

void cBeetleBarrage::Set_Moving_State(Moving_state new_state)
{
    if (new_state == m_state)
        return;

    if (new_state == STA_STAY)
        Set_Animation(false);
    else if (new_state == STA_RUN)
        Set_Animation(true);
    else if (new_state == STA_FALL) {
        /* Ignore */
    }
    else
        throw (InvalidMovingStateError(new_state));

    Reset_Animation();
    Set_Image_Num(0);
    m_state = new_state;
}

void cBeetleBarrage::Set_Active_Range(float range)
{
    m_active_range = range;
}

float cBeetleBarrage::Get_Active_Range()
{
    return m_active_range;
}

void cBeetleBarrage::Calculate_Active_Area(const float& x, const float& y)
{
    m_active_area.Set_X(x + m_rect.m_w / 2.0);
    m_active_area.Set_Y(y + m_rect.m_h / 2.0);
    m_active_area.Set_Radius(m_active_range);
}

void cBeetleBarrage::Set_Beetle_Interval(float time)
{
    m_beetle_interval = time;
}

float cBeetleBarrage::Get_Beetle_Interval()
{
    return m_beetle_interval;
}

void cBeetleBarrage::Set_Beetle_Spit_Count(int count)
{
    m_beetle_spit_count = count;
}

int cBeetleBarrage::Get_Beetle_Spit_Count()
{
    return m_beetle_spit_count;
}

void cBeetleBarrage::Generate_Beetles()
{
    Scripting::cSpit_Event evt;
    evt.Fire(pActive_Level->m_mruby, this);

    for (int i=0; i < m_beetle_spit_count; i++) {
        cBeetle* p_beetle = new cBeetle(m_sprite_manager);
        float x, y;
        Calculate_Fly_Start(p_beetle, x, y);

        p_beetle->Set_Spawned(true);
        p_beetle->Set_Pos(x, y, true);
        p_beetle->Do_Beetle_Barrage_Generation(m_beetle_fly_distance);
        p_beetle->Set_Active(true);

        pActive_Level->m_sprite_manager->Add(p_beetle);
    }
}

void cBeetleBarrage::Set_Beetle_Fly_Distance(float distance)
{
    m_beetle_fly_distance = distance;
}

float cBeetleBarrage::Get_Beetle_Fly_Distance()
{
    return m_beetle_fly_distance;
}

void cBeetleBarrage::Calculate_Fly_Start(const cBeetle* p_beetle, float& x, float& y)
{
    x = m_pos_x + m_rect.m_w / 2.0f - p_beetle->m_rect.m_w / 2.0f;
    y = m_pos_y + m_rect.m_h / 2.0f - p_beetle->m_rect.m_h / 2.0f - 5.0f;
}

void cBeetleBarrage::Editor_Activate()
{
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    // range
    CEGUI::Editbox* p_editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "editor_beetlebarrage_fly_distance"));
    Editor_Add(UTF8_("Fly distance"), _("Initial beetle flying distance"), p_editbox, 90);
    p_editbox->setValidationString("^\\d+$");
    p_editbox->setText(int_to_string(static_cast<int>(m_beetle_fly_distance)));
    p_editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cBeetleBarrage::Editor_Fly_Distance_Text_Changed, this));

    // action area
    p_editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "editor_beetlebarrage_range"));
    Editor_Add(UTF8_("Action range"), _("Radius in which it reacts on Alex"), p_editbox, 90);
    p_editbox->setValidationString("^\\d+$");
    p_editbox->setText(int_to_string(static_cast<int>(m_active_range)));
    p_editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cBeetleBarrage::Editor_Range_Text_Changed, this));

    p_editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "editor_beetlebarrage_count"));
    Editor_Add(UTF8_("Count"), _("Number of beetles spit out at a time"), p_editbox, 90);
    p_editbox->setValidationString("^\\d+$");
    p_editbox->setText(int_to_string(m_beetle_spit_count));
    p_editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cBeetleBarrage::Editor_Spit_Count_Text_Changed, this));

    Editor_Init();
}

bool cBeetleBarrage::Editor_Fly_Distance_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& args = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(args.window)->getText().c_str();

    Set_Beetle_Fly_Distance(string_to_float(str_text));
    return true;
}


bool cBeetleBarrage::Editor_Range_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& args = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(args.window)->getText().c_str();

    Set_Active_Range(string_to_float(str_text));
    return true;
}

bool cBeetleBarrage::Editor_Spit_Count_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& args = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(args.window)->getText().c_str();

    Set_Beetle_Spit_Count(string_to_int(str_text));
    return true;
}

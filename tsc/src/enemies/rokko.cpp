/***************************************************************************
 * rokko.cpp  -  giant, slow-moving bullet
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

#include "../enemies/rokko.hpp"
#include "../core/game_core.hpp"
#include "../level/level_player.hpp"
#include "../video/animation.hpp"
#include "../gui/hud.hpp"
#include "../video/gl_surface.hpp"
#include "../video/renderer.hpp"
#include "../input/mouse.hpp"
#include "../core/i18n.hpp"
#include "../core/xml_attributes.hpp"
#include "../core/global_basic.hpp"
#include "../core/sprite_manager.hpp"
#include "../level/level.hpp"
#include "../level/level_settings.hpp"
#include "../core/editor/editor.hpp"
#include "../level/level_editor.hpp"

using namespace std;

namespace TSC {

/* *** *** *** *** *** *** cRokko *** *** *** *** *** *** *** *** *** *** *** */

const float cRokko::m_corrected_pos_z_delta = 10 * cSprite::m_pos_z_delta;

cRokko::cRokko(cSprite_Manager* sprite_manager)
    : cEnemy(sprite_manager)
{
    cRokko::Init();
}

cRokko::cRokko(XmlAttributes& attributes, cSprite_Manager* sprite_manager)
    : cEnemy(sprite_manager)
{
    cRokko::Init();

    // position
    Set_Pos(string_to_float(attributes["posx"]), string_to_float(attributes["posy"]), true);

    // direction
    Set_Direction(Get_Direction_Id(attributes.fetch("direction", Get_Direction_Name(m_start_direction))));

    // speed
    Set_Speed(string_to_float(attributes.fetch("speed", float_to_string(m_speed))));
}


cRokko::~cRokko(void)
{
    //
}

void cRokko::Init(void)
{
    m_type = TYPE_ROKKO;
    m_name = "Rokko";
    m_massive_type = MASS_PASSIVE;
    m_pos_z = 0.03f;
    m_gravity_max = 0;
    m_editor_pos_z = 0.09f;
    m_can_be_on_ground = 0;
    m_camera_range = 4000;
    Set_Rotation_Affects_Rect(1);
    Set_Active(0);

    m_fire_resistant = 1;
    m_ice_resistance = 1;
    m_can_be_hit_from_shell = 0;

    Set_Direction(DIR_LEFT);
    Set_Speed(8.5f);
    m_min_distance_front = 200;
    Set_Max_Distance_Front(1000);
    Set_Max_Distance_Sides(400);
    m_state = STA_STAY;
    m_manual = false;

    m_smoke_counter = 0;

    m_kill_sound = "enemy/rokko/hit.wav";
    m_kill_points = 250;

    Add_Image_Set("fly", "enemy/rokko/yellow/fly.imgset");
    Add_Image_Set("break", "enemy/rokko/yellow/break.imgset", 0, NULL, &m_break_end);

    Set_Image_Set("fly", true);
}

cRokko* cRokko::Copy(void) const
{
    cRokko* rokko = new cRokko(m_sprite_manager);
    rokko->Set_Pos(m_start_pos_x, m_start_pos_y);
    rokko->Set_Direction(m_start_direction);
    rokko->Set_Speed(m_speed);
    return rokko;
}

std::string cRokko::Get_XML_Type_Name()
{
    return "rokko";
}

xmlpp::Element* cRokko::Save_To_XML_Node(xmlpp::Element* p_element)
{
    xmlpp::Element* p_node = cEnemy::Save_To_XML_Node(p_element);

    Add_Property(p_node, "direction", Get_Direction_Name(m_start_direction));
    Add_Property(p_node, "speed", m_speed);

    return p_node;
}


void cRokko::Load_From_Savegame(cSave_Level_Object* save_object)
{
    cEnemy::Load_From_Savegame(save_object);

    // Don't activate if dead
    if (m_dead) {
        return;
    }

    // activate
    if (m_state == STA_FLY) {
        Activate(0);
    }
}

void cRokko::Set_Direction(const ObjectDirection dir, bool new_start_direction /* = true */)
{
    // already set
    if (m_start_direction == dir) {
        return;
    }

    // clear old images
    //Clear_Images();

    cEnemy::Set_Direction(dir, new_start_direction);

    if (m_direction == DIR_LEFT) {
        Set_Rotation(0.0f, 0.0f, 0.0f, 1);
    }
    else if (m_direction == DIR_RIGHT) {
        Set_Rotation(0.0f, 180.0f, 0.0f, 1);
    }
    else if (m_direction == DIR_UP) {
        Set_Rotation(0.0f, 0.0f, 90.0f, 1);
    }
    else if (m_direction == DIR_DOWN) {
        Set_Rotation(0.0f, 0.0f, 270.0f, 1);
    }
    else {
        cerr << "Warning: Unknown Rokko direction " << Get_Direction_Name(dir) << endl;
    }

    Update_Rotation_Hor();
    Update_Distance_rect();
    //Set_Image_Set("fly", true);
}

void cRokko::Set_Speed(float nspeed)
{
    if (nspeed < 2.0f) {
        nspeed = 2.0f;
    }

    m_speed = nspeed;
}

void cRokko::Set_Max_Distance_Front(float distance)
{
    if (distance < m_min_distance_front) {
        distance = m_min_distance_front;
    }

    m_max_distance_front = distance;

    Update_Distance_rect();
}

void cRokko::Set_Max_Distance_Sides(float distance)
{
    if (distance < 50.0f) {
        distance = 50.0f;
    }

    m_max_distance_sides = distance;

    Update_Distance_rect();
}

void cRokko::Activate(bool with_sound /* = 1 */)
{
    if (with_sound) {
        pAudio->Play_Sound("enemy/rokko/activate.wav");
    }

    m_state = STA_FLY;
    m_massive_type = MASS_MASSIVE;
    Set_Active(1);

    if (m_direction == DIR_LEFT) {
        Set_Velocity(-m_speed, 0.0f);
    }
    else if (m_direction == DIR_RIGHT) {
        Set_Velocity(m_speed, 0.0f);
    }
    else if (m_direction == DIR_UP) {
        Set_Velocity(0.0f, -m_speed);
    }
    else if (m_direction == DIR_DOWN) {
        Set_Velocity(0.0f, m_speed);
    }
}

void cRokko::DownGrade(bool force /* = 0 */)
{
    Set_Dead(1);
    Set_Image_Set("break");
    m_massive_type = MASS_PASSIVE;
    m_gravity_max = 26.0f;
    m_vely = 0;

    if (!force) {
        // animation
        cParticle_Emitter* anim = new cParticle_Emitter(m_sprite_manager);
        Generate_Hit_Animation(anim);

        anim->Set_Quota(8);
        anim->Set_Speed(4, 1);
        anim->Set_Scale(0.9f);
        anim->Emit();
        pActive_Animation_Manager->Add(anim);
    }
}

void cRokko::Update_Normal_Dying(void)
{
    if (m_vely < m_gravity_max) {
        Add_Velocity_Y_Max(1.5f, m_gravity_max);
    }
    if(m_curr_img == m_break_end) {
        Set_Animation(false);
    }

    Move(m_velx, m_vely);

    if (m_rot_z - m_start_rot_z < 90) {
        Add_Rotation_Z(pFramerate->m_speed_factor);
    }

    // generate smoke
    m_smoke_counter += pFramerate->m_speed_factor * 4;
    if (m_smoke_counter >= 2.0f) {
        Generate_Smoke(static_cast<int>(m_smoke_counter));
        Generate_Sparks(static_cast<int>(m_smoke_counter * 0.5f));
        m_smoke_counter -= static_cast<int>(m_smoke_counter);
    }

    // below ground
    if (m_col_rect.m_y - 200.0f > pActive_Camera->m_limit_rect.m_y + game_res_h) {
        m_rot_z = 0.0f;
        m_massive_type = MASS_PASSIVE;
        Set_Active(0);
        m_velx = 0.0f;
    }
}

void cRokko::Update_Instant_Dying()
{
    Update_Normal_Dying();
}

void cRokko::Update(void)
{
    cEnemy::Update();

    if (!m_valid_update || !Is_In_Range())
    {
        // cEnemy::Is_Update_Valid returns false when dead, but we want
        // to continue to show the breaking animation
        Update_Animation();
        return;
    }

    // if not active
    if (m_state != STA_FLY) {
        // Do not self-activate when manual triggering is enabled
        if (m_manual)
            return;
        // Do not activate if Alex is a ghost
        else if (pLevel_Player->m_alex_type == ALEX_GHOST)
            return;
        // if player is in front then activate
        else if (pLevel_Player->m_col_rect.Intersects(Get_Final_Distance_Rect()))
            Activate();
        // Do not activate if Alex is not near by
        else
            return;
    }

    Update_Animation();

    // generate smoke
    m_smoke_counter += pFramerate->m_speed_factor * 4.0f;
    if (m_smoke_counter >= 1.0f) {
        Generate_Smoke(static_cast<int>(m_smoke_counter));
        m_smoke_counter -= static_cast<int>(m_smoke_counter);
    }
}

void cRokko::Draw(cSurface_Request* request /* = NULL */)
{
    if (!m_valid_draw) {
        return;
    }

    // draw distance rect
    if (editor_level_enabled) {
        GL_rect final_distance = Get_Final_Distance_Rect();
        final_distance.m_x -= pActive_Camera->m_x;
        final_distance.m_y -= pActive_Camera->m_y;

        pVideo->Draw_Rect(&final_distance, m_pos_z - m_corrected_pos_z_delta, &whitealpha128);
    }

    bool create_request = 0;

    if (!request) {
        create_request = 1;
        // create request
        request = new cSurface_Request();
    }

    // Draw
    cEnemy::Draw(request);

    // alpha in debug mode
    if (editor_level_enabled) {
        request->m_color.alpha = 64;
    }

    if (create_request) {
        // add request
        pRenderer->Add(request);
    }
}

void cRokko::Update_Distance_rect(void)
{
    if (m_start_direction == DIR_LEFT) {
        m_distance_rect.m_x = -m_max_distance_front;
        m_distance_rect.m_y = ((m_col_pos.m_y + m_col_rect.m_h) * 0.5f) - (m_max_distance_sides * 0.5f);
        m_distance_rect.m_w = m_max_distance_front;
        m_distance_rect.m_h = m_max_distance_sides;

        // add some space to not activate directly in front of the player
        m_distance_rect.m_w -= m_min_distance_front;
    }
    else if (m_start_direction == DIR_RIGHT) {
        m_distance_rect.m_x = m_rect.m_w;
        m_distance_rect.m_y = ((m_col_pos.m_y + m_col_rect.m_h) * 0.5f) - (m_max_distance_sides * 0.5f);
        m_distance_rect.m_w = m_max_distance_front;
        m_distance_rect.m_h = m_max_distance_sides;

        // add some space to not activate directly in front of the player
        m_distance_rect.m_x += m_min_distance_front;
        m_distance_rect.m_w -= m_min_distance_front;
    }
    else if (m_start_direction == DIR_UP) {
        m_distance_rect.m_x = ((m_col_pos.m_x + m_col_rect.m_w) * 0.5f) - (m_max_distance_sides * 0.5f);
        m_distance_rect.m_y = -m_max_distance_front;
        m_distance_rect.m_w = m_max_distance_sides;
        m_distance_rect.m_h = m_max_distance_front;

        // add some space to not activate directly in front of the player
        m_distance_rect.m_h -= m_min_distance_front;
    }
    else if (m_start_direction == DIR_DOWN) {
        m_distance_rect.m_x = ((m_col_pos.m_x + m_col_rect.m_w) * 0.5f) - (m_max_distance_sides * 0.5f);
        m_distance_rect.m_y = m_rect.m_h;
        m_distance_rect.m_w = m_max_distance_sides;
        m_distance_rect.m_h = m_max_distance_front;

        // add some space to not activate directly in front of the player
        m_distance_rect.m_y += m_min_distance_front;
        m_distance_rect.m_h -= m_min_distance_front;
    }
}

GL_rect cRokko::Get_Final_Distance_Rect(void) const
{
    GL_rect final_distance = m_distance_rect;

    final_distance.m_x += m_rect.m_x;
    final_distance.m_y += m_rect.m_y;

    return final_distance;
}

void cRokko::Generate_Smoke(unsigned int amount /* = 10 */) const
{
    cParticle_Emitter* anim = NULL;

    // moving smoke particle animation
    anim = new cParticle_Emitter(m_sprite_manager);

    // not dead
    if (!m_dead) {
        if (m_direction == DIR_LEFT) {
            anim->Set_Emitter_Rect(m_pos_x + m_col_rect.m_w + 20, m_pos_y + 20, 6, m_rect.m_h - 40);
            anim->Set_Direction_Range(280, 100);
        }
        else if (m_direction == DIR_RIGHT) {
            anim->Set_Emitter_Rect(m_pos_x + 80, m_pos_y + 20, 6, m_rect.m_h - 40);
            anim->Set_Direction_Range(180, 100);
        }
        else if (m_direction == DIR_UP) {
            anim->Set_Emitter_Rect(m_pos_x + 150, m_pos_y + m_col_rect.m_h - 75, m_rect.m_w - 40, 6);
            anim->Set_Direction_Range(50, 80);
        }
        // down
        else {
            anim->Set_Emitter_Rect(m_pos_x + 160, m_pos_y - 30, m_rect.m_w - 40, 6);
            anim->Set_Direction_Range(240, 80);
        }

        anim->Set_Scale(0.3f, 0.4f);
    }
    // dead
    else {
        anim->Set_Emitter_Rect(m_pos_x + (m_col_rect.m_w * 0.2f), m_pos_y + (m_col_rect.m_h * 0.2f), m_col_rect.m_w * 0.3f, m_col_rect.m_h * 0.3f);
        anim->Set_Direction_Range(180, 180);
        anim->Set_Scale(0.3f, 0.6f);
    }

    // - m_pos_z_delta caused a weird graphical z pos bug with an ATI card
    anim->Set_Pos_Z(m_pos_z - m_corrected_pos_z_delta);
    anim->Set_Image(pVideo->Get_Surface("animation/particles/smoke_grey_big.png"));
    anim->Set_Quota(amount);
    anim->Set_Time_to_Live(0.8f, 0.8f);
    anim->Set_Speed(1.0f, 0.2f);
    anim->Set_Const_Rotation_Z(-1, 2);
    anim->Set_Color(Color(static_cast<uint8_t>(155), 150, 130));
    anim->Set_Fading_Alpha(1);

    anim->Emit();
    pActive_Animation_Manager->Add(anim);
}

void cRokko::Generate_Sparks(unsigned int amount /* = 5 */) const
{
    // animation
    cParticle_Emitter* anim = new cParticle_Emitter(m_sprite_manager);
    anim->Set_Emitter_Rect(m_pos_x + m_col_rect.m_w * 0.2f, m_pos_y + m_rect.m_h * 0.2f, m_col_rect.m_w * 0.6f, m_rect.m_h * 0.6f);
    anim->Set_Pos_Z(m_pos_z + m_corrected_pos_z_delta);
    anim->Set_Quota(amount);
    anim->Set_Time_to_Live(0.2f, 0.1f);
    anim->Set_Speed(1.2f, 1.1f);
    anim->Set_Image(pVideo->Get_Surface("animation/particles/light.png"));
    anim->Set_Color(Color(static_cast<uint8_t>(250), 250, 200), Color(static_cast<uint8_t>(5), 5, 0, 0));
    anim->Set_Scale(0.3f, 0.3f);
    anim->Set_Fading_Size(1);
    anim->Set_Fading_Alpha(0);
    anim->Emit();
    pActive_Animation_Manager->Add(anim);
}

bool cRokko::Is_Draw_Valid(void)
{
    bool valid = cEnemy::Is_Draw_Valid();

    // if editor enabled
    if (editor_enabled) {
        // if active mouse object
        if (pMouseCursor->m_active_object == this) {
            return 1;
        }
    }

    return valid;
}

Col_Valid_Type cRokko::Validate_Collision(cSprite* obj)
{
    if (obj->m_massive_type == MASS_MASSIVE) {
        switch (obj->m_type) {
        case TYPE_PLAYER: {
            return COL_VTYPE_INTERNAL;
        }
        case TYPE_BALL: {
            return COL_VTYPE_INTERNAL;
        }
        default: {
            break;
        }
        }

        return COL_VTYPE_NOT_VALID;
    }

    return COL_VTYPE_NOT_VALID;
}

void cRokko::Handle_Collision_Player(cObjectCollision* collision)
{
    // if invalid
    if (collision->m_direction == DIR_UNDEFINED) {
        return;
    }

    if (m_direction == DIR_LEFT || m_direction == DIR_RIGHT) {
        // if invincible
        if (pLevel_Player->m_invincible > 0.0f) {
            return;
        }

        if (collision->m_direction == DIR_TOP && pLevel_Player->m_state != STA_FLY) {
            gp_hud->Add_Points(m_kill_points, m_pos_x + m_rect.m_w / 3, m_pos_y - 10.0f, "", static_cast<uint8_t>(255), 1);
            pAudio->Play_Sound(m_kill_sound);
            pLevel_Player->Action_Jump(1);

            pLevel_Player->Add_Kill_Multiplier();
            DownGrade();
        }
        else {
            pLevel_Player->DownGrade_Player();
        }
    }
    else if (m_direction == DIR_UP || m_direction == DIR_DOWN) {
        if ((collision->m_direction == DIR_LEFT || collision->m_direction == DIR_LEFT) && pLevel_Player->m_state == STA_FLY) {
            gp_hud->Add_Points(m_kill_points, m_pos_x, m_pos_y - 5.0f, "", static_cast<uint8_t>(255), 1);
            pAudio->Play_Sound(m_kill_sound);

            pLevel_Player->Add_Kill_Multiplier();
            DownGrade();
        }
        else {
            pLevel_Player->DownGrade_Player();
        }
    }
}

void cRokko::Handle_out_of_Level(ObjectDirection dir)
{
    // fixme : needs a Handle_out_of_Level_Complete function
    //Set_Active( 0 );
}

void cRokko::Editor_Activate(void)
{
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    // direction
    CEGUI::Combobox* combobox = static_cast<CEGUI::Combobox*>(wmgr.createWindow("TSCLook256/Combobox", "editor_rokko_direction"));

    combobox->addItem(new CEGUI::ListboxTextItem("left"));
    combobox->addItem(new CEGUI::ListboxTextItem("right"));
    combobox->addItem(new CEGUI::ListboxTextItem("up"));
    combobox->addItem(new CEGUI::ListboxTextItem("down"));

    combobox->setText(Get_Direction_Name(m_start_direction));
    combobox->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cRokko::Editor_Direction_Select, this));
    pLevel_Editor->Add_Config_Widget(UTF8_("Direction"), UTF8_("Direction it moves into."), combobox);

    // speed
    CEGUI::Editbox* editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TSCLook256/Editbox", "editor_rokko_speed"));
    pLevel_Editor->Add_Config_Widget(UTF8_("Speed"), UTF8_("Speed when activated"), editbox);

    editbox->setValidationString("[+]?[0-9]*\\.?[0-9]*");
    editbox->setText(float_to_string(m_speed, 6, 0));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cRokko::Editor_Speed_Text_Changed, this));

    // init
    Editor_Init();
}

bool cRokko::Editor_Direction_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    Set_Direction(Get_Direction_Id(item->getText().c_str()));

    return 1;
}

bool cRokko::Editor_Speed_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Speed(string_to_float(str_text));

    return 1;
}

void cRokko::Set_Manual(bool manual)
{
    m_manual = manual;
}

bool cRokko::Get_Manual()
{
    return m_manual;
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

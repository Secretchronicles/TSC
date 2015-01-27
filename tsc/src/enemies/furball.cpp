/***************************************************************************
 * furball.cpp  -  little moving around enemy
 *
 * Copyright © 2003 - 2011 Florian Richter
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

#include "../enemies/furball.hpp"
#include "../core/game_core.hpp"
#include "../core/xml_attributes.hpp"
#include "../level/level_player.hpp"
#include "../gui/hud.hpp"
#include "../core/i18n.hpp"
#include "../video/animation.hpp"
#include "../level/level_manager.hpp"
#include "../core/sprite_manager.hpp"
#include "../level/level.hpp"
//#include "../script/events/downgrade_event.hpp"
#include "../core/global_basic.hpp"

using namespace std;

namespace TSC {

/* *** *** *** *** *** *** cFurball *** *** *** *** *** *** *** *** *** *** *** */

cFurball::cFurball(cSprite_Manager* sprite_manager)
    : cEnemy(sprite_manager)
{
    cFurball::Init();
}

cFurball::cFurball(XmlAttributes& attributes, cSprite_Manager* sprite_manager)
    : cEnemy(sprite_manager)
{
    cFurball::Init();

    // position
    Set_Pos(string_to_float(attributes["posx"]), string_to_float(attributes["posy"]), true);

    // color
    Set_Color(static_cast<DefaultColor>(Get_Color_Id(attributes.fetch("color", Get_Color_Name(m_color_type)))));

    // direction
    Set_Direction(Get_Direction_Id(attributes.fetch("direction", Get_Direction_Name(m_start_direction))));

    if (m_type == TYPE_FURBALL_BOSS) {
        // max downgrade count
        Set_Max_Downgrade_Count(string_to_int(attributes.fetch("max_downgrade_count", int_to_string(m_max_downgrade_count))));

        // level ends if killed
        Set_Level_Ends_If_Killed(string_to_bool(attributes.fetch("level_ends_if_killed", bool_to_string(m_level_ends_if_killed))));
    }
}

cFurball::~cFurball(void)
{
    //
}

void cFurball::Init(void)
{
    m_type = TYPE_FURBALL;
    m_name = "Furball";
    m_pos_z = 0.09f;
    m_gravity_max = 19.0f;

    m_counter_hit = 0.0f;
    m_counter_running = 0.0f;
    m_running_particle_counter = 0.0f;
    m_downgrade_count = 0;
    m_max_downgrade_count = 5;
    m_level_ends_if_killed = 1;

    m_color_type = COL_DEFAULT;
    Set_Color(COL_BROWN);
    m_state = STA_FALL;
    Set_Moving_State(STA_WALK);
    Set_Direction(DIR_RIGHT);
}

cFurball* cFurball::Copy(void) const
{
    cFurball* furball = new cFurball(m_sprite_manager);
    furball->Set_Pos(m_start_pos_x, m_start_pos_y);
    furball->Set_Color(m_color_type);
    furball->Set_Direction(m_start_direction);
    if (m_type == TYPE_FURBALL_BOSS) {
        furball->Set_Max_Downgrade_Count(m_max_downgrade_count);
        furball->Set_Level_Ends_If_Killed(m_level_ends_if_killed);
    }
    return furball;
}

std::string cFurball::Get_XML_Type_Name()
{
    return "furball";
}

xmlpp::Element* cFurball::Save_To_XML_Node(xmlpp::Element* p_element)
{
    xmlpp::Element* p_node = cEnemy::Save_To_XML_Node(p_element);

    Add_Property(p_node, "color", Get_Color_Name(m_color_type));
    Add_Property(p_node, "direction", Get_Direction_Name(m_start_direction));

    if (m_type == TYPE_FURBALL_BOSS) {
        Add_Property(p_node, "max_downgrade_count", m_max_downgrade_count);
        Add_Property(p_node, "level_ends_if_killed", m_level_ends_if_killed);
    }

    return p_node;
}

void cFurball::Load_From_Savegame(cSave_Level_Object* save_object)
{
    cEnemy::Load_From_Savegame(save_object);

    Update_Rotation_Hor();
}

void cFurball::Set_Max_Downgrade_Count(int max_downgrade_counts)
{
    m_max_downgrade_count = max_downgrade_counts;

    if (m_max_downgrade_count < 0) {
        m_max_downgrade_count = 0;
    }
}

void cFurball::Set_Level_Ends_If_Killed(bool level_ends_if_killed)
{
    m_level_ends_if_killed = level_ends_if_killed;
}

void cFurball::Set_Direction(const ObjectDirection dir)
{
    // already set
    if (m_start_direction == dir) {
        return;
    }

    cEnemy::Set_Direction(dir, 1);

    Update_Rotation_Hor(1);
}

void cFurball::Set_Color(const DefaultColor& col)
{
    // already set
    if (m_color_type == col) {
        return;
    }

    // clear old images
    Clear_Images();

    m_color_type = col;
    std::string filename_dir;

    if (m_color_type == COL_BROWN) {
        filename_dir = "brown";

        m_kill_points = 10;
        m_fire_resistant = 0;
        m_ice_resistance = 0.0f;
        m_can_be_hit_from_shell = 1;
    }
    else if (m_color_type == COL_BLUE) {
        filename_dir = "blue";

        m_kill_points = 50;
        m_fire_resistant = 0;
        m_ice_resistance = 0.9f;
        m_can_be_hit_from_shell = 1;
    }
    else if (m_color_type == COL_BLACK) {
        filename_dir = "boss";
        m_type = TYPE_FURBALL_BOSS;

        m_kill_points = 2500;
        m_fire_resistant = 1;
        m_ice_resistance = 1.0f;
        m_can_be_hit_from_shell = 0;
    }
    else {
        cerr << "Error : Unknown Furball Color " << m_color_type  << endl;
        return;
    }

    Update_Velocity_Max();

    Add_Image_Set("walk", "enemy/furball/" + filename_dir + "/walk.imgset");
    Add_Image_Set("turn", "enemy/furball/" + filename_dir + "/turn.imgset", 0, &m_turn_start, &m_turn_end);
    Add_Image_Set("dead", "enemy/furball/" + filename_dir + "/dead.imgset");
    if (m_type == TYPE_FURBALL_BOSS) {
        Add_Image_Set("hit", "enemy/furball/" + filename_dir + "/hit.imgset");
    }

    Set_Image_Set("walk", true);
}

void cFurball::Turn_Around(ObjectDirection col_dir /* = DIR_UNDEFINED */)
{
    cEnemy::Turn_Around(col_dir);

    if (col_dir == DIR_LEFT || col_dir == DIR_RIGHT) {
        if (m_state == STA_WALK || m_state == STA_RUN) {
            m_velx *= 0.5f;
        }

        // set turn around image
        Set_Image_Set("turn");
    }
    // only update rotation if no turn around image
    else {
        Update_Rotation_Hor();
    }
}

void cFurball::DownGrade(bool force /* = 0 */)
{
    // boss
    if (!force && m_type == TYPE_FURBALL_BOSS) {
        // can not get hit if staying or running
        if (m_state == STA_STAY || m_state == STA_RUN) {
            return;
        }

        // Increment number of times hit
        m_downgrade_count++;

        // Issue the Downgrade event
        //Script::cDowngrade_Event evt(m_downgrade_count, m_max_downgrade_count);
        //evt.Fire(pActive_Level->m_lua, this);

        // die
        if (m_downgrade_count == m_max_downgrade_count) {
            Set_Dead(1);
        }
        else {
            Generate_Hit_Animation();
            // set hit image
            if(m_type == TYPE_FURBALL_BOSS) {
                Set_Image_Set("hit");
            }
            Set_Moving_State(STA_STAY);
        }
    }
    // normal
    else {
        Set_Dead(1);
    }

    if (m_dead) {
        m_massive_type = MASS_PASSIVE;
        m_counter = 0.0f;
        m_velx = 0.0f;
        m_vely = 0.0f;
        // dead image
        Set_Image_Set("dead");

        // default stomp death
        if (!force || m_type == TYPE_FURBALL_BOSS) {
            Generate_Hit_Animation();

            if (m_type != TYPE_FURBALL_BOSS) {
                Set_Scale_Directions(1, 0, 1, 1);
            }
            else {
                // set scaling for death animation
                Set_Scale_Affects_Rect(1);

                if (m_level_ends_if_killed) {
                    // fade out music
                    pAudio->Fadeout_Music(500);
                    // play finish music
                    pAudio->Play_Music("game/courseclear_A.ogg", 0, 0);
                }
            }
        }
        // falling death
        else {
            Set_Rotation_Z(180);
            Set_Scale_Directions(1, 1, 1, 1);
        }
    }
}

void cFurball::Update_Normal_Dying()
{
    // boss
    if (m_type == TYPE_FURBALL_BOSS) {
        if (m_scale_x > 0.1f) {
            float speed_x = pFramerate->m_speed_factor * 10.0f;

            if (m_direction == DIR_LEFT) {
                speed_x *= -1;
            }

            Add_Rotation_Z(speed_x);
            Add_Scale(-pFramerate->m_speed_factor * 0.025f);

            // star animation
            if (m_dying_counter >= 1.0f) {
                Generate_Smoke(static_cast<unsigned int>(m_dying_counter), 0.3f);
                m_dying_counter -= static_cast<int>(m_dying_counter);
            }

            // finished scale out animation
            if (m_scale_x <= 0.1f) {
                // sound
                pAudio->Play_Sound(m_kill_sound);

                // star explosion animation
                Generate_Smoke(30);

                // set empty image
                cMovingSprite::Set_Image(NULL, 0, 0);
                // reset counter
                m_dying_counter = 0;
            }
        }
        // after scale animation
        else {
            // wait some time
            if (m_dying_counter > 20.0f) {
                if (m_level_ends_if_killed) {
                    // exit level
                    pLevel_Manager->Finish_Level();
                }

                // reset scaling
                Set_Scale_Affects_Rect(0);
            }
        }
    }
    // normal
    else
        cEnemy::Update_Normal_Dying();
}

void cFurball::Set_Moving_State(Moving_state new_state)
{
    if (new_state == m_state) {
        return;
    }

    if (new_state == STA_STAY) {
        Set_Animation(0);
    }
    else if (new_state == STA_WALK) {
        m_counter_running = 0.0f;

        Set_Image_Set("walk");
        if (m_color_type == COL_BLUE) {
            Set_Animation_Speed(1.142);
        }
        else {
            Set_Animation_Speed(1.0);
        }
    }
    else if (new_state == STA_RUN) {
        m_counter_hit = 0.0f;

        Set_Image_Set("walk");
        Set_Animation_Speed(1.142);
    }

    m_state = new_state;

    Update_Velocity_Max();
    // if in the first part of the turn around animation
    Update_Rotation_Hor();
}

void cFurball::Update(void)
{
    cEnemy::Update();

    if (!m_valid_update || !Is_In_Range()) {
        return;
    }

    Update_Animation();

    if (m_state == STA_STAY) {
        m_counter_hit += pFramerate->m_speed_factor;

        // angry
        if (static_cast<int>(m_counter_hit) % 2 == 1) {
            // slowly fade to the color
            cMovingSprite::Set_Color(Color(static_cast<Uint8>(255), 250 - static_cast<Uint8>(m_counter_hit * 1.5f), 250 - static_cast<Uint8>(m_counter_hit * 4)));
        }
        // default
        else {
            cMovingSprite::Set_Color(white);
        }

        // rotate slowly
        Set_Rotation_Z(-m_counter_hit * 0.125f);

        // slow down
        if (!Is_Float_Equal(m_velx, 0.0f)) {
            Add_Velocity_X(-m_velx * 0.25f);

            if (m_velx < 0.3f && m_velx > -0.3f) {
                m_velx = 0.0f;
            }
        }

        // finished hit animation
        if (m_counter_hit > 60.0f) {
            // run
            Set_Moving_State(STA_RUN);

            // jump a bit up
            m_vely = -5.0f;

            //pAudio->Play_Sound( "enemy/boss/furball/run.wav" );
        }
    }
    else if (m_state == STA_RUN) {
        m_counter_running += pFramerate->m_speed_factor;

        // rotate slowly back
        Set_Rotation_Z(-7.5f + m_counter_running * 0.0625f);

        // finished hit animation
        if (m_counter_running > 120.0f) {
            // walk
            Set_Moving_State(STA_WALK);
        }

        // running particles
        m_running_particle_counter += pFramerate->m_speed_factor * 1.5f;



        // create particles
        if (m_running_particle_counter >= 1.0f) {
            cParticle_Emitter* anim = new cParticle_Emitter(m_sprite_manager);
            anim->Set_Emitter_Rect(m_col_rect.m_x, m_col_rect.m_y + m_col_rect.m_h - 2.0f, m_col_rect.m_w);
            anim->Set_Quota(static_cast<int>(m_running_particle_counter));
            anim->Set_Pos_Z(m_pos_z - 0.000001f);
            anim->Set_Image(pVideo->Get_Package_Surface("animation/particles/smoke_black.png"));
            anim->Set_Time_to_Live(0.6f);
            anim->Set_Scale(0.2f);

            float vel;

            if (m_velx > 0.0f) {
                vel = m_velx;
            }
            else {
                vel = -m_velx;
            }

            anim->Set_Speed(vel * 0.08f, 0.1f + vel * 0.1f);

            if (m_direction == DIR_RIGHT) {
                anim->Set_Direction_Range(180.0f, 90.0f);
            }
            else {
                anim->Set_Direction_Range(270.0f, 90.0f);
            }

            anim->Emit();
            pActive_Animation_Manager->Add(anim);

            m_running_particle_counter -= static_cast<int>(m_running_particle_counter);
        }
    }

    if (m_state != STA_STAY) {
        // if turn around image
        if (m_curr_img >= m_turn_start && m_curr_img <= m_turn_end) {
            m_anim_counter += pFramerate->m_elapsed_ticks;

            // back to normal animation
            if (m_anim_counter >= 200) {
                Set_Image_Set("walk");
                Update_Rotation_Hor();
            }
            // rotate the turn image
            else if (m_anim_counter >= 100) {
                Update_Rotation_Hor();
            }
        }
        else {
            Update_Velocity();
        }
    }
}

void cFurball::Generate_Smoke(unsigned int amount /* = 1 */, float particle_scale /* = 0.4f */) const
{
    // animation
    cParticle_Emitter* anim = new cParticle_Emitter(m_sprite_manager);
    anim->Set_Pos(m_pos_x + (m_col_rect.m_w * 0.5f), m_pos_y + (m_col_rect.m_h * 0.5f), 1);
    anim->Set_Image(pVideo->Get_Package_Surface("animation/particles/smoke_grey_big.png"));
    anim->Set_Quota(amount);
    anim->Set_Pos_Z(m_pos_z + 0.000001f);
    anim->Set_Const_Rotation_Z(-6.0f, 12.0f);
    anim->Set_Time_to_Live(1.5f);
    anim->Set_Speed(0.4f, 0.9f);
    anim->Set_Scale(particle_scale, 0.2f);
    anim->Set_Color(black, Color(static_cast<Uint8>(87), 60, 40, 0));
    anim->Emit();
    pActive_Animation_Manager->Add(anim);
}

void cFurball::Update_Velocity_Max(void)
{
    if (m_state == STA_STAY) {
        m_velx_max = 0.0f;
        m_velx_gain = 0.0f;
    }
    else if (m_state == STA_WALK) {
        if (m_color_type == COL_BROWN) {
            m_velx_max = 2.7f;
            m_velx_gain = 0.2f;
        }
        else if (m_color_type == COL_BLUE) {
            m_velx_max = 4.5f;
            m_velx_gain = 0.3f;
        }
        else if (m_color_type == COL_BLACK) {
            m_velx_max = 4.0f + (m_downgrade_count * 1.0f);
            m_velx_gain = 0.6f + (m_downgrade_count * 0.15f);
        }
    }
    else if (m_state == STA_RUN) {
        if (m_color_type == COL_BROWN) {
            m_velx_max = 4.0f;
            m_velx_gain = 0.3f;
        }
        else if (m_color_type == COL_BLUE) {
            m_velx_max = 6.7f;
            m_velx_gain = 0.5f;
        }
        else if (m_color_type == COL_BLACK) {
            m_velx_max = 6.0f + (m_downgrade_count * 1.4f);
            m_velx_gain = 0.8f + (m_downgrade_count * 0.25f);
        }
    }
}

Col_Valid_Type cFurball::Validate_Collision(cSprite* obj)
{
    // basic validation checking
    Col_Valid_Type basic_valid = Validate_Collision_Ghost(obj);

    // found valid collision
    if (basic_valid != COL_VTYPE_NOT_POSSIBLE) {
        return basic_valid;
    }

    if (obj->m_massive_type == MASS_MASSIVE) {
        switch (obj->m_type) {
        case TYPE_PLAYER: {
            if (m_type == TYPE_FURBALL_BOSS) {
                // player is invincible
                if (pLevel_Player->m_invincible > 0.0f) {
                    return COL_VTYPE_NOT_VALID;
                }

                return COL_VTYPE_BLOCKING;
            }

            break;
        }
        case TYPE_STATIC_ENEMY: {
            if (m_type == TYPE_FURBALL_BOSS) {
                return COL_VTYPE_NOT_VALID;
            }

            break;
        }
        case TYPE_SPIKEBALL: {
            if (m_type == TYPE_FURBALL_BOSS) {
                return COL_VTYPE_NOT_VALID;
            }

            break;
        }
        case TYPE_BALL: {
            // ignore balls
            if (m_type == TYPE_FURBALL_BOSS) {
                return COL_VTYPE_NOT_VALID;
            }

            break;
        }
        case TYPE_FLYON: {
            return COL_VTYPE_NOT_VALID;
        }
        case TYPE_ROKKO: {
            return COL_VTYPE_NOT_VALID;
        }
        case TYPE_GEE: {
            return COL_VTYPE_NOT_VALID;
        }
        default: {
            break;
        }
        }

        return COL_VTYPE_BLOCKING;
    }
    else if (obj->m_massive_type == MASS_HALFMASSIVE) {
        // if moving downwards and the object is on bottom
        if (m_vely >= 0.0f && Is_On_Top(obj)) {
            return COL_VTYPE_BLOCKING;
        }
    }
    else if (obj->m_massive_type == MASS_PASSIVE) {
        switch (obj->m_type) {
        case TYPE_ENEMY_STOPPER: {
            return COL_VTYPE_BLOCKING;
        }
        default: {
            break;
        }
        }
    }

    return COL_VTYPE_NOT_VALID;
}

void cFurball::Handle_Collision_Player(cObjectCollision* collision)
{
    // invalid
    if (collision->m_direction == DIR_UNDEFINED) {
        return;
    }

    if (collision->m_direction == DIR_TOP && pLevel_Player->m_state != STA_FLY) {
        if (m_type == TYPE_FURBALL_BOSS) {
            if (m_state == STA_STAY || m_state == STA_RUN) {
                pAudio->Play_Sound("enemy/boss/furball/hit_failed.wav");
            }
            else {
                pAudio->Play_Sound("enemy/boss/furball/hit.wav");
            }
        }
        else {
            pAudio->Play_Sound(m_kill_sound);
        }

        DownGrade();
        pLevel_Player->Action_Jump(1);

        if (m_dead) {
            pHud_Points->Add_Points(m_kill_points, m_pos_x, m_pos_y - 5.0f, "", static_cast<Uint8>(255), 1);
            pLevel_Player->Add_Kill_Multiplier();
        }
    }
    else {
        pLevel_Player->DownGrade_Player();

        if (collision->m_direction == DIR_RIGHT || collision->m_direction == DIR_LEFT) {
            Turn_Around(collision->m_direction);
        }
    }
}

void cFurball::Handle_Collision_Enemy(cObjectCollision* collision)
{
    if (collision->m_direction == DIR_RIGHT || collision->m_direction == DIR_LEFT) {
        Turn_Around(collision->m_direction);
    }

    Send_Collision(collision);
}

void cFurball::Handle_Collision_Massive(cObjectCollision* collision)
{
    if (m_state == STA_OBJ_LINKED) {
        return;
    }

    Send_Collision(collision);

    // get colliding object
    cSprite* col_object = m_sprite_manager->Get_Pointer(collision->m_number);

    if (col_object->m_type == TYPE_BALL) {
        return;
    }

    if (collision->m_direction == DIR_TOP) {
        if (m_vely < 0.0f) {
            m_vely = 0.0f;
        }
    }
    else if (collision->m_direction == DIR_BOTTOM) {
        if (m_vely > 0.0f) {
            m_vely = 0.0f;
        }
    }
    else if (collision->m_direction == DIR_RIGHT || collision->m_direction == DIR_LEFT) {
        Turn_Around(collision->m_direction);
    }
}

void cFurball::Handle_Collision_Box(ObjectDirection cdirection, GL_rect* r2)
{
    pAudio->Play_Sound(m_kill_sound);
    pHud_Points->Add_Points(m_kill_points, m_pos_x, m_pos_y - 5.0f, "", static_cast<Uint8>(255), 1 );
    pLevel_Player->Add_Kill_Multiplier();
    DownGrade(true);
}

void cFurball::Editor_Activate(void)
{
    // get window manager
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    // direction
    CEGUI::Combobox* combobox = static_cast<CEGUI::Combobox*>(wmgr.createWindow("TaharezLook/Combobox", "editor_furball_direction"));
    Editor_Add(UTF8_("Direction"), UTF8_("Starting direction."), combobox, 100, 75);

    combobox->addItem(new CEGUI::ListboxTextItem("left"));
    combobox->addItem(new CEGUI::ListboxTextItem("right"));

    combobox->setText(Get_Direction_Name(m_start_direction));
    combobox->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cFurball::Editor_Direction_Select, this));

    if (m_type == TYPE_FURBALL_BOSS) {
        // max downgrades
        CEGUI::Editbox* editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "editor_furball_max_downgrade_count"));
        Editor_Add(UTF8_("Downgrades"), UTF8_("Downgrades until death"), editbox, 120);

        editbox->setValidationString("^[+]?\\d*$");
        editbox->setText(int_to_string(m_max_downgrade_count));
        editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cFurball::Editor_Max_Downgrade_Count_Text_Changed, this));

        // level ends if killed
        combobox = static_cast<CEGUI::Combobox*>(wmgr.createWindow("TaharezLook/Combobox", "editor_furball_level_ends_if_killed"));
        Editor_Add(UTF8_("End Level"), UTF8_("End the level if it is killed."), combobox, 100, 75);

        combobox->addItem(new CEGUI::ListboxTextItem(UTF8_("Enabled")));
        combobox->addItem(new CEGUI::ListboxTextItem(UTF8_("Disabled")));

        if (m_level_ends_if_killed) {
            combobox->setText(UTF8_("Enabled"));
        }
        else {
            combobox->setText(UTF8_("Disabled"));
        }
        combobox->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cFurball::Editor_Level_Ends_If_Killed, this));
    }

    // init
    Editor_Init();
}

bool cFurball::Editor_Level_Ends_If_Killed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    if (item->getText().compare(UTF8_("Enabled")) == 0) {
        m_level_ends_if_killed = 1;
    }
    else {
        m_level_ends_if_killed = 0;
    }

    return 1;
}

bool cFurball::Editor_Direction_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    Set_Direction(Get_Direction_Id(item->getText().c_str()));

    return 1;
}

bool cFurball::Editor_Max_Downgrade_Count_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Max_Downgrade_Count(string_to_int(str_text));

    return 1;
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

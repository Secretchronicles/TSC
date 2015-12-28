/***************************************************************************
 * turtle_boss.cpp  -  turtle boss enemy class
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

#include "../../enemies/bosses/turtle_boss.hpp"
#include "../../core/game_core.hpp"
#include "../../objects/box.hpp"
#include "../../video/animation.hpp"
#include "../../level/level_player.hpp"
#include "../../level/level.hpp"
#include "../../gui/hud.hpp"
#include "../../video/gl_surface.hpp"
#include "../../core/sprite_manager.hpp"
#include "../../core/i18n.hpp"
#include "../../core/xml_attributes.hpp"
#include "../../core/global_basic.hpp"

using namespace std;

namespace TSC {

/* *** *** *** *** *** *** cTurtleBoss *** *** *** *** *** *** *** *** *** *** *** */

cTurtleBoss::cTurtleBoss(cSprite_Manager* sprite_manager)
    : cEnemy(sprite_manager)
{
    cTurtleBoss::Init();
}

cTurtleBoss::cTurtleBoss(XmlAttributes& attributes, cSprite_Manager* sprite_manager)
    : cEnemy(sprite_manager)
{
    cTurtleBoss::Init();

    // position
    Set_Pos(string_to_float(attributes["posx"]), string_to_float(attributes["posy"]), true);

    // direction
    Set_Direction(Get_Direction_Id(attributes.fetch("direction", Get_Direction_Name(m_start_direction))), true);

    // color
    Set_Color(static_cast<DefaultColor>(Get_Color_Id(attributes.fetch("color", Get_Color_Name(m_color_type)))));

    // max hits
    Set_Max_Hits(string_to_int(attributes.fetch("max_hit_count", int_to_string(m_max_hits))));

    // max downgrade count
    Set_Max_Downgrade_Counts(string_to_int(attributes.fetch("max_downgrade_count", int_to_string(m_max_downgrade_count))));

    // shell time
    Set_Shell_Time(string_to_int(attributes.fetch("shell_time", int_to_string(m_shell_time))));

    // level ends if killed
    Set_Level_Ends_If_Killed(string_to_bool(attributes.fetch("level_ends_if_killed", bool_to_string(m_level_ends_if_killed))));
}


cTurtleBoss::~cTurtleBoss(void)
{
    //
}

void cTurtleBoss::Init(void)
{
    m_type = TYPE_TURTLE_BOSS;
    m_name = "Turtle Boss";
    m_pos_z = 0.092f;
    m_gravity_max = 19.0f;

    m_player_counter = 0.0f;
    m_fire_resistant = 1;
    m_ice_resistance = 1.0f;
    m_kill_sound = "stomp_4.ogg";

    m_hits = 0;
    m_downgrade_count = 0;

    m_max_hits = 3;
    m_max_downgrade_count = 3;
    m_shell_time = 2.5f;

    m_run_time_counter = 0.0f;
    m_level_ends_if_killed = 1;
    m_can_be_hit_from_shell = 0;

    m_color_type = COL_DEFAULT;
    Set_Color(COL_RED); // Load images before calling set moving state

    m_turtle_state = TURTLEBOSS_DEAD;
    Set_Turtle_Moving_State(TURTLEBOSS_WALK);

    Set_Direction(DIR_RIGHT, 1);
}

cTurtleBoss* cTurtleBoss::Copy(void) const
{
    cTurtleBoss* turtle = new cTurtleBoss(m_sprite_manager);
    turtle->Set_Pos(m_start_pos_x, m_start_pos_y);
    turtle->Set_Direction(m_start_direction, 1);
    turtle->Set_Color(m_color_type);
    turtle->Set_Max_Hits(m_max_hits);
    turtle->Set_Max_Downgrade_Counts(m_max_downgrade_count);
    turtle->Set_Shell_Time(m_shell_time);
    turtle->Set_Level_Ends_If_Killed(m_level_ends_if_killed);
    return turtle;
}

std::string cTurtleBoss::Get_XML_Type_Name()
{
    return "turtleboss";
}

xmlpp::Element* cTurtleBoss::Save_To_XML_Node(xmlpp::Element* p_element)
{
    xmlpp::Element* p_node = cEnemy::Save_To_XML_Node(p_element);

    Add_Property(p_node, "color", Get_Color_Name(m_color_type));
    Add_Property(p_node, "direction", Get_Direction_Name(m_start_direction));
    Add_Property(p_node, "max_hit_count", m_max_hits);
    Add_Property(p_node, "max_downgrade_count", m_max_downgrade_count);
    Add_Property(p_node, "shell_time", m_shell_time);
    Add_Property(p_node, "level_ends_if_killed", m_level_ends_if_killed);

    return p_node;
}

void cTurtleBoss::Set_Max_Hits(int nmax_hits)
{
    m_max_hits = nmax_hits;

    if (m_max_hits < 0) {
        m_max_hits = 0;
    }
}

void cTurtleBoss::Set_Max_Downgrade_Counts(int nmax_downgrade_counts)
{
    m_max_downgrade_count = nmax_downgrade_counts;

    if (m_max_downgrade_count < 0) {
        m_max_downgrade_count = 0;
    }
}

void cTurtleBoss::Set_Shell_Time(float nmax_downgrade_time)
{
    m_shell_time = nmax_downgrade_time;

    if (m_shell_time < 0.0f) {
        m_shell_time = 0.0f;
    }
}

void cTurtleBoss::Set_Direction(const ObjectDirection dir, bool new_start_direction /* = 0 */)
{
    if (dir != DIR_RIGHT && dir != DIR_LEFT) {
        cerr << "Warning : Unknown Turtle Boss direction set " << Get_Direction_Name(dir) << endl;
        return;
    }

    cEnemy::Set_Direction(dir, new_start_direction);
    Update_Rotation_Hor(new_start_direction);
}

void cTurtleBoss::Set_Color(DefaultColor col)
{
    // already set
    if (m_color_type == col) {
        return;
    }

    m_color_type = col;

    Update_Velocity_Max();

    Clear_Images();

    if (m_color_type == COL_RED) {
        Add_Image_Set("still", "enemy/bosses/turtle/still.imgset");
        Add_Image_Set("walk", "enemy/bosses/turtle/walk.imgset", 0, &m_walk_start);
        Add_Image_Set("turn", "enemy/bosses/turtle/turn.imgset", 0, &m_turn_start, &m_turn_end);
        Add_Image_Set("dead", "enemy/bosses/turtle/dead.imgset");
        Add_Image_Set("shell_stand", "enemy/bosses/turtle/shell_stand.imgset", 0, &m_shell_stand_start);
        Add_Image_Set("shell_run", "enemy/bosses/turtle/shell_run.imgset");
        Add_Image_Set("shell_active", "enemy/bosses/turtle/shell_active.imgset");

        m_kill_points = 750;
    }
    // unknown color
    else {
        cerr << "Error : Unknown Turtle Boss color : " << m_color_type << endl;
    }

    Set_Image_Set("still", true);
}

void cTurtleBoss::Set_Level_Ends_If_Killed(bool level_ends_if_killed)
{
    m_level_ends_if_killed = level_ends_if_killed;
}

void cTurtleBoss::Turn_Around(ObjectDirection col_dir /* = DIR_UNDEFINED */)
{
    cEnemy::Turn_Around(col_dir);

    if (m_turtle_state == TURTLEBOSS_WALK) {
        m_velx *= 0.5f;
        // hack : disable turn image
        // Set_Image_Set("turn");
    }

    Update_Rotation_Hor();
}

void cTurtleBoss::DownGrade(bool force /* = 0 */)
{
    if (!force) {
        // normal walking
        if (m_turtle_state == TURTLEBOSS_WALK) {
            m_hits++;

            // state change
            if (m_hits >= m_max_hits) {
                m_counter = 0.0f;

                m_hits = 0;
                m_downgrade_count++;

                // die
                if (m_downgrade_count == m_max_downgrade_count) {
                    Set_Dead(1);
                }
                // downgrade
                else {
                    Set_Turtle_Moving_State(TURTLEBOSS_STAND_ANGRY);
                    m_velx *= 0.5f;
                }
            }
        }
        // staying
        else if (m_turtle_state == TURTLEBOSS_SHELL_STAND) {
            Set_Turtle_Moving_State(TURTLEBOSS_SHELL_RUN);
        }
    }
    // falling death
    else {
        // die
        Set_Dead(1);

        if (m_turtle_state == TURTLEBOSS_WALK) {
            if(m_walk_start >= 0 && m_shell_stand_start >= 0)
                Move(0.0f, m_images[m_walk_start].m_image->m_h - m_images[m_shell_stand_start].m_image->m_h, 1);
        }
    }

    if (m_dead) {
        if (m_level_ends_if_killed) {
            // fade out music
            pAudio->Fadeout_Music(500);
            // play finish music
            pAudio->Play_Music("game/courseclear_A.ogg", false, 0);
        }

        // set shell image
        Set_Image_Set("dead");

        m_massive_type = MASS_PASSIVE;
        m_counter = 0.0f;
        m_velx = 0.0f;
        m_vely = 0.0f;

        // set scaling for death animation
        Set_Scale_Affects_Rect(1);
    }
}

void cTurtleBoss::Update_Normal_Dying()
{
    if (m_scale_x > 0.1f) {
        float speed_x = pFramerate->m_speed_factor * 10.0f;

        if (m_direction == DIR_LEFT) {
            speed_x *= -1;
        }

        Add_Rotation_Z(speed_x);
        Add_Scale(-pFramerate->m_speed_factor * 0.025f);

        // star animation
        if (m_dying_counter >= 1.0f) {
            Generate_Stars(static_cast<unsigned int>(m_dying_counter), 0.1f);
            m_dying_counter -= static_cast<int>(m_dying_counter);
        }

        // finished scale out animation
        if (m_scale_x <= 0.1f) {
            // sound
            pAudio->Play_Sound("enemy/turtle/shell/hit.ogg");

            // star explosion animation
            Generate_Stars(30);

            // set empty image
            cMovingSprite::Set_Image(NULL, 0, 0);
            // reset counter
            m_dying_counter = 0.0f;
        }
    }
    // after scale animation
    else {
        // wait some time
        if (m_dying_counter > 20.0f) {
            Set_Active(0);
            m_turtle_state = TURTLEBOSS_DEAD;

            if (m_level_ends_if_killed) {
                // exit level
                pLevel_Manager->Finish_Level();
            }

            // reset scaling
            Set_Scale_Affects_Rect(0);
        }
    }
}

void cTurtleBoss::Update_Instant_Dying()
{
    Update_Normal_Dying();
}

void cTurtleBoss::Set_Turtle_Moving_State(TurtleBoss_state new_state)
{
    if (new_state == m_turtle_state) {
        return;
    }

    if (new_state == TURTLEBOSS_WALK) {
        m_state = STA_WALK;
        m_camera_range = 1500;

        float speed = 1.0 + (m_hits + (m_downgrade_count * m_max_hits)) / 15.0;
        if(speed > 15.0) {
            speed = 15.0;
        }
        Set_Animation_Speed(speed);
        Set_Image_Set("walk");
    }
    else if (new_state == TURTLEBOSS_STAND_ANGRY) {
        m_state = STA_STAY;

        Set_Animation(0);
    }
    else if (new_state == TURTLEBOSS_SHELL_STAND) {
        m_state = STA_STAY;
        m_camera_range = 2000;

        Set_Image_Set("shell_stand");
    }
    else if (new_state == TURTLEBOSS_SHELL_RUN) {
        m_counter = 0.0f;
        m_state = STA_RUN;
        m_camera_range = 5000;

        float speed = 3.0 + (m_hits + (m_downgrade_count * m_max_hits)) / 8.0;
        if(speed > 15) {
            speed = 15;
        }
        Set_Animation_Speed(speed);
        Set_Image_Set("shell_run");
    }

    m_turtle_state = new_state;

    Update_Velocity_Max();
    // if in the first part of the turn around animation
    Update_Rotation_Hor();
}

void cTurtleBoss::Update(void)
{
    cEnemy::Update();

    if (!m_valid_update || !Is_In_Range()) {
        return;
    }

    Update_Animation();


    // walking
    if (m_turtle_state == TURTLEBOSS_WALK) {
        m_anim_counter += pFramerate->m_elapsed_ticks;

        // if turn around image
        if (m_curr_img >= m_turn_start && m_curr_img <= m_turn_end) {
            // set normal image back
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
    // standing shell
    else if (m_turtle_state == TURTLEBOSS_SHELL_STAND) {
        m_counter += pFramerate->m_speed_factor;

        // stop waiting
        if (m_counter > 160.0f) {
            // animation
            if (m_counter < 192.0f) {
                if (static_cast<int>(m_counter) % 5 == 1) {
                    Set_Image_Set("shell_active"); // active
                }
                else {
                    Set_Image_Set("shell_front"); // front
                }
            }
            // activate
            else {
                m_counter = 0.0f;
                Stand_Up();
            }
        }

        // slow down
        if (!Is_Float_Equal(m_velx, 0.0f)) {
            Add_Velocity_X(-m_velx * 0.2f);

            if (m_velx < 0.3f && m_velx > -0.3f) {
                m_velx = 0.0f;
            }
        }
    }
    // moving shell
    else if (m_turtle_state == TURTLEBOSS_SHELL_RUN) {
        m_run_time_counter += pFramerate->m_speed_factor;
        // time = seconds since start
        float time = m_run_time_counter / speedfactor_fps;

        if (time > m_shell_time) {
            m_run_time_counter = 0.0f;
            Stand_Up();
        }
        else {
            Update_Velocity();

            if (m_player_counter > 0.0f) {
                m_player_counter -= pFramerate->m_speed_factor;

                if (m_player_counter <= 0.0f) {
                    // do not start collision detection if colliding with alex
                    if (pLevel_Player->m_col_rect.Intersects(m_col_rect)) {
                        m_player_counter = 5.0f;
                    }
                    else {
                        m_player_counter = 0.0f;
                    }
                }
            }
        }
    }
    else if (m_turtle_state == TURTLEBOSS_STAND_ANGRY) {
        m_counter += pFramerate->m_speed_factor;

        // angry
        if (static_cast<int>(m_counter) % 2 == 1) {
            // slowly fade to the color
            cMovingSprite::Set_Color(Color(static_cast<Uint8>(255), 250 - static_cast<Uint8>(m_counter * 1.5f), 250 - static_cast<Uint8>(m_counter * 4)));
        }
        // default
        else {
            cMovingSprite::Set_Color(white);
        }

        // randomize direction
        if ((static_cast<int>(m_counter) / 10) % 2 == 1) {
            if (m_direction == DIR_RIGHT) {
                Set_Direction(DIR_LEFT);
            }
        }
        else {
            if (m_direction == DIR_LEFT) {
                Set_Direction(DIR_RIGHT);
            }
        }

        // slow down
        if (!Is_Float_Equal(m_velx, 0.0f)) {
            Add_Velocity_X(-m_velx * 0.2f);

            if (m_velx < 0.2f && m_velx > -0.2f) {
                m_velx = 0.0f;
            }
        }

        // finished animation
        if (m_counter > 60.0f) {
            m_counter = 0.0f;
            // shell attack sound
            pAudio->Play_Sound("enemy/boss/turtle/shell_attack.ogg");

            Set_Turtle_Moving_State(TURTLEBOSS_SHELL_RUN);
            if(m_walk_start >= 0 && m_shell_stand_start >= 0)
                Col_Move(0.0f, m_images[m_walk_start].m_image->m_col_h - m_images[m_shell_stand_start].m_image->m_col_h, 1, 1);

            if (m_direction == DIR_RIGHT) {
                m_velx = m_velx_max;
            }
            else {
                m_velx = -m_velx_max;
            }

            // throw more fireballs with each downgrade
            Throw_Fireballs(6 + (m_downgrade_count * 2));
        }
    }
}

void cTurtleBoss::Stand_Up(void)
{
    if (m_turtle_state != TURTLEBOSS_SHELL_STAND && m_turtle_state != TURTLEBOSS_SHELL_RUN) {
        return;
    }

    // get space needed to stand up
    float move_y = m_image->m_col_h - ((m_walk_start >= 0) ? m_images[m_walk_start].m_image->m_col_h : 0);

    cObjectCollisionType* col_list = Collision_Check_Relative(0.0f, move_y, 0.0f, 0.0f, COLLIDE_ONLY_BLOCKING);

    // failed to stand up because something is blocking
    if (!col_list->empty()) {
        delete col_list;
        return;
    }

    delete col_list;

    pAudio->Play_Sound("enemy/boss/turtle/power_up.ogg");
    Col_Move(0.0f, move_y, 1, 1);
    Set_Turtle_Moving_State(TURTLEBOSS_WALK);
}

bool cTurtleBoss::Hit_Enemy(cEnemy* enemy) const
{
    // invalid
    if (!enemy) {
        return 0;
    }

    // don't collide with already dead enemies
    if (enemy->m_dead) {
        return 0;
    }

    // hit enemy
    pAudio->Play_Sound(enemy->m_kill_sound);
    pHud_Points->Add_Points(enemy->m_kill_points, m_pos_x + m_image->m_w / 3, m_pos_y - 5.0f, "", static_cast<Uint8>(255), 1);
    enemy->DownGrade(1);
    pLevel_Player->Add_Kill_Multiplier();

    return 1;
}

void cTurtleBoss::Throw_Fireballs(unsigned int amount /* = 6 */)
{
    // start angle
    float ball_angle = -180.0f;
    // move 180 degrees
    float step_size = 180.0f / amount;

    for (unsigned int i = 0; i < amount; i++) {
        // add step size to angle
        ball_angle += step_size;

        cBall* ball = new cBall(m_sprite_manager);
        ball->Set_Pos(m_pos_x + m_col_rect.m_w / 2, m_pos_y + m_col_rect.m_h / 2, 1);
        ball->Set_Origin(m_sprite_array, m_type);
        ball->Set_Ball_Type(FIREBALL_EXPLOSION);
        ball->Set_Velocity_From_Angle(ball_angle, 15);
        if (ball->m_velx > 0.0f) {
            ball->m_direction = DIR_RIGHT;
        }
        else {
            ball->m_direction = DIR_LEFT;
        }

        ball->Col_Move(ball->m_velx * 2.0f, ball->m_vely * 2.0f, 1);
        m_sprite_manager->Add(ball);
    }

    cAnimation_Fireball* anim = new cAnimation_Fireball(m_sprite_manager, m_pos_x + (m_col_rect.m_w / 2), m_pos_y + (m_col_rect.m_h / 3), 10);
    anim->Set_Fading_Speed(0.15f);
    anim->Set_Pos_Z(m_pos_z + 0.000001f);
    pActive_Animation_Manager->Add(anim);
}

void cTurtleBoss::Generate_Stars(unsigned int amount /* = 1 */, float particle_scale /* = 0.4f */) const
{
    // animation
    cParticle_Emitter* anim = new cParticle_Emitter(m_sprite_manager);
    anim->Set_Pos(m_pos_x + (m_col_rect.m_w * 0.5f), m_pos_y + (m_col_rect.m_h * 0.5f));
    anim->Set_Image(pVideo->Get_Surface("animation/particles/star.png"));
    anim->Set_Quota(amount);
    anim->Set_Pos_Z(m_pos_z + 0.000001f);
    anim->Set_Const_Rotation_Z(-6.0f, 12.0f);
    anim->Set_Time_to_Live(1.0f);
    anim->Set_Speed(1.0f, 4.0f);
    anim->Set_Scale(particle_scale, 0.3f);
    anim->Set_Color(orange, Color(static_cast<Uint8>(6), 60, 20, 0));
    anim->Set_Blending(BLEND_ADD);
    anim->Emit();
    pActive_Animation_Manager->Add(anim);
}

void cTurtleBoss::Update_Velocity_Max(void)
{
    if (m_color_type == COL_RED) {
        if (m_turtle_state == TURTLEBOSS_WALK) {
            const int speed_up = m_hits + (m_downgrade_count * m_max_hits);
            m_velx_max = 3.6f + (speed_up * 1.8f);
            m_velx_gain = 0.5f + (speed_up * 0.15f);
        }
        else if (m_turtle_state == TURTLEBOSS_SHELL_STAND) {
            m_velx_max = 0.0f;
            m_velx_gain = 0.0f;
        }
        else if (m_turtle_state == TURTLEBOSS_SHELL_RUN) {
            m_velx_max = 14.0f + (m_downgrade_count * 3.0f);
            m_velx_gain = 0.8f + (m_downgrade_count * 0.18f);
        }
        else if (m_turtle_state == TURTLEBOSS_STAND_ANGRY) {
            m_velx_max = 0.0f;
            m_velx_gain = 0.0f;
        }
    }
}

Col_Valid_Type cTurtleBoss::Validate_Collision(cSprite* obj)
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
            // player is invincible
            if (pLevel_Player->m_invincible) {
                return COL_VTYPE_NOT_VALID;
            }
            // player counter is active
            if (m_turtle_state == TURTLEBOSS_SHELL_RUN && m_player_counter > 0.0f) {
                return COL_VTYPE_NOT_VALID;
            }

            break;
        }
        case TYPE_FLYON: {
            // if walking
            if (m_turtle_state == TURTLEBOSS_WALK) {
                return COL_VTYPE_NOT_VALID;
            }
            // shell
            if (m_turtle_state == TURTLEBOSS_SHELL_STAND || m_turtle_state == TURTLEBOSS_SHELL_RUN) {
                return COL_VTYPE_INTERNAL;
            }

            break;
        }
        case TYPE_ROKKO: {
            return COL_VTYPE_NOT_VALID;
        }
        case TYPE_GEE: {
            if (m_turtle_state == TURTLEBOSS_SHELL_STAND || m_turtle_state == TURTLEBOSS_SHELL_RUN) {
                return COL_VTYPE_INTERNAL;
            }

            return COL_VTYPE_NOT_VALID;
        }
        case TYPE_STATIC_ENEMY: {
            return COL_VTYPE_NOT_VALID;
        }
        case TYPE_SPIKEBALL: {
            if (m_turtle_state == TURTLEBOSS_SHELL_STAND || m_turtle_state == TURTLEBOSS_SHELL_RUN) {
                return COL_VTYPE_INTERNAL;
            }

            return COL_VTYPE_NOT_VALID;
        }
        case TYPE_BALL: {
            return COL_VTYPE_NOT_VALID;
        }
        default: {
            break;
        }
        }

        if (obj->m_sprite_array == ARRAY_ENEMY) {
            // if moving shell don't collide with enemies
            if (m_turtle_state == TURTLEBOSS_SHELL_RUN) {
                return COL_VTYPE_INTERNAL;
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
            if (m_turtle_state == TURTLEBOSS_WALK) {
                return COL_VTYPE_BLOCKING;
            }

            return COL_VTYPE_NOT_VALID;
        }
        default: {
            break;
        }
        }
    }

    return COL_VTYPE_NOT_VALID;
}

void cTurtleBoss::Handle_Collision_Player(cObjectCollision* collision)
{
    if (collision->m_direction == DIR_UNDEFINED || (m_turtle_state == TURTLEBOSS_SHELL_RUN && m_player_counter > 0.0f) || m_state == STA_OBJ_LINKED) {
        return;
    }

    if (collision->m_direction == DIR_TOP && pLevel_Player->m_state != STA_FLY) {
        if (m_turtle_state == TURTLEBOSS_WALK) {
            pHud_Points->Add_Points(250, pLevel_Player->m_pos_x, pLevel_Player->m_pos_y);

            if (m_hits + 1 == m_max_hits) {
                pAudio->Play_Sound("enemy/boss/turtle/big_hit.ogg");
            }
            else {
                pAudio->Play_Sound("enemy/boss/turtle/hit.ogg");
            }
        }
        else if (m_turtle_state == TURTLEBOSS_SHELL_STAND) {
            pHud_Points->Add_Points(100, pLevel_Player->m_pos_x, pLevel_Player->m_pos_y);
            pAudio->Play_Sound("enemy/turtle/shell/hit.ogg");
        }
        else if (m_turtle_state == TURTLEBOSS_SHELL_RUN) {
            pHud_Points->Add_Points(50, pLevel_Player->m_pos_x, pLevel_Player->m_pos_y);
            pAudio->Play_Sound("enemy/turtle/shell/hit.ogg");
        }

        // animation
        cParticle_Emitter* anim = new cParticle_Emitter(m_sprite_manager);
        anim->Set_Pos(m_pos_x + (m_col_rect.m_w / 2), m_pos_y + (m_col_rect.m_h / 2));
        Generate_Hit_Animation(anim);

        anim->Set_Speed(4, 0.8f);
        anim->Set_Scale(0.9f);
        anim->Emit();
        pActive_Animation_Manager->Add(anim);

        DownGrade();

        // if now running
        if (m_turtle_state == TURTLEBOSS_SHELL_RUN) {
            // if player is on the left side
            if ((pLevel_Player->m_col_rect.m_w / 2) + pLevel_Player->m_pos_x < (m_col_rect.m_w / 2) + m_pos_x) {
                Set_Direction(DIR_RIGHT);
                m_velx = m_velx_max;
            }
            // right side
            else {
                Set_Direction(DIR_LEFT);
                m_velx = -m_velx_max;
            }
        }

        pLevel_Player->Action_Jump(1);
    }
    else {
        if (m_turtle_state == TURTLEBOSS_WALK) {
            pLevel_Player->DownGrade_Player();
            Turn_Around(collision->m_direction);
        }
        else if (m_turtle_state == TURTLEBOSS_SHELL_STAND) {
            pAudio->Play_Sound("enemy/turtle/shell/hit.ogg");
            DownGrade();

            cParticle_Emitter* anim = new cParticle_Emitter(m_sprite_manager);
            anim->Set_Image(pVideo->Get_Surface("animation/particles/light.png"));
            anim->Set_Quota(4);
            anim->Set_Pos_Z(m_pos_z + 0.0001f);
            anim->Set_Time_to_Live(0.3f);
            anim->Set_Speed(4, 0.5f);
            anim->Set_Scale(0.8f);
            anim->Set_Fading_Size(1);
            anim->Set_Color(Color(static_cast<Uint8>(254), 200, 100));

            if (collision->m_direction == DIR_RIGHT) {
                anim->Set_Pos(m_pos_x + m_col_pos.m_x + m_col_rect.m_w, m_pos_y + (m_col_rect.m_h / 2));
                anim->Set_Direction_Range(90.0f, 180.0f);
                Set_Direction(DIR_LEFT);
            }
            else if (collision->m_direction == DIR_LEFT) {
                anim->Set_Pos(m_pos_x, m_pos_y + (m_col_rect.m_h / 2));
                anim->Set_Direction_Range(270.0f, 180.0f);
                Set_Direction(DIR_RIGHT);
            }
            else {
                anim->Set_Pos(m_pos_x + (m_col_rect.m_w / 2), m_pos_y + m_col_pos.m_y + m_col_rect.m_h);
                anim->Set_Direction_Range(180.0f, 180.0f);

                // if player is on the left side
                if ((pLevel_Player->m_col_rect.m_w / 2) + pLevel_Player->m_pos_x < (m_col_rect.m_w / 2) + m_pos_x) {
                    Set_Direction(DIR_RIGHT);
                    m_velx = m_velx_max;
                }
                // right side
                else {
                    Set_Direction(DIR_LEFT);
                    m_velx = -m_velx_max;
                }

                // small upwards kick
                if (collision->m_direction == DIR_BOTTOM) {
                    m_vely = -5.0f + (pLevel_Player->m_vely * 0.3f);
                }
            }

            anim->Emit();
            pActive_Animation_Manager->Add(anim);
            m_player_counter = speedfactor_fps * 0.13f;
        }
        else if (m_turtle_state == TURTLEBOSS_SHELL_RUN) {
            // bottom kicks upwards
            if (collision->m_direction == DIR_BOTTOM) {
                // small upwards kick
                m_vely = -5.0f + (pLevel_Player->m_vely * 0.3f);
            }
            // other directions downgrade
            else {
                pLevel_Player->DownGrade_Player();

                if (collision->m_direction == DIR_RIGHT || collision->m_direction == DIR_LEFT) {
                    Turn_Around(collision->m_direction);
                }
            }
        }
    }
}

void cTurtleBoss::Handle_Collision_Enemy(cObjectCollision* collision)
{
    cEnemy* enemy = static_cast<cEnemy*>(m_sprite_manager->Get_Pointer(collision->m_number));

    if (m_turtle_state == TURTLEBOSS_SHELL_STAND) {
        // if able to collide
        if (m_vely < -5.0f) {
            Hit_Enemy(enemy);
        }
    }
    else if (m_turtle_state == TURTLEBOSS_SHELL_RUN) {
        Hit_Enemy(enemy);
    }
    else if (m_turtle_state == TURTLEBOSS_WALK) {
        // turtle shell
        if (enemy->m_type == TYPE_ARMY && enemy->m_state == STA_RUN) {
            Hit_Enemy(enemy);
        }
        else {
            if (collision->m_direction == DIR_RIGHT || collision->m_direction == DIR_LEFT) {
                Turn_Around(collision->m_direction);
            }

            Send_Collision(collision);
        }
    }
}

void cTurtleBoss::Handle_Collision_Massive(cObjectCollision* collision)
{
    if (m_turtle_state == TURTLEBOSS_WALK) {
        Send_Collision(collision);
    }
    else if (m_turtle_state == TURTLEBOSS_SHELL_RUN) {
        if (collision->m_direction == DIR_RIGHT || collision->m_direction == DIR_LEFT) {
            cSprite* col_object = m_sprite_manager->Get_Pointer(collision->m_number);

            // animation
            cParticle_Emitter* anim = NULL;
            if (collision->m_direction == DIR_RIGHT) {
                anim = new cParticle_Emitter(m_sprite_manager);
                anim->Set_Pos(col_object->m_pos_x + col_object->m_col_pos.m_x + 4, m_pos_y + (m_col_rect.m_h / 1.35f), 1);
                anim->Set_Direction_Range(140.0f, 100.0f);
            }
            else {
                anim = new cParticle_Emitter(m_sprite_manager);
                anim->Set_Pos(col_object->m_pos_x + col_object->m_col_pos.m_x + col_object->m_col_rect.m_w - 4, m_pos_y + (m_col_rect.m_h / 1.35f), 1);
                anim->Set_Direction_Range(320.0f, 100.0f);
            }

            anim->Set_Image(pVideo->Get_Surface("animation/particles/smoke.png"));
            anim->Set_Quota(5);
            anim->Set_Pos_Z(col_object->m_pos_z - 0.0001f, 0.0002f);
            anim->Set_Time_to_Live(0.3f);
            anim->Set_Speed(1.0f, 1.0f);
            anim->Set_Scale(0.5f, 0.4f);
            anim->Emit();
            pActive_Animation_Manager->Add(anim);
        }

        // active object collision
        if (collision->m_array == ARRAY_ACTIVE) {
            Send_Collision(collision);
        }
    }
    else if (m_turtle_state == TURTLEBOSS_SHELL_STAND) {
        // if able to collide
        if (m_vely < -5.0f) {
            // active object box collision
            if (collision->m_array == ARRAY_ACTIVE) {
                // get colliding object
                cSprite* col_object = m_sprite_manager->Get_Pointer(collision->m_number);

                if (col_object->m_type == TYPE_BONUS_BOX || col_object->m_type == TYPE_SPIN_BOX) {
                    // get basebox
                    cBaseBox* box = static_cast<cBaseBox*>(col_object);

                    // if useable
                    if (box->m_useable_count != 0) {
                        pHud_Points->Add_Points(50, m_pos_x + m_image->m_w / 3, m_pos_y - 5.0f);
                        Send_Collision(collision);
                        DownGrade(1);
                    }
                }
            }
        }
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

void cTurtleBoss::Editor_Activate(void)
{
    // get window manager
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    // direction
    CEGUI::Combobox* combobox = static_cast<CEGUI::Combobox*>(wmgr.createWindow("TaharezLook/Combobox", "editor_turtle_boss_direction"));
    Editor_Add(UTF8_("Direction"), UTF8_("Starting direction."), combobox, 100, 75);

    combobox->addItem(new CEGUI::ListboxTextItem("left"));
    combobox->addItem(new CEGUI::ListboxTextItem("right"));

    combobox->setText(Get_Direction_Name(m_start_direction));
    combobox->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cTurtleBoss::Editor_Direction_Select, this));

    // max hits
    CEGUI::Editbox* editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "editor_turtle_boss_max_hits"));
    Editor_Add(UTF8_("Hits"), UTF8_("Hits until a downgrade"), editbox, 120);

    editbox->setValidationString("^[+]?\\d*$");
    editbox->setText(int_to_string(m_max_hits));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cTurtleBoss::Editor_Max_Hits_Text_Changed, this));

    // max downgrades
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "editor_turtle_boss_max_downgrade_count"));
    Editor_Add(UTF8_("Downgrades"), UTF8_("Downgrades until death"), editbox, 120);

    editbox->setValidationString("^[+]?\\d*$");
    editbox->setText(int_to_string(m_max_downgrade_count));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cTurtleBoss::Editor_Max_Downgrade_Counts_Text_Changed, this));

    // max shell time
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "editor_turtle_boss_max_shell_time"));
    Editor_Add(UTF8_("Shell Time"), UTF8_("Time running as shell to rise again"), editbox, 200);

    editbox->setValidationString("[+]?[0-9]*\\.?[0-9]*");
    editbox->setText(float_to_string(m_shell_time, 6, 0));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cTurtleBoss::Editor_Shell_Time_Text_Changed, this));

    // level ends if killed
    combobox = static_cast<CEGUI::Combobox*>(wmgr.createWindow("TaharezLook/Combobox", "editor_turtle_boss_level_ends_if_killed"));
    Editor_Add(UTF8_("End Level"), UTF8_("End the level if it is killed."), combobox, 100, 75);

    combobox->addItem(new CEGUI::ListboxTextItem(UTF8_("Enabled")));
    combobox->addItem(new CEGUI::ListboxTextItem(UTF8_("Disabled")));

    if (m_level_ends_if_killed) {
        combobox->setText(UTF8_("Enabled"));
    }
    else {
        combobox->setText(UTF8_("Disabled"));
    }
    combobox->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cTurtleBoss::Editor_Level_Ends_If_Killed, this));

    // init
    Editor_Init();
}

bool cTurtleBoss::Editor_Direction_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    Set_Direction(Get_Direction_Id(item->getText().c_str()), 1);

    return 1;
}

bool cTurtleBoss::Editor_Max_Hits_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Max_Hits(string_to_int(str_text));

    return 1;
}

bool cTurtleBoss::Editor_Max_Downgrade_Counts_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Max_Downgrade_Counts(string_to_int(str_text));

    return 1;
}

bool cTurtleBoss::Editor_Shell_Time_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Shell_Time(string_to_float(str_text));

    return 1;
}

bool cTurtleBoss::Editor_Level_Ends_If_Killed(const CEGUI::EventArgs& event)
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

std::string cTurtleBoss::Create_Name(void) const
{
    std::string name = m_name; // dup
    name += " " + Get_Direction_Name(m_start_direction);
    return name;
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

int cTurtleBoss::Get_Downgrade_Count()
{
    return m_downgrade_count;
}

int cTurtleBoss::Get_Max_Downgrade_Count()
{
    return m_max_downgrade_count;
}

float cTurtleBoss::Get_Shell_Time()
{
    return m_shell_time;
}

bool cTurtleBoss::Get_Level_Ends_If_Killed()
{
    return m_level_ends_if_killed;
}

} // namespace TSC

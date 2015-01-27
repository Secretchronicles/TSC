/***************************************************************************
 * army.cpp  -  army enemy class
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

#include "../enemies/army.hpp"
#include "../core/game_core.hpp"
#include "../objects/box.hpp"
#include "../video/animation.hpp"
#include "../level/level_player.hpp"
#include "../level/level.hpp"
#include "../gui/hud.hpp"
#include "../video/gl_surface.hpp"
#include "../user/savegame.hpp"
#include "../core/sprite_manager.hpp"
#include "../core/i18n.hpp"
#include "../core/global_basic.hpp"

using namespace std;

namespace TSC {

/* *** *** *** *** *** *** cArmy *** *** *** *** *** *** *** *** *** *** *** */

cArmy::cArmy(cSprite_Manager* sprite_manager)
    : cEnemy(sprite_manager)
{
    cArmy::Init();
}

cArmy::cArmy(XmlAttributes& attributes, cSprite_Manager* sprite_manager)
    : cEnemy(sprite_manager)
{
    cArmy::Init();

    // position
    Set_Pos(string_to_float(attributes["posx"]), string_to_float(attributes["posy"]), true);

    // direction
    Set_Direction(Get_Direction_Id(attributes.fetch("direction", Get_Direction_Name(m_start_direction))), true);

    // color
    Set_Color(static_cast<DefaultColor>(Get_Color_Id(attributes.fetch("color", Get_Color_Name(m_color_type)))));
}


cArmy::~cArmy(void)
{
    //
}

void cArmy::Init(void)
{
    m_type = TYPE_ARMY;
    m_name = "Armadillo";
    m_pos_z = 0.091f;
    m_gravity_max = 24.0f;

    m_color_type = COL_DEFAULT;
    Set_Color(COL_RED);

    m_player_counter = 0.0f;
    m_army_state = ARMY_DEAD;
    Set_Army_Moving_State(ARMY_WALK);

    Set_Direction(DIR_RIGHT, 1);

    m_kill_sound = "stomp_4.ogg";
}

cArmy* cArmy::Copy(void) const
{
    cArmy* army = new cArmy(m_sprite_manager);
    army->Set_Pos(m_start_pos_x, m_start_pos_y);
    army->Set_Direction(m_start_direction, 1);
    army->Set_Color(m_color_type);
    return army;
}

std::string cArmy::Get_XML_Type_Name()
{
    return "army";
}

xmlpp::Element* cArmy::Save_To_XML_Node(xmlpp::Element* p_element)
{
    xmlpp::Element* p_node = cEnemy::Save_To_XML_Node(p_element);

    Add_Property(p_node, "posx", static_cast<int>(m_start_pos_x));
    Add_Property(p_node, "posy", static_cast<int>(m_start_pos_y));
    Add_Property(p_node, "color", Get_Color_Name(m_color_type));
    Add_Property(p_node, "direction", Get_Direction_Name(m_start_direction));

    return p_node;
}

void cArmy::Load_From_Savegame(cSave_Level_Object* save_object)
{
    cEnemy::Load_From_Savegame(save_object);

    // army_state
    if (save_object->exists("army_state")) {
        Army_state mov_state = static_cast<Army_state>(string_to_int(save_object->Get_Value("army_state")));

        //Note: if the army object is in a linked status (STA_OBJ_LINKED), we are holding it and do not want to call
        //Set_Army_Moving_State, since it will destroy this linked state
        if (m_state != STA_OBJ_LINKED && (mov_state == ARMY_SHELL_STAND || mov_state == ARMY_SHELL_RUN)) {
            Set_Army_Moving_State(mov_state);
            // set shell image without position changes
            if(m_shell_start >= 0)
                cSprite::Set_Image(m_images[m_shell_start].m_image);
        }
    }

    if (m_army_state != ARMY_SHELL_RUN) {
        Update_Rotation_Hor();
    }
}

cSave_Level_Object* cArmy::Save_To_Savegame(void)
{
    cSave_Level_Object* save_object = cEnemy::Save_To_Savegame();

    // army_state ( only save if needed )
    if (m_army_state != ARMY_WALK) {
        save_object->m_properties.push_back(cSave_Level_Object_Property("army_state", int_to_string(m_army_state)));
    }

    return save_object;
}

void cArmy::Set_Direction(const ObjectDirection dir, bool new_start_direction /* = 0 */)
{
    if (dir != DIR_RIGHT && dir != DIR_LEFT) {
        cerr << "Warning : Unknown Army direction set " << Get_Direction_Name(dir) << endl;
        return;
    }

    cEnemy::Set_Direction(dir, new_start_direction);

    if (m_army_state != ARMY_SHELL_RUN) {
        Update_Rotation_Hor(new_start_direction);
    }
}

void cArmy::Set_Color(DefaultColor col)
{
    // already set
    if (m_color_type == col) {
        return;
    }

    m_color_type = col;

    std::string filename_dir;

    if (m_color_type == COL_RED) {
        filename_dir = "red";
        m_kill_points = 50;
    }
    else if (m_color_type == COL_GREEN) {
        filename_dir = "green";
        m_kill_points = 150;
    }
    // unknown color
    else {
        cerr << "Error : Unknown Army color : " << m_color_type << endl;
    }

    Update_Velocity_Max();

    Clear_Images();

    // FIXME: Red armadillo currently has not enough images!
    // Hence many images are duplicate for the red armadillo.

    Add_Image_Set("walk", "enemy/army/" + filename_dir + "/walk.imgset", 0, &m_walk_start);
    Add_Image_Set("turn", "enemy/army/" + filename_dir + "/turn.imgset", 0, &m_turn_start, &m_turn_end);
    Add_Image_Set("shell", "enemy/army/" + filename_dir + "/shell.imgset", 0, &m_shell_start);
    Add_Image_Set("shell_look", "enemy/army/" + filename_dir + "/shell_look.imgset");
    Add_Image_Set("roll", "enemy/army/" + filename_dir + "/roll.imgset");

    Set_Image_Set("walk", true);
}

void cArmy::Turn_Around(ObjectDirection col_dir /* = DIR_UNDEFINED */)
{
    cEnemy::Turn_Around(col_dir);

    if (m_army_state == ARMY_WALK) {
        m_velx *= 0.5f;
        // hack : disable turn image
        Set_Image_Set("turn");
    }

    if (m_army_state != ARMY_SHELL_RUN) {
        Update_Rotation_Hor();
    }
}

void cArmy::DownGrade(bool force /* = 0 */)
{
    if (!force) {
        // normal walking
        if (m_army_state == ARMY_WALK) {
            Set_Army_Moving_State(ARMY_SHELL_STAND);
            if(m_walk_start >= 0 && m_shell_start >= 0)
                Col_Move(0, m_images[m_walk_start].m_image->m_col_h - m_images[m_shell_start].m_image->m_col_h, 1, 1);
            m_counter = 0.0f;
        }
        // staying
        else if (m_army_state == ARMY_SHELL_STAND) {
            Set_Army_Moving_State(ARMY_SHELL_RUN);
            m_counter = 0.0f;
        }
        // running shell
        else if (m_army_state == ARMY_SHELL_RUN) {
            Set_Army_Moving_State(ARMY_SHELL_STAND);
        }
    }
    // falling death
    else {
        m_counter = 8.0f;
        Set_Dead(1);
        m_massive_type = MASS_PASSIVE;
        m_velx = 0.0f;
        m_vely = 0.0f;

        if (m_army_state == ARMY_WALK) {
            if(m_walk_start >= 0 && m_shell_start >= 0)
                Move(0.0f, m_images[m_walk_start].m_image->m_h - m_images[m_shell_start].m_image->m_h, 1);
        }

        Set_Image_Set("shell");
    }
}

void cArmy::Update_Normal_Dying()
{
    // if not below the ground : fall
    if (m_col_rect.m_y < pActive_Camera->m_limit_rect.m_y) {
        float speed_y = m_dying_counter;

        // first a little bit upwards
        if (speed_y < 10.0f) {
            speed_y *= -1;
        }
        // limit falling speed
        else if (speed_y > 25.0f) {
            speed_y = 25.0f;
        }

        float speed_x = pFramerate->m_speed_factor;

        if (m_direction == DIR_LEFT) {
            speed_x *= -1;
        }

        Add_Rotation_Z(speed_x);
        Move(speed_x * 15.0f, speed_y);
    }
    // if below disable
    else {
        Set_Active(0);
        m_rot_z = 0.0f;
        m_state = STA_STAY;
        m_army_state = ARMY_DEAD;
    }
}

void cArmy::Update_Instant_Dying()
{
    Update_Normal_Dying();
}

void cArmy::Set_Army_Moving_State(Army_state new_state)
{
    if (new_state == m_army_state) {
        return;
    }

    if (new_state == ARMY_WALK) {
        m_state = STA_WALK;
        m_camera_range = 1500;
        Set_Rotation_Z(0.0f);

        Set_Image_Set("walk");
    }
    else if (new_state == ARMY_SHELL_STAND) {
        m_state = STA_STAY;
        m_camera_range = 2000;
        Set_Rotation_Z(0.0f);

        Set_Image_Set("shell");
    }
    else if (new_state == ARMY_SHELL_RUN) {
        m_state = STA_RUN;
        m_camera_range = 5000;
        Set_Rotation_Z(0.0f);

        Set_Image_Set("shell");
    }

    m_army_state = new_state;

    Update_Velocity_Max();
    Update_Rotation_Hor();
}

void cArmy::Update(void)
{
    cEnemy::Update();

    if (!m_valid_update || !Is_In_Range()) {
        return;
    }

    Update_Animation();

    // walking
    if (m_army_state == ARMY_WALK) {
        // if turn around image
        if (m_curr_img >= m_turn_start && m_curr_img <= m_turn_end) {
            m_anim_counter += pFramerate->m_elapsed_ticks;

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
    else if (m_army_state == ARMY_SHELL_STAND) {
        m_counter += pFramerate->m_speed_factor;

        // stop waiting
        if (m_counter > 160.0f) {
            // animation
            if (m_counter < 192.0f) {
                if (static_cast<int>(m_counter) % 5 == 1) {
                    Set_Image_Set("shell_look");
                }
                else {
                    Set_Image_Set("shell");
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
    else if (m_army_state == ARMY_SHELL_RUN) {
        Update_Velocity();
    }

    // shell
    if (m_army_state == ARMY_SHELL_STAND || m_army_state == ARMY_SHELL_RUN) {
        // update rotation
        if (!Is_Float_Equal(m_velx, 0.0f)) {
            Add_Rotation_Z((m_velx / (m_image->m_w * 0.009f)) * pFramerate->m_speed_factor);
        }
    }

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

void cArmy::Stand_Up(void)
{
    if (m_army_state != ARMY_SHELL_STAND && m_army_state != ARMY_SHELL_RUN) {
        return;
    }

    // get space needed to stand up
    float move_y = m_image->m_col_h - (m_walk_start >= 0 ? m_images[m_walk_start].m_image->m_col_h : 0);

    cObjectCollisionType* col_list = Collision_Check_Relative(0.0f, move_y, 0.0f, 0.0f, COLLIDE_ONLY_BLOCKING);

    // failed to stand up because something is blocking
    if (!col_list->empty()) {
        delete col_list;
        return;
    }

    delete col_list;

    pAudio->Play_Sound("enemy/army/stand_up.wav");
    Col_Move(0.0f, move_y, 1, 1);
    Set_Army_Moving_State(ARMY_WALK);
}

bool cArmy::Hit_Enemy(cEnemy* enemy)
{
    // invalid
    if (!enemy) {
        return 0;
    }

    // don't collide with already dead enemies
    if (enemy->m_dead) {
        return 0;
    }
    // shell can not hit it
    if (!enemy->m_can_be_hit_from_shell) {
        // enemies that only hit us
        if (enemy->m_type == TYPE_SPIKEBALL) {
            DownGrade(1);
        }

        return 0;
    }

    // if red shell check if colliding army is green and also running
    if (m_color_type == COL_RED && enemy->m_type == TYPE_ARMY) {
        cArmy* army = static_cast<cArmy*>(enemy);

        // red shell can't kill the green shell
        if (army->m_color_type == COL_GREEN && army->m_state == STA_RUN) {
            return 0;
        }
    }

    // hit enemy
    pAudio->Play_Sound(enemy->m_kill_sound);
    pHud_Points->Add_Points(enemy->m_kill_points, m_pos_x + m_image->m_w / 3, m_pos_y - 5.0f, "", static_cast<Uint8>(255), 1);
    enemy->DownGrade(1);
    pLevel_Player->Add_Kill_Multiplier();

    // enemies that also hit us
    if (enemy->m_type == TYPE_SPIKA) {
        DownGrade(1);
    }

    return 1;
}

void cArmy::Update_Velocity_Max(void)
{
    if (m_army_state == ARMY_WALK) {
        if (m_color_type == COL_RED) {
            m_velx_max = 3.6f;
            m_velx_gain = 0.2f;
        }
        else if (m_color_type == COL_GREEN) {
            m_velx_max = 4.5f;
            m_velx_gain = 0.3f;
        }
    }
    else if (m_army_state == ARMY_SHELL_STAND) {
        m_velx_max = 0.0f;
        m_velx_gain = 0.0f;
    }
    else if (m_army_state == ARMY_SHELL_RUN) {
        if (m_color_type == COL_RED) {
            m_velx_max = 14.0f;
            m_velx_gain = 0.8f;
        }
        else if (m_color_type == COL_GREEN) {
            m_velx_max = 17.0f;
            m_velx_gain = 1.2f;
        }
    }
}

bool cArmy::Is_Update_Valid()
{
    if (m_dead || m_freeze_counter || m_state == STA_OBJ_LINKED) {
        return false;
    }

    return true;
}

Col_Valid_Type cArmy::Validate_Collision(cSprite* obj)
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
            if (m_army_state == ARMY_SHELL_RUN && m_player_counter > 0.0f) {
                return COL_VTYPE_NOT_VALID;
            }

            break;
        }
        case TYPE_FLYON: {
            // if walking
            if (m_army_state == ARMY_WALK) {
                return COL_VTYPE_NOT_VALID;
            }
            // shell
            else if (m_army_state == ARMY_SHELL_STAND || m_army_state == ARMY_SHELL_RUN) {
                return COL_VTYPE_INTERNAL;
            }

            break;
        }
        case TYPE_ROKKO: {
            return COL_VTYPE_NOT_VALID;
        }
        case TYPE_GEE: {
            if (m_army_state == ARMY_SHELL_STAND || m_army_state == ARMY_SHELL_RUN) {
                return COL_VTYPE_INTERNAL;
            }

            return COL_VTYPE_NOT_VALID;
        }
        case TYPE_BALL: {
            return COL_VTYPE_BLOCKING;
        }
        default: {
            break;
        }
        }

        if (obj->m_sprite_array == ARRAY_ENEMY) {
            // if moving shell don't collide with enemies
            if (m_army_state == ARMY_SHELL_RUN) {
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
            if (m_army_state == ARMY_WALK) {
                return COL_VTYPE_BLOCKING;
            }

            return COL_VTYPE_NOT_VALID;
        }
        case TYPE_BONUS_BOX:
        case TYPE_SPIN_BOX:
        case TYPE_TEXT_BOX: {
            // if shell
            if (m_army_state == ARMY_SHELL_STAND || m_army_state == ARMY_SHELL_RUN) {
                cBaseBox* box = static_cast<cBaseBox*>(obj);

                // invisible semi massive
                if (box->m_box_invisible == BOX_INVISIBLE_SEMI_MASSIVE) {
                    // if moving upwards and the object is on top
                    if (m_vely <= 0.0f && obj->Is_On_Top(this)) {
                        return COL_VTYPE_BLOCKING;
                    }
                }
            }

            break;
        }
        default: {
            break;
        }
        }
    }

    return COL_VTYPE_NOT_VALID;
}

void cArmy::Handle_Collision_Player(cObjectCollision* collision)
{
    if (collision->m_direction == DIR_UNDEFINED || (m_army_state == ARMY_SHELL_RUN && m_player_counter > 0.0f)) {
        return;
    }

    if (m_state == STA_OBJ_LINKED) {
        if (m_freeze_counter) {
            DownGrade();
        }

        return;
    }


    if (collision->m_direction == DIR_TOP && pLevel_Player->m_state != STA_FLY) {
        if (m_army_state == ARMY_WALK) {
            pHud_Points->Add_Points(25, m_pos_x, m_pos_y - 5.0f);
            pAudio->Play_Sound("enemy/army/hit.ogg");
        }
        else if (m_army_state == ARMY_SHELL_STAND) {
            pHud_Points->Add_Points(10, m_pos_x, m_pos_y - 5.0f);
            pAudio->Play_Sound("enemy/army/shell/hit.ogg");
        }
        else if (m_army_state == ARMY_SHELL_RUN) {
            pHud_Points->Add_Points(5, m_pos_x, m_pos_y - 5.0f);
            pAudio->Play_Sound("enemy/army/shell/hit.ogg");
        }

        // animation
        cParticle_Emitter* anim = new cParticle_Emitter(m_sprite_manager);
        Generate_Hit_Animation(anim);
        anim->Set_Speed(4.0f, 0.8f);
        anim->Set_Scale(0.6f);
        anim->Emit();
        pActive_Animation_Manager->Add(anim);

        DownGrade();

        // if now running
        if (m_army_state == ARMY_SHELL_RUN) {
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
        if (m_army_state == ARMY_WALK) {
            pLevel_Player->DownGrade_Player();

            if (collision->m_direction == DIR_RIGHT || collision->m_direction == DIR_LEFT) {
                Turn_Around(collision->m_direction);
            }
        }
        else if (m_army_state == ARMY_SHELL_STAND) {
            pAudio->Play_Sound("enemy/army/shell/hit.ogg");
            DownGrade();

            cParticle_Emitter* anim = new cParticle_Emitter(m_sprite_manager);
            anim->Set_Image(pVideo->Get_Package_Surface("animation/particles/light.png"));
            anim->Set_Quota(4);
            anim->Set_Pos_Z(m_pos_z + 0.0001f);
            anim->Set_Time_to_Live(0.3f);
            anim->Set_Speed(4.0f, 0.5f);
            anim->Set_Scale(0.6f);
            anim->Set_Fading_Size(1);
            anim->Set_Color(Color(static_cast<Uint8>(254), 200, 100));

            if (collision->m_direction == DIR_RIGHT) {
                anim->Set_Pos(m_pos_x + m_col_pos.m_x + m_col_rect.m_w, m_pos_y + (m_col_rect.m_h / 2));
                anim->Set_Direction_Range(90.0f, 180.0f);
                Set_Direction(DIR_LEFT);
                m_velx = -m_velx_max;
            }
            else if (collision->m_direction == DIR_LEFT) {
                anim->Set_Pos(m_pos_x, m_pos_y + (m_col_rect.m_h / 2));
                anim->Set_Direction_Range(270.0f, 180.0f);
                Set_Direction(DIR_RIGHT);
                m_velx = m_velx_max;
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
        else if (m_army_state == ARMY_SHELL_RUN) {
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

void cArmy::Handle_Collision_Enemy(cObjectCollision* collision)
{
    if (m_army_state == ARMY_SHELL_STAND) {
        cEnemy* enemy = static_cast<cEnemy*>(m_sprite_manager->Get_Pointer(collision->m_number));

        // if able to collide
        if (m_state == STA_OBJ_LINKED || m_vely < -5.0f) {
            if (Hit_Enemy(enemy)) {
                DownGrade(1);
            }
        }
    }
    else if (m_army_state == ARMY_SHELL_RUN) {
        cEnemy* enemy = static_cast<cEnemy*>(m_sprite_manager->Get_Pointer(collision->m_number));

        if (Hit_Enemy(enemy)) {
            // create animation
            cParticle_Emitter* anim = new cParticle_Emitter(m_sprite_manager);
            anim->Set_Emitter_Rect(m_col_rect.m_x + (m_col_rect.m_w * 0.2f), m_col_rect.m_y + (m_col_rect.m_h * 0.2f), m_col_rect.m_w * 0.6f, m_col_rect.m_h * 0.8f);
            anim->Set_Image(pVideo->Get_Package_Surface("animation/particles/light.png"));
            anim->Set_Quota(5);
            anim->Set_Pos_Z(m_pos_z + 0.000001f);
            anim->Set_Time_to_Live(0.3f);
            anim->Set_Speed(1.2f, 0.8f);
            anim->Set_Scale(0.7f);
            anim->Set_Fading_Alpha(1);
            anim->Set_Blending(BLEND_ADD);

            if (collision->m_direction == DIR_RIGHT) {
                anim->Set_Direction_Range(0.0f);
            }
            else if (collision->m_direction == DIR_LEFT) {
                anim->Set_Direction_Range(180.0f);
            }

            anim->Emit();
            pActive_Animation_Manager->Add(anim);
        }
    }
    else if (m_army_state == ARMY_WALK) {
        if (collision->m_direction == DIR_RIGHT || collision->m_direction == DIR_LEFT) {
            Turn_Around(collision->m_direction);
        }

        Send_Collision(collision);
    }
}

void cArmy::Handle_Collision_Massive(cObjectCollision* collision)
{
    // get colliding object
    cSprite* col_object = m_sprite_manager->Get_Pointer(collision->m_number);

    if (m_army_state == ARMY_WALK) {
        Send_Collision(collision);
    }
    else if (m_army_state == ARMY_SHELL_RUN) {
        if (collision->m_direction == DIR_RIGHT || collision->m_direction == DIR_LEFT) {
            // animation
            cParticle_Emitter* anim = NULL;
            if (collision->m_direction == DIR_RIGHT) {
                anim = new cParticle_Emitter(m_sprite_manager);
                anim->Set_Pos(col_object->m_pos_x + col_object->m_col_pos.m_x + 4.0f, m_pos_y + (m_col_rect.m_h / 1.35f), 1);
                anim->Set_Direction_Range(140.0f, 100.0f);
            }
            else {
                anim = new cParticle_Emitter(m_sprite_manager);
                anim->Set_Pos(col_object->m_pos_x + col_object->m_col_pos.m_x + col_object->m_col_rect.m_w - 4.0f, m_pos_y + (m_col_rect.m_h / 1.35f), 1);
                anim->Set_Direction_Range(320.0f, 100.0f);
            }

            anim->Set_Image(pVideo->Get_Package_Surface("animation/particles/smoke.png"));
            anim->Set_Quota(5);
            anim->Set_Pos_Z(col_object->m_pos_z - 0.0001f, 0.0002f);
            anim->Set_Time_to_Live(0.2f, 0.2f);
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
    else if (m_army_state == ARMY_SHELL_STAND) {
        // if able to collide
        if (m_state == STA_OBJ_LINKED || m_vely < -5.0f) {
            // active object box collision
            if (collision->m_array == ARRAY_ACTIVE) {
                if (col_object->m_type == TYPE_BONUS_BOX || col_object->m_type == TYPE_SPIN_BOX) {
                    // get basebox
                    cBaseBox* box = static_cast<cBaseBox*>(col_object);

                    // if useable
                    if (box->m_useable_count != 0) {
                        Send_Collision(collision);

                        if (m_state == STA_OBJ_LINKED) {
                            DownGrade(1);
                        }
                        else {
                            m_vely = 0.0f;
                        }
                    }
                }
            }
        }
    }

    if (m_state == STA_OBJ_LINKED && m_freeze_counter <= 0.0f) {
        return;
    }

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
        if (m_freeze_counter) {
            m_velx = 0.0f;
        }
        else {
            Turn_Around(collision->m_direction);
        }
    }
}

void cArmy::Handle_Collision_Box(ObjectDirection cdirection, GL_rect* r2)
{
    pAudio->Play_Sound(m_kill_sound);
    pHud_Points->Add_Points(m_kill_points, m_pos_x, m_pos_y - 5.0f, "", static_cast<Uint8>(255), 1);
    pLevel_Player->Add_Kill_Multiplier();
    DownGrade(true);
}

void cArmy::Editor_Activate(void)
{
    // get window manager
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    // direction
    CEGUI::Combobox* combobox = static_cast<CEGUI::Combobox*>(wmgr.createWindow("TaharezLook/Combobox", "editor_army_direction"));
    Editor_Add(UTF8_("Direction"), UTF8_("Starting direction."), combobox, 100, 75);

    combobox->addItem(new CEGUI::ListboxTextItem("left"));
    combobox->addItem(new CEGUI::ListboxTextItem("right"));

    combobox->setText(Get_Direction_Name(m_start_direction));
    combobox->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cArmy::Editor_Direction_Select, this));

    // init
    Editor_Init();
}

bool cArmy::Editor_Direction_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    Set_Direction(Get_Direction_Id(item->getText().c_str()), 1);

    return 1;
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

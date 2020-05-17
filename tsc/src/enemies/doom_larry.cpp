/***************************************************************************
 * doom_larry.cpp - Run or die.
 *
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

#include "doom_larry.hpp"
#include "../core/xml_attributes.hpp"
#include "../core/game_core.hpp"
#include "../core/sprite_manager.hpp"
#include "../core/i18n.hpp"
#include "../level/level_player.hpp"
#include "../level/level.hpp"
#include "../video/animation.hpp"
#include "../objects/box.hpp"
#include "../objects/crate.hpp"

using namespace TSC;

cDoomLarry::cDoomLarry(cSprite_Manager* p_sprite_manager)
    : cLarry(p_sprite_manager)
{
    Init();
}

cDoomLarry::cDoomLarry(XmlAttributes& attributes, cSprite_Manager* p_sprite_manager)
    : cLarry(p_sprite_manager)
{
    Init();

    Set_Pos(attributes.fetch<float>("posx", 0), attributes.fetch<float>("posy", 0), true);
    Set_Direction(Get_Direction_Id(attributes.fetch("direction", Get_Direction_Name(m_start_direction))));
}

cDoomLarry::~cDoomLarry()
{
    //
}

void cDoomLarry::Init()
{
    //The Doom_Larry constructor will have called the Larry constructor, creating an image set full of grey Larry
    //images that we do not want
    //Remove them so that they can be replaced with Doom Larry images.  If Init() is called again, this clear will
    //also be needed again.
    Clear_Images(true, true);

    m_type = TYPE_DOOM_LARRY;
    m_name = "Doom Larry";
    m_pos_z = 0.09f;
    m_gravity_max = 29.0f;

    m_kill_points = 300;
    m_fire_resistant = false;
    m_ice_resistance = 1.0f;
    m_can_be_hit_from_shell = true;
    m_explosion_counter = 0.0f;
    m_kill_sound = "enemy/larry/red/die.ogg";

    Add_Image_Set("walk", "enemy/larry/red/walk.imgset");
    Add_Image_Set("walk_turn", "enemy/larry/red/walk_turn.imgset", 0, &m_walk_turn_start, &m_walk_turn_end);
    Add_Image_Set("run", "enemy/larry/red/run.imgset");
    Add_Image_Set("run_turn", "enemy/larry/red/run_turn.imgset", 0, &m_run_turn_start, &m_run_turn_end);
    Add_Image_Set("action", "enemy/larry/red/action.imgset", 0, &m_action_start, &m_action_end);

    m_state = STA_STAY;         //This triggers logic in method Set_Moving_State to reset the start image after having called method Clear_Images above
    Set_Moving_State(STA_WALK);
    Set_Direction(DIR_RIGHT);
}

cDoomLarry* cDoomLarry::Copy() const
{
    cDoomLarry* p_dl = new cDoomLarry(m_sprite_manager);
    p_dl->Set_Pos(m_start_pos_x, m_start_pos_y);
    p_dl->Set_Direction(m_start_direction);
    return p_dl;
}

std::string cDoomLarry::Get_XML_Type_Name()
{
    return "doom_larry";
}

void cDoomLarry::Update()
{
    cEnemy::Update();
    if (!m_valid_update || !Is_In_Range())
        return;

    Update_Animation();
    Update_Velocity();

    if (m_state == STA_RUN) {
        m_explosion_counter += pFramerate->m_speed_factor;

        if (m_explosion_counter > 200.0f)
            DownGrade(true);
    }

    // If currently turning ’round
    if ((m_curr_img >= m_walk_turn_start && m_curr_img <= m_walk_turn_end) ||
        (m_curr_img >= m_run_turn_start && m_curr_img <= m_run_turn_end)) {
        m_anim_counter += pFramerate->m_elapsed_ticks;

        if ((m_state == STA_WALK && m_anim_counter >= 600) || /* normal walk */
                (m_state == STA_RUN && m_anim_counter >= 100)) { /* fusing */

            if (m_state == STA_WALK) {
                Set_Image_Set("walk");
                m_velx_max = 1.5f;
            } else if (m_state == STA_RUN) {
                Set_Image_Set("run");
                m_velx_max = 3.0f;
            }
            else {
                throw (TSCError("Invalid doom larry walking state!"));
            }

            Update_Rotation_Hor();
        }
    }
    else if (m_curr_img >= m_action_start && m_curr_img <= m_action_end) { // if currently activating
        m_anim_counter += pFramerate->m_elapsed_ticks;

        // back to normal animation
        if (m_anim_counter >= 600) {
            Set_Image_Set("run");
            m_velx_max = 3.0f;

            Update_Rotation_Hor();
        }
    }
}

void cDoomLarry::Kill_Objects_in_Explosion_Range()
{
    GL_Circle explosion_radius(m_pos_x + m_rect.m_w / 2.0,
                               m_pos_y + m_rect.m_h / 2.0,
                               200.0f);

    // Find all objects we can destroy (note that if another
    // Larry is inside this radius, he will explode likewise
    // and check himself for further destructions in his radius)
    cSprite_List objects;
    pActive_Level->m_sprite_manager->Get_Colliding_Objects(objects, explosion_radius, true, this);

    // DESTROY ’EM ALL.
    // Alex WILL be killed instantly.
    cSprite_List::iterator iter;
    for (iter=objects.begin(); iter != objects.end(); iter++) {
        cSprite* p_obj = *iter;
        cEnemy* p_enemy = NULL;
        cBaseBox* p_box = NULL;
        cCrate* p_crate = NULL;

        if (p_obj->m_type == TYPE_PLAYER) // This means p_obj == pLevel_Player
            pLevel_Player->DownGrade_Player(true, true);
        else if ((p_enemy = dynamic_cast<cEnemy*>(p_obj)))
            p_enemy->DownGrade(true);
        else if ((p_box = dynamic_cast<cBaseBox*>(p_obj)))
            p_box->Activate();
        else if ((p_crate = dynamic_cast<cCrate*>(p_obj)))
            p_crate->Smash();
    }
}

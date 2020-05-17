/***************************************************************************
 * camera.cpp  -  class for handling screen camera movement
 *
 * Copyright © 2006 - 2011 Florian Richter
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

#include "../core/camera.hpp"
#include "../core/game_core.hpp"
#include "../level/level_player.hpp"
#include "../core/framerate.hpp"
#include "../input/mouse.hpp"
#include "../overworld/world_manager.hpp"
#include "../level/level.hpp"
#include "../overworld/overworld.hpp"
#include "../core/main.hpp"
#include "../audio/audio.hpp"
#include "../gui/menu.hpp"

namespace TSC {

/* *** *** *** *** *** *** *** cCamera *** *** *** *** *** *** *** *** *** *** */

const GL_rect cCamera::m_default_limits = GL_rect(0, 0, 20000, -4000);

cCamera::cCamera(cSprite_Manager* sprite_manager)
{
    m_sprite_manager = sprite_manager;
    m_x = 0.0f;
    m_y = -game_res_h;

    m_x_offset = 0.0f;
    m_y_offset = 0.0f;
    m_hor_offset_speed = 0.3f;
    m_ver_offset_speed = 0.2f;

    m_fixed_hor_vel = 0.0f;

    // default camera limit
    Reset_Limits();
}

cCamera::~cCamera(void)
{

}

void cCamera::Set_Sprite_Manager(cSprite_Manager* sprite_manager)
{
    m_sprite_manager = sprite_manager;
}

void cCamera::Set_Pos(float x, float y)
{
    // level mode
    if (Game_Mode == MODE_LEVEL || Game_Mode == MODE_OVERWORLD) {
        if (!editor_enabled) {
            // camera offset
            x += m_x_offset;
            y += m_y_offset;

            // camera limits
            Update_Limit(x, y);
        }
    }

    m_x = x;
    m_y = y;

    Update_Position();
}

void cCamera::Set_Pos_X(float x)
{
    // level mode
    if (Game_Mode == MODE_LEVEL || Game_Mode == MODE_OVERWORLD) {
        if (!editor_enabled) {
            // camera offset
            x += m_x_offset;

            // camera limits
            Update_Limit_X(x);
        }
    }

    m_x = x;

    Update_Position();
}

void cCamera::Set_Pos_Y(float y)
{
    // level mode
    if (Game_Mode == MODE_LEVEL || Game_Mode == MODE_OVERWORLD) {
        if (!editor_enabled) {
            // camera offset
            y += m_y_offset;

            // camera limits
            Update_Limit_Y(y);
        }
    }

    m_y = y;

    Update_Position();
}

void cCamera::Reset_Pos(void)
{
    m_x = m_limit_rect.m_x;
    m_y = m_limit_rect.m_y - static_cast<float>(game_res_h);

    m_x_offset = 0.0f;
    m_y_offset = 0.0f;

    Update_Position();
}

void cCamera::Move(const float move_x, const float move_y)
{
    if ((Game_Mode == MODE_LEVEL || Game_Mode == MODE_OVERWORLD) && !editor_enabled) {
        Set_Pos(m_x + move_x - m_x_offset, m_y + move_y - m_y_offset);
    }
    else {
        Set_Pos(m_x + move_x, m_y + move_y);
    }
}

bool cCamera::Move_to_Position_Gradually(const float pos_x, const float pos_y, const unsigned int frames /* = 200 */)
{
    for (float i = 0.0f; i < frames; i += pFramerate->m_speed_factor) {
        if (Step_to_Position_Gradually(pos_x, pos_y) == 0) {
            return 0;
        }

        // keep particles on screen
        for (cSprite_List::iterator itr = m_sprite_manager->objects.begin(); itr != m_sprite_manager->objects.end(); ++itr) {
            cSprite* obj = (*itr);

            if (obj->m_type == TYPE_PARTICLE_EMITTER) {
                cParticle_Emitter* emitter = static_cast<cParticle_Emitter*>(obj);
                emitter->Update_Position();
            }
        }

        // update audio
        pAudio->Update();
        // draw
        Draw_Game();

        pVideo->Render();
        pFramerate->Update();
    }

    return 1;
}

bool cCamera::Step_to_Position_Gradually(const float pos_x, const float pos_y)
{
    // limit to nearest possible position
    float pos_x_final = pos_x + m_x_offset;
    float pos_y_final = pos_y + m_y_offset;
    Update_Limit(pos_x_final, pos_y_final);

    // check distance to new position
    float distance_x = pos_x_final - m_x;
    float distance_y = pos_y_final - m_y;
    // velocity
    float vel_x = (distance_x * 0.04f) * pFramerate->m_speed_factor;
    float vel_y = (distance_y * 0.04f) * pFramerate->m_speed_factor;
    // true if reached position
    bool reached_x = 0;
    bool reached_y = 0;

    if (distance_x > 2.0f) {
        if (vel_x < 2.0f) {
            vel_x = 2.0f;
        }
    }
    else if (distance_x < -2.0f) {
        if (vel_x > -2.0f) {
            vel_x = -2.0f;
        }
    }
    // reached destination position x
    else {
        reached_x = 1;
    }

    if (distance_y > 2.0f) {
        if (vel_y < 2.0f) {
            vel_y = 2.0f;
        }
    }
    else if (distance_y < -2.0f) {
        if (vel_y > -2.0f) {
            vel_y = -2.0f;
        }
    }
    // reached destination position y
    else {
        reached_y = 1;
    }

    // reached position
    if (reached_x && reached_y) {
        return 0;
    }
    // move
    else {
        Move(vel_x, vel_y);
    }

    // position not reached
    return 1;
}

void cCamera::Center(const ObjectDirection direction /* = DIR_ALL */)
{
    // Center camera on a not changing player position
    if (direction == DIR_VERTICAL) {
        Set_Pos_X(Get_Center_Pos_X());
    }
    else if (direction == DIR_HORIZONTAL) {
        Set_Pos_Y(Get_Center_Pos_Y());
    }
    else if (direction == DIR_ALL) {
        Set_Pos(Get_Center_Pos_X(), Get_Center_Pos_Y());
    }
}

float cCamera::Get_Center_Pos_X(void) const
{
    return pActive_Player->m_col_rect.m_x + 5.0f - (game_res_w * 0.5f);
}

float cCamera::Get_Center_Pos_Y(void) const
{
    return pActive_Player->m_col_rect.m_y + pActive_Player->m_col_rect.m_h - 5.0f - (game_res_h * 0.5f);
}

GL_rect cCamera::Get_Rect(void) const
{
    return GL_rect(m_x, m_y, static_cast<float>(game_res_w), static_cast<float>(game_res_h));
}

void cCamera::Set_Limits(const GL_rect& rect)
{
    m_limit_rect = rect;

    // minimal size
    if (m_limit_rect.m_w < m_limit_rect.m_x + game_res_w) {
        m_limit_rect.m_w = m_limit_rect.m_x + game_res_w;
    }
    if (m_limit_rect.m_h > m_limit_rect.m_y - game_res_h) {
        m_limit_rect.m_h = m_limit_rect.m_y - game_res_h;
    }
}

void cCamera::Update_Limit_X(float& x) const
{
    // left
    if (x < m_limit_rect.m_x) {
        x = m_limit_rect.m_x;
    }
    // right
    else if (x + game_res_w > m_limit_rect.m_x + m_limit_rect.m_w) {
        x = m_limit_rect.m_x + m_limit_rect.m_w - game_res_w;
    }
}

void cCamera::Update_Limit_Y(float& y) const
{
    // down
    if (y > m_limit_rect.m_y - game_res_h) {
        y = m_limit_rect.m_y - game_res_h;
    }
    // up
    else if (y < m_limit_rect.m_y + m_limit_rect.m_h) {
        y = m_limit_rect.m_y + m_limit_rect.m_h;
    }
}

void cCamera::Update(void)
{
    // level
    if (Game_Mode == MODE_LEVEL) {
        // no leveleditor mode
        if (!editor_enabled) {
            // player is moving vertical
            if (pLevel_Player->m_vely != 0 && m_ver_offset_speed > 0.0f) {
                pLevel_Player->m_no_vely_counter = 0.0f;

                if ((m_y_offset < 100.0f && pLevel_Player->m_vely > 0.0f) || (m_y_offset > -100.0f && pLevel_Player->m_vely < 0.0f)) {
                    m_y_offset += (pLevel_Player->m_vely * m_ver_offset_speed) * pFramerate->m_speed_factor;
                }
            }
            // slowly remove offset
            else {
                pLevel_Player->m_no_vely_counter += pFramerate->m_speed_factor;

                if (pLevel_Player->m_no_vely_counter > 10.0f && (m_y_offset > 20.0f || m_y_offset < -20.0f)) {
                    m_y_offset += -(m_y_offset / 30) * pFramerate->m_speed_factor;
                }
            }

            if (m_fixed_hor_vel) {
                // todo : remove hackiness
                m_x += m_fixed_hor_vel * pFramerate->m_speed_factor;
                // check limit
                Update_Limit_X(m_x);
                // center one side
                Center(DIR_HORIZONTAL);

                // scrolls to the right
                if (m_fixed_hor_vel > 0.0f) {
                    // out of camera on the left side
                    if (pLevel_Player->m_col_rect.m_x + pLevel_Player->m_col_rect.m_w < m_x) {
                        pLevel_Player->DownGrade_Player(1, 1);
                    }
                }
                // scrolls to the left
                else {
                    // out of camera on the right side
                    if (pLevel_Player->m_col_rect.m_x - game_res_w > m_x) {
                        pLevel_Player->DownGrade_Player(1, 1);
                    }
                }
            }
            else {
                // player is moving horizontal
                if (pLevel_Player->m_velx != 0 && m_hor_offset_speed > 0.0f) {
                    pLevel_Player->m_no_velx_counter = 0.0f;

                    if ((m_x_offset < 200.0f && pLevel_Player->m_velx > 0.0f) || (m_x_offset > -200.0f && pLevel_Player->m_velx < 0.0f)) {
                        m_x_offset += (pLevel_Player->m_velx * m_hor_offset_speed) * pFramerate->m_speed_factor;
                    }
                }
                // slowly remove offset
                else {
                    pLevel_Player->m_no_velx_counter += pFramerate->m_speed_factor;

                    if (pLevel_Player->m_no_velx_counter > 10.0f && (m_x_offset > 40.0f || m_x_offset < -40.0f)) {
                        m_x_offset += -(m_x_offset / 50) * pFramerate->m_speed_factor;
                    }
                }

                // set position
                Center();
            }
        }
    }
    // world
    else if (Game_Mode == MODE_OVERWORLD) {
        // Left
        if (pActive_Player->m_pos_x - m_x < game_res_w * 0.4f) {
            Set_Pos_X(pActive_Player->m_pos_x - game_res_w * 0.4f);
        }
        // Right
        else if (pActive_Player->m_pos_x - m_x > game_res_w * 0.6f) {
            Set_Pos_X(pActive_Player->m_pos_x - game_res_w * 0.6f);
        }

        // Up
        if (pActive_Player->m_pos_y - m_y < game_res_h * 0.4f) {
            Set_Pos_Y(pActive_Player->m_pos_y - game_res_h * 0.4f);
        }
        // Down
        else if (pActive_Player->m_pos_y - m_y > game_res_h * 0.6f) {
            Set_Pos_Y(pActive_Player->m_pos_y - game_res_h * 0.6f);
        }
    }
}

void cCamera::Update_Position(void) const
{
    // mouse
    pMouseCursor->Update_Position();

    if (Game_Mode == MODE_LEVEL || Game_Mode == MODE_OVERWORLD) {
        // update player
        pActive_Player->Update_Valid_Draw();
        // update sprite manager
        m_sprite_manager->Update_Items_Valid_Draw();
    }
    else if (Game_Mode == MODE_MENU) {
        // update player
        pActive_Player->Update_Valid_Draw();
        // update sprite manager
        m_sprite_manager->Update_Items_Valid_Draw();
    }
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

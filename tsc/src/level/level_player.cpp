/***************************************************************************
 * level_player.cpp  -  level player class
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

#include "../core/global_basic.hpp"
#include "../level/level_player.hpp"
#include "../core/main.hpp"
#include "../video/animation.hpp"
#include "../core/game_core.hpp"
#include "../core/filesystem/resource_manager.hpp"
#include "../core/filesystem/package_manager.hpp"
#include "../user/preferences.hpp"
#include "../input/joystick.hpp"
#include "../core/sprite_manager.hpp"
#include "../core/framerate.hpp"
#include "../audio/audio.hpp"
#include "../enemies/army.hpp"
#include "../overworld/overworld.hpp"
#include "../level/level.hpp"
#include "../gui/menu.hpp"
#include "../objects/level_exit.hpp"
#include "../objects/box.hpp"
#include "../input/keyboard.hpp"
#include "../core/math/utilities.hpp"
#include "../core/i18n.hpp"
#include "../video/gl_surface.hpp"
#include "../core/filesystem/filesystem.hpp"
#include "../video/renderer.hpp"
#include "../scripting/events/jump_event.hpp"
#include "../scripting/events/shoot_event.hpp"
#include "../scripting/events/downgrade_event.hpp"

namespace TSC {

// Milliseconds to enable power jump when ducking
const int power_jump_delta = 1000;

const float cLevel_Player::m_default_pos_x = 200.0f;
const float cLevel_Player::m_default_pos_y = -300.0f;

/* *** *** *** *** *** *** *** *** cLevel_Player *** *** *** *** *** *** *** *** *** */

cLevel_Player::cLevel_Player(cSprite_Manager* sprite_manager)
    : cMovingSprite(sprite_manager)
{
    m_sprite_array = ARRAY_PLAYER;
    m_type = TYPE_PLAYER;
    m_massive_type = MASS_MASSIVE;
    m_state = STA_FALL;

    m_alex_type = ALEX_SMALL;
    m_alex_type_temp_power = ALEX_DEAD;
    m_name = "Alex";

    m_pos_z = cSprite::m_pos_z_player;
    m_gravity_max = 25.0f;
    m_images.reserve(20);
    m_ducked_counter = 0;
    m_ducked_animation_counter = 0.0f;
    m_parachute = 0;
    m_throwing_counter = 0.0f;

    // invincible data
    m_invincible = 0.0f;
    m_invincible_star = 0.0f;
    m_invincible_mod = 0.0f;
    m_invincible_star_counter = 0.0f;

    m_walk_count = 0.0f;
    m_running_particle_counter = 0.0f;

    m_god_mode = 0;

    m_walk_time = 0.0f;
    m_ghost_time = 0.0f;
    m_ghost_time_mod = 0.0f;

    // Starting with 3 lives
    m_lives = 3;
    m_goldpieces = 0;
    m_points = 0;
    m_kill_multiplier = 1.0f;
    m_last_kill_counter = 0.0f;

    // jump data
    m_up_key_time = 0.0f;
    m_force_jump = 0;
    m_next_jump_sound = 1;
    m_next_jump_power = 17.0f;
    m_next_jump_accel = 4.0f;
    m_jump_power = 0.0f;
    m_jump_accel_up = 4.5f;
    m_jump_vel_deaccel = 0.06f;

    // no movement timer
    m_no_velx_counter = 0.0f;
    m_no_vely_counter = 0.0f;

    m_shoot_counter = 0.0f;
    m_active_object = NULL;
    m_duck_direction = DIR_UNDEFINED;

    Set_Pos(m_default_pos_x, m_default_pos_y, 1);
}

cLevel_Player::~cLevel_Player(void)
{
    Ball_Clear();
}

cLevel_Player* cLevel_Player::Copy(void) const
{
    // not copyable
    return NULL;
}

void cLevel_Player::Init(void)
{
    Load_Images();
    // default direction : right
    Set_Direction(DIR_RIGHT, 1);
    // default uid 0
    m_uid = 0;
}

void cLevel_Player::Hold(void)
{
    if (!m_ground_object || (m_state != STA_WALK && m_state != STA_RUN)) {
        return;
    }

    Set_Moving_State(STA_STAY);
}

void cLevel_Player::Set_Direction(const ObjectDirection dir, bool new_start_direction /* = 0 */)
{
    // set start image
    if (new_start_direction) {
        cMovingSprite::Set_Image(pVideo->Get_Package_Surface("alex/small/stand_" + Get_Direction_Name(dir) + ".png"), 1);

        // set back current image
        m_curr_img = -1;
        Set_Image_Num(Get_Image() + m_direction);
    }

    cMovingSprite::Set_Direction(dir, new_start_direction);
}

bool cLevel_Player::Set_On_Ground(cSprite* obj, bool set_on_top /* = 1 */)
{
    bool valid = cMovingSprite::Set_On_Ground(obj, set_on_top);

    // if valid ground
    if (valid) {
        // if moving
        if (m_velx) {
            if (m_state != STA_STAY && m_state != STA_RUN) {
                Set_Moving_State(STA_WALK);
            }
        }
        // not moving
        else {
            Set_Moving_State(STA_STAY);
        }

        // if massive ground and ducking key is pressed
        if (m_ground_object->m_massive_type == MASS_MASSIVE && pKeyboard->m_keys[pPreferences->m_key_down]) {
            Start_Ducking();
        }
    }

    return valid;
}

void cLevel_Player::DownGrade(bool force /* = 0 */)
{
    DownGrade_Player(true, force);
}

void cLevel_Player::DownGrade_Player(bool delayed /* = true */, bool force /* = false */, bool ignore_invincible /* = false */)
{
    if (m_god_mode)
        return;
    if (m_invincible && !ignore_invincible)
        return;

    // already dead
    if (m_alex_type == ALEX_DEAD) {
        return;
    }

    if (delayed) {
        Game_Action = GA_DOWNGRADE_PLAYER;
        if (force) {
            Game_Action_Data_Middle.add("downgrade_force", "1");
            if (ignore_invincible) {
                Game_Action_Data_Middle.add("downgrade_ignore_invincible", "1");
            }
        }

        return;
    }

    // if not weakest state or not forced
    if (m_alex_type != ALEX_SMALL && !force) {
        pAudio->Play_Sound("player/powerdown.ogg", RID_ALEX_POWERDOWN);

        // power down
        Set_Type(ALEX_SMALL);

        m_invincible = speedfactor_fps * 2.5f;
        m_invincible_mod = 0.0f;

        pHud_Itembox->Request_Item();

        // Issue the Downgrade event
        Scripting::cDowngrade_Event evt(1, 2); // downgrades = 1, max. downgrades = 2
        evt.Fire(pActive_Level->m_mruby, this);

        return;
    }

    Set_Type(ALEX_DEAD, 0, 0);
    pHud_Time->Reset();
    pHud_Points->Clear();
    Ball_Clear();
    pHud_Lives->Add_Lives(-1);
    pAudio->Fadeout_Music(1700);

    // lost a live
    if (m_lives >= 0) {
        pAudio->Play_Sound(utf8_to_path("player/dead.ogg"), RID_ALEX_DEATH);
    }
    // game over
    else {
        pAudio->Play_Sound(pPackage_Manager->Get_Music_Reading_Path("game/lost_1.ogg"), RID_ALEX_DEATH);
    }

    // dying animation
    Set_Image_Num(ALEX_IMG_DEAD + 1);

    // draw
    Draw_Game();
    // render
    pVideo->Render();

    // wait
    SDL_Delay(500);
    pFramerate->Reset();

    Set_Image_Num(ALEX_IMG_DEAD);

    float i;

    for (i = 0.0f; i < 7.0f; i += pFramerate->m_speed_factor) {
        while (SDL_PollEvent(&input_event)) {
            if (input_event.type == SDL_KEYDOWN) {
                if (input_event.key.keysym.sym == SDLK_ESCAPE) {
                    goto animation_end;
                }
                else if (input_event.key.keysym.sym == pPreferences->m_key_screenshot) {
                    pVideo->Save_Screenshot();
                }
            }
            else if (input_event.type == SDL_JOYBUTTONDOWN) {
                if (input_event.jbutton.button == pPreferences->m_joy_button_exit) {
                    goto animation_end;
                }
            }
        }

        // move up
        Move(0.0f, -13.0f);
        // draw
        Draw_Game();
        // render
        pVideo->Render();
        pFramerate->Update();
    }

    // very small delay until falling animation
    SDL_Delay(300);

    pFramerate->Reset();
    m_walk_count = 0.0f;

    for (i = 0.0f; m_col_rect.m_y < pActive_Camera->m_y + game_res_h; i++) {
        while (SDL_PollEvent(&input_event)) {
            if (input_event.type == SDL_KEYDOWN) {
                if (input_event.key.keysym.sym == SDLK_ESCAPE) {
                    goto animation_end;
                }
                else if (input_event.key.keysym.sym == pPreferences->m_key_screenshot) {
                    pVideo->Save_Screenshot();
                }
            }
            else if (input_event.type == SDL_JOYBUTTONDOWN) {
                if (input_event.jbutton.button == pPreferences->m_joy_button_exit) {
                    goto animation_end;
                }
            }
        }

        m_walk_count += pFramerate->m_speed_factor * 0.75f;

        if (m_walk_count > 4.0f) {
            m_walk_count = 0.0f;
        }

        // move down
        Move(0.0f, 14.0f);

        if (m_walk_count > 2.0f) {
            Set_Image_Num(ALEX_IMG_DEAD);
        }
        else {
            Set_Image_Num(ALEX_IMG_DEAD + 1);
        }

        // draw
        Draw_Game();
        // render
        pVideo->Render();
        pFramerate->Update();
    }

animation_end:

    // game over
    if (m_lives < 0) {
        cGL_Surface* game_over = pVideo->Get_Package_Surface("game/game_over.png");
        cSprite* sprite = new cSprite(m_sprite_manager);
        sprite->Set_Image(game_over);
        sprite->Set_Pos((game_res_w * 0.5f) - (game_over->m_w * 0.5f), (game_res_h * 0.5f) - (game_over->m_h * 0.5f));
        sprite->m_pos_z = 0.8f;
        sprite->Set_Spawned(1);
        sprite->Set_Ignore_Camera(1);
        m_sprite_manager->Add(sprite);

        cParticle_Emitter* anim = new cParticle_Emitter(m_sprite_manager);
        anim->Set_Emitter_Rect(sprite->m_rect.m_x + pActive_Camera->m_x, sprite->m_rect.m_y + pActive_Camera->m_y, sprite->m_rect.m_w, sprite->m_rect.m_h);
        anim->Set_Emitter_Time_to_Live(24.0f);
        anim->Set_Emitter_Iteration_Interval(0.1f);
        anim->Set_Quota(1);
        anim->Set_Pos_Z(0.79f);
        anim->Set_Image(pVideo->Get_Package_Surface("animation/particles/axis.png"));
        anim->Set_Time_to_Live(10.0f);
        anim->Set_Fading_Alpha(1);
        anim->Set_Speed(2.0f, 0.5f);
        anim->Set_Scale(0.9f);
        anim->Set_Color(Color(static_cast<Uint8>(250), 140, 90), Color(static_cast<Uint8>(5), 100, 0, 0));
        anim->Set_Const_Rotation_Z(-2.0f, 4.0f);

        for (i = 10.0f; i > 0.0f; i -= 0.011f * pFramerate->m_speed_factor) {
            while (SDL_PollEvent(&input_event)) {
                if (input_event.type == SDL_KEYDOWN) {
                    if (input_event.key.keysym.sym == pPreferences->m_key_screenshot) {
                        pVideo->Save_Screenshot();
                    }
                }
            }

            Uint8* keys = SDL_GetKeyState(NULL);
            // Escape stops
            if (keys[SDLK_ESCAPE] || keys[SDLK_RETURN] || keys[SDLK_SPACE] || keys[pPreferences->m_key_action]) {
                break;
            }

            // if joystick enabled and exit pressed
            if (pPreferences->m_joy_enabled && SDL_JoystickGetButton(pJoystick->m_joystick, pPreferences->m_joy_button_exit)) {
                break;
            }

            // update animation
            anim->Update();

            // draw
            Draw_Game();
            anim->Draw();

            pVideo->Render();
            pFramerate->Update();
        }

        delete anim;
        pAudio->Stop_Sounds();
    }

    // clear
    Clear_Input_Events();
    pFramerate->Reset();

    // game over
    if (m_lives < 0) {
        Game_Action = GA_ENTER_MENU;
        // reset saved data
        Game_Action_Data_Middle.add("reset_save", "1");
        Game_Action_Data_Middle.add("load_menu", int_to_string(MENU_MAIN));
    }
    // custom level
    else if (Game_Mode_Type == MODE_TYPE_LEVEL_CUSTOM) {
        Game_Action = GA_ENTER_MENU;
        Game_Action_Data_Middle.add("load_menu", int_to_string(MENU_START));
        Game_Action_Data_Middle.add("menu_start_current_level", path_to_utf8(Trim_Filename(pActive_Level->m_level_filename, 0, 0)));
        // reset saved data
        Game_Action_Data_Middle.add("reset_save", "1");
    }
    // back to overworld
    else {
        Set_Type(ALEX_SMALL, 0, 0);
        Game_Action = GA_ENTER_WORLD;
    }

    // fade out
    Game_Action_Data_Start.add("music_fadeout", "1500");
    Game_Action_Data_Start.add("screen_fadeout", CEGUI::PropertyHelper::intToString(EFFECT_OUT_BLACK));
    Game_Action_Data_Start.add("screen_fadeout_speed", "3");
    // delay unload level
    Game_Action_Data_Middle.add("unload_levels", "1");
    Game_Action_Data_End.add("screen_fadein", CEGUI::PropertyHelper::intToString(EFFECT_IN_BLACK));
}

void cLevel_Player::Move_Player(float velocity, float vel_wrongway)
{
    if (m_direction == DIR_LEFT) {
        velocity *= -1;
        vel_wrongway *= -1;
    }

    // get collision list
    cObjectCollisionType* col_list = Collision_Check_Relative(velocity, 0.0f, 0.0f, 0.0f, COLLIDE_ONLY_BLOCKING);
    // if collision with a blocking object
    bool is_col = 0;

    // check collisions
    for (cObjectCollision_List::iterator itr = col_list->objects.begin(); itr != col_list->objects.end(); ++itr) {
        cObjectCollision* col_obj = (*itr);

        // massive object is blocking
        if (col_obj->m_obj->m_massive_type == MASS_MASSIVE) {
            is_col = 1;
            break;
        }
    }

    delete col_list;

    // don't move if colliding
    if (is_col) {
        if (Is_Float_Equal(m_velx, 0.0f)) {
            Set_Moving_State(STA_STAY);
        }
        // slow down
        else {
            m_velx -= (m_velx * 0.1f) * pFramerate->m_speed_factor;

            if (m_velx > -0.1f && m_velx < 0.1f) {
                m_velx = 0.0f;
            }
        }

        return;
    }

    // move right
    if (m_direction == DIR_RIGHT) {
        if (m_velx > 0.0f) {
            Add_Velocity_X_Max(velocity, 10.0f * Get_Vel_Modifier());
        }
        else {
            // small smoke clouds under foots
            if (m_velx < 0.0f) {
                Generate_Feet_Clouds();
            }

            // slow down
            Add_Velocity_X(vel_wrongway);
        }
    }
    // move left
    else if (m_direction == DIR_LEFT) {
        if (m_velx < 0.0f) {
            Add_Velocity_X_Min(velocity, -10.0f * Get_Vel_Modifier());
        }
        else {
            // small smoke clouds under foots
            if (m_velx > 0.0f) {
                Generate_Feet_Clouds();
            }

            // slow down
            Add_Velocity_X(vel_wrongway);
        }
    }

    // start walking
    if (m_state == STA_STAY) {
        Set_Moving_State(STA_WALK);
    }
}

void cLevel_Player::Generate_Feet_Clouds(cParticle_Emitter* anim /* = NULL */)
{
    // check if valid
    if (!m_ground_object || !m_ground_object->m_image || m_ground_object->m_image->m_ground_type == GROUND_NORMAL) {
        return;
    }

    bool create_anim = 0;

    if (!anim) {
        create_anim = 1;
        // create animation
        anim = new cParticle_Emitter(m_sprite_manager);
    }

    anim->Set_Emitter_Rect(m_col_rect.m_x, m_col_rect.m_y + m_col_rect.m_h - 2.0f, m_col_rect.m_w);
    anim->Set_Pos_Z(m_pos_z - 0.000001f);

    float vel;

    if (m_velx > 0.0f) {
        vel = m_velx;
    }
    else {
        vel = -m_velx;
    }

    // ground type
    switch (m_ground_object->m_image->m_ground_type) {
    case GROUND_EARTH: {
        anim->Set_Image(pVideo->Get_Package_Surface("animation/particles/dirt.png"));
        anim->Set_Time_to_Live(0.3f);
        anim->Set_Scale(0.5f);
        anim->Set_Speed(0.08f + vel * 0.1f, vel * 0.05f);
        break;
    }
    case GROUND_ICE: {
        anim->Set_Image(pVideo->Get_Package_Surface("animation/particles/ice_1.png"));
        anim->Set_Time_to_Live(0.6f);
        anim->Set_Scale(0.3f);
        anim->Set_Speed(0.1f + vel * 0.05f, vel * 0.04f);
        break;
    }
    case GROUND_SAND: {
        anim->Set_Emitter_Iteration_Interval(4.0f);
        anim->Set_Image(pVideo->Get_Package_Surface("animation/particles/cloud.png"));
        anim->Set_Time_to_Live(0.3f);
        anim->Set_Scale(0.2f);
        anim->Set_Color(lightorange);
        anim->Set_Speed(0.2f + vel * 0.15f, vel * 0.15f);
        break;
    }
    case GROUND_STONE: {
        anim->Set_Image(pVideo->Get_Package_Surface("animation/particles/smoke_black.png"));
        anim->Set_Time_to_Live(0.3f);
        anim->Set_Scale(0.3f);
        anim->Set_Speed(vel * 0.08f, 0.1f + vel * 0.1f);
        break;
    }
    case GROUND_PLASTIC: {
        anim->Set_Image(pVideo->Get_Package_Surface("animation/particles/light.png"));
        anim->Set_Time_to_Live(0.2f);
        anim->Set_Scale(0.2f);
        anim->Set_Color(lightgrey);
        anim->Set_Speed(0.05f, vel * 0.05f);
        break;
    }
    default: {
        anim->Set_Image(pVideo->Get_Package_Surface("animation/particles/smoke.png"));
        anim->Set_Time_to_Live(0.3f);
        anim->Set_Speed(0.1f + vel * 0.1f, vel * 0.1f);
        break;
    }
    }

    if (m_direction == DIR_RIGHT) {
        anim->Set_Direction_Range(180.0f, 90.0f);
    }
    else {
        anim->Set_Direction_Range(270.0f, 90.0f);
    }

    if (create_anim) {
        anim->Emit();
        pActive_Animation_Manager->Add(anim);
    }
}

void cLevel_Player::Update_Walking(void)
{
    if (m_ducked_counter || !m_ground_object || (m_state != STA_STAY && m_state != STA_WALK && m_state != STA_RUN)) {
        return;
    }

    // validate ground object
    if ((m_ground_object->m_type == TYPE_EATO || m_ground_object->m_type == TYPE_SPIKA || m_ground_object->m_type == TYPE_ROKKO || m_ground_object->m_type == TYPE_STATIC_ENEMY) && m_invincible <= 0 && !m_god_mode) {
        Reset_On_Ground();
    }

    // only if left or right is pressed
    if (pKeyboard->m_keys[pPreferences->m_key_left] || pKeyboard->m_keys[pPreferences->m_key_right] || pJoystick->m_left || pJoystick->m_right) {
        float ground_mod = 1.0f;

        if (m_ground_object && m_ground_object->m_image) {
            // ground type
            switch (m_ground_object->m_image->m_ground_type) {
            case GROUND_ICE: {
                ground_mod = 0.5f;
                break;
            }
            case GROUND_SAND: {
                ground_mod = 0.7f;
                break;
            }
            case GROUND_PLASTIC: {
                ground_mod = 0.85f;
                break;
            }
            default: {
                break;
            }
            }
        }

        Move_Player((0.6f * ground_mod * Get_Vel_Modifier()), (1.2f * ground_mod * Get_Vel_Modifier()));
    }

    // update walking time
    if (m_state == STA_WALK) {
        // update
        if (m_velx > 10.0f || m_velx < -10.0f) {
            m_walk_time += pFramerate->m_speed_factor;

            if (m_walk_time > speedfactor_fps) {
                Set_Moving_State(STA_RUN);
            }
        }
        // reset
        else if (m_walk_time) {
            m_walk_time = 0.0f;
        }
    }
}

void cLevel_Player::Update_Running(void)
{
    if (m_ducked_counter || !m_ground_object || m_state != STA_RUN) {
        return;
    }

    m_running_particle_counter += pFramerate->m_speed_factor * 0.5f;

    // create particles
    while (m_running_particle_counter > 1.0f) {
        float vel;

        if (m_velx > 0.0f) {
            vel = m_velx;
        }
        else {
            vel = -m_velx;
        }

        // start on high velocity
        if (vel > 17.0f) {
            // light particles
            cParticle_Emitter* anim = new cParticle_Emitter(m_sprite_manager);
            anim->Set_Emitter_Rect(m_col_rect.m_x + m_col_rect.m_w * 0.25f, m_col_rect.m_y + m_col_rect.m_h * 0.1f, m_col_rect.m_w * 0.5f, m_col_rect.m_h * 0.8f);
            anim->Set_Image(pVideo->Get_Package_Surface("animation/particles/light.png"));
            anim->Set_Pos_Z(m_pos_z + 0.000001f);
            anim->Set_Time_to_Live(0.1f + vel * 0.03f);
            anim->Set_Fading_Alpha(1);
            anim->Set_Fading_Size(1);
            anim->Set_Speed(vel * 0.9f, 0.0f);
            // right
            if (m_velx > 0.0f) {
                anim->Set_Direction_Range(0.0f, 0.0f);
            }
            // left
            else {
                anim->Set_Direction_Range(180.0f, 0.0f);
            }
            anim->Set_Scale(0.15f);

            anim->Set_Blending(BLEND_ADD);
            anim->Emit();
            pActive_Animation_Manager->Add(anim);
        }

        // smoke clouds under foots
        Generate_Feet_Clouds();

        m_running_particle_counter--;
    }
}

void cLevel_Player::Update_Staying(void)
{
    // only if player is onground
    if (!m_ground_object || m_ducked_counter || m_state == STA_JUMP || m_state == STA_CLIMB) {
        return;
    }

    // if left and right is not pressed
    if (!pKeyboard->m_keys[pPreferences->m_key_left] && !pKeyboard->m_keys[pPreferences->m_key_right] && !pJoystick->m_left && !pJoystick->m_right) {
        // walking
        if (m_velx) {
            if (m_ground_object->m_image && m_ground_object->m_image->m_ground_type == GROUND_ICE) {
                Auto_Slow_Down(1.1f / 5.0f);
            }
            else {
                Auto_Slow_Down(1.1f);
            }

            // stopped walking
            if (!m_velx) {
                Set_Moving_State(STA_STAY);
            }
        }

        // walk on spika
        if (m_ground_object->m_type == TYPE_SPIKA) {
            cMovingSprite* moving_ground_object = static_cast<cMovingSprite*>(m_ground_object);

            if (moving_ground_object->m_velx < 0.0f) {
                m_walk_count -= moving_ground_object->m_velx * pFramerate->m_speed_factor * 0.1f;
            }
            else if (moving_ground_object->m_velx > 0.0f) {
                m_walk_count += moving_ground_object->m_velx * pFramerate->m_speed_factor * 0.1f;
            }
        }
        // if not moving anymore
        else if (Is_Float_Equal(m_velx, 0.0f)) {
            m_walk_count = 0.0f;
        }
    }

    // if staying don't move vertical
    if (m_vely > 0.0f) {
        m_vely = 0.0f;
    }
}

void cLevel_Player::Update_Flying(void)
{
    /* only if not onground
     * if jumping update jump is already used
    */
    if (m_ground_object || m_state == STA_CLIMB || m_state == STA_JUMP) {
        return;
    }

    // flying
    if (m_state == STA_FLY) {
        if (m_direction == DIR_LEFT) {
            const float max_vel = -15.0f * Get_Vel_Modifier();

            if (m_velx > max_vel) {
                Add_Velocity_X_Min(-1.1f, max_vel);
            }
        }
        else {
            const float max_vel = 15.0f * Get_Vel_Modifier();

            if (m_velx < max_vel) {
                Add_Velocity_X_Max(1.1f, max_vel);
            }
        }

        // move down
        if (pKeyboard->m_keys[pPreferences->m_key_down] || pJoystick->m_down) {
            const float max_vel = 5.0f * Get_Vel_Modifier();

            if (m_vely < max_vel) {
                Add_Velocity_Y_Max(0.6f, max_vel);
            }
        }
        // move up
        else if (pKeyboard->m_keys[pPreferences->m_key_up] || pJoystick->m_up) {
            const float max_vel = -5.0f * Get_Vel_Modifier();

            if (m_vely > max_vel) {
                Add_Velocity_Y_Min(-0.6f, max_vel);
            }
        }
        // slow down if not pressed
        else {
            Auto_Slow_Down(0.0f, 0.4f);
        }

        // set limited flying rotation
        const float fly_rot = Clamp(m_vely * 2.5f, -70.0f, 70.0f);
        Set_Rotation_Z(fly_rot);
    }
    // falling
    else {
        // move left
        if ((pKeyboard->m_keys[pPreferences->m_key_left] || pJoystick->m_left) && !m_ducked_counter) {
            if (!m_parachute) {
                const float max_vel = -10.0f * Get_Vel_Modifier();

                if (m_velx > max_vel) {
                    Add_Velocity_X_Min(-1.2f, max_vel);
                }
            }
            // slow fall/parachute
            else {
                const float max_vel = -5.0f * Get_Vel_Modifier();

                if (m_velx > max_vel) {
                    Add_Velocity_X_Min(-0.6f, max_vel);
                }
            }
        }
        // move right
        else if ((pKeyboard->m_keys[pPreferences->m_key_right] || pJoystick->m_right) && !m_ducked_counter) {
            if (!m_parachute) {
                const float max_vel = 10.0f * Get_Vel_Modifier();

                if (m_velx < max_vel) {
                    Add_Velocity_X_Max(1.2f, max_vel);
                }
            }
            // slow fall/parachute
            else {
                const float max_vel = 5.0f * Get_Vel_Modifier();

                if (m_velx < max_vel) {
                    Add_Velocity_X_Max(0.6f, max_vel);
                }
            }
        }
        // slow down if not pressed
        else {
            Auto_Slow_Down(0.4f);
        }
    }
}

void cLevel_Player::Stop_Flying(bool parachute /* = 1 */)
{
    if (m_state != STA_FLY) {
        return;
    }

    if (parachute) {
        Parachute(1);
    }

    Set_Moving_State(STA_FALL);
}

void cLevel_Player::Parachute(bool enable)
{
    if (m_parachute == enable) {
        return;
    }

    m_parachute = enable;

    if (m_parachute) {
        m_gravity_max = 10.0f;
    }
    else {
        m_gravity_max = 25.0f;
    }
}

void cLevel_Player::Start_Falling(void)
{
    const float move_y = 1.9f;

    // check if something else is now blocking
    cObjectCollisionType* col_list = Collision_Check_Relative(0.0f, move_y, 0.0f, 0.0f, COLLIDE_ONLY_BLOCKING);

    // check possible new ground objects
    for (cObjectCollision_List::iterator itr = col_list->objects.begin(); itr != col_list->objects.end(); ++itr) {
        cObjectCollision* col_obj = (*itr);

        // blocked by new ground object
        if (col_obj->m_obj != m_ground_object && col_obj->m_obj->m_can_be_ground) {
            Set_On_Ground(col_obj->m_obj);
            delete col_list;
            return;
        }
    }

    delete col_list;

    // fall through ground object
    Move(0.0f, move_y, 1);
    Set_Moving_State(STA_FALL);
}

void cLevel_Player::Update_Falling(void)
{
    // only if not on ground
    if (m_ground_object || m_state == STA_CLIMB || m_state == STA_FLY) {
        return;
    }

    // Add Gravitation
    if (!m_parachute) {
        if (m_vely < m_gravity_max) {
            Add_Velocity_Y_Max(2.8f, m_gravity_max);
        }
    }
    // slow fall/parachute
    else {
        if (m_vely < m_gravity_max) {
            Add_Velocity_Y_Max(0.6f, m_gravity_max);
        }
    }

    if (m_state != STA_JUMP && m_state != STA_FALL) {
        Set_Moving_State(STA_FALL);
    }
}

void cLevel_Player::Start_Ducking(void)
{
    // only if massive ground
    if ((m_state != STA_STAY && m_state != STA_WALK && m_state != STA_RUN) || m_state == STA_CLIMB || !m_ground_object || m_ground_object->m_massive_type == MASS_HALFMASSIVE || m_ducked_counter) {
        return;
    }

    /* Always set the initial duck direction according to the moving direction,
     * not to Alex’s direction, because the player could run otherwise, turn
     * around, and duck then, allowing him to hop through narrow passages
     * where it isn’t allowed (see Start_Jumping() for the actual test
     * of the m_duck_direction variable). */
    if (m_velx > 0.0f)
        m_duck_direction = DIR_RIGHT;
    else if (m_velx < 0.0f)
        m_duck_direction = DIR_LEFT;

    Release_Item(1, 1);

    // set ducking image ( without Check_OutofLevel from cMovingSprite )
    cSprite::Move(0.0f, m_image->m_col_h - m_images[ALEX_IMG_DUCK].m_image->m_col_h, 1);
    Set_Image_Num(ALEX_IMG_DUCK + m_direction);

    m_ducked_counter = 1;
    Set_Moving_State(STA_STAY);
}

void cLevel_Player::Stop_Ducking(void)
{
    if (!m_ducked_counter) {
        return;
    }

    // get space needed to stand up
    const float move_y = -(m_images[ALEX_IMG_STAND].m_image->m_col_h - m_image->m_col_h);

    cObjectCollisionType* col_list = Collision_Check_Relative(0.0f, move_y, 0.0f, 0.0f, COLLIDE_ONLY_BLOCKING);

    // failed to stand up because something is blocking
    if (col_list->size()) {
        // set ducked time again to stop possible power jump while in air
        m_ducked_counter = 1;
        delete col_list;
        return;
    }

    delete col_list;

    // unset ducking image ( without Check_out_of_Level from cMovingSprite )
    cSprite::Move(0.0f, move_y, 1);
    Set_Image_Num(ALEX_IMG_STAND + m_direction);

    m_ducked_counter = 0;
    m_ducked_animation_counter = 0.0f;
    Set_Moving_State(STA_STAY);
}

void cLevel_Player::Update_Ducking(void)
{
    if (!m_ducked_counter) {
        return;
    }

    // lost ground object
    if (!m_ground_object) {
        Stop_Ducking();
        return;
    }

    m_ducked_counter += pFramerate->m_elapsed_ticks;

    if (m_ducked_counter > power_jump_delta) {
        // particle animation
        m_ducked_animation_counter += pFramerate->m_speed_factor * 2;

        if (m_ducked_animation_counter > 1.0f) {
            // create particle
            cParticle_Emitter* anim = new cParticle_Emitter(m_sprite_manager);
            anim->Set_Emitter_Rect(m_col_rect.m_x, m_col_rect.m_y + (m_col_rect.m_h * 0.8f), m_col_rect.m_w * 0.9f, m_col_rect.m_h * 0.1f);
            anim->Set_Quota(static_cast<int>(m_ducked_animation_counter));
            anim->Set_Image(pVideo->Get_Package_Surface("animation/particles/star_2.png"));
            anim->Set_Pos_Z(m_pos_z - 0.000001f, 0.000002f);
            anim->Set_Time_to_Live(0.3f);
            anim->Set_Fading_Alpha(1);
            anim->Set_Fading_Size(1);
            anim->Set_Const_Rotation_Z(-5.0f, 10.0f);
            anim->Set_Direction_Range(90.0f, 20.0f);
            anim->Set_Speed(1.5f, 0.5f);
            anim->Set_Scale(0.2f);
            anim->Set_Color(whitealpha128);
            anim->Set_Blending(BLEND_ADD);
            anim->Emit();
            pActive_Animation_Manager->Add(anim);

            m_ducked_animation_counter -= static_cast<int>(m_ducked_animation_counter);
        }
    }

    // slow down
    if (m_velx) {
        Auto_Slow_Down(0.6f);
    }
}

void cLevel_Player::Start_Climbing(void)
{
    // invalid state
    if (m_ducked_counter || m_state == STA_CLIMB || m_jump_power > 5.0f) {
        return;
    }

    const float move_y = -1.0f;

    // if not on climbable
    if (!Is_On_Climbable(move_y)) {
        return;
    }

    // avoid that we are set on the ground again
    Col_Move(0.0f, move_y, 1, 1);

    Release_Item(1, 1);
    Set_Moving_State(STA_CLIMB);
}

void cLevel_Player::Update_Climbing(void)
{
    if (m_state != STA_CLIMB) {
        return;
    }

    m_velx = 0.0f;
    m_vely = 0.0f;

    if (Is_On_Climbable()) {
        // set velocity
        if (pKeyboard->m_keys[pPreferences->m_key_left] || pJoystick->m_left) {
            m_velx = -2.0f * Get_Vel_Modifier();
        }
        else if (pKeyboard->m_keys[pPreferences->m_key_right] || pJoystick->m_right) {
            m_velx = 2.0f * Get_Vel_Modifier();
        }

        if (pKeyboard->m_keys[pPreferences->m_key_up] || pJoystick->m_up) {
            m_vely = -4.0f * Get_Vel_Modifier();
        }
        else if (pKeyboard->m_keys[pPreferences->m_key_down] || pJoystick->m_down) {
            m_vely = 4.0f * Get_Vel_Modifier();
        }

        // check if reached climbable top
        if (m_vely < 0.0f) {
            // do not climb further upwards if alex will loose contact
            if (!Is_On_Climbable(-10.0f)) {
                // only stop if loosing contact upwards
                m_vely = 0.0f;
            }
        }
    }
    else {
        // lost contact
        Set_Moving_State(STA_FALL);
    }
}

bool cLevel_Player::Is_On_Climbable(float move_y /* = 0.0f */)
{
    // create climb rect
    GL_rect climb_rect = m_col_rect;
    climb_rect.m_x += (climb_rect.m_w * 0.5f) - 2.0f;
    climb_rect.m_y += move_y;
    climb_rect.m_w = 4.0f;
    climb_rect.m_h = 4.0f;

    // collision testing
    cObjectCollisionType* col_list = Collision_Check(&climb_rect, COLLIDE_ONLY_INTERNAL);

    // check objects
    for (cObjectCollision_List::iterator itr = col_list->objects.begin(); itr != col_list->objects.end(); ++itr) {
        cObjectCollision* col_obj = (*itr);

        // collision with climbable object
        if (col_obj->m_obj->m_massive_type == MASS_CLIMBABLE) {
            delete col_list;
            return 1;
        }
    }

    delete col_list;
    return 0;
}

void cLevel_Player::Start_Jump_Keytime(void)
{
    if (m_god_mode || m_state == STA_STAY || m_state == STA_WALK || m_state == STA_RUN || m_state == STA_FALL || m_state == STA_FLY || m_state == STA_JUMP || (m_state == STA_CLIMB && !pKeyboard->m_keys[pPreferences->m_key_up])) {
        m_up_key_time = speedfactor_fps / 4;
    }
}

void cLevel_Player::Update_Jump_Keytime(void)
{
    // handle jumping start
    if (m_force_jump || (m_up_key_time && (m_ground_object || m_god_mode || m_state == STA_CLIMB))) {
        Start_Jump();
    }
}

void cLevel_Player::Start_Jump(float deaccel /* = 0.08f */)
{
    // play sound
    if (m_next_jump_sound) {
        // small
        if (m_alex_type == ALEX_SMALL) {
            if (m_force_jump) {
                pAudio->Play_Sound("player/jump_small_power.ogg", RID_ALEX_JUMP);
            }
            else {
                pAudio->Play_Sound("player/jump_small.ogg", RID_ALEX_JUMP);
            }
        }
        // ghost
        else if (m_alex_type == ALEX_GHOST) {
            pAudio->Play_Sound("player/jump_ghost.ogg", RID_ALEX_JUMP);
        }
        // big
        else {
            if (m_force_jump) {
                pAudio->Play_Sound("player/jump_big_power.ogg", RID_ALEX_JUMP);
            }
            else {
                pAudio->Play_Sound("player/jump_big.ogg", RID_ALEX_JUMP);
            }
        }
    }

    bool jump_key = 0;

    // if jump key pressed
    if (pKeyboard->m_keys[pPreferences->m_key_jump] || (pPreferences->m_joy_analog_jump && pJoystick->m_up) || pJoystick->Button(pPreferences->m_joy_button_jump)) {
        jump_key = 1;
    }

    // todo : is this needed ?
    // avoid that we are set on the ground again
    Col_Move(0.0f, -1.0f, 1, 1);

    // fly
    if (m_alex_type == ALEX_CAPE && !m_force_jump && m_state == STA_RUN && jump_key && ((m_direction == DIR_RIGHT && m_velx > 14) || (m_direction == DIR_LEFT && m_velx < -14))) {
        m_vely = -m_next_jump_power * 0.5f;
        Set_Moving_State(STA_FLY);
    }
    // jump
    else {
        m_jump_accel_up = m_next_jump_accel;
        m_jump_vel_deaccel = deaccel;

        if (jump_key) {
            m_jump_power = m_next_jump_power * 0.59f;
        }
        else {
            m_jump_power = m_next_jump_power * 0.12f;
        }

        // Issue jump event
        Scripting::cJump_Event evt;
        evt.Fire(pActive_Level->m_mruby, this);

        m_vely = -m_next_jump_power;
        Set_Moving_State(STA_JUMP);
    }

    // jump higher when running
    if (m_velx < 0.0f) {
        m_vely += m_velx / 9.5f;
    }
    else if (m_velx > 0.0f) {
        m_vely -= m_velx / 9.5f;
    }

    // slow down if running
    m_velx = m_velx * 0.9f;

    // jump with velx if ducking but only into the opposite start duck direction to get out of a hole
    if (m_ducked_counter) {
        if (m_direction == DIR_RIGHT && m_duck_direction != m_direction) {
            if (m_velx < 5.0f) {
                m_velx += 2.0f;
            }
        }
        else if (m_direction == DIR_LEFT && m_duck_direction != m_direction) {
            if (m_velx > -5.0f) {
                m_velx -= 2.0f;
            }
        }
    }

    // reset variables
    m_up_key_time = 0.0f;
    m_force_jump = 0;
    m_next_jump_sound = 1;
    m_next_jump_power = 17.0f;
    m_next_jump_accel = 4.0f;
}

void cLevel_Player::Update_Jump(void)
{
    // jumping keytime
    if (m_up_key_time) {
        m_up_key_time -= pFramerate->m_speed_factor;

        if (m_up_key_time < 0.0f) {
            m_up_key_time = 0.0f;
        }
    }

    // only if jumping
    if (m_state != STA_JUMP) {
        return;
    }

    // jumping physics
    if (pKeyboard->m_keys[pPreferences->m_key_jump] || (pPreferences->m_joy_analog_jump && pJoystick->m_up) || pJoystick->Button(pPreferences->m_joy_button_jump)) {
        Add_Velocity_Y(-(m_jump_accel_up + (m_vely * m_jump_vel_deaccel) / Get_Vel_Modifier()));
        m_jump_power -= pFramerate->m_speed_factor;
    }
    else {
        Add_Velocity_Y(0.5f);
        m_jump_power -= 6.0f * pFramerate->m_speed_factor;
    }

    // left right physics
    if ((pKeyboard->m_keys[pPreferences->m_key_left] || pJoystick->m_left) && !m_ducked_counter) {
        const float max_vel = -10.0f * Get_Vel_Modifier();

        if (m_velx > max_vel) {
            Add_Velocity_X_Min((-1.1f * Get_Vel_Modifier()) - (m_velx / 100), max_vel);
        }

    }
    else if ((pKeyboard->m_keys[pPreferences->m_key_right] || pJoystick->m_right) && !m_ducked_counter) {
        const float max_vel = 10.0f * Get_Vel_Modifier();

        if (m_velx < max_vel) {
            Add_Velocity_X_Max((1.1f * Get_Vel_Modifier()) + (m_velx / 100), max_vel);
        }
    }
    // slow down
    else {
        Auto_Slow_Down(0.2f);
    }

    // if no more jump power set to falling
    if (m_jump_power <= 0.0f) {
        Set_Moving_State(STA_FALL);
    }
}

void cLevel_Player::Update_Item(void)
{
    // if active object item is available
    if (m_active_object) {
        // collision with something or activated
        if ((m_active_object->m_type == TYPE_ARMY || m_active_object->m_type == TYPE_SHELL) && static_cast<cArmy*>(m_active_object)->m_dead) { // shell is an armadillo anyways
            Release_Item(false);
            return;
        }

        // update the item position
        float item_pos_x = 0.0f;
        float item_pos_y = 0.0f;

        if (m_alex_type == ALEX_SMALL) {
            if (m_direction == DIR_LEFT) {
                item_pos_x = (m_rect.m_w * 0.20f) - m_active_object->m_col_rect.m_w;
            }
            else {
                item_pos_x = m_rect.m_w * 0.64f;
            }

            item_pos_y = m_rect.m_h * 0.14f;
        }
        // big
        else {
            if (m_direction == DIR_LEFT) {
                item_pos_x = (m_rect.m_w * 0.21f) - m_active_object->m_col_rect.m_w;
            }
            else {
                item_pos_x = m_rect.m_w * 0.66f;
            }

            item_pos_y = m_rect.m_h * 0.18f;
        }

        m_active_object->Set_Pos(m_pos_x + item_pos_x, m_pos_y + item_pos_y);

        return;
    }

    // invalid state
    if (m_state == STA_FLY || m_ducked_counter || m_state == STA_CLIMB) {
        return;
    }

    // if control is pressed search for items in front of the player
    if (pKeyboard->m_keys[pPreferences->m_key_action] || pJoystick->Button(pPreferences->m_joy_button_action)) {
        // next position velocity with extra size
        float check_x = (m_velx > 0.0f) ? (m_velx + 5.0f) : (m_velx - 5.0f);

        // if wrong direction return
        if ((m_direction == DIR_LEFT && check_x >= 0.0f) || (m_direction == DIR_RIGHT && check_x <= 0.0f)) {
            return;
        }

        // check the next player position for objects
        cObjectCollisionType* col_list = Collision_Check_Relative((m_direction == DIR_LEFT) ? (check_x) : (m_col_rect.m_w), 0, (m_direction == DIR_LEFT) ? (-check_x) : (check_x));

        // possible objects
        for (cObjectCollision_List::iterator itr = col_list->objects.begin(); itr != col_list->objects.end(); ++itr) {
            cObjectCollision* col = (*itr);

            // enemy item
            if (col->m_array == ARRAY_ENEMY) {
                if (col->m_obj->m_type == TYPE_ARMY) {
                    cEnemy* enemy = static_cast<cEnemy*>(col->m_obj);

                    if (enemy->m_state == STA_STAY) {
                        Get_Item(TYPE_ARMY, 0, enemy);
                        break;
                    }
                }
                else if (col->m_obj->m_type == TYPE_SHELL) {
                    cEnemy* enemy = static_cast<cEnemy*>(col->m_obj);

                    if (enemy->m_state == STA_STAY) {
                        Get_Item(TYPE_SHELL, 0, enemy);
                        break;
                    }
                }
            }
            // other items here...
        }

        delete col_list;
    }
}

void cLevel_Player::Release_Item(bool set_position /* = 1 */, bool no_action /* = 0 */)
{
    if (!m_active_object) {
        return;
    }

    ObjectDirection kick_direction = DIR_UNDEFINED;

    // if flying : opposite direction
    if (m_state == STA_FLY) {
        kick_direction = Get_Opposite_Direction(m_direction);
    }
    // if jumping
    else if (m_jump_power > 0.0f) {
        // kick it upwards
        kick_direction = DIR_UP;
    }
    // use current direction
    else {
        kick_direction = m_direction;
    }

    // add back to level
    if (m_active_object->m_type == TYPE_ARMY || m_active_object->m_type == TYPE_SHELL) {
        cArmy* army = static_cast<cArmy*>(m_active_object);

        // play kick sound if not dead
        if (!army->m_dead) {
            pAudio->Play_Sound("enemy/army/shell/hit.ogg");
        }

        // if object got kicked upwards use state stay
        if (kick_direction == DIR_UP || no_action) {
            army->m_army_state = ARMY_DEAD;
            army->Set_Army_Moving_State(ARMY_SHELL_STAND);

            // small direction acceleration if no object action
            if (kick_direction == DIR_LEFT) {
                if (no_action) {
                    army->m_direction = kick_direction;
                    army->m_velx = -5.0f;
                }
            }
            else if (kick_direction == DIR_RIGHT) {
                if (no_action) {
                    army->m_direction = kick_direction;
                    army->m_velx = 5.0f;
                }
            }
            // upwards
            else if (kick_direction == DIR_UP) {
                if (!no_action) {
                    army->m_vely = m_vely - 10.0f;
                }
            }
        }
        // default object horizontal kicking
        else {
            army->Set_Direction(kick_direction);
            army->m_army_state = ARMY_DEAD;
            army->Set_Army_Moving_State(ARMY_SHELL_RUN);

            if (army->m_direction == DIR_RIGHT) {
                army->m_velx = army->m_velx_max;
            }
            else {
                army->m_velx = -army->m_velx_max;
            }
        }

        if (!army->m_dead) {
            army->m_massive_type = MASS_MASSIVE;

            // if shell and not frozen
            if (army->m_state == STA_RUN && !army->m_freeze_counter) {
                // safe time
                army->m_player_counter = speedfactor_fps * 0.5f;
            }
        }

        army->Update_Valid_Update();
    }

    // set position
    if (set_position) {
        // set start position
        // center it horizontally
        float start_posx = m_pos_x + m_col_pos.m_x + (m_active_object->m_col_rect.m_w / 2) - (m_col_rect.m_w / 2);
        float start_posy = m_pos_y + m_col_pos.m_y;

        // put in front if left or right
        if (kick_direction == DIR_LEFT) {
            start_posx -= m_col_rect.m_w + 9.0f;
        }
        else if (kick_direction == DIR_RIGHT) {
            start_posx += m_col_rect.m_w + 2.0f;
        }
        else if (kick_direction == DIR_UP) {
            // put on top
            start_posy += -(m_active_object->m_col_pos.m_y + m_active_object->m_col_rect.m_h) + (m_active_object->m_vely * pFramerate->m_speed_factor);
        }

        m_active_object->Set_Pos(start_posx, start_posy);

        // set step size
        float step_size = 0.0f;
        cObjectCollisionType* col_list = NULL;

        // check for a valid position to release the object
        while (step_size < 50.0f) {
            if (col_list) {
                delete col_list;
            }

            // check left side
            col_list = m_active_object->Collision_Check_Relative(step_size, 0.0f, 0.0f, 0.0f, COLLIDE_ONLY_BLOCKING);

            // collides with a blocking object on the left side
            if (!col_list->empty() && (col_list->Is_Included(ARRAY_MASSIVE) || col_list->Is_Included(ARRAY_ACTIVE) || col_list->Is_Included(ARRAY_ENEMY))) {
                delete col_list;
                // check right side
                col_list = m_active_object->Collision_Check_Relative(-step_size, 0.0f, 0.0f, 0.0f, COLLIDE_ONLY_BLOCKING);

                // also collides on the right side
                if (!col_list->empty() && (col_list->Is_Included(ARRAY_MASSIVE) || col_list->Is_Included(ARRAY_ACTIVE) || col_list->Is_Included(ARRAY_ENEMY))) {
                    // continue moving
                }
                else {
                    // move to the right
                    m_active_object->Move(-step_size, 0.0f, 1);
                    break;
                }
            }
            else {
                // move to the left
                m_active_object->Move(step_size, 0.0f, 1);
                break;
            }

            step_size += 2.0f;
        }

        // check if still blocking objects on the final position
        if (!col_list->empty() && (col_list->Is_Included(ARRAY_MASSIVE) || col_list->Is_Included(ARRAY_ACTIVE) || col_list->Is_Included(ARRAY_ENEMY))) {
            if (m_active_object->m_type == TYPE_ARMY || m_active_object->m_type == TYPE_SHELL) {
                // shell
                if (m_active_object->m_state == STA_RUN) {
                    // if collision with static blocking objects
                    if (col_list->Is_Included(ARRAY_MASSIVE) || col_list->Is_Included(ARRAY_ACTIVE)) {
                        m_active_object->DownGrade(1);
                    }
                }
                // normal
                else if (m_active_object->m_state == STA_STAY) {
                    // downgrade always
                    m_active_object->DownGrade(1);
                }
            }
        }

        delete col_list;
    }

    m_active_object->Clear_Collisions();
    m_active_object->Reset_On_Ground();

    // unset link
    m_active_object = NULL;
    // load default images
    Load_Images();
}

void cLevel_Player::Set_Type(SpriteType item_type, bool animation /* = 1 */, bool sound /* = 1 */, bool temp_power /* = 0 */)
{
    if (item_type == TYPE_PLAYER) {
        Set_Type(ALEX_SMALL, animation, sound, temp_power);
    }
    else if (item_type == TYPE_MUSHROOM_DEFAULT) {
        Set_Type(ALEX_BIG, animation, sound, temp_power);
    }
    else if (item_type == TYPE_MUSHROOM_BLUE) {
        Set_Type(ALEX_ICE, animation, sound, temp_power);
    }
    else if (item_type == TYPE_MUSHROOM_GHOST) {
        Set_Type(ALEX_GHOST, animation, sound, temp_power);
    }
    else if (item_type == TYPE_FIREPLANT) {
        Set_Type(ALEX_FIRE, animation, sound, temp_power);
    }
}

void cLevel_Player::Set_Type(Alex_type new_type, bool animation /* = 1 */, bool sound /* = 1 */, bool temp_power /* = 0 */)
{
    // already set
    if (m_alex_type == new_type) {
        return;
    }

    // play sound
    if (sound) {
        if (new_type == ALEX_BIG) {
            pAudio->Play_Sound("item/mushroom.ogg", RID_MUSHROOM);
        }
        else if (new_type == ALEX_FIRE) {
            pAudio->Play_Sound("item/fireplant.ogg", RID_FIREPLANT);
        }
        else if (new_type == ALEX_ICE) {
            pAudio->Play_Sound("item/mushroom_blue.wav", RID_MUSHROOM_BLUE);
        }
        else if (new_type == ALEX_CAPE) {
            pAudio->Play_Sound("item/feather.ogg", RID_FEATHER);
        }
        else if (new_type == ALEX_GHOST) {
            pAudio->Play_Sound("item/mushroom_ghost.ogg", RID_MUSHROOM_GHOST);
        }
    }

    if (!temp_power) {
        // was flying
        if (m_alex_type == ALEX_CAPE) {
            Stop_Flying(0);
        }
    }

    // remember old type
    Alex_type old_type = m_alex_type;

    // draw animation and set new type
    if (animation) {
        Draw_Animation(new_type);

        if (temp_power) {
            m_alex_type = old_type;
            m_alex_type_temp_power = new_type;
            // Draw_Animation ends with the new type images
            Load_Images();
        }

    }
    // only set type
    else {
        if (temp_power) {
            m_alex_type_temp_power = new_type;
        }
        else {
            m_alex_type = new_type;
            Load_Images();
        }
    }

    // nothing more needed for setting the temp power
    if (temp_power) {
        return;
    }

    // to ghost
    if (m_alex_type == ALEX_GHOST) {
        m_ghost_time = speedfactor_fps * 10;
        m_alex_type_temp_power = old_type;
    }
    // was ghost
    else if (old_type == ALEX_GHOST) {
        m_ghost_time = 0;
        m_ghost_time_mod = 0;
        m_alex_type_temp_power = ALEX_DEAD;

        // check for ghost ground
        if (m_ground_object) {
            cBaseBox* box = dynamic_cast<cBaseBox*>(m_ground_object);

            if (box) {
                // ghost box
                if (box->m_box_invisible == BOX_GHOST) {
                    Reset_On_Ground();
                }
            }
        }
    }

    // check if on ground as the size could also change
    Check_on_Ground();
}

void cLevel_Player::Set_Moving_State(Moving_state new_state)
{
    if (new_state == m_state) {
        return;
    }

    // can not change from linked state
    if (m_state == STA_OBJ_LINKED) {
        return;
    }

    // was falling
    if (m_state == STA_FALL) {
        // reset slow fall/parachute
        Parachute(0);
    }
    // was flying
    else if (m_state == STA_FLY) {
        Change_Size(0, -(m_images[ALEX_IMG_FALL].m_image->m_h - m_images[ALEX_IMG_FLY].m_image->m_h));
        Set_Image_Num(ALEX_IMG_FALL + m_direction);

        // reset flying rotation
        m_rot_z = 0.0f;
    }
    // was jumping
    else if (m_state == STA_JUMP) {
        // reset variables
        m_up_key_time = 0.0f;
        m_jump_power = 0.0f;
        m_force_jump = 0;
        m_next_jump_sound = 1;
        m_next_jump_power = 17.0f;
        m_next_jump_accel = 4.0f;
    }
    // was running
    else if (m_state == STA_RUN) {
        m_walk_time = 0.0f;
        m_running_particle_counter = 0.0f;
    }

    // # before new state is set
    if (new_state == STA_STAY) {
        if (m_ground_object) {
            m_vely = 0.0f;
        }
    }
    else if (new_state == STA_FALL) {
        Reset_On_Ground();
        m_walk_count = 0.0f;
        m_jump_power = 0.0f;
    }
    else if (new_state == STA_FLY) {
        Reset_On_Ground();
    }
    else if (new_state == STA_JUMP) {
        Reset_On_Ground();
    }
    else if (new_state == STA_CLIMB) {
        Reset_On_Ground();
        m_velx = 0.0f;
        m_vely = 0.0f;
    }

    m_state = new_state;

    // # after new state is set
    if (m_state == STA_FLY) {
        Release_Item(1);
    }
}

bool cLevel_Player::Change_Size(float x, float y, bool only_check /* = 0 */)
{
    bool valid_hor = 0;
    float check_pos = x;

    // no value
    if (Is_Float_Equal(x, 0.0f)) {
        valid_hor = 1;
    }

    while (!valid_hor) {
        cObjectCollisionType* col_list = Collision_Check_Relative(check_pos, 0.0f, 0.0f, 0.0f, COLLIDE_ONLY_BLOCKING);

        if (col_list->empty()) {
            if (!only_check) {
                Col_Move(check_pos, 0.0f, 1, 1);
            }

            valid_hor = 1;
            delete col_list;
            break;
        }

        delete col_list;

        // move to opposite direction
        if (x > 0.0f) {
            check_pos--;

            // nothing found
            if (check_pos < -x) {
                break;
            }
        }
        else {
            check_pos++;

            // nothing found
            if (check_pos > -x) {
                break;
            }
        }
    }

    bool valid_ver = 0;
    check_pos = y;

    // no value
    if (Is_Float_Equal(y, 0.0f)) {
        valid_ver = 1;
    }

    while (!valid_ver) {
        cObjectCollisionType* col_list = Collision_Check_Relative(0.0f, check_pos, 0.0f, 0.0f, COLLIDE_ONLY_BLOCKING);

        if (col_list->empty()) {
            if (!only_check) {
                Col_Move(0.0f, check_pos, 1, 1);
            }

            valid_ver = 1;
            delete col_list;
            break;
        }

        delete col_list;

        // move to opposite direction
        if (y > 0.0f) {
            check_pos--;

            // nothing found
            if (check_pos < -y) {
                break;
            }
        }
        else {
            check_pos++;

            // nothing found
            if (check_pos > -y) {
                break;
            }
        }
    }


    // if both directions valid
    if (valid_hor && valid_ver) {
        return 1;
    }

    return 0;
}

void cLevel_Player::Reset_Save(void)
{
    // reset worlds
    pOverworld_Manager->Reset();
    // unload levels
    pLevel_Manager->Unload();

    // reset player
    Set_Type(ALEX_SMALL, 0, 0);
    Reset();
    m_lives = 3;
    m_goldpieces = 0;
    m_points = 0;

    pHud_Time->Reset();
    pHud_Manager->Update_Text();
    pHud_Itembox->Reset();
}

void cLevel_Player::Reset(bool full /* = 1 */)
{
    // reset level states
    Set_Active(1);
    Reset_Position();
    Set_Direction(m_start_direction);
    m_ducked_counter = 0;
    Set_Moving_State(STA_FALL);
    Set_Image_Num(Get_Image() + m_direction);
    m_jump_power = 0.0f;
    m_jump_accel_up = 0.0f;
    m_jump_vel_deaccel = 0.0f;
    m_no_velx_counter = 0.0f;
    m_no_vely_counter = 0.0f;
    m_up_key_time = 0.0f;
    m_velx = 0.0f;
    m_vely = 0.0f;
    Clear_Collisions();
    m_walk_count = 0.0f;
    Reset_On_Ground();
    if (m_active_object) {
        m_active_object = NULL;
        Load_Images();
    }
    m_kill_multiplier = 1.0f;
    m_last_kill_counter = 0.0f;

    // reset item and camera
    if (full) {
        Ball_Clear();
        m_invincible = 0.0f;
        m_invincible_mod = 0.0f;
        m_invincible_star = 0.0f;
        Set_Color_Combine(0.0f, 0.0f, 0.0f, 0);
        pHud_Itembox->Push_back();
    }
}

void cLevel_Player::Reset_Position(void)
{
    Set_Pos(m_start_pos_x, m_start_pos_y);

    /* change player position to the current state height
     * move to correct position but do not change start position
    */
    Col_Move(0.0f, m_start_image->m_h - m_rect.m_h, 1, 0, 0);
}

void cLevel_Player::Update(void)
{
    if (editor_enabled) {
        return;
    }

    // check if got stuck
    if (!m_ducked_counter) {
        Update_Anti_Stuck();
    }

    // check if starting a jump is possible
    Update_Jump_Keytime();

    // update states
    Update_Jump();
    Update_Climbing();
    Update_Falling();
    Update_Walking();
    Update_Running();
    Update_Ducking();
    Update_Staying();
    Update_Flying();
    // throw animation counter
    if (m_throwing_counter > 0.0f) {
        m_throwing_counter -= pFramerate->m_speed_factor;

        if (m_throwing_counter < 0.0f) {
            m_throwing_counter = 0.0f;
        }
    }
    // shoot counter
    if (m_shoot_counter > 0.0f) {
        m_shoot_counter -= pFramerate->m_speed_factor;

        if (m_shoot_counter < 0.0f) {
            m_shoot_counter = 0.0f;
        }
    }
    // ghost
    if (m_ghost_time > 0.0f) {
        m_ghost_time -= pFramerate->m_speed_factor;

        // ended
        if (m_ghost_time <= 0.0f) {
            pAudio->Play_Sound("player/ghost_end.ogg", RID_MUSHROOM_GHOST);
            Set_Type(m_alex_type_temp_power, 1, 0);
        }
        // near end
        else if (m_ghost_time * 0.5f < speedfactor_fps * 3) {
            m_ghost_time_mod = (speedfactor_fps * 3) - (m_ghost_time * 0.5f);
        }
    }
    // invincible
    if (m_invincible > 0.0f) {
        m_invincible -= pFramerate->m_speed_factor;

        if (m_invincible < 0.0f) {
            m_invincible = 0.0f;
        }
        else {
            m_invincible_mod -= pFramerate->m_speed_factor * 20.0f;

            if (m_invincible_mod < 0.0f) {
                m_invincible_mod = m_invincible * 3.0f;

                if (m_invincible_mod > 180.0f) {
                    m_invincible_mod = 180.0f;
                }
            }
        }
    }
    // draw stars if in godmode or star invincible
    if (m_god_mode || m_invincible_star > 0.0f) {
        if (editor_level_enabled) {
            Set_Color_Combine(0.0f, 0.0f, 0.0f, 0);
        }
        else {
            // draw stars
            m_invincible_star_counter += pFramerate->m_speed_factor;

            if (m_invincible_star > 0.0f) {
                m_invincible_star -= pFramerate->m_speed_factor;

                if (m_invincible_star <= 0.0f) {
                    Set_Color_Combine(0.0f, 0.0f, 0.0f, 0);
                    m_invincible_star = 0.0f;
                    pAudio->Fadeout_Music(500);
                }
            }

            if (m_invincible_star_counter > 1.0f) {
                // set particle color
                Color particle_color = orange;
                particle_color.green += static_cast<Uint8>(m_invincible_mod / 5.0f);
                particle_color.blue += static_cast<Uint8>(m_invincible_mod / 1.5f);

                // create particle
                cParticle_Emitter* anim = new cParticle_Emitter(m_sprite_manager);
                anim->Set_Emitter_Rect(m_col_rect.m_x + m_col_rect.m_w * 0.1f, m_col_rect.m_y + m_col_rect.m_h * 0.1f, m_col_rect.m_w * 0.8f, m_col_rect.m_h * 0.8f);
                anim->Set_Quota(static_cast<int>(m_invincible_star_counter));
                anim->Set_Image(pVideo->Get_Package_Surface("animation/particles/star.png"));
                anim->Set_Pos_Z(m_pos_z - 0.000001f);
                anim->Set_Time_to_Live(0.3f);
                anim->Set_Fading_Alpha(1);
                anim->Set_Fading_Size(1);
                anim->Set_Const_Rotation_Z(-5.0f, 10.0f);
                // godmode stars
                if (m_god_mode) {
                    anim->Set_Direction_Range(180.0f, 180.0f);
                    anim->Set_Speed(3.5f, 2.5f);
                    anim->Set_Scale(0.3f);
                    anim->Set_Color(lightblue);
                }
                // default
                else {
                    anim->Set_Time_to_Live(2.0f);
                    anim->Set_Speed(1.0f, 1.5f);
                    anim->Set_Scale(0.2f);
                    anim->Set_Color(particle_color);
                }

                anim->Set_Blending(BLEND_ADD);
                anim->Emit();
                pActive_Animation_Manager->Add(anim);

                m_invincible_star_counter -= static_cast<int>(m_invincible_star_counter);
            }
        }
    }

    // update active item
    Update_Item();

    // image counter
    if (m_state == STA_WALK || (m_alex_type != ALEX_CAPE && m_state == STA_RUN)) {
        // 4 frames
        if (m_alex_type == ALEX_SMALL) {
            m_walk_count += pFramerate->m_speed_factor * 0.35f;
        }
        // 4 frames
        else if (m_alex_type == ALEX_BIG) {
            m_walk_count += pFramerate->m_speed_factor * 0.3f;
        }
        // 4 frames
        else {
            m_walk_count += pFramerate->m_speed_factor * 0.3f;
        }

        // ground type modification
        float vel = (m_velx > 0.0f ? m_velx : -m_velx);

        if (vel && m_ground_object && m_ground_object->m_image) {
            float ground_mod = 0.0f;

            switch (m_ground_object->m_image->m_ground_type) {
            case GROUND_ICE: {
                ground_mod = 0.125f;
                break;
            }
            case GROUND_SAND: {
                ground_mod = 0.075f;
                break;
            }
            case GROUND_PLASTIC: {
                ground_mod = 0.03f;
                break;
            }
            default: {
                break;
            }
            }

            if (ground_mod) {
                m_walk_count += ground_mod * (5.0f / vel) * pFramerate->m_speed_factor;
            }
        }
    }
    else if (m_state == STA_RUN) {
        // ? frames
        if (m_alex_type == ALEX_SMALL) {
            //m_walk_count += pFramerate->m_speed_factor * 0.35f;
        }
        // ? frames
        else if (m_alex_type == ALEX_BIG) {
            //m_walk_count += pFramerate->m_speed_factor * 0.3f;
        }
        // 2 frames
        else if (m_alex_type == ALEX_CAPE) {
            m_walk_count += pFramerate->m_speed_factor * 0.35f;
        }
        // ? frames
        else {
            //m_walk_count += pFramerate->m_speed_factor * 0.3f;
        }
    }
    /*else if( state == FALL || state == JUMP )
    {
        m_walk_count += pFramerate->m_speed_factor * 0.3f;
    }*/
    else if (m_state == STA_CLIMB) {
        if (!Is_Float_Equal(m_vely, 0.0f) || !Is_Float_Equal(m_velx, 0.0f)) {
            m_walk_count += pFramerate->m_speed_factor * 0.7f;
        }
    }
    else if (m_state == STA_FLY) {
        m_walk_count += pFramerate->m_speed_factor * 0.6f;
    }

    // add x velocity
    if (m_state == STA_WALK || m_state == STA_RUN || m_state == STA_CLIMB) {
        if (m_velx > 0.0f) {
            m_walk_count += (m_velx * 0.05f) * pFramerate->m_speed_factor;
        }
        else if (m_velx < 0.0f) {
            m_walk_count += (-m_velx * 0.05f) * pFramerate->m_speed_factor;
        }
    }

    if (m_state == STA_WALK || m_state == STA_STAY || (m_alex_type != ALEX_CAPE && m_state == STA_RUN)) {
        // 4 frames
        if (m_alex_type == ALEX_SMALL) {
            if (m_walk_count >= 8.0f) {
                m_walk_count = 0.0f;
            }
        }
        // 4 frames
        else if (m_alex_type == ALEX_BIG) {
            if (m_walk_count >= 8.0f) {
                m_walk_count = 0.0f;
            }
        }
        // 4 frames
        else {
            if (m_walk_count >= 8.0f) {
                m_walk_count = 0.0f;
            }
        }
    }
    else if (m_state == STA_RUN) {
        // ? frames
        if (m_alex_type == ALEX_SMALL) {
        }
        // ? frames
        else if (m_alex_type == ALEX_BIG) {
        }
        // 2 frames
        else if (m_alex_type == ALEX_CAPE) {
            if (m_walk_count >= 4.0f) {
                m_walk_count = 0.0f;
            }
        }
        // ? frames
        else {
        }
    }
    /*else if( m_state == STA_FALL || m_state == STA_JUMP )
    {
        if( m_walk_count > 6.0f )
        {
            m_walk_count = 6.0f;
        }
    }*/
    else if (m_state == STA_FLY) {
        // 4 frames
        if (m_walk_count > 8.0f) {
            m_walk_count = 0.0f;
        }
    }
    else {
        // 4 frames
        if (m_walk_count >= 8.0f) {
            m_walk_count = 0.0f;
        }
    }

    // Set image
    if (m_state != STA_CLIMB) {
        Set_Image_Num(Get_Image() + m_direction);
    }
    else {
        Set_Image_Num(Get_Image());
    }

    // Special
    Update_Kill_Multiplier();
}

void cLevel_Player::Draw(cSurface_Request* request /* = NULL */)
{
    if (!m_valid_draw) {
        return;
    }

    // invincible
    if (m_invincible > 0.0f) {
        // star invincible
        if (m_invincible_star > 0.0f) {
            Set_Color_Combine(m_invincible_mod / 130, m_invincible_mod / 130, m_invincible_mod / 180, GL_ADD);
        }
        // default invincible
        else {
            Set_Color(255, 255, 255, 255 - static_cast<Uint8>(m_invincible_mod));
        }
    }
    // ghost
    if (m_ghost_time > 0.0f) {
        // ghost shadows
        m_color.alpha = 32 + static_cast<int>(m_ghost_time_mod);
        float old_posx = m_pos_x;
        float old_posy = m_pos_y;
        float old_posz = m_pos_z;

        for (unsigned int i = 0; i < 5; i++) {
            m_color.alpha -= 5;
            m_pos_x -= m_velx * 0.2f + Get_Random_Float(0, 1);
            m_pos_y -= m_vely * 0.2f + Get_Random_Float(0, 1);
            m_pos_z -= 0.000001f;

            cMovingSprite::Draw(request);
        }

        // set original values
        m_pos_x = old_posx;
        m_pos_y = old_posy;
        m_pos_z = old_posz;
        m_color.alpha = 48 + static_cast<int>(m_ghost_time_mod);
    }

    cMovingSprite::Draw(request);

    if (m_invincible > 0.0f || m_ghost_time > 0.0f) {
        Set_Color(white);
    }

    if (!editor_enabled) {
        // draw debug rect
        if (game_debug) {
            // if on ground
            if (m_ground_object) {
                // create request
                cRect_Request* rect_request = new cRect_Request();
                // draw
                pVideo->Draw_Rect(&m_ground_object->m_col_rect, m_pos_z + 0.000009f, &grey, rect_request);
                rect_request->m_no_camera = 0;
                // blending
                rect_request->m_blend_sfactor = GL_SRC_COLOR;
                rect_request->m_blend_dfactor = GL_DST_ALPHA;

                // add request
                pRenderer->Add(rect_request);
            }
        }
    }
}

void cLevel_Player::Draw_Animation(Alex_type new_mtype)
{
    // already set or invalid
    if (new_mtype == m_alex_type || m_alex_type == ALEX_DEAD || new_mtype == ALEX_DEAD) {
        return;
    }

    Alex_type alex_type_old = m_alex_type;
    bool parachute_old = m_parachute;

    float posx_old = m_pos_x;
    float posy_old = m_pos_y;

    // Change_Size needs new state size
    m_alex_type = alex_type_old;
    Parachute(parachute_old);
    Load_Images();

    // correct position for bigger alex
    if (alex_type_old == ALEX_SMALL && (new_mtype == ALEX_BIG || new_mtype == ALEX_FIRE || new_mtype == ALEX_ICE || new_mtype == ALEX_CAPE || new_mtype == ALEX_GHOST)) {
        Change_Size(-5.0f, -12.0f);
    }
    // correct position for small alex
    else if ((alex_type_old == ALEX_BIG || alex_type_old == ALEX_FIRE || alex_type_old == ALEX_ICE || new_mtype == ALEX_CAPE || alex_type_old == ALEX_GHOST) && new_mtype == ALEX_SMALL) {
        Change_Size(5.0f, 12.0f);
    }

    float posx_new = m_pos_x;
    float posy_new = m_pos_y;

    // draw animation
    for (unsigned int i = 0; i < 7; i++) {
        // set to current type
        if (i % 2) {
            m_alex_type = alex_type_old;
            Parachute(parachute_old);
            Load_Images();

            Set_Pos(posx_old, posy_old);
        }
        // set to new type
        else {
            m_alex_type = new_mtype;
            if (new_mtype != ALEX_CAPE) {
                Parachute(0);
            }
            Load_Images();

            // always set the ghost type to draw the ghost rect until it ends
            if (i < 6 && alex_type_old == ALEX_GHOST) {
                m_alex_type = alex_type_old;
            }

            Set_Pos(posx_new, posy_new);
        }

        // draw
        Draw_Game();
        pVideo->Render();

        // frame delay
        SDL_Delay(120);
    }

    pFramerate->Reset();
}

unsigned int cLevel_Player::Get_Image(void) const
{
    // throwing
    if (m_throwing_counter && (m_alex_type == ALEX_FIRE || m_alex_type == ALEX_ICE) && !m_ducked_counter && (m_state == STA_FALL || m_state == STA_STAY || m_state == STA_WALK || m_state == STA_RUN || m_state == STA_JUMP)) {
        int imgnum = 0;

        if (m_throwing_counter < speedfactor_fps * 0.2f) {
            imgnum = 2;
        }

        return ALEX_IMG_THROW + imgnum;
    }

    // ducked
    if (m_ducked_counter && (m_state == STA_STAY || m_state == STA_WALK || m_state == STA_RUN || m_state == STA_JUMP || m_state == STA_FALL)) {
        return ALEX_IMG_DUCK;
    }

    // parachute
    if (m_parachute && m_state == STA_FALL) {
        return ALEX_IMG_SPECIAL_1;
    }

    if (m_state == STA_STAY || m_state == STA_WALK || (m_alex_type != ALEX_CAPE && m_state == STA_RUN)) {
        unsigned int imgnum = static_cast<unsigned int>(m_walk_count);

        for (unsigned int i = 0; i < imgnum; i++) {
            if (imgnum % 2) {
                imgnum--;
            }
        }

        return ALEX_IMG_WALK + imgnum;
    }
    else if (m_state == STA_RUN) {
        unsigned int imgnum = static_cast<unsigned int>(m_walk_count);

        for (unsigned int i = 0; i < imgnum; i++) {
            if (imgnum % 2) {
                imgnum--;
            }
        }

        return ALEX_IMG_RUN + imgnum;
    }
    else if (m_state == STA_FALL) {
        /*unsigned int imgnum = static_cast<unsigned int>(m_walk_count);

        for( unsigned int i = 0; i < imgnum; i++ )
        {
            if( imgnum % 2 )
            {
                imgnum--;
            }
        }

        return ALEX_IMG_FALL + imgnum;*/
        return ALEX_IMG_FALL;
    }
    /*else if( m_state == STAY )
    {
        return ALEX_IMG_STAND;
    }*/
    else if (m_state == STA_JUMP) {
        /*unsigned int imgnum = static_cast<unsigned int>(m_walk_count);

        for( unsigned int i = 0; i < imgnum; i++ )
        {
            if( imgnum % 2 )
            {
                imgnum--;
            }
        }

        return ALEX_IMG_JUMP + imgnum;*/
        return ALEX_IMG_JUMP;
    }
    else if (m_state == STA_FLY) {
        unsigned int imgnum = static_cast<unsigned int>(m_walk_count);

        for (unsigned int i = 0; i < imgnum; i++) {
            if (imgnum % 2) {
                imgnum--;
            }
        }

        return ALEX_IMG_FLY + imgnum;
    }
    else if (m_state == STA_CLIMB) {
        if (m_walk_count > 4) {
            return ALEX_IMG_CLIMB;
        }
        else {
            return ALEX_IMG_CLIMB + 1;
        }
    }

    return ALEX_IMG_STAND;
}

void cLevel_Player::Load_Images(void)
{
    // not valid
    if (m_alex_type == ALEX_DEAD) {
        return;
    }

    Clear_Images();

    // special alex images state
    std::string special_state;
    // if holding item
    if (m_active_object) {
        special_state = "_holding";
    }

    if (m_alex_type == ALEX_SMALL) {
        /********************* Small **************************/
        // standing
        Add_Image(pVideo->Get_Package_Surface("alex/small/stand_left" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/small/stand_right" + special_state + ".png"));
        // walking
        Add_Image(pVideo->Get_Package_Surface("alex/small/walk_left_1" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/small/walk_right_1" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/small/walk_left_2" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/small/walk_right_2" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/small/walk_left_1" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/small/walk_right_1" + special_state + ".png"));
        // running
        Add_Image(NULL);
        Add_Image(NULL);
        Add_Image(NULL);
        Add_Image(NULL);
        // falling
        Add_Image(pVideo->Get_Package_Surface("alex/small/fall_left" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/small/fall_right" + special_state + ".png"));
        // jumping
        Add_Image(pVideo->Get_Package_Surface("alex/small/jump_left" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/small/jump_right" + special_state + ".png"));
        // dead
        Add_Image(pVideo->Get_Package_Surface("alex/small/dead_left.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/small/dead_right.png"));
        // ducked
        Add_Image(pVideo->Get_Package_Surface("alex/small/duck_left.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/small/duck_right.png"));
        // climbing
        Add_Image(pVideo->Get_Package_Surface("alex/small/climb_left.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/small/climb_right.png"));
        /****************************************************/
    }
    else if (m_alex_type == ALEX_BIG) {
        /********************* Big ****************************/
        // standing
        Add_Image(pVideo->Get_Package_Surface("alex/big/stand_left" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/big/stand_right" + special_state + ".png"));
        // walking
        Add_Image(pVideo->Get_Package_Surface("alex/big/walk_left_1" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/big/walk_right_1" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/big/walk_left_2" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/big/walk_right_2" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/big/walk_left_1" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/big/walk_right_1" + special_state + ".png"));
        // running
        Add_Image(NULL);
        Add_Image(NULL);
        Add_Image(NULL);
        Add_Image(NULL);
        // falling
        Add_Image(pVideo->Get_Package_Surface("alex/big/fall_left" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/big/fall_right" + special_state + ".png"));
        // jumping
        Add_Image(pVideo->Get_Package_Surface("alex/big/jump_left" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/big/jump_right" + special_state + ".png"));
        // dead
        Add_Image(pVideo->Get_Package_Surface("alex/small/dead_left.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/small/dead_right.png"));
        // ducked
        Add_Image(pVideo->Get_Package_Surface("alex/big/duck_left.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/big/duck_right.png"));
        // climbing
        Add_Image(pVideo->Get_Package_Surface("alex/big/climb_left.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/big/climb_right.png"));
        // throwing
        Add_Image(NULL);
        Add_Image(NULL);
        Add_Image(NULL);
        Add_Image(NULL);
        /****************************************************/
    }
    else if (m_alex_type == ALEX_FIRE) {
        /********************* Fire **************************/
        // standing
        Add_Image(pVideo->Get_Package_Surface("alex/fire/stand_left" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/fire/stand_right" + special_state + ".png"));
        // walking
        Add_Image(pVideo->Get_Package_Surface("alex/fire/walk_left_1" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/fire/walk_right_1" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/fire/walk_left_2" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/fire/walk_right_2" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/fire/walk_left_1" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/fire/walk_right_1" + special_state + ".png"));
        // running
        Add_Image(NULL);
        Add_Image(NULL);
        Add_Image(NULL);
        Add_Image(NULL);
        // falling
        Add_Image(pVideo->Get_Package_Surface("alex/fire/fall_left" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/fire/fall_right" + special_state + ".png"));
        // jumping
        Add_Image(pVideo->Get_Package_Surface("alex/fire/jump_left" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/fire/jump_right" + special_state + ".png"));
        // dead
        Add_Image(pVideo->Get_Package_Surface("alex/small/dead_left.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/small/dead_right.png"));
        // ducked
        Add_Image(pVideo->Get_Package_Surface("alex/fire/duck_left.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/fire/duck_right.png"));
        // climbing
        Add_Image(pVideo->Get_Package_Surface("alex/fire/climb_left.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/fire/climb_right.png"));
        // throwing
        Add_Image(pVideo->Get_Package_Surface("alex/fire/throw_left_1.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/fire/throw_right_1.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/fire/throw_left_2.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/fire/throw_right_2.png"));
        /****************************************************/
    }
    else if (m_alex_type == ALEX_ICE) {
        /********************* Ice **************************/
        // standing
        Add_Image(pVideo->Get_Package_Surface("alex/ice/stand_left" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/ice/stand_right" + special_state + ".png"));
        // walking
        Add_Image(pVideo->Get_Package_Surface("alex/ice/walk_left_1" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/ice/walk_right_1" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/ice/walk_left_2" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/ice/walk_right_2" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/ice/walk_left_1" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/ice/walk_right_1" + special_state + ".png"));
        // running
        Add_Image(NULL);
        Add_Image(NULL);
        Add_Image(NULL);
        Add_Image(NULL);
        // falling
        Add_Image(pVideo->Get_Package_Surface("alex/ice/fall_left" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/ice/fall_right" + special_state + ".png"));
        // jumping
        Add_Image(pVideo->Get_Package_Surface("alex/ice/jump_left" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/ice/jump_right" + special_state + ".png"));
        // dead
        Add_Image(pVideo->Get_Package_Surface("alex/small/dead_left.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/small/dead_right.png"));
        // ducked
        Add_Image(pVideo->Get_Package_Surface("alex/ice/duck_left.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/ice/duck_right.png"));
        // climbing
        Add_Image(pVideo->Get_Package_Surface("alex/ice/climb_left.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/ice/climb_right.png"));
        // throwing
        Add_Image(pVideo->Get_Package_Surface("alex/ice/throw_left_1.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/ice/throw_right_1.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/ice/throw_left_2.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/ice/throw_right_2.png"));
        /****************************************************/
    }
    else if (m_alex_type == ALEX_CAPE) {
        /********************* Cape **************************/
        // standing
        Add_Image(pVideo->Get_Package_Surface("alex/flying/left" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/flying/right" + special_state + ".png"));
        // walking
        Add_Image(pVideo->Get_Package_Surface("alex/flying/walk_left_1" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/flying/walk_right_1" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/flying/walk_left_2" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/flying/walk_right_2" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/flying/walk_left_1" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/flying/walk_right_1" + special_state + ".png"));
        // running
        Add_Image(pVideo->Get_Package_Surface("alex/flying/run_left_1" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/flying/run_right_1" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/flying/run_left_2" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/flying/run_right_2" + special_state + ".png"));
        // falling
        Add_Image(pVideo->Get_Package_Surface("alex/flying/fall_left" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/flying/fall_right" + special_state + ".png"));
        // jumping
        Add_Image(pVideo->Get_Package_Surface("alex/flying/jump_left" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/flying/jump_right" + special_state + ".png"));
        // dead
        Add_Image(pVideo->Get_Package_Surface("alex/small/dead_left.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/small/dead_right.png"));
        // ducked
        Add_Image(pVideo->Get_Package_Surface("alex/flying/duck_left.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/flying/duck_right.png"));
        // climbing
        Add_Image(pVideo->Get_Package_Surface("alex/flying/climb_left.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/flying/climb_right.png"));
        // throwing
        Add_Image(NULL);
        Add_Image(NULL);
        Add_Image(NULL);
        Add_Image(NULL);
        // flying
        Add_Image(pVideo->Get_Package_Surface("alex/flying/fly_left_1.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/flying/fly_right_1.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/flying/fly_left_2.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/flying/fly_right_2.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/flying/fly_left_3.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/flying/fly_right_3.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/flying/fly_left_4.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/flying/fly_right_4.png"));
        // slow fall/parachute
        Add_Image(pVideo->Get_Package_Surface("alex/flying/slow_fall_left.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/flying/slow_fall_right.png"));
        /****************************************************/
    }
    else if (m_alex_type == ALEX_GHOST) {
        /********************* Ghost **************************/
        // standing
        Add_Image(pVideo->Get_Package_Surface("alex/ghost/stand_left" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/ghost/stand_right" + special_state + ".png"));
        // walking
        Add_Image(pVideo->Get_Package_Surface("alex/ghost/walk_left_1" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/ghost/walk_right_1" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/ghost/walk_left_2" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/ghost/walk_right_2" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/ghost/walk_left_1" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/ghost/walk_right_1" + special_state + ".png"));
        // running
        Add_Image(NULL);
        Add_Image(NULL);
        Add_Image(NULL);
        Add_Image(NULL);
        // falling
        Add_Image(pVideo->Get_Package_Surface("alex/ghost/fall_left" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/ghost/fall_right" + special_state + ".png"));
        // jumping
        Add_Image(pVideo->Get_Package_Surface("alex/ghost/jump_left" + special_state + ".png"));
        Add_Image(pVideo->Get_Package_Surface("alex/ghost/jump_right" + special_state + ".png"));
        // dead
        Add_Image(pVideo->Get_Package_Surface("alex/small/dead_left.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/small/dead_right.png"));
        // ducked
        Add_Image(pVideo->Get_Package_Surface("alex/ghost/duck_left.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/ghost/duck_right.png"));
        // climbing
        Add_Image(pVideo->Get_Package_Surface("alex/ghost/climb_left.png"));
        Add_Image(pVideo->Get_Package_Surface("alex/ghost/climb_right.png"));
        // throwing
        Add_Image(NULL);
        Add_Image(NULL);
        Add_Image(NULL);
        Add_Image(NULL);
        /****************************************************/
    }

    // set image
    Set_Image_Num(Get_Image() + m_direction);
}

void cLevel_Player::Get_Item(SpriteType item_type, bool force /* = 0 */, cMovingSprite* base /* = NULL */)
{
    Alex_type current_alex_type;
    bool use_temp_power;

    // use original type
    if (m_alex_type == ALEX_GHOST) {
        current_alex_type = m_alex_type_temp_power;
        use_temp_power = 1;
    }
    // already using original type
    else {
        current_alex_type = m_alex_type;
        use_temp_power = 0;
    }

    // Default Mushroom
    if (item_type == TYPE_MUSHROOM_DEFAULT) {
        if ((current_alex_type == ALEX_SMALL || force) && Change_Size(-5, -12, 1)) {
            // change to big
            if (current_alex_type == ALEX_SMALL) {
                // set type
                Set_Type(ALEX_BIG, 1, 1, use_temp_power);
            }
            // move item to itembox
            else if (current_alex_type == ALEX_BIG) {
                // item to itembox
                pHud_Itembox->Set_Item(TYPE_MUSHROOM_DEFAULT);
            }
            // change to big
            else if (current_alex_type == ALEX_FIRE) {
                // set type
                Set_Type(ALEX_BIG, 0, 1, use_temp_power);
                // old item to itembox
                pHud_Itembox->Set_Item(TYPE_FIREPLANT);
            }
        }
        // move item to itembox
        else {
            pHud_Itembox->Set_Item(TYPE_MUSHROOM_DEFAULT);
        }
    }
    // Fireplant
    else if (item_type == TYPE_FIREPLANT) {
        if (((current_alex_type == ALEX_SMALL || force) && Change_Size(-5, -12, 1)) ||
                (current_alex_type == ALEX_BIG || current_alex_type == ALEX_ICE || force)) {
            // move item to itembox
            if (current_alex_type == ALEX_FIRE) {
                // move item to itembox
                pHud_Itembox->Set_Item(TYPE_FIREPLANT);
            }
            // change to fire
            else {
                // set type
                Set_Type(ALEX_FIRE, 1, 1, use_temp_power);
            }
        }
        // fire explosion
        else if (current_alex_type == ALEX_FIRE && pHud_Itembox->m_item_id == TYPE_FIREPLANT) {
            unsigned int ball_amount = 10;

            // if star add another ball
            if (m_invincible_star) {
                ball_amount = 20;
            }

            Ball_Add(FIREBALL_EXPLOSION, 180, ball_amount);
            pHud_Points->Add_Points(1000, m_pos_x + (m_col_rect.m_w / 2), m_pos_y + 2);
        }
        // move item to itembox
        else {
            pHud_Itembox->Set_Item(TYPE_FIREPLANT);
        }
    }
    // Blue Mushroom
    else if (item_type == TYPE_MUSHROOM_BLUE) {
        if (((current_alex_type == ALEX_SMALL || force) && Change_Size(-5, -20, 1)) ||
                ((current_alex_type == ALEX_BIG || current_alex_type == ALEX_FIRE || force))) {
            // move item to itembox
            if (current_alex_type == ALEX_ICE) {
                // move item to itembox
                pHud_Itembox->Set_Item(TYPE_MUSHROOM_BLUE);
            }
            // change to ice
            else {
                // set type
                Set_Type(ALEX_ICE, 1, 1, use_temp_power);
            }
        }
        // ice explosion
        else if (current_alex_type == ALEX_ICE && pHud_Itembox->m_item_id == TYPE_MUSHROOM_BLUE) {
            unsigned int ball_amount = 10;

            // if star add another ball
            if (m_invincible_star) {
                ball_amount = 20;
            }

            Ball_Add(ICEBALL_EXPLOSION, 180, ball_amount);
            pHud_Points->Add_Points(1000, m_pos_x + (m_col_rect.m_w / 2), m_pos_y + 2);
        }
        // move item to itembox
        else {
            pHud_Itembox->Set_Item(TYPE_MUSHROOM_BLUE);
        }
    }
    // Ghost Mushroom
    else if (item_type == TYPE_MUSHROOM_GHOST) {
        if (((m_alex_type == ALEX_SMALL || force) && Change_Size(-5, -20, 1)) ||
                m_alex_type != ALEX_SMALL) {
            // set back ghost time
            if (m_alex_type == ALEX_GHOST) {
                m_ghost_time = speedfactor_fps * 10;
                m_ghost_time_mod = 0;
            }
            // change to ghost
            else {
                // set type
                Set_Type(ALEX_GHOST, 1, 1);
            }
        }
        // move item to itembox
        else {
            pHud_Itembox->Set_Item(TYPE_MUSHROOM_GHOST);
        }
    }
    // Mushroom 1-UP
    else if (item_type == TYPE_MUSHROOM_LIVE_1) {
        pAudio->Play_Sound("item/live_up.ogg", RID_1UP_MUSHROOM);
        pHud_Lives->Add_Lives(1);
    }
    // Mushroom Poison
    else if (item_type == TYPE_MUSHROOM_POISON) {
        DownGrade_Player();
    }
    // Moon
    else if (item_type == TYPE_MOON) {
        pAudio->Play_Sound("item/moon.ogg", RID_MOON);
        pHud_Lives->Add_Lives(3);
    }
    // Star
    else if (item_type == TYPE_STAR) {
        // todo : check if music is already playing
        pAudio->Play_Music("game/star.ogg", 0, 1, 500);
        pAudio->Play_Music(pActive_Level->m_musicfile, -1, 0);
        pHud_Points->Add_Points(1000, m_pos_x + (m_col_rect.m_w / 2), m_pos_y + 2);
        m_invincible = speedfactor_fps * 16.0f;
        m_invincible_star = speedfactor_fps * 15.0f;
    }
    // Armadillo Shell
    else if (item_type == TYPE_ARMY || item_type == TYPE_SHELL) {
        pAudio->Play_Sound("player/pickup_item.wav");

        m_active_object = base;
        m_active_object->m_massive_type = MASS_PASSIVE;
        m_active_object->m_state = STA_OBJ_LINKED;
        m_active_object->Update_Valid_Update();
        m_active_object->Reset_On_Ground();
        m_active_object->m_velx = 0.0f;
        m_active_object->m_vely = 0.0f;


        cArmy* army = static_cast<cArmy*>(m_active_object);  // shells are armys by inheritance anyway
        // clear the standing counter
        army->m_counter = 0.0f;
        // clear player counter
        army->m_player_counter = 0.0f;
        army->Set_Image_Num(5 + 5);

        // load holding images
        Load_Images();
    }
}

float cLevel_Player::Get_Vel_Modifier(void) const
{
    float vel_mod = 1.0f;

    // if running key is pressed or always run
    if (pPreferences->m_always_run || pKeyboard->m_keys[pPreferences->m_key_action] || pJoystick->Button(pPreferences->m_joy_button_action)) {
        vel_mod = 1.5f;
    }

    if (m_invincible_star > 0.0f) {
        vel_mod *= 1.2f;
    }

    if (m_state == STA_RUN) {
        vel_mod *= 1.2f;
    }

    return vel_mod;
}

void cLevel_Player::Action_Jump(bool enemy_jump /* = 0 */)
{
    if (m_ducked_counter) {
        // power jump
        if (m_ducked_counter > power_jump_delta) {
            m_force_jump = 1;
            m_next_jump_power += 2.0f;
            m_next_jump_accel += 0.2f;
        }
        // stop ducking after setting power jump
        Stop_Ducking();
    }

    // enemy jump
    if (enemy_jump) {
        m_force_jump = 1;
        m_next_jump_sound = 0;
        m_next_jump_power += 1.0f;
        m_next_jump_accel += 0.1f;
    }

    // start keytime
    Start_Jump_Keytime();
    // check if starting a jump is possible
    Update_Jump_Keytime();
}

void cLevel_Player::Action_Interact(input_identifier key_type)
{
    // Up
    if (key_type == INP_UP) {
        // Search for colliding level exit objects
        for (cSprite_List::iterator itr = m_sprite_manager->objects.begin(); itr != m_sprite_manager->objects.end(); ++itr) {
            cSprite* obj = (*itr);

            // skip destroyed objects
            if (obj->m_auto_destroy) {
                continue;
            }

            if (!m_col_rect.Intersects(obj->m_col_rect)) {
                continue;
            }

            // level exit
            if (obj->m_type == TYPE_LEVEL_EXIT) {
                cLevel_Exit* level_exit = static_cast<cLevel_Exit*>(obj);

                // beam
                if (level_exit->m_exit_type == LEVEL_EXIT_BEAM) {
                    // needs to be on ground
                    if (m_ground_object) {
                        Game_Action = GA_ACTIVATE_LEVEL_EXIT;
                        Game_Action_ptr = level_exit;
                    }
                }
                // warp
                else if (level_exit->m_exit_type == LEVEL_EXIT_WARP) {
                    if (level_exit->m_direction == DIR_UP) {
                        if (m_vely <= 0) {
                            Game_Action = GA_ACTIVATE_LEVEL_EXIT;
                            Game_Action_ptr = level_exit;
                        }
                    }
                }

                // if leaving level
                if (level_exit->m_dest_level.empty() && level_exit->m_dest_entry.empty()) {
                    Game_Action_Data_Start.add("music_fadeout", "1000");
                }

                return;
            }
            // climbable
            else if (obj->m_massive_type == MASS_CLIMBABLE) {
                Start_Climbing();
            }
        }
    }
    // Down
    else if (key_type == INP_DOWN) {
        // Search for colliding level exit objects
        for (cSprite_List::iterator itr = m_sprite_manager->objects.begin(); itr != m_sprite_manager->objects.end(); ++itr) {
            cSprite* obj = (*itr);

            // skip destroyed objects
            if (obj->m_auto_destroy) {
                continue;
            }

            if (!m_col_rect.Intersects(obj->m_col_rect)) {
                continue;
            }

            // level exit
            if (obj->m_type == TYPE_LEVEL_EXIT) {
                cLevel_Exit* level_exit = static_cast<cLevel_Exit*>(obj);

                // warp
                if (level_exit->m_exit_type == LEVEL_EXIT_WARP) {
                    if (level_exit->m_direction == DIR_DOWN) {
                        // needs to be on ground
                        if (m_ground_object) {
                            Game_Action = GA_ACTIVATE_LEVEL_EXIT;
                            Game_Action_ptr = level_exit;

                            // if leaving level
                            if (level_exit->m_dest_level.empty() && level_exit->m_dest_entry.empty()) {
                                Game_Action_Data_Start.add("music_fadeout", "1000");
                            }
                            return;
                        }
                    }
                }
            }
        }

        // ducking / falling
        if (m_state != STA_FLY) {
            if (m_ground_object) {
                if (m_ground_object->m_massive_type == MASS_MASSIVE) {
                    Start_Ducking();
                }
                else if (m_ground_object->m_massive_type == MASS_HALFMASSIVE) {
                    Start_Falling();
                }
            }
        }
    }
    // Left
    else if (key_type == INP_LEFT) {
        // Search for colliding level exit objects
        for (cSprite_List::iterator itr = m_sprite_manager->objects.begin(); itr != m_sprite_manager->objects.end(); ++itr) {
            cSprite* obj = (*itr);

            // skip destroyed objects
            if (obj->m_auto_destroy) {
                continue;
            }

            if (!m_col_rect.Intersects(obj->m_col_rect)) {
                continue;
            }

            // level exit
            if (obj->m_type == TYPE_LEVEL_EXIT) {
                cLevel_Exit* level_exit = static_cast<cLevel_Exit*>(obj);

                // warp
                if (level_exit->m_exit_type == LEVEL_EXIT_WARP) {
                    if (level_exit->m_direction == DIR_LEFT) {
                        if (m_velx >= 0) {
                            Game_Action = GA_ACTIVATE_LEVEL_EXIT;
                            Game_Action_ptr = level_exit;
                            // if leaving level
                            if (level_exit->m_dest_level.empty() && level_exit->m_dest_entry.empty()) {
                                Game_Action_Data_Start.add("music_fadeout", "1000");
                            }
                            return;
                        }
                    }
                }
            }
        }

        // direction
        if (m_state != STA_FLY) {
            if (m_direction != DIR_LEFT) {
                // play stop sound if already running
                if (m_velx > 12.0f && m_ground_object) {
                    pAudio->Play_Sound("player/run_stop.ogg", RID_ALEX_STOP);
                }

                m_direction = DIR_LEFT;
            }
        }
    }
    // Right
    else if (key_type == INP_RIGHT) {
        // Search for colliding level exit objects
        for (cSprite_List::iterator itr = m_sprite_manager->objects.begin(); itr != m_sprite_manager->objects.end(); ++itr) {
            cSprite* obj = (*itr);

            // skip destroyed objects
            if (obj->m_auto_destroy) {
                continue;
            }

            if (!m_col_rect.Intersects(obj->m_col_rect)) {
                continue;
            }

            // level exit
            if (obj->m_type == TYPE_LEVEL_EXIT) {
                cLevel_Exit* level_exit = static_cast<cLevel_Exit*>(obj);

                // warp
                if (level_exit->m_exit_type == LEVEL_EXIT_WARP) {
                    if (level_exit->m_direction == DIR_RIGHT) {
                        if (m_velx <= 0) {
                            Game_Action = GA_ACTIVATE_LEVEL_EXIT;
                            Game_Action_ptr = level_exit;
                            // if leaving level
                            if (level_exit->m_dest_level.empty() && level_exit->m_dest_entry.empty()) {
                                Game_Action_Data_Start.add("music_fadeout", "1000");
                            }
                            return;
                        }
                    }
                }
            }
        }

        // direction
        if (m_state != STA_FLY) {
            if (m_direction != DIR_RIGHT) {
                // play stop sound if already running
                if (m_velx < -12.0f && m_ground_object) {
                    pAudio->Play_Sound("player/run_stop.ogg", RID_ALEX_STOP);
                }

                m_direction = DIR_RIGHT;
            }
        }
    }
    // Shoot
    else if (key_type == INP_SHOOT) {
        Action_Shoot();
    }
    // Jump
    else if (key_type == INP_JUMP) {
        Action_Jump();
    }
    // Request Item
    else if (key_type == INP_ITEM) {
        pHud_Itembox->Request_Item();
    }
    // Exit
    else if (key_type == INP_EXIT) {
        Game_Action = GA_ENTER_MENU;
        Game_Action_Data_Middle.add("menu_exit_back_to", int_to_string(MODE_LEVEL));

        if (Game_Mode_Type == MODE_TYPE_LEVEL_CUSTOM) {
            Game_Action_Data_Middle.add("load_menu", int_to_string(MENU_START));
            Game_Action_Data_Middle.add("menu_start_current_level", path_to_utf8(Trim_Filename(pActive_Level->m_level_filename, 0, 0)));
        }
        else {
            Game_Action_Data_Middle.add("load_menu", int_to_string(MENU_MAIN));
        }
    }
}

void cLevel_Player::Action_Shoot(void)
{
    // add fire or ice-ball
    ball_effect ball_type = FIREBALL_DEFAULT;
    // if ice alex
    if (m_alex_type == ALEX_ICE || m_alex_type_temp_power == ALEX_ICE) {
        ball_type = ICEBALL_DEFAULT;
    }

    unsigned int ball_amount = 1;

    // if star add another ball
    if (m_invincible_star) {
        ball_amount = 2;
    }

    // if added ball
    if (Ball_Add(ball_type, -1, ball_amount)) {
        m_shoot_counter = speedfactor_fps;
        m_throwing_counter = speedfactor_fps * 0.3f;

        // Issue shoot event
        Scripting::cShoot_Event evt(ball_type == ICEBALL_DEFAULT ? "ice" : "fire");
        evt.Fire(pActive_Level->m_mruby, this);
    }

}

void cLevel_Player::Action_Stop_Jump(void)
{
    Stop_Flying();
    m_up_key_time = 0;
}

void cLevel_Player::Action_Stop_Interact(input_identifier key_type)
{
    // Action
    if (key_type == INP_ACTION) {
        Release_Item();
    }
    // Down
    else if (key_type == INP_DOWN) {
        Stop_Ducking();
    }
    // Left
    else if (key_type == INP_LEFT) {
        // if key in opposite direction is still pressed only change direction
        if (pKeyboard->m_keys[pPreferences->m_key_right] || pJoystick->m_right) {
            m_direction = DIR_RIGHT;
        }
        else {
            Hold();
        }
    }
    // Right
    else if (key_type == INP_RIGHT) {
        // if key in opposite direction is still pressed only change direction
        if (pKeyboard->m_keys[pPreferences->m_key_left] || pJoystick->m_left) {
            m_direction = DIR_LEFT;
        }
        else {
            Hold();
        }
    }
    // Jump
    else if (key_type == INP_JUMP) {
        Action_Stop_Jump();
    }
    // Shoot
    else if (key_type == INP_SHOOT) {
        Action_Stop_Shoot();
    }
}

void cLevel_Player::Action_Stop_Shoot(void)
{
    // nothing
}

bool cLevel_Player::Ball_Add(ball_effect effect_type /* = FIREBALL_DEFAULT */, float ball_start_angle /* = -1 */, unsigned int amount /* = 0 */) const
{
    if ((m_alex_type != ALEX_FIRE && m_alex_type_temp_power != ALEX_FIRE && m_alex_type != ALEX_ICE && m_alex_type_temp_power != ALEX_ICE) || m_ducked_counter) {
        return 0;
    }

    if (amount == 0) {
        amount = 1;
    }

    // default fire/ice-ball
    if (effect_type == FIREBALL_DEFAULT || effect_type == ICEBALL_DEFAULT) {
        // if time not passed between last shot
        if ((effect_type == FIREBALL_DEFAULT || effect_type == ICEBALL_DEFAULT) && m_shoot_counter > 0.0f) {
            return 0;
        }

        // position
        float ball_posx = m_pos_x;
        // horizontal speed
        float ball_vel_x = 17;

        if (effect_type == ICEBALL_DEFAULT) {
            // iceball has slow horizontal speed
            ball_vel_x = 12;

            // sound
            pAudio->Play_Sound("item/iceball.wav", RID_ALEX_BALL);
        }
        // fireball
        else {
            // sound
            pAudio->Play_Sound("item/fireball.ogg", RID_ALEX_BALL);
        }

        if (m_direction == DIR_LEFT) {
            ball_posx += 8;
            // change velocity direction
            ball_vel_x *= -1;
        }
        else {
            ball_posx += 20;
        }

        for (unsigned int i = 0; i < amount; i++) {
            // add ball
            cBall* ball = new cBall(m_sprite_manager);
            ball->Set_Pos(ball_posx, m_rect.m_y + (m_rect.m_h * 0.7f) - 20, 1);
            ball->Set_Origin(m_sprite_array, m_type);
            ball->Set_Ball_Type(effect_type);
            m_sprite_manager->Add(ball);

            // set speed
            if (ball_start_angle >= 0) {
                ball->Set_Velocity_From_Angle(ball_start_angle, ball_vel_x);
                if (ball->m_velx > 0.0f) {
                    ball->m_direction = DIR_RIGHT;
                }
                else {
                    ball->m_direction = DIR_LEFT;
                }

                ball_start_angle += 20;
            }
            else {
                ball->Set_Velocity(ball_vel_x, -2.0f - static_cast<float>(i * 4));
            }
        }
    }
    // half circle fire/ice-ball explosion
    else if (effect_type == FIREBALL_EXPLOSION || effect_type == ICEBALL_EXPLOSION) {
        // start angle
        float ball_angle = 180;

        if (ball_start_angle >= 0) {
            ball_angle = ball_start_angle;
        }

        for (unsigned int i = 0; i < amount; i++) {
            // 10 * 18 = 180 degrees
            ball_angle += 180.0f / amount;

            // add ball
            cBall* ball = new cBall(m_sprite_manager);
            ball->Set_Pos(m_pos_x + m_col_rect.m_w / 2, m_pos_y + m_col_rect.m_h / 2, 1);
            ball->Set_Origin(m_sprite_array, m_type);
            ball->Set_Ball_Type(effect_type);
            ball->Set_Velocity_From_Angle(ball_angle, 15);
            if (ball->m_velx > 0.0f) {
                ball->m_direction = DIR_RIGHT;
            }
            else {
                ball->m_direction = DIR_LEFT;
            }

            ball->Col_Move(ball->m_velx * 2, ball->m_vely * 2, 1);
            m_sprite_manager->Add(ball);
        }

        // explosion animation and sound
        if (effect_type == FIREBALL_EXPLOSION) {
            cAnimation_Fireball* anim = new cAnimation_Fireball(m_sprite_manager, m_pos_x + (m_col_rect.m_w / 2), m_pos_y + (m_col_rect.m_h / 3), 10);
            anim->Set_Fading_Speed(0.3f);
            pActive_Animation_Manager->Add(anim);

            pAudio->Play_Sound("item/fireball_explosion.wav", RID_ALEX_BALL);
        }
        else {
            // create animation
            cParticle_Emitter* anim = new cParticle_Emitter(m_sprite_manager);
            anim->Set_Pos(m_pos_x + (m_col_rect.m_w * 0.5f), m_pos_y + (m_col_rect.m_h * 0.5f));
            anim->Set_Image(pVideo->Get_Package_Surface("animation/particles/light.png"));
            anim->Set_Quota(10);
            anim->Set_Time_to_Live(1.5f);
            anim->Set_Pos_Z(m_pos_z + 0.0001f);
            anim->Set_Color(Color(static_cast<Uint8>(50), 50, 250));
            anim->Set_Blending(BLEND_ADD);
            anim->Set_Speed(0.8f, 0.7f);
            anim->Set_Scale(0.4f, 0.2f);
            anim->Emit();
            pActive_Animation_Manager->Add(anim);

            pAudio->Play_Sound("item/iceball_explosion.wav", RID_ALEX_BALL);
        }
    }
    // unknown type
    else {
        return 0;
    }

    return 1;
}

void cLevel_Player::Ball_Clear(void) const
{
    // destroy all fireballs from the player
    for (cSprite_List::iterator itr = m_sprite_manager->objects.begin(); itr != m_sprite_manager->objects.end(); ++itr) {
        cSprite* obj = (*itr);

        if (obj->m_type == TYPE_BALL) {
            cBall* ball = static_cast<cBall*>(obj);

            // if from player
            if (ball->m_origin_type == TYPE_PLAYER) {
                obj->Destroy();
            }
        }
    }
}

void cLevel_Player::Add_Kill_Multiplier(void)
{
    m_kill_multiplier += 0.1f;

    // give lives after 10 continuous kills
    if (m_kill_multiplier >= 2) {
        // only every fifth kill
        if (static_cast<int>(m_kill_multiplier * 10) % 5 == 0) {
            // add 1 live
            pHud_Goldpieces->Add_Gold(100);
        }
    }

    m_last_kill_counter = 0;
}

void cLevel_Player::Update_Kill_Multiplier(void)
{
    m_last_kill_counter += pFramerate->m_speed_factor;

    if (m_kill_multiplier > 1) {
        // if longer than a second reset the multiplier
        if (m_last_kill_counter > speedfactor_fps) {
            m_kill_multiplier = 1;
        }
    }
}

Col_Valid_Type cLevel_Player::Validate_Collision(cSprite* obj)
{
    // don't collide with anything if dead
    if (m_alex_type == ALEX_DEAD) {
        return COL_VTYPE_NOT_VALID;
    }

    if (obj->m_massive_type == MASS_MASSIVE) {
        switch (obj->m_type) {
        case TYPE_FLYON: {
            if (m_invincible_star > 0.0f) {
                return COL_VTYPE_INTERNAL;
            }

            if (m_invincible > 0.0f) {
                return COL_VTYPE_NOT_VALID;
            }

            // internal as it could be frozen
            return COL_VTYPE_INTERNAL;
        }
        case TYPE_GEE: {
            if (m_invincible_star > 0.0f) {
                return COL_VTYPE_INTERNAL;
            }

            // block if on top
            if (m_vely >= 0.0f && Is_On_Top(obj)) {
                return COL_VTYPE_BLOCKING;
            }

            if (m_invincible > 0.0f) {
                return COL_VTYPE_NOT_VALID;
            }

            // internal as it could be frozen
            return COL_VTYPE_INTERNAL;
        }
        case TYPE_ROKKO: {
            // block if on top (for walking on it)
            if (m_vely >= 0.0f && Is_On_Top(obj)) {
                return COL_VTYPE_BLOCKING;
            }

            if (m_invincible_star > 0) {
                return COL_VTYPE_INTERNAL;
            }

            return COL_VTYPE_NOT_VALID;
        }
        case TYPE_SPIKA:
        case TYPE_EATO: {
            // block if on top (for walking on it)
            if (m_vely >= 0.0f && Is_On_Top(obj)) {
                return COL_VTYPE_BLOCKING;
            }

            if (m_invincible_star > 0.0f) {
                return COL_VTYPE_INTERNAL;
            }

            if (m_invincible > 0.0f) {
                return COL_VTYPE_NOT_VALID;
            }

            return COL_VTYPE_BLOCKING;
        }
        case TYPE_ARMY: {
            // block if on top
            if (m_vely >= 0.0f && Is_On_Top(obj)) {
                return COL_VTYPE_BLOCKING;
            }
            // if not on top
            else {
                cArmy* army = static_cast<cArmy*>(obj);

                // if player counter active
                if (army->m_player_counter > 0.0f) {
                    // let shell slide through us
                    return COL_VTYPE_NOT_VALID;
                }
            }

            if (m_invincible_star > 0.0f) {
                return COL_VTYPE_INTERNAL;
            }

            return COL_VTYPE_BLOCKING;
        }
        case TYPE_BALL: {
            cBall* ball = static_cast<cBall*>(obj);

            if (ball->m_origin_type != TYPE_PLAYER) {
                return COL_VTYPE_INTERNAL;
            }

            return COL_VTYPE_NOT_VALID;
        }
        case TYPE_BONUS_BOX:
        case TYPE_SPIN_BOX: {
            cBaseBox* box = static_cast<cBaseBox*>(obj);

            // ghost
            if (box->m_box_invisible == BOX_GHOST) {
                // alex is not ghost
                if (m_alex_type != ALEX_GHOST) {
                    return COL_VTYPE_NOT_VALID;
                }
            }

            return COL_VTYPE_BLOCKING;
        }
        default: {
            break;
        }
        }

        // temporarily disabled
        // don't collide with ground
        /*if( m_ground_object == obj )
        {
            return COL_VTYPE_NOT_VALID;
        }*/

        return COL_VTYPE_BLOCKING;
    }
    else if (obj->m_massive_type == MASS_HALFMASSIVE) {
        // fall through
        if (pKeyboard->m_keys[pPreferences->m_key_down]) {
            return COL_VTYPE_NOT_VALID;
        }

        // if moving downwards and the object is on bottom
        if (m_vely >= 0.0f && Is_On_Top(obj)) {
            // don't collide with ground
            if (m_ground_object == obj) {
                return COL_VTYPE_NOT_VALID;
            }

            return COL_VTYPE_BLOCKING;
        }

        return COL_VTYPE_NOT_VALID;
    }
    // allow climbing
    else if (obj->m_massive_type == MASS_CLIMBABLE) {
        // if flying
        if (m_state == STA_FLY) {
            return COL_VTYPE_NOT_VALID;
        }

        return COL_VTYPE_INTERNAL;
    }
    else if (obj->m_massive_type == MASS_PASSIVE) {
        switch (obj->m_type) {
        case TYPE_BONUS_BOX:
        case TYPE_SPIN_BOX:
        case TYPE_TEXT_BOX: {
            cBaseBox* box = static_cast<cBaseBox*>(obj);

            // invisible semi massive
            if (box->m_box_invisible == BOX_INVISIBLE_SEMI_MASSIVE) {
                // if moving upwards and the object is on top
                if (m_vely <= 0.0f && obj->Is_On_Top(this)) {
                    return COL_VTYPE_BLOCKING;
                }
            }

            return COL_VTYPE_NOT_VALID;
        }
        case TYPE_LEVEL_EXIT: {
            cLevel_Exit* levelexit = static_cast<cLevel_Exit*>(obj);

            // warp levelexit key check
            if (levelexit->m_exit_type == LEVEL_EXIT_WARP) {
                // joystick events are sent as keyboard keys
                if (pKeyboard->m_keys[pPreferences->m_key_up]) {
                    if (levelexit->m_start_direction == DIR_UP) {
                        Action_Interact(INP_UP);
                    }
                }
                else if (pKeyboard->m_keys[pPreferences->m_key_down]) {
                    if (levelexit->m_start_direction == DIR_DOWN) {
                        Action_Interact(INP_DOWN);
                    }
                }
                else if (pKeyboard->m_keys[pPreferences->m_key_right]) {
                    if (levelexit->m_start_direction == DIR_RIGHT) {
                        Action_Interact(INP_RIGHT);
                    }
                }
                else if (pKeyboard->m_keys[pPreferences->m_key_left]) {
                    if (levelexit->m_start_direction == DIR_LEFT) {
                        Action_Interact(INP_LEFT);
                    }
                }
            }

            return COL_VTYPE_NOT_VALID;
        }
        case TYPE_FIREPLANT:
        case TYPE_GOLDPIECE:
        case TYPE_MOON: {
            if (!obj->m_active) {
                return COL_VTYPE_NOT_VALID;
            }

            return COL_VTYPE_INTERNAL;
        }
        default: {
            break;
        }
        }
    }

    return COL_VTYPE_NOT_VALID;
}

void cLevel_Player::Handle_Collision_Enemy(cObjectCollision* collision)
{
    // if invalid
    if (collision->m_direction == DIR_UNDEFINED || m_alex_type == ALEX_DEAD || !m_active) {
        return;
    }

    cEnemy* enemy = static_cast<cEnemy*>(m_sprite_manager->Get_Pointer(collision->m_number));

    // if enemy already dead
    if (enemy->m_dead) {
        return;
    }

    // if invincible
    if (m_invincible_star > 0.0f) {
        bool hit_enemy = 1;

        // These enemies ignore top collisions
        if ((enemy->m_type == TYPE_SPIKA || enemy->m_type == TYPE_EATO || enemy->m_type == TYPE_THROMP || enemy->m_type == TYPE_BEETLE_BARRAGE) && collision->m_direction == DIR_DOWN) {
            hit_enemy = 0;
        }
        // rokko
        else if (enemy->m_type == TYPE_ROKKO) {
            hit_enemy = 0;
        }
        // turtle boss
        else if (enemy->m_type == TYPE_TURTLE_BOSS) {
            hit_enemy = 0;
        }
        // furball boss
        else if (enemy->m_type == TYPE_FURBALL_BOSS) {
            hit_enemy = 0;
        }

        // hit
        if (hit_enemy) {
            pAudio->Play_Sound("item/star_kill.ogg");
            pHud_Points->Add_Points(static_cast<unsigned int>(enemy->m_kill_points * 1.2f), enemy->m_pos_x, enemy->m_pos_y - 5.0f, "", yellow, 1);
            // force complete downgrade
            enemy->DownGrade(1);
            Add_Kill_Multiplier();

            // check if enemy is still the ground object
            if (enemy == m_ground_object) {
                Check_on_Ground();
            }
            return;
        }
        // no hit
        else {
            // eato blocks
            if (enemy->m_type == TYPE_EATO) {
                if (collision->m_direction == DIR_LEFT) {
                    m_velx = 0;
                }
                else if (collision->m_direction == DIR_RIGHT) {
                    m_velx = 0;
                }
            }
        }
    }

    // enemy is frozen
    if (enemy->m_freeze_counter) {
        // animation
        cParticle_Emitter* anim = new cParticle_Emitter(m_sprite_manager);
        anim->Set_Image(pVideo->Get_Package_Surface("animation/particles/light.png"));
        anim->Set_Time_to_Live(0.6f, 0.4f);
        anim->Set_Color(Color(static_cast<Uint8>(160), 160, 240), Color(static_cast<Uint8>(rand() % 80), rand() % 80, rand() % 10, 0));
        anim->Set_Fading_Alpha(1);
        anim->Set_Fading_Size(1);
        anim->Set_Speed(0.5f, 0.2f);
        anim->Set_Blending(BLEND_DRIVE);

        // enemy rect ice particle animation
        for (unsigned int w = 0; w < enemy->m_col_rect.m_w; w += 10) {
            for (unsigned int h = 0; h < enemy->m_col_rect.m_h; h += 10) {
                anim->Set_Pos(enemy->m_pos_x + w, enemy->m_pos_y + h);
                anim->Emit();
            }
        }

        pActive_Animation_Manager->Add(anim);

        // ice sound
        pAudio->Play_Sound("item/ice_kill.wav");

        // get points
        pHud_Points->Add_Points(enemy->m_kill_points, enemy->m_pos_x, enemy->m_pos_y - 10.0f, "", static_cast<Uint8>(255), 1);

        // kill enemy
        enemy->DownGrade(1);
        Add_Kill_Multiplier();

        // slow down
        m_velx *= 0.7f;
        m_vely *= 0.7f;

        // if enemy was ground object
        if (enemy == m_ground_object) {
            Reset_On_Ground();
        }

        return;
    }

    if (collision->m_direction == DIR_TOP) {
        m_vely = 0;

        if (m_state != STA_FLY) {
            if (m_state != STA_CLIMB) {
                Set_Moving_State(STA_FALL);
                m_up_key_time = 0;
                m_jump_power = 0;
            }
        }
    }

    // send collision
    Send_Collision(collision);
}

void cLevel_Player::Handle_Collision_Massive(cObjectCollision* collision)
{
    // if invalid
    if (collision->m_direction == DIR_UNDEFINED) {
        return;
    }

    /* fixme: the collision direction is sometimes wrong as left/right if landing on a moving platform which moves upwards
    * this seems to cause alex to hang or to get stuck in it
    * easily reproducible with a low framerate and fast moving platform
    */
    //printf( "direction is %s\n", Get_Direction_Name( collision->m_direction ).c_str() );

    cSprite* col_obj = m_sprite_manager->Get_Pointer(collision->m_number);

    // climbable
    if (col_obj->m_massive_type == MASS_CLIMBABLE && m_state != STA_CLIMB && m_state != STA_FLY) {
        // if not climbing and player wants to climb
        if (pKeyboard->m_keys[pPreferences->m_key_up] || pJoystick->m_up || ((pKeyboard->m_keys[pPreferences->m_key_down] || pJoystick->m_down) && !m_ground_object)) {
            // start climbing
            Start_Climbing();
        }

        return;
    }

    // ignore ball
    if (col_obj->m_type == TYPE_BALL) {
        return;
    }

    // send back if crate
    if (col_obj->m_type == TYPE_CRATE) {
        Send_Collision(collision);
        return;
    }

    // send back if item
    if (col_obj->m_type == TYPE_FIREPLANT || col_obj->m_type == TYPE_GOLDPIECE || col_obj->m_type == TYPE_MOON) {
        Send_Collision(collision);
        return;
    }

    if (collision->m_direction == DIR_UP) {
        m_vely = 0;

        if (m_state != STA_FLY) {
            if (m_state != STA_CLIMB) {
                Set_Moving_State(STA_FALL);

                cMovingSprite* moving_object = dynamic_cast<cMovingSprite*>(col_obj);

                // if valid moving sprite
                if (moving_object) {
                    // add its velocity
                    if (moving_object->m_vely > 0) {
                        m_vely += moving_object->m_vely;
                    }
                }

                if (collision->m_array == ARRAY_MASSIVE) {
                    pAudio->Play_Sound("wall_hit.wav", RID_ALEX_WALL_HIT);

                    // create animation
                    cParticle_Emitter* anim = new cParticle_Emitter(m_sprite_manager);
                    anim->Set_Emitter_Rect(m_col_rect.m_x, m_col_rect.m_y + 6, m_col_rect.m_w);
                    anim->Set_Image(pVideo->Get_Package_Surface("animation/particles/light.png"));
                    anim->Set_Quota(4);
                    anim->Set_Pos_Z(m_pos_z - 0.000001f);
                    anim->Set_Time_to_Live(0.3f);
                    anim->Set_Color(Color(static_cast<Uint8>(150), 150, 150, 200), Color(static_cast<Uint8>(rand() % 55), rand() % 55, rand() % 55, 0));
                    anim->Set_Speed(2, 0.6f);
                    anim->Set_Scale(0.6f);
                    anim->Set_Direction_Range(0, 180);
                    anim->Set_Fading_Alpha(1);
                    anim->Set_Fading_Size(1);
                    anim->Emit();
                    pActive_Animation_Manager->Add(anim);
                }
            }
        }
        // flying
        else {
            m_velx -= m_velx * 0.05f * pFramerate->m_speed_factor;

            // too slow
            if (m_velx > -5.0f && m_velx < 5.0f) {
                Stop_Flying();
            }
        }
    }
    else if (collision->m_direction == DIR_DOWN) {
        // flying
        if (m_state == STA_FLY) {
            m_vely = 0.0f;
            m_velx -= m_velx * 0.05f * pFramerate->m_speed_factor;

            // too slow
            if (m_velx > -5.0f && m_velx < 5.0f) {
                Stop_Flying();
            }
        }
        // not flying
        else {
            // jumping
            if (m_state == STA_JUMP) {
                /* hack : only pick up after some time of jumping because moving objects collide move before the player
                 * also see cMovingSprite::Validate_Collision_Object_On_Top
                */
                if (m_jump_power < 6.0f) {
                    m_vely = 0.0f;
                }
            }
            // all other states
            else {
                m_vely = 0.0f;
            }
        }
    }
    else if (collision->m_direction == DIR_LEFT) {
        m_velx = 0.0f;
        m_vely -= m_vely * 0.01f * pFramerate->m_speed_factor;

        if (m_state == STA_WALK || m_state == STA_RUN) {
            m_walk_count = 0.0f;
        }
        // flying
        else if (m_state == STA_FLY) {
            // box collision
            if (col_obj->m_type == TYPE_BONUS_BOX || col_obj->m_type == TYPE_SPIN_BOX) {
                cBaseBox* box = static_cast<cBaseBox*>(col_obj);
                box->Activate_Collision(collision->m_direction);
            }

            Stop_Flying();
        }
    }
    else if (collision->m_direction == DIR_RIGHT) {
        m_velx = 0.0f;
        m_vely -= m_vely * 0.01f * pFramerate->m_speed_factor;

        if (m_state == STA_WALK || m_state == STA_RUN) {
            m_walk_count = 0.0f;
        }
        // flying
        else if (m_state == STA_FLY) {
            // box collision
            if (col_obj->m_type == TYPE_BONUS_BOX || col_obj->m_type == TYPE_SPIN_BOX) {
                cBaseBox* box = static_cast<cBaseBox*>(col_obj);
                box->Activate_Collision(collision->m_direction);
            }

            Stop_Flying();
        }
    }

    if (collision->m_array == ARRAY_ACTIVE) {
        // send collision
        Send_Collision(collision);
    }
}

void cLevel_Player::Handle_Collision_Passive(cObjectCollision* collision)
{
    // if invalid
    if (collision->m_direction == DIR_UNDEFINED) {
        return;
    }

    //cSprite *col_obj = m_sprite_manager->Get_Pointer( collision->m_number );
    //printf( "passive %s\n", col_obj->name.c_str() );

    // send it
    Send_Collision(collision);
}

void cLevel_Player::Handle_Collision_Lava(cObjectCollision* p_collision)
{
    if (p_collision->m_direction == DIR_UNDEFINED)
        return;

    // Delegate to cLava::Handle_Collision_Player().
    Send_Collision(p_collision);
}

void cLevel_Player::Handle_out_of_Level(ObjectDirection dir)
{
    if (dir == DIR_LEFT) {
        Set_Pos_X(pLevel_Manager->m_camera->m_limit_rect.m_x - m_col_pos.m_x);
        m_velx = 0.0f;
        Stop_Flying();
    }
    else if (dir == DIR_RIGHT) {
        Set_Pos_X(pLevel_Manager->m_camera->m_limit_rect.m_x + pLevel_Manager->m_camera->m_limit_rect.m_w - m_col_pos.m_x - m_col_rect.m_w - 0.01f);
        m_velx = 0.0f;
        Stop_Flying();
    }
    else if (dir == DIR_TOP) {
        /*if( pActive_Level->limit_top_blocks )
        {
            Set_Pos_Y( pLevel_Manager->m_camera->m_limit_rect.m_y + pLevel_Manager->m_camera->m_limit_rect.m_h + 0.01f );
            m_vely = 0.0f;
        }*/
    }
    else if (dir == DIR_BOTTOM) {
        if (m_god_mode) {
            m_vely = -35.0f;
        }
        else {
            /*if( pActive_Level->limit_bottom_blocks )
            {
                Set_Pos_Y( pLevel_Manager->m_camera->m_limit_rect.m_y + game_res_h - m_col_pos.m_x - m_col_rect.m_w + 0.01f );
                m_vely = 0.0f;
            }
            // falling below ground
            else
            {*/
            DownGrade_Player(true, true, true);
            //}
        }
    }
}

void cLevel_Player::Editor_Activate(void)
{
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    // direction
    CEGUI::Combobox* combobox = static_cast<CEGUI::Combobox*>(wmgr.createWindow("TaharezLook/Combobox", "editor_player_direction"));
    Editor_Add(UTF8_("Direction"), UTF8_("Initial direction"), combobox, 100, 75);

    combobox->addItem(new CEGUI::ListboxTextItem("right"));
    combobox->addItem(new CEGUI::ListboxTextItem("left"));
    combobox->setText(Get_Direction_Name(m_start_direction));

    combobox->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cLevel_Player::Editor_Direction_Select, this));

    // init
    Editor_Init();
}

bool cLevel_Player::Editor_Direction_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    Set_Direction(Get_Direction_Id(item->getText().c_str()), 1);

    return 1;
}

void cLevel_Player::Push_Return(const std::string& level, const std::string& entry)
{
    cLevel_Player_Return_Entry e;
    e.level = level;
    e.entry = entry;

    m_return_stack.push_back(e);
}

bool cLevel_Player::Pop_Return(std::string& level, std::string& entry)
{
    if (!m_return_stack.empty()) {
        cLevel_Player_Return_Entry e = m_return_stack.back();
        m_return_stack.pop_back();

        level = e.level;
        entry = e.entry;
        return true;
    }
    else {
        level = "";
        entry = "";
        return false;
    }
}

void cLevel_Player::Clear_Return(void)
{
    m_return_stack.clear();
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

cLevel_Player* pLevel_Player = NULL;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

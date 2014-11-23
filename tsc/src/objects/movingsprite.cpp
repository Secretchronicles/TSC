/***************************************************************************
 * movingsprite.cpp  -  moving sprite class
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
#include "../objects/movingsprite.hpp"
#include "../core/framerate.hpp"
#include "../core/game_core.hpp"
#include "../level/level.hpp"
#include "../user/preferences.hpp"
#include "../audio/audio.hpp"
#include "../level/level_player.hpp"
#include "../enemies/turtle.hpp"
#include "../objects/box.hpp"
#include "../video/renderer.hpp"
#include "../video/gl_surface.hpp"
#include "../core/sprite_manager.hpp"

namespace TSC {

/* *** *** *** *** *** *** *** cMovingSprite *** *** *** *** *** *** *** *** *** *** */

cMovingSprite::cMovingSprite(cSprite_Manager* sprite_manager, std::string type_name /* = "sprite" */)
    : cSprite(sprite_manager, type_name)
{
    cMovingSprite::Init();
}

cMovingSprite::cMovingSprite(XmlAttributes& attributes, cSprite_Manager* sprite_manager, std::string type_name /* = "sprite" */)
    : cSprite(sprite_manager, type_name)
{
    cMovingSprite::Init();

    // TODO: This is the same as in the cSprite constructor!
    // position
    Set_Pos(string_to_float(attributes["posx"]), string_to_float(attributes["posy"]), true);
    // image
    Set_Image(pVideo->Get_Surface(utf8_to_path(attributes["image"])), true) ;
}

cMovingSprite::~cMovingSprite(void)
{
    //
}

void cMovingSprite::Init(void)
{
    m_state = STA_STAY;
    m_type = TYPE_UNDEFINED;

    m_velx = 0.0f;
    m_vely = 0.0f;
    m_gravity_max = 0.0f;

    m_direction = DIR_UNDEFINED;
    m_start_direction = DIR_UNDEFINED;
    m_can_be_on_ground = 1;
    m_ground_object = NULL;

    m_ice_resistance = 0.0f;
    m_freeze_counter = 0.0f;
}

cMovingSprite* cMovingSprite::Copy(void) const
{
    cMovingSprite* moving_sprite = new cMovingSprite(m_sprite_manager);
    moving_sprite->Set_Image(m_start_image, 1);
    moving_sprite->Set_Pos(m_start_pos_x, m_start_pos_y, 1);
    moving_sprite->m_type = m_type;
    moving_sprite->m_sprite_array = m_sprite_array;
    moving_sprite->Set_Massive_Type(m_massive_type);
    moving_sprite->m_can_be_ground = m_can_be_ground;
    moving_sprite->m_can_be_on_ground = m_can_be_on_ground;
    moving_sprite->Set_Ignore_Camera(m_no_camera);
    moving_sprite->Set_Shadow_Pos(m_shadow_pos);
    moving_sprite->Set_Shadow_Color(m_shadow_color);
    return moving_sprite;
}

void cMovingSprite::Set_Image(cGL_Surface* new_image, bool new_start_image /* = 0 */, bool del_img /* = 0 */)
{
    // get a possible collision point change
    GL_point col_pos_change = GL_point();

    if (m_image && new_image) {
        col_pos_change = new_image->m_col_pos - m_image->m_col_pos;
    }

    cSprite::Set_Image(new_image, new_start_image, del_img);

    // handle collision point change
    if (col_pos_change.m_x != 0.0f || col_pos_change.m_y != 0.0f) {
        Move(-col_pos_change.m_x, -col_pos_change.m_y, 1);
    }

    // check onground
    Check_on_Ground();
}

void cMovingSprite::Set_Direction(const ObjectDirection dir, bool new_start_direction /* = 0 */)
{
    m_direction = dir;

    // turn velocity if wrong
    if ((dir == DIR_LEFT && m_velx > 0.0f) || (dir == DIR_RIGHT && m_velx < 0.0f)) {
        m_velx *= -1;
    }

    if (new_start_direction) {
        m_start_direction = dir;
    }
}

void cMovingSprite::Auto_Slow_Down(float x_speed, float y_speed /* = 0 */)
{
    // horizontal slow down
    if (x_speed > 0.0f) {
        if (m_velx > 0.0f) {
            Add_Velocity_X_Min(-x_speed, 0.0f);
        }
        else if (m_velx < 0.0f) {
            Add_Velocity_X_Max(x_speed, 0.0f);
        }
    }

    // vertical slow down
    if (y_speed > 0.0f) {
        if (m_vely > 0.0f) {
            Add_Velocity_Y_Min(-y_speed, 0.0f);
        }
        else if (m_vely < 0.0f) {
            Add_Velocity_Y_Max(y_speed, 0.0f);
        }
    }
}

void cMovingSprite::Move(float move_x, float move_y, bool real /* = 0 */)
{
    cSprite::Move(move_x, move_y, real);

    // handle if moved out of level rect
    Check_And_Handle_Out_Of_Level(move_x, move_y);
}

cObjectCollisionType* cMovingSprite::Col_Move_in_Steps(float move_x, float move_y, float step_size_x, float step_size_y, float final_pos_x, float final_pos_y, cSprite_List sprite_list, bool stop_on_internal /* = 0 */)
{
    if (sprite_list.empty()) {
        cSprite::Move(final_pos_x - m_pos_x, final_pos_y - m_pos_y, 1);
        return NULL;
    }

    // collision list
    cObjectCollisionType* col_list = new cObjectCollisionType();

    bool move_x_valid = 1;
    bool move_y_valid = 1;

    /* Checks in both directions simultaneously
     * if a collision occurs it saves the direction
    */
    while (move_x_valid || move_y_valid) {
        if (move_x_valid) {
            // nothing to do
            if (Is_Float_Equal(step_size_x, 0.0f)) {
                move_x_valid = 0;
                continue;
            }

            // collision check
            cObjectCollisionType* col_list_temp = Collision_Check_Relative(step_size_x, 0.0f, 0.0f, 0.0f, COLLIDE_COMPLETE, &sprite_list);

            bool collision_found = 0;

            // stop on everything
            if (stop_on_internal) {
                if (col_list_temp->size()) {
                    collision_found = 1;
                }
            }
            // stop only on blocking
            else {
                if (col_list_temp->Is_Included(COL_VTYPE_BLOCKING)) {
                    collision_found = 1;
                }
                // remove internal collision from further checks
                else if (col_list_temp->objects.size()) {
                    for (cObjectCollision_List::iterator itr = col_list_temp->objects.begin(); itr != col_list_temp->objects.end(); ++itr) {
                        cObjectCollision* col = (*itr);

                        if (col->m_valid_type != COL_VTYPE_INTERNAL) {
                            continue;
                        }

                        // find in sprite list
                        cSprite_List::iterator sprite_itr = std::find(sprite_list.begin(), sprite_list.end(), col->m_obj);

                        // not found
                        if (sprite_itr == sprite_list.end()) {
                            continue;
                        }

                        sprite_list.erase(sprite_itr);
                    }

                    // if no objects left
                    if (sprite_list.empty()) {
                        // move to final position
                        m_pos_x = final_pos_x;
                    }
                }
            }

            if (col_list_temp->size()) {
                col_list->objects.insert(col_list->objects.end(), col_list_temp->objects.begin(), col_list_temp->objects.end());
                col_list_temp->objects.clear();
            }

            delete col_list_temp;

            if (!collision_found) {
                m_pos_x += step_size_x;

                if ((step_size_x > 0.0f && final_pos_x <= m_pos_x) || (step_size_x < 0.0f && final_pos_x >= m_pos_x)) {
                    m_pos_x = final_pos_x;
                    move_x_valid = 0;
                    step_size_x = 0.0f;
                }

                // update collision rects
                Update_Position_Rect();
            }
            // collision found
            else {
                step_size_x = 0.0f;
                move_x_valid = 0;
            }
        }

        if (move_y_valid) {
            // nothing to do
            if (Is_Float_Equal(step_size_y, 0.0f)) {
                move_y_valid = 0;
                continue;
            }

            // collision check
            cObjectCollisionType* col_list_temp = Collision_Check_Relative(0.0f, step_size_y, 0.0f, 0.0f, COLLIDE_COMPLETE, &sprite_list);

            bool collision_found = 0;

            // stop on everything
            if (stop_on_internal) {
                if (col_list_temp->size()) {
                    collision_found = 1;
                }
            }
            // stop only on blocking
            else {
                if (col_list_temp->Is_Included(COL_VTYPE_BLOCKING)) {
                    collision_found = 1;
                }
                // remove internal collision from further checks
                else if (col_list_temp->objects.size()) {
                    for (cObjectCollision_List::iterator itr = col_list_temp->objects.begin(); itr != col_list_temp->objects.end(); ++itr) {
                        cObjectCollision* col = (*itr);

                        if (col->m_valid_type != COL_VTYPE_INTERNAL) {
                            continue;
                        }

                        // find in sprite list
                        cSprite_List::iterator sprite_itr = std::find(sprite_list.begin(), sprite_list.end(), col->m_obj);

                        // not found
                        if (sprite_itr == sprite_list.end()) {
                            continue;
                        }

                        sprite_list.erase(sprite_itr);

                        // if no objects left
                        if (sprite_list.empty()) {
                            // move to final position
                            m_pos_y = final_pos_y;
                        }
                    }
                }
            }

            if (col_list_temp->size()) {
                col_list->objects.insert(col_list->objects.end(), col_list_temp->objects.begin(), col_list_temp->objects.end());
                col_list_temp->objects.clear();
            }

            delete col_list_temp;

            if (!collision_found) {
                m_pos_y += step_size_y;

                if ((step_size_y > 0.0f && final_pos_y <= m_pos_y) || (step_size_y < 0.0f && final_pos_y >= m_pos_y)) {
                    m_pos_y = final_pos_y;
                    move_y_valid = 0;
                    step_size_y = 0.0f;
                }

                // update collision rects
                Update_Position_Rect();
            }
            // collision found
            else {
                step_size_y = 0.0f;
                move_y_valid = 0;
            }
        }
    }

    return col_list;
}

void cMovingSprite::Col_Move(float move_x, float move_y, bool real /* = 0 */, bool force /* = 0 */, bool check_on_ground /* = 1 */)
{
    // no need to move
    if (Is_Float_Equal(move_x, 0.0f) && Is_Float_Equal(move_y, 0.0f)) {
        return;
    }

    // invalid collision rect
    if (Is_Float_Equal(m_col_rect.m_w, 0.0f) || Is_Float_Equal(m_col_rect.m_h, 0.0f)) {
        return;
    }

    // use speedfactor
    if (!real) {
        move_x *= pFramerate->m_speed_factor;
        move_y *= pFramerate->m_speed_factor;
    }

    // check for collisions
    if (!force) {
        // get all possible colliding items
        GL_rect complete_rect = m_col_rect;

        if (move_x > 0.0f) {
            complete_rect.m_w += move_x;
        }
        else {
            complete_rect.m_x += move_x;
            complete_rect.m_w -= move_x;
        }

        if (move_y > 0.0f) {
            complete_rect.m_h += move_y;
        }
        else {
            complete_rect.m_y += move_y;
            complete_rect.m_h -= move_y;
        }

        cSprite_List sprite_list;
        m_sprite_manager->Get_Colliding_Objects(sprite_list, complete_rect, 1, this);

        // step size
        float step_size_x = move_x;
        float step_size_y = move_y;

        // check if object collision rect is smaller as the position check size
        if (step_size_x > m_col_rect.m_w) {
            step_size_x = m_col_rect.m_w;
        }
        else if (step_size_x < -m_col_rect.m_w) {
            step_size_x = -m_col_rect.m_w;
        }

        if (step_size_y > m_col_rect.m_h) {
            step_size_y = m_col_rect.m_h;
        }
        else if (step_size_y < -m_col_rect.m_h) {
            step_size_y = -m_col_rect.m_h;
        }

        float final_pos_x = m_pos_x + move_x;
        float final_pos_y = m_pos_y + move_y;

        // move in big steps
        cObjectCollisionType* col_list = Col_Move_in_Steps(move_x, move_y, step_size_x, step_size_y, final_pos_x, final_pos_y, sprite_list, 1);

        // if a collision is found enter pixel checking
        if (col_list && col_list->size()) {
            // change to pixel checking
            if (step_size_x < -1.0f) {
                step_size_x = -1.0f;
            }
            else if (step_size_x > 1.0f) {
                step_size_x = 1.0f;
            }

            if (step_size_y < -1.0f) {
                step_size_y = -1.0f;
            }
            else if (step_size_y > 1.0f) {
                step_size_y = 1.0f;
            }

            delete col_list;
            col_list = Col_Move_in_Steps(move_x, move_y, step_size_x, step_size_y, final_pos_x, final_pos_y, sprite_list);

            Add_Collisions(col_list, 1);
        }

        if (col_list) {
            delete col_list;
        }
    }
    // don't check for collisions
    else {
        m_pos_x += move_x;
        m_pos_y += move_y;
        Update_Position_Rect();
    }

    // if check on ground
    if (check_on_ground) {
        Check_on_Ground();
    }

    // check/handle if moved out of level rect
    Check_And_Handle_Out_Of_Level(move_x, move_y);
}

void cMovingSprite::Add_Velocity(const float x, const float y, const bool real /* = 0 */)
{
    if (real) {
        m_velx += x;
        m_vely += y;
    }
    else {
        m_velx += x * pFramerate->m_speed_factor;
        m_vely += y * pFramerate->m_speed_factor;
    }
}

void cMovingSprite::Add_Velocity_X(const float x, const bool real /* = 0 */)
{
    if (real) {
        m_velx += x;
    }
    else {
        m_velx += x * pFramerate->m_speed_factor;
    }
}

void cMovingSprite::Add_Velocity_Y(const float y, const bool real /* = 0 */)
{
    if (real) {
        m_vely += y;
    }
    else {
        m_vely += y * pFramerate->m_speed_factor;
    }
}

void cMovingSprite::Turn_Around(ObjectDirection col_dir /* = DIR_UNDEFINED */)
{
    // check if the collision direction is not in front
    if (col_dir != DIR_UNDEFINED && m_direction != DIR_UNDEFINED && m_direction != col_dir) {
        return;
    }

    // reverse velocity
    if (m_direction == DIR_LEFT || m_direction == DIR_RIGHT) {
        m_velx *= -1;
    }
    else if (m_direction == DIR_UP || m_direction == DIR_DOWN) {
        m_vely *= -1;
    }
    else {
        m_velx *= -1;
        m_vely *= -1;
    }

    m_direction = Get_Opposite_Direction(m_direction);
}

void cMovingSprite::Update(void)
{
    if (m_freeze_counter > 0.0f) {
        m_freeze_counter -= pFramerate->m_speed_factor;

        if (m_freeze_counter <= 0.0f) {
            // todo : Event_Freeze_Ended()
            m_freeze_counter = 0.0f;
            Update_Valid_Update();
        }
    }

    Update_Gravity();
}

void cMovingSprite::Update_Gravity(void)
{
    // Shortcut if this object is not subject to gravity at all
    if (Is_Float_Equal(m_gravity_max, 0.0f))
        return;

    if (!m_ground_object) {
        if (m_vely < m_gravity_max) {
            Add_Velocity_Y_Max(1.5f, m_gravity_max);
        }
    }
    // has ground object
    else {
        // stop falling
        if (m_vely > 0.0f) {
            m_vely = 0.0f;
        }
    }
}

void cMovingSprite::Draw(cSurface_Request* request /* = NULL */)
{
    if (!m_valid_draw) {
        return;
    }

    bool create_request = 0;

    if (!request) {
        create_request = 1;
        // create request
        request = new cSurface_Request();
    }

    cSprite::Draw(request);

    if (!editor_enabled) {
        // frozen
        if (m_freeze_counter) {
            request->m_combine_type = GL_ADD;

            float counter_add = m_freeze_counter;

            if (counter_add > 1000.0f) {
                counter_add = 1000.0f;
            }

            request->m_combine_color[0] = counter_add * 0.003f;
            request->m_combine_color[1] = counter_add * 0.003f;
            request->m_combine_color[2] = counter_add * 0.0099f;
        }
    }

    if (create_request) {
        // add request
        pRenderer->Add(request);
    }
}

cObjectCollisionType* cMovingSprite::Collision_Check_Absolute(const float x, const float y, const float w /* = 0 */, const float h /* = 0 */, const ColCheckType check_type /* = COLLIDE_COMPLETE */, cSprite_List* objects /* = NULL */)
{
    // save original rect
    GL_rect new_rect;

    // if given use x position
    if (!Is_Float_Equal(x, 0.0f)) {
        new_rect.m_x = x;
    }
    else {
        new_rect.m_x = m_col_rect.m_x;
    }

    // if given use y position
    if (!Is_Float_Equal(y, 0.0f)) {
        new_rect.m_y = y;
    }
    else {
        new_rect.m_y = m_col_rect.m_y;
    }

    // if given use width
    if (w > 0.0f) {
        new_rect.m_w = w;
    }
    else {
        new_rect.m_w = m_col_rect.m_w;
    }

    // if given use height
    if (h > 0.0f) {
        new_rect.m_h = h;
    }
    else {
        new_rect.m_h = m_col_rect.m_h;
    }

    // visual debugging
    if (game_debug) {
        // create request
        cRect_Request* request = new cRect_Request();

        pVideo->Draw_Rect(&new_rect, m_pos_z + 0.00001f, &green, request);
        request->m_no_camera = 0;

        request->m_blend_sfactor = GL_SRC_COLOR;
        request->m_blend_dfactor = GL_DST_ALPHA;


        // add request
        pRenderer->Add(request);
    }

    // return collisions list
    return Collision_Check(&new_rect, check_type, objects);
}

cObjectCollisionType* cMovingSprite::Collision_Check(const GL_rect& new_rect, const ColCheckType check_type /* = COLLIDE_COMPLETE */, cSprite_List* objects /* = NULL */)
{
    // blocking collisions list
    cObjectCollisionType* col_list = new cObjectCollisionType();

    // no width or height is invalid
    if (Is_Float_Equal(new_rect.m_w, 0.0f) || Is_Float_Equal(new_rect.m_h, 0.0f)) {
        return col_list;
    }

    // if no object list is given get all objects available
    if (!objects) {
        objects = &m_sprite_manager->objects;

        // Player
        if (m_type != TYPE_PLAYER && new_rect.Intersects(pActive_Player->m_col_rect)) {
            // validate
            Col_Valid_Type col_valid = Validate_Collision(pActive_Player);

            // ignore internal collisions
            if (check_type == COLLIDE_ONLY_BLOCKING) {
                if (col_valid == COL_VTYPE_INTERNAL) {
                    col_valid = COL_VTYPE_NOT_VALID;
                }
            }
            // ignore blocking collisions
            else if (check_type == COLLIDE_ONLY_INTERNAL) {
                if (col_valid == COL_VTYPE_BLOCKING) {
                    col_valid = COL_VTYPE_NOT_VALID;
                }
            }

            // valid collision
            if (col_valid != COL_VTYPE_NOT_VALID) {
                // add to list
                col_list->Add(Create_Collision_Object(this, pActive_Player, col_valid));
            }
        }
    }

    // Check objects
    for (cSprite_List::iterator itr = objects->begin(); itr != objects->end(); ++itr) {
        // get object pointer
        cSprite* level_object = (*itr);

        // if the same object or destroyed object
        if (this == level_object || level_object->m_auto_destroy) {
            continue;
        }

        // if rects don't touch
        if (!new_rect.Intersects(level_object->m_col_rect)) {
            continue;
        }

        // if undefined, hud or animation
        if (level_object->m_sprite_array == ARRAY_UNDEFINED || level_object->m_sprite_array == ARRAY_HUD || level_object->m_sprite_array == ARRAY_ANIM) {
            continue;
        }

        // if enemy is dead
        if (level_object->m_sprite_array == ARRAY_ENEMY && static_cast<cEnemy*>(level_object)->m_dead) {
            continue;
        }

        // validate
        Col_Valid_Type col_valid = Validate_Collision(level_object);

        // not a valid collision
        if (col_valid == COL_VTYPE_NOT_VALID) {
            continue;
        }

        // ignore internal collisions
        if (check_type == COLLIDE_ONLY_BLOCKING) {
            if (col_valid == COL_VTYPE_INTERNAL) {
                continue;
            }
        }
        // ignore blocking collisions
        else if (check_type == COLLIDE_ONLY_INTERNAL) {
            if (col_valid == COL_VTYPE_BLOCKING) {
                continue;
            }
        }

        // add to list
        col_list->Add(Create_Collision_Object(this, level_object, col_valid));
    }

    return col_list;
}

void cMovingSprite::Check_And_Handle_Out_Of_Level(const float move_x, const float move_y)
{
    if (Is_Out_Of_Level_Left(move_x)) {
        Handle_out_of_Level(DIR_LEFT);
    }
    else if (Is_Out_Of_Level_Right(move_x)) {
        Handle_out_of_Level(DIR_RIGHT);
    }

    if (Is_Out_Of_Level_Top(move_y)) {
        Handle_out_of_Level(DIR_TOP);
    }
    else if (Is_Out_Of_Level_Bottom(move_y)) {
        Handle_out_of_Level(DIR_BOTTOM);
    }
}

bool cMovingSprite::Is_Out_Of_Level_Left(const float move_x) const
{
    if (m_col_rect.m_x < pActive_Camera->m_limit_rect.m_x && m_col_rect.m_x - (move_x - 0.00001f) >= pActive_Camera->m_limit_rect.m_x) {
        return 1;
    }

    return 0;
}

bool cMovingSprite::Is_Out_Of_Level_Right(const float move_x) const
{
    if (m_col_rect.m_x + m_col_rect.m_w > pActive_Camera->m_limit_rect.m_x + pActive_Camera->m_limit_rect.m_w && m_col_rect.m_x + m_col_rect.m_w - (move_x + 0.00001f) <= pActive_Camera->m_limit_rect.m_x + pActive_Camera->m_limit_rect.m_w) {
        return 1;
    }

    return 0;
}

bool cMovingSprite::Is_Out_Of_Level_Top(const float move_y) const
{
    if (m_col_rect.m_y < pActive_Camera->m_limit_rect.m_y + pActive_Camera->m_limit_rect.m_h && m_col_rect.m_y - (move_y - 0.00001f) >= pActive_Camera->m_limit_rect.m_h + pActive_Camera->m_limit_rect.m_y) {
        return 1;
    }

    return 0;
}

bool cMovingSprite::Is_Out_Of_Level_Bottom(const float move_y) const
{
    if (m_col_rect.m_y + m_col_rect.m_h > pActive_Camera->m_limit_rect.m_y && m_col_rect.m_y + m_col_rect.m_h - (move_y + 0.00001f) <= pActive_Camera->m_limit_rect.m_y) {
        return 1;
    }

    return 0;
}

bool cMovingSprite::Set_On_Ground(cSprite* obj, bool set_on_top /* = 1 */)
{
    // invalid or can't be on ground
    if (!obj || !m_can_be_on_ground) {
        return 0;
    }

    // if wanted object can't be ground object
    if (!obj->m_can_be_ground) {
        return 0;
    }

    // set groundobject
    m_ground_object = obj;
    // set on top
    if (set_on_top) {
        Set_On_Top(m_ground_object, 0);
    }

    return 1;
}

void cMovingSprite::Check_on_Ground(void)
{
    // can't be on ground
    if (!m_can_be_on_ground) {
        return;
    }

    if (m_type != TYPE_PLAYER && m_sprite_array != ARRAY_ENEMY && m_sprite_array != ARRAY_ACTIVE) {
        return;
    }

    // if ground object
    if (m_ground_object) {
        GL_rect rect2(m_col_rect.m_x, m_col_rect.m_y + m_col_rect.m_h, m_col_rect.m_w, 1.0f);

        // if on ground object
        if (m_ground_object->m_col_rect.Intersects(rect2) && m_ground_object->m_can_be_ground) {
            return;
        }
    }

    // don't check if flying or linked
    if (m_state == STA_FLY || m_state == STA_OBJ_LINKED) {
        return;
    }

    // new onground check
    cObjectCollisionType* col_list = Collision_Check_Relative(0.0f, m_col_rect.m_h, 0.0f, 1.0f, COLLIDE_ONLY_BLOCKING);

    Reset_On_Ground();

    // possible ground objects
    for (cObjectCollision_List::iterator itr = col_list->objects.begin(); itr != col_list->objects.end(); ++itr) {
        cObjectCollision* col = (*itr);

        // ground collision found
        if (col->m_direction == DIR_BOTTOM) {
            if (Set_On_Ground(col->m_obj)) {
                // send collision ( needed for falling platform )
                Send_Collision(col);
                break;
            }
        }
    }

    delete col_list;
}

void cMovingSprite::Update_Anti_Stuck(void)
{
    // collision count
    cObjectCollisionType* col_list = Collision_Check(&m_col_rect, COLLIDE_ONLY_BLOCKING);

    // check collisions
    for (cObjectCollision_List::iterator itr = col_list->objects.begin(); itr != col_list->objects.end(); ++itr) {
        cObjectCollision* collision = (*itr);
        cSprite* col_obj = collision->m_obj;

        if (collision->m_array == ARRAY_ENEMY || (collision->m_array == ARRAY_ACTIVE && (col_obj->m_massive_type == MASS_HALFMASSIVE || col_obj->m_massive_type == MASS_CLIMBABLE))) {
            continue;
        }

        debug_print("Anti Stuck detected object %s on %s side\n", col_obj->Create_Name().c_str(), Get_Direction_Name(collision->m_direction).c_str());

        if (collision->m_direction == DIR_LEFT) {
            Col_Move(1.0f, 0.0f, 0, 1);
        }
        else if (collision->m_direction == DIR_RIGHT) {
            Col_Move(-1.0f, 0.0f, 0, 1);
        }
        else if (collision->m_direction == DIR_UP) {
            Col_Move(0.0f, 1.0f, 0, 1);
        }
        else if (collision->m_direction == DIR_DOWN) {
            Col_Move(0.0f, -1.0f, 0, 1);
        }
    }

    delete col_list;
}

void cMovingSprite::Collide_Move(void)
{
    if (!m_valid_update || !Is_In_Range()) {
        return;
    }

    // move and create collision data
    Col_Move(m_velx, m_vely);

    Move_With_Ground();
}

void cMovingSprite::Move_With_Ground(void)
{
    if (!m_ground_object || (m_ground_object->m_sprite_array != ARRAY_ACTIVE && m_ground_object->m_sprite_array != ARRAY_ENEMY)) {  // || m_ground_object->sprite_array == ARRAY_MASSIVE
        return;
    }

    cMovingSprite* moving_ground_object = dynamic_cast<cMovingSprite*>(m_ground_object);

    // invalid moving sprite
    if (!moving_ground_object) {
        return;
    }

    // does not move
    if (Is_Float_Equal(moving_ground_object->m_velx, 0.0f) && Is_Float_Equal(moving_ground_object->m_vely, 0.0f)) {
        return;
    }

    // check ground first because of the moving object velocity
    Check_on_Ground();
    // save posx for possible can not move test
    float posy_orig = m_pos_y;
    /* stop object from getting stopped of the moving object which did not yet move itself
     * for example the player always moves as last
    */
    bool is_massive = 0;
    if (moving_ground_object->m_massive_type == MASS_MASSIVE) {
        moving_ground_object->m_massive_type = MASS_PASSIVE;
        is_massive = 1;
    }
    // move
    Col_Move(moving_ground_object->m_velx, moving_ground_object->m_vely, 0, 0, 0);

    if (is_massive) {
        moving_ground_object->m_massive_type = MASS_MASSIVE;
    }
    // if ground object is moving up
    if (moving_ground_object->m_vely < -0.01f) {
        // test if we could not move upwards because something did block us in Col_Move()
        if (Is_Float_Equal(m_pos_y, posy_orig)) {
            // massive
            if (moving_ground_object->m_massive_type == MASS_MASSIVE) {
                // got crunched
                DownGrade(1);
            }
            // halfmassive
            else if (moving_ground_object->m_massive_type == MASS_HALFMASSIVE) {
                // lost ground
                Move(0.0f, 1.9f, 1);
                Reset_On_Ground();
            }
        }
    }
}

void cMovingSprite::Freeze(float freeze_time /* = speedfactor_fps * 10 */)
{
    m_freeze_counter = freeze_time;

    // apply resistance
    if (m_ice_resistance > 0.0f) {
        m_freeze_counter *= (m_ice_resistance * -1) + 1;
    }

    Update_Valid_Update();
}

void cMovingSprite::Update_Rotation_Hor(bool start_rotation /* = 0 */)
{
    switch (m_direction) {
    case DIR_LEFT: {
        m_rot_y = 0.0f;
        break;
    }
    case DIR_RIGHT: {
        m_rot_y = 180.0f;
        break;
    }
    case DIR_UP: {
        m_rot_y = 0.0f;
        break;
    }
    case DIR_DOWN: {
        m_rot_y = 180.0f;
        break;
    }
    case DIR_HORIZONTAL: {
        // left
        if (m_velx < 0.0f) {
            m_rot_y = 0.0f;
        }
        // right
        else if (m_velx > 0.0f) {
            m_rot_y = 180.0f;
        }
        break;
    }
    case DIR_VERTICAL: {
        // up
        if (m_vely < 0.0f) {
            m_rot_y = 0.0f;
        }
        // down
        else if (m_vely > 0.0f) {
            m_rot_y = 180.0f;
        }
        break;
    }
    default: {
        // not supported
        return;
    }
    }

    if (start_rotation) {
        m_start_rot_y = m_rot_y;
    }
}

void cMovingSprite::Handle_Move_Object_Collision(const cObjectCollision* collision)
{
    // if not massive
    if (m_massive_type != MASS_MASSIVE) {
        return;
    }

    // get object
    cMovingSprite* obj = NULL;

    if (collision->m_array == ARRAY_ENEMY) {
        obj = static_cast<cMovingSprite*>(m_sprite_manager->Get_Pointer(collision->m_number));

        // ignore these enemies
        if (obj->m_type == TYPE_THROMP || obj->m_type == TYPE_EATO || obj->m_type == TYPE_FLYON || obj->m_type == TYPE_STATIC_ENEMY) {
            return;
        }
    }
    else if (collision->m_array == ARRAY_PLAYER) {
        obj = static_cast<cMovingSprite*>(pLevel_Player);
    }
    // not a valid type
    else {
        return;
    }

    // top collision is handled in cMovingSprite::Collide_Move()
    if (collision->m_direction == DIR_BOTTOM) {
        if (obj->m_ground_object && obj->m_ground_object->m_massive_type == MASS_MASSIVE) {
            // got crunched
            obj->DownGrade(1);
        }
        else {
            // move
            obj->Col_Move(0.0f, m_vely, 0, 0, 0);
        }
    }
    else if ((collision->m_direction == DIR_LEFT && m_velx < 0.0f) || (collision->m_direction == DIR_RIGHT && m_velx > 0.0f)) {
        // save posx for possible can not move test
        float posx_orig = obj->m_pos_x;
        // move with check on ground
        obj->Col_Move(m_velx, 0.0f, 0, 0);

        // test if we could not move it upwards because something did block it
        if (Is_Float_Equal(obj->m_pos_x, posx_orig)) {
            // got crunched
            obj->DownGrade(1);
        }
    }
}

Col_Valid_Type cMovingSprite::Validate_Collision(cSprite* obj)
{
    if (obj->m_massive_type == MASS_MASSIVE) {
        return COL_VTYPE_BLOCKING;
    }
    else if (obj->m_massive_type == MASS_HALFMASSIVE) {
        // if moving downwards and the object is on bottom
        if (m_vely >= 0.0f && Is_On_Top(obj)) {
            return COL_VTYPE_BLOCKING;
        }
    }

    return COL_VTYPE_NOT_VALID;
}

Col_Valid_Type cMovingSprite::Validate_Collision_Ghost(cSprite* obj)
{
    if (obj->m_type == TYPE_BONUS_BOX || obj->m_type == TYPE_SPIN_BOX) {
        cBaseBox* box = static_cast<cBaseBox*>(obj);

        // ghost
        if (box->m_box_invisible == BOX_GHOST) {
            // enemies ignore ghost objects
            // todo : unless ghost itself
            if (m_sprite_array == ARRAY_ENEMY) {
                return COL_VTYPE_NOT_VALID;
            }

            // maryo is not ghost
            if (pLevel_Player->m_maryo_type != MARYO_GHOST) {
                return COL_VTYPE_NOT_VALID;
            }
        }
    }

    return COL_VTYPE_NOT_POSSIBLE;
}

Col_Valid_Type cMovingSprite::Validate_Collision_Object_On_Top(cMovingSprite* moving_sprite)
{
    /* hack : ignore early jumping player because we collide move earlier
    * also see cPlayer::Handle_Collision_Massive
    */
    if (moving_sprite->m_type == TYPE_PLAYER) {
        cLevel_Player* player = static_cast<cLevel_Player*>(moving_sprite);

        if (player->m_jump_power > 6.0f) {
            // invalid
            return COL_VTYPE_NOT_POSSIBLE;
        }
    }

    // don't handle if not moving upwards or slower
    if (moving_sprite->Is_On_Top(this) && m_direction == DIR_UP && moving_sprite->m_vely > m_vely && moving_sprite->m_can_be_on_ground) {
        // halfmassive
        if (m_massive_type == MASS_HALFMASSIVE) {
            // only if no ground
            if (!moving_sprite->m_ground_object) {
                return COL_VTYPE_BLOCKING;
            }
        }
        // massive
        else if (m_massive_type == MASS_MASSIVE) {
            // always pick up
            if (moving_sprite->m_ground_object != this) {
                moving_sprite->m_ground_object = this;
                return COL_VTYPE_NOT_VALID;
            }
        }
    }

    // invalid
    return COL_VTYPE_NOT_POSSIBLE;
}

void cMovingSprite::Send_Collision(const cObjectCollision* collision, bool handle_now /* = 0 */)
{
    // empty collision
    if (!collision) {
        return;
    }

    // if no target object number is available
    if (collision->m_number < 0) {
        return;
    }

    /* if collision is received ignore it
     * a received collision can't create another received collision
     * only a self detected collision can create a received collision
    */
    if (collision->m_received) {
        return;
    }

    int my_number;

    // player is always 0
    if (m_type == TYPE_PLAYER) {
        my_number = 0;
    }
    // get sprite array number
    else {
        my_number = m_sprite_manager->Get_Array_Num(this);

        // object not available in manager
        if (my_number < 0) {
            debug_print("Warning : Object %s did send Collision but doesn't exists in Manager\n", m_name.c_str());
            return;
        }
    }

    cSprite* target_obj;

    if (collision->m_array == ARRAY_PLAYER) {
        target_obj = pActive_Player;
    }
    else {
        target_obj = m_sprite_manager->Get_Pointer(collision->m_number);
    }

    // check if this is already in list
    if (target_obj->Is_Collision_Included(this)) {
        return;
    }

    // create the new collision
    cObjectCollision* new_collision = new cObjectCollision();
    // this is a received collision
    new_collision->m_received = 1;

    // set object
    new_collision->m_obj = this;
    // set object manager id
    new_collision->m_number = my_number;

    // set direction
    if (collision->m_direction != DIR_UNDEFINED) {
        new_collision->m_direction = Get_Opposite_Direction(collision->m_direction);
    }

    // set type
    new_collision->m_array = m_sprite_array;

    // handle now
    if (handle_now) {
        target_obj->Handle_Collision(new_collision);
        delete new_collision;
    }
    // add collision to the list
    else {
        target_obj->Add_Collision(new_collision);
    }
}

void cMovingSprite::Handle_Collision(cObjectCollision* collision)
{
    // ignore player/enemy if frozen
    if (collision->m_array == ARRAY_PLAYER || collision->m_array == ARRAY_ENEMY) {
        if (m_freeze_counter) {
            return;
        }
    }

    cSprite::Handle_Collision(collision);
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

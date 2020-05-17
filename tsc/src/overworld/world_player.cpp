/***************************************************************************
 * world_player.cpp  - Overworld Player class
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

#include "../core/global_basic.hpp"
#include "../core/game_core.hpp"
#include "../core/framerate.hpp"
#include "../overworld/overworld.hpp"
#include "../level/level.hpp"
#include "../audio/audio.hpp"
#include "../gui/menu.hpp"
#include "../video/renderer.hpp"
#include "../overworld/world_player.hpp"

using namespace std;

namespace TSC {

/* *** *** *** *** *** *** *** *** cOverworld_Player *** *** *** *** *** *** *** *** *** */

cOverworld_Player::cOverworld_Player(cSprite_Manager* sprite_manager, cOverworld* overworld)
    : cMovingSprite(sprite_manager)
{
    m_sprite_array = ARRAY_PLAYER;
    m_type = TYPE_PLAYER;
    m_pos_z = 0.0999f;
    m_massive_type = MASS_MASSIVE;
    m_camera_range = 0;
    m_name = "Alex";

    m_overworld = overworld;
    m_current_waypoint = -2; // no waypoint
    m_line_waypoint = 0;
    m_current_line = -2;

    m_fixed_walking = 0;

    m_line_hor = cLine_collision();
    m_line_ver = cLine_collision();

    m_debug_current_line_last = -100;
    m_debug_lines_last = -100;
    m_debug_current_waypoint_last = -100;

    m_debug_lines = new cHudSprite(m_sprite_manager);
    m_debug_lines->Set_Pos(150, game_res_h * 0.97f, 1);
    m_debug_current_line = new cHudSprite(m_sprite_manager);
    m_debug_current_line->Set_Pos(300, game_res_h * 0.97f, 1);
    m_debug_current_waypoint = new cHudSprite(m_sprite_manager);
    m_debug_current_waypoint->Set_Pos(500, game_res_h * 0.97f, 1);

    Set_Type(ALEX_SMALL);
    m_direction = DIR_DOWN;
    Set_Direction(DIR_UNDEFINED);
}

cOverworld_Player::~cOverworld_Player(void)
{
    delete m_debug_lines;
    delete m_debug_current_line;
    delete m_debug_current_waypoint;

    Unload_Images();
}

void cOverworld_Player::Load_Images(void)
{
    Unload_Images();

    if (m_alex_state == ALEX_SMALL) {
        Add_Image(pVideo->Get_Surface("world/alex/small/down.png"));
        Add_Image(pVideo->Get_Surface("world/alex/small/down_1.png"));
        Add_Image(pVideo->Get_Surface("world/alex/small/down.png"));
        Add_Image(pVideo->Get_Surface("world/alex/small/down_2.png"));
        Add_Image(pVideo->Get_Surface("world/alex/small/up_1.png"));
        Add_Image(pVideo->Get_Surface("world/alex/small/up.png"));
        Add_Image(pVideo->Get_Surface("world/alex/small/up_1.png"));
        Add_Image(pVideo->Get_Surface("world/alex/small/up_2.png"));
        Add_Image(pVideo->Get_Surface("world/alex/small/left.png"));
        Add_Image(pVideo->Get_Surface("world/alex/small/left_1.png"));
        Add_Image(pVideo->Get_Surface("world/alex/small/left.png"));
        Add_Image(pVideo->Get_Surface("world/alex/small/left_2.png"));
        Add_Image(pVideo->Get_Surface("world/alex/small/right.png"));
        Add_Image(pVideo->Get_Surface("world/alex/small/right_1.png"));
        Add_Image(pVideo->Get_Surface("world/alex/small/right.png"));
        Add_Image(pVideo->Get_Surface("world/alex/small/right_2.png"));

        Set_Animation(1);
    }
    else {
        cerr << "Unsupported Alex state : " << static_cast<unsigned int>(m_alex_state) << endl;
        return;
    }

    Set_Image_Num(0, 1);
}

void cOverworld_Player::Unload_Images(void)
{
    Clear_Images();
    Reset_Animation();
}

void cOverworld_Player::Set_Overworld(cOverworld* overworld)
{
    m_overworld = overworld;
}

void cOverworld_Player::Set_Direction(const ObjectDirection dir, bool new_start_direction /* = 0 */)
{
    if (dir != m_direction) {
        Reset_Animation();

        // undefined is set if not moving
        if (dir == DIR_UNDEFINED) {
            Set_Animation_Image_Range(0, 3);
            Set_Time_All(300, 1);
        }
        else if (dir == DIR_DOWN) {
            Set_Animation_Image_Range(0, 3);
            Set_Time_All(200, 1);
        }
        else if (dir == DIR_UP) {
            Set_Animation_Image_Range(4, 7);
            Set_Time_All(200, 1);
        }
        else if (dir == DIR_LEFT) {
            Set_Animation_Image_Range(8, 11);
            Set_Time_All(200, 1);
        }
        else if (dir == DIR_RIGHT) {
            Set_Animation_Image_Range(12, 15);
            Set_Time_All(200, 1);
        }

        Set_Image_Num(m_anim_img_start);
    }

    cMovingSprite::Set_Direction(dir, new_start_direction);

    Update_Vel();
}

void cOverworld_Player::Set_Type(Alex_type new_type)
{
    // already set
    if (m_alex_state == new_type) {
        return;
    }

    m_alex_state = new_type;
    Load_Images();
}

void cOverworld_Player::Update(void)
{
    cMovingSprite::Update();

    Update_Animation();

    if (m_direction == DIR_UNDEFINED) {
        return;
    }

    // default walking
    if (!m_fixed_walking) {
        Update_Walk();
    }
    // fixed walking
    else {
        Update_Waypoint_Walk();
    }
}

void cOverworld_Player::Draw(cSurface_Request* request /* = NULL */)
{
    bool create_request = 0;

    if (!request) {
        create_request = 1;
        // create request
        request = new cSurface_Request();
    }

    // Draw player
    cMovingSprite::Draw(request);
    // alpha in debug mode
    if (pOverworld_Manager->m_debug_mode) {
        request->m_color.alpha = 64;
    }

    if (create_request) {
        // add request
        pRenderer->Add(request);
    }

    // Draw debug text
    Draw_Debug_Text();
}

void cOverworld_Player::Draw_Debug_Text(void)
{
    if (!pOverworld_Manager->m_debug_mode) {
        return;
    }
}

void cOverworld_Player::Reset(void)
{
    m_current_waypoint = -2;
    m_line_waypoint = 0;
    m_current_line = -2;

    m_fixed_walking = 0;
    Set_Direction(DIR_UNDEFINED);
}

void cOverworld_Player::Action_Interact(input_identifier key_type)
{
    // Left
    if (key_type == INP_LEFT) {
        pOverworld_Player->Start_Walk(DIR_LEFT);
    }
    // Right
    else if (key_type == INP_RIGHT) {
        pOverworld_Player->Start_Walk(DIR_RIGHT);
    }
    // Up
    else if (key_type == INP_UP) {
        pOverworld_Player->Start_Walk(DIR_UP);
    }
    // Down
    else if (key_type == INP_DOWN) {
        pOverworld_Player->Start_Walk(DIR_DOWN);
    }
    // Action
    else if (key_type == INP_ACTION) {
        // enter
        pOverworld_Player->Activate_Waypoint();
        Clear_Input_Events();
    }
    // Exit
    else if (key_type == INP_EXIT) {
        Game_Action = GA_ENTER_MENU;
        Game_Action_Data_Middle.add("load_menu", int_to_string(MENU_MAIN));
        Game_Action_Data_Middle.add("menu_exit_back_to", int_to_string(MODE_OVERWORLD));
    }
}

void cOverworld_Player::Action_Stop_Interact(input_identifier key_type)
{
    // nothing yet
}

void cOverworld_Player::Activate_Waypoint(void)
{
    // if no waypoint or already walking
    if (m_current_waypoint < 0 || m_direction != DIR_UNDEFINED) {
        return;
    }

    // get waypoint
    cWaypoint* waypoint = Get_Waypoint();

    // normal level waypoint
    if (waypoint->m_waypoint_type == WAYPOINT_NORMAL) {
        // Enter Level
        Game_Action = GA_ENTER_LEVEL;
        Game_Action_Data_Start.add("music_fadeout", "1000");
        Game_Action_Data_Start.add("screen_fadeout", int_to_string(EFFECT_OUT_FIXED_COLORBOX));
        Game_Action_Data_Middle.add("load_level", waypoint->Get_Destination());
        Game_Action_Data_End.add("screen_fadein", int_to_string(EFFECT_IN_RANDOM));
    }
    // world link waypoint
    else if (waypoint->m_waypoint_type == WAYPOINT_WORLD_LINK) {
        // remember Overworld origin
        cOverworld* overworld_origin = m_overworld;

        // world string
        std::string str_world = waypoint->Get_Destination();

        // Enter Credits Menu ( World End )
        if (str_world.compare("credits") == 0) {
            Game_Action = GA_ENTER_MENU;
            Game_Action_Data_Start.add("music_fadeout", "1500");
            Game_Action_Data_Start.add("screen_fadeout", int_to_string(EFFECT_OUT_HORIZONTAL_VERTICAL));
            Game_Action_Data_Middle.add("load_menu", int_to_string(MENU_CREDITS));
            Game_Action_Data_Middle.add("menu_exit_back_to", int_to_string(MODE_OVERWORLD));
            Game_Action_Data_End.add("screen_fadein", int_to_string(EFFECT_IN_RANDOM));
        }
        // world link
        else {
            if (pOverworld_Manager->Get(str_world)) {
                Game_Action = GA_ENTER_WORLD;
                Game_Action_Data_Start.add("music_fadeout", "1500");
                Game_Action_Data_Start.add("screen_fadeout", int_to_string(EFFECT_OUT_BLACK_TILED_RECTS));
                Game_Action_Data_Middle.add("enter_world", str_world.c_str());
                Game_Action_Data_Middle.add("world_player_waypoint", path_to_utf8(overworld_origin->m_description->m_path).c_str());
                Game_Action_Data_End.add("screen_fadein", int_to_string(EFFECT_IN_BLACK));
            }
            else {
                cerr << "Warning : Overworld not found " << str_world << endl;
            }
        }
    }
}

void cOverworld_Player::Update_Vel(void)
{
    if (m_direction == DIR_UP) {
        m_velx = 0.0f;
        m_vely = -3.0f;
    }
    else if (m_direction == DIR_DOWN) {
        m_velx = 0.0f;
        m_vely = 3.0f;
    }
    else if (m_direction == DIR_LEFT) {
        m_velx = -3.0f;
        m_vely = 0.0f;
    }
    else if (m_direction == DIR_RIGHT) {
        m_velx = 3.0f;
        m_vely = 0.0f;
    }
}

bool cOverworld_Player::Start_Walk(ObjectDirection new_direction)
{
    // already walking into the given direction
    if (new_direction == m_direction) {
        return 0;
    }

    // invalid direction
    if (!(new_direction == DIR_UP || new_direction == DIR_DOWN || new_direction == DIR_LEFT || new_direction == DIR_RIGHT)) {
        cerr << "Warning : New direction is invalid : " << new_direction << endl;
        return 0;
    }

    // print debug info
    if (pOverworld_Manager->m_debug_mode) {
        cout << "Current Alex Direction : " << m_direction << endl;

        if (m_current_waypoint > 0) {
            cout << "Legacy Waypoint Direction Forward : " << Get_Waypoint()->m_direction_forward << " Backward : " << Get_Waypoint()->m_direction_backward << endl;
            cout << "Waypoint exit count: " << Get_Waypoint()->m_exits.size() << endl;
        }
    }

    cLayer_Line_Point_Start* walk_line = NULL;
    if (m_current_waypoint >= 0 && Get_Waypoint()->m_exits.size() > 0) { // Since 2.1.0
        waypoint_exit* p_exit = NULL;

        // Find the line matching the requested direction
        for(auto iter=Get_Waypoint()->m_exits.begin(); iter != Get_Waypoint()->m_exits.end(); iter++) {
            if (iter->direction == new_direction && !iter->locked) {
                p_exit = &(*iter);
                break;
            }
        }

        // No matching unlocked waypoint exit
        if (!p_exit) {
            return 0;
        }

        walk_line = Get_Line_By_UID(p_exit->line_start_uid);
        if (!walk_line) {
            cout << "Line with UID point " << p_exit->line_start_uid << " not found." << endl;
            return 0;
        }

        // Not only lines, also waypoints have access rights
        cWaypoint* next_waypoint = (walk_line->Get_Opposite_Waypoint_for_UID(p_exit->line_start_uid));
        if (!check_access_to_waypoint(next_waypoint))
            return 0;

        // set line waypoint
        m_line_waypoint = walk_line->m_origin;
    }
    else { // Legacy, pre-2.1.0 format using forward and backward direction
        // a start from waypoint
        if (m_current_waypoint >= 0 && m_direction == DIR_UNDEFINED) {
            // Get Layer Line in front
            cLayer_Line_Point_Start* front_line = Get_Front_Line(new_direction);

            if (!front_line) {
                if (pOverworld_Manager->m_debug_mode) {
                    cout << "Waypoint Front line not detected" << endl;
                }

                return 0;
            }

            if (pOverworld_Manager->m_debug_mode) {
                cout << "Waypoint Front line id " << m_overworld->m_layer->Get_Array_Num(front_line) << ", origin id " << front_line->m_origin << endl;
            }

            // forward
            if (Get_Waypoint()->m_direction_forward == new_direction) {
                cWaypoint* next_waypoint = front_line->Get_End_Waypoint();

                if (!check_access_to_waypoint(next_waypoint))
                    return 0;

                // set line waypoint
                m_line_waypoint = front_line->m_origin;
            }
            // backward: always unlocked, no need to check access
            else if (Get_Waypoint()->m_direction_backward == new_direction) {
                // set line waypoint
                m_line_waypoint = front_line->m_origin;
            }
            // invalid direction
            else {
                return 0;
            }
        }
    }

    // remember current direction
    ObjectDirection direction_old = m_direction;

    // if already walking
    if (m_direction != DIR_UNDEFINED) {
        if (m_direction == Get_Opposite_Direction(new_direction) && !m_fixed_walking && m_overworld->Get_Waypoint_Collision(m_col_rect) < 0) {
            Set_Direction(new_direction);
        }
    }
    // start walking
    else {
        Set_Direction(new_direction);
        // force a small move into the new direction
        Move(m_velx, m_vely, 1);
    }

    // if alex is not on his current waypoint collide with every waypoint
    if (m_current_waypoint >= 0 && !m_col_rect.Intersects(Get_Waypoint()->m_rect)) {
        m_current_waypoint = -1;
    }

    // if alex is walking into a new direction
    if (direction_old != m_direction) {
        return 1;
    }

    // direction unchanged
    return 0;
}

void cOverworld_Player::Update_Walk(void)
{
    Move(m_velx, m_vely);
    Update_Path_Diff();

    /* Check if Alex is too far away from the line in the current direction
     * or if no line is anymore connected
     * if true change direction
    */
    if (((m_direction == DIR_UP || m_direction == DIR_DOWN) && m_line_hor.m_line_number >= 0 && (m_line_hor.m_difference > 20 || m_line_hor.m_difference < -20)) ||
            ((m_direction == DIR_LEFT || m_direction == DIR_RIGHT) && m_line_ver.m_line_number >= 0 && (m_line_ver.m_difference > 20 || m_line_ver.m_difference < -20)) ||
            (m_line_hor.m_line_number < 0 && m_line_ver.m_line_number < 0)) {
        if (pOverworld_Manager->m_debug_mode) {
            if (m_direction == DIR_UP || m_direction == DIR_DOWN) {
                cout << "horizontal line connection broken : num " << m_line_hor.m_line_number << " diff " << m_line_hor.m_difference << endl;
            }
            else if (m_direction == DIR_LEFT || m_direction == DIR_RIGHT) {
                cout << "vertical line connection broken : num " << m_line_ver.m_line_number << " diff " << m_line_ver.m_difference << endl;
            }
        }

        m_current_waypoint = -1;

        Change_Direction();
    }

    Auto_Pos_Correction();

    // check if a new waypoint is near alex
    for (unsigned int i = 0; i < m_overworld->m_waypoints.size(); i++) {
        // skip the start waypoint
        if (static_cast<int>(i) == m_current_waypoint) {
            continue;
        }

        if (m_overworld->m_waypoints[i]->m_rect.Intersects(m_col_rect)) {
            Start_Waypoint_Walk(i);
            break;
        }
    }
}

void cOverworld_Player::Start_Waypoint_Walk(int new_waypoint)
{
    m_fixed_walking = 1;
    m_current_waypoint = new_waypoint;
    m_current_line = -2;

    // Unlock backwards direction waypoint exit on a non-legacy waypoint
    // (otherwise the player can't go backwards).
    cWaypoint* waypoint = m_overworld->m_waypoints[m_current_waypoint];
    if (!waypoint->m_exits.empty()) {
        for(waypoint_exit& ex: waypoint->m_exits) {
            if (ex.direction == Get_Opposite_Direction(m_direction)) {
                ex.locked = false;
                break;
            }
        }
    }

    m_overworld->Update_Waypoint_text();
}

void cOverworld_Player::Update_Waypoint_Walk(void)
{
    // invalid current waypoint
    if (m_current_waypoint < 0) {
        return;
    }

    unsigned int reached = 0;
    cWaypoint* waypoint = Get_Waypoint();

    // horizontal position to waypoint
    const float x = m_col_rect.m_x + (m_col_rect.m_w * 0.5f);

    if (waypoint->m_rect.m_x + (waypoint->m_rect.m_w * 0.4f) > x) {
        Move(3.0f, 0.0f);
    }
    else if (waypoint->m_rect.m_x + (waypoint->m_rect.m_w * 0.7f) < x) {
        Move(-3.0f, 0.0f);
    }
    else {
        reached++;
    }

    // vertical position to waypoint
    const float y = m_col_rect.m_y + (m_col_rect.m_h * 0.5f);

    if (waypoint->m_rect.m_y + (waypoint->m_rect.m_h * 0.4f) > y) {
        Move(0.0f, 3.0f);
    }
    else if (waypoint->m_rect.m_y + (waypoint->m_rect.m_h * 0.7f) < y) {
        Move(0.0f, -3.0f);
    }
    else {
        reached++;
    }

    if (reached == 2) {
        m_fixed_walking = 0;

        Set_Direction(DIR_UNDEFINED);
        Set_Waypoint(m_current_waypoint);

        pAudio->Play_Sound("waypoint_reached.ogg");
    }
}

bool cOverworld_Player::Set_Waypoint(int waypoint, bool new_startpos /* = 0 */)
{
    if (waypoint < 0 || waypoint >= static_cast<int>(m_overworld->m_waypoints.size())) {
        return 0;
    }

    cWaypoint* wp = m_overworld->m_waypoints[waypoint];

    Set_Pos(wp->m_rect.m_x - m_col_pos.m_x + ((wp->m_rect.m_w - m_col_rect.m_w) * 0.5f), wp->m_rect.m_y - m_col_pos.m_y + ((wp->m_rect.m_h - m_col_rect.m_h) * 0.5f), new_startpos);
    m_current_waypoint = waypoint;

    // Update Camera
    pActive_Camera->Update();
    // Update Waypoint text
    m_overworld->Update_Waypoint_text();

    return 1;
}

cWaypoint* cOverworld_Player::Get_Waypoint(void)
{
    if (m_current_waypoint < 0) {
        return NULL;
    }

    return m_overworld->Get_Waypoint(m_current_waypoint);
}

cLayer_Line_Point_Start* cOverworld_Player::Get_Front_Line(ObjectDirection dir) const
{
    return m_overworld->m_layer->Get_Line_Collision_Direction(m_col_rect.m_x + (m_col_rect.m_w * 0.5f), m_col_rect.m_y + (m_col_rect.m_h * 0.5f), dir).m_line;
}

cLayer_Line_Point_Start* cOverworld_Player::Get_Line_By_UID(int uid)
{
    return m_overworld->m_layer->Get_Line_Start_By_UID(uid);
}

void cOverworld_Player::Update_Path_Diff(unsigned int check_size /* = 25 */)
{
    float x = m_col_rect.m_x + (m_col_rect.m_w * 0.5f) + m_velx;
    float y = m_col_rect.m_y + (m_col_rect.m_h * 0.5f) + m_vely;

    // bigger direction check size
    unsigned int hor_advance = 0;
    unsigned int ver_advance = 0;
    if (m_direction == DIR_LEFT || m_direction == DIR_RIGHT) {
        ver_advance += 10;
    }
    else if (m_direction == DIR_UP || m_direction == DIR_DOWN) {
        hor_advance += 10;
    }

    // get nearest lines
    m_line_hor = m_overworld->m_layer->Get_Nearest(x, y, DIR_HORIZONTAL, check_size + hor_advance, m_line_waypoint);
    m_line_ver = m_overworld->m_layer->Get_Nearest(x, y, DIR_VERTICAL, check_size + ver_advance, m_line_waypoint);
}

void cOverworld_Player::Change_Direction(void)
{
    m_line_hor = cLine_collision();
    m_line_ver = cLine_collision();

    float x = m_col_rect.m_x + (m_col_rect.m_w * 0.5f);
    float y = m_col_rect.m_y + (m_col_rect.m_h * 0.5f);

    /* check with a big collision line the difference to the next layer line behind alex
     * if no line is found turn around
     * if a line is found walk into it's direction
    */
    if (m_direction == DIR_RIGHT || m_direction == DIR_LEFT) {
        // search for a line with increasing distance
        for (unsigned int i = 0; i < 30; i++) {
            m_line_ver = m_overworld->m_layer->Get_Nearest(x - i, y, DIR_VERTICAL, 80, m_line_waypoint);

            // found
            if (m_line_ver.m_line_number >= 0) {
                break;
            }

            m_line_ver = m_overworld->m_layer->Get_Nearest(x + i, y, DIR_VERTICAL, 80, m_line_waypoint);

            // found
            if (m_line_ver.m_line_number >= 0) {
                break;
            }
        }

        // if line is found
        if (m_line_ver.m_line_number >= 0) {
            if (m_line_ver.m_difference < 0) {
                Set_Direction(DIR_UP);
            }
            else if (m_line_ver.m_difference > 0) {
                Set_Direction(DIR_DOWN);
            }
        }
        // if no line found walk in to the opposite direction
        else {
            Set_Direction(Get_Opposite_Direction(m_direction));
        }
    }
    else if (m_direction == DIR_UP || m_direction == DIR_DOWN) {
        // search for a line with increasing distance
        for (unsigned int i = 0; i < 30; i++) {
            m_line_hor = m_overworld->m_layer->Get_Nearest(x, y + i, DIR_HORIZONTAL, 80, m_line_waypoint);

            // found
            if (m_line_hor.m_line_number >= 0) {
                break;
            }

            m_line_hor = m_overworld->m_layer->Get_Nearest(x, y - i, DIR_HORIZONTAL, 80, m_line_waypoint);

            // found
            if (m_line_hor.m_line_number >= 0) {
                break;
            }
        }

        // if line is found
        if (m_line_hor.m_line_number >= 0) {
            if (m_line_hor.m_difference < 0) {
                Set_Direction(DIR_LEFT);
            }
            else if (m_line_hor.m_difference > 0) {
                Set_Direction(DIR_RIGHT);
            }
        }
        // if no line found walk in to the opposite direction
        else {
            Set_Direction(Get_Opposite_Direction(m_direction));
        }
    }
    else {
        cerr << "Warning : Unknown Alex direction " << static_cast<int>(m_direction) << endl;
        Set_Direction(DIR_DOWN);
    }

    if (pOverworld_Manager->m_debug_mode) {
        cout << "Changed direction to " << Get_Direction_Name(m_direction) << endl;
    }
}

void cOverworld_Player::Auto_Pos_Correction(float size /* = 1.7f */, float min_distance /* = 5.0f */)
{
    // if colliding with a waypoint
    if (m_overworld->Get_Waypoint_Collision(m_col_rect) != -1) {
        return;
    }

    if (m_direction == DIR_LEFT || m_direction == DIR_RIGHT) {
        // up and down correction
        if (m_line_ver.m_difference > min_distance) {
            Move(0.0f, size);
        }
        else if (m_line_ver.m_difference < -min_distance) {
            Move(0.0f, -size);
        }

        // Set current line number
        if (m_current_line != m_line_ver.m_line_number) {
            m_current_line = m_line_ver.m_line_number;
        }
    }
    else if (m_direction == DIR_UP || m_direction == DIR_DOWN) {
        // left correction
        if (m_line_hor.m_difference < -min_distance) {
            Move(-size, 0.0f);
        }
        // right correction
        else if (m_line_hor.m_difference > min_distance) {
            Move(size, 0.0f);
        }

        // Set current line number
        if (m_current_line != m_line_hor.m_line_number) {
            m_current_line = m_line_hor.m_line_number;
        }
    }
}

bool cOverworld_Player::check_access_to_waypoint(cWaypoint* p_wp)
{
    if (!p_wp) {
        cout << "Next waypoint not detected" << endl;
        return 0;
    }

    // next waypoint is not accessible
    if (!p_wp->m_access) {
        if (pOverworld_Manager->m_debug_mode) {
            cout << "No access to next waypoint" << endl;
        }

        return 0;
    }

    return 1;
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

cOverworld_Player* pOverworld_Player = NULL;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

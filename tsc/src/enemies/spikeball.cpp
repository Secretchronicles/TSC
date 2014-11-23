/***************************************************************************
 * spikeball.cpp  -  spiky enemy
 *
 * Copyright © 2009 - 2011 Florian Richter
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

#include "../enemies/spikeball.hpp"
#include "../core/game_core.hpp"
#include "../level/level_player.hpp"
#include "../gui/hud.hpp"
#include "../core/i18n.hpp"
#include "../video/animation.hpp"
#include "../core/sprite_manager.hpp"
#include "../core/xml_attributes.hpp"

namespace TSC {

/* *** *** *** *** *** *** cSpikeball *** *** *** *** *** *** *** *** *** *** *** */

cSpikeball::cSpikeball(cSprite_Manager* sprite_manager)
    : cEnemy(sprite_manager)
{
    cSpikeball::Init();
}

cSpikeball::cSpikeball(XmlAttributes& attributes, cSprite_Manager* sprite_manager)
    : cEnemy(sprite_manager)
{
    cSpikeball::Init();

    // position
    Set_Pos(string_to_float(attributes["posx"]), string_to_float(attributes["posy"]), true);

    // color
    Set_Color(static_cast<DefaultColor>(Get_Color_Id(attributes.fetch("color", Get_Color_Name(m_color_type)))));

    // direction
    Set_Direction(Get_Direction_Id(attributes.fetch("direction", Get_Direction_Name(m_start_direction))));
}

cSpikeball::~cSpikeball(void)
{
    //
}

void cSpikeball::Init(void)
{
    m_type = TYPE_SPIKEBALL;
    m_name = "Spikeball";
    m_pos_z = 0.09f;
    m_gravity_max = 29.0f;

    m_counter_stay = 0.0f;
    m_counter_walk = 0.0f;
    m_counter_running = 0.0f;
    m_running_particle_counter = 0.0f;

    m_color_type = COL_DEFAULT;
    Set_Color(COL_GREY);
    m_state = STA_FALL;
    Set_Moving_State(STA_WALK);
    Set_Direction(DIR_RIGHT);
}

cSpikeball* cSpikeball::Copy(void) const
{
    cSpikeball* spikeball = new cSpikeball(m_sprite_manager);
    spikeball->Set_Pos(m_start_pos_x, m_start_pos_y);
    spikeball->Set_Color(m_color_type);
    spikeball->Set_Direction(m_start_direction);
    return spikeball;
}

std::string cSpikeball::Get_XML_Type_Name()
{
    return "spikeball";
}

xmlpp::Element* cSpikeball::Save_To_XML_Node(xmlpp::Element* p_element)
{
    xmlpp::Element* p_node = cEnemy::Save_To_XML_Node(p_element);

    Add_Property(p_node, "color", Get_Color_Name(m_color_type));
    Add_Property(p_node, "direction", Get_Direction_Name(m_start_direction));

    return p_node;
}

void cSpikeball::Load_From_Savegame(cSave_Level_Object* save_object)
{
    cEnemy::Load_From_Savegame(save_object);

    Update_Rotation_Hor();
}

void cSpikeball::Set_Direction(const ObjectDirection dir, bool initial /* = true */)
{
    // already set
    if (m_start_direction == dir) {
        return;
    }

    cEnemy::Set_Direction(dir, initial);

    Update_Rotation_Hor(1);
}

void cSpikeball::Set_Color(const DefaultColor& col)
{
    // already set
    if (m_color_type == col) {
        return;
    }

    // clear old images
    Clear_Images();

    m_color_type = col;
    std::string filename_dir;

    if (m_color_type == COL_GREY) {
        filename_dir = "grey";

        m_kill_points = 400;
        m_fire_resistant = 1;
        m_ice_resistance = 1.0f;
        m_can_be_hit_from_shell = 0;
    }
    else {
        printf("Error : Unknown Spikeball Color %d\n", m_color_type);
        return;
    }

    Update_Velocity_Max();

    Add_Image(pVideo->Get_Surface("enemy/spikeball/" + filename_dir + "/walk_1.png"));
    Add_Image(pVideo->Get_Surface("enemy/spikeball/" + filename_dir + "/walk_2.png"));
    Add_Image(pVideo->Get_Surface("enemy/spikeball/" + filename_dir + "/walk_3.png"));
    Add_Image(pVideo->Get_Surface("enemy/spikeball/" + filename_dir + "/walk_4.png"));
    Add_Image(pVideo->Get_Surface("enemy/spikeball/" + filename_dir + "/walk_5.png"));
    Add_Image(pVideo->Get_Surface("enemy/spikeball/" + filename_dir + "/walk_6.png"));
    Add_Image(pVideo->Get_Surface("enemy/spikeball/" + filename_dir + "/walk_7.png"));
    Add_Image(pVideo->Get_Surface("enemy/spikeball/" + filename_dir + "/walk_8.png"));
    Add_Image(pVideo->Get_Surface("enemy/spikeball/" + filename_dir + "/turn.png"));
    //Add_Image( pVideo->Get_Surface( "enemy/spikeball/" + filename_dir + "/dead.png" ) );

    Set_Image_Num(0, 1);
}

void cSpikeball::Turn_Around(ObjectDirection col_dir /* = DIR_UNDEFINED */)
{
    cEnemy::Turn_Around(col_dir);

    if (col_dir == DIR_LEFT || col_dir == DIR_RIGHT) {
        if (m_state == STA_WALK || m_state == STA_RUN) {
            m_velx *= 0.5f;
        }

        // set turn around image
        Set_Image_Num(8);
        Set_Animation(0);
        Reset_Animation();
    }
    // only update rotation if no turn around image
    else {
        Update_Rotation_Hor();
    }
}

void cSpikeball::DownGrade(bool force /* = 0 */)
{
    Set_Dead(1);
    m_massive_type = MASS_PASSIVE;
    m_counter = 0.0f;
    m_velx = 0.0f;
    m_vely = 0.0f;
    // dead image
    Set_Image_Num(9);
    Set_Animation(0);

    // default stomp death
    if (!force) {
        Generate_Hit_Animation();
        Set_Scale_Directions(1, 0, 1, 1);
    }
    // falling death
    else {
        Set_Rotation_Z(180);
        Set_Scale_Directions(1, 1, 1, 1);
    }
}

void cSpikeball::Set_Moving_State(Moving_state new_state)
{
    if (new_state == m_state) {
        return;
    }

    if (new_state == STA_STAY) {
        m_counter_walk = 0.0f;
        Set_Animation(0);
    }
    else if (new_state == STA_WALK) {
        m_counter_running = 0.0f;
        m_counter_walk = Get_Random_Float(0.0f, 80.0f);

        Set_Animation(1);
        Set_Animation_Image_Range(0, 7);
        Set_Time_All(130, 1);
        Reset_Animation();
    }
    else if (new_state == STA_RUN) {
        m_counter_stay = 0.0f;

        Set_Animation(1);
        Set_Animation_Image_Range(0, 7);
        Set_Time_All(60, 1);
        Reset_Animation();
    }

    m_state = new_state;

    Update_Velocity_Max();
    // if in the first part of the turn around animation
    Update_Rotation_Hor();
}

void cSpikeball::Update(void)
{
    cEnemy::Update();

    if (!m_valid_update || !Is_In_Range()) {
        return;
    }

    Update_Animation();

    if (m_state == STA_STAY) {
        m_counter_stay += pFramerate->m_speed_factor;

        // slow down
        if (!Is_Float_Equal(m_velx, 0.0f)) {
            Add_Velocity_X(-m_velx * 0.25f);

            if (m_velx < 0.3f && m_velx > -0.3f) {
                m_velx = 0.0f;
            }
        }

        // set turn around image
        if (m_counter_stay > 40.0f && m_curr_img != 8) {
            Set_Image_Num(8);

            // random direction
            if ((rand() % 2) == 1) {
                // turn around
                m_direction = Get_Opposite_Direction(m_direction);
                Update_Rotation_Hor();
            }
        }

        // finished stay animation
        if (m_counter_stay > 60.0f) {
            // run
            Set_Moving_State(STA_RUN);
            //pAudio->Play_Sound( "enemy/spikeball/run.wav" );
        }
    }
    else if (m_state == STA_WALK) {
        m_counter_walk += pFramerate->m_speed_factor;

        // finished walking
        if (m_counter_walk > 240.0f) {
            // stay
            Set_Moving_State(STA_STAY);
        }
    }
    else if (m_state == STA_RUN) {
        m_counter_running += pFramerate->m_speed_factor;

        // finished running
        if (m_counter_running > 120.0f) {
            // walk
            Set_Moving_State(STA_WALK);
        }

        // running particles
        m_running_particle_counter += pFramerate->m_speed_factor * 0.5f;

        // create particles
        if (m_running_particle_counter > 1.0f) {
            cParticle_Emitter* anim = new cParticle_Emitter(m_sprite_manager);
            anim->Set_Emitter_Rect(m_col_rect.m_x, m_col_rect.m_y + m_col_rect.m_h - 2.0f, m_col_rect.m_w);
            anim->Set_Quota(static_cast<int>(m_running_particle_counter));
            anim->Set_Pos_Z(m_pos_z - 0.000001f);
            anim->Set_Image(pVideo->Get_Surface("animation/particles/smoke_black.png"));
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
        if (m_curr_img == 8) {
            m_anim_counter += pFramerate->m_elapsed_ticks;

            // back to normal animation
            if (m_anim_counter >= 200) {
                Reset_Animation();
                Set_Image_Num(m_anim_img_start);
                Set_Animation(1);
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

void cSpikeball::Update_Velocity_Max(void)
{
    if (m_state == STA_WALK) {
        m_velx_max = 1.5f;
        m_velx_gain = 0.3f;
    }
    else if (m_state == STA_RUN) {
        m_velx_max = 3.7f;
        m_velx_gain = 0.6f;
    }
}

Col_Valid_Type cSpikeball::Validate_Collision(cSprite* obj)
{
    // basic validation checking
    Col_Valid_Type basic_valid = Validate_Collision_Ghost(obj);

    // found valid collision
    if (basic_valid != COL_VTYPE_NOT_POSSIBLE) {
        return basic_valid;
    }

    if (obj->m_massive_type == MASS_MASSIVE) {
        switch (obj->m_type) {
        case TYPE_FLYON: {
            return COL_VTYPE_NOT_VALID;
        }
        case TYPE_ROKKO: {
            return COL_VTYPE_NOT_VALID;
        }
        case TYPE_GEE: {
            return COL_VTYPE_NOT_VALID;
        }
        case TYPE_SPIKA: {
            return COL_VTYPE_NOT_VALID;
        }
        case TYPE_STATIC_ENEMY: {
            return COL_VTYPE_NOT_VALID;
        }
        case TYPE_TURTLE_BOSS: {
            return COL_VTYPE_NOT_VALID;
        }
        case TYPE_FURBALL_BOSS: {
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

void cSpikeball::Handle_Collision_Player(cObjectCollision* collision)
{
    // invalid
    if (collision->m_direction == DIR_UNDEFINED) {
        return;
    }

    pLevel_Player->DownGrade_Player();

    if (collision->m_direction == DIR_RIGHT || collision->m_direction == DIR_LEFT) {
        Turn_Around(collision->m_direction);
    }
}

void cSpikeball::Handle_Collision_Enemy(cObjectCollision* collision)
{
    if (collision->m_direction == DIR_RIGHT || collision->m_direction == DIR_LEFT) {
        Turn_Around(collision->m_direction);
    }

    Send_Collision(collision);
}

void cSpikeball::Handle_Collision_Massive(cObjectCollision* collision)
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

void cSpikeball::Handle_Collision_Box(ObjectDirection cdirection, GL_rect* r2)
{
    if (cdirection == DIR_DOWN) {
        m_vely = -10.0f;

        // left
        if (m_pos_x > r2->m_x) {
            m_velx += 3.0f;
        }
        // right
        else if (m_pos_x < r2->m_x) {
            m_velx -= 3.0f;
        }

        Reset_On_Ground();
    }
}

void cSpikeball::Editor_Activate(void)
{
    // get window manager
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    // direction
    CEGUI::Combobox* combobox = static_cast<CEGUI::Combobox*>(wmgr.createWindow("TaharezLook/Combobox", "editor_spikeball_direction"));
    Editor_Add(UTF8_("Direction"), UTF8_("Starting direction."), combobox, 100, 75);

    combobox->addItem(new CEGUI::ListboxTextItem("left"));
    combobox->addItem(new CEGUI::ListboxTextItem("right"));

    combobox->setText(Get_Direction_Name(m_start_direction));
    combobox->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cSpikeball::Editor_Direction_Select, this));

    // init
    Editor_Init();
}

bool cSpikeball::Editor_Direction_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    Set_Direction(Get_Direction_Id(item->getText().c_str()));

    return 1;
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

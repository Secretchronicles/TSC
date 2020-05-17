/***************************************************************************
 * pip.cpp  -  The worm
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

#include "../core/global_game.hpp"
#include "../core/game_core.hpp"
#include "../core/errors.hpp"
#include "../core/property_helper.hpp"
#include "../core/i18n.hpp"
#include "../core/sprite_manager.hpp"
#include "../core/xml_attributes.hpp"
#include "../video/animation.hpp"
#include "../video/gl_surface.hpp"
#include "../gui/hud.hpp"
#include "../level/level_player.hpp"
#include "../user/savegame/savegame.hpp"
#include "../core/sprite_manager.hpp"
#include "../level/level.hpp"
#include "../level/level_settings.hpp"
#include "../core/editor/editor.hpp"
#include "../level/level_editor.hpp"
#include "pip.hpp"

using namespace TSC;

/* *** *** *** *** *** cPip *** *** *** *** *** *** *** *** *** *** *** *** */

cPip::cPip(cSprite_Manager* p_sprite_manager)
    : cEnemy(p_sprite_manager)
{
    cPip::Init();
}

cPip::cPip(XmlAttributes& attributes, cSprite_Manager* p_sprite_manager)
    : cEnemy(p_sprite_manager)
{
    cPip::Init();

    // position
    Set_Pos(string_to_float(attributes["posx"]), string_to_float(attributes["posy"]), true);

    // direction
    Set_Direction(Get_Direction_Id(attributes.fetch("direction", Get_Direction_Name(m_start_direction))));
}

cPip::~cPip()
{
    //
}

void cPip::Init()
{
    m_type = TYPE_PIP;
    m_name = "Pip";
    m_pos_z = 0.093f;
    m_gravity_max = 13.0f;

    Add_Image_Set("big", "enemy/pip/big.imgset", 0, &m_big_start);
    Add_Image_Set("small", "enemy/pip/small.imgset", 0, &m_small_start);

    m_state = STA_FALL;
    Set_Moving_State(STA_WALK);
    Set_Direction(DIR_RIGHT);

    // FIXME: Own die sound
    m_kill_sound = "enemy/krush/die.ogg";
}

cPip* cPip::Copy() const
{
    cPip* p_pip = new cPip(m_sprite_manager);
    p_pip->Set_Pos(m_start_pos_x, m_start_pos_y);
    p_pip->Set_Direction(m_start_direction);
    return p_pip;
}

std::string cPip::Get_XML_Type_Name()
{
    return "pip";
}

xmlpp::Element* cPip::Save_To_XML_Node(xmlpp::Element* p_element)
{
    xmlpp::Element* p_node = cEnemy::Save_To_XML_Node(p_element);

    Add_Property(p_node, "direction", Get_Direction_Name(m_start_direction));

    return p_node;
}

void cPip::Load_From_Savegame(cSave_Level_Object* p_save_object)
{
    // pip_state
    if (p_save_object->exists("state")) {
        Moving_state mov_state = static_cast<Moving_state>(string_to_int(p_save_object->Get_Value("state")));

        if (mov_state == STA_RUN)
            Set_Moving_State(mov_state);
    }

    cEnemy::Load_From_Savegame(p_save_object);

    Update_Rotation_Hor();
}

void cPip::Set_Direction(const ObjectDirection dir)
{
    // already set
    if (m_start_direction == dir)
        return;

    cEnemy::Set_Direction(dir, true);
    Update_Rotation_Hor(true);
}

void cPip::Turn_Around(ObjectDirection col_dir /* DIR_UNDEFINED */)
{
    cEnemy::Turn_Around(col_dir);

    if (col_dir == DIR_LEFT || col_dir == DIR_RIGHT || col_dir == DIR_UNDEFINED) {
        m_velx *= 0.5f;
        Update_Rotation_Hor();
    }
}

void cPip::DownGrade(bool force /* = false */)
{
    if (force) { // Falling death
        Set_Dead(true);
        Set_Rotation_Z(180.0f);
    }
    else {
        /* Note: Issueing a Downgrade event for Pip is difficult, because
         * Pip gets split into two after the first hit. What shall happen
         * to event handlers registered to Pip before it split? Invoking
         * them for both small Pips on the second hit seems not right. */

        if (m_state == STA_WALK) { // Split big up into two small ones
            Set_Moving_State(STA_RUN);

            int big_pip_width = 0;
            if(m_big_start >= 0)
                big_pip_width = m_images[m_big_start].m_image->m_col_w; //Image width of a big pip

            // Spawn a second pip so it looks as if cut in two
            cPip* p_newpip = Copy();
            p_newpip->Set_Spawned(true); // Do not save into level file when editor is activated + saved!
            p_newpip->Set_Moving_State(STA_RUN);
            p_newpip->m_pos_x = m_pos_x + big_pip_width / 2; //Stay within bounds of original big pip
            p_newpip->m_pos_y = m_pos_y;

            //We just changed pips' positions.  Update the collision rectangles accordingly.
            Update_Position_Rect();
            p_newpip ->Update_Position_Rect();

            // Accelerate us up-left
            m_velx = -15.0f;
            m_vely = -15.0f;

            // Accelerate the new one up-right
            p_newpip->m_velx = 15.0f; // Opposite direction than above!
            p_newpip->m_vely = -15.0f;
            m_sprite_manager->Add(p_newpip);

            // animation
            cParticle_Emitter* p_anim = new cParticle_Emitter(m_sprite_manager);
            Generate_Hit_Animation(p_anim);
            p_anim->Set_Speed(3.5f, 0.6f);
            p_anim->Set_Fading_Alpha(true);
            p_anim->Emit();
            pActive_Animation_Manager->Add(p_anim);
        }
        else { // == STA_RUN
            Set_Scale_Directions(true, false, true, true);
            Set_Dead(true);
            cParticle_Emitter* p_anim = new cParticle_Emitter(m_sprite_manager);
            Generate_Hit_Animation(p_anim);
            p_anim->Set_Speed(4.5f, 1.6f);
            p_anim->Set_Scale(0.6f);
            p_anim->Emit();
            pActive_Animation_Manager->Add(p_anim);
        }
    }

    if (m_dead) {
        m_massive_type = MASS_PASSIVE;
        m_velx = 0.0f;
        m_vely = 0.0f;
    }
}

void cPip::Set_Moving_State(Moving_state new_state)
{
    if (new_state == m_state)
        return;

    if (new_state == STA_WALK) {
        Set_Image_Set("big");
        m_kill_points = 35;
    }
    else if (new_state == STA_RUN) {
        Set_Image_Set("small");
        m_kill_points = 70;
    }

    m_state = new_state;
    Update_Velocity_Max();
}

void cPip::Update()
{
    cEnemy::Update();

    if (!m_valid_update || !Is_In_Range())
        return;

    Update_Velocity();
    Update_Animation();
}

void cPip::Update_Velocity_Max()
{
    if (m_state == STA_WALK) {
        m_velx_max = 2.0f;
        m_velx_gain = 0.2f;
    }
    else if (m_state == STA_RUN) {
        m_velx_max = 7.0f;
        m_velx_gain = 0.5f;
    }
}

Col_Valid_Type cPip::Validate_Collision(cSprite* p_obj)
{
    // basic validation checking
    Col_Valid_Type basic_valid = Validate_Collision_Ghost(p_obj);

    // found valid collision
    if (basic_valid != COL_VTYPE_NOT_POSSIBLE)
        return basic_valid;

    if (p_obj->m_massive_type == MASS_MASSIVE) {
        switch (p_obj->m_type) {
        case TYPE_FLYON:
        case TYPE_ROKKO: // fallthrough
        case TYPE_GEE:   // fallthrough
            return COL_VTYPE_NOT_VALID;
        default:
            return COL_VTYPE_BLOCKING;
        }
    }
    else if (p_obj->m_massive_type == MASS_HALFMASSIVE) {
        // if moving downwards and the object is on bottom
        if (m_vely >= 0.0f && Is_On_Top(p_obj))
            return COL_VTYPE_BLOCKING;
    }
    else if (p_obj->m_massive_type == MASS_PASSIVE) {
        if (p_obj->m_type == TYPE_ENEMY_STOPPER)
            return COL_VTYPE_BLOCKING;
    }

    return COL_VTYPE_NOT_VALID;
}

void cPip::Handle_Collision_Player(cObjectCollision* p_collision)
{
    // invalid
    if (p_collision->m_direction == DIR_UNDEFINED)
        return;
    if (pLevel_Player->m_invincible) {
        DownGrade(true);
        return;
    }

    // Hit from the top. Downgrade if Alex is not small.
    if (p_collision->m_direction == DIR_TOP && pLevel_Player->m_state != STA_FLY) {
        if (pLevel_Player->m_alex_type == ALEX_SMALL) {
            pAudio->Play_Sound("wall_hit.wav");
            pLevel_Player->Action_Jump(true);
            return;
        }

        gp_hud->Add_Points(m_kill_points, m_pos_x, m_pos_y - 5.0f, "", static_cast<uint8_t>(255), true);
        pAudio->Play_Sound(m_kill_sound);

        // big walking
        if (m_state == STA_WALK) {
            DownGrade();
            pLevel_Player->Action_Jump(true);
	    
            //Move the player up a little bit so that they don't instantaneously stomp on a pip
            pLevel_Player->m_pos_y -= 10;
            pLevel_Player->Update_Position_Rect();

            // It is very hard to not get hit by the two resulting small pips. Thus,
            // grant the player a short period of invincibility.
            pLevel_Player->m_invincible = speedfactor_fps;
            pLevel_Player->m_invincible_mod = 0.0f;
        }
        else if (m_state == STA_RUN) { // small walking
            DownGrade();
            pLevel_Player->Add_Kill_Multiplier();
            // Whoooohooooo!  Shoot the player up high!
            pLevel_Player->m_vely = -60.0f;
            pAudio->Play_Sound("player/jump_big_power.ogg");
        }
    }
    else { // Otherwise downgrade Alex
        pLevel_Player->DownGrade_Player();
        Turn_Around(p_collision->m_direction);
    }
}

void cPip::Handle_Collision_Enemy(cObjectCollision* p_collision)
{
    if (p_collision->m_direction == DIR_RIGHT || p_collision->m_direction == DIR_LEFT)
        Turn_Around(p_collision->m_direction);

    Send_Collision(p_collision);
}

void cPip::Handle_Collision_Massive(cObjectCollision* p_collision)
{
    if (m_state == STA_OBJ_LINKED)
        return;

    Send_Collision(p_collision);

    // get colliding object
    cSprite* p_colobj = m_sprite_manager->Get_Pointer(p_collision->m_number);
    if (p_colobj->m_type == TYPE_BALL)
        return;
    else if (p_colobj->m_type == TYPE_CRATE && p_collision->m_direction == DIR_TOP) {
        // Ouch. Crate from above
        DownGrade(true);
        static_cast<cMovingSprite*>(p_colobj)->Reset_On_Ground();
    }

    if (p_collision->m_direction == DIR_TOP && m_vely < 0.0f)
        m_vely = 0.0f;
    else if (p_collision->m_direction == DIR_BOTTOM && m_vely > 0.0f)
        m_vely = 0.0f;
    else if (p_collision->m_direction == DIR_RIGHT || p_collision->m_direction == DIR_LEFT)
        Turn_Around(p_collision->m_direction);
}

void cPip::Editor_Activate()
{
    CEGUI::WindowManager& wm = CEGUI::WindowManager::getSingleton();

    // direction
    CEGUI::Combobox* p_combo = static_cast<CEGUI::Combobox*>(wm.createWindow("TSCLook256/Combobox", "editor_pip_direction"));

    p_combo->addItem(new CEGUI::ListboxTextItem("left"));
    p_combo->addItem(new CEGUI::ListboxTextItem("right"));
    p_combo->setText(Get_Direction_Name(m_start_direction));
    p_combo->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cPip::Editor_Direction_Select, this));
    pLevel_Editor->Add_Config_Widget(UTF8_("Direction"), UTF8_("Starting direction"), p_combo);

    // init
    Editor_Init();
}

bool cPip::Editor_Direction_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& args = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* p_item = static_cast<CEGUI::Combobox*>(args.window)->getSelectedItem();

    Set_Direction(Get_Direction_Id(p_item->getText().c_str()));
    return true;
}

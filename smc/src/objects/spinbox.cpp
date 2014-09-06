/***************************************************************************
 * spinbox.cpp  -  spinning box
 *
 * Copyright © 2003 - 2011 The SMC Contributors
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../objects/spinbox.hpp"
#include "../core/framerate.hpp"
#include "../core/game_core.hpp"
#include "../user/savegame.hpp"

namespace SMC {

/* *** *** *** *** *** *** *** *** cSpinBox *** *** *** *** *** *** *** *** *** */

cSpinBox :: cSpinBox(cSprite_Manager* sprite_manager)
    : cBaseBox(sprite_manager)
{
    cSpinBox::Init();
}

cSpinBox :: cSpinBox(XmlAttributes& attributes, cSprite_Manager* sprite_manager)
    : cBaseBox(sprite_manager)
{
    cSpinBox::Init();
    cSpinBox::Load_From_XML(attributes);
}

cSpinBox :: ~cSpinBox(void)
{

}

void cSpinBox :: Init(void)
{
    m_type = TYPE_SPIN_BOX;
    box_type = m_type;
    m_name = "Spinbox";
    m_camera_range = 5000;
    m_can_be_on_ground = 0;

    m_spin_counter = 0.0f;
    m_spin = 0;

    // default is infinite times activate-able
    Set_Useable_Count(-1, 1);
    // Spinbox Animation
    Set_Animation_Type("Spin");

    // editor image
    m_item_image = pVideo->Get_Package_Surface("game/arrow/small/white/up.png");
}

cSpinBox* cSpinBox :: Copy(void) const
{
    cSpinBox* spinbox = new cSpinBox(m_sprite_manager);
    spinbox->Set_Pos(m_start_pos_x, m_start_pos_y, 1);
    spinbox->Set_Invisible(m_box_invisible);
    spinbox->Set_Useable_Count(m_start_useable_count, 1);
    return spinbox;
}

void cSpinBox :: Load_From_XML(XmlAttributes& attributes)
{
    cBaseBox::Load_From_XML(attributes);
}

xmlpp::Element* cSpinBox :: Save_To_XML_Node(xmlpp::Element* p_element)
{
    return cBaseBox::Save_To_XML_Node(p_element);
}

void cSpinBox :: Load_From_Savegame(cSave_Level_Object* save_object)
{
    if(save_object->exists("spin_counter")) {
        // Let Activate do the work, then just set the counter
        Activate();
        m_spin_counter = string_to_float(save_object->Get_Value("spin_counter"));
    }
}

cSave_Level_Object* cSpinBox :: Save_To_Savegame(void)
{
    if(!m_spin)
        return NULL;

    cSave_Level_Object* save_object = new cSave_Level_Object();

    // default values
    save_object->m_type = m_type;
    save_object->m_properties.push_back(cSave_Level_Object_Property("posx", int_to_string(static_cast<int>(m_start_pos_x))));
    save_object->m_properties.push_back(cSave_Level_Object_Property("posy", int_to_string(static_cast<int>(m_start_pos_y))));

    // spin counter
    save_object->m_properties.push_back(cSave_Level_Object_Property("spin_counter", float_to_string(m_spin_counter)));

    return save_object;
}

void cSpinBox :: Activate(void)
{
    // already spinning
    if (m_spin) {
        return;
    }

    cBaseBox::Activate();

    m_spin = 1;
    Update_Valid_Update();
    // passive box for spinning
    m_massive_type = MASS_PASSIVE;

    m_spin_counter = 0.0f;
    // enable animation
    Set_Animation(1);
    Reset_Animation();
}

void cSpinBox :: Stop(void)
{
    // already stopped spinning
    if (!m_spin) {
        return;
    }

    // disabled image
    if (!m_useable_count) {
        Set_Image_Num(0);
    }
    // default image
    else {
        Set_Image_Num(1);
    }
    // reset
    m_spin = 0;
    Update_Valid_Update();
    m_spin_counter = 0.0f;

    // back to a massive box
    m_massive_type = MASS_MASSIVE;
    // disable animation
    Set_Animation(0);
    Reset_Animation();
}

void cSpinBox :: Update(void)
{
    if (!m_valid_update || !Is_In_Range()) {
        return;
    }

    cBaseBox::Update();

    if (m_spin) {
        m_spin_counter += pFramerate->m_speed_factor;

        // spinning animation finished
        if (m_curr_img == 1) {
            // spinning time finished
            if (m_spin_counter > speedfactor_fps * 5) {
                // reset spin counter
                m_spin_counter = 0.0f;
                // set to massive for collision check
                m_massive_type = MASS_MASSIVE;
                // collision data
                cObjectCollisionType* col_list = Collision_Check(&m_col_rect, COLLIDE_ONLY_BLOCKING);

                // check if spinning should continue
                bool spin_again = 0;

                // colliding with player or enemy
                if (col_list->Is_Included(TYPE_PLAYER) || col_list->Is_Included(ARRAY_ENEMY)) {
                    spin_again = 1;
                }
                // colliding with an active object
                else if (col_list->Is_Included(ARRAY_ACTIVE)) {
                    cSprite* col_obj = col_list->Find_First(ARRAY_ACTIVE)->m_obj;

                    // check for items
                    if (col_obj->m_type == TYPE_MUSHROOM_LIVE_1 || col_obj->m_type == TYPE_MUSHROOM_DEFAULT ||
                            col_obj->m_type == TYPE_MUSHROOM_POISON || col_obj->m_type == TYPE_MUSHROOM_BLUE || col_obj->m_type == TYPE_MUSHROOM_GHOST ||
                            col_obj->m_type == TYPE_FIREPLANT || col_obj->m_type == TYPE_STAR || col_obj->m_type == TYPE_FALLING_GOLDPIECE) {
                        // found blocking active object
                        spin_again = 1;
                    }
                }

                delete col_list;

                // continue spinning
                if (spin_again) {
                    // spin some time again
                    m_spin_counter = speedfactor_fps * 2;
                    // passive for spinning
                    m_massive_type = MASS_PASSIVE;
                }
                // finished spinning
                else {
                    Stop();
                }
            }
        }
    }
}

bool cSpinBox :: Is_Update_Valid(void)
{
    if (m_spin) {
        return 1;
    }

    return cBaseBox::Is_Update_Valid();
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace SMC

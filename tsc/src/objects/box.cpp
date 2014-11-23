/***************************************************************************
 * box.cpp  -  class for the basic box handler
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

#include "../objects/box.hpp"
#include "../audio/audio.hpp"
#include "../core/camera.hpp"
#include "../core/framerate.hpp"
#include "../level/level.hpp"
#include "../core/game_core.hpp"
#include "../level/level_player.hpp"
#include "../video/gl_surface.hpp"
#include "../user/savegame.hpp"
#include "../core/sprite_manager.hpp"
#include "../core/math/utilities.hpp"
#include "../core/i18n.hpp"
#include "../enemies/turtle.hpp"
#include "../enemies/bosses/turtle_boss.hpp"
#include "../gui/hud.hpp"
#include "../scripting/events/activate_event.hpp"
#include "../level/level.hpp"

namespace TSC {

/* *** *** *** *** *** *** *** *** cBaseBox *** *** *** *** *** *** *** *** *** */

cBaseBox::cBaseBox(cSprite_Manager* sprite_manager)
    : cAnimated_Sprite(sprite_manager, "box")
{
    m_type = TYPE_ACTIVE_SPRITE;
    m_sprite_array = ARRAY_ACTIVE;
    m_massive_type = MASS_MASSIVE;
    m_name = "Box";
    m_can_be_ground = 1;
    Set_Scale_Directions(1, 1, 1, 1);

    box_type = TYPE_UNDEFINED;
    m_item_image = NULL;
    m_pos_z = 0.055f;

    m_move_col_dir = DIR_UNDEFINED;
    m_move_counter = 0.0f;
    m_move_back = 0;
    // default = usable once
    m_useable_count = 1;
    m_start_useable_count = 1;

    m_box_invisible = BOX_VISIBLE;

    m_particle_counter_active = 0.0f;
}

cBaseBox::~cBaseBox(void)
{
    //
}

void cBaseBox::Load_From_XML(XmlAttributes& attributes)
{
    // Position
    Set_Pos(string_to_float(attributes["posx"]), string_to_float(attributes["posy"]), true);

    // animation (`box_type' is set by subclasses’ constructors’ as default values)
    // FIXME: This should be in the respective subclasses somehow
    if (box_type != TYPE_SPIN_BOX && box_type != TYPE_TEXT_BOX)
        Set_Animation_Type(attributes.fetch("animation", m_anim_type));

    // Invisible
    Set_Invisible(static_cast<Box_Invisible_Type>(string_to_float(attributes["invisible"])));

    // Usable count
    Set_Useable_Count(attributes.fetch<int>("useable_count", m_start_useable_count), true);
}

std::string cBaseBox::Get_XML_Type_Name()
{
    // return is just as good as break here
    switch (box_type) {
    case TYPE_SPIN_BOX:
        return "spin";
    case TYPE_TEXT_BOX:
        return "text";
    default:
        return "bonus";
    }
}

xmlpp::Element* cBaseBox::Save_To_XML_Node(xmlpp::Element* p_element)
{
    xmlpp::Element* p_node = cAnimated_Sprite::Save_To_XML_Node(p_element);

    if (box_type != TYPE_SPIN_BOX && box_type != TYPE_TEXT_BOX) {
        // animation type
        Add_Property(p_node, "animation", m_anim_type);
        // best possible item
        Add_Property(p_node, "item", box_type);
    }

    // invisible
    Add_Property(p_node, "invisible", m_box_invisible);
    // useable count
    Add_Property(p_node, "useable_count", m_start_useable_count);

    return p_node;
}

void cBaseBox::Load_From_Savegame(cSave_Level_Object* save_object)
{
    // useable count
    int save_useable_count = string_to_int(save_object->Get_Value("useable_count"));
    Set_Useable_Count(save_useable_count);
}

cSave_Level_Object* cBaseBox::Save_To_Savegame(void)
{
    // only save if needed
    if (m_useable_count == m_start_useable_count) {
        return NULL;
    }

    cSave_Level_Object* save_object = new cSave_Level_Object();

    // default values
    save_object->m_type = m_type;
    save_object->m_properties.push_back(cSave_Level_Object_Property("posx", int_to_string(static_cast<int>(m_start_pos_x))));
    save_object->m_properties.push_back(cSave_Level_Object_Property("posy", int_to_string(static_cast<int>(m_start_pos_y))));

    // Useable Count
    save_object->m_properties.push_back(cSave_Level_Object_Property("useable_count", int_to_string(m_useable_count)));

    return save_object;
}

void cBaseBox::Set_Animation_Type(const std::string& new_anim_type)
{
    // already set
    if (m_anim_type.compare(new_anim_type) == 0) {
        return;
    }

    Clear_Images();
    m_anim_type = new_anim_type;

    if (m_anim_type.compare("Bonus") == 0) {
        // disabled image
        Add_Image(pVideo->Get_Surface("game/box/brown1_1.png"));
        // animation images
        Add_Image(pVideo->Get_Surface("game/box/yellow/bonus/1.png"));
        Add_Image(pVideo->Get_Surface("game/box/yellow/bonus/2.png"));
        Add_Image(pVideo->Get_Surface("game/box/yellow/bonus/3.png"));
        Add_Image(pVideo->Get_Surface("game/box/yellow/bonus/4.png"));
        Add_Image(pVideo->Get_Surface("game/box/yellow/bonus/5.png"));
        Add_Image(pVideo->Get_Surface("game/box/yellow/bonus/6.png"));

        Set_Animation(1);
        Set_Animation_Image_Range(1, 6);
        Set_Time_All(90, 1);
    }
    else if (m_anim_type.compare("Default") == 0) {
        // disabled image
        Add_Image(pVideo->Get_Surface("game/box/brown1_1.png"));
        // default
        Add_Image(pVideo->Get_Surface("game/box/yellow/default.png"));

        Set_Animation(0);
        Set_Animation_Image_Range(1, 1);
    }
    else if (m_anim_type.compare("Power") == 0) {
        // disabled image
        Add_Image(pVideo->Get_Surface("game/box/brown1_1.png"));
        // animation images
        Add_Image(pVideo->Get_Surface("game/box/yellow/power_1.png"));
        Add_Image(pVideo->Get_Surface("game/box/yellow/power_2.png"));
        Add_Image(pVideo->Get_Surface("game/box/yellow/power_3.png"));
        Add_Image(pVideo->Get_Surface("game/box/yellow/power_4.png"));

        Set_Animation(1);
        Set_Animation_Image_Range(1, 4);
        Set_Time_All(80, 1);
    }
    else if (m_anim_type.compare("Spin") == 0) {
        // disabled image
        Add_Image(pVideo->Get_Surface("game/box/yellow/spin/disabled.png"));
        // animation images
        Add_Image(pVideo->Get_Surface("game/box/yellow/default.png"));
        Add_Image(pVideo->Get_Surface("game/box/yellow/spin/1.png"));
        Add_Image(pVideo->Get_Surface("game/box/yellow/spin/2.png"));
        Add_Image(pVideo->Get_Surface("game/box/yellow/spin/3.png"));
        Add_Image(pVideo->Get_Surface("game/box/yellow/spin/4.png"));
        Add_Image(pVideo->Get_Surface("game/box/yellow/spin/5.png"));

        Set_Animation(0);
        Set_Animation_Image_Range(1, 6);
        Set_Time_All(80, 1);
    }
    else {
        printf("Warning : Unknown Box Animation Type %s\n", m_anim_type.c_str());
        Set_Animation_Type("Bonus");
    }

    Reset_Animation();
    // set start image
    Set_Image_Num(m_anim_img_start, 1, 0);
}

void cBaseBox::Set_Useable_Count(int count, bool new_startcount /* = 0 */)
{
    m_useable_count = count;

    if (new_startcount) {
        m_start_useable_count = m_useable_count;

        // unlimited
        if (m_start_useable_count < -1) {
            m_start_useable_count = -1;
        }
    }
}

void cBaseBox::Set_Invisible(Box_Invisible_Type type)
{
    // already set
    if (m_box_invisible == type) {
        return;
    }

    // remember old type
    Box_Invisible_Type type_old = m_box_invisible;
    // set new type
    m_box_invisible = type;

    // was invisible
    if (type_old == BOX_INVISIBLE_MASSIVE || type_old == BOX_INVISIBLE_SEMI_MASSIVE) {
        // was semi massive
        if (type_old == BOX_INVISIBLE_SEMI_MASSIVE) {
            m_massive_type = MASS_MASSIVE;
        }
    }
    // was ghost
    else if (type_old == BOX_GHOST) {
        Set_Color(255, 255, 255, 255);
        Set_Color_Combine(0, 0, 0, 0);
    }

    // got invisible
    if (type == BOX_INVISIBLE_MASSIVE || type == BOX_INVISIBLE_SEMI_MASSIVE) {
        // got semi massive
        if (type == BOX_INVISIBLE_SEMI_MASSIVE) {
            m_massive_type = MASS_PASSIVE;
        }
    }
    // got ghost
    else if (type == BOX_GHOST) {
        Set_Color(192, 192, 255, 128);
        Set_Color_Combine(0.2f, 0.2f, 0.55f, GL_ADD);
    }
}

void cBaseBox::Activate_Collision(ObjectDirection col_direction)
{
    // if already active ignore event
    if (m_move_col_dir != DIR_UNDEFINED) {
        return;
    }

    // not useable
    if (m_useable_count == 0) {
        return;
    }

    // if invisible go visible
    if (m_box_invisible) {
        // get massive
        if (m_box_invisible == BOX_INVISIBLE_SEMI_MASSIVE) {
            m_massive_type = MASS_MASSIVE;
        }
    }

    // set scaling based on direction
    if (col_direction == DIR_UP) {
        Set_Scale_Directions(1, 0, 1, 1);
    }
    else if (col_direction == DIR_DOWN) {
        Set_Scale_Directions(0, 1, 1, 1);
    }
    else if (col_direction == DIR_LEFT) {
        Set_Scale_Directions(1, 0, 1, 0);
    }
    else if (col_direction == DIR_RIGHT) {
        Set_Scale_Directions(1, 0, 0, 1);
    }

    // set collision direction
    m_move_col_dir = col_direction;
    Update_Valid_Update();

    Check_Collision(Get_Opposite_Direction(m_move_col_dir));
    Activate();
}

void cBaseBox::Update_Collision(void)
{
    // not moving
    if (m_move_col_dir == DIR_UNDEFINED) {
        return;
    }

    // invalid direction
    if (m_move_col_dir != DIR_UP && m_move_col_dir != DIR_DOWN && m_move_col_dir != DIR_RIGHT && m_move_col_dir != DIR_LEFT) {
        printf("Warning : wrong box collision direction %d\n", m_move_col_dir);
        m_move_col_dir = DIR_UNDEFINED;
        Update_Valid_Update();
        return;
    }

    // speed mod
    float mod = pFramerate->m_speed_factor * 0.05f;
    // counter
    m_move_counter += pFramerate->m_speed_factor * 0.2f;

    // move into the given direction
    if (!m_move_back) {
        // scale
        Add_Scale(mod);

        // Particles
        Generate_Activation_Particles();

        // check if reached final position
        if (m_move_counter > 1.0f) {
            m_move_back = 1;
            m_move_counter = 0.0f;
        }
    }
    // move back to the original position
    else {
        // scale
        Add_Scale(-mod);

        // check if reached original position
        if (m_move_counter > 1.0f) {
            m_move_col_dir = DIR_UNDEFINED;
            Update_Valid_Update();
            m_move_back = 0;
            m_move_counter = 0.0f;

            // reset rect
            m_col_pos = m_image->m_col_pos;
            // reset scale
            Set_Scale(1);
            Set_Scale_Directions(1, 1, 1, 1);
            // reset position
            Set_Pos(m_start_pos_x, m_start_pos_y);
        }
    }
}

void cBaseBox::Check_Collision(ObjectDirection col_direction)
{
    // additional direction based check position
    float check_x = 0.0f;
    float check_y = 0.0f;

    // set the collision size based on the collision direction
    if (col_direction == DIR_BOTTOM) {
        check_y -= 10.0f;
    }
    else if (col_direction == DIR_TOP) {
        check_y += 10.0f;
    }
    else if (col_direction == DIR_LEFT) {
        check_x += 5.0f;
    }
    else if (col_direction == DIR_RIGHT) {
        check_x -= 5.0f;
    }

    // collision count
    cObjectCollisionType* col_list = Collision_Check_Relative(check_x, check_y, m_col_rect.m_w - (check_x * 0.5f), m_col_rect.m_h - (check_y * 0.5f));

    // handle collisions
    for (cObjectCollision_List::iterator itr = col_list->objects.begin(); itr != col_list->objects.end(); ++itr) {
        cObjectCollision* col_obj = (*itr);

        // send box collision
        col_obj->m_obj->Handle_Collision_Box(Get_Opposite_Direction(col_obj->m_direction), &m_col_rect);
    }

    delete col_list;
}

void cBaseBox::Activate(void)
{
    if (m_useable_count > 0 || m_useable_count == -1) {
        Scripting::cActivate_Event evt;
        evt.Fire(pActive_Level->m_mruby, this);
    }

    // set useable count
    if (m_useable_count > 0) {
        Set_Useable_Count(m_useable_count - 1);
    }
}

void cBaseBox::Update(void)
{
    // animate only a visible box or an activated invisible box
    if (m_box_invisible == BOX_VISIBLE || (m_box_invisible == BOX_GHOST && pLevel_Player->m_maryo_type == MARYO_GHOST) || m_useable_count != m_start_useable_count) {
        // Spinbox animation handling
        if ((m_type == TYPE_SPIN_BOX || m_useable_count != 0) && m_anim_enabled) {
            // Set_Image in Update_Animation overwrites col_pos
            GL_point col_pos_temp = m_col_pos;

            Update_Animation();

            // save col_pos
            m_col_pos = col_pos_temp;
        }
    }

    Update_Collision();
}

void cBaseBox::Draw(cSurface_Request* request /* = NULL */)
{
    if (!m_valid_draw) {
        return;
    }

    // editor disabled
    if (!editor_level_enabled) {
        // visible box or activated invisible box
        if (m_box_invisible == BOX_VISIBLE || (m_box_invisible == BOX_GHOST && pLevel_Player->m_maryo_type == MARYO_GHOST) || m_useable_count != m_start_useable_count) {
            cAnimated_Sprite::Draw(request);
        }
    }
    // editor enabled
    else {
        // draw invisible box only in editor mode
        if (m_box_invisible) {
            Color color;

            // default invisible
            if (m_box_invisible == BOX_INVISIBLE_MASSIVE) {
                color = Color(static_cast<Uint8>(240), 0, 30, 128);
            }
            // ghost
            else if (m_box_invisible == BOX_GHOST) {
                color = Color(static_cast<Uint8>(20), 20, 150, 128);
            }
            // invisible semi massive
            else if (m_box_invisible == BOX_INVISIBLE_SEMI_MASSIVE) {
                color = Color(static_cast<Uint8>(180), 0, 10, 128);
            }

            pVideo->Draw_Rect(m_start_pos_x - pActive_Camera->m_x, m_start_pos_y - pActive_Camera->m_y, m_rect.m_w, m_rect.m_h, m_pos_z, &color);
        }
        // visible box
        else {
            cAnimated_Sprite::Draw(request);
        }

        // draw item image
        if (m_item_image) {
            // auto position
            m_item_image->Blit(m_start_pos_x - ((m_item_image->m_w - m_rect.m_w) / 2) - pActive_Camera->m_x, m_start_pos_y - ((m_item_image->m_h - m_rect.m_h) / 2) - pActive_Camera->m_y, m_pos_z + 0.000003f);
        }
    }
}

void cBaseBox::Generate_Activation_Particles(void)
{
    // no default/unimportant boxes
    if (m_type == TYPE_SPIN_BOX || m_type == TYPE_TEXT_BOX || box_type == TYPE_GOLDPIECE) {
        return;
    }

    m_particle_counter_active += pFramerate->m_speed_factor;

    if (m_particle_counter_active > 0.0f) {
        cParticle_Emitter* anim = new cParticle_Emitter(m_sprite_manager);
        if (box_type == TYPE_MUSHROOM_POISON) {
            anim->Set_Image(pVideo->Get_Surface("animation/particles/green_ghost_light.png"));
            anim->Set_Speed(1.0f, 0.1f);
            anim->Set_Time_to_Live(1.4f);
        }
        else {
            anim->Set_Image(pVideo->Get_Surface("animation/particles/light.png"));
            anim->Set_Speed(7.0f, 0.5f);
            anim->Set_Time_to_Live(0.2f);
        }
        anim->Set_Quota(static_cast<int>(m_particle_counter_active));
        anim->Set_Emitter_Rect(m_pos_x + 4.0f, m_pos_y + 8.0f, m_rect.m_w - 8.0f, 1.0f);
        if (box_type == TYPE_UNDEFINED) {
            anim->Set_Emitter_Time_to_Live(0.2f);
            anim->Set_Scale(0.2f, 0.05f);
        }
        else {
            anim->Set_Emitter_Time_to_Live(0.5f);
            anim->Set_Scale(0.3f, 0.1f);
        }
        anim->Set_Pos_Z(m_pos_z - 0.000001f);
        anim->Set_Direction_Range(260, 20);

        Color color;
        Color color_rand;

        if (box_type == TYPE_UNDEFINED) {
            color = Color(static_cast<Uint8>(128), 128, 128, 128);
            color_rand = Color(static_cast<Uint8>(0), 0, 0, 128);
        }
        else if (box_type == TYPE_MUSHROOM_DEFAULT) {
            color = Color(static_cast<Uint8>(180), 140, 120);
            color_rand = Color(static_cast<Uint8>(70), 30, 30, 0);
        }
        else if (box_type == TYPE_FIREPLANT) {
            color = Color(static_cast<Uint8>(220), 150, 70);
            color_rand = Color(static_cast<Uint8>(30), 10, 10, 0);
        }
        else if (box_type == TYPE_MUSHROOM_BLUE) {
            color = Color(static_cast<Uint8>(180), 180, 240);
            color_rand = Color(static_cast<Uint8>(40), 40, 10, 0);
        }
        else if (box_type == TYPE_MUSHROOM_GHOST) {
            color = Color(static_cast<Uint8>(100), 100, 100);
            color_rand = Color(static_cast<Uint8>(10), 10, 10, 0);
        }
        else if (box_type == TYPE_MUSHROOM_LIVE_1) {
            color = Color(static_cast<Uint8>(120), 200, 120);
            color_rand = Color(static_cast<Uint8>(20), 50, 20, 0);
        }
        else if (box_type == TYPE_STAR) {
            color = Color(static_cast<Uint8>(180), 150, 100);
            color_rand = Color(static_cast<Uint8>(70), 70, 0, 0);
        }
        else if (box_type == TYPE_MUSHROOM_POISON) {
            color = Color(static_cast<Uint8>(80), 100, 10);
            color_rand = Color(static_cast<Uint8>(20), 90, 20, 0);
        }
        else {
            color = white;
            color_rand = Color(static_cast<Uint8>(0), 0, 0, 0);
        }
        anim->Set_Color(color, color_rand);

        anim->Set_Blending(BLEND_ADD);
        anim->Emit();
        pActive_Animation_Manager->Add(anim);

        m_particle_counter_active -= static_cast<int>(m_particle_counter_active);
    }
}

bool cBaseBox::Is_Update_Valid()
{
    // if not activateable and not animating
    if (!m_useable_count && m_move_col_dir == DIR_UNDEFINED) {
        return 0;
    }

    // if not active
    if (!m_active) {
        return 0;
    }

    return 1;
}

bool cBaseBox::Is_Draw_Valid(void)
{
    // if editor not enabled
    if (!editor_enabled) {
        // if not active or no image is set
        if (!m_active || !m_image) {
            return 0;
        }

        // ghost
        if (m_box_invisible == BOX_GHOST) {
            // maryo is not ghost
            if (pLevel_Player->m_maryo_type != MARYO_GHOST) {
                return 0;
            }
        }
    }
    // editor enabled
    else {
        // if destroyed
        if (m_auto_destroy) {
            return 0;
        }

        // no image
        if (!m_start_image && !m_box_invisible) {
            return 0;
        }
    }

    // not visible on the screen
    if (!Is_Visible_On_Screen()) {
        return 0;
    }

    return 1;
}

Col_Valid_Type cBaseBox::Validate_Collision(cSprite* obj)
{
    switch (obj->m_type) {
    case TYPE_MUSHROOM_LIVE_1:
    case TYPE_MUSHROOM_DEFAULT:
    case TYPE_MUSHROOM_POISON:
    case TYPE_MUSHROOM_BLUE:
    case TYPE_MUSHROOM_GHOST:
    case TYPE_FALLING_GOLDPIECE: {
        return COL_VTYPE_BLOCKING;
    }
    default: {
        break;
    }
    }

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

void cBaseBox::Handle_Collision_Player(cObjectCollision* collision)
{
    // if player jumps from below or flies against it
    if (collision->m_direction == DIR_BOTTOM && pLevel_Player->m_state != STA_FLY) {
        if (m_useable_count != 0) {
            Activate_Collision(Get_Opposite_Direction(collision->m_direction));
        }
        else {
            if (Is_Visible_On_Screen()) {
                pAudio->Play_Sound("wall_hit.wav", RID_MARYO_WALL_HIT);
            }
        }
    }
}

void cBaseBox::Handle_Collision_Enemy(cObjectCollision* collision)
{
    cEnemy* enemy = static_cast<cEnemy*>(m_sprite_manager->Get_Pointer(collision->m_number));

    // if turtle
    if (enemy->m_type == TYPE_TURTLE || enemy->m_type == TYPE_SHELL) {
        cTurtle* turtle = static_cast<cTurtle*>(enemy);  // Shell is a turtle anyway

        // if not shell
        if (turtle->m_turtle_state != TURTLE_SHELL_RUN && turtle->m_turtle_state != TURTLE_SHELL_STAND) {
            return;
        }
    }
    // if turtle boss
    else if (enemy->m_type == TYPE_TURTLE_BOSS) {
        cTurtleBoss* turtleboss = static_cast<cTurtleBoss*>(enemy);

        // if not shell
        if (turtleboss->m_turtle_state != TURTLEBOSS_SHELL_RUN && turtleboss->m_turtle_state != TURTLEBOSS_SHELL_STAND) {
            return;
        }
    }
    // not a valid enemy
    else {
        return;
    }

    if (collision->m_direction == DIR_RIGHT || collision->m_direction == DIR_LEFT || collision->m_direction == DIR_BOTTOM) {
        if (m_useable_count != 0 && m_type != TYPE_TEXT_BOX) {
            Activate_Collision(Get_Opposite_Direction(collision->m_direction));
        }
        else {
            if (Is_Visible_On_Screen()) {
                pAudio->Play_Sound("wall_hit.wav");
            }
        }
    }
}


void cBaseBox::Editor_Activate(void)
{
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    if (m_type != TYPE_TEXT_BOX) {
        // useable count
        CEGUI::Editbox* editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "editor_basebox_useable_count"));
        Editor_Add(UTF8_("Useable Count"), UTF8_("Useable Count"), editbox, 80);

        editbox->setText(int_to_string(m_start_useable_count));
        editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cBaseBox::Editor_Useable_Count_Text_Changed, this));
    }

    // Invisible
    CEGUI::Combobox* combobox = static_cast<CEGUI::Combobox*>(wmgr.createWindow("TaharezLook/Combobox", "editor_basebox_invisible"));
    Editor_Add(UTF8_("Invisible"), UTF8_("Massive is invisible until activated.\nGhost is only visible and activateable if in ghost mode.\nSemi Massive is like Massive but only touchable in the activation direction."), combobox, 120, 100);

    combobox->addItem(new CEGUI::ListboxTextItem(UTF8_("Disabled")));
    combobox->addItem(new CEGUI::ListboxTextItem(UTF8_("Massive")));
    combobox->addItem(new CEGUI::ListboxTextItem(UTF8_("Ghost")));
    combobox->addItem(new CEGUI::ListboxTextItem(UTF8_("Semi Massive")));

    if (m_box_invisible == BOX_INVISIBLE_MASSIVE) {
        combobox->setText(UTF8_("Massive"));
    }
    else if (m_box_invisible == BOX_GHOST) {
        combobox->setText(UTF8_("Ghost"));
    }
    else if (m_box_invisible == BOX_INVISIBLE_SEMI_MASSIVE) {
        combobox->setText(UTF8_("Semi Massive"));
    }
    else {
        combobox->setText(UTF8_("Disabled"));
    }

    combobox->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cBaseBox::Editor_Invisible_Select, this));

    if (m_type == TYPE_SPIN_BOX) {
        // init
        Editor_Init();
    }
}

bool cBaseBox::Editor_Useable_Count_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Useable_Count(string_to_int(str_text), 1);

    return 1;
}

bool cBaseBox::Editor_Invisible_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    if (item->getText().compare(UTF8_("Massive")) == 0) {
        Set_Invisible(BOX_INVISIBLE_MASSIVE);
    }
    else if (item->getText().compare(UTF8_("Ghost")) == 0) {
        Set_Invisible(BOX_GHOST);
    }
    else if (item->getText().compare(UTF8_("Semi Massive")) == 0) {
        Set_Invisible(BOX_INVISIBLE_SEMI_MASSIVE);
    }
    else {
        Set_Invisible(BOX_VISIBLE);
    }

    return 1;
}

std::string cBaseBox::Create_Name(void) const
{
    std::string name = m_name; // dup

    if (box_type == TYPE_UNDEFINED) {
        name += " Empty";
    }
    else if (box_type == TYPE_POWERUP) {
        name += " Random";
    }
    else if (box_type == TYPE_SPIN_BOX) {
        name += " Spinning";
    }
    else if (box_type == TYPE_TEXT_BOX) {
        name += " Text";
    }
    else if (box_type == TYPE_MUSHROOM_DEFAULT) {
        name += " Mushroom";
    }
    else if (box_type == TYPE_FIREPLANT) {
        name += " Mushroom - Fireplant";
    }
    else if (box_type == TYPE_MUSHROOM_BLUE) {
        name += " Mushroom - Blue Mushroom";
    }
    else if (box_type == TYPE_MUSHROOM_GHOST) {
        name = " Mushroom - Ghost Mushroom";
    }
    else if (box_type == TYPE_MUSHROOM_LIVE_1) {
        name += " 1-UP";
    }
    else if (box_type == TYPE_STAR) {
        name += " Star";
    }
    else if (box_type == TYPE_GOLDPIECE) {
        name += " Goldpiece";
    }
    else if (box_type == TYPE_MUSHROOM_POISON) {
        name += " Mushroom Poison";
    }
    else { // Shouldn't happen
        name += " Unknown Item Type";
    }

    if (m_box_invisible == BOX_INVISIBLE_MASSIVE) {
        name.insert(0, _("Invisible "));
    }
    else if (m_box_invisible == BOX_GHOST) {
        name.insert(0, _("Ghost "));
    }
    else if (m_box_invisible == BOX_INVISIBLE_SEMI_MASSIVE) {
        name.insert(0, _("Invisible Semi Massive "));
    }

    return name;
}

void cBaseBox::Set_Massive_Type(MassiveType type)
{
    // Ignore to prevent "m" toggling in level editor
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

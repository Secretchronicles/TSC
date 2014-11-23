/***************************************************************************
 * eato.cpp  -  eating static plant :P
 *
 * Copyright © 2006 - 2011 Florian Richter
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

#include "../enemies/eato.hpp"
#include "../core/game_core.hpp"
#include "../video/animation.hpp"
#include "../level/level_player.hpp"
#include "../video/gl_surface.hpp"
#include "../core/i18n.hpp"
#include "../core/filesystem/filesystem.hpp"
#include "../core/filesystem/resource_manager.hpp"
#include "../core/xml_attributes.hpp"

namespace fs = boost::filesystem;

namespace TSC {

/* *** *** *** *** *** *** cEato *** *** *** *** *** *** *** *** *** *** *** */

cEato::cEato(cSprite_Manager* sprite_manager)
    : cEnemy(sprite_manager)
{
    cEato::Init();
}

cEato::cEato(XmlAttributes& attributes, cSprite_Manager* sprite_manager)
    : cEnemy(sprite_manager)
{
    cEato::Init();

    // position
    Set_Pos(string_to_float(attributes["posx"]), string_to_float(attributes["posy"]), true);

    // image directory
    Set_Image_Dir(utf8_to_path(attributes.fetch("image_dir", path_to_utf8(m_img_dir))));

    // direction
    Set_Direction(Get_Direction_Id(attributes.fetch("direction", Get_Direction_Name(m_start_direction))));
}

cEato::~cEato(void)
{
    //
}

void cEato::Init(void)
{
    m_type = TYPE_EATO;
    m_name = "Eato";
    m_camera_range = 1000;
    m_pos_z = 0.087f;
    m_can_be_on_ground = 0;
    Set_Rotation_Affects_Rect(1);
    m_fire_resistant = 1;

    m_state = STA_STAY;
    Set_Image_Dir(utf8_to_path("enemy/eato/brown/"));
    Set_Direction(DIR_UP_LEFT);

    m_kill_sound = "enemy/eato/die.ogg";
    m_kill_points = 150;
}

cEato* cEato::Copy(void) const
{
    cEato* eato = new cEato(m_sprite_manager);
    eato->Set_Pos(m_start_pos_x, m_start_pos_y);
    eato->Set_Image_Dir(m_img_dir);
    eato->Set_Direction(m_start_direction);
    return eato;
}

std::string cEato::Get_XML_Type_Name()
{
    return "eato";
}

xmlpp::Element* cEato::Save_To_XML_Node(xmlpp::Element* p_element)
{
    xmlpp::Element* p_node = cEnemy::Save_To_XML_Node(p_element);

    Add_Property(p_node, "image_dir", path_to_utf8(m_img_dir));
    Add_Property(p_node, "direction", Get_Direction_Name(m_start_direction));

    return p_node;
}

void cEato::Set_Image_Dir(fs::path dir)
{
    if (dir.empty()) {
        return;
    }

    // if not image directory
    if (!File_Exists(pResource_Manager->Get_Game_Pixmaps_Directory() / dir / utf8_to_path("1.settings")) && !File_Exists(pResource_Manager->Get_Game_Pixmaps_Directory() / dir / utf8_to_path("1.png"))) {
        std::cerr   << "Warning: Eato image files not found; does the eato directory "
                    << path_to_utf8(dir) << " exist?" << std::endl;
        return;
    }

    m_img_dir = dir;

    // clear images
    Clear_Images();
    // set images
    Add_Image(pVideo->Get_Surface(m_img_dir / utf8_to_path("1.png")));
    Add_Image(pVideo->Get_Surface(m_img_dir / utf8_to_path("2.png")));
    Add_Image(pVideo->Get_Surface(m_img_dir / utf8_to_path("3.png")));
    Add_Image(pVideo->Get_Surface(m_img_dir / utf8_to_path("2.png")));
    // set start image
    Set_Image_Num(0, 1);

    Set_Animation(1);
    Set_Animation_Image_Range(0, 3);
    Set_Time_All(180, 1);
    Reset_Animation();
}

void cEato::Set_Direction(const ObjectDirection dir)
{
    // already set
    if (m_start_direction == dir) {
        return;
    }

    cEnemy::Set_Direction(dir, 1);

    // clear
    Set_Rotation(0.0f, 0.0f, 0.0f, 1);

    if (m_start_direction == DIR_UP_LEFT) {
        Set_Rotation_Y(180.0f, 1);
    }
    else if (m_start_direction == DIR_UP_RIGHT) {
        // default
    }
    else if (m_start_direction == DIR_LEFT_UP) {
        Set_Rotation_Z(90.0f, 1);
        Set_Rotation_X(180.0f, 1);
    }
    else if (m_start_direction == DIR_LEFT_DOWN) {
        Set_Rotation_Z(90.0f, 1);
    }
    else if (m_start_direction == DIR_RIGHT_UP) {
        Set_Rotation_Z(270.0f, 1);
    }
    else if (m_start_direction == DIR_RIGHT_DOWN) {
        Set_Rotation_Z(270.0f, 1);
        Set_Rotation_X(180.0f, 1);
    }
    else if (m_start_direction == DIR_DOWN_LEFT) {
        Set_Rotation_X(180.0f, 1);
    }
    else if (m_start_direction == DIR_DOWN_RIGHT) {
        Set_Rotation_Z(180.0f, 1);
    }
}

void cEato::DownGrade(bool force /* = 0 */)
{
    Set_Dead(1);
    m_massive_type = MASS_PASSIVE;
    m_counter = 0.0f;

    if (!force) {
        // animation
        cParticle_Emitter* anim = new cParticle_Emitter(m_sprite_manager);
        Generate_Hit_Animation(anim);

        anim->Set_Scale(0.8f);
        anim->Set_Direction_Range(0.0f, 360.0f);
        anim->Emit();
        pActive_Animation_Manager->Add(anim);
    }
    else {
        Set_Rotation_Z(180.0f);
    }
}

void cEato::Update_Normal_Dying()
{
    // Immediately disappears
    Set_Active(false);
}

void cEato::Update(void)
{
    cEnemy::Update();

    if (!m_valid_update || !Is_In_Range()) {
        return;
    }

    Update_Animation();
}

Col_Valid_Type cEato::Validate_Collision(cSprite* obj)
{
    if (obj->m_massive_type == MASS_MASSIVE) {
        switch (obj->m_type) {
        case TYPE_PLAYER: {
            return COL_VTYPE_BLOCKING;
        }
        case TYPE_BALL: {
            return COL_VTYPE_BLOCKING;
        }
        default: {
            break;
        }
        }

        return COL_VTYPE_NOT_VALID;
    }

    return COL_VTYPE_NOT_VALID;
}

void cEato::Handle_Collision_Player(cObjectCollision* collision)
{
    // unknown direction
    if (collision->m_direction == DIR_UNDEFINED) {
        return;
    }

    // only if not invincible
    if (pLevel_Player->m_invincible <= 0.0f) {
        // if player is big and not a bottom collision
        if (pLevel_Player->m_maryo_type != MARYO_SMALL && (collision->m_direction != DIR_BOTTOM)) {
            // todo : create again
            //pAudio->PlaySound( "player/maryo_au.ogg", RID_MARYO_AU );
            pLevel_Player->Action_Jump(1);
        }

        pLevel_Player->DownGrade_Player();
    }
}

void cEato::Editor_Activate(void)
{
    // get window manager
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    // direction
    CEGUI::Combobox* combobox = static_cast<CEGUI::Combobox*>(wmgr.createWindow("TaharezLook/Combobox", "editor_eato_direction"));
    Editor_Add(UTF8_("Direction"), UTF8_("Direction"), combobox, 100, 200);

    combobox->addItem(new CEGUI::ListboxTextItem("top_left"));
    combobox->addItem(new CEGUI::ListboxTextItem("top_right"));
    combobox->addItem(new CEGUI::ListboxTextItem("bottom_left"));
    combobox->addItem(new CEGUI::ListboxTextItem("bottom_right"));
    combobox->addItem(new CEGUI::ListboxTextItem("left_top"));
    combobox->addItem(new CEGUI::ListboxTextItem("left_bottom"));
    combobox->addItem(new CEGUI::ListboxTextItem("right_top"));
    combobox->addItem(new CEGUI::ListboxTextItem("right_bottom"));

    combobox->setText(Get_Direction_Name(m_start_direction));
    combobox->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cEato::Editor_Direction_Select, this));

    // image dir
    CEGUI::Editbox* editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "editor_eato_image_dir"));
    Editor_Add(UTF8_("Image directory"), UTF8_("Directory containing the images"), editbox, 200);

    editbox->setText(path_to_utf8(m_img_dir).c_str());
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cEato::Editor_Image_Dir_Text_Changed, this));
    // init
    Editor_Init();
}

bool cEato::Editor_Direction_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    Set_Direction(Get_Direction_Id(item->getText().c_str()));

    return 1;
}

bool cEato::Editor_Image_Dir_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Image_Dir(utf8_to_path(str_text));

    return 1;
}

std::string cEato::Create_Name(void) const
{
    std::string name = m_name; // dup
    name += " ";
    name += _(Get_Direction_Name(m_start_direction).c_str());

    if (m_start_image && !m_start_image->m_name.empty()) {
        name += " " + m_start_image->m_name;
    }

    return name;
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

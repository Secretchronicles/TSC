/***************************************************************************
 * level_exit.cpp  -  area to exit the current level
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

#include "../objects/level_exit.hpp"
#include "../level/level_player.hpp"
#include "../core/game_core.hpp"
#include "../user/preferences.hpp"
#include "../audio/audio.hpp"
#include "../core/framerate.hpp"
#include "../core/main.hpp"
#include "../video/gl_surface.hpp"
#include "../video/font.hpp"
#include "../video/renderer.hpp"
#include "../level/level.hpp"
#include "../core/i18n.hpp"
#include "../core/filesystem/filesystem.hpp"
#include "../core/xml_attributes.hpp"
#include "../scripting/events/exit_event.hpp"

namespace fs = boost::filesystem;

namespace TSC {

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

cLevel_Exit::cLevel_Exit(cSprite_Manager* sprite_manager)
    : cAnimated_Sprite(sprite_manager, "levelexit")
{
    cLevel_Exit::Init();
}

cLevel_Exit::cLevel_Exit(XmlAttributes& attributes, cSprite_Manager* sprite_manager)
    : cAnimated_Sprite(sprite_manager, "levelexit")
{
    cLevel_Exit::Init();

    // position
    Set_Pos(string_to_float(attributes["posx"]), string_to_float(attributes["posy"]), true);

    // type
    Level_Exit_type type = m_exit_type;
    if (attributes.count("type") > 0)
        type = static_cast<Level_Exit_type>(string_to_int(attributes["type"]));
    Set_Type(type);

    // motion
    Camera_movement cm = m_exit_motion;
    if (attributes.count("camera_motion") > 0)
        cm = static_cast<Camera_movement>(string_to_int(attributes["camera_motion"]));
    Set_Camera_Motion(cm);

    // destination level
    Set_Level(attributes["level_name"]);

    // destination entry
    Set_Entry(attributes["entry"]);

    // return level
    Set_Return_Level(attributes["return_level_name"]);

    // return entry
    Set_Return_Entry(attributes["return_entry"]);

    // path identifier
    if (m_exit_motion == CAMERA_MOVE_ALONG_PATH || m_exit_motion == CAMERA_MOVE_ALONG_PATH_BACKWARDS)
        Set_Path_Identifier(attributes["path_identifier"]);

    // direction
    if (m_exit_type == LEVEL_EXIT_WARP) {
        ObjectDirection dir = m_start_direction;

        if (attributes.count("direction") > 0)
            dir = Get_Direction_Id(attributes["direction"]);

        Set_Direction(dir);
    }
}

cLevel_Exit::~cLevel_Exit(void)
{
    if (m_editor_entry_name) {
        delete m_editor_entry_name;
        m_editor_entry_name = NULL;
    }
}

void cLevel_Exit::Init(void)
{
    m_sprite_array = ARRAY_ACTIVE;
    m_type = TYPE_LEVEL_EXIT;
    m_name = "Level Exit";
    m_massive_type = MASS_PASSIVE;
    m_editor_pos_z = 0.111f;
    m_camera_range = 1000;

    // size
    m_rect.m_w = 10;
    m_rect.m_h = 20;
    m_col_rect.m_w = m_rect.m_w;
    m_col_rect.m_h = m_rect.m_h;
    m_start_rect.m_w = m_rect.m_w;
    m_start_rect.m_h = m_rect.m_h;

    m_exit_type = LEVEL_EXIT_BEAM;
    m_exit_motion = CAMERA_MOVE_FLY;

    Set_Direction(DIR_DOWN);

    m_editor_color = red;
    m_editor_color.alpha = 128;

    m_editor_entry_name = NULL;
}

cLevel_Exit* cLevel_Exit::Copy(void) const
{
    cLevel_Exit* level_exit = new cLevel_Exit(m_sprite_manager);
    level_exit->Set_Pos(m_start_pos_x, m_start_pos_y, 1);
    level_exit->Set_Type(m_exit_type);
    level_exit->Set_Camera_Motion(m_exit_motion);
    level_exit->Set_Path_Identifier(m_path_identifier);
    level_exit->Set_Direction(m_start_direction);
    level_exit->Set_Level(m_dest_level);
    level_exit->Set_Entry(m_dest_entry);
    level_exit->Set_Return_Level(m_return_level);
    level_exit->Set_Return_Entry(m_return_entry);
    return level_exit;
}

std::string cLevel_Exit::Get_XML_Type_Name()
{
    return int_to_string(m_exit_type);
}

xmlpp::Element* cLevel_Exit::Save_To_XML_Node(xmlpp::Element* p_element)
{
    xmlpp::Element* p_node = cAnimated_Sprite::Save_To_XML_Node(p_element);

    // camera motion
    Add_Property(p_node, "camera_motion", m_exit_motion);

    // destination level
    if (!m_dest_level.empty())
        Add_Property(p_node, "level_name", m_dest_level);

    // destination entry name
    if (!m_dest_entry.empty())
        Add_Property(p_node, "entry", m_dest_entry);

    // return level name
    if (!m_return_level.empty())
        Add_Property(p_node, "return_level_name", m_return_level);

    // return entry
    if (!m_return_entry.empty())
        Add_Property(p_node, "return_entry", m_return_entry);

    // path identifier
    if (m_exit_motion == CAMERA_MOVE_ALONG_PATH || m_exit_motion == CAMERA_MOVE_ALONG_PATH_BACKWARDS) {
        if (!m_path_identifier.empty()) {
            Add_Property(p_node, "path_identifier", m_path_identifier);
        }
    }

    if (m_exit_type == LEVEL_EXIT_WARP)
        Add_Property(p_node, "direction", Get_Direction_Name(m_start_direction));

    return p_node;
}

void cLevel_Exit::Set_Direction(const ObjectDirection dir, bool initial /* = true */)
{
    // already set
    if (m_direction == dir) {
        return;
    }

    cAnimated_Sprite::Set_Direction(dir, initial);
}

std::string cLevel_Exit::Create_Name(void) const
{
    std::string name = m_name; // dup

    if (m_exit_type == LEVEL_EXIT_BEAM) {
        name += _(" Beam");
    }
    else if (m_exit_type == LEVEL_EXIT_WARP) {
        name += _(" Warp");

        if (m_direction == DIR_UP) {
            name += " U";
        }
        else if (m_direction == DIR_LEFT) {
            name += " L";
        }
        else if (m_direction == DIR_DOWN) {
            name += " D";
        }
        else if (m_direction == DIR_RIGHT) {
            name += " R";
        }
    }

    return name;
}

void cLevel_Exit::Draw(cSurface_Request* request /* = NULL */)
{
    if (!m_valid_draw) {
        return;
    }

    // draw color rect
    pVideo->Draw_Rect(m_col_rect.m_x - pActive_Camera->m_x, m_col_rect.m_y - pActive_Camera->m_y, m_col_rect.m_w, m_col_rect.m_h, m_editor_pos_z, &m_editor_color);

    // draw destination entry name
    if (m_editor_entry_name) {
        // create request
        cSurface_Request* surface_request = new cSurface_Request();
        // blit
        m_editor_entry_name->Blit(m_col_rect.m_x + m_col_rect.m_w + 5 - pActive_Camera->m_x, m_col_rect.m_y - pActive_Camera->m_y, m_editor_pos_z, surface_request);
        surface_request->m_shadow_pos = 2;
        surface_request->m_shadow_color = lightgreyalpha64;
        // add request
        pRenderer->Add(surface_request);
    }
}

void cLevel_Exit::Activate(void)
{
    // warp player out
    if (m_exit_type == LEVEL_EXIT_WARP) {
        pAudio->Play_Sound("enter_pipe.ogg");

        pLevel_Player->Set_Moving_State(STA_FALL);
        pLevel_Player->Set_Image_Num(pLevel_Player->Get_Image() + pLevel_Player->m_direction);
        pLevel_Player->Stop_Ducking();
        pLevel_Player->Reset_On_Ground();

        // set position and image
        if (m_direction == DIR_UP || m_direction == DIR_DOWN) {
            pLevel_Player->Set_Pos_X(m_col_rect.m_x - pLevel_Player->m_col_pos.m_x + (m_col_rect.m_w * 0.5f) - (pLevel_Player->m_col_rect.m_w * 0.5f));
        }
        else if (m_direction == DIR_LEFT || m_direction == DIR_RIGHT) {
            pLevel_Player->Set_Pos_Y(m_col_rect.m_y - pLevel_Player->m_col_pos.m_y + (m_col_rect.m_h * 0.5f) - (pLevel_Player->m_col_rect.m_h * 0.5f));

            // set rotation
            if (m_direction == DIR_RIGHT) {
                pLevel_Player->Set_Rotation_Z(90.0f);
            }
            else if (m_direction == DIR_LEFT) {
                pLevel_Player->Set_Rotation_Z(270.0f);
            }
        }

        float player_posz = pLevel_Player->m_pos_z;
        // change position z to be behind massive for the animation
        pLevel_Player->m_pos_z = 0.0799f;

        // set the speed
        float speedx = 0.0f;
        float speedy = 0.0f;

        if (m_direction == DIR_DOWN) {
            speedy = 2.7f;
        }
        else if (m_direction == DIR_UP) {
            speedy = -2.7f;
        }
        else if (m_direction == DIR_RIGHT) {
            speedx = 2.7f;
        }
        else if (m_direction == DIR_LEFT) {
            speedx = -2.7f;
        }

        // size moved is the height
        float maryo_size = pLevel_Player->m_col_rect.m_h;

        // move slowly in
        while (maryo_size > 0.0f) {
            pLevel_Player->Move(speedx, speedy);

            // reduce size
            if (speedx > 0.0f) {
                maryo_size -= speedx * pFramerate->m_speed_factor;
            }
            else if (speedx < 0.0f) {
                maryo_size += speedx * pFramerate->m_speed_factor;
            }
            else if (speedy > 0.0f) {
                maryo_size -= speedy * pFramerate->m_speed_factor;
            }
            else if (speedy < 0.0f) {
                maryo_size += speedy * pFramerate->m_speed_factor;
            }
            else {
                break;
            }

            // update audio
            pAudio->Update();
            // draw
            Draw_Game();

            pVideo->Render();
            pFramerate->Update();
        }

        // set position z back
        pLevel_Player->m_pos_z = player_posz;
        // set invisible
        pLevel_Player->Set_Active(0);

        if (m_direction == DIR_RIGHT || m_direction == DIR_LEFT) {
            pLevel_Player->Set_Rotation_Z(0);
        }
    }

    pLevel_Player->Clear_Collisions();

    // Fire exit event
    Scripting::cExit_Event evt;
    evt.Fire(pActive_Level->m_mruby, this);

    // exit level
    if (m_dest_level.empty() && m_dest_entry.empty()) {
        // If there is no destionation, we ignore any return level/entry and do no push it onto the stack
        std::string return_level, return_entry;

        if (pLevel_Player->Pop_Return(return_level, return_entry)) {
            pLevel_Manager->Goto_Sub_Level(return_level, return_entry, m_exit_motion, m_path_identifier);
        }
        else {
            pLevel_Manager->Finish_Level(1);
        }
    }
    // enter entry
    else {
        // Push return level/entry if any
        if (!m_return_level.empty() || !m_return_entry.empty()) {
            if (m_return_level.empty()) {
                // If user does not specify level, we need to push this level onto the stack
                // so when the exit is encountered in possbile another level, it will know
                // which level to return to.
                pLevel_Player->Push_Return(pActive_Level->Get_Level_Name(), m_return_entry);
            }
            else {
                pLevel_Player->Push_Return(m_return_level, m_return_entry);
            }
        }

        pLevel_Manager->Goto_Sub_Level(m_dest_level, m_dest_entry, m_exit_motion, m_path_identifier);
    }
}

void cLevel_Exit::Set_Type(Level_Exit_type exit_type)
{
    m_exit_type = exit_type;
}

void cLevel_Exit::Set_Camera_Motion(Camera_movement camera_motion)
{
    m_exit_motion = camera_motion;
}

void cLevel_Exit::Set_Level(std::string filename)
{
    if (filename.empty() && m_dest_entry.empty()) {
        m_dest_level.clear();
        // red for no destination level
        m_editor_color = red;
        m_editor_color.alpha = 128;
        return;
    }

    // lila for set destination level
    m_editor_color = lila;
    m_editor_color.alpha = 128;

    m_dest_level = filename;
}

std::string cLevel_Exit::Get_Level() const
{
    return m_dest_level;
}

fs::path cLevel_Exit::Get_Level_Path()
{
    return pLevel_Manager->Get_Path(m_dest_level);
}

void cLevel_Exit::Set_Entry(const std::string& entry_name)
{
    if (m_editor_entry_name) {
        delete m_editor_entry_name;
        m_editor_entry_name = NULL;
    }

    // Set new name
    m_dest_entry = entry_name;

    // if empty don't create the editor image
    if (m_dest_entry.empty()) {
        return;
    }

    m_editor_entry_name = pFont->Render_Text(pFont->m_font_small, m_dest_entry, white);
}

void cLevel_Exit::Set_Return_Level(const std::string& level)
{
    m_return_level = level;
}

void cLevel_Exit::Set_Return_Entry(const std::string& entry)
{
    m_return_entry = entry;
}

void cLevel_Exit::Set_Path_Identifier(const std::string& identifier)
{
    m_path_identifier = identifier;
}

bool cLevel_Exit::Is_Draw_Valid(void)
{
    // if editor not enabled
    if (!editor_enabled) {
        return 0;
    }

    // if not visible on the screen
    if (!m_active || !Is_Visible_On_Screen()) {
        return 0;
    }

    return 1;
}

void cLevel_Exit::Editor_Activate(void)
{
    // get window manager
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    // warp
    if (m_exit_type == LEVEL_EXIT_WARP) {
        // direction
        CEGUI::Combobox* combobox = static_cast<CEGUI::Combobox*>(wmgr.createWindow("TaharezLook/Combobox", "level_exit_direction"));
        Editor_Add(UTF8_("Direction"), UTF8_("Direction to move in"), combobox, 100, 105);

        combobox->addItem(new CEGUI::ListboxTextItem("up"));
        combobox->addItem(new CEGUI::ListboxTextItem("down"));
        combobox->addItem(new CEGUI::ListboxTextItem("right"));
        combobox->addItem(new CEGUI::ListboxTextItem("left"));
        combobox->setText(Get_Direction_Name(m_start_direction));

        combobox->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cLevel_Exit::Editor_Direction_Select, this));
    }

    // motion
    CEGUI::Combobox* combobox = static_cast<CEGUI::Combobox*>(wmgr.createWindow("TaharezLook/Combobox", "CAMERA_MOVEotion"));
    Editor_Add(UTF8_("Motion"), UTF8_("Camera Motion"), combobox, 100, 105);

    combobox->addItem(new CEGUI::ListboxTextItem("fly"));
    combobox->addItem(new CEGUI::ListboxTextItem("blink"));
    combobox->addItem(new CEGUI::ListboxTextItem("path"));
    combobox->addItem(new CEGUI::ListboxTextItem("path backwards"));

    if (m_exit_motion == CAMERA_MOVE_FLY) {
        combobox->setText("fly");
    }
    else if (m_exit_motion == CAMERA_MOVE_BLINK) {
        combobox->setText("blink");
    }
    else if (m_exit_motion == CAMERA_MOVE_ALONG_PATH_BACKWARDS) {
        combobox->setText("path backwards");
    }
    else {
        combobox->setText("path");
    }

    combobox->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cLevel_Exit::Editor_Motion_Select, this));

    // destination level
    CEGUI::Editbox* editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "level_exit_destination_level"));
    Editor_Add(UTF8_("Destination Level"), UTF8_("Name of the level that should be entered. If empty uses the current level."), editbox, 150);

    editbox->setText(Get_Level());
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cLevel_Exit::Editor_Destination_Level_Text_Changed, this));

    // destination entry
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "level_exit_destination_entry"));
    Editor_Add(UTF8_("Destination Entry"), UTF8_("Name of the Entry in the destination level. If empty the entry point is the player start position."), editbox, 150);

    editbox->setText(m_dest_entry.c_str());
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cLevel_Exit::Editor_Destination_Entry_Text_Changed, this));

    // return level
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "level_exit_return_level"));
    Editor_Add(UTF8_("Return Level"), UTF8_("Name of the level that should be pushed onto return stack."), editbox, 150);

    editbox->setText(m_return_level);
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cLevel_Exit::Editor_Return_Level_Text_Changed, this));

    // return entry
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "level_exit_return_entry"));
    Editor_Add(UTF8_("Return Entry"), UTF8_("Name of the Entry in the return level."), editbox, 150);

    editbox->setText(m_return_entry.c_str());
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cLevel_Exit::Editor_Return_Entry_Text_Changed, this));

    // path identifier
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "level_exit_path_identifier"));
    Editor_Add(UTF8_("Path Identifier"), UTF8_("Name of the Path to use for the camera movement."), editbox, 150);

    editbox->setText(m_path_identifier.c_str());
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cLevel_Exit::Editor_Path_Identifier_Text_Changed, this));


    // init
    Editor_Init();
}

void cLevel_Exit::Editor_State_Update(void)
{
    // get window manager
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    // path identifier
    CEGUI::Editbox* editbox_path_identifier = static_cast<CEGUI::Editbox*>(wmgr.getWindow("level_exit_path_identifier"));
    // destination level
    CEGUI::Editbox* editbox_destination_level = static_cast<CEGUI::Editbox*>(wmgr.getWindow("level_exit_destination_level"));
    // direction
    //CEGUI::Combobox *combobox_direction = static_cast<CEGUI::Combobox *>(wmgr.getWindow( "level_exit_direction" ));


    if (m_exit_motion == CAMERA_MOVE_ALONG_PATH || m_exit_motion == CAMERA_MOVE_ALONG_PATH_BACKWARDS) {
        editbox_path_identifier->setEnabled(1);
        editbox_destination_level->setEnabled(0);
    }
    else if (m_exit_motion == CAMERA_MOVE_FLY) {
        editbox_path_identifier->setEnabled(0);
        editbox_destination_level->setEnabled(0);
    }
    else {
        editbox_path_identifier->setEnabled(0);
        editbox_destination_level->setEnabled(1);
    }

    /*if( m_exit_type == LEVEL_EXIT_WARP )
    {
        combobox_direction->setEnabled( 1 );
    }
    else
    {
        combobox_direction->setEnabled( 0 );
    }*/
}

bool cLevel_Exit::Editor_Direction_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    Set_Direction(Get_Direction_Id(item->getText().c_str()));

    return 1;
}

bool cLevel_Exit::Editor_Motion_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();
    std::string str_text = item->getText().c_str();

    if (str_text.compare("fly") == 0) {
        Set_Camera_Motion(CAMERA_MOVE_FLY);
    }
    else if (str_text.compare("blink") == 0) {
        Set_Camera_Motion(CAMERA_MOVE_BLINK);
    }
    else if (str_text.compare("path backwards") == 0) {
        Set_Camera_Motion(CAMERA_MOVE_ALONG_PATH_BACKWARDS);
    }
    else {
        Set_Camera_Motion(CAMERA_MOVE_ALONG_PATH);
    }

    Editor_State_Update();

    return 1;
}

bool cLevel_Exit::Editor_Destination_Level_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Level(str_text);

    return 1;
}

bool cLevel_Exit::Editor_Destination_Entry_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Entry(str_text);

    return 1;
}

bool cLevel_Exit::Editor_Return_Level_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Return_Level(str_text);

    return 1;
}

bool cLevel_Exit::Editor_Return_Entry_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Return_Entry(str_text);

    return 1;
}

bool cLevel_Exit::Editor_Path_Identifier_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Path_Identifier(str_text);

    return 1;
}

void cLevel_Exit::Set_Massive_Type(MassiveType type)
{
    // Ignore to prevent "m" toggling in level editor
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

/***************************************************************************
 * level_entry.cpp  -  entry point to enter a level
 *
 * Copyright © 2007 - 2011 Florian Richter
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

#include "../objects/level_entry.hpp"
#include "../level/level_player.hpp"
#include "../core/game_core.hpp"
#include "../audio/audio.hpp"
#include "../core/framerate.hpp"
#include "../core/main.hpp"
#include "../video/gl_surface.hpp"
#include "../video/font.hpp"
#include "../video/renderer.hpp"
#include "../level/level.hpp"
#include "../core/i18n.hpp"
#include "../core/sprite_manager.hpp"
#include "../core/xml_attributes.hpp"
#include "../scripting/events/enter_event.hpp"

namespace TSC {

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

cLevel_Entry::cLevel_Entry(cSprite_Manager* sprite_manager)
    : cAnimated_Sprite(sprite_manager, "level_entry")
{
    cLevel_Entry::Init();
}

cLevel_Entry::cLevel_Entry(XmlAttributes& attributes, cSprite_Manager* sprite_manager)
    : cAnimated_Sprite(sprite_manager, "level_entry")
{
    cLevel_Entry::Init();

    // position
    Set_Pos(string_to_float(attributes["posx"]), string_to_float(attributes["posy"]), true);
    // type
    Set_Type(static_cast<Level_Entry_type>(attributes.fetch<int>("type", m_entry_type)));
    // name
    Set_Name(attributes["name"]);
    // direction
    Set_Direction(Get_Direction_Id(attributes.fetch("direction", Get_Direction_Name(m_start_direction))));
}

cLevel_Entry::~cLevel_Entry(void)
{
    if (m_editor_entry_name) {
        delete m_editor_entry_name;
        m_editor_entry_name = NULL;
    }
}

void cLevel_Entry::Init(void)
{
    m_sprite_array = ARRAY_ACTIVE;
    m_type = TYPE_LEVEL_ENTRY;
    m_massive_type = MASS_PASSIVE;
    m_name = "Level Entry";
    m_editor_pos_z = 0.112f;
    m_camera_range = 1000;

    // size
    m_rect.m_w = 10;
    m_rect.m_h = 20;
    m_col_rect.m_w = m_rect.m_w;
    m_col_rect.m_h = m_rect.m_h;
    m_start_rect.m_w = m_rect.m_w;
    m_start_rect.m_h = m_rect.m_h;

    m_entry_type = LEVEL_ENTRY_WARP;
    Set_Direction(DIR_UP);

    m_editor_color = lightblue;
    m_editor_color.alpha = 128;

    m_editor_entry_name = NULL;
}

cLevel_Entry* cLevel_Entry::Copy(void) const
{
    cLevel_Entry* level_entry = new cLevel_Entry(m_sprite_manager);
    level_entry->Set_Pos(m_start_pos_x, m_start_pos_y, 1);
    level_entry->Set_Type(m_entry_type);
    level_entry->Set_Direction(m_start_direction);
    level_entry->Set_Name(m_entry_name);
    return level_entry;
}

std::string cLevel_Entry::Get_XML_Type_Name()
{
    return int_to_string(m_entry_type);
}

xmlpp::Element* cLevel_Entry::Save_To_XML_Node(xmlpp::Element* p_element)
{
    xmlpp::Element* p_node = cAnimated_Sprite::Save_To_XML_Node(p_element);

    // direction
    if (m_entry_type == LEVEL_ENTRY_WARP)
        Add_Property(p_node, "direction", Get_Direction_Name(m_start_direction));
    // name
    if (!m_entry_name.empty())
        Add_Property(p_node, "name", m_entry_name);

    return p_node;
}

void cLevel_Entry::Set_Direction(const ObjectDirection dir)
{
    // already set
    if (m_direction == dir) {
        return;
    }

    cAnimated_Sprite::Set_Direction(dir, 1);
}

std::string cLevel_Entry::Create_Name(void) const
{
    std::string name = m_name; // Dup

    if (m_entry_type == LEVEL_ENTRY_BEAM) {
        name += _(" Beam");
    }
    else if (m_entry_type == LEVEL_ENTRY_WARP) {
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

void cLevel_Entry::Draw(cSurface_Request* request /* = NULL */)
{
    if (!m_valid_draw) {
        return;
    }

    // draw color rect
    pVideo->Draw_Rect(m_col_rect.m_x - pActive_Camera->m_x, m_col_rect.m_y - pActive_Camera->m_y, m_col_rect.m_w, m_col_rect.m_h, m_editor_pos_z, &m_editor_color);

    // draw entry name
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

void cLevel_Entry::Activate(void)
{
    // warp player in
    if (m_entry_type == LEVEL_ENTRY_WARP) {
        pAudio->Play_Sound("leave_pipe.ogg");

        // set state to linked to stop checking for on ground objects which sometimes changes the position
        Moving_state player_state = pLevel_Player->m_state;
        pLevel_Player->m_state = STA_OBJ_LINKED;

        pLevel_Player->Stop_Ducking();
        pLevel_Player->Reset_On_Ground();

        // set position
        pLevel_Player->Set_Pos(Get_Player_Pos_X(), Get_Player_Pos_Y());

        // set image
        if (m_direction == DIR_UP || m_direction == DIR_DOWN) {
            pLevel_Player->Set_Image_Num(MARYO_IMG_FALL + pLevel_Player->m_direction);
        }
        else if (m_direction == DIR_LEFT || m_direction == DIR_RIGHT) {
            pLevel_Player->Set_Image_Num(pLevel_Player->m_direction);

            // set rotation
            if (m_direction == DIR_RIGHT) {
                pLevel_Player->Set_Rotation_Z(90);
            }
            else if (m_direction == DIR_LEFT) {
                pLevel_Player->Set_Rotation_Z(270);
            }
        }

        // change position z to be behind massive for the animation
        float player_posz = pLevel_Player->m_pos_z;
        pLevel_Player->m_pos_z = 0.0799f;

        // move slowly out
        while (1) {
            if (m_direction == DIR_DOWN) {
                pLevel_Player->Move(0.0f, 2.8f);

                if (pLevel_Player->m_pos_y > m_rect.m_y + m_rect.m_h) {
                    break;
                }
            }
            else if (m_direction == DIR_UP) {
                pLevel_Player->Move(0.0f, -2.8f);

                if (pLevel_Player->m_col_rect.m_y + pLevel_Player->m_col_rect.m_h < m_rect.m_y) {
                    break;
                }
            }
            else if (m_direction == DIR_RIGHT) {
                pLevel_Player->Move(2.0f, 0.0f);

                if (pLevel_Player->m_pos_x > m_rect.m_x + m_rect.m_w) {
                    break;
                }
            }
            else if (m_direction == DIR_LEFT) {
                pLevel_Player->Move(-2.0f, 0.0f);

                if (pLevel_Player->m_col_rect.m_x + pLevel_Player->m_col_rect.m_w < m_rect.m_x) {
                    break;
                }
            }
            else {
                break;
            }

            // update audio
            pAudio->Update();
            // center camera
            pActive_Camera->Center();
            // keep particles on screen
            for (cSprite_List::iterator itr = m_sprite_manager->objects.begin(); itr != m_sprite_manager->objects.end(); ++itr) {
                cSprite* obj = (*itr);

                if (obj->m_type == TYPE_PARTICLE_EMITTER) {
                    cParticle_Emitter* emitter = static_cast<cParticle_Emitter*>(obj);
                    emitter->Update_Position();
                }
            }
            // draw
            Draw_Game();

            pVideo->Render();
            pFramerate->Update();
        }

        // set position z back
        pLevel_Player->m_pos_z = player_posz;
        // set state back
        pLevel_Player->m_state = player_state;

        if (m_direction == DIR_RIGHT || m_direction == DIR_LEFT) {
            pLevel_Player->Set_Rotation_Z(0);
        }
    }
    // beam player in
    else if (m_entry_type == LEVEL_ENTRY_BEAM) {
        // set position
        pLevel_Player->Set_Pos(Get_Player_Pos_X(), Get_Player_Pos_Y());
    }

    pLevel_Player->Clear_Collisions();

    // Fire entry event
    Scripting::cEnter_Event evt;
    evt.Fire(pActive_Level->m_mruby, this);
}

void cLevel_Entry::Set_Type(Level_Entry_type new_type)
{
    m_entry_type = new_type;
}

float cLevel_Entry::Get_Player_Pos_X(void) const
{
    if (m_entry_type == LEVEL_ENTRY_WARP) {
        // left
        if (m_direction == DIR_LEFT) {
            return m_col_rect.m_x - pLevel_Player->m_col_pos.m_y + m_col_rect.m_w;
        }
        // right
        else if (m_direction == DIR_RIGHT) {
            return m_col_rect.m_x - pLevel_Player->m_col_pos.m_y - m_col_rect.m_w - pLevel_Player->m_col_rect.m_w;
        }

        // up/down
        return m_col_rect.m_x - pLevel_Player->m_col_pos.m_x + (m_col_rect.m_w * 0.5f) - (pLevel_Player->m_col_rect.m_w * 0.5f);
    }
    else if (m_entry_type == LEVEL_ENTRY_BEAM) {
        return m_col_rect.m_x + (m_col_rect.m_w * 0.5f) - pLevel_Player->m_col_pos.m_y - (pLevel_Player->m_col_rect.m_w * 0.5f);
    }

    return 0;
}

float cLevel_Entry::Get_Player_Pos_Y(void) const
{
    if (m_entry_type == LEVEL_ENTRY_WARP) {
        // up
        if (m_direction == DIR_UP) {
            return m_col_rect.m_y - pLevel_Player->m_col_pos.m_y + m_col_rect.m_h;
        }
        // down
        else if (m_direction == DIR_DOWN) {
            return m_col_rect.m_y - pLevel_Player->m_col_pos.m_y - 5 - pLevel_Player->m_rect.m_h;
        }

        // left/right
        return m_col_rect.m_y - pLevel_Player->m_col_pos.m_y + (m_col_rect.m_h * 0.5f) - (pLevel_Player->m_col_rect.m_h * 0.5f);
    }
    else if (m_entry_type == LEVEL_ENTRY_BEAM) {
        return m_col_rect.m_y + m_col_rect.m_h - pLevel_Player->m_col_pos.m_y - pLevel_Player->m_col_rect.m_h;
    }

    return 0;
}

void cLevel_Entry::Set_Name(const std::string& str_name)
{
    // delete editor image
    if (m_editor_entry_name) {
        delete m_editor_entry_name;
        m_editor_entry_name = NULL;
    }

    // Set new name
    m_entry_name = str_name;

    // if empty don't create editor image
    if (m_entry_name.empty()) {
        return;
    }

    m_editor_entry_name = pFont->Render_Text(pFont->m_font_small, m_entry_name, white);
}

bool cLevel_Entry::Is_Draw_Valid(void)
{
    // if editor not enabled
    if (!editor_enabled) {
        return 0;
    }

    // if not active or not visible on the screen
    if (!m_active || !Is_Visible_On_Screen()) {
        return 0;
    }

    return 1;
}

void cLevel_Entry::Editor_Activate(void)
{
    // get window manager
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    // warp
    if (m_entry_type == LEVEL_ENTRY_WARP) {
        // direction
        CEGUI::Combobox* combobox = static_cast<CEGUI::Combobox*>(wmgr.createWindow("TaharezLook/Combobox", "level_entry_direction"));
        Editor_Add(UTF8_("Direction"), UTF8_("The direction to come out"), combobox, 100, 105);

        combobox->addItem(new CEGUI::ListboxTextItem("up"));
        combobox->addItem(new CEGUI::ListboxTextItem("down"));
        combobox->addItem(new CEGUI::ListboxTextItem("right"));
        combobox->addItem(new CEGUI::ListboxTextItem("left"));
        combobox->setText(Get_Direction_Name(m_start_direction));

        combobox->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cLevel_Entry::Editor_Direction_Select, this));
    }

    // destination entry
    CEGUI::Editbox* editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "level_entry_name"));
    Editor_Add(reinterpret_cast<const CEGUI::utf8*>(C_("level", "Name")), UTF8_("Name for identification"), editbox, 150);

    editbox->setText(m_entry_name.c_str());
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cLevel_Entry::Editor_Name_Text_Changed, this));

    // init
    Editor_Init();
}

bool cLevel_Entry::Editor_Direction_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    Set_Direction(Get_Direction_Id(item->getText().c_str()));

    return 1;
}

bool cLevel_Entry::Editor_Name_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Name(str_text);

    return 1;
}

void cLevel_Entry::Set_Massive_Type(MassiveType type)
{
    // Ignore to prevent "m" toggling in level editor
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

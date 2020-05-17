/***************************************************************************
 * waypoint.cpp  -  waypoint class for the Overworld
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

#include "../overworld/world_waypoint.hpp"
#include "../overworld/overworld.hpp"
#include "../core/game_core.hpp"
#include "../core/framerate.hpp"
#include "../user/preferences.hpp"
#include "../video/renderer.hpp"
#include "../level/level.hpp"
#include "../core/math/utilities.hpp"
#include "../core/i18n.hpp"
#include "../video/gl_surface.hpp"
#include "../core/filesystem/filesystem.hpp"
#include "../core/filesystem/resource_manager.hpp"
#include "../core/xml_attributes.hpp"
#include "../core/global_basic.hpp"
#include "../core/sprite_manager.hpp"
#include "../core/editor/editor.hpp"
#include "world_editor.hpp"

// Maximum number of waypoint exits is 4. One for each direction.
#define MAX_WAYPOINT_EXITS 4

using namespace std;

namespace TSC {

/* *** *** *** *** *** *** *** *** cWaypoint *** *** *** *** *** *** *** *** *** */

cWaypoint::cWaypoint(cSprite_Manager* sprite_manager)
    : cSprite(sprite_manager, "waypoint")
{
    cWaypoint::Init();
}

cWaypoint::cWaypoint(XmlAttributes& attributes, cSprite_Manager* sprite_manager)
    : cSprite(sprite_manager, "waypoint")
{
    cWaypoint::Init();

    // position
    Set_Pos(static_cast<float>(attributes.fetch<int>("posx", 0)), static_cast<float>(attributes.fetch<int>("posy", 0)), true);

    // image
    /*
    if (attributes.exists("image"))
        Set_Image(pVideo->Get_Surface(utf8_to_path(attributes["image"])), true);
    */

    // type
    m_waypoint_type = static_cast<Waypoint_type>(attributes.fetch<int>("type", WAYPOINT_NORMAL));

    // destination
    // pre 0.99.6 : world
    if (attributes.exists("world"))
        Set_Destination(attributes["world"]);
    // pre 0.99.6 : level
    else if (attributes.exists("level"))
        Set_Destination(attributes["level"]);
    // default : destination
    else
        Set_Destination(attributes["destination"]);

    if (attributes.exists("direction_backward")) // pre 2.1.0
        // backward direction
        Set_Direction_Backward(Get_Direction_Id(attributes.fetch<std::string>("direction_backward", "left")));

    if (attributes.exists("direction_forward")) // pre 2.1.0
        // forward direction
        Set_Direction_Forward(Get_Direction_Id(attributes.fetch<std::string>("direction_forward", "right")));

    // access
    Set_Access(attributes.fetch<bool>("access", true), true);

    // >=-2.1.0 way to save waypoint exits
    for(int i=0; i < MAX_WAYPOINT_EXITS; i++) {
        std::string direction_attr       = "waypoint_exit_" + int_to_string(i) + "_direction";
        std::string level_exit_name_attr = "waypoint_exit_" + int_to_string(i) + "_level_exit_name";
        std::string line_start_uid_attr  = "waypoint_exit_" + int_to_string(i) + "_line_start_uid";
        std::string locked_attr          = "waypoint_exit_" + int_to_string(i) + "_locked";

        if (attributes.exists(direction_attr)) { // assuming that in this case all 4 attributes exist
            waypoint_exit exit;
            exit.direction       = Get_Direction_Id(attributes[direction_attr]);
            exit.level_exit_name = attributes[level_exit_name_attr];
            exit.line_start_uid  = attributes.fetch<int>(line_start_uid_attr, 0);
            exit.locked          = attributes.fetch<bool>(locked_attr, true);

            m_exits.push_back(exit);
        }
    }
}

cWaypoint::~cWaypoint(void)
{
    //
}

void cWaypoint::Init(void)
{
    m_sprite_array = ARRAY_PASSIVE;
    m_type = TYPE_OW_WAYPOINT;
    // Pretend we are passive (so the player doesn’t get stuck on us),
    // but set the Z for massive objects, so waypoints always appear
    // above the background (which is passive).
    m_massive_type = MASS_PASSIVE;
    m_pos_z = cSprite::m_pos_z_massive_start;

    m_camera_range = 0;

    m_waypoint_type = WAYPOINT_NORMAL;
    m_name = _("Waypoint");

    m_access = 0;
    m_access_default = 0;
    m_direction_forward = DIR_UNDEFINED;
    m_direction_backward = DIR_UNDEFINED;

    m_glim_color = Get_Random_Float(0, 100);
    m_glim_mod = 1;

    m_arrow_forward = NULL;
    m_arrow_backward = NULL;

    mp_wp_exits_box      = NULL;
    mp_wp_exit_dir_box   = NULL;
    mp_wp_exit_lock_box  = NULL;
    mp_wp_exit_name_edit = NULL;
    mp_wp_exit_uid_edit  = NULL;

    Set_Image(pVideo->Get_Surface("world/waypoint/default_1.png"));
}

cWaypoint* cWaypoint::Copy(void) const
{
    cWaypoint* waypoint = new cWaypoint(m_sprite_manager);
    waypoint->Set_Pos(m_start_pos_x, m_start_pos_y, 1);
    waypoint->Set_Image(m_start_image, 1);
    waypoint->m_waypoint_type = m_waypoint_type;
    waypoint->Set_Destination(m_destination);
    waypoint->Set_Direction_Forward(m_direction_forward);
    waypoint->Set_Direction_Backward(m_direction_backward);
    waypoint->Set_Access(m_access_default, 1);
    return waypoint;
}

std::string cWaypoint::Get_XML_Type_Name()
{
    return int_to_string(m_waypoint_type);
}

xmlpp::Element* cWaypoint::Save_To_XML_Node(xmlpp::Element* p_element)
{
    xmlpp::Element* p_node = cSprite::Save_To_XML_Node(p_element);

    // destination
    Add_Property(p_node, "destination", m_destination);

    // The following attributes only affect pre 2.1.0 worlds.

    if (m_direction_backward != DIR_UNDEFINED)
        // direction backward
        Add_Property(p_node, "direction_backward", Get_Direction_Name(m_direction_backward));
    if (m_direction_forward != DIR_UNDEFINED)
        // direction forward
        Add_Property(p_node, "direction_forward", Get_Direction_Name(m_direction_forward));

    // access
    Add_Property(p_node, "access", m_access_default);

    // post-2.1.0 way of saving waypoint exits. Note that this is rather ugly
    // XML, but TSC's XML loader does not support nested tags as of now. Everything
    // must be expressed using a flat list of properties.
    for(size_t i=0; i < m_exits.size(); i++) {
        std::string str_pos = int_to_string(i);
        const waypoint_exit& exit  = m_exits[i];

        Add_Property(p_node, "waypoint_exit_" + str_pos + "_direction", Get_Direction_Name(exit.direction));
        Add_Property(p_node, "waypoint_exit_" + str_pos + "_level_exit_name", exit.level_exit_name);
        Add_Property(p_node, "waypoint_exit_" + str_pos + "_line_start_uid", exit.line_start_uid);
        Add_Property(p_node, "waypoint_exit_" + str_pos + "_locked", exit.locked);
    }

    return p_node;
}

void cWaypoint::Update(void)
{
    if (m_auto_destroy) {
        return;
    }

    cSprite::Update();

    if (m_glim_mod) {
        m_glim_color += pFramerate->m_speed_factor * 3.0f;
    }
    else {
        m_glim_color -= pFramerate->m_speed_factor * 3.0f;
    }

    if (m_glim_color > 120.0f) {
        m_glim_mod = 0;
    }
    else if (m_glim_color < 7.0f) {
        m_glim_mod = 1;
    }
}

void cWaypoint::Draw(cSurface_Request* request /* = NULL  */)
{
    if (m_auto_destroy) {
        return;
    }

    if (pOverworld_Manager->m_debug_mode || editor_world_enabled) {
        float x;
        float y;

        if (m_exits.empty()) { // pre 2.1.0 forward and backward direction
            // direction back arrow
            if (m_direction_backward == DIR_RIGHT || m_direction_backward == DIR_LEFT || m_direction_backward == DIR_UP || m_direction_backward == DIR_DOWN) {
                x = m_rect.m_x - pActive_Camera->m_x;
                y = m_rect.m_y - pActive_Camera->m_y;

                // create request
                cSurface_Request* surface_request = new cSurface_Request();

                if (m_direction_backward == DIR_RIGHT) {
                    x += m_rect.m_w;
                    y += (m_rect.m_h * 0.5f) - (m_arrow_backward->m_w * 0.5f);
                }
                else if (m_direction_backward == DIR_LEFT) {
                    x -= m_arrow_backward->m_w;
                    y += (m_rect.m_h * 0.5f) - (m_arrow_backward->m_w * 0.5f);
                }
                else if (m_direction_backward == DIR_UP) {
                    y -= m_arrow_backward->m_h;
                    x += (m_rect.m_w * 0.5f) - (m_arrow_backward->m_h * 0.5f);
                }
                // down
                else {
                    y += m_rect.m_h;
                    x += (m_rect.m_w * 0.5f) - (m_arrow_backward->m_h * 0.5f);
                }

                m_arrow_backward->Blit(x, y, 0.089f, surface_request);
                surface_request->m_shadow_pos = 2;
                surface_request->m_shadow_color = lightgreyalpha64;
                // add request
                pRenderer->Add(surface_request);
            }

            // direction forward arrow
            if (m_direction_forward == DIR_RIGHT || m_direction_forward == DIR_LEFT || m_direction_forward == DIR_UP || m_direction_forward == DIR_DOWN) {
                x = m_rect.m_x - pActive_Camera->m_x;
                y = m_rect.m_y - pActive_Camera->m_y;

                // create request
                cSurface_Request* surface_request = new cSurface_Request();

                if (m_direction_forward == DIR_RIGHT) {
                    x += m_rect.m_w;
                    y += (m_rect.m_h * 0.5f) - (m_arrow_forward->m_w * 0.5f);
                }
                else if (m_direction_forward == DIR_LEFT) {
                    x -= m_arrow_forward->m_w;
                    y += (m_rect.m_h * 0.5f) - (m_arrow_forward->m_w * 0.5f);
                }
                else if (m_direction_forward == DIR_UP) {
                    y -= m_arrow_forward->m_h;
                    x += (m_rect.m_w * 0.5f) - (m_arrow_forward->m_h * 0.5f);
                }
                // down
                else {
                    y += m_rect.m_h;
                    x += (m_rect.m_w * 0.5f) - (m_arrow_forward->m_h * 0.5f);
                }

                m_arrow_forward->Blit(x, y, 0.089f, surface_request);
                surface_request->m_shadow_pos = 2;
                surface_request->m_shadow_color = lightgreyalpha64;
                // add request
                pRenderer->Add(surface_request);
            }
        }
        else { // Since 2.1.0: one arrow per direction
            for(waypoint_exit& ex: m_exits) {
                cGL_Surface* arrow = NULL;

                x = m_rect.m_x - pActive_Camera->m_x;
                y = m_rect.m_y - pActive_Camera->m_y;

                std::string color;
                if (ex.locked)
                    color = "blue";
                else
                    color = "white";

                switch(ex.direction) {
                case DIR_RIGHT:
                    arrow = pVideo->Get_Surface("game/arrow/small/" + color + "/right.png");
                    x += m_rect.m_w;
                    y += (m_rect.m_h * 0.5f) - (arrow->m_w * 0.5f);
                    break;
                case DIR_LEFT:
                    arrow = pVideo->Get_Surface("game/arrow/small/" + color + "/left.png");
                    x -= arrow->m_w;
                    y += (m_rect.m_h * 0.5f) - (arrow->m_w * 0.5f);
                    break;
                case DIR_UP:
                    arrow = pVideo->Get_Surface("game/arrow/small/" + color + "/up.png");
                    y -= arrow->m_h;
                    x += (m_rect.m_w * 0.5f) - (arrow->m_h * 0.5f);
                    break;
                case DIR_DOWN:
                    arrow = pVideo->Get_Surface("game/arrow/small/" + color + "/down.png");
                    y += m_rect.m_h;
                    x += (m_rect.m_w * 0.5f) - (arrow->m_h * 0.5f);
                    break;
                default: // invalid direction
                    continue;
                }

                // create request
                cSurface_Request* surface_request = new cSurface_Request();
                arrow->Blit(x, y, 0.089f, surface_request);
                surface_request->m_shadow_pos = 2;
                surface_request->m_shadow_color = lightgreyalpha64;
                // add request
                pRenderer->Add(surface_request);

                // TODO: Does `arrow' have to be freed?
            }
        }
    }

    // draw waypoint
    if ((m_access && (m_waypoint_type == 1 || m_waypoint_type == 2)) || pOverworld_Manager->m_debug_mode || editor_world_enabled) {
        bool create_request = 0;

        if (!request) {
            create_request = 1;
            // create request
            request = new cSurface_Request();
        }

        // draw
        cSprite::Draw(request);

        // default color
        if (!pOverworld_Manager->m_debug_mode) {
            request->m_color = Color(static_cast<uint8_t>(255), 100 + static_cast<uint8_t>(m_glim_color), 10);
        }
        // change to debug color
        else {
            if (m_access) {
                request->m_color = Color(static_cast<uint8_t>(255), 100 + static_cast<uint8_t>(m_glim_color), 200);
            }
            else {
                request->m_color =  Color(static_cast<uint8_t>(20), 100 + static_cast<uint8_t>(m_glim_color), 10);
            }
        }

        if (create_request) {
            // add request
            pRenderer->Add(request);
        }
    }
}

void cWaypoint::Set_Direction_Forward(ObjectDirection direction)
{
    m_direction_forward = direction;

    if (direction == DIR_LEFT) {
        m_arrow_forward = pVideo->Get_Surface("game/arrow/small/white/left.png");
    }
    else if (direction == DIR_RIGHT) {
        m_arrow_forward = pVideo->Get_Surface("game/arrow/small/white/right.png");
    }
    else if (direction == DIR_UP) {
        m_arrow_forward = pVideo->Get_Surface("game/arrow/small/white/up.png");
    }
    else if (direction == DIR_DOWN) {
        m_arrow_forward = pVideo->Get_Surface("game/arrow/small/white/down.png");
    }
}

void cWaypoint::Set_Direction_Backward(ObjectDirection direction)
{
    m_direction_backward = direction;

    if (direction == DIR_LEFT) {
        m_arrow_backward = pVideo->Get_Surface("game/arrow/small/blue/left.png");
    }
    else if (direction == DIR_RIGHT) {
        m_arrow_backward = pVideo->Get_Surface("game/arrow/small/blue/right.png");
    }
    else if (direction == DIR_UP) {
        m_arrow_backward = pVideo->Get_Surface("game/arrow/small/blue/up.png");
    }
    else if (direction == DIR_DOWN) {
        m_arrow_backward = pVideo->Get_Surface("game/arrow/small/blue/down.png");
    }
}

void cWaypoint::Set_Access(bool enabled, bool new_start_access /* = 0 */)
{
    m_access = enabled;

    if (new_start_access) {
        m_access_default = m_access;
    }
}

bool cWaypoint::Unlock_Exit(std::string exit_name)
{
    for(waypoint_exit& ex: m_exits) {
        if (ex.level_exit_name == exit_name) {
            ex.locked = false;
            return true;
        }
    }

    return false;
}

void cWaypoint::Unlock_All_Exits()
{
    for(waypoint_exit& ex: m_exits)
        ex.locked = false;
}

void cWaypoint::Set_Destination(std::string level_or_worldname)
{
    m_destination = level_or_worldname;
}

std::string cWaypoint::Get_Destination() const
{
    return m_destination;
}

boost::filesystem::path cWaypoint::Get_Destination_Path()
{
    boost::filesystem::path result;

    switch (m_waypoint_type) {
    case WAYPOINT_NORMAL:
        return pLevel_Manager->Get_Path(m_destination);
    case WAYPOINT_WORLD_LINK:
        // I don't think this function ever gets called
        result = pResource_Manager->Get_User_World(m_destination);
        if (!boost::filesystem::exists(result))
            result = pResource_Manager->Get_Game_Overworld(m_destination);
        return result;
    default:
        // FIXME: Throw an exception
        cerr << "Error: Undefined waypoint type" << m_waypoint_type << endl;
        return boost::filesystem::path();
    }
}

void cWaypoint::Editor_Activate(void)
{
    // get window manager
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    // Type
    CEGUI::Combobox* combobox = static_cast<CEGUI::Combobox*>(wmgr.createWindow("TSCLook256/Combobox", "waypoint_type"));

    combobox->addItem(new CEGUI::ListboxTextItem(UTF8_("Level")));
    combobox->addItem(new CEGUI::ListboxTextItem(UTF8_("World")));

    if (m_waypoint_type == WAYPOINT_NORMAL) {
        combobox->setText(UTF8_("Level"));
    }
    else {
        combobox->setText(UTF8_("World"));
    }

    combobox->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cWaypoint::Editor_Type_Select, this));
    pWorld_Editor->Add_Config_Widget(UTF8_("Type"), UTF8_("Destination type"), combobox);

    // destination
    CEGUI::Editbox* editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TSCLook256/Editbox", "waypoint_destination"));
    pWorld_Editor->Add_Config_Widget(UTF8_("Destination"), UTF8_("Destination level or world"), editbox);

    editbox->setText(Get_Destination());
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cWaypoint::Editor_Destination_Text_Changed, this));

    // pre 2.1.0: direction backward
    if (m_direction_backward != DIR_UNDEFINED) {
        // backward direction
        combobox = static_cast<CEGUI::Combobox*>(wmgr.createWindow("TSCLook256/Combobox", "waypoint_backward_direction"));

        combobox->addItem(new CEGUI::ListboxTextItem("up"));
        combobox->addItem(new CEGUI::ListboxTextItem("down"));
        combobox->addItem(new CEGUI::ListboxTextItem("right"));
        combobox->addItem(new CEGUI::ListboxTextItem("left"));
        combobox->setText(Get_Direction_Name(m_direction_backward));

        combobox->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cWaypoint::Editor_Backward_Direction_Select, this));
        pWorld_Editor->Add_Config_Widget(UTF8_("Backward Direction"), UTF8_("Backward Direction"), combobox);
    }

    // pre 2.1.0: direction forward
    if (m_direction_forward != DIR_UNDEFINED) {
        // forward direction
        combobox = static_cast<CEGUI::Combobox*>(wmgr.createWindow("TSCLook256/Combobox", "waypoint_forward_direction"));

        combobox->addItem(new CEGUI::ListboxTextItem("up"));
        combobox->addItem(new CEGUI::ListboxTextItem("down"));
        combobox->addItem(new CEGUI::ListboxTextItem("right"));
        combobox->addItem(new CEGUI::ListboxTextItem("left"));
        combobox->setText(Get_Direction_Name(m_direction_forward));

        combobox->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cWaypoint::Editor_Forward_Direction_Select, this));
        pWorld_Editor->Add_Config_Widget(UTF8_("Forward Direction"), UTF8_("Forward Direction"), combobox);
    }

    // Access
    combobox = static_cast<CEGUI::Combobox*>(wmgr.createWindow("TSCLook256/Combobox", "waypoint_access"));

    combobox->addItem(new CEGUI::ListboxTextItem(UTF8_("Enabled")));
    combobox->addItem(new CEGUI::ListboxTextItem(UTF8_("Disabled")));

    if (m_access_default) {
        combobox->setText(UTF8_("Enabled"));
    }
    else {
        combobox->setText(UTF8_("Disabled"));
    }

    combobox->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cWaypoint::Editor_Access_Select, this));
    pWorld_Editor->Add_Config_Widget(UTF8_("Default Access"), UTF8_("Enable if the Waypoint should be always accessible."), combobox);

    // waypoint exits
    mp_wp_exits_box = static_cast<CEGUI::Combobox*>(wmgr.createWindow("TSCLook256/Combobox", "waypoint_exit_select"));
    rebuild_waypoint_exit_list();

    mp_wp_exits_box->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cWaypoint::Editor_Waypoint_Exit_Select, this));
    pWorld_Editor->Add_Config_Widget(UTF8_("Waypoint Exit"), UTF8_("Select the waypoint exit to edit."), mp_wp_exits_box);

    // new waypoint exit
    CEGUI::PushButton* button = static_cast<CEGUI::PushButton*>(wmgr.createWindow("TSCLook256/Button", "new_waypoint_exit"));
    button->setText(UTF8_("New Exit"));
    button->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&cWaypoint::Editor_Waypoint_New_Exit_Clicked, this));
    pWorld_Editor->Add_Config_Widget(UTF8_("New Exit"), UTF8_("Create a new exit by clicking."), button);

    // Remove waypoint exit
    button = static_cast<CEGUI::PushButton*>(wmgr.createWindow("TSCLook256/Button", "delete_waypoint_exit"));
    button->setText(UTF8_("Delete Exit"));
    button->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&cWaypoint::Editor_Waypoint_Delete_Exit_Clicked, this));
    pWorld_Editor->Add_Config_Widget(UTF8_("Delete Exit"), UTF8_("Delete the currently selected exit by clicking."), button);

    // Widgets for configuring a waypoint exit

    // waypoint exit direction
    mp_wp_exit_dir_box = static_cast<CEGUI::Combobox*>(wmgr.createWindow("TSCLook256/Combobox", "waypoint_exit_direction"));

    mp_wp_exit_dir_box->addItem(new CEGUI::ListboxTextItem("up"));
    mp_wp_exit_dir_box->addItem(new CEGUI::ListboxTextItem("down"));
    mp_wp_exit_dir_box->addItem(new CEGUI::ListboxTextItem("right"));
    mp_wp_exit_dir_box->addItem(new CEGUI::ListboxTextItem("left"));
    mp_wp_exit_dir_box->setText("up");

    mp_wp_exit_dir_box->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cWaypoint::Editor_Waypoint_Exit_Direction_Select, this));
    pWorld_Editor->Add_Config_Widget(UTF8_("Leave direction"), UTF8_("Direction key to press for leaving the waypoint for this exit"), mp_wp_exit_dir_box);

    // waypoint exit level exit name
    mp_wp_exit_name_edit = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TSCLook256/Editbox", "waypoint_exit_level_exit_name"));
    mp_wp_exit_name_edit->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cWaypoint::Editor_Waypoint_Exit_Level_Exit_Name_Changed, this));
    pWorld_Editor->Add_Config_Widget(UTF8_("Level Exit"), UTF8_("Name of the level exit in the level that activates this waypoint exit."), mp_wp_exit_name_edit);

    // waypoint exit start line point UID
    mp_wp_exit_uid_edit = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TSCLook256/Editbox", "waypoint_exit_line_start_uid"));
    mp_wp_exit_uid_edit->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cWaypoint::Editor_Waypoint_Exit_Line_Start_UID_Changed, this));
    pWorld_Editor->Add_Config_Widget(UTF8_("Line start UID"), UTF8_("UID of the line start point to set Alex onto when taking this exit."), mp_wp_exit_uid_edit);

    // waypoint exit locked
    mp_wp_exit_lock_box = static_cast<CEGUI::Combobox*>(wmgr.createWindow("TSCLook256/Combobox", "waypoint_locked"));

    mp_wp_exit_lock_box->addItem(new CEGUI::ListboxTextItem(UTF8_("Locked")));
    mp_wp_exit_lock_box->addItem(new CEGUI::ListboxTextItem(UTF8_("Unlocked")));
    mp_wp_exit_lock_box->setText(UTF8_("Locked"));

    mp_wp_exit_lock_box->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cWaypoint::Editor_Waypoint_Exit_Locked_Select, this));
    pWorld_Editor->Add_Config_Widget(UTF8_("Lock state"), UTF8_("Is this exit locked by default? PLEASE also set access to enabled on the target waypoint if you unlock by default!"), mp_wp_exit_lock_box);

    update_exit_widgets();

    // init
    Editor_Init();
}

bool cWaypoint::Editor_Type_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    if (item->getText().compare(UTF8_("Level")) == 0) {
        m_waypoint_type = WAYPOINT_NORMAL;
    }
    else {
        m_waypoint_type = WAYPOINT_WORLD_LINK;
    }

    return 1;

}
bool cWaypoint::Editor_Destination_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Destination(str_text);

    return 1;
}

bool cWaypoint::Editor_Backward_Direction_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    Set_Direction_Backward(Get_Direction_Id(item->getText().c_str()));

    return 1;
}

bool cWaypoint::Editor_Forward_Direction_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    Set_Direction_Forward(Get_Direction_Id(item->getText().c_str()));

    return 1;
}

bool cWaypoint::Editor_Access_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    if (item->getText().compare(UTF8_("Enabled")) == 0) {
        Set_Access(1, 1);
    }
    else {
        Set_Access(0, 1);
    }

    return 1;
}

bool cWaypoint::Editor_Waypoint_Exit_Select(const CEGUI::EventArgs& event)
{
    update_exit_widgets();
    return 1;
}

bool cWaypoint::Editor_Waypoint_New_Exit_Clicked(const CEGUI::EventArgs& event)
{
    waypoint_exit ex { DIR_UP, "", 0, true };
    m_exits.push_back(std::move(ex));

    rebuild_waypoint_exit_list();

    // Jump to the newly created waypoint exit
    mp_wp_exits_box->setText(int_to_string(m_exits.size()));
    update_exit_widgets();
    return 1;
}

bool cWaypoint::Editor_Waypoint_Delete_Exit_Clicked(const CEGUI::EventArgs& event)
{
    size_t index = string_to_int(mp_wp_exits_box->getText().c_str()); // string_to_int() returns 0 on texts like "(none)"

    // out of range
    if (index <= 0 || index > m_exits.size())
        return 1;

    // Convert from 1-based to 0-based
    index--;

    m_exits.erase(m_exits.begin() + index);
    rebuild_waypoint_exit_list();
    update_exit_widgets();

    return 1;
}

bool cWaypoint::Editor_Waypoint_Exit_Direction_Select(const CEGUI::EventArgs& event)
{
    waypoint_exit* p_exit = get_edited_waypoint_exit();
    if (!p_exit)
        return 1;

    p_exit->direction = Get_Direction_Id(mp_wp_exit_dir_box->getText().c_str());
    return 1;
}

bool cWaypoint::Editor_Waypoint_Exit_Level_Exit_Name_Changed(const CEGUI::EventArgs& event)
{
    waypoint_exit* p_exit = get_edited_waypoint_exit();
    if (!p_exit)
        return 1;

    p_exit->level_exit_name = mp_wp_exit_name_edit->getText().c_str();
    return 1;
}

bool cWaypoint::Editor_Waypoint_Exit_Line_Start_UID_Changed(const CEGUI::EventArgs& event)
{
    waypoint_exit* p_exit = get_edited_waypoint_exit();
    if (!p_exit)
        return 1;

    p_exit->line_start_uid = string_to_int(mp_wp_exit_uid_edit->getText().c_str());
    return 1;
}

bool cWaypoint::Editor_Waypoint_Exit_Locked_Select(const CEGUI::EventArgs& event)
{
    waypoint_exit* p_exit = get_edited_waypoint_exit();
    if (!p_exit)
        return 1;

    p_exit->locked = mp_wp_exit_lock_box->getItemIndex(mp_wp_exit_lock_box->getSelectedItem()) == 0;
    return 1;
}

void cWaypoint::rebuild_waypoint_exit_list()
{
    // Wipe all entries from the list
    mp_wp_exits_box->resetList();

    // Add dummy item plus real items found in m_exits
    mp_wp_exits_box->addItem(new CEGUI::ListboxTextItem(UTF8_("(None)")));
    for(size_t i=0; i < m_exits.size(); i++)
        mp_wp_exits_box->addItem(new CEGUI::ListboxTextItem(int_to_string(i+1)));
    mp_wp_exits_box->setText(UTF8_("(None)"));
}

void cWaypoint::update_exit_widgets()
{
    const waypoint_exit* p_exit = get_edited_waypoint_exit();

    if (p_exit) {
        mp_wp_exit_dir_box->setText(Get_Direction_Name(p_exit->direction));

        if (p_exit->locked)
            mp_wp_exit_lock_box->setText(UTF8_("Locked"));
        else
            mp_wp_exit_lock_box->setText(UTF8_("Unlocked"));

        mp_wp_exit_name_edit->setText(p_exit->level_exit_name);
        mp_wp_exit_uid_edit->setText(int_to_string(p_exit->line_start_uid));

        mp_wp_exit_dir_box->enable();
        mp_wp_exit_lock_box->enable();
        mp_wp_exit_name_edit->enable();
        mp_wp_exit_uid_edit->enable();
    }
    else {
        mp_wp_exit_dir_box->setText(Get_Direction_Name(DIR_UP));
        mp_wp_exit_lock_box->setText(UTF8_("Locked"));
        mp_wp_exit_name_edit->setText("");
        mp_wp_exit_uid_edit->setText("");

        mp_wp_exit_dir_box->disable();
        mp_wp_exit_lock_box->disable();
        mp_wp_exit_name_edit->disable();
        mp_wp_exit_uid_edit->disable();
    }
}

waypoint_exit* cWaypoint::get_edited_waypoint_exit()
{
    size_t index = string_to_int(mp_wp_exits_box->getText().c_str()); // string_to_int() returns 0 on texts like "(none)"
    if (index > 0 && index <= m_exits.size()) {
        return &m_exits[--index]; // Convert from 1-based to 0-based
    }
    else {
        return NULL;
    }
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

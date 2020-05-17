/***************************************************************************
 * path.h  -  movement along a defined path
 *
 * Copyright © 2008 - 2011 Florian Richter
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

#include "../objects/path.hpp"
#include "../core/game_core.hpp"
#include "../core/i18n.hpp"
#include "../core/xml_attributes.hpp"
#include "../video/renderer.hpp"
#include "../input/mouse.hpp"
#include "../user/savegame/savegame.hpp"
#include "../level/level.hpp"
#include "../core/sprite_manager.hpp"
#include "../enemies/static.hpp"
#include "../objects/moving_platform.hpp"
#include "../core/sprite_manager.hpp"
#include "../level/level.hpp"
#include "../level/level_settings.hpp"
#include "../core/editor/editor.hpp"
#include "../level/level_editor.hpp"

namespace TSC {

/* *** *** *** *** *** *** *** Path state class *** *** *** *** *** *** *** *** *** *** */

cPath_State::cPath_State(cSprite_Manager* sprite_manager)
{
    m_sprite_manager = sprite_manager;
    m_path = NULL;
    m_forward = 1;
    m_pos_x = 0;
    m_pos_y = 0;

    m_current_segment_pos = 0;
    m_current_segment = 0;
}

cPath_State::~cPath_State(void)
{
    // remove link
    if (m_path) {
        m_path->Remove_Link(this);
    }
}

void cPath_State::Load_From_Savegame(cSave_Level_Object* save_object)
{
    // path position
    if (save_object->exists("new_pos_x")) {
        m_pos_x = string_to_float(save_object->Get_Value("new_pos_x"));
    }
    if (save_object->exists("new_pos_y")) {
        m_pos_y = string_to_float(save_object->Get_Value("new_pos_y"));
    }
    // current segment
    if (save_object->exists("current_segment")) {
        m_current_segment = string_to_int(save_object->Get_Value("current_segment"));
    }
    if (save_object->exists("current_segment_pos")) {
        m_current_segment_pos = string_to_float(save_object->Get_Value("current_segment_pos"));
    }
    if (save_object->exists("forward")) {
        m_forward = string_to_int(save_object->Get_Value("forward")) > 0;
    }
}

bool cPath_State::Save_To_Savegame_XML_Node(xmlpp::Element* p_element) const
{
    // path position
    Add_Property(p_element, "new_pos_x", float_to_string(m_pos_x));
    Add_Property(p_element, "new_pos_y", float_to_string(m_pos_y));

    // current segment
    Add_Property(p_element, "current_segment", int_to_string(m_current_segment));

    // current segment position
    Add_Property(p_element, "current_segmant_pos", float_to_string(m_current_segment_pos));

    // forward
    Add_Property(p_element, "forward", int_to_string(static_cast<int>(m_forward)));

    return true;
}

void cPath_State::Set_Sprite_Manager(cSprite_Manager* sprite_manager)
{
    m_sprite_manager = sprite_manager;
}

void cPath_State::Draw(void)
{
    if (!m_path) {
        return;
    }

    pVideo->Draw_Rect(m_path->m_col_rect.m_x + m_pos_x - 4 - pActive_Camera->m_x, m_path->m_col_rect.m_y + m_pos_y - 4 - pActive_Camera->m_y, 8, 8, m_path->m_editor_pos_z + 0.00002f, &orange);
}

cPath* cPath_State::Get_Path_Object(const std::string& identifier)
{
    if (identifier.empty()) {
        return NULL;
    }

    // Search for path
    for (cSprite_List::iterator itr = m_sprite_manager->objects.begin(); itr != m_sprite_manager->objects.end(); ++itr) {
        cSprite* obj = (*itr);

        if (obj->m_type != TYPE_PATH || obj->m_auto_destroy) {
            continue;
        }

        cPath* path = static_cast<cPath*>(obj);

        // found
        if (path->m_identifier.compare(identifier) == 0) {
            return path;
        }
    }

    return NULL;
}

void cPath_State::Set_Path_Identifier(const std::string& path)
{
    // remove old link
    if (m_path) {
        m_path->Remove_Link(this);
    }

    // set path
    m_path_identifier = path;
    m_path = Get_Path_Object(m_path_identifier);

    // not found
    if (!m_path) {
        return;
    }

    // create link
    m_path->Create_Link(this);

    // set position to start
    Move_Reset();
}

void cPath_State::Path_Destroyed_Event(void)
{
    m_path = NULL;
}

void cPath_State::Move_Toggle(void)
{
    if (m_forward) {
        Move_Start_Backward();
    }
    else {
        Move_Start_Forward();
    }
}

void cPath_State::Move_Reset(void)
{
    if (m_forward) {
        Move_Start_Forward();
    }
    else {
        Move_Start_Backward();
    }
}

void cPath_State::Move_Reverse(void)
{
    m_forward = !m_forward;
}

void cPath_State::Move_Start_Forward(void)
{
    if (!m_path || m_path->m_segments.empty()) {
        return;
    }

    m_forward = 1;
    Move_From_Segment(0);
}

void cPath_State::Move_Start_Backward(void)
{
    if (!m_path || m_path->m_segments.empty()) {
        return;
    }

    m_forward = 0;
    Move_From_Segment(m_path->m_segments.size() - 1);
}

void cPath_State::Move_From_Segment(unsigned int segment)
{
    if (!m_path) {
        return;
    }

    // invalid segment
    if (segment >= m_path->m_segments.size()) {
        return;
    }

    m_current_segment = segment;

    if (m_forward) {
        m_current_segment_pos = 0;
        m_pos_x = m_path->m_segments[m_current_segment].m_x1;
        m_pos_y = m_path->m_segments[m_current_segment].m_y1;
    }
    // backward
    else {
        m_current_segment_pos = m_path->m_segments[m_current_segment].m_distance;
        m_pos_x = m_path->m_segments[m_current_segment].m_x2;
        m_pos_y = m_path->m_segments[m_current_segment].m_y2;
    }
}

bool cPath_State::Path_Move(float distance)
{
    if (!m_path) {
        return 0;
    }

    // invalid slot
    if (m_current_segment >= m_path->m_segments.size()) {
        return 0;
    }

    // get current segment object
    cPath_Segment obj = m_path->m_segments[m_current_segment];

    // walk forward
    if (m_forward) {
        while (1) {
            // how much is left
            float remaining = obj.m_distance - m_current_segment_pos;

            if (distance > remaining) {
                m_pos_x = obj.m_x2;
                m_pos_y = obj.m_y2;

                // finished
                if (m_current_segment + 1 >= m_path->m_segments.size()) {
                    // rewind
                    if (m_path->m_rewind) {
                        m_current_segment = 0;
                        m_current_segment_pos = 0;
                        return 0;
                    }
                    // mirror
                    else {
                        return 0;
                    }
                }

                // set current segment object
                m_current_segment++;
                obj = m_path->m_segments[m_current_segment];

                m_current_segment_pos = 0;
                distance -= remaining;
            }
            else {
                m_current_segment_pos += distance;
                m_pos_x = obj.m_x1 + obj.m_ux * m_current_segment_pos;
                m_pos_y = obj.m_y1 + obj.m_uy * m_current_segment_pos;

                return 1;
            }
        }
    }
    // walk backward
    else {
        while (1) {
            // how much is left
            float remaining = m_current_segment_pos;

            if (distance > remaining) {
                m_pos_x = obj.m_x1;
                m_pos_y = obj.m_y1;

                // finished
                if (m_current_segment == 0) {
                    // rewind
                    if (m_path->m_rewind) {
                        m_current_segment = m_path->m_segments.size() - 1;
                        obj = m_path->m_segments[m_current_segment];
                        m_current_segment_pos = obj.m_distance;
                        return 0;
                    }
                    // mirror
                    else {
                        return 0;
                    }
                }

                // set current segment object
                m_current_segment--;
                obj = m_path->m_segments[m_current_segment];

                m_current_segment_pos = obj.m_distance;
                distance -= remaining;
            }
            else {
                m_current_segment_pos -= distance;
                m_pos_x = obj.m_x1 + obj.m_ux * m_current_segment_pos;
                m_pos_y = obj.m_y1 + obj.m_uy * m_current_segment_pos;

                return 1;
            }
        }
    }

    return 0;
}

/* *** *** *** *** *** *** cPath_Segment *** *** *** *** *** *** *** *** *** *** *** */

cPath_Segment::cPath_Segment(void)
{
    m_x1 = 0;
    m_y1 = 0;
    m_x2 = 0;
    m_y2 = 0;

    m_ux = 0;
    m_uy = 0;
    m_distance = 0;
}

cPath_Segment::~cPath_Segment(void)
{

}

void cPath_Segment::Set_Pos(float x1, float y1, float x2, float y2)
{
    m_x1 = x1;
    m_y1 = y1;
    m_x2 = x2;
    m_y2 = y2;

    Update();
}

void cPath_Segment::Set_Pos_Start(float x1, float y1)
{
    m_x1 = x1;
    m_y1 = y1;

    Update();
}

void cPath_Segment::Set_Pos_End(float x2, float y2)
{
    m_x2 = x2;
    m_y2 = y2;

    Update();
}

void cPath_Segment::Set_Pos_Start_X(float x1)
{
    m_x1 = x1;

    Update();
}

void cPath_Segment::Set_Pos_Start_Y(float y1)
{
    m_y1 = y1;

    Update();
}

void cPath_Segment::Set_Pos_End_X(float x2)
{
    m_x2 = x2;

    Update();
}

void cPath_Segment::Set_Pos_End_Y(float y2)
{
    m_y2 = y2;

    Update();
}

void cPath_Segment::Update(void)
{
    float dx = m_x2 - m_x1;
    float dy = m_y2 - m_y1;
    // distance
    m_distance = sqrt(dx * dx + dy * dy);
    // unit vector
    if (m_distance != 0) {
        m_ux = dx / m_distance;
        m_uy = dy / m_distance;
    }
}

/* *** *** *** *** *** *** *** Path class *** *** *** *** *** *** *** *** *** *** */

cPath::cPath(cSprite_Manager* sprite_manager)
    : cSprite(sprite_manager, "path")
{
    // Set defaults
    cPath::Init();
}

cPath::cPath(XmlAttributes& attributes, cSprite_Manager* sprite_manager)
    : cSprite(sprite_manager, "path")
{
    cPath::Init();

    m_segments.clear();

    // position
    Set_Pos(string_to_float(attributes["posx"]), string_to_float(attributes["posy"]), true);

    // identifier
    Set_Identifier(attributes["identifier"]);

    // show line
    Set_Show_Line(attributes.fetch<bool>("show_line", m_show_line));

    // rewind
    Set_Rewind(attributes.fetch<bool>("rewind", m_rewind));

    // load segments
    unsigned int count = 0;
    while (true) {
        std::string str_pos = int_to_string(count);

        // next line not available
        if (!attributes.exists("segment_" + str_pos + "_x1"))
            break;

        cPath_Segment obj;

        obj.Set_Pos(string_to_float(attributes["segment_" + str_pos + "_x1"]),
                    string_to_float(attributes["segment_" + str_pos + "_y1"]),
                    string_to_float(attributes["segment_" + str_pos + "_x2"]),
                    string_to_float(attributes["segment_" + str_pos + "_y2"]));

        m_segments.push_back(obj);
        count++;
    }
}

cPath::~cPath(void)
{
    Remove_Links();
}

void cPath::Init(void)
{
    m_sprite_array = ARRAY_ACTIVE;
    m_type = TYPE_PATH;
    m_massive_type = MASS_PASSIVE;
    m_editor_pos_z = 0.11f;
    m_show_line = false;

    m_name = _("Path");

    // size
    m_rect.m_w = 10;
    m_rect.m_h = 10;
    m_col_rect.m_w = m_rect.m_w;
    m_col_rect.m_h = m_rect.m_h;
    m_start_rect.m_w = m_rect.m_w;
    m_start_rect.m_h = m_rect.m_h;

    m_rewind = 0;
    m_editor_color = Color(static_cast<uint8_t>(100), 150, 200, 128);
    m_editor_selected_segment = 0;

    mp_x1_box      = NULL;
    mp_x2_box      = NULL;
    mp_y1_box      = NULL;
    mp_y2_box      = NULL;
    mp_segment_box = NULL;
}

cPath* cPath::Copy(void) const
{
    cPath* path = new cPath(m_sprite_manager);
    path->Set_Pos(m_start_pos_x, m_start_pos_y, 1);
    path->m_segments = m_segments;
    path->Set_Identifier(m_identifier);
    path->Set_Rewind(m_rewind);
    return path;
}

std::string cPath::Get_XML_Type_Name()
{
    return "";
}

xmlpp::Element* cPath::Save_To_XML_Node(xmlpp::Element* p_element)
{
    xmlpp::Element* p_node = cSprite::Save_To_XML_Node(p_element);

    // Attributes
    Add_Property(p_node, "identifier", m_identifier);
    Add_Property(p_node, "show_line", m_show_line);
    Add_Property(p_node, "rewind", m_rewind);

    // segments
    // This is ugly XML because TSC's loader does not support nested
    // XML tags currently.
    for (unsigned int i=0; i < m_segments.size(); i++) {
        std::string str_pos = int_to_string(i);

        Add_Property(p_node, "segment_" + str_pos + "_x1", m_segments[i].m_x1);
        Add_Property(p_node, "segment_" + str_pos + "_y1", m_segments[i].m_y1);
        Add_Property(p_node, "segment_" + str_pos + "_x2", m_segments[i].m_x2);
        Add_Property(p_node, "segment_" + str_pos + "_y2", m_segments[i].m_y2);
    }

    return p_node;
}

void cPath::Load_From_Savegame(cSave_Level_Object* save_object)
{

}

void cPath::Set_Identifier(const std::string& identifier)
{
    m_identifier = identifier;

    // remove linked objects
    Remove_Links();

    if (m_identifier.empty()) {
        return;
    }

    /* search for linked objects
     * needed to update the links
    */
    for (cSprite_List::iterator itr = pActive_Level->m_sprite_manager->objects.begin(); itr != pActive_Level->m_sprite_manager->objects.end(); ++itr) {
        cSprite* obj = (*itr);

        if (obj->m_auto_destroy) {
            continue;
        }

        if (obj->m_type == TYPE_STATIC_ENEMY) {
            cStaticEnemy* static_enemy = static_cast<cStaticEnemy*>(obj);

            // found
            if (static_enemy->m_path_state.m_path_identifier.compare(m_identifier) == 0) {
                // link to me
                static_enemy->Init_Links();
                //static_enemy->m_path_state.Set_Path_Identifier( m_identifier );
            }
        }
        else if (obj->m_type == TYPE_MOVING_PLATFORM) {
            cMoving_Platform* moving_platform = static_cast<cMoving_Platform*>(obj);

            // found
            if (moving_platform->m_path_state.m_path_identifier.compare(m_identifier) == 0) {
                // link to me
                moving_platform->Init_Links();
                //moving_platform->m_path_state.Set_Path_Identifier( m_identifier );
            }
        }
    }
}

void cPath::Set_Show_Line(bool show)
{
    m_show_line = show;
}

void cPath::Set_Rewind(bool rewind)
{
    // already set
    if (m_rewind == rewind) {
        return;
    }

    m_rewind = rewind;

    for (PathStateList::iterator itr = m_linked_path_states.begin(); itr != m_linked_path_states.end(); ++itr) {
        cPath_State* obj = (*itr);

        // reset backwards moving
        if (!obj->m_forward) {
            obj->Move_From_Segment(obj->m_current_segment);
        }
    }
}

void cPath::Create_Link(cPath_State* path_state)
{
    if (!path_state) {
        return;
    }

    m_linked_path_states.push_back(path_state);
}

void cPath::Remove_Link(cPath_State* path_state)
{
    if (!path_state) {
        return;
    }

    // get iterator
    PathStateList::iterator itr = std::find(m_linked_path_states.begin(), m_linked_path_states.end(), path_state);

    // if available
    if (itr != m_linked_path_states.end()) {
        // erase
        m_linked_path_states.erase(itr);
    }
}

void cPath::Remove_Links(void)
{
    for (PathStateList::iterator itr = m_linked_path_states.begin(); itr != m_linked_path_states.end(); ++itr) {
        cPath_State* obj = (*itr);

        obj->Path_Destroyed_Event();
    }
}

void cPath::Update(void)
{
    if (!m_valid_update) {
        return;
    }
}

void cPath::Draw(cSurface_Request* request /* = NULL */)
{
    if (!m_valid_draw) {
        return;
    }

    // Draw the color rect (the path’s clickable area in the editor).
    // When ingame path drawing has been requested, don’t do so as it
    // doesn’t belong to the real path.
    if (!m_show_line || editor_enabled || game_debug)
        pVideo->Draw_Rect(m_col_rect.m_x - pActive_Camera->m_x, m_col_rect.m_y - pActive_Camera->m_y, m_col_rect.m_w, m_col_rect.m_h, m_editor_pos_z, &m_editor_color);

    // draw segments
    int count = 0;
    for (PathList::iterator itr = m_segments.begin(); itr != m_segments.end(); ++itr) {
        cPath_Segment obj = (*itr);

        Color line_color = m_editor_color;

        if (count % 2 != 1) {
            line_color.blue = 255;
        }

        // create request
        cLine_Request* line_request = new cLine_Request();
        pVideo->Draw_Line(m_col_rect.m_x + obj.m_x1 - pActive_Camera->m_x, m_col_rect.m_y + obj.m_y1 - pActive_Camera->m_y, m_col_rect.m_x + obj.m_x2 - pActive_Camera->m_x, m_col_rect.m_y + obj.m_y2 - pActive_Camera->m_y, m_editor_pos_z + 0.00001f, &line_color, line_request);
        line_request->m_line_width = 2;
        // add request
        pRenderer->Add(line_request);

        count++;
    }
}

bool cPath::Is_Draw_Valid(void)
{
    // if editor not enabled, line drawing hasn’t been requested
    // manually and debug mode is not active
    if (!editor_enabled && !m_show_line && !game_debug) {
        return 0;
    }

    // if not active on the screen or not mouse object
    if (!m_active || (!Is_Visible_On_Screen() && pMouseCursor->m_active_object != this)) {
        return 0;
    }

    return 1;
}

void cPath::Editor_Activate(void)
{
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    // identifier
    CEGUI::Editbox* editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TSCLook256/Editbox", "path_identifier"));
    pLevel_Editor->Add_Config_Widget(UTF8_("Identifier"), UTF8_("Identifier name of the Path. This is needed for objects that can link to this."), editbox);

    editbox->setText(m_identifier.c_str());
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cPath::Editor_Identifier_Text_Changed, this));

    // show path moving line
    CEGUI::Combobox* combobox_line = static_cast<CEGUI::Combobox*>(wmgr.createWindow("TSCLook256/Combobox", "path_show_line"));

    combobox_line->addItem(new CEGUI::ListboxTextItem("show"));
    combobox_line->addItem(new CEGUI::ListboxTextItem("hide"));

    if (m_show_line)
        combobox_line->setText("show");
    else
        combobox_line->setText("hide");

    combobox_line->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cPath::Editor_Show_Line_Select, this));
    pLevel_Editor->Add_Config_Widget(UTF8_("Show Line"), UTF8_("Wether or not to show the path's moving line ingame."), combobox_line);

    // move type
    CEGUI::Combobox* combobox = static_cast<CEGUI::Combobox*>(wmgr.createWindow("TSCLook256/Combobox", "path_move_type"));

    combobox->addItem(new CEGUI::ListboxTextItem("mirror"));
    combobox->addItem(new CEGUI::ListboxTextItem("rewind"));

    if (m_rewind) {
        combobox->setText("rewind");
    }
    else {
        combobox->setText("mirror");
    }

    combobox->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cPath::Editor_Move_Type_Select, this));
    pLevel_Editor->Add_Config_Widget(UTF8_("Move Type"), UTF8_("Movement type. Mirror moves forth and back and rewind starts from the beginning again."), combobox);

    // selected segment
    mp_segment_box = static_cast<CEGUI::Combobox*>(wmgr.createWindow("TSCLook256/Combobox", "path_selected_segment"));

    for (unsigned int count = 0; count < m_segments.size(); count++) {
        mp_segment_box->addItem(new CEGUI::ListboxTextItem(int_to_string(count)));
    }

    mp_segment_box->setText(int_to_string(m_editor_selected_segment));

    mp_segment_box->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cPath::Editor_Selected_Segment_Select, this));
    pLevel_Editor->Add_Config_Widget(UTF8_("Segment"), UTF8_("Select the Segment to edit."), mp_segment_box);

    // button add
    CEGUI::PushButton* pushbutton = static_cast<CEGUI::PushButton*>(wmgr.createWindow("TSCLook256/Button", "path_segment_add"));
    pLevel_Editor->Add_Config_Widget("-", UTF8_("Add a Segment after the selected one."), pushbutton);

    pushbutton->setText(UTF8_("Add"));
    pushbutton->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&cPath::Editor_Button_Add_Segment_Clicked, this));

    // button delete
    pushbutton = static_cast<CEGUI::PushButton*>(wmgr.createWindow("TSCLook256/Button", "path_segment_delete"));
    pLevel_Editor->Add_Config_Widget("-", UTF8_("Delete the selected Segment."), pushbutton);

    pushbutton->setText(UTF8_("Delete"));
    pushbutton->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&cPath::Editor_Button_Delete_Segment_Clicked, this));

    // selected segment x1
    mp_x1_box = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TSCLook256/Editbox", "path_segment_x1"));
    pLevel_Editor->Add_Config_Widget(UTF8_("Pos X1"), UTF8_("Line position X1"), mp_x1_box);

    mp_x1_box->setValidationString("[-+]?[0-9]*\\.?[0-9]*");
    mp_x1_box->setText(int_to_string(static_cast<int>(m_segments[m_editor_selected_segment].m_x1)));
    mp_x1_box->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cPath::Editor_Pos_X1_Text_Changed, this));

    // selected segment y1
    mp_y1_box = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TSCLook256/Editbox", "path_segment_y1"));
    pLevel_Editor->Add_Config_Widget(UTF8_("Y1"), UTF8_("Line position Y1"), mp_y1_box);

    mp_y1_box->setValidationString("[-+]?[0-9]*\\.?[0-9]*");
    mp_y1_box->setText(int_to_string(static_cast<int>(m_segments[m_editor_selected_segment].m_y1)));
    mp_y1_box->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cPath::Editor_Pos_Y1_Text_Changed, this));

    // selected segment x2
    mp_x2_box = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TSCLook256/Editbox", "path_segment_x2"));
    pLevel_Editor->Add_Config_Widget(UTF8_("Pos X2"), UTF8_("Line position X2"), mp_x2_box);

    mp_x2_box->setValidationString("[-+]?[0-9]*\\.?[0-9]*");
    mp_x2_box->setText(int_to_string(static_cast<int>(m_segments[m_editor_selected_segment].m_x2)));
    mp_x2_box->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cPath::Editor_Pos_X2_Text_Changed, this));

    // selected segment y2
    mp_y2_box = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TSCLook256/Editbox", "path_segment_y2"));
    pLevel_Editor->Add_Config_Widget(UTF8_("Y2"), UTF8_("Line position Y2"), mp_y2_box);

    mp_y2_box->setValidationString("[-+]?[0-9]*\\.?[0-9]*");
    mp_y2_box->setText(int_to_string(static_cast<int>(m_segments[m_editor_selected_segment].m_y2)));
    mp_y2_box->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cPath::Editor_Pos_Y2_Text_Changed, this));

    // init
    Editor_Init();
}

void cPath::Editor_Deactivate(void)
{
    cSprite::Editor_Deactivate();

    // Clear pointers for debugging sanetity
    mp_x1_box      = NULL;
    mp_x2_box      = NULL;
    mp_y1_box      = NULL;
    mp_y2_box      = NULL;
    mp_segment_box = NULL;
}

void cPath::Editor_State_Update(void)
{
    // selected segment
    mp_segment_box->resetList();

    for (unsigned int count = 0; count < m_segments.size(); count++) {
        mp_segment_box->addItem(new CEGUI::ListboxTextItem(int_to_string(count)));
    }

    mp_segment_box->setText(int_to_string(m_editor_selected_segment));

    // Set selected segment values
    // x1
    mp_x1_box->setText(float_to_string(m_segments[m_editor_selected_segment].m_x1, 6, 0));
    // y1
    mp_y1_box->setText(float_to_string(m_segments[m_editor_selected_segment].m_y1, 6, 0));
    // x2
    mp_x2_box->setText(float_to_string(m_segments[m_editor_selected_segment].m_x2, 6, 0));
    // y2
    mp_y2_box->setText(float_to_string(m_segments[m_editor_selected_segment].m_y2, 6, 0));

    // do not allow to change the start point position
    if (m_editor_selected_segment == 0) {
        mp_x1_box->setEnabled(0);
        mp_y1_box->setEnabled(0);
    }
    else {
        mp_x1_box->setEnabled(1);
        mp_y1_box->setEnabled(1);
    }
}

bool cPath::Editor_Identifier_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Identifier(str_text);

    return 1;
}

bool cPath::Editor_Show_Line_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    if (item->getText() == "show")
        Set_Show_Line(true);
    else
        Set_Show_Line(false);

    return true;
}

bool cPath::Editor_Move_Type_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();
    std::string str_text = item->getText().c_str();

    if (str_text.compare("mirror") == 0) {
        Set_Rewind(0);
    }
    else {
        Set_Rewind(1);
    }

    return 1;
}

bool cPath::Editor_Selected_Segment_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();
    m_editor_selected_segment = string_to_int(item->getText().c_str());

    // invalid selected segment
    if (m_editor_selected_segment >= m_segments.size()) {
        m_editor_selected_segment = m_segments.size() - 1;
    }

    Editor_State_Update();

    return 1;
}

bool cPath::Editor_Button_Add_Segment_Clicked(const CEGUI::EventArgs& event)
{
    cPath_Segment new_segment = m_segments[m_editor_selected_segment];
    new_segment.Set_Pos(new_segment.m_x2, new_segment.m_y2, new_segment.m_x2 + 20, new_segment.m_y2 - 20);
    m_segments.insert(m_segments.begin() + m_editor_selected_segment + 1, new_segment);

    m_editor_selected_segment++;
    Editor_State_Update();

    return 1;
}

bool cPath::Editor_Button_Delete_Segment_Clicked(const CEGUI::EventArgs& event)
{
    // always keep one
    if (m_segments.size() == 1) {
        return 1;
    }

    m_segments.erase(m_segments.begin() + m_editor_selected_segment);

    for (PathStateList::iterator itr = m_linked_path_states.begin(); itr != m_linked_path_states.end(); ++itr) {
        cPath_State* obj = (*itr);

        if (m_editor_selected_segment == obj->m_current_segment) {
            if (obj->m_current_segment > 0) {
                obj->m_current_segment--;
            }

            obj->Move_From_Segment(obj->m_current_segment);
        }
    }

    m_editor_selected_segment--;
    Editor_State_Update();

    return 1;
}

bool cPath::Editor_Pos_X1_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    m_segments[m_editor_selected_segment].Set_Pos_Start_X(string_to_float(str_text));

    Editor_Segment_Pos_Changed();

    return 1;
}

bool cPath::Editor_Pos_Y1_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    m_segments[m_editor_selected_segment].Set_Pos_Start_Y(string_to_float(str_text));

    Editor_Segment_Pos_Changed();

    return 1;
}

bool cPath::Editor_Pos_X2_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    m_segments[m_editor_selected_segment].Set_Pos_End_X(string_to_float(str_text));

    Editor_Segment_Pos_Changed();

    return 1;
}

bool cPath::Editor_Pos_Y2_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    m_segments[m_editor_selected_segment].Set_Pos_End_Y(string_to_float(str_text));

    Editor_Segment_Pos_Changed();

    return 1;
}

void cPath::Editor_Segment_Pos_Changed(void)
{
    for (PathStateList::iterator itr = m_linked_path_states.begin(); itr != m_linked_path_states.end(); ++itr) {
        cPath_State* obj = (*itr);

        if (m_editor_selected_segment == obj->m_current_segment) {
            obj->Move_From_Segment(m_editor_selected_segment);
        }
    }
}

void cPath::Add_Segment(cPath_Segment segment)
{
    m_segments.push_back(segment);
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

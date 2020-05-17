/***************************************************************************
 * mouse.cpp  -  mouse handling class
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
#include "../input/mouse.hpp"
#include "../input/keyboard.hpp"
#include "../core/game_core.hpp"
#include "../level/level_settings.hpp"
#include "../scene/scene.hpp"
#include "../core/sprite_manager.hpp"
#include "../level/level_editor.hpp"
#include "../overworld/world_editor.hpp"
#include "../overworld/overworld.hpp"
#include "../gui/menu.hpp"
#include "../level/level.hpp"
#include "../user/preferences.hpp"
#include "../core/framerate.hpp"
#include "../objects/goldpiece.hpp"
#include "../objects/level_exit.hpp"
#include "../objects/level_entry.hpp"
#include "../objects/secret_area.hpp"
#include "../objects/bonusbox.hpp"
#include "../enemies/furball.hpp"
#include "../video/renderer.hpp"
#include "../core/i18n.hpp"

using namespace std;

namespace TSC {

/* *** *** *** *** *** cSelectedObject *** *** *** *** *** *** *** *** *** *** *** *** */

cSelectedObject::cSelectedObject(void)
{
    m_obj = NULL;

    m_mouse_offset_y = 0;
    m_mouse_offset_x = 0;

    m_user = 0;
}

cSelectedObject::~cSelectedObject(void)
{

}

/* *** *** *** *** *** cCopyObject *** *** *** *** *** *** *** *** *** *** *** *** */

cCopyObject::cCopyObject(void)
{
    m_obj = NULL;
}

cCopyObject::~cCopyObject(void)
{
    if (m_obj) {
        delete m_obj;
    }
}

/* *** *** *** *** *** cMouseCursor *** *** *** *** *** *** *** *** *** *** *** *** */

cMouseCursor::cMouseCursor(cSprite_Manager* sprite_manager)
    : cMovingSprite(sprite_manager)
{
    m_type = TYPE_MOUSECURSOR;

    m_click_counter = 0.0f;

    m_selection_mode = 0;
    m_selection_rect = GL_rect(0, 0, 0, 0);

    m_hovering_object = new cSelectedObject();
    m_active_object = NULL;

    m_snap_to_object_mode = 0;
    m_snap_pos_available = 0;
    m_snap_pos = GL_point();

    m_fastcopy_mode = 0;
    m_mover_mode = 0;
    m_last_clicked_object = NULL;

    m_dragmode = DRAGMODE_SELECTION;

    Reset_Keys();
    Update_Position();
    // disable mouse initially
    Set_Active(0);
}

cMouseCursor::~cMouseCursor(void)
{
    Clear_Copy_Objects();
    Clear_Selected_Objects();
    delete m_hovering_object;
}

void cMouseCursor::Set_Active(bool enabled)
{
    cMovingSprite::Set_Active(enabled);
    CEGUI::System::getSingleton().getDefaultGUIContext().getMouseCursor().setVisible(enabled);
}

void cMouseCursor::Reset(bool clear_copy_buffer /* = 1 */)
{
    // only clear copy buffer if requested
    if (clear_copy_buffer) {
        Clear_Copy_Objects();
    }
    Clear_Selected_Objects();
    Clear_Hovered_Object();
    Clear_Active_Object();

    // change to default cursor
    if (m_mover_mode) {
        CEGUI::System::getSingleton().getDefaultGUIContext().getMouseCursor().setImage("TSCLook256/MouseArrow");
    }

    m_mover_mode = 0;
    m_selection_mode = 0;
    m_fastcopy_mode = 0;

    Clear_Collisions();

    // show mouse
    if (editor_enabled) {
        Set_Active(1);
    }
}

bool cMouseCursor::Handle_Event(const sf::Event& ev)
{
    switch (ev.type) {
    case sf::Event::MouseMoved: {
        CEGUI::System::getSingleton().getDefaultGUIContext().injectMousePosition(static_cast<float>(ev.mouseMove.x), static_cast<float>(ev.mouseMove.y));
        Update_Position();

        if (Handle_Mouse_Move(ev)) {
            // processed
            return 1;
        }
        break;
    }
    case sf::Event::MouseButtonReleased: {
        if (Handle_Mouse_Up(ev.mouseButton.button)) {
            // processed
            return 1;
        }

        break;
    }
    case sf::Event::MouseButtonPressed: {
        if (Handle_Mouse_Down(ev.mouseButton.button)) {
            // processed
            return 1;
        }

        break;
    }
    case sf::Event::MouseWheelScrolled: {
        if (ev.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel &&
            Handle_Mouse_Wheel(ev.mouseWheelScroll.delta)) {
            // processed
            return 1;
        }
    }
    default: {
        break;
    }
    }

    return 0;
}

bool cMouseCursor::Handle_Mouse_Down(sf::Mouse::Button button)
{
    CEGUI::GUIContext& gui_context = CEGUI::System::getSingleton().getDefaultGUIContext();

    // First submit the event to CEGUI for processing
    switch (button) {
    // mouse buttons
    case sf::Mouse::Left: {
        if (gui_context.injectMouseButtonDown(CEGUI::LeftButton)) {
            return 1;
        }
        m_left = 1;
        break;
    }
    case sf::Mouse::Middle: {
        if (gui_context.injectMouseButtonDown(CEGUI::MiddleButton)) {
            return 1;
        }
        m_middle = 1;
        break;
    }
    case sf::Mouse::Right: {
        if (gui_context.injectMouseButtonDown(CEGUI::RightButton)) {
            return 1;
        }
        m_right = 1;
        break;
    }
    default: {
        break;
    }
    }

    // if CEGUI didn't handle it, forward to the current mode
    if (Game_Mode == MODE_LEVEL) {
        // processed by the level
        if (pActive_Level->Mouse_Down(button)) {
            return 1;
        }
    }
    else if (Game_Mode == MODE_OVERWORLD) {
        // processed by the overworld
        if (pActive_Overworld->Mouse_Down(button)) {
            return 1;
        }
    }
    else if (Game_Mode == MODE_MENU) {
        // processed by the menu
        if (pMenuCore->Mouse_Down(button)) {
            return 1;
        }
    }
    else if (Game_Mode == MODE_SCENE) {
        // processed by the active scene
        if (pActive_Scene->Mouse_Down(button)) {
            return 1;
        }
    }

    return 0;
}

bool cMouseCursor::Handle_Mouse_Wheel(float delta)
{
    // mouse wheel
    if (CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseWheelChange(delta))
        return true;
    else
        return false;
}

bool cMouseCursor::Handle_Mouse_Up(sf::Mouse::Button button)
{
    CEGUI::GUIContext& gui_context = CEGUI::System::getSingleton().getDefaultGUIContext();

    // First submit the button release to CEGUI
    switch (button) {
    case sf::Mouse::Left: {
        m_left = 0;
        if (gui_context.injectMouseButtonUp(CEGUI::LeftButton)) {
            return 1;
        }
    }
    break;
    case sf::Mouse::Middle: {
        m_middle = 0;
        if (gui_context.injectMouseButtonUp(CEGUI::MiddleButton)) {
            return 1;
        }
    }
    break;
    case sf::Mouse::Right: {
        m_right = 0;
        if (gui_context.injectMouseButtonUp(CEGUI::RightButton)) {
            return 1;
        }
    }
    break;
    default: {
        break;
    }
    }

    // if CEGUI didn't want it, forward button to the current mode
    if (Game_Mode == MODE_LEVEL) {
        // processed by the level
        if (pActive_Level->Mouse_Up(button)) {
            return 1;
        }
    }
    else if (Game_Mode == MODE_OVERWORLD) {
        // processed by the overworld
        if (pActive_Overworld->Mouse_Up(button)) {
            return 1;
        }
    }
    else if (Game_Mode == MODE_MENU) {
        // processed by the menu
        if (pMenuCore->Mouse_Up(button)) {
            return 1;
        }
    }
    else if (Game_Mode == MODE_SCENE) {
        // processed by the active scene
        if (pActive_Scene->Mouse_Up(button)) {
            return 1;
        }
    }

    return 0;
}

bool cMouseCursor::Handle_Mouse_Move(const sf::Event& evt)
{
    if (Game_Mode == MODE_LEVEL && pLevel_Editor->m_enabled) {
        if (pLevel_Editor->Mouse_Move(evt)) {
            return true;
        }
    }

    if (Game_Mode == MODE_OVERWORLD && pWorld_Editor->m_enabled) {
        if (pWorld_Editor->Mouse_Move(evt)) {
            return true;
        }
    }

    return false;
}

cObjectCollision* cMouseCursor::Get_First_Editor_Collsion(float px /* = 0.0f */, float py /* = 0.0f */)
{
    if (m_mover_mode) {
        return NULL;
    }

    // Get CEGUI Window containing the mouse
    CEGUI::Window* mouse_window = CEGUI::System::getSingleton().getDefaultGUIContext().getWindowContainingMouse();

    // if mouse is over a blocking CEGUI window
    if (mouse_window && !mouse_window->isMousePassThroughEnabled()) {
        return NULL;
    }

    // mouse rect
    GL_rect mouse_rect;

    // use given posx
    if (!Is_Float_Equal(px, 0.0f)) {
        mouse_rect.m_x = px;
    }
    else {
        mouse_rect.m_x = m_pos_x;
    }

    // use given posy
    if (!Is_Float_Equal(py, 0.0f)) {
        mouse_rect.m_y = py;
    }
    else {
        mouse_rect.m_y = m_pos_y;
    }

    mouse_rect.m_w = 1.0f;
    mouse_rect.m_h = 1.0f;

    // no collisions
    return Get_First_Mouse_Collision(mouse_rect);
}

cObjectCollision* cMouseCursor::Get_First_Mouse_Collision(const GL_rect& mouse_rect)
{
    cSprite_List sprite_objects;
    m_sprite_manager->Get_Objects_sorted(sprite_objects, 1, 1);

    // check objects
    for (cSprite_List::reverse_iterator itr = sprite_objects.rbegin(); itr != sprite_objects.rend(); ++itr) {
        cSprite* obj = (*itr);

        // ignore spawned or destroyed objects
        if (obj->m_spawned || obj->m_auto_destroy) {
            continue;
        }

        // Always match against the start position rect (and not the
        // current rect), because that is where the object is drawn in
        // the editor and placed on initial level start.
        if (mouse_rect.Intersects(obj->m_start_rect)) {
            return Create_Collision_Object(this, obj, COL_VTYPE_INTERNAL);
        }
    }

    return NULL;
}

void cMouseCursor::Update(void)
{
    // only if editor is enabled
    if (!editor_enabled) {
        return;
    }

    Update_Doubleclick();
}

void cMouseCursor::Draw(void)
{
    // only if editor is enabled
    if (!editor_enabled) {
        return;
    }

    Update_Selection();

    // if in Level or Overworld
    if (Game_Mode == MODE_LEVEL || Game_Mode == MODE_OVERWORLD) {
        // draw if not in mover mode
        if (!m_mover_mode) {
            Draw_Object_Rects();
        }
    }
}

void cMouseCursor::Update_Position(void)
{
    if (!m_mover_mode) {
        sf::Vector2i curpos = sf::Mouse::getPosition(*pVideo->mp_window);
        // scale to the virtual game size
        m_x = static_cast<int>(static_cast<float>(curpos.x) * global_downscalex);
        m_y = static_cast<int>(static_cast<float>(curpos.y) * global_downscaley);
    }

    if (editor_enabled) {
        if (!m_mover_mode) {
            Set_Pos(m_x + pActive_Camera->m_x, m_y + pActive_Camera->m_y, 1);
        }

        Update_Hovered_Object();
        Update_Selected_Objects();
        Update_Snap_Pos();
    }
    else {
        Set_Pos(static_cast<float>(m_x), static_cast<float>(m_y), 1);
    }
}

void cMouseCursor::Update_Doubleclick(void)
{
    if (m_click_counter) {
        m_click_counter -= pFramerate->m_speed_factor;

        if (m_click_counter < 0.0f) {
            m_click_counter = 0.0f;
        }
    }
}

void cMouseCursor::Left_Click_Down(void)
{
    if (m_mover_mode) {
        return;
    }

    // if no hovering object
    if (!m_hovering_object->m_obj) {
        // if shift is pressed
        if (pKeyboard->Is_Shift_Down() && !pKeyboard->Is_Ctrl_Down()) {
            cObjectCollision* col = Get_First_Editor_Collsion();

            // collision with an object
            if (col) {
                // add object
                if (!Add_Selected_Object(col->m_obj, 1)) {
                    // if already added remove it
                    Remove_Selected_Object(col->m_obj);
                }

                delete col;
            }
        }

        Start_Selection();
        m_click_counter = 0.0f;
        return;
    }
    else {
        if (!Is_Selected_Object(m_hovering_object->m_obj, 1)) {
            Clear_Selected_Objects();
            Add_Selected_Object(m_hovering_object->m_obj, 1);
        }

        // select same object types
        if (pKeyboard->Is_Ctrl_Down() && pKeyboard->Is_Shift_Down()) {
            if (Game_Mode == MODE_LEVEL) {
                pLevel_Editor->Select_Same_Object_Types(m_hovering_object->m_obj);
            }
            else if (Game_Mode == MODE_OVERWORLD) {
                pWorld_Editor->Select_Same_Object_Types(m_hovering_object->m_obj);
            }
        }

        Update_Snap_Pos();
    }

    if (m_click_counter > 0.0f) {
        // if last clicked object was the same
        if (m_last_clicked_object == m_hovering_object->m_obj) {
            Double_Click();
        }
        else {
            m_click_counter = 0.0f;
        }
    }
    else {
        // double click counter
        m_click_counter = speedfactor_fps * 0.3f;
        // save last clicked object
        m_last_clicked_object = m_hovering_object->m_obj;
    }
}

void cMouseCursor::Double_Click(bool activate /* = 1 */)
{
    Clear_Active_Object();

    // add new
    if (activate) {
        Set_Active_Object(m_hovering_object->m_obj);
    }

    m_click_counter = 0.0f;
}

void cMouseCursor::Set_Hovered_Object(cSprite* sprite)
{
    // return if mouse object is the same or in mouse selection mode
    if (m_hovering_object->m_obj == sprite || (sprite && m_selection_mode)) {
        return;
    }

    // remove old m_hovering_object from selected objects
    if (m_hovering_object->m_obj) {
        Remove_Selected_Object(m_hovering_object->m_obj, 1);
    }

    // set new mouse object
    m_hovering_object->m_obj = sprite;

    if (sprite) {
        // add new mouse object to selected objects
        Add_Selected_Object(sprite);
    }

    Update_Selected_Object_Offset(m_hovering_object);
}

void cMouseCursor::Update_Hovered_Object(void)
{
    if (!editor_enabled || !m_hovering_object->m_obj || (m_mover_mode && (Game_Mode == MODE_LEVEL || Game_Mode == MODE_OVERWORLD))) {
        return;
    }

    if (m_left) {
        if (!Is_Selected_Object(m_hovering_object->m_obj, 1)) {
            Set_Object_Position(m_hovering_object);
        }
    }
    else {
        Update_Selected_Object_Offset(m_hovering_object);
    }
}

void cMouseCursor::Add_Copy_Object(cSprite* sprite)
{
    if (!sprite) {
        return;
    }

    // check if not already added
    for (CopyObjectList::iterator itr = m_copy_objects.begin(); itr != m_copy_objects.end(); ++itr) {
        if ((*itr)->m_obj == sprite) {
            return;
        }
    }

    cSprite* copy = sprite->Copy();

    // can't be copied
    if (!copy) {
        return;
    }

    // insert object
    cCopyObject* copy_object = new cCopyObject();
    copy_object->m_obj = copy;
    m_copy_objects.push_back(copy_object);
}

void cMouseCursor::Add_Copy_Objects(cSprite_List& spritelist)
{
    if (spritelist.empty()) {
        return;
    }

    // insert all objects
    for (cSprite_List::iterator itr = spritelist.begin(); itr != spritelist.end(); ++itr) {
        cSprite* obj = (*itr);

        Add_Copy_Object(obj);
    }
}

bool cMouseCursor::Remove_Copy_Object(const cSprite* sprite)
{
    if (!sprite) {
        return 0;
    }

    for (CopyObjectList::iterator itr = m_copy_objects.begin(); itr != m_copy_objects.end(); ++itr) {
        cCopyObject* copy_obj = (*itr);

        if (copy_obj->m_obj == sprite) {
            m_copy_objects.erase(itr);
            delete copy_obj;

            return 1;
        }
    }

    return 0;
}

void cMouseCursor::Clear_Copy_Objects(void)
{
    for (CopyObjectList::iterator itr = m_copy_objects.begin(); itr != m_copy_objects.end(); ++itr) {
        delete *itr;
    }

    m_copy_objects.clear();
}

GL_Vector cMouseCursor::Get_Copy_Object_Base(float px, float py)
{
    GL_Vector vec = GL_Vector();

    if (m_copy_objects.empty()) {
        return vec;
    }

    // set a base
    float base_x = m_copy_objects[0]->m_obj->m_pos_x;
    float base_y = m_copy_objects[0]->m_obj->m_pos_y;

    for (CopyObjectList::iterator itr = m_copy_objects.begin(); itr != m_copy_objects.end(); ++itr) {
        cCopyObject* copy_obj = (*itr);

        // if more in top or left
        if (copy_obj->m_obj->m_pos_x < base_x || copy_obj->m_obj->m_pos_y < base_y) {
            base_x = copy_obj->m_obj->m_pos_x;
            base_y = copy_obj->m_obj->m_pos_y;
        }
    }

    vec.x = px - base_x;
    vec.y = py - base_y;

    return vec;
}

void cMouseCursor::Paste_Copy_Objects(float px, float py)
{
    if (m_copy_objects.empty()) {
        return;
    }

    cSprite_List new_objects;

    for (CopyObjectList::iterator itr = m_copy_objects.begin(); itr != m_copy_objects.end(); ++itr) {
        cCopyObject* copy_obj = (*itr);

        // set base vector
        GL_Vector base_vec = Get_Copy_Object_Base(copy_obj->m_obj->m_pos_x, copy_obj->m_obj->m_pos_y);

        // copy
        cSprite* new_object = Copy(copy_obj->m_obj, px + base_vec.x, py + base_vec.y);

        if (new_object) {
            new_objects.push_back(new_object);
        }
    }

    if (!m_copy_objects.empty()) {
        m_hovering_object->m_mouse_offset_y = static_cast<int>(m_copy_objects[0]->m_obj->m_col_rect.m_h / 2);
        m_hovering_object->m_mouse_offset_x = static_cast<int>(m_copy_objects[0]->m_obj->m_col_rect.m_w / 2);
    }

    // pasted objects are selected
    Clear_Selected_Objects();
    Add_Selected_Objects(new_objects, 1);
}

bool cMouseCursor::Add_Selected_Object(cSprite* sprite, bool from_user /* = 0 */)
{
    if (!sprite) {
        return 0;
    }

    // check if not already added
    for (SelectedObjectList::iterator itr = m_selected_objects.begin(); itr != m_selected_objects.end(); ++itr) {
        cSelectedObject* sel_obj = (*itr);

        if (sel_obj->m_obj == sprite) {
            // overwrite user if given
            if (from_user && !sel_obj->m_user) {
                sel_obj->m_user = 1;
                return 1;
            }

            return 0;
        }
    }

    // insert object
    cSelectedObject* selected_object = new cSelectedObject();
    selected_object->m_obj = sprite;
    selected_object->m_user = from_user;
    m_selected_objects.push_back(selected_object);

    Update_Selected_Object_Offset(selected_object);

    return 1;
}

void cMouseCursor::Add_Selected_Objects(cSprite_List& spritelist, bool from_user /* = 0 */)
{
    if (spritelist.empty()) {
        return;
    }

    // insert all objects
    for (cSprite_List::iterator itr = spritelist.begin(); itr != spritelist.end(); ++itr) {
        cSprite* obj = (*itr);

        Add_Selected_Object(obj, from_user);
    }
}

bool cMouseCursor::Remove_Selected_Object(const cSprite* sprite, bool no_user /* = 0 */)
{
    if (!sprite) {
        return 0;
    }

    for (SelectedObjectList::iterator itr = m_selected_objects.begin(); itr != m_selected_objects.end(); ++itr) {
        cSelectedObject* sel_obj = (*itr);

        if (sel_obj->m_obj == sprite) {
            // don't delete user added selected object
            if (no_user && sel_obj->m_user) {
                return 0;
            }

            m_selected_objects.erase(itr);
            delete sel_obj;

            return 1;
        }
    }

    return 0;
}

cSprite_List cMouseCursor::Get_Selected_Objects(void)
{
    cSprite_List spritelist;

    for (SelectedObjectList::iterator itr = m_selected_objects.begin(); itr != m_selected_objects.end(); ++itr) {
        cSelectedObject* object = (*itr);

        spritelist.push_back(object->m_obj);
    }

    return spritelist;
}

GL_rect cMouseCursor::Get_Selected_Objects_Rect(void)
{
    if (m_selected_objects.empty()) {
        return GL_rect();
    }

    // set dimension of first object
    GL_rect sel_rect = m_selected_objects[0]->m_obj->m_start_rect;

    for (SelectedObjectList::iterator itr = m_selected_objects.begin(); itr != m_selected_objects.end(); ++itr) {
        cSelectedObject* obj = (*itr);

        // shortcut to the rect
        const GL_rect obj_rect = obj->m_obj->m_start_rect;

        // update left-most position and width
        if (sel_rect.m_x > obj_rect.m_x) {
            sel_rect.m_w = (sel_rect.m_x + sel_rect.m_w) - obj_rect.m_x;
            sel_rect.m_x = obj_rect.m_x;
        }
        // update width
        if (sel_rect.m_x + sel_rect.m_w < obj_rect.m_x + obj_rect.m_w) {
            sel_rect.m_w = (obj_rect.m_x + obj_rect.m_w) - sel_rect.m_x;
        }
        // update top-most position and height
        if (sel_rect.m_y > obj_rect.m_y) {
            sel_rect.m_h = (sel_rect.m_y + sel_rect.m_h) - obj_rect.m_y;
            sel_rect.m_y = obj_rect.m_y;
        }
        // update height
        if (sel_rect.m_y + sel_rect.m_h < obj_rect.m_y + obj_rect.m_h) {
            sel_rect.m_h = (obj_rect.m_y + obj_rect.m_h) - sel_rect.m_y;
        }
    }

    return sel_rect;
}

void cMouseCursor::Clear_Selected_Objects(void)
{
    for (SelectedObjectList::iterator itr = m_selected_objects.begin(); itr != m_selected_objects.end(); ++itr) {
        delete *itr;
    }

    m_selected_objects.clear();
}

void cMouseCursor::Update_Selected_Objects(void)
{
    if (!editor_enabled || (m_mover_mode && (Game_Mode == MODE_LEVEL || Game_Mode == MODE_OVERWORLD))) {
        return;
    }

    // update selected objects position with the mouse offset
    for (SelectedObjectList::iterator itr = m_selected_objects.begin(); itr != m_selected_objects.end(); ++itr) {
        cSelectedObject* sel_obj = (*itr);

        if (!m_left || m_selection_mode) {
            Update_Selected_Object_Offset(sel_obj);
        }
    }

    // if a user selected object is not active
    if (!Is_Selected_Object(m_hovering_object->m_obj, 1)) {
        return;
    }

    for (SelectedObjectList::iterator itr = m_selected_objects.begin(); itr != m_selected_objects.end(); ++itr) {
        cSelectedObject* sel_obj = (*itr);

        // left mouse is pressed
        if (m_left) {
            Set_Object_Position(sel_obj);
        }
    }
}

void cMouseCursor::Update_Selected_Object_Offset(cSelectedObject* obj)
{
    if (!obj) {
        return;
    }

    if (!obj->m_obj) {
        obj->m_mouse_offset_x = 0;
        obj->m_mouse_offset_y = 0;
        return;
    }

    obj->m_mouse_offset_x = static_cast<int>(m_pos_x) - static_cast<int>(obj->m_obj->m_start_pos_x);
    obj->m_mouse_offset_y = static_cast<int>(m_pos_y) - static_cast<int>(obj->m_obj->m_start_pos_y);
}

bool cMouseCursor::Is_Selected_Object(const cSprite* sprite, bool only_user /* = 0 */)
{
    if (!sprite) {
        return 0;
    }

    for (SelectedObjectList::iterator itr = m_selected_objects.begin(); itr != m_selected_objects.end(); ++itr) {
        cSelectedObject* sel_obj = (*itr);

        if (sel_obj->m_obj == sprite) {
            // if only user objects
            if (only_user && !sel_obj->m_user) {
                continue;
            }

            // found
            return 1;
        }
    }

    return 0;
}

void cMouseCursor::Delete_Selected_Objects(void)
{
    for (int i = m_selected_objects.size() - 1; i >= 0; i--) {
        Delete(m_selected_objects[i]->m_obj);
    }

    Clear_Selected_Objects();
}

bool cMouseCursor::Get_Snap_Pos(GL_point& new_pos, int snap, cSelectedObject* src_obj)
{
    GL_rect src_rect;
    GL_rect full_snap_rect;
    GL_point multiselect_offset;

    // set bounding box for multiple objects
    if (m_selected_objects.size() > 1) {
        src_rect = Get_Selected_Objects_Rect();
        full_snap_rect = GL_rect(src_rect.m_x - snap, src_rect.m_y - snap, src_rect.m_w + snap * 2, src_rect.m_h + snap * 2);

        // offset between selected object and bounding box
        multiselect_offset.m_x = src_obj->m_obj->m_start_rect.m_x - src_rect.m_x;
        multiselect_offset.m_y = src_obj->m_obj->m_start_rect.m_y - src_rect.m_y;

        // update the top-left co-ord of the new bounding box using the mouse position
        src_rect.m_x = (m_pos_x - src_obj->m_mouse_offset_x) - multiselect_offset.m_x;
        src_rect.m_y = (m_pos_y - src_obj->m_mouse_offset_y) - multiselect_offset.m_y;
    }
    else {
        src_rect = GL_rect(m_pos_x - src_obj->m_mouse_offset_x, m_pos_y - src_obj->m_mouse_offset_y, src_obj->m_obj->m_start_rect.m_w, src_obj->m_obj->m_start_rect.m_h);
        full_snap_rect = GL_rect(src_obj->m_obj->m_start_rect.m_x - snap, src_obj->m_obj->m_start_rect.m_y - snap, src_obj->m_obj->m_start_rect.m_w + snap * 2, src_obj->m_obj->m_start_rect.m_h + snap * 2);
    }

    float distance_left = snap + 1;
    float distance_right = snap + 1;
    float distance_top = snap + 1;
    float distance_bottom = snap + 1;
    float distance_best = snap + 1;
    float distance_best_edge = snap + 1;

    // counter for number of snapable objects
    int num_snap_obj = 0;
    cSprite* snap_obj = NULL;

    // check objects for overlap
    for (cSprite_List::iterator itr = m_sprite_manager->objects.begin(); itr != m_sprite_manager->objects.end(); ++itr) {
        cSprite* obj = (*itr);

        // don't check selected objects
        if (Is_Selected_Object(obj, 0)) {
            continue;
        }

        // ignore spawned or destroyed objects
        if (obj->m_spawned || obj->m_auto_destroy) {
            continue;
        }

        // ignore enemies
        if (obj->m_sprite_array == ARRAY_ENEMY) {
            continue;
        }

        if (full_snap_rect.Intersects(obj->m_start_rect)) {
            const float obj_distance_left = fabs(src_rect.m_x - (obj->m_start_rect.m_x + obj->m_start_rect.m_w));
            const float obj_distance_right = fabs((src_rect.m_x + src_rect.m_w) - obj->m_start_rect.m_x);
            const float obj_distance_top = fabs(src_rect.m_y - (obj->m_start_rect.m_y + obj->m_start_rect.m_h));
            const float obj_distance_bottom = fabs((src_rect.m_y + src_rect.m_h) - obj->m_start_rect.m_y);
            float obj_distance_best;

            if (obj_distance_left < obj_distance_right) {
                obj_distance_best = obj_distance_left;
            }
            else {
                obj_distance_best = obj_distance_right;
            }

            if (obj_distance_top < obj_distance_best) {
                obj_distance_best = obj_distance_top;
            }

            if (obj_distance_bottom < obj_distance_best) {
                obj_distance_best = obj_distance_bottom;
            }

            // found a better snap object
            if (obj_distance_best <= distance_best) {
                // -- second round of testing --
                // update count of snapable objects
                num_snap_obj++;

                const float obj_distance_left_to_left = fabs(src_rect.m_x - obj->m_start_rect.m_x);
                const float obj_distance_right_to_right = fabs((src_rect.m_x + src_rect.m_w) - (obj->m_start_rect.m_x + obj->m_start_rect.m_w));
                const float obj_distance_top_to_top = fabs(src_rect.m_y - obj->m_start_rect.m_y);
                const float obj_distance_bottom_to_bottom = fabs((src_rect.m_y + src_rect.m_h) - (obj->m_start_rect.m_y + obj->m_start_rect.m_h));
                float obj_distance_best_edge;

                // look for closest edge
                if (obj_distance_left_to_left < obj_distance_right_to_right) {
                    obj_distance_best_edge = obj_distance_left_to_left;
                }
                else {
                    obj_distance_best_edge = obj_distance_right_to_right;
                }
                if (obj_distance_top_to_top < distance_best_edge) {
                    obj_distance_best_edge = obj_distance_top_to_top;
                }
                if (obj_distance_bottom_to_bottom < obj_distance_best_edge) {
                    obj_distance_best_edge = obj_distance_bottom_to_bottom;
                }

                // found a better edge
                if (obj_distance_best_edge < distance_best_edge) {
                    distance_best_edge = obj_distance_best_edge;
                }
                // if not the better edge and more than one snapable object available
                else if (num_snap_obj > 1) {
                    continue;
                }

                snap_obj = obj;
                distance_left = obj_distance_left;
                distance_right = obj_distance_right;
                distance_top = obj_distance_top;
                distance_bottom = obj_distance_bottom;
                distance_best = obj_distance_best;
            }
        }
    }

    // if no object for snapping found
    if (!snap_obj) {
        return 0;
    }

    const GL_rect snap_obj_rect = snap_obj->m_start_rect;

    // snap to colliding object
    // vertical
    if ((distance_top < distance_left && distance_top < distance_right) || (distance_bottom < distance_left && distance_bottom < distance_right)) {
        // out of snap range
        if (distance_top > snap && distance_bottom > snap) {
            new_pos.m_y = src_rect.m_y;
        }
        // snap bottom to top edge
        else if (distance_top > distance_bottom) {
            new_pos.m_y = snap_obj_rect.m_y - src_rect.m_h;
        }
        // snap top to bottom edge
        else {
            new_pos.m_y = snap_obj_rect.m_y + snap_obj_rect.m_h;
        }

        const float distance_left_to_left = fabs(src_rect.m_x - snap_obj_rect.m_x);
        const float distance_right_to_right = fabs((src_rect.m_x + src_rect.m_w) - (snap_obj_rect.m_x + snap_obj_rect.m_w));

        // out of snap range
        if (distance_left_to_left > snap && distance_right_to_right > snap) {
            if (distance_left > snap && distance_right > snap) {
                new_pos.m_x = src_rect.m_x;
            }
            // snap left corner to right corner
            else if (distance_left < distance_right) {
                new_pos.m_x = snap_obj_rect.m_x + snap_obj_rect.m_w;
            }
            // snap right corner to left corner
            else {
                new_pos.m_x = snap_obj_rect.m_x - src_rect.m_w;
            }
        }
        // snap left to left edge
        else if (distance_left_to_left < distance_right_to_right) {
            new_pos.m_x = snap_obj_rect.m_x;
        }
        // snap right to right edge
        else {
            new_pos.m_x = snap_obj_rect.m_x + snap_obj_rect.m_w - src_rect.m_w;
        }
    }
    // horizontal
    else {
        // out of snap range
        if (distance_left > snap && distance_right > snap) {
            new_pos.m_x = src_rect.m_x;
        }
        // snap right to left edge
        else if (distance_left > distance_right) {
            new_pos.m_x = snap_obj_rect.m_x - src_rect.m_w;
        }
        // snap left to right edge
        else {
            new_pos.m_x = snap_obj_rect.m_x + snap_obj_rect.m_w;
        }

        const float distance_top_to_top = fabs(src_rect.m_y - snap_obj_rect.m_y);
        const float distance_bottom_to_bottom = fabs((src_rect.m_y + src_rect.m_h) - (snap_obj_rect.m_y + snap_obj_rect.m_h));

        // out of snap range
        if (distance_top_to_top > snap && distance_bottom_to_bottom > snap) {
            if (distance_top > snap && distance_bottom > snap) {
                new_pos.m_y = src_rect.m_y;
            }
            // snap top corner to bottom corner
            else if (distance_top < distance_bottom) {
                new_pos.m_y = snap_obj_rect.m_y + snap_obj_rect.m_h;
            }
            // snap bottom corner to top corner
            else {
                new_pos.m_y = snap_obj_rect.m_y - src_rect.m_h;
            }
        }
        // snap top to top edge
        else if (distance_top_to_top < distance_bottom_to_bottom) {
            new_pos.m_y = snap_obj_rect.m_y;
        }
        // snap bottom to bottom edge
        else {
            new_pos.m_y = snap_obj_rect.m_y + snap_obj_rect.m_h - src_rect.m_h;
        }
    }

    new_pos.m_x += src_obj->m_mouse_offset_x + multiselect_offset.m_x;
    new_pos.m_y += src_obj->m_mouse_offset_y + multiselect_offset.m_y;

    return 1;
}

void cMouseCursor::Update_Snap_Pos(void)
{
    if (m_snap_to_object_mode && m_left && m_hovering_object->m_obj) {
        m_snap_pos_available = Get_Snap_Pos(m_snap_pos, 10, m_hovering_object);
    }
}

void cMouseCursor::Toggle_Snap_Mode(void)
{
    m_snap_to_object_mode = !m_snap_to_object_mode;

    if (m_snap_to_object_mode) {
        gp_hud->Set_Text(_("Snap to object mode enabled"));
    }
    else {
        gp_hud->Set_Text(_("Snap to object mode disabled"));
    }
}

void cMouseCursor::Set_Active_Object(cSprite* sprite)
{
    // clear existing
    Clear_Active_Object();

    if (sprite) {
        m_active_object = sprite;
        m_active_object->Editor_Activate();
    }
}

void cMouseCursor::Clear_Active_Object(void)
{
    if (!m_active_object) {
        return;
    }

    m_active_object->Editor_Deactivate();

    m_active_object = NULL;
}

cSprite* cMouseCursor::Copy(const cSprite* copy_object, float px, float py) const
{
    if (!copy_object) {
        return NULL;
    }

    // only copy in editor mode
    if (!editor_enabled) {
        cerr << "Warning: No editor enabled for copy object: " << copy_object->Create_Name() << endl;
        return NULL;
    }

    // if invalid
    if (!copy_object->Is_Sprite_Managed()) {
        cerr << "Warning : cMouseCursor copy : invalid object array : " << copy_object->m_sprite_array << endl;
        return NULL;
    }

    // copy
    cSprite* new_sprite = copy_object->Copy();

    // failed to copy
    if (!new_sprite) {
        debug_print("Warning : Mouse object copying failed for %s\n", copy_object->Create_Name().c_str());
        return NULL;
    }

    // set sprite manager
    new_sprite->Set_Sprite_Manager(m_sprite_manager);
    // set position
    new_sprite->Set_Pos(px, py, 1);
    // add it
    m_sprite_manager->Add(new_sprite);

    return new_sprite;
}

void cMouseCursor::Delete(cSprite* sprite)
{
    // if invalid
    if (!sprite || !sprite->Is_Sprite_Managed()) {
        return;
    }

    // if this can not be auto-deleted
    if (sprite->m_disallow_managed_delete) {
        return;
    }

    // deactivate active object
    if (m_active_object == sprite) {
        Double_Click(0);
    }

    // remove if current player active item object
    if (pLevel_Player->m_active_object == sprite) {
        pLevel_Player->m_active_object = NULL;
    }

    // remove selected object
    Remove_Selected_Object(sprite);
    // remove copy object
    Remove_Copy_Object(sprite);

    // delete object
    if (editor_enabled) {
        sprite->Destroy();
    }
}

void cMouseCursor::Set_Object_Position(cSelectedObject* sel_obj)
{
    // if in snap mode and snap available
    if (m_snap_to_object_mode && m_snap_pos_available) {
        sel_obj->m_obj->Set_Pos(m_snap_pos.m_x - sel_obj->m_mouse_offset_x, m_snap_pos.m_y - sel_obj->m_mouse_offset_y, 1);
    }
    else {
        // set new position
        sel_obj->m_obj->Set_Pos(static_cast<float>(static_cast<int>(m_pos_x) - sel_obj->m_mouse_offset_x), static_cast<float>(static_cast<int>(m_pos_y) - sel_obj->m_mouse_offset_y), 1);
    }
}

void cMouseCursor::Draw_Object_Rects(void)
{
    // current hover rect
    GL_rect hover_rect;
    // current object color
    Color obj_color = white;

    if (!m_left && m_hovering_object->m_obj) {
        hover_rect.m_x = m_hovering_object->m_obj->m_start_pos_x - pActive_Camera->m_x;
        hover_rect.m_y = m_hovering_object->m_obj->m_start_pos_y - pActive_Camera->m_y;
        hover_rect.m_w = m_hovering_object->m_obj->m_start_rect.m_w;
        hover_rect.m_h = m_hovering_object->m_obj->m_start_rect.m_h;

        // get object color
        obj_color = Get_Massive_Type_Color(m_hovering_object->m_obj->m_massive_type);

        // create request
        cRect_Request* request = new cRect_Request();
        pVideo->Draw_Rect(&hover_rect, 0.6f, &obj_color, request);

        if (m_fastcopy_mode) {
            request->m_color.alpha = 64;
        }
        // not fastcopy
        else {
            // not filled
            request->m_filled = 0;
            request->m_line_width = 2.0f;
        }

        // add request
        pRenderer->Add(request);
    }


    // draw selected objects if not left mouse is pressed, shift selection or mouse selection
    if (!m_left || (pKeyboard->Is_Shift_Down() && !pKeyboard->Is_Ctrl_Down()) || m_selection_mode) {
        obj_color = white;

        for (SelectedObjectList::iterator itr = m_selected_objects.begin(); itr != m_selected_objects.end(); ++itr) {
            cSprite* object = (*itr)->m_obj;

            // check if visible on screen or mouse object
            if (!object->Is_Visible_On_Screen() || object == m_hovering_object->m_obj) {
                continue;
            }

            hover_rect.m_x = object->m_start_pos_x - pActive_Camera->m_x;
            hover_rect.m_y = object->m_start_pos_y - pActive_Camera->m_y;
            hover_rect.m_w = object->m_start_rect.m_w;
            hover_rect.m_h = object->m_start_rect.m_h;

            // object color
            obj_color = Get_Massive_Type_Color(object->m_massive_type);

            // z position
            float pos_z = 0.5f;

            // different z position for other massive type
            if (object->m_massive_type == MASS_HALFMASSIVE) {
                pos_z += 0.001f;
            }
            else if (object->m_massive_type == MASS_MASSIVE) {
                pos_z += 0.002f;
            }
            else if (object->m_massive_type == MASS_CLIMBABLE) {
                pos_z += 0.003f;
            }

            // create request
            cRect_Request* rect_request = new cRect_Request();
            pVideo->Draw_Rect(&hover_rect, pos_z, &obj_color, rect_request);

            rect_request->m_line_width = 2.0f;
            // not filled
            rect_request->m_filled = 0;
            // with stipple
            rect_request->m_stipple_pattern = 0xAAAA;

            // add request
            pRenderer->Add(rect_request);
        }
    }

    // draw bounding box if multiple objects snapped at once
    if (m_left && m_snap_to_object_mode && m_selected_objects.size() > 1) {
        // create request
        cRect_Request* rect_request = new cRect_Request();
        GL_rect sel_rect = Get_Selected_Objects_Rect();
        pVideo->Draw_Rect(sel_rect.m_x - pActive_Camera->m_x, sel_rect.m_y - pActive_Camera->m_y, sel_rect.m_w, sel_rect.m_h, 0.51f, &lightgrey, rect_request);

        // not filled
        rect_request->m_filled = 0;

        // add request
        pRenderer->Add(rect_request);
    }
}

void cMouseCursor::Start_Selection(void)
{
    Clear_Hovered_Object();
    m_selection_mode = 1;
    m_selection_rect.m_x = m_x + pActive_Camera->m_x;
    m_selection_rect.m_y = m_y + pActive_Camera->m_y;
}

void cMouseCursor::End_Selection(void)
{
    if (m_selection_mode) {
        m_selection_mode = 0;

        if (m_dragmode == DRAGMODE_SECRETAREA && Game_Mode == MODE_LEVEL) {
            // Insert secret area into the level
            cSecret_Area* secarea = new cSecret_Area(pActive_Level->m_sprite_manager);
            secarea->Set_Rect(m_normalized_selection_rect);
            pActive_Level->m_sprite_manager->Add(secarea);

            // Attach it to the cursor for positioning
            Add_Selected_Object(secarea, 1);
        }
    }

    Update_Position();
}

void cMouseCursor::Update_Selection(void)
{
    if (!m_selection_mode) {
        return;
    }

    m_selection_rect.m_w = m_x - m_selection_rect.m_x + pActive_Camera->m_x;
    m_selection_rect.m_h = m_y - m_selection_rect.m_y + pActive_Camera->m_y;

    // create collision checking rect
    GL_rect rect = GL_rect(m_selection_rect.m_x, m_selection_rect.m_y, 0.0f, 0.0f);

    // negative to positive
    if (m_selection_rect.m_w < 0.0f) {
        rect.m_x = m_selection_rect.m_x + m_selection_rect.m_w;
        rect.m_w = -m_selection_rect.m_w;
    }
    // positive
    else {
        rect.m_w = m_selection_rect.m_w;
    }

    // negative to positive
    if (m_selection_rect.m_h < 0.0f) {
        rect.m_y = m_selection_rect.m_y + m_selection_rect.m_h;
        rect.m_h = -m_selection_rect.m_h;
    }
    // positive
    else {
        rect.m_h = m_selection_rect.m_h;
    }

    // if rect is below minimum
    if (rect.m_w < 2.0f && rect.m_h < 2.0f) {
        return;
    }

    // Preserve result
    m_normalized_selection_rect = rect;

    if (m_dragmode == DRAGMODE_SELECTION) {
        // only clear if not shift is pressed
        if (!(pKeyboard->Is_Shift_Down() && !pKeyboard->Is_Ctrl_Down())) {
            Clear_Selected_Objects();
        }

        // add selected objects
        for (cSprite_List::iterator itr = m_sprite_manager->objects.begin(); itr != m_sprite_manager->objects.end(); ++itr) {
            cSprite* obj = (*itr);

            // don't check spawned/destroyed objects
            if (obj->m_spawned || obj->m_auto_destroy) {
                continue;
            }

            // if it doesn't collide with the rect
            if (!rect.Intersects(obj->m_rect)) {
                continue;
            }

            Add_Selected_Object(obj, 1);
        }

        if (rect.Intersects(pActive_Player->m_rect)) {
            Add_Selected_Object(pActive_Player, 1);
        }
    }

    // ## inner rect
    cRect_Request* rect_request = new cRect_Request();
    pVideo->Draw_Rect(m_selection_rect.m_x - pActive_Camera->m_x + 0.5f, m_selection_rect.m_y - pActive_Camera->m_y + 0.5f, m_selection_rect.m_w, m_selection_rect.m_h, 0.509f, &white, rect_request);

    // not filled
    rect_request->m_filled = 0;
    // color
    rect_request->m_color.alpha = 128;

    // add request
    pRenderer->Add(rect_request);

    // ## outer rect
    // create request
    rect_request = new cRect_Request();
    pVideo->Draw_Rect(m_selection_rect.m_x - pActive_Camera->m_x, m_selection_rect.m_y - pActive_Camera->m_y, m_selection_rect.m_w, m_selection_rect.m_h, 0.51f, &lightblue, rect_request);

    // not filled
    rect_request->m_filled = 0;

    // add request
    pRenderer->Add(rect_request);
}

void cMouseCursor::Toggle_Mover_Mode(void)
{
    m_mover_mode = !m_mover_mode;

    m_mover_center_x = m_x * global_upscalex;
    m_mover_center_y = m_y * global_upscaley;

    if (m_mover_mode) {
        CEGUI::System::getSingleton().getDefaultGUIContext().getMouseCursor().setImage("TSCLook256/MouseMoveCursor");
    }
    else {
        CEGUI::System::getSingleton().getDefaultGUIContext().getMouseCursor().setImage("TSCLook256/MouseArrow");
    }
}

void cMouseCursor::Mover_Update(int move_x, int move_y)
{
    if (!m_mover_mode) {
        return;
    }

    // mouse moves the camera
    pActive_Camera->Move(move_x - m_mover_center_x, move_y - m_mover_center_y);

    // keep mouse at it's position
    sf::Vector2i center(m_mover_center_x, m_mover_center_y);
    sf::Mouse::setPosition(center, *pVideo->mp_window);

    sf::Event inEvent;

    while (pVideo->PollEvent(inEvent)) {
        switch (inEvent.type) {
        case sf::Event::MouseButtonPressed: {
            if (inEvent.mouseButton.button == sf::Mouse::Middle) {
                m_mover_mode = 0;
            }
            break;
        }
        case sf::Event::KeyPressed: {
            m_mover_mode = 0;
            pKeyboard->Key_Down(inEvent);
            break;
        }
        case sf::Event::Closed: {
            game_exit = 1;
            break;
        }
        default: {
            // ignore
            break;
        }
        }
    }
}

void cMouseCursor::Set_Drag_Mode(DragMode mode)
{
    m_dragmode = mode;
}

void cMouseCursor::Editor_Update(void)
{
    if (!editor_enabled) {
        return;
    }

    cObjectCollision* col = Get_First_Editor_Collsion();

    // if no collision
    if (!col) {
        if (!m_left) {
            if (m_hovering_object->m_obj) {
                Clear_Hovered_Object();
            }
        }
        else {
            // no mouse object
            if (!m_hovering_object->m_obj) {
                // clear active object
                Double_Click(0);

                // clear selected objects if no shift is pressed
                if (!(pKeyboard->Is_Shift_Down() && !pKeyboard->Is_Ctrl_Down())) {
                    Clear_Selected_Objects();
                }
            }
        }

        return;
    }

    // set object data
    if (col->m_obj) {
        if ((!m_left || !m_hovering_object->m_obj) && !(pKeyboard->Is_Shift_Down() && !pKeyboard->Is_Ctrl_Down())) {
            Set_Hovered_Object(col->m_obj);
        }
    }

    delete col;
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

cMouseCursor* pMouseCursor = NULL;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

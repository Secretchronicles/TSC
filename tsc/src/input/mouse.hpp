/***************************************************************************
 * mouse.h
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

#ifndef TSC_MOUSE_HPP
#define TSC_MOUSE_HPP

#include "../core/global_basic.hpp"
#include "../objects/movingsprite.hpp"
#include "../core/math/rect.hpp"
#include "../core/math/vector.hpp"

namespace TSC {

    /**
     * Determines what happens when the user leftclicks and starts
     * dragging.
     */
    enum DragMode {
        DRAGMODE_SELECTION, //< Select objects.
        DRAGMODE_SECRETAREA //< Draw a secret area,
    };

    /* *** *** *** *** *** *** cSelectedObject *** *** *** *** *** *** *** *** *** *** *** */

    class cSelectedObject {
    public:
        cSelectedObject(void);
        ~cSelectedObject(void);

        // base object
        cSprite* m_obj;
        // additional selection mouse position
        int m_mouse_offset_x;
        int m_mouse_offset_y;

        // if user controlled
        bool m_user;
    };

    typedef vector<cSelectedObject*> SelectedObjectList;

    /* *** *** *** *** *** *** cCopyObject *** *** *** *** *** *** *** *** *** *** *** */

    class cCopyObject {
    public:
        cCopyObject(void);
        ~cCopyObject(void);

        // base object
        cSprite* m_obj;
    };

    typedef vector<cCopyObject*> CopyObjectList;

    /* *** *** *** *** *** *** cMouseCursor *** *** *** *** *** *** *** *** *** *** *** */

    class cMouseCursor : public cMovingSprite {
    public:
        cMouseCursor(cSprite_Manager* sprite_manager);
        virtual ~cMouseCursor(void);

        // Set active/visible
        virtual void Set_Active(bool enabled);
        // resets editor features
        void Reset(bool clear_copy_buffer = 1);
        // only reset buttons
        inline void Reset_Keys(void)
        {
            m_left = 0;
            m_right = 0;
            m_middle = 0;
        }

        // Handles the Mouse events
        bool Handle_Event(const sf::Event& ev);
        /* GUI handle mouse down function
         * Returns true if GUI handled the event
        */
        bool Handle_Mouse_Down(sf::Mouse::Button button);
        /* GUI handle mouse up function
         * Returns true if GUI handled the event
        */
        bool Handle_Mouse_Up(sf::Mouse::Button button);
        /* GUI handle mouse wheel function
         * Returns true if GUI handled the event
         */
        bool Handle_Mouse_Wheel(float delta);
        /* GUI handle mouse move function
         * Returns true if GUI handled the event
         */
        bool Handle_Mouse_Move(const sf::Event& evt);

        /* Set what happens when the user leftclicks and
         * drags.
         */
        void Set_Drag_Mode(DragMode mode);

        /* Get the first object colliding with the mouse which is valid for the editor
         * returns the collision if an object was found else NULL
         * The collision data should be deleted if not used anymore
        */
        cObjectCollision* Get_First_Editor_Collsion(float px = 0.0f, float py = 0.0f);
        /* Get the first object colliding with the mouse
         * returns the collision if an object was found else NULL
         * The collision data should be deleted if not used anymore
        */
        cObjectCollision* Get_First_Mouse_Collision(const GL_rect& mouse_rect);

        // Update
        virtual void Update(void);
        // Draw
        virtual void Draw(void);

        // Update the Mouse Position
        void Update_Position(void);
        // Update the Double Click event
        void Update_Doubleclick(void);

        // Editor left click
        void Left_Click_Down(void);
        // Editor default double left click
        void Double_Click(bool activate = 1);

        // ## Hovered Object
        // Set new hovered object
        void Set_Hovered_Object(cSprite* sprite);
        // update the hovered object position
        void Update_Hovered_Object(void);

        // clear hovered object
        inline void Clear_Hovered_Object(void)
        {
            Set_Hovered_Object(NULL);
        }

        // ## Copy Objects
        // Add new copy object
        void Add_Copy_Object(cSprite* sprite);
        void Add_Copy_Objects(cSprite_List& spritelist);
        /* Remove a copy_object
         * returns true if removed
        */
        bool Remove_Copy_Object(const cSprite* sprite);
        // clear copy object list
        void Clear_Copy_Objects(void);
        /* returns the position of the first top left copy object
         * if no copy_objects returns 0, 0
        */
        GL_Vector Get_Copy_Object_Base(float px, float py);
        // paste copy object list
        void Paste_Copy_Objects(float px, float py);

        // ## Selected Objects
        /* Add new selected_object
         * from_user : if the user selected this object
         * Returns true if object got added or changed to user object
        */
        bool Add_Selected_Object(cSprite* sprite, bool from_user = 0);
        void Add_Selected_Objects(cSprite_List& spritelist, bool from_user = 0);
        /* Remove a selected_object
         * returns true if found and removed
        */
        bool Remove_Selected_Object(const cSprite* sprite, bool no_user = 0);
        // Returns all selected objects
        cSprite_List Get_Selected_Objects(void);
        /* Gets the area enclosing all selected objects
         * if no objects are selected the rect is empty
        */
        GL_rect Get_Selected_Objects_Rect(void);
        // clear selected object list
        void Clear_Selected_Objects(void);
        // updates the selected objects positions
        void Update_Selected_Objects(void);
        // updates the selected object offset to the mouse position
        void Update_Selected_Object_Offset(cSelectedObject* obj);

        /* returns true if the given object is a selected_object
         * if only_user is given only check user objects
        */
        bool Is_Selected_Object(const cSprite* sprite, bool only_user = 0);
        // deletes the selected objects
        void Delete_Selected_Objects(void);
        // returns the selected objects size
        inline unsigned int Get_Selected_Object_Size(void) const
        {
            return m_selected_objects.size();
        }

        // ## Snap Object
        /* set new_pos of the nearest object in the snap range using the given object as the source
         * returns 0 if no object found
        */
        bool Get_Snap_Pos(GL_point& new_pos, int snap, cSelectedObject* src_obj);
        // update the snap position
        void Update_Snap_Pos(void);
        // Toggle Snap mode
        void Toggle_Snap_Mode(void);

        // ## Active Object
        // Set new active object
        void Set_Active_Object(cSprite* sprite);
        // clear active object
        void Clear_Active_Object(void);

        /* Copies the given object to the given position
         * object is automatically added to the object manager
         * returns the new object
        */
        cSprite* Copy(const cSprite* copy_object, float px, float py) const;
        // Deletes the given Object
        void Delete(cSprite* sprite);
        // Set the mouse position to the given object
        void Set_Object_Position(cSelectedObject* sel_obj);

        // Draws a rect around the m_hovering_object and selected objects
        void Draw_Object_Rects(void);

        // Start selection mode
        void Start_Selection(void);
        // End selection mode
        void End_Selection(void);
        // update selection mode
        void Update_Selection(void);

        // Toggle Mover mode
        void Toggle_Mover_Mode(void);
        // Updates the Mover Mode
        void Mover_Update(int move_x, int move_y);
        // Updates the editor Mouse
        void Editor_Update(void);

        // current internal mouse position
        // (this is the on-screen coordinate pair, use
        // the inherited m_pos_x and m_pos_y for the in-level coordinates)
        int m_x;
        int m_y;

        // if object selection mode is active
        bool m_selection_mode;
        // selection rect
        GL_rect m_selection_rect;
        // Same as selection rect, but no negative dimensions
        GL_rect m_normalized_selection_rect;

        // if activated the mouse cursor movement moves the screen
        bool m_mover_mode;
        // The position of the cursor when it entered mover mode
        int m_mover_center_x;
        int m_mover_center_y;
        // fast copy mode
        bool m_fastcopy_mode;

        // if activated the selected object(s) snap to the nearest object
        bool m_snap_to_object_mode;
        // if a snap position was found
        bool m_snap_pos_available;
        // position that is currently being snapped to
        GL_point m_snap_pos;

        /* selected objects
         * the mouse object is also always a selected object
        */
        SelectedObjectList m_selected_objects;
        // currently colliding object with the mouse
        cSelectedObject* m_hovering_object;
        // objects selected for copying
        CopyObjectList m_copy_objects;
        // settings activated object
        cSprite* m_active_object;

        // buttons pressed state
        bool m_left;
        bool m_right;
        bool m_middle;

        // last clicked object
        cSprite* m_last_clicked_object;
        // counter for catching double-clicks
        float m_click_counter;
        // Dragging mode (what happens if dragged)
        DragMode m_dragmode;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

// The Mouse
    extern cMouseCursor* pMouseCursor;

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

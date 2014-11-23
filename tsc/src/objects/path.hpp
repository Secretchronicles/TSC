/***************************************************************************
 * path.h
 *
 * Copyright © 2008 - 2011 Florian Richter
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

#ifndef TSC_PATH_HPP
#define TSC_PATH_HPP

#include "../core/global_basic.hpp"
#include "../objects/sprite.hpp"
#include "../scripting/objects/specials/mrb_path.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** Path state class *** *** *** *** *** *** *** *** *** *** */

// forward declaration
    class cPath;

    class cPath_State {
    public:
        // constructor
        cPath_State(cSprite_Manager* sprite_manager);
        ~cPath_State(void);

        // load from savegame
        void Load_From_Savegame(cSave_Level_Object* save_object);
        // save to an existing savegame object
        void Save_To_Savegame(cSave_Level_Object* save_object);
        // Set the parent sprite manager
        void Set_Sprite_Manager(cSprite_Manager* sprite_manager);

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "Path"), &Scripting::rtTSC_Scriptable, this));
        }

        // draw current position
        void Draw(void);

        // Get the level path object with the given identifier
        cPath* Get_Path_Object(const std::string& identifier);

        /* Set the parent path identifier
         * sets the position back to the start
        */
        void Set_Path_Identifier(const std::string& path);
        // event if parent path got destroyed
        void Path_Destroyed_Event(void);

        // Start movement from the start of the opposite direction
        void Move_Toggle(void);
        // Move from the set start direction
        void Move_Reset(void);
        // Reverse movement direction
        void Move_Reverse(void);
        // Move from the beginning to the end
        void Move_Start_Forward(void);
        // Move from the end to the beginning
        void Move_Start_Backward(void);
        // Move from the given segment
        void Move_From_Segment(unsigned int segment);

        /* Returns true if it can move further, false if not.
         * May also return true once at end of path.
         * If distance is negative, walk backwards.
        */
        bool Path_Move(float distance);

        // the parent sprite manager
        cSprite_Manager* m_sprite_manager;

        // parent path identifier
        std::string m_path_identifier;
        // parent path pointer ( auto removes the link if destroyed itself )
        cPath* m_path;

        // current position
        float m_pos_x;
        float m_pos_y;

        // current direction
        bool m_forward;
        // current segment position
        float m_current_segment_pos;
        // current segment
        unsigned int m_current_segment;
    };

    /* *** *** *** *** *** *** *** cPath_Segment *** *** *** *** *** *** *** *** *** *** */

    class cPath_Segment {
    public:
        // constructor
        cPath_Segment(void);
        // destructor
        ~cPath_Segment(void);

        // Set position
        void Set_Pos(float x1, float y1, float x2, float y2);
        // Set start position
        void Set_Pos_Start(float x1, float y1);
        // Set end position
        void Set_Pos_End(float x2, float y2);
        // Set start x1 position
        void Set_Pos_Start_X(float x1);
        // Set start y1 position
        void Set_Pos_Start_Y(float y1);
        // Set end x2 position
        void Set_Pos_End_X(float x2);
        // Set end y2 position
        void Set_Pos_End_Y(float y2);

        // update unit vector and distance
        void Update(void);

        // start point
        float m_x1;
        float m_y1;
        // end point
        float m_x2;
        float m_y2;
        // unit vector from point 1 to point 2
        float m_ux;
        float m_uy;
        // total distance
        float m_distance;
    };

    /* *** *** *** *** *** *** *** Path class *** *** *** *** *** *** *** *** *** *** */

    class cPath : public cSprite {
    public:
        // constructor
        cPath(cSprite_Manager* sprite_manager);
        // create from stream
        cPath(XmlAttributes& attributes, cSprite_Manager* sprite_manager);
        // destructor
        virtual ~cPath(void);

        // initialize defaults
        void Init(void);
        // copy this sprite
        virtual cPath* Copy(void) const;

        // load from savegame
        virtual void Load_From_Savegame(cSave_Level_Object* save_object);
        // save to savegame
        virtual cSave_Level_Object* Save_To_Savegame(void);

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "Path"), &Scripting::rtTSC_Scriptable, this));
        }

        // Set the identifier
        void Set_Identifier(const std::string& identifier);
        // Set the showing of the line
        void Set_Show_Line(bool show);
        // Set if we move from the beginning again if reached the end instead of turning around
        void Set_Rewind(bool rewind);

        // Add a link to a path state
        void Create_Link(cPath_State* path_state);
        // Remove a link to a path state
        void Remove_Link(cPath_State* path_state);
        // Remove all links
        void Remove_Links(void);

        // Add a segment to the path. Note the segment will be copied
        // and the copy will be used, so cPath doesn't take control of
        // your memory management.
        void Add_Segment(cPath_Segment segment);

        // update
        virtual void Update(void);
        // draw
        virtual void Draw(cSurface_Request* request /* = NULL */);

        // if draw is valid for the current state and position
        virtual bool Is_Draw_Valid(void);

        // level editor activation
        virtual void Editor_Activate(void);
        // editor state update
        virtual void Editor_State_Update(void);
        // editor identifier text changed event
        bool Editor_Identifier_Text_Changed(const CEGUI::EventArgs& event);
        // editor show line option selected event
        bool Editor_Show_Line_Select(const CEGUI::EventArgs& event);
        // editor move type option selected event
        bool Editor_Move_Type_Select(const CEGUI::EventArgs& event);
        // editor selected segment option selected event
        bool Editor_Selected_Segment_Select(const CEGUI::EventArgs& event);
        // button add segment clicked event
        bool Editor_Button_Add_Segment_Clicked(const CEGUI::EventArgs& event);
        // button delete segment clicked event
        bool Editor_Button_Delete_Segment_Clicked(const CEGUI::EventArgs& event);
        // position x1 text changed event
        bool Editor_Pos_X1_Text_Changed(const CEGUI::EventArgs& event);
        // position y1 text changed event
        bool Editor_Pos_Y1_Text_Changed(const CEGUI::EventArgs& event);
        // position x2 text changed event
        bool Editor_Pos_X2_Text_Changed(const CEGUI::EventArgs& event);
        // position y2 text changed event
        bool Editor_Pos_Y2_Text_Changed(const CEGUI::EventArgs& event);
        // set linked path states to move again from start of the current segment
        void Editor_Segment_Pos_Changed(void);

        // string identifier (so objects can link to us)
        std::string m_identifier;
        // move from the beginning again if reached the end instead of turning around
        bool m_rewind;
        // show moving lines ingame if set
        bool m_show_line;

        // line segments
        typedef vector<cPath_Segment> PathList;
        PathList m_segments;

        // linked path states
        typedef vector<cPath_State*> PathStateList;
        PathStateList m_linked_path_states;

        // Save to XML node
        virtual xmlpp::Element* Save_To_XML_Node(xmlpp::Element* p_element);

    protected:
        virtual std::string Get_XML_Type_Name();

    private:
        // editor color
        Color m_editor_color;
        // editor selected segment
        unsigned int m_editor_selected_segment;
    };


    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

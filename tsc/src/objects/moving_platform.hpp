/***************************************************************************
 * moving_platform.h
 *
 * Copyright © 2005 - 2011 Florian Richter
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

#ifndef TSC_MOVING_PLATFORM_HPP
#define TSC_MOVING_PLATFORM_HPP

#include "../core/global_basic.hpp"
#include "../objects/animated_sprite.hpp"
#include "../objects/path.hpp"
#include "../scripting/objects/specials/mrb_moving_platform.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** *** Moving Platform type *** *** *** *** *** *** *** *** *** */

    enum Moving_Platform_Type {
        MOVING_PLATFORM_TYPE_LINE = 0,
        MOVING_PLATFORM_TYPE_CIRCLE = 1,
        MOVING_PLATFORM_TYPE_PATH = 2,
        MOVING_PLATFORM_TYPE_PATH_BACKWARDS = 3
    };

    /* *** *** *** *** *** *** *** *** Moving Platform state *** *** *** *** *** *** *** *** *** */

    enum Moving_Platform_State {
        MOVING_PLATFORM_STAY = 0,
        MOVING_PLATFORM_TOUCHED = 1,
        MOVING_PLATFORM_SHAKE = 2,
        MOVING_PLATFORM_FALL = 3
    };

    /* *** *** *** *** *** *** cMoving_Platform *** *** *** *** *** *** *** *** *** *** *** */

    /* Moving Platform
     * Move along a line, circle or path
     * todo : circle start angle and circle radius for height and width ?
    */
    class cMoving_Platform : public cAnimated_Sprite {
    public:
        // constructor
        cMoving_Platform(cSprite_Manager* sprite_manager);
        // create from stream
        cMoving_Platform(XmlAttributes& attributes, cSprite_Manager* sprite_manager);
        // destructor
        ~cMoving_Platform(void);

        // init defaults
        void Init(void);
        /* late initialization
         * this needs linked objects to be already loaded
        */
        virtual void Init_Links(void);
        // copy
        virtual cMoving_Platform* Copy(void) const;

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "MovingPlatform"), &Scripting::rtTSC_Scriptable, this));
        }

        // Set the parent sprite manager
        virtual void Set_Sprite_Manager(cSprite_Manager* sprite_manager);

        // load from savegame
        virtual void Load_From_Savegame(cSave_Level_Object* save_object);
        // save to savegame
        virtual cSave_Level_Object* Save_To_Savegame(void);

        // Set move type
        void Set_Move_Type(Moving_Platform_Type move_type);
        /* Set the path identifier
         * only used if move type is path
        */
        void Set_Path_Identifier(const std::string& identifier);
        // Set Massive type
        virtual void Set_Massive_Type(MassiveType new_type);
        // Set Direction
        void Set_Direction(const ObjectDirection dir, bool new_start_direction = 0);
        /* Set Maximum distance
         * if move type is line this is the length and if circle the radius
         * but not used if path
        */
        void Set_Max_Distance(int new_max_distance);
        // Set the Speed
        void Set_Speed(float val);
        // Set time when touched until it starts shaking
        void Set_Touch_Time(float val);
        // Set time it's shaking until falling
        void Set_Shake_Time(float val);
        /* Set time when touched until it starts moving
         * if set to 0 automatically moves
        */
        void Set_Touch_Move_Time(float val);

        // Set the middle image count
        void Set_Middle_Count(const unsigned int val);
        // set image
        void Set_Image_Top_Left(cGL_Surface* surface);
        void Set_Image_Top_Middle(cGL_Surface* surface);
        void Set_Image_Top_Right(cGL_Surface* surface);

        // update
        virtual void Update(void);
        // draw
        virtual void Draw(cSurface_Request* request /* = NULL */);

        // Update rect
        void Update_Rect(void);
        // Update velocity
        void Update_Velocity(void);

        // if update is valid for the current state
        virtual bool Is_Update_Valid();
        // if draw is valid for the current state and position
        virtual bool Is_Draw_Valid(void);

        /* Validate the given collision object
         * returns 0 if not valid
         * returns 1 if an internal collision with this object is valid
         * returns 2 if the given object collides with this object (blocking)
        */
        virtual Col_Valid_Type Validate_Collision(cSprite* obj);
        // collision from player
        virtual void Handle_Collision_Player(cObjectCollision* collision);
        // collision from an enemy
        virtual void Handle_Collision_Enemy(cObjectCollision* collision);

        // editor activation
        virtual void Editor_Activate(void);
        // editor state update
        virtual void Editor_State_Update(void);
        // editor events
        bool Editor_Move_Type_Select(const CEGUI::EventArgs& event);
        bool Editor_Path_Identifier_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Direction_Select(const CEGUI::EventArgs& event);
        bool Editor_Max_Distance_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Speed_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Hor_Middle_Count_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Image_Top_Left_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Image_Top_Middle_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Image_Top_Right_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Touch_Time_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Shake_Time_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Touch_Move_Time_Text_Changed(const CEGUI::EventArgs& event);

        // platform moving type
        Moving_Platform_Type m_move_type;
        // internal platform state
        Moving_Platform_State m_platform_state;

        // current angle if move type is circle
        float m_moving_angle;
        // movement speed
        float m_speed;
        // max moving distance
        int m_max_distance;
        // how many times the middle image is drawn
        unsigned int m_middle_count;

        // time when touched until it starts shaking
        float m_touch_time;
        // time since the platform got touched
        float m_touch_counter;
        // time it's shaking until falling
        float m_shake_time;
        // time since shaking started
        float m_shake_counter;
        // shaking direction
        bool m_shake_dir;
        // shaking direction counter
        float m_shake_dir_counter;
        // time when touched until it starts moving
        float m_touch_move_time;

        // path state if linked to a path
        cPath_State m_path_state;

        // Save to XML node
        virtual xmlpp::Element* Save_To_XML_Node(xmlpp::Element* p_element);
        virtual std::string Create_Name(void) const;

    protected:
        virtual std::string Get_XML_Type_Name();

    private:
        // position when movement should slow down
        float m_max_distance_slow_down_pos;
        // lowest possible speed when moving
        float m_lowest_speed;
        // editor color
        Color m_editor_color;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

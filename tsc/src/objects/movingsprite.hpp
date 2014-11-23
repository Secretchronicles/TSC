/***************************************************************************
 * movingsprite.h
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

#ifndef TSC_MOVINGSPRITE_HPP
#define TSC_MOVINGSPRITE_HPP

#include "../objects/sprite.hpp"
#include "../scripting/objects/sprites/mrb_moving_sprite.hpp"

namespace TSC {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

    const float deg_to_rad = static_cast<float>(M_PI / 180.0f);

    /* *** *** *** *** *** *** *** Movement states *** *** *** *** *** *** *** *** *** *** */

    enum Moving_state {
        STA_STAY    = 0,
        STA_WALK    = 1,
        // This one is abused special movements
        STA_RUN     = 2,
        STA_FALL    = 3,
        STA_FLY     = 4,
        STA_JUMP    = 5,
        STA_CLIMB   = 6,
        // another object controls this object ( object doesn't update itself )
        STA_OBJ_LINKED = 7
    };

    /* *** *** *** *** *** *** *** collision check type *** *** *** *** *** *** *** *** *** *** */

    enum ColCheckType {
        // Only allow internal (not blocking) collisions
        COLLIDE_ONLY_INTERNAL = 1,
        // Only allow blocking collisions
        COLLIDE_ONLY_BLOCKING = 2,
        // Complete collision check
        COLLIDE_COMPLETE = 3
    };

    /* *** *** *** *** *** *** *** cMovingSprite *** *** *** *** *** *** *** *** *** *** */

    class cMovingSprite : public cSprite {
    public:
        // constructor
        cMovingSprite(cSprite_Manager* sprite_manager, std::string type_name = "sprite");
        // create from stream
        cMovingSprite(XmlAttributes& attributes, cSprite_Manager* sprite_manager, std::string type_name = "sprite");
        // destructor
        virtual ~cMovingSprite(void);

        // Init defaults
        void Init(void);
        // copy this object
        virtual cMovingSprite* Copy(void) const;

        // Create the MRuby instance for this object.
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "MovingSprite"), &Scripting::rtTSC_Scriptable, this));
        }

        /* Sets the image for drawing
         * if new_start_image is set the default start_image will be set to the given image
         * if del_img is set the given image will be deleted
        */
        virtual void Set_Image(cGL_Surface* new_image, bool new_start_image = 0, bool del_img = 0);

        /* Set the direction
         * if new_start_direction is set also set the start/editor direction
        */
        virtual void Set_Direction(const ObjectDirection dir, bool new_start_direction = 0);

        // automatically slows down until not moving anymore
        void Auto_Slow_Down(float x_speed, float y_speed = 0);

        /* Moves this object
         * real : if set the speedfactor is not used
        */
        virtual void Move(float move_x, float move_y, const bool real = 0);
        /* Collision Move
         * if real is set the speedfactor will not be used
         * if force is set no collision detection will be performed
         * if check_on_ground is set after moving onground will be checked
        */
        virtual void Col_Move(float move_x, float move_y, bool real = 0, bool force = 0, bool check_on_ground = 1);

        /* if the ground object moves then move us with it
         * massive moving ground can crunch us
        */
        void Move_With_Ground(void);

        // Set velocity
        inline void Set_Velocity(const float x, const float y)
        {
            m_velx = x;
            m_vely = y;
        };
        // Sets the velocity from the given angle and speed
        inline void Set_Velocity_From_Angle(const float angle, const float speed, const bool new_start_direction = 0)
        {
            m_velx = cos(angle * deg_to_rad) * speed;
            m_vely = sin(angle * deg_to_rad) * speed;

            if (new_start_direction) {
                m_start_direction = m_direction;
            }
        };
        /* Add Velocity
         * real : if set the speedfactor isn't used
        */
        void Add_Velocity(const float x, const float y, const bool real = 0);
        void Add_Velocity_X(const float x, const bool real = 0);
        void Add_Velocity_Y(const float y, const bool real = 0);
        /* Add Velocity
        * max : the maximum velocity
        * real : if set the speedfactor isn't used
        */
        inline void Add_Velocity_X_Max(const float x, const float max_x, const bool real = 0)
        {
            Add_Velocity_X(x, real);

            if (m_velx > max_x) {
                m_velx = max_x;
            }
        }
        inline void Add_Velocity_Y_Max(const float y, const float max_y, const bool real = 0)
        {
            Add_Velocity_Y(y, real);

            if (m_vely > max_y) {
                m_vely = max_y;
            }
        }
        /* Add Velocity
        * min : the minimum velocity
        * real : if set the speedfactor isn't used
        */
        inline void Add_Velocity_X_Min(const float x, const float min_x, const bool real = 0)
        {
            Add_Velocity_X(x, real);

            if (m_velx < min_x) {
                m_velx = min_x;
            }
        }
        inline void Add_Velocity_Y_Min(const float y, const float min_y, const bool real = 0)
        {
            Add_Velocity_Y(y, real);

            if (m_vely < min_y) {
                m_vely = min_y;
            }
        }

        /* Move into the opposite Direction
         * if col_dir is given only turns around if the collision direction is in front
         */
        virtual void Turn_Around(ObjectDirection col_dir = DIR_UNDEFINED);

        // update
        virtual void Update(void);
        // Update gravity velocity
        virtual void Update_Gravity(void);
        /* draw
        * if request is NULL automatically creates the request
        */
        virtual void Draw(cSurface_Request* request = NULL);

        /* Check if moving the current collision rect position with the given values is valid
         * check_type : set which collision types are added to the list
         * objects : if set check these object instead of all
         * The collision data should be deleted if not used anymore
        */
        cObjectCollisionType* Collision_Check_Relative(const float x, const float y, const float w = 0.0f, const float h = 0.0f, const ColCheckType check_type = COLLIDE_COMPLETE, cSprite_List* objects = NULL)
        {
            return Collision_Check_Absolute(m_col_rect.m_x + x, m_col_rect.m_y + y, w, h, check_type, objects);
        }
        /* Check if the given position is valid
         * Creates a collision rect with the given values
         * check_type : set which collision types are added to the list
         * objects : if set check these object instead of all
         * The collision data should be deleted if not used anymore
        */
        cObjectCollisionType* Collision_Check_Absolute(const float x, const float y, const float w = 0.0f, const float h = 0.0f, const ColCheckType check_type = COLLIDE_COMPLETE, cSprite_List* objects = NULL);
        /* Check if the given position is valid
         * new_rect : this is the source collision rect
         * check_type : set which collision types are added to the list
         * objects : if set check these object instead of all
         * The collision data should be deleted if not used anymore
        */
        cObjectCollisionType* Collision_Check(const GL_rect& new_rect, const ColCheckType check_type = COLLIDE_COMPLETE, cSprite_List* objects = NULL);

        // Check if the given movement goes out of the level rect and handle possible out of level events
        void Check_And_Handle_Out_Of_Level(const float move_x, const float move_y);
        // Check if the given movement goes out of the level rect
        bool Is_Out_Of_Level_Left(const float move_x) const;
        bool Is_Out_Of_Level_Right(const float move_x) const;
        bool Is_Out_Of_Level_Top(const float move_y) const;
        bool Is_Out_Of_Level_Bottom(const float move_x) const;
        // Set the ground object
        virtual bool Set_On_Ground(cSprite* obj, bool set_on_top = 1);
        // Check if the Object is onground and sets the state to onground
        virtual void Check_on_Ground(void);
        // object looses onground state
        inline void Reset_On_Ground(void)
        {
            m_ground_object = NULL;
        };
        // Corrects the position if the object got stuck
        void Update_Anti_Stuck(void);

        // default collision and movement handling
        virtual void Collide_Move(void);

        /* Freeze for the given time
        */
        void Freeze(float freeze_time = speedfactor_fps * 10);

        /* downgrade state ( if already at weakest state it dies )
         * force : usually dies or a complete downgrade
        */
        virtual void DownGrade(bool force = 0) {};

        /* set the horizontal sprite rotation based on the current direction
         * if start_rotation is set also set the start/editor rotation
        */
        void Update_Rotation_Hor(bool start_rotation = 0);

        /* handle collision with object that moves it with the current velocity
         * if moved object gets stuck it is downgraded
        */
        void Handle_Move_Object_Collision(const cObjectCollision* collision);

        /* Validate the given collision object
         * returns 0 if not valid
         * returns 1 if an internal collision with this object is valid
         * returns 2 if the given object collides with this object (blocking)
        */
        virtual Col_Valid_Type Validate_Collision(cSprite* obj);
        /* Check if colliding with ghost objects
         * returns normal validation type if it can be handled else 3
         */
        virtual Col_Valid_Type Validate_Collision_Ghost(cSprite* obj);
        /* Validate the collision if this could be moving and the object could be on top
         * returns -1 if nothing found
        */
        Col_Valid_Type Validate_Collision_Object_On_Top(cMovingSprite* moving_sprite);
        /* send a collision to an object based from the given collision
         * handle_now : if set to true it is handled now instead of adding it to the list
        */
        virtual void Send_Collision(const cObjectCollision* collision, bool handle_now = 0);
        // default collision handler
        virtual void Handle_Collision(cObjectCollision* collision);
        // handle moved out of Level event
        virtual void Handle_out_of_Level(ObjectDirection dir) {};

        // velocity
        float m_velx;
        float m_vely;
        // maximum gravity velocity
        float m_gravity_max;

        // current direction
        ObjectDirection m_direction;
        // start direction
        ObjectDirection m_start_direction;

        // can be on a ground object
        bool m_can_be_on_ground;
        // colliding ground object
        cSprite* m_ground_object;

        /* the different states
         * look at the definitions
         */
        Moving_state m_state;

        // ice resistance
        float m_ice_resistance;
        // time counter if frozen
        float m_freeze_counter;

    private:
        /* moves in steps and checks in both directions simultaneous
         * returns the found collisions
         * sprite_list : objects to check
         * stop_on_internal : if set stops moving if internal collision was found
        */
        cObjectCollisionType* Col_Move_in_Steps(float move_x, float move_y, float step_size_x, float step_size_y, float final_pos_x, float final_pos_y, cSprite_List sprite_list, bool stop_on_internal = 0);
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

/***************************************************************************
 * level_player.h
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

#ifndef TSC_LEVEL_PLAYER_HPP
#define TSC_LEVEL_PLAYER_HPP

#include "../core/global_basic.hpp"
#include "../objects/ball.hpp"
#include "../objects/animated_sprite.hpp"
#include "../scripting/scripting.hpp"
#include "../scripting/objects/misc/mrb_level_player.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** *** Maryo states *** *** *** *** *** *** *** *** *** */

    enum Maryo_type {
        // normal
        MARYO_DEAD = 0,
        MARYO_SMALL = 1,
        MARYO_BIG = 2,
        MARYO_FIRE = 3,
        MARYO_ICE = 4,
        MARYO_CAPE = 5,
        MARYO_GHOST = 6
    };

    /* *** *** *** *** *** *** *** *** Maryo image array positions *** *** *** *** *** *** *** *** *** */

    enum Maryo_imgpos {
        MARYO_IMG_STAND = 0,
        MARYO_IMG_WALK = 0, // walking also uses the stay image
        MARYO_IMG_RUN = 8,
        MARYO_IMG_FALL = 12,
        MARYO_IMG_JUMP = 14,
        MARYO_IMG_DEAD = 16,
        MARYO_IMG_DUCK = 18,
        MARYO_IMG_CLIMB = 20,
        MARYO_IMG_THROW = 22,
        MARYO_IMG_FLY = 26,
        MARYO_IMG_SPECIAL_1 = 34
    };

    /* *** *** *** *** *** *** *** Level player return stack entry *** *** *** *** *** *** *** *** *** *** */
    class cLevel_Player_Return_Entry {
    public:
        std::string level;
        std::string entry;
    };

    /* *** *** *** *** *** *** *** Level player *** *** *** *** *** *** *** *** *** *** */

    class cLevel_Player : public cAnimated_Sprite {
    public:
        // constructor
        cLevel_Player(cSprite_Manager* sprite_manager);
        // destructor
        virtual ~cLevel_Player(void);

        // copy
        virtual cLevel_Player* Copy(void) const;

        // Init
        virtual void Init(void);
        // lets the Player hold in
        void Hold(void);

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "LevelPlayer"), &Scripting::rtTSC_Scriptable, this));
        }

        /* Set the direction
         * if new_start_direction is set also set the start/editor direction
        */
        virtual void Set_Direction(const ObjectDirection dir, bool new_start_direction = 0);

        // Set the ground object
        virtual bool Set_On_Ground(cSprite* obj, bool set_on_top = 1);

        /* downgrade state ( if small maryo : dies )
         * force : dies or a complete downgrade
        */
        virtual void DownGrade(bool force = 0);
        /* downgrade state ( if small maryo : dies )
         * if delayed is set the downgrade is handled on the next game update
         * force : Also kill big/fire/ice/etc Maryo
         * ignore_invincible: If Maryo was just hurt, he is invicable for a short
         * time and this method immediately returns usually without doing anything
         * (this also applies to star effect). Setting `ignore_invicable' will make
         * this method ignore this restriction. This does *not* affect god mode, which
         * will always be honoured by this method (by doing nothing).
        */
        void DownGrade_Player(bool delayed = true, bool force = false, bool ignore_invicible = false);

        // moves in the current direction
        void Move_Player(float velocity, float vel_wrongway);
        // generate foot clouds
        void Generate_Feet_Clouds(cParticle_Emitter* anim = NULL);
        // Update the walking
        void Update_Walking(void);
        // Update the running
        void Update_Running(void);
        // Update the staying
        void Update_Staying(void);
        // Update the flying
        void Update_Flying(void);
        /* Stop flying
         * parachute : if set fall slowly with a parachute
        */
        void Stop_Flying(bool parachute = 1);
        /* Set the parachute for flying
         * doesn't reload images
        */
        void Parachute(bool enable);
        // Start falling
        void Start_Falling(void);
        // Update falling
        void Update_Falling();
        // Start ducking
        void Start_Ducking(void);
        // Stop ducking
        void Stop_Ducking(void);
        // Update ducking
        void Update_Ducking(void);
        // Start climbing
        void Start_Climbing(void);
        // Update the climbing
        void Update_Climbing(void);
        // Test if colliding with climbable
        bool Is_On_Climbable(float move_y = 0.0f);
        // Set up the m_up_key_time for better jumping detection
        void Start_Jump_Keytime(void);
        // Update the m_up_key_time and start a jump if possible
        void Update_Jump_Keytime(void);
        // Start a jump with the given de-acceleration
        void Start_Jump(float deaccel = 0.08f);
        // Update jump
        void Update_Jump(void);

        // Update the active item
        void Update_Item(void);

        /* Release the Active Item
         * if set_position is set the active item will be set in front of the player
         * if no_action is set the item will just be released
        */
        void Release_Item(bool set_position = 1, bool no_action = 0);

        /* Set the Type
         * animate : show type change animation
         * sound : if set the upgrade sound is played
         * temp_power : set the temp power item
        */
        void Set_Type(Maryo_type new_type, bool animation = 1, bool sound = 1, bool temp_power = 0);
        void Set_Type(SpriteType item_type, bool animation = 1, bool sound = 1, bool temp_power = 0);
        // set the moving state
        void Set_Moving_State(Moving_state new_state);

        // default update
        virtual void Update(void);

        // Draw
        virtual void Draw(cSurface_Request* request = NULL);
        /* Draw an animation using the given new type
         * ends with the new type set
        */
        void Draw_Animation(Maryo_type new_mtype);
        // Returns the current image number without the direction modifier
        unsigned int Get_Image(void) const;

        // Loads the images depending on maryo_type
        void Load_Images(void);

        /* Sets the best position to advance in size
         * if only_check is set position is unchanged
        */
        bool Change_Size(float x, float y, bool only_check = 0);

        /* Player gets the given Item
         * force : if true the player will use the item immediately
         * and it will not be placed in the itembox
         * base : must be given on non-item objects ( like shell )
         */
        void Get_Item(SpriteType item_type, bool force = 0, cMovingSprite* base = NULL);

        // returns the current velocity modifier
        float Get_Vel_Modifier(void) const;

        // Start a jump
        void Action_Jump(bool enemy_jump = 0);
        // General input interact event
        void Action_Interact(input_identifier key_type);
        // Shoot if available
        void Action_Shoot(void);

        // Stop jump
        void Action_Stop_Jump(void);
        // Stop Interacting event
        void Action_Stop_Interact(input_identifier key_type);
        // Stop Shoot action
        void Action_Stop_Shoot(void);

        /* Adds a Fire/Ice-ball
         * ball_start_angle : starting angle
         * amount : the amount of balls to add
         * Returns 1 if successful
        */
        bool Ball_Add(ball_effect effect_type = FIREBALL_DEFAULT, float ball_start_angle = -1, unsigned int amount = 0) const;
        // Destroys all Fire/Ice-balls
        void Ball_Clear(void) const;

        // Resets the complete game state
        void Reset_Save(void);
        /* Reset state and status
         * if full is set reset item and camera state
        */
        void Reset(bool full = 1);
        // Reset position to start position
        void Reset_Position(void);

        // Add an enemy to the kill multiplier
        void Add_Kill_Multiplier(void);
        // Update the kill multiplier
        void Update_Kill_Multiplier(void);

        /* Validate the given collision object
         * returns 0 if not valid
         * returns 1 if an internal collision with this object is valid
         * returns 2 if the given object collides with this object (blocking)
        */
        virtual Col_Valid_Type Validate_Collision(cSprite* obj);

        // collision from an enemy
        virtual void Handle_Collision_Enemy(cObjectCollision* collision);
        // collision with massive
        virtual void Handle_Collision_Massive(cObjectCollision* collision);
        // collision with passive
        virtual void Handle_Collision_Passive(cObjectCollision* collision);
        // collision with lava
        virtual void Handle_Collision_Lava(cObjectCollision* collision);
        // handle moved out of Level event
        virtual void Handle_out_of_Level(ObjectDirection dir);

        // editor activation
        virtual void Editor_Activate(void);
        // editor direction option selected event
        bool Editor_Direction_Select(const CEGUI::EventArgs& event);

        // Push a return stack
        void Push_Return(const std::string& level, const std::string& entry);
        // Pop a return item off the stack. False if no more items
        bool Pop_Return(std::string& level, std::string& entry);
        // Clear return items off the stack.
        void Clear_Return(void);

        // current Maryo type
        Maryo_type m_maryo_type;
        //  Maryo type after the temporary powerup
        Maryo_type m_maryo_type_temp_power;

        // true if player god mode is active
        bool m_god_mode;

        // time maryo walked
        float m_walk_time;
        // running particle counter
        float m_running_particle_counter;
        // time maryo is in ghost mode
        float m_ghost_time;
        // ghost mode drawing modifier
        float m_ghost_time_mod;

        // Amount of time left for invincibility (from damage, star, or file load). 0 means not invincible.
        float m_invincible;
        // Amount of time left for star (used for graphic flash effect)
        float m_invincible_star;
        // invincible drawing modifier
        float m_invincible_mod;
        // invincible star counter
        float m_invincible_star_counter;

        // walking animation counter
        float m_walk_count;
        // lives left
        int m_lives;

        // jumping key time
        float m_up_key_time;
        // force next jump
        bool m_force_jump;
        // next jump with sound
        bool m_next_jump_sound;
        // next jump power
        float m_next_jump_power;
        // next jump acceleration
        float m_next_jump_accel;
        // Acceleration if up key is pressed
        float m_jump_accel_up;
        // vely De-acceleration use 0.05 - 0.08
        float m_jump_vel_deaccel;
        // next jump power
        float m_jump_power;

        // throw animation counter
        float m_throwing_counter;
        // if maryo is ducked
        Uint32 m_ducked_counter;
        // ducked animation counter
        float m_ducked_animation_counter;
        // if parachute falling
        bool m_parachute;
        // amount of goldpieces collected
        unsigned int m_goldpieces;
        // collected points
        long m_points;
        // enemy killed in a row multiplier
        float m_kill_multiplier;
        // time since the last killed enemy
        float m_last_kill_counter;
        // time player didn't move
        float m_no_velx_counter;
        float m_no_vely_counter;

        // fire/ice-ball shoot counter
        float m_shoot_counter;
        // active/holding object ( f.e. shell )
        cMovingSprite* m_active_object;
        // direction when ducking started ( for the anti stuck under a block test )
        ObjectDirection m_duck_direction;

        // default position
        static const float m_default_pos_x;
        static const float m_default_pos_y;

        // Level return stack
        std::vector<cLevel_Player_Return_Entry> m_return_stack;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

// The Player
    extern cLevel_Player* pLevel_Player;

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

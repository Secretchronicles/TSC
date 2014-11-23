/***************************************************************************
 * enemy.h
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

#ifndef TSC_ENEMY_HPP
#define TSC_ENEMY_HPP

#include "../objects/animated_sprite.hpp"
#include "../core/framerate.hpp"
#include "../audio/audio.hpp"
#include "../scripting/objects/enemies/mrb_enemy.hpp"
#include "../objects/ball.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

    class cEnemy : public cAnimated_Sprite {
    public:
        // constructor
        cEnemy(cSprite_Manager* sprite_manager);
        // destructor
        virtual ~cEnemy(void);

        // load from savegame
        virtual void Load_From_Savegame(cSave_Level_Object* save_object);
        // save to savegame
        virtual cSave_Level_Object* Save_To_Savegame(void);

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "Enemy"), &Scripting::rtTSC_Scriptable, this));
        }

        // Set Dead
        virtual void Set_Dead(bool enable = 1);

        virtual void Set_Massive_Type(MassiveType type);

        // dying animation update
        void Update_Dying(void);
        // Animation update for an enemy killed regularlyly.
        // Default animation squishes the enemy.
        virtual void Update_Normal_Dying();
        // Animation update for an enemy killed with DownGrade(true),
        // e.g. abyss or lava.
        // Default animation makes the enemy move upwards a bit, then immediately
        // fall off the screen quickly.
        virtual void Update_Instant_Dying();
        // handle basic enemy updates
        virtual void Update(void);
        /* late update
         * use if it is needed that other objects are already updated
        */
        virtual void Update_Late(void);
        // update current velocity if needed
        void Update_Velocity(void);

        // Generates the default Hit Animation Particles
        void Generate_Hit_Animation(cParticle_Emitter* anim = NULL) const;

        // default collision handler
        virtual void Handle_Collision(cObjectCollision* collision);
        // collision with massive
        virtual void Handle_Collision_Massive(cObjectCollision* collision);
        // collision with lava
        virtual void Handle_Collision_Lava(cObjectCollision* collision);
        // handle moved out of Level event
        virtual void Handle_out_of_Level(ObjectDirection dir);
        // Handle hit by ball
        virtual void Handle_Ball_Hit(const cBall& ball, const cObjectCollision* p_collision);

        // Save to XML node
        virtual xmlpp::Element* Save_To_XML_Node(xmlpp::Element* p_element);

        virtual std::string Create_Name() const;

        // if dead
        bool m_dead;

        // default counter for animations
        float m_counter;

        // sound filename if got killed
        std::string m_kill_sound;
        // points if enemy got killed
        unsigned int m_kill_points;

        // maximum velocity for the current state
        float m_velx_max;
        // velocity gain speed
        float m_velx_gain;

        // is fire able to kill this enemy
        bool m_fire_resistant;
        // if this can be hit from the top
        //bool m_can_be_hit_from_top;
        // if this can be hit from a shell
        bool m_can_be_hit_from_shell;
        // if this moves into an abyss
        //bool m_moves_into_abyss;

    protected:
        // Counter for dying animation
        float m_dying_counter;

        // Will not execute update if this returns true. By default
        // this checks for m_dead and m_freeze_counter.
        virtual bool Is_Update_Valid();

        void Ball_Destroy_Animation(const cBall& ball);
        void Ball_Generate_Goldpiece(const cObjectCollision* p_collision);
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

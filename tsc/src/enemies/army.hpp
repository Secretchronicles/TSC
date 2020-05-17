/***************************************************************************
 * army.h
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

#ifndef TSC_ARMY_HPP
#define TSC_ARMY_HPP

#include "../enemies/enemy.hpp"
#include "../scripting/objects/enemies/mrb_armadillo.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

    enum Army_state {
        ARMY_DEAD     = 0,
        ARMY_WALK     = 1,
        ARMY_SHELL_STAND  = 2,
        ARMY_SHELL_RUN    = 3,
        ARMY_FLY      = 4 // todo
    };

    /* *** *** *** *** *** *** cArmy *** *** *** *** *** *** *** *** *** *** *** */
    /* The evolved walking Army
     * Likes to play roller coaster with alex
    */
    class cArmy : public cEnemy {
    public:
        // constructor
        cArmy(cSprite_Manager* sprite_manager);
        // create from stream
        cArmy(XmlAttributes& attributes, cSprite_Manager* sprite_manager);
        // destructor
        virtual ~cArmy(void);

        // init defaults
        void Init(void);
        // copy
        virtual cArmy* Copy(void) const;

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "Armadillo"), &Scripting::rtTSC_Scriptable, this));
        }


        // load from savegame
        virtual void Load_From_Savegame(cSave_Level_Object* save_object);
        // save to savegame
        virtual bool Save_To_Savegame_XML_Node(xmlpp::Element* p_element) const;

        // Set Direction
        virtual void Set_Direction(const ObjectDirection dir, bool new_start_direction = 0);
        // set color
        virtual void Set_Color(DefaultColor col);

        /* Move into the opposite Direction
         * if col_dir is given only turns around if the collision direction is in front
         */
        virtual void Turn_Around(ObjectDirection col_dir = DIR_UNDEFINED);

        /* downgrade state ( if already weakest state : dies )
         * force : usually dies or a complete downgrade
        */
        virtual void DownGrade(bool force = 0);
        // special dying animations
        virtual void Update_Normal_Dying(void);
        virtual void Update_Instant_Dying(void);

        // set the armadillo moving state
        void Set_Army_Moving_State(Army_state new_state);

        // update
        virtual void Update(void);

        // Change state to walking if it is shell
        virtual void Stand_Up(void);
        /* Hit the given enemy
         * returns true if enemy could get hit
        */
        bool Hit_Enemy(cEnemy* enemy);

        // update maximum velocity values
        void Update_Velocity_Max(void);

        // if update is valid for the current state
        virtual bool Is_Update_Valid();

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
        // collision with massive
        virtual void Handle_Collision_Massive(cObjectCollision* collision);
        virtual void Handle_Collision_Box(ObjectDirection cdirection, GL_rect* r2);

        // editor activation
        virtual void Editor_Activate(void);
        // editor direction option selected event
        bool Editor_Direction_Select(const CEGUI::EventArgs& event);

        // internal armadillo state
        Army_state m_army_state;

        /* If the player kicked the shell this counter is set.
         * if this counter is higher than 0
         * alex cannot get killed by the shell
         */
        float m_player_counter;

        // Color
        DefaultColor m_color_type;

        // Save to XML node
        virtual xmlpp::Element* Save_To_XML_Node(xmlpp::Element* p_element);

    protected:

        virtual std::string Get_XML_Type_Name();

        // image indexes
        int m_walk_start;
        int m_shell_start;
        int m_turn_start;
        int m_turn_end;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

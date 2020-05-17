/***************************************************************************
 * larry.hpp - The walking bomb.
 *
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

#ifndef TSC_LARRY_HPP
#define TSC_LARRY_HPP
#include "enemy.hpp"
#include "../scripting/objects/enemies/mrb_larry.hpp"

namespace TSC {

    class cLarry: public cEnemy {
    public:
        cLarry(cSprite_Manager* p_sprite_manager);
        cLarry(XmlAttributes& attributes, cSprite_Manager* p_sprite_manager);
        virtual ~cLarry();

        virtual cLarry* Copy() const;
        virtual void Update();
        virtual Col_Valid_Type Validate_Collision(cSprite* p_obj);
        virtual void Update_Normal_Dying();
        virtual void Handle_Collision_Massive(cObjectCollision* p_collision);
        virtual void Handle_Collision_Player(cObjectCollision* p_collision);
        virtual void Handle_Collision_Enemy(cObjectCollision* p_collision);
        virtual void Handle_Ball_Hit(const cBall&, const cObjectCollision* p_collision);

        void Fuse();
        virtual void DownGrade(bool force = false);
        virtual void Set_Direction(const ObjectDirection dir, bool initial = true);
        virtual void Turn_Around(ObjectDirection col_dir = DIR_UNDEFINED);
        virtual void Set_Moving_State(Moving_state new_state);

        virtual void Editor_Activate();

        virtual xmlpp::Element* Save_To_XML_Node(xmlpp::Element* p_element);

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "Larry"), &Scripting::rtTSC_Scriptable, this));
        }

    protected:
        virtual std::string Get_XML_Type_Name();
        virtual void Kill_Objects_in_Explosion_Range();

        float m_explosion_counter;

        // image positions
        int m_walk_turn_start;
        int m_walk_turn_end;
        int m_run_turn_start;
        int m_run_turn_end;
        int m_action_start;
        int m_action_end;

    private:
        void Init();
        void Explosion_Animation();

        bool On_Editor_Direction_Select(const CEGUI::EventArgs& event);
    };

}

#endif

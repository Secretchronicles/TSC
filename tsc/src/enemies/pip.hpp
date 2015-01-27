/***************************************************************************
 * pip.hpp
 *
 * Copyright © 2014 The TSC Contributors
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TSC_PIP_HPP
#define TSC_PIP_HPP
#include "enemy.hpp"

namespace TSC {

    class cPip: public cEnemy {
    public:

        cPip(cSprite_Manager* p_sprite_manager);
        cPip(XmlAttributes& attributes, cSprite_Manager* p_sprite_manager);
        virtual ~cPip();

        // Init defaults
        void Init();

        virtual cPip* Copy() const;
        virtual void Load_From_Savegame(cSave_Level_Object* p_save_object);

        virtual void Set_Direction(const ObjectDirection dir);
        /* Move into the opposite Direction
         * if col_dir is given only turns around if the collision direction is in front
         */
        virtual void Turn_Around(ObjectDirection col_dir = DIR_UNDEFINED);

        // Downgrade state (dies if already in weakest state).
        // force forces death.
        virtual void DownGrade(bool force = false);

        void Set_Moving_State(Moving_state new_state);

        // update
        virtual void Update();

        void Update_Velocity_Max();

        /* Validate the given collision object
         * returns 0 if not valid
         * returns 1 if an internal collision with this object is valid
         * returns 2 if the given object collides with this object (blocking)
         */
        virtual Col_Valid_Type Validate_Collision(cSprite* p_obj);
        // collision from player
        virtual void Handle_Collision_Player(cObjectCollision* p_collision);
        // collision from an enemy
        virtual void Handle_Collision_Enemy(cObjectCollision* p_collision);
        // collision with massive
        virtual void Handle_Collision_Massive(cObjectCollision* p_collision);

        // editor activation
        virtual void Editor_Activate();
        // editor direction option selected event
        bool Editor_Direction_Select(const CEGUI::EventArgs& event);

        // Save to XML node
        virtual xmlpp::Element* Save_To_XML_Node(xmlpp::Element* p_elemet);

    protected:
        virtual std::string Get_XML_Type_Name();

        // image indexes
        int m_big_start;
        int m_small_start;
    };

}

#endif

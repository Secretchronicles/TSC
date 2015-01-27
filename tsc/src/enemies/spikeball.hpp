/***************************************************************************
 * spikeball.h
 *
 * Copyright © 2009 - 2011 Florian Richter
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

#ifndef TSC_SPIKEBALL_HPP
#define TSC_SPIKEBALL_HPP

#include "../enemies/enemy.hpp"
#include "../scripting/objects/enemies/mrb_spikeball.hpp"

namespace TSC {

    /* *** *** *** *** *** *** cSpikeball *** *** *** *** *** *** *** *** *** *** *** */
    /* Paranoid hard metal ball.
     */
    class cSpikeball : public cEnemy {
    public:
        // constructor
        cSpikeball(cSprite_Manager* sprite_manager);
        // create from stream
        cSpikeball(XmlAttributes& attributes, cSprite_Manager* sprite_manager);
        // destructor
        virtual ~cSpikeball(void);

        // init defaults
        void Init(void);
        // copy
        virtual cSpikeball* Copy(void) const;

        // load from savegame
        virtual void Load_From_Savegame(cSave_Level_Object* save_object);

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "Spikeball"), &Scripting::rtTSC_Scriptable, this));
        }


        // Set Direction
        virtual void Set_Direction(const ObjectDirection dir, bool initial = true);
        /* set color
         * TODO: Currently there are only grey spikeballs.
         * grey = normal
        */
        void Set_Color(const DefaultColor& col);

        /* Move into the opposite Direction
         * if col_dir is given only turns around if the collision direction is in front
         */
        virtual void Turn_Around(ObjectDirection col_dir = DIR_UNDEFINED);

        /* downgrade state ( if already weakest state : dies )
         * force : usually dies or a complete downgrade
        */
        virtual void DownGrade(bool force = 0);

        // set the moving state
        void Set_Moving_State(Moving_state new_state);

        // update
        virtual void Update(void);

        // update maximum velocity values
        void Update_Velocity_Max(void);

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
        // collision from a box
        virtual void Handle_Collision_Box(ObjectDirection cdirection, GL_rect* r2);

        // editor activation
        virtual void Editor_Activate(void);
        // editor direction option selected event
        bool Editor_Direction_Select(const CEGUI::EventArgs& event);

        // Color
        DefaultColor m_color_type;

        // Save to XML node
        virtual xmlpp::Element* Save_To_XML_Node(xmlpp::Element* p_element);

    protected:

        virtual std::string Get_XML_Type_Name();

    private:
        // counter if staying
        float m_counter_stay;
        // counter if walking
        float m_counter_walk;
        // counter if running
        float m_counter_running;
        // particle counter if running
        float m_running_particle_counter;

        // image indexes
        int m_turn_start;
        int m_turn_end;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

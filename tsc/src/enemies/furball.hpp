/***************************************************************************
 * furball.h
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

#ifndef TSC_FURBALL_HPP
#define TSC_FURBALL_HPP

#include "../enemies/enemy.hpp"
#include "../scripting/objects/enemies/mrb_furball.hpp"

namespace TSC {

    /* *** *** *** *** *** *** cFurball *** *** *** *** *** *** *** *** *** *** *** */
    /* Secret attacks: Lulls you into a false sense of security so that you will
     * forget to do anything and just stupidly run into them.
     * Don't fall for it! Take them very seriously! Pay attention!
     */
    class cFurball : public cEnemy {
    public:
        // constructor
        cFurball(cSprite_Manager* sprite_manager);
        // create from stream
        cFurball(XmlAttributes& attributes, cSprite_Manager* sprite_manager);
        // destructor
        virtual ~cFurball(void);

        // init defaults
        void Init(void);
        // copy
        virtual cFurball* Copy(void) const;

        // load from savegame
        virtual void Load_From_Savegame(cSave_Level_Object* save_object);

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "Furball"), &Scripting::rtTSC_Scriptable, this));
        }


        // maximum downgrades until death
        void Set_Max_Downgrade_Count(int max_downgrade_count);
        inline int Get_Max_Downgrade_Count()
        {
            return m_max_downgrade_count;
        }

        // Set if the level ends when killed
        void Set_Level_Ends_If_Killed(bool level_ends_if_killed);
        inline bool Level_Ends_If_Killed()
        {
            return m_level_ends_if_killed;
        }

        // Number of times already downgaded
        inline int Get_Downgrade_Count()
        {
            return m_downgrade_count;
        }

        // Set Direction
        virtual void Set_Direction(const ObjectDirection dir);
        /* set color
         * brown = normal, blue = ice, black = boss
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
        // Special normal dying for furball boss
        virtual void Update_Normal_Dying(void);

        // set the moving state
        void Set_Moving_State(Moving_state new_state);

        // update
        virtual void Update(void);

        // Generates Star Particles (only used if boss)
        void Generate_Smoke(unsigned int amount = 1, float particle_scale = 0.4f) const;

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
        virtual void Handle_Collision_Box(ObjectDirection cdirection, GL_rect* r2);

        // editor activation
        virtual void Editor_Activate(void);
        // editor direction option selected event
        bool Editor_Direction_Select(const CEGUI::EventArgs& event);
        // editor max downgrades text changed event
        bool Editor_Max_Downgrade_Count_Text_Changed(const CEGUI::EventArgs& event);
        // editor level ends if killed
        bool Editor_Level_Ends_If_Killed(const CEGUI::EventArgs& event);

        // Color
        DefaultColor m_color_type;

        // Save to XML node
        virtual xmlpp::Element* Save_To_XML_Node(xmlpp::Element* p_element);

    protected:

        virtual std::string Get_XML_Type_Name();

    private:
        // counter if hit (only used if boss)
        float m_counter_hit;
        // counter if running (only used if boss)
        float m_counter_running;
        // particle counter if running (only used if boss)
        float m_running_particle_counter;

        // times downgraded (only used if boss)
        int m_downgrade_count;
        // maximum downgrades until death (only used if boss)
        int m_max_downgrade_count;
        // if the level ends when killed (only used if boss)
        bool m_level_ends_if_killed;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

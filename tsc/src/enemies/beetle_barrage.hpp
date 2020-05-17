/***************************************************************************
 * beetle_barrage.hpp
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

#ifndef TSC_BEETLE_BARRAGE_HPP
#define TSC_BEETLE_BARRAGE_HPP
#include "enemy.hpp"
#include "beetle.hpp"
#include "../scripting/objects/enemies/mrb_beetle_barrage.hpp"

namespace TSC {

    class cBeetleBarrage: public cEnemy {
    public:
        cBeetleBarrage(cSprite_Manager* p_sprite_manager);
        cBeetleBarrage(XmlAttributes& attributes, cSprite_Manager* p_sprite_manager);
        virtual ~cBeetleBarrage();

        virtual void DownGrade(bool force = false);
        void Set_Moving_State(Moving_state new_state);
        void Set_Active_Range(float range);
        float Get_Active_Range();
        void Set_Beetle_Interval(float time);
        float Get_Beetle_Interval();
        void Set_Beetle_Spit_Count(int count);
        int Get_Beetle_Spit_Count();
        void Set_Beetle_Fly_Distance(float distance);
        float Get_Beetle_Fly_Distance();

        virtual Col_Valid_Type Validate_Collision(cSprite* p_obj);
        virtual void Handle_Collision_Player(cObjectCollision* p_collision);
        virtual void Handle_Collision_Enemy(cObjectCollision* p_collision);
        virtual void Handle_Collision_Massive(cObjectCollision* p_collision);

        virtual cBeetleBarrage* Copy() const;
        virtual void Draw(cSurface_Request* p_request = NULL);
        virtual void Update();

        virtual void Editor_Activate();

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "BeetleBarrage"), &Scripting::rtTSC_Scriptable, this));
        }

        virtual xmlpp::Element* Save_To_XML_Node(xmlpp::Element* p_element);

    protected:
        virtual std::string Get_XML_Type_Name();

    private:
        // Constructor common stuff
        void Init();

        /* Map m_active_range onto the current position for checking
         * whether Alex is there. For performance reasons we don’t
         * return or reference it here, but set an instance member
         * m_active_area.
         * Depending on whether you want to calculate the circle for
         * the current or for the starting point coords, pass the
         * respective coordinates.
         */
        void Calculate_Active_Area(const float& x, const float& y);
        // The point beetles start flying upwards from (in the plant)
        void Calculate_Fly_Start(const cBeetle* p_beetle, float& x, float& y);

        void Generate_Beetles();

        // Editor event handlers
        bool Editor_Fly_Distance_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Range_Text_Changed(const CEGUI::EventArgs& event);
        bool Editor_Spit_Count_Text_Changed(const CEGUI::EventArgs& event);

        // The area we react on Alex.
        float m_active_range;
        GL_Circle m_active_area;
        float m_beetle_interval;
        float m_beetle_interval_counter;
        float m_spitting_beetles_counter;
        bool m_is_spitting_out_beetles;
        int m_beetle_spit_count;
        float m_beetle_fly_distance;
    };

}

#endif

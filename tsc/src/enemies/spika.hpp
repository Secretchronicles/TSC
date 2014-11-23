/***************************************************************************
 * spika.h
 *
 * Copyright © 2006 - 2011 Florian Richter
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

#ifndef TSC_SPIKA_HPP
#define TSC_SPIKA_HPP

#include "../enemies/enemy.hpp"
#include "../scripting/objects/enemies/mrb_spika.hpp"

namespace TSC {

    /* *** *** *** *** *** *** cSpika *** *** *** *** *** *** *** *** *** *** *** */
    /* Waits for you and wheel you if you can't run fast enough :D
     *
     * Colors : Orange, Green and Grey
     */
    class cSpika : public cEnemy {
    public:
        // constructor
        cSpika(cSprite_Manager* sprite_manager);
        // create from stream
        cSpika(XmlAttributes& attributes, cSprite_Manager* sprite_manager);
        // destructor
        virtual ~cSpika(void);

        // init defaults
        void Init(void);
        // copy
        virtual cSpika* Copy(void) const;

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "Spika"), &Scripting::rtTSC_Scriptable, this));
        }


        // set color
        void Set_Color(DefaultColor col);

        /* downgrade state ( if already weakest state : dies )
         * force : usually dies or a complete downgrade
        */
        virtual void DownGrade(bool force = 0);

        // update
        virtual void Update(void);

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

        // Color
        DefaultColor m_color_type;
        // default speed
        float m_speed;
        // default detection size
        float m_detection_size;

        // counter for walking
        float m_walk_count;

        // Save to XML node
        virtual xmlpp::Element* Save_To_XML_Node(xmlpp::Element* p_element);

    protected:

        virtual std::string Get_XML_Type_Name();
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

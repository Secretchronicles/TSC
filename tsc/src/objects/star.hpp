/***************************************************************************
 * star.h
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

#ifndef TSC_STAR_HPP
#define TSC_STAR_HPP

#include "../core/global_basic.hpp"
#include "../objects/powerup.hpp"
#include "../scripting/objects/powerups/mrb_star.hpp"

namespace TSC {

    /* *** *** *** *** *** Star *** *** *** *** *** *** *** *** *** *** *** *** */

    class cjStar : public cPowerUp {
    public:
        // constructor
        cjStar(cSprite_Manager* sprite_manager);
        // create from stream
        cjStar(XmlAttributes& attributes, cSprite_Manager* sprite_manager);
        // destructor
        virtual ~cjStar(void);

        // init defaults
        void Init(void);
        // copy
        virtual cjStar* Copy(void) const;


        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "Star"), &Scripting::rtTSC_Scriptable, this));
        }

        // Activate the star
        void Activate(void);

        // update the Star
        virtual void Update(void);
        // draw the Star
        virtual void Draw(cSurface_Request* request = NULL);

        // Adds Star Particles
        void Generate_Particles(float x = 0.0f, float y = 0.0f, bool random = 1, unsigned int quota = 2) const;

        // collision with massive
        virtual void Handle_Collision_Massive(cObjectCollision* collision);

        // small stars animation counter
        float m_anim_counter;

        // glim animation modifier
        bool m_glim_mod;
        // glim animation counter
        float m_glim_counter;

        // node saving inherited

    protected:
        virtual std::string Get_XML_Type_Name()
        {
            return "jstar";
        }
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

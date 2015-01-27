/***************************************************************************
 * enemystopper.h
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

#ifndef TSC_ENEMYSTOPPER_HPP
#define TSC_ENEMYSTOPPER_HPP

#include "../core/global_basic.hpp"
#include "../objects/movingsprite.hpp"
#include "../scripting/objects/specials/mrb_enemy_stopper.hpp"

namespace TSC {

    /* *** *** *** *** *** cEnemyStopper *** *** *** *** *** *** *** *** *** *** *** *** */

    class cEnemyStopper : public cMovingSprite {
    public:
        // constructor
        cEnemyStopper(cSprite_Manager* sprite_manager);
        // create from stream
        cEnemyStopper(XmlAttributes& attributes, cSprite_Manager* sprite_manager);
        // destructor
        virtual ~cEnemyStopper(void);

        // init defaults
        void Init(void);
        // copy
        virtual cEnemyStopper* Copy(void) const;

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "Enemy_Stopper"), &Scripting::rtTSC_Scriptable, this));
        }

        // draw
        virtual void Draw(cSurface_Request* request = NULL);

        // if draw is valid for the current state and position
        virtual bool Is_Draw_Valid(void);

        // editor color
        Color m_editor_color;

    protected:
        // save to XML node inherited
        virtual std::string Get_XML_Type_Name()
        {
            return "";
        }
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

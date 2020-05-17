/***************************************************************************
 * powerup.h
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

#ifndef TSC_POWERUP_HPP
#define TSC_POWERUP_HPP

#include "../core/global_basic.hpp"
#include "../core/xml_attributes.hpp"
#include "../objects/movingsprite.hpp"
#include "../scripting/objects/powerups/mrb_powerup.hpp"
#include "../scripting/objects/powerups/mrb_berry.hpp"
#include "../scripting/objects/powerups/mrb_fireberry.hpp"
#include "../scripting/objects/powerups/mrb_cookie.hpp"

namespace TSC {

    /* *** *** *** *** *** cPowerUp *** *** *** *** *** *** *** *** *** *** *** *** */

    class cPowerUp : public cMovingSprite {
    public:
        // constructor
        cPowerUp(cSprite_Manager* sprite_manager);
        // destructor
        virtual ~cPowerUp(void);

        // load from savegame
        virtual void Load_From_Savegame(cSave_Level_Object* save_object);

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "Berry"), &Scripting::rtTSC_Scriptable, this));
        }

        /* draw
         * a spawned powerup doesn't draw in editor mode
        */
        virtual void Draw(cSurface_Request* request = NULL);

        // if update is valid for the current state
        virtual bool Is_Update_Valid(void);

        // Activate this powerup
        virtual void Activate(void);

        /* Validate the given collision object
         * returns 0 if not valid
         * returns 1 if an internal collision with this object is valid
         * returns 2 if the given object collides with this object (blocking)
        */
        virtual Col_Valid_Type Validate_Collision(cSprite* obj);
        // handle moved out of Level event
        virtual void Handle_out_of_Level(ObjectDirection dir);
        // collision from player
        virtual void Handle_Collision_Player(cObjectCollision* collision);

        float m_counter;

        // node saving inherited
    };

    /* *** *** *** *** *** cMushroom *** *** *** *** *** *** *** *** *** *** *** *** */

    class cMushroom : public cPowerUp {
    public:
        // constructor
        cMushroom(cSprite_Manager* sprite_manager);
        // create from stream
        cMushroom(XmlAttributes& attributes, cSprite_Manager* sprite_manager);
        // destructor
        virtual ~cMushroom(void);

        // init defaults
        void Init(void);
        // copy
        virtual cMushroom* Copy(void) const;

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "Berry"), &Scripting::rtTSC_Scriptable, this));
        }

        // Set the Mushroom Type
        void Set_Type(SpriteType new_type);

        // Activates the item
        virtual void Activate(void);

        // update
        virtual void Update(void);

        // collision with massive
        virtual void Handle_Collision_Massive(cObjectCollision* collision);
        // collision from a box
        virtual void Handle_Collision_Box(ObjectDirection cdirection, GL_rect* r2);

        // glim animation modifier
        bool m_glim_mod;

        // Save to XML node
        virtual xmlpp::Element* Save_To_XML_Node(xmlpp::Element* p_element);

    protected:
        virtual std::string Get_XML_Type_Name();
    };

    /* *** *** *** *** *** cFirePlant *** *** *** *** *** *** *** *** *** *** *** *** */

    class cFirePlant : public cPowerUp {
    public:
        // constructor
        cFirePlant(cSprite_Manager* sprite_manager);
        // create from stream
        cFirePlant(XmlAttributes& attributes, cSprite_Manager* sprite_manager);
        // destructor
        virtual ~cFirePlant(void);

        // init defaults
        void Init(void);
        // copy
        virtual cFirePlant* Copy(void) const;


        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "Fireberry"), &Scripting::rtTSC_Scriptable, this));
        }

        // Activates the item
        virtual void Activate(void);

        // update
        virtual void Update(void);

        float m_particle_counter;

        // node saving inherited

    protected:
        virtual std::string Get_XML_Type_Name()
        {
            return "fireplant";
        }
    };

    /* *** *** *** *** *** cMoon *** *** *** *** *** *** *** *** *** *** *** *** */

    class cMoon : public cPowerUp {
    public:
        // constructor
        cMoon(cSprite_Manager* sprite_manager);
        // create from stream
        cMoon(XmlAttributes& attributes, cSprite_Manager* sprite_manager);
        // destructor
        virtual ~cMoon(void);

        // init defaults
        void Init(void);
        // copy
        virtual cMoon* Copy(void) const;

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "Cookie"), &Scripting::rtTSC_Scriptable, this));
        }

        // Activates the item
        virtual void Activate(void);

        // update
        virtual void Update(void);

        float m_particle_counter;

        // node saving inherited

    protected:
        // save to stream
        // stream saving inherited
        virtual std::string Get_XML_Type_Name()
        {
            return "moon";
        }
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

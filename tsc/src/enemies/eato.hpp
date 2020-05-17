/***************************************************************************
 * eato.h
 *
 * Copyright © 2006 - 2011 Florian Richter
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

#ifndef TSC_EATO_HPP
#define TSC_EATO_HPP

#include "../enemies/enemy.hpp"
#include "../scripting/objects/enemies/mrb_eato.hpp"

namespace TSC {

    /* *** *** *** *** *** cEato *** *** *** *** *** *** *** *** *** *** *** *** */
    /* Eats your Butt !
     * Secret attack: Merges visually with the background and becomes an anti-alex mine.
    */
    class cEato : public cEnemy {
    public:
        // constructor
        cEato(cSprite_Manager* sprite_manager);
        // create from stream
        cEato(XmlAttributes& attributes, cSprite_Manager* sprite_manager);
        // destructor
        virtual ~cEato(void);

        // init defaults
        void Init(void);
        // copy
        virtual cEato* Copy(void) const;

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "Eato"), &Scripting::rtTSC_Scriptable, this));
        }

        // Set the image directory. `dir' must be relative to the pixmaps/
        // directory.
        void Set_Image_Dir(boost::filesystem::path dir);
        // Set direction
        virtual void Set_Direction(const ObjectDirection dir);

        /* downgrade state ( if already weakest state : dies )
         * force : usually dies or a complete downgrade
        */
        virtual void DownGrade(bool force = 0);
        // Special normal death animation
        virtual void Update_Normal_Dying(void);

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
        virtual void Handle_Collision_Massive(cObjectCollision* collision);

        // editor activation
        virtual void Editor_Activate(void);
        // editor direction option selected event
        bool Editor_Direction_Select(const CEGUI::EventArgs& event);
        // editor image dir text changed event
        bool Editor_Image_Dir_Text_Changed(const CEGUI::EventArgs& event);

        // image directory
        boost::filesystem::path m_img_dir;

        // Save to XML node
        virtual xmlpp::Element* Save_To_XML_Node(xmlpp::Element* p_element);
        virtual std::string Create_Name() const;

    protected:

        virtual std::string Get_XML_Type_Name();
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

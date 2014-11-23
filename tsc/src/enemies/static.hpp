/***************************************************************************
 * static.h
 *
 * Copyright © 2007 - 2011 Florian Richter
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

#ifndef TSC_STATIC_ENEMY_HPP
#define TSC_STATIC_ENEMY_HPP

#include "../enemies/enemy.hpp"
#include "../objects/path.hpp"
#include "../scripting/objects/enemies/mrb_static.hpp"

namespace TSC {

    /* *** *** *** *** *** *** cStaticEnemy *** *** *** *** *** *** *** *** *** *** *** */
    /* It doesn't move but will hit you if you touch it.
     */
    class cStaticEnemy : public cEnemy {
    public:
        // constructor
        cStaticEnemy(cSprite_Manager* sprite_manager);
        // create from stream
        cStaticEnemy(XmlAttributes& attributes, cSprite_Manager* sprite_manager);
        // destructor
        virtual ~cStaticEnemy(void);

        // init defaults
        void Init(void);
        /* late initialization
         * this needs linked objects to be already loaded
        */
        virtual void Init_Links(void);
        // copy
        virtual cStaticEnemy* Copy(void) const;

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "StaticEnemy"), &Scripting::rtTSC_Scriptable, this));
        }


        // Set the parent sprite manager
        virtual void Set_Sprite_Manager(cSprite_Manager* sprite_manager);

        // load from savegame
        virtual void Load_From_Savegame(cSave_Level_Object* save_object);
        // save to savegame
        virtual cSave_Level_Object* Save_To_Savegame(void);

        // Set the rotation speed
        void Set_Rotation_Speed(float speed);
        // Set the movement speed
        void Set_Speed(float speed);
        // Set the path identifier
        void Set_Path_Identifier(const std::string& path);

        /* downgrade state ( if already weakest state : dies )
         * force : usually dies or a complete downgrade
        */
        virtual void DownGrade(bool force = 0);

        // update
        virtual void Update(void);
        // draw
        virtual void Draw(cSurface_Request* request /* = NULL */);

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

        // leveleditor activation
        virtual void Editor_Activate(void);
        // editor image text changed event
        bool Editor_Image_Text_Changed(const CEGUI::EventArgs& event);
        // editor rotation speed text changed event
        bool Editor_Rotation_Speed_Text_Changed(const CEGUI::EventArgs& event);
        // editor path identifier text changed event
        bool Editor_Path_Identifier_Text_Changed(const CEGUI::EventArgs& event);
        // editor speed text changed event
        bool Editor_Speed_Text_Changed(const CEGUI::EventArgs& event);
        // editor fire resistant option selected event
        bool Editor_Fire_Resistant_Select(const CEGUI::EventArgs& event);
        // editor ice resistance text changed event
        bool Editor_Ice_Resistance_Text_Changed(const CEGUI::EventArgs& event);

        // rotation speed
        float m_rotation_speed;
        // movement speed if using path
        float m_speed;

        // path state if linked to a path
        cPath_State m_path_state;

        // Save to XML node
        virtual xmlpp::Element* Save_To_XML_Node(xmlpp::Element* p_element);
        virtual std::string Create_Name(void) const;

    protected:

        virtual std::string Get_XML_Type_Name();
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

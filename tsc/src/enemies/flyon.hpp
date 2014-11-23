/***************************************************************************
 * flyon.h
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

#ifndef TSC_FLYON_HPP
#define TSC_FLYON_HPP

#include "../enemies/enemy.hpp"
#include "../scripting/objects/enemies/mrb_flyon.hpp"

namespace TSC {

    /* *** *** *** *** *** cFlyon *** *** *** *** *** *** *** *** *** *** *** *** */
    /*  Eats your butt in the sky.
    */
    class cFlyon : public cEnemy {
    public:
        // constructor
        cFlyon(cSprite_Manager* sprite_manager);
        // create from stream
        cFlyon(XmlAttributes& attributes, cSprite_Manager* sprite_manager);
        // destructor
        virtual ~cFlyon(void);

        // initialize defaults
        void Init(void);
        // copy
        virtual cFlyon* Copy(void) const;

        // load from savegame
        virtual void Load_From_Savegame(cSave_Level_Object* save_object);
        // save to savegame
        virtual cSave_Level_Object* Save_To_Savegame(void);

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "Flyon"), &Scripting::rtTSC_Scriptable, this));
        }

        // Set the image directory. `dir' must be a relative
        // to the pixmaps/ directory.
        void Set_Image_Dir(boost::filesystem::path dir);
        // Set Direction
        virtual void Set_Direction(const ObjectDirection dir);
        // Set Max Distance
        void Set_Max_Distance(float nmax_distance);
        // Set the Speed
        void Set_Speed(float val);

        /* downgrade state ( if already weakest state : dies )
         * force : usually dies or a complete downgrade
        */
        virtual void DownGrade(bool force = 0);
        // special instant death animation
        virtual void Update_Normal_Dying(void);

        // set the moving state
        void Set_Moving_State(Moving_state new_state);

        // update
        virtual void Update(void);
        // draw
        virtual void Draw(cSurface_Request* request = NULL);

        // Returns the distance to the end position
        float Get_End_Distance(void) const;
        // update destination velocity from the speed
        void Update_Dest_Vel(void);

        // if draw is valid for the current state and position
        virtual bool Is_Draw_Valid(void);

        /* Validate the given collision object
         * returns 0 if not valid
         * returns 1 if an internal collision with this object is valid
         * returns 2 if the given object collides with this object (blocking)
        */
        virtual Col_Valid_Type Validate_Collision(cSprite* obj);
        // collision from player
        virtual void Handle_Collision_Player(cObjectCollision* collision);
        // flyon does not die in abyss
        virtual void Handle_out_of_Level(ObjectDirection dir);

        // editor activation
        virtual void Editor_Activate(void);
        // editor direction option selected event
        bool Editor_Direction_Select(const CEGUI::EventArgs& event);
        // editor image dir text changed event
        bool Editor_Image_Dir_Text_Changed(const CEGUI::EventArgs& event);
        // editor max distance text changed event
        bool Editor_Max_Distance_Text_Changed(const CEGUI::EventArgs& event);
        // editor speed text changed event
        bool Editor_Speed_Text_Changed(const CEGUI::EventArgs& event);

        // image directory
        boost::filesystem::path m_img_dir;

        // moving speed
        float m_speed;
        // destination direction velocity
        float m_dest_velx, m_dest_vely;
        // maximum distance from the startposition
        float m_max_distance;

        // time to wait until next approach
        float m_wait_time;
        // moving back to the original position
        bool m_move_back;

        // Save to XML node
        virtual xmlpp::Element* Save_To_XML_Node(xmlpp::Element* p_element);
        std::string Create_Name(void) const;

    protected:

        virtual std::string Get_XML_Type_Name();
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

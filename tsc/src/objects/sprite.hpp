/***************************************************************************
 * sprite.h
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

#ifndef TSC_SPRITE_HPP
#define TSC_SPRITE_HPP

#include "../core/global_game.hpp"
#include "../core/math/rect.hpp"
#include "../video/video.hpp"
#include "../core/collision.hpp"
#include "../scripting/scriptable_object.hpp"
#include "../scripting/scripting.hpp"
#include "../scripting/objects/sprites/mrb_sprite.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** cCollidingSprite *** *** *** *** *** *** *** *** *** *** */

    class cCollidingSprite: public Scripting::cScriptable_Object {
    public:
        cCollidingSprite(cSprite_Manager* sprite_manager);
        virtual ~cCollidingSprite(void);

        /// Set the parent sprite manager.
        virtual void Set_Sprite_Manager(cSprite_Manager* sprite_manager);

        // Handle collision data
        void Handle_Collisions(void);
        /* Create a collision object
         * base : base/origin object
         * col : colliding object ( needed for the collision direction )
         * valid_type : validation type
        */
        cObjectCollision* Create_Collision_Object(const cSprite* base, cSprite* col, Col_Valid_Type valid_type) const;
        /* Add a collision object to the collision list
         * returns true if successful
         * add_if_new : add the collision only if the collision object doesn't already collide with us and is deleted if it does
         */
        bool Add_Collision(cObjectCollision* collision, bool add_if_new = 0);
        /* Add the collision objects to the collision list
         * add_if_new : add each collision only if the collision object doesn't already collide with us and is deleted if it does
         */
        void Add_Collisions(cObjectCollisionType* col_list, bool add_if_new = 0);

        // Delete the given collision from the list
        void Delete_Collision(cObjectCollision* collision);
        // Delete the last collision
        void Delete_Last_Collision(void);
        /* Check if a collision is active in the given direction
         * and returns the collision object number else -1
         */
        int Is_Collision_In_Direction(const ObjectDirection dir) const;
        // Returns the first added collision
        inline cObjectCollision* Get_First_Collision(void) const
        {
            // no collision available
            if (m_collisions.empty()) {
                return NULL;
            }

            return *m_collisions.begin();
        };
        /* Returns the last added collision
         * if only_blocking is set only movement blocking collisions are returned
        */
        cObjectCollision* Get_Last_Collision(bool only_blocking = 0) const;
        // returns true if the given collision obj was found in the list
        bool Is_Collision_Included(const cSprite* obj) const;
        // Clear the Collision list
        void Clear_Collisions(void);

        // default collision handler
        virtual void Handle_Collision(cObjectCollision* collision);
        // collision from player
        virtual void Handle_Collision_Player(cObjectCollision* collision) {};
        // collision from an enemy
        virtual void Handle_Collision_Enemy(cObjectCollision* collision) {};
        // collision with massive
        virtual void Handle_Collision_Massive(cObjectCollision* collision) {};
        // collision with passive
        virtual void Handle_Collision_Passive(cObjectCollision* collision) {};
        // collision with lava
        virtual void Handle_Collision_Lava(cObjectCollision* collision) {};
        /**
         * \brief collision from a box below.
         *
         * This is called on your object when it
         * is on top of a box that is being activated currently; in 90%
         * of the cases `cdirection' will be DIR_BOTTOM as the box is below
         * you. Still, it moves around a little bit so you may also get
         * another direction.
        */
        virtual void Handle_Collision_Box(ObjectDirection cdirection, GL_rect* r2) {};

        // the parent sprite manager
        cSprite_Manager* m_sprite_manager;
        // object collision list
        cObjectCollision_List m_collisions;
    };

    /* *** *** *** *** *** *** *** cSprite *** *** *** *** *** *** *** *** *** *** */

    class cSprite : public cCollidingSprite {
    public:
        // constructor
        cSprite(cSprite_Manager* sprite_manager, const std::string type_name = "sprite");
        // create from stream
        cSprite(XmlAttributes& attributes, cSprite_Manager* sprite_manager, const std::string type_name = "sprite");
        // destructor
        virtual ~cSprite(void);

        // initialize defaults
        virtual void Init(void);
        /* late initialization
         * this needs linked objects to be already loaded
        */
        virtual void Init_Links(void) {};
        // copy this sprite
        virtual cSprite* Copy(void) const;

        /// Save the level below the given XML node.
        virtual xmlpp::Element* Save_To_XML_Node(xmlpp::Element* p_element);

        // load from savegame
        virtual void Load_From_Savegame(cSave_Level_Object* save_object) {};
        // save to savegame
        virtual cSave_Level_Object* Save_To_Savegame(void)
        {
            return NULL;
        };

        /// Sets the image for drawing
        virtual void Set_Image(cGL_Surface* new_image, bool new_start_image = 0, bool del_img = 0);

        // Set the sprite type
        void Set_Sprite_Type(SpriteType type);

        /* Set if the camera should be ignored
         * default : disabled
        */
        void Set_Ignore_Camera(bool enable = 0);
        /* set if spawned
         * if set it is not saved in the level/world file
        */
        virtual void Set_Spawned(bool enable = 0)
        {
            m_spawned = enable;
        };

        // Sets the Position
        void Set_Pos(float x, float y, bool new_startpos = 0);
        void Set_Pos_X(float x, bool new_startpos = 0);
        void Set_Pos_Y(float y, bool new_startpos = 0);
        // Set if active
        virtual void Set_Active(bool enabled);
        /* Set the shadow
         * if position is set to 0 the shadow is disabled
        */
        inline void Set_Shadow(const Color& shadow, const float pos)
        {
            Set_Shadow_Pos(pos);
            Set_Shadow_Color(shadow);
        };
        /* Set the shadow position
         * if set to 0 the shadow is disabled
        */
        inline void Set_Shadow_Pos(const float pos)
        {
            m_shadow_pos = pos;
        };
        // Set the shadow color
        inline void Set_Shadow_Color(const Color& shadow)
        {
            m_shadow_color = shadow;
        };
        // Set image color
        inline void Set_Color(const Uint8 red, const Uint8 green, const Uint8 blue, const Uint8 alpha = 255)
        {
            m_color.red = red;
            m_color.green = green;
            m_color.blue = blue;
            m_color.alpha = alpha;
        };
        inline void Set_Color(const Color& col)
        {
            m_color = col;
        };

        /// Set a Color Combination ( GL_ADD, GL_MODULATE or GL_REPLACE )
        void Set_Color_Combine(const float red, const float green, const float blue, const GLint com_type);

        /** Set if rotation affects the collision rect
         * only supports 90° steps currently
         * if enabled col_pos, col_rect and rect must be reset manually before changing rotation
        */
        inline void Set_Rotation_Affects_Rect(bool enable = 0)
        {
            m_rotation_affects_rect = enable;
        };
        /** Set the rect rotation
         * does not reset col_pos, col_rect and rect before rotation
         * default : disabled
        */
        inline void Update_Rect_Rotation(void)
        {
            // Z must be first for correct rotation ( see Eato )
            Update_Rect_Rotation_Z();
            Update_Rect_Rotation_X();
            Update_Rect_Rotation_Y();
        };
        // Set the X rect rotation
        inline void Update_Rect_Rotation_X(void)
        {
            // mirror
            if (m_rot_x >= 180.0f) {
                m_col_pos.m_y = m_rect.m_h - (m_col_rect.m_h + m_col_pos.m_y);
            }
        };
        // Set the Y rect rotation
        inline void Update_Rect_Rotation_Y(void)
        {
            // mirror
            if (m_rot_y >= 180.0f) {
                m_col_pos.m_x = m_rect.m_w - (m_col_rect.m_w + m_col_pos.m_x);
            }
        }
        // Set the Z rect rotation
        void Update_Rect_Rotation_Z(void);

        // Set the X rotation
        void Set_Rotation_X(float rot, bool new_start_rot = 0);
        // Set the Y rotation
        void Set_Rotation_Y(float rot, bool new_start_rot = 0);
        // Set the Z rotation
        void Set_Rotation_Z(float rot, bool new_start_rot = 0);
        // Set the rotation
        inline void Set_Rotation(float x, float y, float z, bool new_start_rot = 0)
        {
            Set_Rotation_X(x, new_start_rot);
            Set_Rotation_Y(y, new_start_rot);
            Set_Rotation_Z(z, new_start_rot);
        };

        // Add X rotation
        inline void Add_Rotation_X(float rot)
        {
            Set_Rotation_X(m_rot_x + rot);
        };
        // Add Y rotation
        inline void Add_Rotation_Y(float rot)
        {
            Set_Rotation_Y(m_rot_y + rot);
        };
        // Add Z rotation
        inline void Add_Rotation_Z(float rot)
        {
            Set_Rotation_Z(m_rot_z + rot);
        };
        // Add rotation
        inline void Add_Rotation(float x, float y, float z)
        {
            Set_Rotation_X(m_rot_x + x);
            Set_Rotation_Y(m_rot_y + y);
            Set_Rotation_Z(m_rot_z + z);
        };

        /**
         * Set if scale affects the collision rect.
         * default : disabled
         *
         * \param enable
         *   if enabled previous scale is always undone from rect before setting the new value
        */
        inline void Set_Scale_Affects_Rect(const bool enable = 0)
        {
            m_scale_affects_rect = enable;
        };
        /* Set which directions of the image get scaled
         * if all are set scaling is centered and if all are not set scaling is disabled
         * todo : if enabled with scale_affects_rect it should also scale the rect position
        */
        inline void Set_Scale_Directions(const bool up = 0, const bool down = 1, const bool left = 0, const bool right = 1)
        {
            m_scale_up = up;
            m_scale_down = down;
            m_scale_left = left;
            m_scale_right = right;
        };
        // Set the scale
        void Set_Scale_X(const float scale, const bool new_startscale = 0);
        void Set_Scale_Y(const float scale, const bool new_startscale = 0);
        inline void Set_Scale(const float scale, const bool new_startscale = 0)
        {
            Set_Scale_X(scale, new_startscale);
            Set_Scale_Y(scale, new_startscale);
        };

        // Add scale
        inline void Add_Scale_X(const float val)
        {
            Set_Scale_X(m_scale_x + val);
        };
        inline void Add_Scale_Y(const float val)
        {
            Set_Scale_Y(m_scale_y + val);
        };
        inline void Add_Scale(const float val)
        {
            Set_Scale_X(m_scale_x + val);
            Set_Scale_Y(m_scale_y + val);
        };

        // Set this sprite on top of the given one
        void Set_On_Top(const cSprite* sprite, bool optimize_hor_pos = 1);

        /**
         * Return the object that will be placed in the UIDS hash
         * for this sprite. Be sure to override in relevant subclasses.
         * See mrb_uids.cpp for a lengthy explanation.
         */
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "Sprite"), &Scripting::rtTSC_Scriptable, this));
        }

        /* Move this object
         * real : if set the speedfactor is not used
        */
        virtual void Move(float move_x, float move_y, const bool real = 0);

        // default collision and movement handling
        virtual void Collide_Move(void) {};

        // Update the position rect values
        void Update_Position_Rect(void);
        // default update
        virtual void Update(void) {};
        /* late update
         * use if it is needed that other objects are already updated
        */
        virtual void Update_Late(void) {};
        // update drawing validation
        virtual void Update_Valid_Draw(void);
        // update updating validation
        virtual void Update_Valid_Update(void);

        /* Draw
        * if request is NULL automatically creates the request
        */
        virtual void Draw(cSurface_Request* request = NULL);
        /* draws the image only.
         * using start/editor data if editor is enabled.
        */
        void Draw_Image(cSurface_Request* request = NULL) const;
        /* draws the image only.
         * doesn't check if draw is valid and requires a set drawing request
         * does not use start/editor data even if editor is enabled.
        */
        void Draw_Image_Normal(cSurface_Request* request = NULL) const;
        /* draws the image only.
         * doesn't check if draw is valid and requires a set drawing request
         * uses start/editor data even if editor is disabled.
        */
        void Draw_Image_Editor(cSurface_Request* request = NULL) const;

        /// Set the massive type.
        virtual void Set_Massive_Type(MassiveType type);

        // Check if this sprite is on top of the given object
        bool Is_On_Top(const cSprite* obj) const;

        // if the sprite is visible on the screen
        bool Is_Visible_On_Screen(void) const;
        // if this is in range of the maximum camera distance
        bool Is_In_Range(void) const;
        // if update is valid for the current state
        virtual bool Is_Update_Valid();
        // if draw is valid for the current state and position
        virtual bool Is_Draw_Valid(void);

        // returns true if this is a basic sprite type
        inline bool Is_Basic_Sprite(void) const
        {
            // TODO: There are no basic sprites anymore due to code cleanup.
            // Instead, we need a new cSprite subclass cStaticSprite that
            // is specifically intended for just displaying images.
            /*
            if( m_type == TYPE_PASSIVE || m_type == TYPE_FRONT_PASSIVE || m_type == TYPE_MASSIVE || m_type == TYPE_CLIMBABLE || m_type == TYPE_HALFMASSIVE )
            {
                return 1;
            }
            */

            return 0;
        };
        // returns true if this is a sprite that is in the sprite manager
        inline bool Is_Sprite_Managed(void) const
        {
            if (m_sprite_array == ARRAY_MASSIVE || m_sprite_array == ARRAY_PASSIVE || m_sprite_array == ARRAY_ENEMY || m_sprite_array == ARRAY_ACTIVE || m_sprite_array == ARRAY_LAVA) {
                return true;
            }

            return false;
        };

        /* set this sprite to destroyed and completely disable it
         * sprite is still in the sprite manager but only to get possibly replaced
        */
        virtual void Destroy(void);

        // editor add window object
        void Editor_Add(const CEGUI::String& name, const CEGUI::String& tooltip, CEGUI::Window* window_setting, float obj_width, float obj_height = 28, bool advance_row = 1);
        // editor activation
        virtual void Editor_Activate(void);
        // editor deactivation
        virtual void Editor_Deactivate(void);
        // editor init
        virtual void Editor_Init(void);
        // editor position update
        virtual void Editor_Position_Update(void);
        // editor state update
        virtual void Editor_State_Update(void) {};

        // editor image text changed event
        bool Editor_Image_Text_Changed(const CEGUI::EventArgs& event);

        /// current image used for drawing
        cGL_Surface* m_image;
        /// editor and first image
        cGL_Surface* m_start_image;

        /// complete image rect
        GL_rect m_rect;
        /// editor and first image rect
        GL_rect m_start_rect;
        /// collision rect
        GL_rect m_col_rect;
        /// collision start point
        GL_point m_col_pos;

        /// current position
        float m_pos_x;
        float m_pos_y;
        float m_pos_z;
        /// start position
        float m_start_pos_x;
        float m_start_pos_y;
        /** editor z position
         * it's only used if not 0
        */
        float m_editor_pos_z;

        /// if set rotation not only affects the image but also the rectangle
        bool m_rotation_affects_rect;
        /// editor and start rotation
        float m_start_rot_x;
        float m_start_rot_y;
        float m_start_rot_z;
        /// X rotation. Can only be "0" (no rotation) or "180" (mirror on X axis).
        float m_rot_x;
        /// Y rotation. Can only be "0" (no rotation) or "180" (mirror on Y axis).
        float m_rot_y;
        /// Z rotation. The real rotation on the Z axis, which is what one would
        /// expect to be the "rotation". Can be anything 0 <= r <= 360.
        float m_rot_z;
        // FIXME: m_rot_x and m_rot_y should probably be renamed
        // to m_mirror_x and m_mirror_y, m_rot_z should be renamed
        // to m_rot(ation).

        /// if set scale not only affects the image but also the rectangle
        bool m_scale_affects_rect;
        /** which parts of the image get scaled
         * if all are set scaling is centered
        */
        bool m_scale_up;
        bool m_scale_down;
        bool m_scale_left;
        bool m_scale_right;
        /// editor and start scale
        float m_start_scale_x;
        float m_start_scale_y;
        /// scale
        float m_scale_x;
        float m_scale_y;

        /// color
        Color m_color;
        /// combine type
        GLint m_combine_type;
        /// combine color
        float m_combine_color[3];

        /// sprite type
        SpriteType m_type;
        /// internal type name
        const std::string m_type_name;
        /// sprite array type
        ArrayType m_sprite_array;
        /// massive collision type
        MassiveType m_massive_type;

        /// sprite editor tags
        std::string m_editor_tags;

        /// true if not using the camera position
        bool m_no_camera;
        /// if true we are active and can be updated and drawn
        bool m_active;
        /// if spawned
        bool m_spawned;
        /// maximum distance to the camera to get updated
        unsigned int m_camera_range;
        /// can be used as ground object
        bool m_can_be_ground;

        /// delete the given image when it gets unloaded
        bool m_delete_image;
        /// if this can not be auto-deleted because the object is controlled from elsewhere
        bool m_disallow_managed_delete;
        /** if true this sprite is not used anywhere anymore
         * and is ready to be replaced with a new sprite
         * should not be used for objects needed by the editor
         * should be used for not active spawned objects
        */
        bool m_auto_destroy;
        /// shadow position
        float m_shadow_pos;
        /// shadow color
        Color m_shadow_color;

        /// if drawing is valid
        bool m_valid_draw;
        /// if updating is valid
        bool m_valid_update;

        /// editor active window list
        typedef vector<cEditor_Object_Settings_Item*> Editor_Object_Settings_List;
        Editor_Object_Settings_List m_editor_windows;
        /// width for all name windows based on largest name text width
        float m_editor_window_name_width;

        /// ID to uniquely identify this sprite (UIDS[idhere] uses this)
        int m_uid;

        static const float m_pos_z_passive_start; ///< Start Z position for passive elements
        static const float m_pos_z_massive_start; ///< Start Z position for massive elements
        static const float m_pos_z_front_passive_start; ///< Start Z position for front passive elements
        static const float m_pos_z_halfmassive_start; ///< Start Z position for half-massive elements
        static const float m_pos_z_player; // Z position for the level player
        static const float m_pos_z_delta; // Minimum possible Z difference (i.e. one Z step).

        /// Name as shown in the editor.
        virtual std::string Create_Name() const;

    protected:
        /// visible main name component for the user.
        /// Additions such as direction are added behind this.
        std::string m_name;

        /// XML type property.
        virtual std::string Get_XML_Type_Name();
    };

    typedef vector<cSprite*> cSprite_List;

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

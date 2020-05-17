/***************************************************************************
 * layer.h
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

#ifndef TSC_WORLD_LAYER_HPP
#define TSC_WORLD_LAYER_HPP

#include "../core/global_basic.hpp"
#include "../objects/movingsprite.hpp"
#include "../core/obj_manager.hpp"
#include "../overworld/world_waypoint.hpp"

namespace TSC {

    /* *** *** *** *** *** *** Layer_Line *** *** *** *** *** *** *** *** *** *** *** */

// Layer line point class
// TODO : create a real basic/virtual sprite ?
    class cLayer_Line_Point : public cSprite {
    public:
        // constructor
        cLayer_Line_Point(cSprite_Manager* sprite_manager, cOverworld* overworld, SpriteType new_type);
        // destructor
        virtual ~cLayer_Line_Point(void);

        virtual xmlpp::Element* Save_To_XML_Node(xmlpp::Element* p_element);

        // draw
        virtual void Draw(cSurface_Request* request = NULL);

        /* set this sprite to destroyed and completely disable it
         * sprite is still in the sprite manager but only to get possibly replaced
        */
        virtual void Destroy(void);

        // Get center position x
        float Get_Line_Pos_X(void) const;
        // Get center position y
        float Get_Line_Pos_Y(void) const;

        // parent overworld
        cOverworld* m_overworld;
        // the linked start/end point
        cLayer_Line_Point* m_linked_point;
    };

// Layer line start point class
    class cLayer_Line_Point_Start : public cLayer_Line_Point {
    public:
        // constructor
        cLayer_Line_Point_Start(cSprite_Manager* sprite_manager, cOverworld* overworld);
        // destructor
        virtual ~cLayer_Line_Point_Start(void);
        // create from stream
        cLayer_Line_Point_Start(XmlAttributes& attributes, cSprite_Manager* sprite_manager, cOverworld* overworld);

        // init defaults
        void Init(void);

        // copy (end point can not be copied)
        virtual cLayer_Line_Point_Start* Copy(void) const;

        // Set the parent sprite manager
        virtual void Set_Sprite_Manager(cSprite_Manager* sprite_manager);

        // Draw
        virtual void Draw(cSurface_Request* request = NULL);

        // return a normal line
        GL_line Get_Line(void) const;

        /* Returns the Waypoint on the end of the line(s)
         * if the line continues on another line it is followed to the end
        */
        cWaypoint* Get_End_Waypoint(void) const;
        cWaypoint* Get_Start_Waypoint(void) const;
        /* Returns the waypoint for the line point with/without the
         * given UID. Also follows concatenated lines.
         * Do not use these functions with UIDS of conncetion points between
         * multi-element lines.
        */
        cWaypoint* Get_Waypoint_for_UID(int uid) const;
        cWaypoint* Get_Opposite_Waypoint_for_UID(int uid) const;

        // editor activation
        virtual void Editor_Activate(void);
        // editor origin text changed event
        bool Editor_Origin_Text_Changed(const CEGUI::EventArgs& event);

        /* animation type
         * 0 = normal walking, 1 = swimming
         */
        unsigned int m_anim_type;
        // waypoint origin identifier
        unsigned int m_origin;
    };

    /* *** *** *** *** *** *** *** Layer *** *** *** *** *** *** *** *** *** *** */

// Layer Line Collision data
    class cLine_collision {
    public:
        cLine_collision(void);

        // nearest line pointer
        cLayer_Line_Point_Start* m_line;
        // nearest line number
        int m_line_number;
        // position difference
        float m_difference;
    };

    typedef vector<cLayer_Line_Point_Start*> LayerLineList;

// Layer class
// handles the line collision detection
    class cLayer : public cObject_Manager<cLayer_Line_Point_Start> {
    public:
        cLayer(cOverworld* origin);
        virtual ~cLayer(void);

        // Add a layer line
        virtual void Add(cLayer_Line_Point_Start* line_point);

        // Save to file, raises xmlpp::exception on failure
        void Save_To_File(const boost::filesystem::path& filename);

        // Delete all objects
        virtual void Delete_All(void);

        /* Returns the colliding Line start point
         * if not found returns NULL
        */
        cLayer_Line_Point_Start* Get_Line_Collision_Start(const GL_rect& line_rect);
        /* Returns the colliding Line from the given position and the added direction size
         * if not found returns a NULL line in the class
        */
        cLine_collision Get_Line_Collision_Direction(float x, float y, ObjectDirection dir, float dir_size = 15, unsigned int check_size = 10) const;
        /* Returns the start point of a line with a point matching
         * the given uid
        */
        cLayer_Line_Point_Start* Get_Line_Start_By_UID(int uid);

        /* Return the collision data between the nearest line and the given position
         * check_size is maximum size for both direction checking lines
         * if only_origin_id is set only checks lines with the given id
        */
        cLine_collision Get_Nearest(float x, float y, ObjectDirection dir = DIR_HORIZONTAL, unsigned int check_size = 15, int only_origin_id = -1) const;
        // Return the collision data between the given line and position
        cLine_collision Get_Nearest_Line(cLayer_Line_Point_Start* map_layer_line, float x, float y, ObjectDirection dir = DIR_HORIZONTAL, unsigned int check_size = 15) const;

        // parent overworld
        cOverworld* m_overworld;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

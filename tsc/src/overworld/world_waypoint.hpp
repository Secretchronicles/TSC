/***************************************************************************
 * waypoint.h
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

#ifndef TSC_WORLD_WAYPOINT_HPP
#define TSC_WORLD_WAYPOINT_HPP

#include "../core/global_basic.hpp"
#include "../video/video.hpp"
#include "../objects/movingsprite.hpp"

namespace TSC {

    /* *** *** *** *** *** Waypoint types *** *** *** *** *** *** *** *** *** *** *** *** */

    enum Waypoint_type {
        WAYPOINT_NORMAL = 1,
        WAYPOINT_WORLD_LINK = 2 // Enters another World
    };

    /**
     * This struct encapsulates information about a possibility
     * to leave a waypoint on the world map.
     */
    struct waypoint_exit
    {
        /// Leave direction (for mapping the user's key presses)
        ObjectDirection direction;
        /// Connected level exit in the waypoint's associated level
        /// that unlocked this waypoint exit if taken.
        std::string level_exit_name;
        /// UID of the start point of the line which the player
        /// shall move to if this exit is chosen.
        int line_start_uid;
        /// Whether or not this exit is locked. Unlocking requires
        /// finding the level exit named with `level_exit_name`.
        bool locked;
    };

    /* *** *** *** *** *** *** cWaypoint *** *** *** *** *** *** *** *** *** *** *** */

    class cWaypoint : public cSprite {
    public:
        // constructor
        cWaypoint(cSprite_Manager* sprite_manager);
        // create from stream
        cWaypoint(XmlAttributes& attributes, cSprite_Manager* sprite_manager);
        // destructor
        virtual ~cWaypoint(void);

        // Init defaults
        void Init(void);

        // copy this object
        virtual cWaypoint* Copy(void) const;

        // Update
        virtual void Update(void);
        // Draw
        virtual void Draw(cSurface_Request* request = NULL);

        // Set direction forward (DEPRECATED)
        void Set_Direction_Forward(ObjectDirection direction);
        // Set direction backward (DEPRECATED)
        void Set_Direction_Backward(ObjectDirection direction);
        // Set Access
        void Set_Access(bool enabled, bool new_start_access = 0);
        // Unlock an exit of this waypoint as denoted by the level exit name passed.
        // Returns whether the exit was unlocked successfuly (will
        // fail if there is no exit with the given name).
        bool Unlock_Exit(std::string exit_name);
        // Unlock all exits of this waypoint.
        void Unlock_All_Exits();

        // Set the Destination, either a level or world name (NOT a path!)
        void Set_Destination(std::string level_or_worldname);
        // Returns the Destination's level or world name (no path)
        std::string Get_Destination() const;
        // Returns the Destination’s full filename (world or level file).
        boost::filesystem::path Get_Destination_Path();

        // editor activation
        virtual void Editor_Activate(void);
        // editor type option selected event
        bool Editor_Type_Select(const CEGUI::EventArgs& event);
        // editor destination text changed event
        bool Editor_Destination_Text_Changed(const CEGUI::EventArgs& event);
        // editor access option selected event
        bool Editor_Access_Select(const CEGUI::EventArgs& event);
        // editor direction backward option selected event
        bool Editor_Backward_Direction_Select(const CEGUI::EventArgs& event);
        // editor direction forward option selected event
        bool Editor_Forward_Direction_Select(const CEGUI::EventArgs& event);
        // a waypoint exit was selected in the combobox
        bool Editor_Waypoint_Exit_Select(const CEGUI::EventArgs& event);
        // a new waypoint exit was requested
        bool Editor_Waypoint_New_Exit_Clicked(const CEGUI::EventArgs& event);
        // deletion of selected waypoint exit was requested
        bool Editor_Waypoint_Delete_Exit_Clicked(const CEGUI::EventArgs& event);
        // direction of the exit was changed
        bool Editor_Waypoint_Exit_Direction_Select(const CEGUI::EventArgs& event);
        // mapped level name was changed
        bool Editor_Waypoint_Exit_Level_Exit_Name_Changed(const CEGUI::EventArgs& event);
        // UID of start line point was changed
        bool Editor_Waypoint_Exit_Line_Start_UID_Changed(const CEGUI::EventArgs& event);
        // default lock state was changed
        bool Editor_Waypoint_Exit_Locked_Select(const CEGUI::EventArgs& event);

        // forward direction (DEPRECATED)
        ObjectDirection m_direction_forward;
        // backward direction (DEPRECATED)
        ObjectDirection m_direction_backward;

        /* The Waypoint type
         * see the definitions
        */
        Waypoint_type m_waypoint_type;
        // destination
        std::string m_destination;

        // if this waypoint is accessible (checked for drawing waypoint graphic)
        bool m_access;
        // the default access defined in the definition
        bool m_access_default;

        // color for the glim effect
        float m_glim_color;
        // glim effect type switch
        bool m_glim_mod;

        // arrow forward (DEPRECATED)
        cGL_Surface* m_arrow_forward;
        // arrow backward (DEPRECATED)
        cGL_Surface* m_arrow_backward;

        // All exits the user may take from this waypoint.
        std::vector<waypoint_exit> m_exits;

        CEGUI::Combobox* mp_wp_exits_box;
        CEGUI::Combobox* mp_wp_exit_dir_box;
        CEGUI::Combobox* mp_wp_exit_lock_box;
        CEGUI::Editbox* mp_wp_exit_name_edit;
        CEGUI::Editbox* mp_wp_exit_uid_edit;

        // Save to node
        virtual xmlpp::Element* Save_To_XML_Node(xmlpp::Element* p_element);
    protected:
        virtual std::string Get_XML_Type_Name();
    private:
        void rebuild_waypoint_exit_list();
        void update_exit_widgets();
        waypoint_exit* get_edited_waypoint_exit();
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

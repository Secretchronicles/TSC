/***************************************************************************
 * save_level.cpp - Handler for saving a cLevel instance to a savegame.
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

#ifndef TSC_SAVEGAME_SAVE_LEVEL_HPP
#define TSC_SAVEGAME_SAVE_LEVEL_HPP
#include "../../objects/sprite.hpp"

namespace TSC {
    // This is used in the scripting API's Save event for storing the
    // data a script writer needs to save. Format:
    //     key => [typename, value_as_string]
    typedef std::map<std::string, std::pair<std::string, std::string>> Script_Data;

    /* *** *** *** *** *** *** *** cSave_Level_Object_Property *** *** *** *** *** *** *** *** *** *** */
    /**
    * Legacy class that holds the diff for a single attribute
    * of a regular level object. See the description of
    * cSave_Level_Object for why this class still exists (backward
    * compatibility only).
    *
    * This class is only used for level loading still, level saving uses
    * a newer mechanism already that doesn’t need it.
    */
    class cSave_Level_Object_Property {
    public:
        cSave_Level_Object_Property(const std::string& new_name = "", const std::string& new_value = "");
        std::string m_name;
        std::string m_value;
    };
    typedef vector<cSave_Level_Object_Property> Save_Level_Object_ProprtyList;

    /* *** *** *** *** *** *** *** cSave_Level_Object *** *** *** *** *** *** *** *** *** *** */
    /**
     * Legacy class for loading regular level objects from diffs.
     * The old save system stored only diffs in the savegame files for
     * the level objects that can be loaded from the XML, but due to
     * backward compatibility we can’t get rid of this class currently,
     * because using a newer format that saves regular level objects just
     * like spawned objects we’d have to change the level save format.
     *
     * This class is only used for level loading still, level saving uses
     * a newer mechanism already that doesn’t need it.
     */
    class cSave_Level_Object {
    public:
        cSave_Level_Object(void);
        ~cSave_Level_Object(void);
        // Check if property exists
        bool exists(const std::string& val_name);
        // Returns the value
        std::string Get_Value(const std::string& val_name);
        SpriteType m_type;
        // object properties
        Save_Level_Object_ProprtyList m_properties;
    };
    typedef vector<cSave_Level_Object*> Save_Level_ObjectList;

    /* *** *** *** *** *** *** *** cSave_Level *** *** *** *** *** *** *** *** *** *** */
    /**
     * Represents a cLevel instance in the savegame containing a list of
     * all spawned sprites. Note a single savegame may include multiple
     * levels when a user saves while or after he visited a sublevel
     */
    class cSave_Level {
    public:
        cSave_Level(void);
        ~cSave_Level(void);

        void Save_To_Node(xmlpp::Element* p_parent_node);

        std::string m_name;
        /// True if this is the active level.
        bool is_active;
        // player position is only set if level is the active one
        float m_level_pos_x;
        float m_level_pos_y;

        /// List of objects that originate from the level XML.
        std::vector<const cSprite*> m_regular_objects;
        /// List of spawned objects (i.e. not from the level XML).
        /// TODO: Should probably be list of const cSprite* also.
        cSprite_List m_spawned_objects; // TODO: Should be std::vector<const cSprite*>, however, during loading this is filled via new() (in contrast to saving)
        /** Legacy list of objects that originate from the level XML.
         * This member is intended to be removed once we break the
         * save level format. It holds diffs rather than entire
         * objects and is used only during savegame level loading
         * still, savegame level saving already uses the proper new
         * m_regular_objects member above that holds complete cSprite
         * instances rather than diffs. */
        Save_Level_ObjectList m_level_objects;

        // Data a script writer wants to store
        // This is a list of key-value tables (one table per save event handler).
        std::vector<Script_Data> m_script_datas;
    };

    typedef vector<cSave_Level*> Save_LevelList;
}

#endif

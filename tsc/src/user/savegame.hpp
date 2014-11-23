/***************************************************************************
 * savegame.h
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

#ifndef TSC_SAVEGAME_HPP
#define TSC_SAVEGAME_HPP

#include "../objects/sprite.hpp"
#include "../scripting/scriptable_object.hpp"
#include "../scripting/objects/misc/mrb_level.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

#define SAVEGAME_VERSION 12
#define SAVEGAME_VERSION_UNSUPPORTED 5

    /* *** *** *** *** *** *** *** cSave_Overworld_Waypoint *** *** *** *** *** *** *** *** *** *** */
// Overworld Waypoint save data
    class cSave_Overworld_Waypoint {
    public:
        cSave_Overworld_Waypoint(void);
        ~cSave_Overworld_Waypoint(void);

        std::string m_destination;
        bool m_access;
    };

    typedef vector<cSave_Overworld_Waypoint*> Save_Overworld_WaypointList;


    /* *** *** *** *** *** *** *** cSave_Overworld *** *** *** *** *** *** *** *** *** *** */
// Overworld save data
    class cSave_Overworld {
    public:
        cSave_Overworld(void);
        ~cSave_Overworld(void);

        std::string m_name;

        // waypoint data
        Save_Overworld_WaypointList m_waypoints;
    };

    typedef vector<cSave_Overworld*> Save_OverworldList;

    /* *** *** *** *** *** *** *** cSave_Level_Object_Property *** *** *** *** *** *** *** *** *** *** */
// Level object string property
    class cSave_Level_Object_Property {
    public:
        cSave_Level_Object_Property(const std::string& new_name = "", const std::string& new_value = "");

        std::string m_name;
        std::string m_value;
    };

    typedef vector<cSave_Level_Object_Property> Save_Level_Object_ProprtyList;

    /* *** *** *** *** *** *** *** cSave_Level_Object *** *** *** *** *** *** *** *** *** *** */
// Level object save data
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
// Level save data
    class cSave_Level {
    public:
        cSave_Level(void);
        ~cSave_Level(void);

        std::string m_name;
        // player position is only set if level is the active one
        float m_level_pos_x;
        float m_level_pos_y;

        // objects data
        Save_Level_ObjectList m_level_objects;
        // spawned objects
        cSprite_List m_spawned_objects;

        // Data a script writer wants to store
        std::string m_mruby_data;
    };

    typedef vector<cSave_Level*> Save_LevelList;

    /* *** *** *** *** *** cSave_Player_Return_Entry *** *** *** *** *** *** *** *** */
    class cSave_Player_Return_Entry {
    public:
        cSave_Player_Return_Entry(const std::string& level, const std::string& entry);

        std::string m_level;
        std::string m_entry;
    };

    /* *** *** *** *** *** *** *** cSave *** *** *** *** *** *** *** *** *** *** */
    class cSave {
    public:
        /// Load a savegame from the given file. The returned cSave
        /// instance must be freed by you.
        static cSave* Load_From_File(boost::filesystem::path filepath);

        cSave(void);
        ~cSave(void);

        // Initialize data to empty values
        void Init(void);

        // return the active level if available
        std::string Get_Active_Level(void);

        // Write the savegame out to the given file; raises
        // xmlpp::exception on error.
        void Write_To_File(boost::filesystem::path filepath);

        // savegame version
        int m_version;
        // time ( seconds since 1970 )
        time_t m_save_time;
        // description
        std::string m_description;

        // lives
        unsigned int m_lives;
        // points
        long m_points;
        // goldpieces
        unsigned int m_goldpieces;
        // player type
        unsigned int m_player_type;
        // player's previous type (previous power before a temporary power)
        unsigned int m_player_type_temp_power;
        // player moving state
        unsigned int m_player_state;
        // item in the itembox
        unsigned int m_itembox_item;
        //Amount of time left for invincibility (from damage, star, or file load).  0 means not invincible.
        float m_invincible;
        //Amount of time left for star (used for graphic flash effect)
        float m_invincible_star;
        // time maryo is in ghost mode
        float m_ghost_time;
        // ghost mode drawing modifier
        float m_ghost_time_mod;

        // return stack entries
        std::vector<cSave_Player_Return_Entry> m_return_entries;

        // level data
        Save_LevelList m_levels;
        // level time
        Uint32 m_level_time;
        // level engine version
        int m_level_engine_version;

        /* active overworld
         * if not set game mode is custom level
        */
        std::string m_overworld_active;
        // current waypoint
        unsigned int m_overworld_current_waypoint;

        // overworld data
        Save_OverworldList m_overworlds;
    };

    /* *** *** *** *** *** *** *** cSavegame *** *** *** *** *** *** *** *** *** *** */

// TODO: Maybe this class should be removed entirely and merged with cSave?
    class cSavegame: public Scripting::cScriptable_Object {
    public:
        cSavegame(void);
        virtual ~cSavegame(void);

        /* Load a save
        * Previous progress should be reset before.
        * Returns:
        * 0 if failed
        * 1 if level save
        * 2 if overworld save
        */
        int Load_Game(unsigned int save_slot);
        // Save the game with the given description
        bool Save_Game(unsigned int save_slot, std::string description);

        /**
         * \brief Load a Save
         *
         * The returned object should be deleted if not used anymore.
         * Raises xmlpp exceptions or Errors::InvalidSavegameError when
         * something is wrong with the savegame file.
         */
        cSave* Load(unsigned int save_slot);

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            // See docs in mrb_level.cpp for why we associate ourself
            // with the Level class here instead of a savegame class.
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "LevelClass"), &Scripting::rtTSC_Scriptable, this));
        }

        /**
         * \brief Returns only the Savegame description.
         *
         * Raises the same exceptions as Load().
         */
        std::string Get_Description(unsigned int save_slot, bool only_description = 0);

        // Returns true if the Savegame is valid
        bool Is_Valid(unsigned int save_slot) const;

        // savegame directory
        boost::filesystem::path m_savegame_dir;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

// The Savegame Handler
    extern cSavegame* pSavegame;

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

/***************************************************************************
 * save.hpp  -  Main savegame save object
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

#ifndef TSC_SAVEGAME_SAVE_HPP
#define TSC_SAVEGAME_SAVE_HPP
#include "../../core/global_basic.hpp"
#include "save_level.hpp"
#include "save_overworld.hpp"

namespace TSC {

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
        // time alex is in ghost mode
        float m_ghost_time;
        // ghost mode drawing modifier
        float m_ghost_time_mod;

        // return stack entries
        std::vector<cSave_Player_Return_Entry> m_return_entries;

        // level data
        Save_LevelList m_levels;
        // level time
        uint32_t m_level_time;
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

}

#endif

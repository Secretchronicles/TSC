/***************************************************************************
 * savegame.h
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

#ifndef TSC_SAVEGAME_HPP
#define TSC_SAVEGAME_HPP

#include "../../objects/sprite.hpp"
#include "../../scripting/scriptable_object.hpp"
#include "../../scripting/objects/misc/mrb_level.hpp"
#include "save.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

#define SAVEGAME_VERSION 12
#define SAVEGAME_VERSION_UNSUPPORTED 5

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

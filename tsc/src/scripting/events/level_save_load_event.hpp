/***************************************************************************
 * level_save_load_event.hpp
 *
 * Copyright © 2012-2020 The TSC Contributors
 ***************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TSC_SCRIPTING_LEVEL_SAVE_LOAD_EVENT_HPP
#define TSC_SCRIPTING_LEVEL_SAVE_LOAD_EVENT_HPP
#include "../scripting.hpp"
#include "../../user/savegame/save_level.hpp"
#include "event.hpp"

namespace TSC {
    namespace Scripting {
        class cLevel_SaveLoad_Event: public cEvent {
        public:
            cLevel_SaveLoad_Event(bool is_save);
            virtual std::string Event_Name();
            std::vector<Script_Data> Get_Storage();
            void Set_Storage(const std::vector<Script_Data>& storage);
        protected:
            virtual void Run_MRuby_Callback(cMRuby_Interpreter* p_mruby, mrb_value callback);
        private:
            bool m_is_save;
            std::vector<Script_Data> m_cpp_storage;
        };
    }
}
#endif

/***************************************************************************
 * scriptable_object.hpp - Baseclass for everything that can be scripted.
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

#ifndef TSC_SCRIPTING_SCRIPTABLE_OBJECT_HPP
#define TSC_SCRIPTING_SCRIPTABLE_OBJECT_HPP
#include "../core/global_basic.hpp"

namespace TSC {
    namespace Scripting {

        /**
         * This class encapsulates the stuff that is common
         * to all objects exposed to the mruby scripting
         * interface. That is, it holds the mruby event tables.
         */
        class cScriptable_Object {
        public:
            cScriptable_Object();
            virtual ~cScriptable_Object();

            void clear_event_handlers(const std::string& levelname = "");
            void register_event_handler(const std::string& evtname, mrb_value callback);
            std::vector<mrb_value>::iterator event_handlers_begin(const std::string& evtname);
            std::vector<mrb_value>::iterator event_handlers_end(const std::string& evtname);

        protected:
            /// Mapping of level + event names and registered callbacks.
            /// Example in ruby syntax:
            /// {"mylevel" => {"myevent" => [handle1, handle2]}, "mevent2" => ["handle3"]}
            std::map<std::string, std::map<std::string, std::vector<mrb_value> > > m_callbacks;
        private:
            std::string get_active_level_name();
        };
    };
};
#endif

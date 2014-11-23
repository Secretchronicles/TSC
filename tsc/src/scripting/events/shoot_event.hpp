/***************************************************************************
 * shoot_event.hpp
 *
 * Copyright © 2013-2014 The TSC Contributors
 ***************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TSC_SCRIPTING_SHOOT_EVENT_HPP
#define TSC_SCRIPTING_SHOOT_EVENT_HPP
#include "../scripting.hpp"
#include "event.hpp"

namespace TSC {
    namespace Scripting {
        class cShoot_Event: public cEvent {
        public:
            cShoot_Event(std::string ball_type);
            virtual std::string Event_Name();
            std::string Get_Ball_Type();
        protected:
            virtual void Run_MRuby_Callback(cMRuby_Interpreter* p_mruby, mrb_value callback);
        private:
            std::string m_ball_type;
        };
    }
}
#endif

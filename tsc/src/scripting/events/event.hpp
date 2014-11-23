/***************************************************************************
 * base_class.hpp - Base class for all events
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

#ifndef TSC_SCRIPTING_EVENT_HPP
#define TSC_SCRIPTING_EVENT_HPP
#include "../scripting.hpp"
#include "../../scripting/scriptable_object.hpp"

// Defines an event handler function that forwards to the Eventable#bind
// method, passing `evtname' as the first argument. Effectively implements
// the C++ side of the "on_*" methods. The MRUBY_EVENT_HANDLER macro returns
// the name of the function defined by this macro, so you can pass the function
// to mrb_define_method.
#define MRUBY_IMPLEMENT_EVENT(evtname) \
    static mrb_value Scripting_Event_On_##evtname(mrb_state* p_state, mrb_value self) \
    { \
    mrb_value callback; \
    mrb_get_args(p_state, "&", &callback); \
    \
    mrb_value arg = mrb_str_new_cstr(p_state, #evtname); \
    return mrb_funcall_with_block(p_state, \
        self, \
        mrb_intern_cstr(p_state, "bind"), \
        1, \
        &arg, \
        callback); \
    }
// ↑ Note mrb_funcall_with_block() takes a C array of mrb_values ↑

// Expands to the name of the function definend
// by MRUBY_IMPLEMENT_EVENT.
#define MRUBY_EVENT_HANDLER(evtname) Scripting_Event_On_##evtname

namespace TSC {
    namespace Scripting {
        // TODO: Pass the cMruby_Interpreter instance to the constructor!
        // There are cases where the event itself needs the interpreter,
        // see for example level_save_event!
        class cEvent {
        public:
            void Fire(cMRuby_Interpreter* p_mruby, Scripting::cScriptable_Object* p_obj);
            virtual std::string Event_Name();
        protected:
            virtual void Run_MRuby_Callback(cMRuby_Interpreter* p_mruby, mrb_value callback);
        };
    };
};
#endif

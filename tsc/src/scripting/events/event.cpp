/***************************************************************************
 * event.cpp - Base class for all events
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

#include "event.hpp"
#include "../../core/property_helper.hpp"
#include "../../core/global_basic.hpp"

using namespace TSC;
using namespace TSC::Scripting;
using namespace std;

/**
 * Cycles through all registered event handlers for the event
 * name returned by the Event_Name() method and calls the
 * Run_MRuby_Callback() method for each of them. See Run_MRuby_Callback()’s
 * documentation for more information on this.
 *
 * For subclasses, you don’t want to override Fire(), but rather
 * Run_MRuby_Callback() and Event_Name().
 */
void cEvent::Fire(cMRuby_Interpreter* p_mruby, Scripting::cScriptable_Object* p_obj)
{
    // Menu level has no mruby interpreter
    if (!p_mruby)
        return;
    mrb_state* p_state = p_mruby->Get_MRuby_State();

    // Iterate through the list of callbacks and execute them
    std::string evtname = Event_Name();
    std::vector<mrb_value>::iterator start = p_obj->event_handlers_begin(evtname);
    std::vector<mrb_value>::iterator end = p_obj->event_handlers_end(evtname);

    std::vector<mrb_value>::iterator iter;
    for (iter=start; iter != end; iter++) {
        Run_MRuby_Callback(p_mruby, *iter);
        if (p_state->exc) {
            cerr << "Warning: Error running mruby handler:" << endl;
            mrb_print_error(p_state);
        }
    }
}

/**
 * Returns the name of the event, used for determining which
 * callbacks to run when Fire() is called. Subclasses must override
 * this method in order to return a proper name identifying them
 * uniquely among all possible event names. The names returned
 * by all the Event_Name() functions must correspond to the names
 * you pass to the MRUBY_IMPLEMENT_EVENT and MRUBY_EVENT_HANDLER
 * macros that implement the "on_*" methods.
 */
std::string cEvent::Event_Name()
{
    return "generic";
}

/**
 * Called whenever a MRuby callback shall be run. The callback is
 * passed as a mruby lambda via the `callback' argument.
 *
 * This method is intended to be overridden in more specific event classes,
 * that should however eventually call the #call() method on the lambda object.
 * The default implemention of this method just does exactly that, without
 * passing any further arguments or inspecting the return value of the callback.
 *
 * Leave p_mruby->Get_MRuby_State()->exc set to get that exception automatically
 * printed out to cerr (via the Fire() method that calls this method).
 */
void cEvent::Run_MRuby_Callback(cMRuby_Interpreter* p_mruby, mrb_value callback)
{
    mrb_funcall(p_mruby->Get_MRuby_State(), callback, "call", 0);
}

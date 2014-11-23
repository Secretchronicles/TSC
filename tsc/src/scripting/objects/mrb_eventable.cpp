/***************************************************************************
 * mrb_eventable.cpp - Event receipt mixin
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

#include "mrb_eventable.hpp"
#include "../scriptable_object.hpp"
#include "../../core/global_basic.hpp"

/**
 * Module: Eventable
 *
 * Mixin module that allows objects to receive events from TSC.
 * Mixing in this module on the MRuby side of things is quite
 * useless as you may then receive events, but nobody actually
 * _fires_ those for you.
 *
 * This module however is included by default by all classes
 * that can receive events from the C++ side and its methods
 * are therefore available to instances of those classes. It
 * is also used internally heavily in the implementation of
 * the `on_*` methods.
 */

using namespace TSC;
using namespace TSC::Scripting;


/**
 * Method: Eventable#bind
 *
 *   bind(evtname){|evtname, *args| ...} → nil
 *
 * Listen for the event `evtname` and register the block as the
 * event handler. It gets passed the name of the event as the
 * first argument, followed by any additional arguments specific
 * to the event.
 */
mrb_value Bind(mrb_state* p_state, mrb_value self)
{
    char* evtname = NULL;
    mrb_value callback;
    mrb_get_args(p_state, "z&", &evtname, &callback);

    Scripting::cScriptable_Object* p_obj = (Scripting::cScriptable_Object*) DATA_PTR(self);
    if (!p_obj)
        mrb_raise(p_state, MRB_RUNTIME_ERROR(p_state), "No associated C++ object found.");

    debug_print("Registering callback for event '%s' for MRuby object %d of class '%s'\n", evtname, mrb_obj_id(self), mrb_obj_classname(p_state, self));

    // Set `callbacks' instance variable (without @, so invisible to the Ruby
    // side) to an empty array if it isn’t set already.
    mrb_sym callbacks_sym = mrb_intern_cstr(p_state, "callbacks");
    if (!mrb_iv_defined(p_state, self, callbacks_sym))
        mrb_iv_set(p_state, self, callbacks_sym, mrb_ary_new(p_state));

    /* The C++ cScriptableObject instance must have access to the callbacks
     * so it can execute them when an event occurs. However, mruby’s GC can’t
     * see through our C++ pointers, so we have to prevent it from garbage-
     * collecting the callback explicitely by referencing it from this object.
     * This causes somewhat duplicate information, as the callbacks are now
     * referenced from both the `callbacks' instance variable and the m_callbacks
     * member of the C++ object instance, which *must* be kept in sync to
     * prevent bad side-effects like unexpected segmentation faults. */
    mrb_ary_push(p_state, mrb_iv_get(p_state, self, callbacks_sym), callback);
    p_obj->register_event_handler(evtname, callback);

    return mrb_nil_value();
}

void TSC::Scripting::Init_Eventable(mrb_state* p_state)
{
    struct RClass* p_rmEventable = mrb_define_module(p_state, "Eventable");
    mrb_define_method(p_state, p_rmEventable, "bind", Bind, MRB_ARGS_REQ(1) | MRB_ARGS_BLOCK());
}

/***************************************************************************
 * scriptable_object.cpp - Baseclass for everything that can be scripted.
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

#include "scriptable_object.hpp"
#include "../level/level.hpp"
#include "../core/property_helper.hpp"

using namespace TSC;
using namespace TSC::Scripting;

/* The `m_callbacks' member variable of the cScriptableObject class
 * is blasphemical currently. It holds mruby objects (mrb_value instances)
 * of DIFFERENT mruby interpreters! The reason for this is sublevel
 * handling. Each level has its own mruby interpreter attached, but
 * while the sublevel is active, mruby handlers from the outer
 * main level are “suspended” and should not be run. Problem is,
 * some objects, most notably the level player (cLevel_Player singleton
 * instance), is shared amongst all currently active levels. This is
 * a design flaw that should probably be fixed, but to work around
 * the problem m_callbacks just maps an event handler by both level
 * and event name. If you tried to run an event handler from a level
 * different from the active one (pActive_Level), this would actually
 * work and have effect on the currently invisible level. However, this
 * is unintended and not allowed by the outbound interface of the
 * cScriptable_Object class hence. When a sublevel is destroyed, it
 * is required to remove all objects it has from the `m_callbacks'
 * member by employing clear_event_handlers() with its level name
 * passed. */

cScriptable_Object::cScriptable_Object()
{
    //
}

cScriptable_Object::~cScriptable_Object()
{
    clear_event_handlers();
}

/**
 * Clear all event handlers for all events. This is necessary when
 * you wipe out the mruby interpreter and set up a new one, as does
 * cLevel::Reinitialize_MRuby_Interpreter(), because deleting the
 * mruby interpreter automatically invalidates any mruby object
 * reference (`mrb_value`) still flying around. If you don’t clear
 * the event handler list (which precisely holds `mrb_values`) you
 * will get unpredictable behaviour reaching from mruby exceptions
 * to segmentation faults when an event gets fired for which an
 * event handler has been registered prior to deleting the mruby
 * interpreter.
 *
 * This method may also be used to just wipe out the event handlers for
 * a specific level by passing the parameter `levelname`. This situation
 * can arise if you are dealing with sublevels, where more than one level
 * can be loaded at a time.
 *
 * \param[in] level ("") level to clear. If an empty string, all event
 * handlers for all levels a cleared. If the level you want to clear
 * the event handlers for is ~/.local/share/tsc/foo.tsclvl, then you
 * pass in `"foo"` for this parameter. A user and a game level of the
 * same name cannot be loaded at the same time, so this is not a
 * problem here.
 */
void cScriptable_Object::clear_event_handlers(const std::string& levelname /* = "" */)
{
    if (levelname.empty())
        m_callbacks.clear();
    else
        m_callbacks[levelname].clear();
}

/**
 * Register a new event handler for an event for the currently active
 * level, adding to the list of already existing event handlers (if
 * any).
 *
 * \param evtname
 *   Name of the event to register for. This has to match a return value
 *   of a cEvent::Event_Name() override (otherwise the handler will
 *   never get executed).
 * \param callback
 *   An mruby proc object to be executed when the event gets fired.
 */
void cScriptable_Object::register_event_handler(const std::string& evtname, mrb_value callback)
{
    m_callbacks[get_active_level_name()][evtname].push_back(callback);
}

/**
 * Start iterator for the list of callbacks registered for the
 * given event name.
 *
 * \param evtname Name of the event you want the handlers for.
 *
 * \returns Iterator pointing to the first callback.
 */
std::vector<mrb_value>::iterator cScriptable_Object::event_handlers_begin(const std::string& evtname)
{
    return m_callbacks[get_active_level_name()][evtname].begin();
}

/**
 * Stop iterator for the list of callbacks registered for the
 * given event name.
 *
 * \param evtname Name of the event you want the handlers for.
 *
 * \returns Iterator pointing post the last callback (termination iterator).
 */
std::vector<mrb_value>::iterator cScriptable_Object::event_handlers_end(const std::string& evtname)
{
    return m_callbacks[get_active_level_name()][evtname].end();
}

std::string cScriptable_Object::get_active_level_name()
{
    return path_to_utf8(pActive_Level->m_level_filename.stem());
}

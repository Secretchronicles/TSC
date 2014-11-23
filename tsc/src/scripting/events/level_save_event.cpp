/***************************************************************************
 * level_save_event.cpp
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

#include "level_save_event.hpp"

using namespace TSC;
using namespace TSC::Scripting;

/**
 * The `storage_hash' passed here MUST belong to the same mruby interpreter
 * as the one you are going to pass to the Fire() method! It is used for
 * allowing consecutive save events to build up a hash that can be stored
 * into the savegame file; it has to be an mruby Hash instance. The contained
 * hash WILL be changed when you call Fire().
 */
cLevel_Save_Event::cLevel_Save_Event(mrb_value storage_hash)
{
    m_storage_hash = storage_hash;
}

std::string cLevel_Save_Event::Event_Name()
{
    return "save";
}

// TODO: Would be better if m_storage_hash and p_mruby weren’t separated.
// See event.hpp for the suggestion to move the passing of cMRuby_Interpreter
// to the event constructor instead to guarantee only one single mruby interpreter is
// involved.
void cLevel_Save_Event::Run_MRuby_Callback(cMRuby_Interpreter* p_mruby, mrb_value callback)
{
    mrb_state* p_state = p_mruby->Get_MRuby_State();

    mrb_funcall(p_state, callback, "call", 1, m_storage_hash);
}

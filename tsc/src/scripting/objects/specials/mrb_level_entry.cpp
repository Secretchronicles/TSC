/***************************************************************************
 * mrb_level_entry.cpp
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

#include "../../../objects/level_entry.hpp"
#include "../../../core/sprite_manager.hpp"
#include "../../../level/level.hpp"
#include "mrb_level_entry.hpp"
#include "../sprites/mrb_moving_sprite.hpp"
#include "../../events/event.hpp"

/**
 * Class: LevelEntry
 *
 * Parent: L<MovingSprite>
 *
 * A I<level entry> can be used as a warp target for Alex even
 * without scripting. It has a name under which it can be
 * referenced from a L<LevelExit> to make Alex change places.
 *
 * =head2 Events
 *
 * =over
 *
 * =item [Enter]
 *
 * This event gets triggered when the level entry is activated,
 * but after Alex has moved out of the warp area. That is,
 * when this is an up-direction level entry, the event will
 * not be triggered before Alex has completed his upwards
 * move (but before eventual gravity is applied).
 *
 * =back
 */

using namespace TSC;
using namespace TSC::Scripting;


MRUBY_IMPLEMENT_EVENT(enter);

/**
 * Method: LevelEntry::new
 *
 *   new()
 *
 * Creates a new instance of this class.
 */
static mrb_value Initialize(mrb_state* p_state, mrb_value self)
{
    cLevel_Entry* p_entry = new cLevel_Entry(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_entry;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    // This is a generated object
    p_entry->Set_Spawned(true);

    // Let TSC manage the memory
    pActive_Level->m_sprite_manager->Add(p_entry);

    return self;
}

/**
 * Method: LevelEntry#name
 *
 *   name() → a_string
 *
 * Returns the name of the level entry.
 */
static mrb_value Get_Name(mrb_state* p_state, mrb_value self)
{
    cLevel_Entry* p_entry = Get_Data_Ptr<cLevel_Entry>(p_state, self);

    return mrb_str_new_cstr(p_state, p_entry->m_entry_name.c_str());
}

/**
 * Method: LevelEntry#name=
 *
 *   name=( str )
 *
 * Set the name of the level entry.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [str] The new name of the level entry.
 *
 * =back
 */
static mrb_value Set_Name(mrb_state* p_state, mrb_value self)
{
    char* name = NULL;
    mrb_get_args(p_state, "z", &name);

    cLevel_Entry* p_entry = Get_Data_Ptr<cLevel_Entry>(p_state, self);
    p_entry->Set_Name(name);

    return mrb_str_new_cstr(p_state, name);
}

/**
 * Method: LevelEntry#type
 *
 *   type() → a_symbol
 *
 * Retrieve the entry’s type. See L<#type=> for a list of possible types.
 */
static mrb_value Get_Type(mrb_state* p_state, mrb_value self)
{
    cLevel_Entry* p_entry = Get_Data_Ptr<cLevel_Entry>(p_state, self);

    switch (p_entry->m_entry_type) {
    case LEVEL_ENTRY_BEAM:
        return str2sym(p_state, "beam");
    case LEVEL_ENTRY_WARP:
        return str2sym(p_state, "warp");
    default:
        return mrb_nil_value();
    }
}

/**
 * Method: LevelEntry#type=
 *
 *   type=( sym )
 *
 * Change the level entry’s type.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [sym]
 *
 * The new type of the entry. One of the following symbols:
 *
 * =over
 *
 * =item [C<:beam>]
 *
 * Make Alex immediately stand at the entry’s position.
 *
 * =item [C<:warp>]
 *
 * Make Alex slowly move into the direction specified
 * by L<MovingSprite#direction>. This is useful for pipes.
 *
 * =back
 *
 * =back
 */
static mrb_value Set_Type(mrb_state* p_state, mrb_value self)
{
    mrb_sym type;
    mrb_get_args(p_state, "n", &type);
    std::string typestr(mrb_sym2name(p_state, type));

    Level_Entry_type typid;
    if (typestr == "beam")
        typid = LEVEL_ENTRY_BEAM;
    else if (typestr == "warp")
        typid = LEVEL_ENTRY_WARP;
    else {
        mrb_raisef(p_state, MRB_ARGUMENT_ERROR(p_state), "Invalid level entry type %s", typestr.c_str());
        return mrb_nil_value(); // Not reached
    }

    cLevel_Entry* p_entry = Get_Data_Ptr<cLevel_Entry>(p_state, self);
    p_entry->Set_Type(typid);

    return mrb_symbol_value(type);
}

void TSC::Scripting::Init_LevelEntry(mrb_state* p_state)
{
    struct RClass* p_rcLevel_Entry = mrb_define_class(p_state, "LevelEntry", mrb_class_get(p_state, "MovingSprite"));
    MRB_SET_INSTANCE_TT(p_rcLevel_Entry, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcLevel_Entry, "initialize", Initialize, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel_Entry, "name", Get_Name, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel_Entry, "name=", Set_Name, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcLevel_Entry, "type", Get_Type, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel_Entry, "type=", Set_Type, MRB_ARGS_REQ(1));

    mrb_define_method(p_state, p_rcLevel_Entry, "on_enter", MRUBY_EVENT_HANDLER(enter), MRB_ARGS_NONE());
}

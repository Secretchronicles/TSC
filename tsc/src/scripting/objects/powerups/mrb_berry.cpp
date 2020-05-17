/***************************************************************************
 * mrb_mushroom.cpp
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

#include "mrb_berry.hpp"
#include "mrb_powerup.hpp"
#include "../../../level/level.hpp"
#include "../../../core/sprite_manager.hpp"
#include "../../../objects/powerup.hpp"

/**
 * Class: Berry
 *
 * Parent: L<Powerup>
 *
 * The I<Berry>.
 */

using namespace TSC;
using namespace TSC::Scripting;


/**
 * Method: Berry::new
 *
 *   new() → a_berry
 *
 * Creates a new berry powerup with the default values.
 */
static mrb_value Initialize(mrb_state* p_state, mrb_value self)
{
    cMushroom* p_berry = new cMushroom(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_berry;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    // This is a generated object
    p_berry->Set_Spawned(true);

    // Let TSC manage the memory
    pActive_Level->m_sprite_manager->Add(p_berry);

    return self;
}

/**
 * Method: Berry#type=
 *
 *   type=( type ) → type
 *
 * Specify the berry’s type.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [type]
 *
 * The berry’s new type. One of the following symbols:
 *
 * =over
 *
 * =item [red] The normal red berry
 *
 * =item [life] The green +1 life berry
 *
 * =item [poison] The green -1 life berry
 *
 * =item [blue] The blue ice-alex berry
 *
 * =item [ghost] The transparent ghost-alex berry
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

    SpriteType spritetype;
    if (typestr == "default" || typestr == "red")
        spritetype = TYPE_MUSHROOM_DEFAULT;
    else if (typestr == "life" || typestr == "1up")
        spritetype = TYPE_MUSHROOM_LIVE_1;
    else if (typestr == "poison")
        spritetype = TYPE_MUSHROOM_POISON;
    else if (typestr == "blue")
        spritetype = TYPE_MUSHROOM_BLUE;
    else if (typestr == "ghost")
        spritetype = TYPE_MUSHROOM_GHOST;
    else {
        mrb_raisef(p_state, MRB_ARGUMENT_ERROR(p_state), "Invalid berry type %s", typestr.c_str());
        return mrb_nil_value(); // Not reached
    }

    cMushroom* p_berry = Get_Data_Ptr<cMushroom>(p_state, self);
    p_berry->Set_Type(spritetype);

    return mrb_symbol_value(type);
}

/**
 * Method: Berry#type
 *
 *   type() → a_symbol
 *
 * Returns the berry’s current type as a symbol See L<type=>
 * for a list of possible symbols returned by this method.
 */
static mrb_value Get_Type(mrb_state* p_state, mrb_value self)
{
    cMushroom* p_berry = Get_Data_Ptr<cMushroom>(p_state, self);
    switch (p_berry->m_type) {
    case TYPE_MUSHROOM_DEFAULT:
        return str2sym(p_state, "red");
    case TYPE_MUSHROOM_LIVE_1:
        return str2sym(p_state, "life");
    case TYPE_MUSHROOM_POISON:
        return str2sym(p_state, "poison");
    case TYPE_MUSHROOM_BLUE:
        return str2sym(p_state, "blue");
    case TYPE_MUSHROOM_GHOST:
        return str2sym(p_state, "ghost");
    default:
        return mrb_nil_value();
    }
}

/**
 * Method: Berry#glimming=
 *
 *   glimming=( bool ) → bool
 *
 * Make this berry look mysterious by making it glimming.
 * Note glimming is the default for newly created berrys.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [bool] Whether or not to glim.
 *
 * =back
 */
static mrb_value Set_Glim_Mode(mrb_state* p_state, mrb_value self)
{
    mrb_bool glim;
    mrb_get_args(p_state, "b", &glim);

    cMushroom* p_berry = Get_Data_Ptr<cMushroom>(p_state, self);
    p_berry->m_glim_mod = glim;

    return mrb_bool_value(glim);
}

/**
 * Method: Berry#glimming?
 *
 *   glimming?() → true or false
 *
 * Is this berry glimming mysteriously?
 */
static mrb_value Get_Glim_Mode(mrb_state* p_state, mrb_value self)
{
    cMushroom* p_berry = Get_Data_Ptr<cMushroom>(p_state, self);
    return mrb_bool_value(p_berry->m_glim_mod);
}

/**
 * Method: Berry#activate!
 *
 *   activate!()
 *
 * Apply the item to Alex. Doing so will destroy the berry in
 * any case, but note that applying it does not necessarily cause
 * it to have an effect on Alex (use L<LevelPlayer#type=> for
 * that). Instead, it may be stored in the item box, or even
 * just have no effect if that is already full.
 */
static mrb_value Activate(mrb_state* p_state, mrb_value self)
{
    cMushroom* p_berry = Get_Data_Ptr<cMushroom>(p_state, self);
    p_berry->Activate();
    return mrb_nil_value();
}

void TSC::Scripting::Init_Berry(mrb_state* p_state)
{
    struct RClass* p_rcBerry = mrb_define_class(p_state, "Berry", mrb_class_get(p_state, "Powerup"));
    MRB_SET_INSTANCE_TT(p_rcBerry, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcBerry, "initialize", Initialize, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcBerry, "type=", Set_Type, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcBerry, "type", Get_Type, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcBerry, "glimming=", Set_Glim_Mode, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcBerry, "glimming?", Get_Glim_Mode, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcBerry, "activate!", Activate, MRB_ARGS_NONE());
}

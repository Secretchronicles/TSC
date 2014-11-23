/***************************************************************************
 * mrb_mushroom.cpp
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

#include "mrb_mushroom.hpp"
#include "mrb_powerup.hpp"
#include "../../../level/level.hpp"
#include "../../../core/sprite_manager.hpp"
#include "../../../objects/powerup.hpp"

/**
 * Class: Mushroom
 *
 * Parent: [Powerup](powerup.html)
 * {: .superclass}
 *
 * The _Mushroom_.
 */

using namespace TSC;
using namespace TSC::Scripting;


/**
 * Method: Mushroom::new
 *
 *   new() → a_mushroom
 *
 * Creates a new mushroom powerup with the default values.
 */
static mrb_value Initialize(mrb_state* p_state, mrb_value self)
{
    cMushroom* p_mushroom = new cMushroom(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_mushroom;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    // This is a generated object
    p_mushroom->Set_Spawned(true);

    // Let TSC manage the memory
    pActive_Level->m_sprite_manager->Add(p_mushroom);

    return self;
}

/**
 * Method: Mushroom#type=
 *
 *   type=( type ) → type
 *
 * Specify the mushroom’s type.
 *
 * #### Parameters
 * type
 * : The mushroom’s new type. One of the following symbols:
 *
 *   red
 *   : The normal red mushroom
 *
 *   life
 *   : The green +1 life mushroom
 *
 *   poison
 *   : The green -1 life mushroom
 *
 *   blue
 *   : The blue ice-maryo mushroom
 *
 *   ghost
 *   : The transparent ghost-maryo mushroom
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
        mrb_raisef(p_state, MRB_ARGUMENT_ERROR(p_state), "Invalid mushroom type %s", typestr.c_str());
        return mrb_nil_value(); // Not reached
    }

    cMushroom* p_mushroom = Get_Data_Ptr<cMushroom>(p_state, self);
    p_mushroom->Set_Type(spritetype);

    return mrb_symbol_value(type);
}

/**
 * Method: Mushroom#type
 *
 *   type() → a_symbol
 *
 * Returns the mushroom’s current type as a symbol See `type=`
 * for a list of possible symbols returned by this method.
 */
static mrb_value Get_Type(mrb_state* p_state, mrb_value self)
{
    cMushroom* p_mushroom = Get_Data_Ptr<cMushroom>(p_state, self);
    switch (p_mushroom->m_type) {
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
 * Method: Mushroom#glimming=
 *
 *   glimming=( bool ) → bool
 *
 * Make this mushroom look mysterious by making it glimming.
 * Note glimming is the default for newly created mushrooms.
 *
 * #### Parameters
 * bool
 * : Whether or not to glim.
 */
static mrb_value Set_Glim_Mode(mrb_state* p_state, mrb_value self)
{
    mrb_bool glim;
    mrb_get_args(p_state, "b", &glim);

    cMushroom* p_mushroom = Get_Data_Ptr<cMushroom>(p_state, self);
    p_mushroom->m_glim_mod = glim;

    return mrb_bool_value(glim);
}

/**
 * Method: Mushroom#glimming?
 *
 *   glimming?() → true or false
 *
 * Is this mushroom glimming mysteriously?
 */
static mrb_value Get_Glim_Mode(mrb_state* p_state, mrb_value self)
{
    cMushroom* p_mushroom = Get_Data_Ptr<cMushroom>(p_state, self);
    return mrb_bool_value(p_mushroom->m_glim_mod);
}

/**
 * Method: Mushroom#activate!
 *
 *   activate!()
 *
 * Apply the item to Maryo. Doing so will destroy the mushroom in
 * any case, but note that applying it does not necessarily cause
 * it to have an effect on Maryo (use `LevelPlayer#type=` for
 * that). Instead, it may be stored in the item box, or even
 * just have no effect if that is already full.
 */
static mrb_value Activate(mrb_state* p_state, mrb_value self)
{
    cMushroom* p_mushroom = Get_Data_Ptr<cMushroom>(p_state, self);
    p_mushroom->Activate();
    return mrb_nil_value();
}

void TSC::Scripting::Init_Mushroom(mrb_state* p_state)
{
    struct RClass* p_rcMushroom = mrb_define_class(p_state, "Mushroom", mrb_class_get(p_state, "Powerup"));
    MRB_SET_INSTANCE_TT(p_rcMushroom, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcMushroom, "initialize", Initialize, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcMushroom, "type=", Set_Type, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcMushroom, "type", Get_Type, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcMushroom, "glimming=", Set_Glim_Mode, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcMushroom, "glimming?", Get_Glim_Mode, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcMushroom, "activate!", Activate, MRB_ARGS_NONE());
}

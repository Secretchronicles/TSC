/***************************************************************************
 * mrb_box.cpp
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

#include "mrb_box.hpp"
#include "../../../objects/box.hpp"
#include "../../events/event.hpp"
#include "../sprites/mrb_moving_sprite.hpp"

/**
 * Class: Box
 *
 * Parent L<MovingSprite>
 *
 * I<Box> is the superclass for all boxes accessible
 * from the scripting interface. Boxes are the little
 * blocks that contain coins, powerups, or just spin
 * around if jump against them.
 *
 * =head2 Events
 *
 * =over
 *
 * =item [Activate]
 *
 * The box was activated by Alex jumping against it or otherwise.
 *
 * =back
 */

using namespace TSC;
using namespace TSC::Scripting;


MRUBY_IMPLEMENT_EVENT(activate);

static mrb_value Initialize(mrb_state* p_state, mrb_value self)
{
    mrb_raise(p_state, MRB_NOTIMP_ERROR(p_state), "Cannot create instances of Box.");
    return self;
}

/**
 * Method: Box#animation_type=
 *
 *   animation_type=( ani ) → ani
 *
 * Specify the box’ appeareance.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [ani]
 *
 * The box’ new animation type. One of the following symbols:
 *
 * =over
 *
 * =item [bonus] The "?" box. Usually used for powerups.
 *
 * =item [default] The blank box. Usually used for coins.
 *
 * =item [power] The "!" box. Usually used for 1-ups.
 *
 * =item [spin] The spinbox. This box just spins if jumped against it.
 *
 * =back
 *
 * =back
 */
static mrb_value Set_Animation_Type(mrb_state* p_state, mrb_value self)
{
    mrb_sym type;
    mrb_get_args(p_state, "n", &type);
    std::string typestr(mrb_sym2name(p_state, type));

    std::string anitype;
    if (typestr == "bonus")
        anitype = "Bonus";
    else if (typestr == "default")
        anitype = "Default";
    else if (typestr == "power")
        anitype = "Power";
    else if (typestr == "spin")
        anitype = "Spain";
    else {
        mrb_raisef(p_state, MRB_ARGUMENT_ERROR(p_state), "Invalid box animation type %s", typestr.c_str());
        return mrb_nil_value(); // Not reached
    }

    cBaseBox* p_box = Get_Data_Ptr<cBaseBox>(p_state, self);
    p_box->Set_Animation_Type(anitype);

    return mrb_symbol_value(type);
}

/**
 * Method: Box#animation_type
 *
 *   animation_type() → a_symbol
 *
 * Returns the box’ current animation type. See L<#animation_type=>
 * for a list of possible return values.
 */
static mrb_value Get_Animation_Type(mrb_state* p_state, mrb_value self)
{
    cBaseBox* p_box = Get_Data_Ptr<cBaseBox>(p_state, self);
    return str2sym(p_state, p_box->m_anim_type);
}

/**
 * Method: Box#max_uses=
 *
 *   max_uses=( count ) → count
 *
 * Specify how many times this max be used at maximum before
 * it stops to work.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [count]
 *
 * The maximum number of times this box may be used. Specify
 * a value of -1 to indicate it may be used infinitely.
 *
 * =back
 */
static mrb_value Set_Usable_Count(mrb_state* p_state, mrb_value self)
{
    mrb_int count;
    mrb_get_args(p_state, "i", &count);

    cBaseBox* p_box = Get_Data_Ptr<cBaseBox>(p_state, self);
    p_box->Set_Useable_Count(count, true);

    return mrb_fixnum_value(count);
}

/**
 * Method: Box#max_uses
 *
 *   max_uses() → an_integer
 *
 * Returns the maximum number of times this box can
 * be used.
 */
static mrb_value Get_Usable_Count(mrb_state* p_state, mrb_value self)
{
    cBaseBox* p_box = Get_Data_Ptr<cBaseBox>(p_state, self);
    return mrb_fixnum_value(p_box->m_start_useable_count);
}

/**
 * Method: Box#remaining_uses
 *
 *   remaining_uses() → an_integer
 *
 * Returns the remaining number of times this box
 * may be used. Returns -1 for boxes that may be
 * used infinitely.
 */
static mrb_value Get_Remaining_Usable_Count(mrb_state* p_state, mrb_value self)
{
    cBaseBox* p_box = Get_Data_Ptr<cBaseBox>(p_state, self);
    return mrb_fixnum_value(p_box->m_useable_count);
}

/**
 * Method: Box#invisible=
 *
 *   invisible=( invis ) → invis
 *
 * Specify the box’ invisibility type.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [invis]
 *
 * The box’ new invisibility type. One of the following symbols:
 *
 * =over
 *
 * =item [visible]
 *
 * The box isn’t invisible.
 *
 * =item [massive]
 *
 * The box gets visible if jumped against it. It however
 * acts as an invisible massive block even if not activated
 * (i.e. you can stand on it).</dd>
 *
 * =item [ghost]
 *
 * The box is only visible for Ghost Alex. Only Ghost Alex
 * can stand on it.
 *
 * =item [semi_massive]
 *
 * The box is visible after jumping against it from
 * its activation direction (usually from below). While
 * not activated, Alex can’t stand on it.
 *
 * =back
 *
 * =back
 */
static mrb_value Set_Invisible(mrb_state* p_state, mrb_value self)
{
    mrb_sym type;
    mrb_get_args(p_state, "n", &type);
    std::string typestr(mrb_sym2name(p_state, type));

    Box_Invisible_Type invis;
    if (typestr == "visible")
        invis = BOX_VISIBLE;
    else if (typestr == "massive")
        invis = BOX_INVISIBLE_MASSIVE;
    else if (typestr == "ghost")
        invis = BOX_GHOST;
    else if (typestr == "semi_massive")
        invis = BOX_INVISIBLE_SEMI_MASSIVE;
    else {
        mrb_raisef(p_state, MRB_ARGUMENT_ERROR(p_state), "Invalid box invisible type %s", typestr.c_str());
        return mrb_nil_value(); // Not reached
    }

    cBaseBox* p_box = Get_Data_Ptr<cBaseBox>(p_state, self);
    p_box->Set_Invisible(invis);

    return mrb_symbol_value(type);
}

/**
 * Method: Box#invisible
 *
 *   invisible() → a_symbol
 *
 * Returns the box’ invisibility state. See L<#invisible=> for
 * possible return values.
 */
static mrb_value Get_Invisible(mrb_state* p_state, mrb_value self)
{
    cBaseBox* p_box = Get_Data_Ptr<cBaseBox>(p_state, self);
    switch (p_box->m_box_invisible) {
    case BOX_VISIBLE:
        return str2sym(p_state, "visible");
    case BOX_INVISIBLE_MASSIVE:
        return str2sym(p_state, "massive");
    case BOX_GHOST:
        return str2sym(p_state, "ghost");
    case BOX_INVISIBLE_SEMI_MASSIVE:
        return str2sym(p_state, "semi_massive");
    default:
        return mrb_nil_value();
    }
}

/**
 * Method: Box#activate
 *
 *   activate()
 *
 * Activate the box, revealing what it’s inside and
 * decrementing the usage count by one.
 */
static mrb_value Activate(mrb_state* p_state, mrb_value self)
{
    cBaseBox* p_box = Get_Data_Ptr<cBaseBox>(p_state, self);
    p_box->Activate();
    return mrb_nil_value();
}

void TSC::Scripting::Init_Box(mrb_state* p_state)
{
    struct RClass* p_rcBox = mrb_define_class(p_state, "Box", mrb_class_get(p_state, "MovingSprite"));
    MRB_SET_INSTANCE_TT(p_rcBox, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcBox, "initialize", Initialize, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcBox, "animation_type=", Set_Animation_Type, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcBox, "animation_type", Get_Animation_Type, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcBox, "max_uses=", Set_Usable_Count, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcBox, "max_uses", Get_Usable_Count, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcBox, "remaining_uses", Get_Remaining_Usable_Count, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcBox, "invisible=", Set_Invisible, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcBox, "invisible", Get_Invisible, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcBox, "activate", Activate, MRB_ARGS_NONE());

    mrb_define_method(p_state, p_rcBox, "on_activate", MRUBY_EVENT_HANDLER(activate), MRB_ARGS_BLOCK());
}

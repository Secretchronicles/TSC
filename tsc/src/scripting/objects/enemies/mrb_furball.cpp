/***************************************************************************
 * mrb_furball.cpp
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

#include "../../../enemies/furball.hpp"
#include "../../../level/level.hpp"
#include "../../../core/sprite_manager.hpp"
#include "../../../core/property_helper.hpp"
#include "../../events/event.hpp"
#include "mrb_enemy.hpp"
#include "mrb_furball.hpp"

/**
 * Class: Furball
 *
 * Parent: L<Enemy>
 *
 * Being the most dangerous enemy of the game, the I<furball> is practically
 * everywhere. Although it seems it isn’t dangerous, this is not necessarily
 * true...
 *
 * Note that the furball boss is not represented by a separate class. To
 * instanciate one, you’d use this class like so:
 *
 *     f = Furball.new
 *     f.color = :black # This makes it a boss
 *     f.x = 530
 *     f.y = -120
 *     f.show
 *
 * The L<#boss?> method can be used to check whether any given furball is
 * a furball boss.
 *
 * =head2 Events
 *
 * =over
 *
 * =item [Downgrade]
 *
 * This event is triggered each time the furball gets hit.
 * It receives two arguments: the number of hits the furball
 * already took and the maximum number it takes before dying.
 * This event is only issued for the furball boss.
 *
 * =back
 */

using namespace TSC;
using namespace TSC::Scripting;

MRUBY_IMPLEMENT_EVENT(downgrade);

/**
 * Method: Furball::new
 *
 *   new() → a_furball
 *
 * Create a new instance with the default values.
 */
static mrb_value Initialize(mrb_state* p_state,  mrb_value self)
{
    cFurball* p_furball = new cFurball(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_furball;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    // This is a generated object
    p_furball->Set_Spawned(true);

    // Let TSC manage the memory
    pActive_Level->m_sprite_manager->Add(p_furball);

    return self;
}

/**
 * Method: Furball#color=
 *
 *   color=( color ) → color
 *
 * Specify the furball’s color.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [color]
 *
 * The furball’s new color. One of C<:brown>, C<:blue>, or C<:black>;
 * Note that C<:black> makes the furball a furball boss.
 *
 * =back
 */
static mrb_value Set_Color(mrb_state* p_state, mrb_value self)
{
    mrb_sym color;
    mrb_get_args(p_state, "n", &color);
    std::string colorstr = mrb_sym2name(p_state, color);

    DefaultColor col;
    if (colorstr == "brown")
        col = COL_BROWN;
    else if (colorstr == "blue")
        col = COL_BLUE;
    else if (colorstr == "black")
        col = COL_BLACK;
    else {
        mrb_raisef(p_state, MRB_ARGUMENT_ERROR(p_state), "Invalid furball color %s", colorstr.c_str());
        return mrb_nil_value(); // Not reached
    }

    cFurball* p_furball = Get_Data_Ptr<cFurball>(p_state, self);
    p_furball->Set_Color(col);

    return mrb_symbol_value(color);
}

/**
 * Method: Furball#boss?
 *
 *   boss?() → true or false
 *
 * Hurries to check whether this really is the big, black boss furball,
 * and if so, quickly returns true (and then disappears). Otherwise
 * returns false.
 */
static mrb_value Is_Boss(mrb_state* p_state, mrb_value self)
{
    cFurball* p_furball = Get_Data_Ptr<cFurball>(p_state, self);
    return p_furball->m_color_type == COL_BLACK ? mrb_true_value() : mrb_false_value();
}

/**
 * Method: Furball#color
 *
 *   color() → a_symbol
 *
 * Returns the furballs current color as a symbol. For possible
 * values, see L<color=>.
 */
static mrb_value Get_Color(mrb_state* p_state, mrb_value self)
{
    cFurball* p_furball = Get_Data_Ptr<cFurball>(p_state, self);

    switch (p_furball->m_color_type) {
    case COL_BROWN:
        return str2sym(p_state, "brown");
    case COL_BLUE:
        return str2sym(p_state, "blue");
    case COL_BLACK:
        return str2sym(p_state, "black");
    default:
        return mrb_nil_value();
    }
}

/**
 * Method: Furball#max_downgrade_count=
 *
 *   max_downgrade_count=( val ) → val
 *
 * Sets the number of downgrades this boss furball needs to have
 * before it dies.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [val]
 *
 * Maximum number of downgrades before it dies. Cannot be negative.
 *
 * =back
 *
 * =head4 Raises
 *
 * =over
 *
 * =item [TypeError] This is not the big, black boss furball.
 *
 * =back
 */
static mrb_value Set_Max_Downgrade_Count(mrb_state* p_state, mrb_value self)
{
    mrb_int downgrades;
    mrb_get_args(p_state, "i", &downgrades);

    cFurball* p_furball = Get_Data_Ptr<cFurball>(p_state, self);
    if (p_furball->m_color_type != COL_BLACK) {
        mrb_raise(p_state, MRB_TYPE_ERROR(p_state), "This is not a boss furball.");
        return mrb_nil_value(); // Not reached
    }
    else if (downgrades < 0) {
        mrb_raise(p_state, MRB_RANGE_ERROR(p_state), "Max downgrade count must be > 0.");
        return mrb_nil_value(); // Not reached
    }

    p_furball->Set_Max_Downgrade_Count(downgrades);
    return mrb_fixnum_value(downgrades);
}

/**
 * Method: Furball#max_downgrade_count
 *
 *   max_downgrade_count() → an_integer
 *
 * Retrieves the maximum number of downgrades this furball needs
 * to have before it dies. Only useful for the boss furball.
 */
static mrb_value Get_Max_Downgrade_Count(mrb_state* p_state, mrb_value self)
{
    cFurball* p_furball = Get_Data_Ptr<cFurball>(p_state, self);
    return mrb_fixnum_value(p_furball->Get_Max_Downgrade_Count());
}

/**
 * Method: Furball#level_ends_if_killed=
 *
 *   level_ends_if_killed=( val ) → val
 *
 * Set to C<true> if you want the level to automatically end when this
 * furball boss dies.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [val] Wheter or not to end the level.
 *
 * =back
 *
 * =head4 Raises
 *
 * =over
 *
 * =item [TypeError] This is not the big, black furball boss.
 *
 * =back
 */
static mrb_value Set_Level_Ends_If_Killed(mrb_state* p_state, mrb_value self)
{
    mrb_bool ends;
    mrb_get_args(p_state, "b", &ends);

    cFurball* p_furball = Get_Data_Ptr<cFurball>(p_state, self);
    if (p_furball->m_color_type != COL_BLACK) {
        mrb_raise(p_state, MRB_TYPE_ERROR(p_state), "This is not a boss furball.");
        return mrb_nil_value(); // Not reached
    }

    p_furball->Set_Level_Ends_If_Killed(ends);
    return mrb_bool_value(ends);
}

/**
 * Method: Furball#level_ends_if_killed?
 *
 *   level_ends_if_killed? → true or false
 *
 * Whether or not to finish the level if this furball boss gets killed.
 */
static mrb_value Does_Level_End_If_Killed(mrb_state* p_state, mrb_value self)
{
    cFurball* p_furball = Get_Data_Ptr<cFurball>(p_state, self);
    return mrb_bool_value(p_furball->Level_Ends_If_Killed());
}

void TSC::Scripting::Init_Furball(mrb_state* p_state)
{
    struct RClass* p_rcFurball = mrb_define_class(p_state, "Furball", mrb_class_get(p_state, "Enemy"));
    MRB_SET_INSTANCE_TT(p_rcFurball, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcFurball, "initialize", Initialize, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcFurball, "color=", Set_Color, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcFurball, "color", Get_Color, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcFurball, "boss?", Is_Boss, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcFurball, "max_downgrade_count=", Set_Max_Downgrade_Count, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcFurball, "max_downgrade_count", Get_Max_Downgrade_Count, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcFurball, "level_ends_if_killed=", Set_Level_Ends_If_Killed, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcFurball, "level_ends_if_killed?", Does_Level_End_If_Killed, MRB_ARGS_NONE());

    mrb_define_method(p_state, p_rcFurball, "on_downgrade", MRUBY_EVENT_HANDLER(downgrade), MRB_ARGS_NONE());
}

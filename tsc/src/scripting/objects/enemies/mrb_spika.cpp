/***************************************************************************
 * mrb_spika.cpp
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

#include "../../../enemies/spika.hpp"
#include "../../../level/level.hpp"
#include "../../../core/sprite_manager.hpp"
#include "../../../core/property_helper.hpp"
#include "mrb_enemy.hpp"
#include "mrb_spika.hpp"

/**
 * Class: Spika
 *
 * Parent: [Enemy](enemy.html)
 * {: .superclass}
 *
 * _Spikas_ are big, rolling balls armed with spikes. They follow
 * Maryo and try to overroll him. Big spikas tend to block paths,
 * so you somehow need to make them move away. The default spika
 * is not resistant to fire or ice, so you may want to change
 * that using the apropriate methods from the `Enemy` class.
 */

using namespace TSC;
using namespace TSC::Scripting;


/**
 * Method: Spika::new
 *
 *   new() → a_spika
 *
 * Create a new spika enemy with the default values.
 */
static mrb_value Initialize(mrb_state* p_state,  mrb_value self)
{
    cSpika* p_spika = new cSpika(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_spika;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    // This is a generated object
    p_spika->Set_Spawned(true);

    // Let TSC manage the memory
    pActive_Level->m_sprite_manager->Add(p_spika);

    return self;
}

/**
 * Method: Spika#color=
 *
 *   color= ( color ) → color
 *
 * Specifies the color/type of this spika.
 *
 * #### Parameters
 * color
 * : The spika’s new color. One of the following symbols:
 *   `:orange`, `:green`, `:grey`, `:red`.
 */
static mrb_value Set_Color(mrb_state* p_state, mrb_value self)
{
    mrb_sym color;
    mrb_get_args(p_state, "n", &color);
    std::string colorstr(mrb_sym2name(p_state, color));

    DefaultColor col;
    if (colorstr == "orange")
        col = COL_ORANGE;
    else if (colorstr == "green")
        col = COL_GREEN;
    else if (colorstr == "grey")
        col = COL_GREY;
    else if (colorstr == "red")
        col = COL_RED;
    else {
        mrb_raisef(p_state, MRB_ARGUMENT_ERROR(p_state), "Invalid spika color %s", colorstr.c_str());
        return mrb_nil_value(); // Not reached
    }

    cSpika* p_spika = Get_Data_Ptr<cSpika>(p_state, self);
    p_spika->Set_Color(col);

    return mrb_symbol_value(color);
}

/**
 * Method: Spika#color
 *
 *   color() → a_symbol
 *
 * Returns the spika’s current color. See #color= for
 * possible return values.
 */
static mrb_value Get_Color(mrb_state* p_state, mrb_value self)
{
    cSpika* p_spika = Get_Data_Ptr<cSpika>(p_state, self);
    switch (p_spika->m_color_type) {
    case COL_ORANGE:
        return str2sym(p_state, "orange");
    case COL_GREEN:
        return str2sym(p_state, "green");
    case COL_GREY:
        return str2sym(p_state, "grey");
    default:
        return mrb_nil_value();
    }
}

void TSC::Scripting::Init_Spika(mrb_state* p_state)
{
    struct RClass* p_rcSpika = mrb_define_class(p_state, "Spika", mrb_class_get(p_state, "Enemy"));
    MRB_SET_INSTANCE_TT(p_rcSpika, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcSpika, "initialize", Initialize, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcSpika, "color=", Set_Color, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcSpika, "color", Get_Color, MRB_ARGS_NONE());
}

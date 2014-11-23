/***************************************************************************
 * mrb_rokko.cpp
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

#include "../../../enemies/rokko.hpp"
#include "../../../level/level.hpp"
#include "../../../core/sprite_manager.hpp"
#include "../../../core/property_helper.hpp"
#include "mrb_enemy.hpp"
#include "mrb_rokko.hpp"

/**
 * Class: Rokko
 *
 * Parent: [Enemy](enemy.html)
 * {: .superclass}
 *
 * _Rokko_ is the biggest enemy in the game. When he appears, the best you can
 * do usually is this: Run! Rokko is basically invincable and ignores all
 * restrictions put upon other objects such as massive walls or gravity. He
 * just bursts through everything. The only way to defeat Rokko is to somehow
 * get upon his head, which — depending on the circumstances — can be hard
 * to impossible. When not flying sideways, but upwards or downwards, this
 * is entirely impossible.
 *
 * Rokko may either work in self-activating or manual triggering mode,
 * the former being the default. When self-triggering, Rokko will go off
 * as soon as Maryo enters the area Rokko observers towards the front
 * (see `max_front_distance=`) and towards the sides (see `max_sides_distance=`).
 * While for "ordinary" levels created without scripting this behaviour is
 * just fine, but for scripting you might want to activate Rokko depending
 * on other arbitrary conditions (such as enemy death). This is called
 * _manual triggering_ and can be activated like this:
 *
 * ~~~~~~~~~~ ruby
 * rokko = Rokko.new
 * rokko.direction = :right
 * rokko.manual = true
 * ~~~~~~~~~~
 *
 * In manual triggering mode, Rokko doesn’t observe anything and hence
 * does not self-trigger, regardless of Maryo’s position. To fire off,
 * check your condition and call `activate!`:
 *
 * ~~~~~~~~ ruby
 * if some_condition
 *   rokko.activate!
 * end
 * ~~~~~~~~
 */

using namespace TSC;
using namespace TSC::Scripting;


/**
 * Method: Rokko::new
 *
 *   new() → a_rokko
 *
 * Creates a new rokko with the default values.
 */
static mrb_value Initialize(mrb_state* p_state,  mrb_value self)
{
    cRokko* p_rokko = new cRokko(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_rokko;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    // This is a generated object
    p_rokko->Set_Spawned(true);

    // Let TSC manage the memory
    pActive_Level->m_sprite_manager->Add(p_rokko);

    return self;
}

/**
 * Method: Rokko#speed=
 *
 *   speed=( val ) → val
 *
 * TODO: Docs.
 */
static mrb_value Set_Speed(mrb_state* p_state, mrb_value self)
{
    mrb_float speed;
    mrb_get_args(p_state, "f", &speed);

    if (speed <= 0) {
        mrb_raise(p_state, MRB_RANGE_ERROR(p_state), "Rokko speed mustbe > 0");
        return mrb_nil_value(); // Not reached
    }

    cRokko* p_rokko = Get_Data_Ptr<cRokko>(p_state, self);
    p_rokko->Set_Speed(speed);

    return mrb_float_value(p_state, speed);
}

/**
 * Method: Rokko#speed
 *
 *   speed() → a_float
 *
 * TODO: Docs.
 */
static mrb_value Get_Speed(mrb_state* p_state, mrb_value self)
{
    cRokko* p_rokko = Get_Data_Ptr<cRokko>(p_state, self);
    return mrb_float_value(p_state, p_rokko->m_speed);
}

/**
 * Method: Rokko#max_front_distance=
 *
 *   max_front_distance=( val ) → val
 *
 * Sets the maximum number of pixels Rokko will go off in if Maryo
 * enters that room before Rokko’s head.
 *
 * #### Parameters
 * val
 * : Maximum number of pixels Rokko will use as detection area.
 */
static mrb_value Set_Max_Distance_Front(mrb_state* p_state, mrb_value self)
{
    mrb_float distance;
    mrb_get_args(p_state, "f", &distance);

    if (distance < 0) {
        mrb_raise(p_state, MRB_RANGE_ERROR(p_state), "Rokko max front distance must be >= 0.");
        return mrb_nil_value(); // Not reached
    }

    cRokko* p_rokko = Get_Data_Ptr<cRokko>(p_state, self);
    p_rokko->Set_Max_Distance_Front(distance);

    return mrb_float_value(p_state, distance);
}

/**
 * Method: Rokko#max_front_distance
 *
 *   max_front_distance() → a_float
 *
 * Returns the maximum number of pixels Rokko will use as the Maryo
 * detection area to the front.
 */
static mrb_value Get_Max_Distance_Front(mrb_state* p_state, mrb_value self)
{
    cRokko* p_rokko = Get_Data_Ptr<cRokko>(p_state, self);
    return mrb_float_value(p_state, p_rokko->m_max_distance_front);
}

/**
 * Method: Rokko#max_sides_distance=
 *
 *   max_sides_distance=( val ) → val
 *
 * Sets the maximum number of pixels Rokko will go off in if Maryo
 * enters that room next to Rokko (but not behind).
 *
 * #### Parameters
 * val
 * : Maximum number of pixels Rokko will use as detection area.
 */
static mrb_value Set_Max_Distance_Sides(mrb_state* p_state, mrb_value self)
{
    mrb_float distance;
    mrb_get_args(p_state, "f", &distance);

    if (distance < 0) {
        mrb_raise(p_state, MRB_RANGE_ERROR(p_state), "Rokko max sides distance must be >= 0.");
        return mrb_nil_value(); // Not reached
    }

    cRokko* p_rokko = Get_Data_Ptr<cRokko>(p_state, self);
    p_rokko->Set_Max_Distance_Sides(distance);

    return mrb_float_value(p_state, distance);
}

/**
 * Method: Rokko#max_sides_distance
 *
 *   max_sides_distance() → a_float
 *
 * Returns the maximum number of pixels Rokko will use as the Maryo
 * detection area to the sides.
 */
static mrb_value Get_Max_Distance_Sides(mrb_state* p_state, mrb_value self)
{
    cRokko* p_rokko = Get_Data_Ptr<cRokko>(p_state, self);
    return mrb_float_value(p_state, p_rokko->m_max_distance_sides);
}

/**
 * Method: Rokko#manual=
 *
 *   manual=( bool ) → bool
 *
 * Set this to `true` in order to activate manual triggering. `false`
 * activates self-triggering mode.
 */
static mrb_value Set_Manual(mrb_state* p_state, mrb_value self)
{
    mrb_bool manual;
    mrb_get_args(p_state, "b", &manual);

    cRokko* p_rokko = Get_Data_Ptr<cRokko>(p_state, self);
    p_rokko->Set_Manual(manual);

    return mrb_bool_value(manual);
}

/**
 * Method: Rokko#manual?
 *
 *   manual?() → true or false
 *
 * Whether or not Rokko is in manual triggering mode.
 */
static mrb_value Is_Manual(mrb_state* p_state, mrb_value self)
{
    cRokko* p_rokko = Get_Data_Ptr<cRokko>(p_state, self);
    return mrb_bool_value(p_rokko->Get_Manual());
}

/**
 * Method: Rokko#flying?
 *
 *   flying?() → true or false
 *
 * Checks wheter Rokko has been activated, either manually or
 * by self-triggering.
 */
static mrb_value Is_Flying(mrb_state* p_state, mrb_value self)
{
    cRokko* p_rokko = Get_Data_Ptr<cRokko>(p_state, self);
    return p_rokko->m_state == STA_FLY ? mrb_true_value() : mrb_false_value();
}

/**
 * Method: Rokko#activate
 *
 *   activate()
 *
 * Triggers Rokko. He will fly and downgrade Maryo if he is in
 * his way.
 */
static mrb_value Activate(mrb_state* p_state, mrb_value self)
{
    cRokko* p_rokko = Get_Data_Ptr<cRokko>(p_state, self);
    p_rokko->Activate();
    return mrb_nil_value();
}

/**
 * Method: Rokko#activate!
 *
 *   activate!()
 *
 * Like #activate, but does not play the Rokko start sound. The player
 * will not notice Rokko has started!
 */
static mrb_value Activate_Bang(mrb_state* p_state, mrb_value self)
{
    cRokko* p_rokko = Get_Data_Ptr<cRokko>(p_state, self);
    p_rokko->Activate(false);
    return mrb_nil_value();
}

void TSC::Scripting::Init_Rokko(mrb_state* p_state)
{
    struct RClass* p_rcRokko = mrb_define_class(p_state, "Rokko", mrb_class_get(p_state, "Enemy"));
    MRB_SET_INSTANCE_TT(p_rcRokko, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcRokko, "initialize", Initialize, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcRokko, "speed=", Set_Speed, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcRokko, "speed", Get_Speed, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcRokko, "max_front_distance=", Set_Max_Distance_Front, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcRokko, "max_front_distance", Get_Max_Distance_Front, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcRokko, "max_sides_distance=", Set_Max_Distance_Sides, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcRokko, "max_sides_distance", Get_Max_Distance_Sides, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcRokko, "manual=", Set_Manual, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcRokko, "manual?", Is_Manual, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcRokko, "flying?", Is_Flying, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcRokko, "activate", Activate, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcRokko, "activate!", Activate_Bang, MRB_ARGS_NONE());
}

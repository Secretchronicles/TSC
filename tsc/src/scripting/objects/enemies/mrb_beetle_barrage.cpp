/***************************************************************************
 * mrb_beetle_barrage.cpp
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

#include "../../../enemies/beetle_barrage.hpp"
#include "../../../level/level.hpp"
#include "../../../core/sprite_manager.hpp"
#include "../../../core/property_helper.hpp"
#include "mrb_enemy.hpp"
#include "mrb_beetle_barrage.hpp"
#include "../../events/event.hpp"

/**
 * Class: BeetleBarrage
 *
 * Parent: L<Enemy>
 *
 * The I<Beetle Barrage> is a giant plant that spits out a number
 * of small bugs at a time to defend itself when Alex comes in
 * range. They generally don’t move (unless you script them
 * to), but the bugs are very vivid (see L<Beetle> if
 * you want to generate some of them standalone).
 *
 * =head2 Events
 *
 * =over
 *
 * =item [Spit]
 *
 * Whenever this enemy spits out beetles, this event is triggered.
 * Note there is only one event for one beetle bulk, the event
 * is not triggered for each single beetle.
 *
 * =back
 */

using namespace TSC;
using namespace TSC::Scripting;


MRUBY_IMPLEMENT_EVENT(spit);

/**
 * Method: BeetleBarrage::new
 *
 *   new() → a_beetle_barrage
 *
 * Creates a new instance of this class.
 *
 * =head4 Return value
 *
 * A new instance.
 */
static mrb_value Initialize(mrb_state* p_state,  mrb_value self)
{
    cBeetleBarrage* p_bb = new cBeetleBarrage(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_bb;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    // This is a generated object
    p_bb->Set_Spawned(true);

    // Let TSC manage the memory
    pActive_Level->m_sprite_manager->Add(p_bb);

    return self;
}

/**
 * Method: BeetleBarrage#active_range=
 *
 *   active_range=( range ) → range
 *
 * Sets the radius in which the plant reacts on Alex, i.e. in which
 * it will spit out beetles.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [range] Range radius.
 *
 * =back
 */
static mrb_value Set_Active_Range(mrb_state* p_state, mrb_value self)
{
    float range = 0.0f;
    mrb_get_args(p_state, "f", &range);

    cBeetleBarrage* p_bb = Get_Data_Ptr<cBeetleBarrage>(p_state, self);
    p_bb->Set_Active_Range(range);

    return mrb_float_value(p_state, range);
}

/**
 * Method: BeetleBarrage#active_range
 *
 *   active_range() → a_float
 *
 * Returns the radius in which the plant reacts on Alex.
 */
static mrb_value Get_Active_Range(mrb_state* p_state, mrb_value self)
{
    cBeetleBarrage* p_bb = Get_Data_Ptr<cBeetleBarrage>(p_state, self);
    return mrb_float_value(p_state, p_bb->Get_Active_Range());
}

/**
 * Method: BeetleBarrage#spit_count=
 *
 *   spit_count=( count ) → count
 *
 * Sets the number of bugs spit out at a time.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [count] Number of beetles to spit out.
 *
 * =back
 *
 */
static mrb_value Set_Spit_Count(mrb_state* p_state, mrb_value self)
{
    int count = 0;
    mrb_get_args(p_state, "i", &count);

    cBeetleBarrage* p_bb = Get_Data_Ptr<cBeetleBarrage>(p_state, self);
    p_bb->Set_Beetle_Spit_Count(count);

    return mrb_fixnum_value(count);
}

/**
 * Method: BeetleBarrage#spit_count
 *
 *   spit_count() → an_integer
 *
 * Returns the number of bugs to spit out at a time.
 */
static mrb_value Get_Spit_Count(mrb_state* p_state, mrb_value self)
{
    cBeetleBarrage* p_bb = Get_Data_Ptr<cBeetleBarrage>(p_state, self);
    return mrb_fixnum_value(p_bb->Get_Beetle_Spit_Count());
}

/**
 * Method: BeetleBarrage#fly_distance=
 *
 *   fly_distance=( distance ) → distance
 *
 * Set the distance which the bugs fly upward before flying randomly
 * into all directions.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [distance]
 *
 * The number of pixels to fly up before starting randomness.
 *
 * =back
 */
static mrb_value Set_Fly_Distance(mrb_state* p_state, mrb_value self)
{
    float distance = 0.0f;
    mrb_get_args(p_state, "f", &distance);

    cBeetleBarrage* p_bb = Get_Data_Ptr<cBeetleBarrage>(p_state, self);
    p_bb->Set_Beetle_Fly_Distance(distance);

    return mrb_float_value(p_state, distance);
}

/**
 * Method: BeetleBarrage#fly_distance
 *
 *   fly_distance() → a_float
 *
 * Returns the number of pixels generated bugs fly upwards before
 * flying around randomly.
 */
static mrb_value Get_Fly_Distance(mrb_state* p_state, mrb_value self)
{
    cBeetleBarrage* p_bb = Get_Data_Ptr<cBeetleBarrage>(p_state, self);
    return mrb_float_value(p_state, p_bb->Get_Beetle_Fly_Distance());
}

void TSC::Scripting::Init_BeetleBarrage(mrb_state* p_state)
{
    struct RClass* p_rcBeetleBarrage = mrb_define_class(p_state, "BeetleBarrage", mrb_class_get(p_state, "Enemy"));
    MRB_SET_INSTANCE_TT(p_rcBeetleBarrage, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcBeetleBarrage, "initialize", Initialize, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcBeetleBarrage, "active_range", Get_Active_Range, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcBeetleBarrage, "active_range=", Set_Active_Range, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcBeetleBarrage, "spit_count", Get_Spit_Count, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcBeetleBarrage, "spit_count=", Set_Spit_Count, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcBeetleBarrage, "fly_distance", Get_Fly_Distance, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcBeetleBarrage, "fly_distance=", Set_Fly_Distance, MRB_ARGS_REQ(1));

    mrb_define_method(p_state, p_rcBeetleBarrage, "on_spit", MRUBY_EVENT_HANDLER(spit), MRB_ARGS_NONE());
}

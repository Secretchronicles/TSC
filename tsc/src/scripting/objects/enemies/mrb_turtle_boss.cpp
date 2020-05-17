/***************************************************************************
 * mrb_turtle_boss.cpp
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

#include "../../../enemies/bosses/turtle_boss.hpp"
#include "../../../level/level.hpp"
#include "../../../core/sprite_manager.hpp"
#include "../../../core/property_helper.hpp"
#include "../../events/event.hpp"
#include "mrb_enemy.hpp"
#include "mrb_turtle_boss.hpp"

/**
 * Class: TurtleBoss
 *
 * Parent: L<Enemy>
 *
 * The I<Turtle Boss> is a giant turtle that needs multiple hits
 * to be defeated. It can throw fireballs while hiding in its
 * shell.
 *
 * =head2 Events
 *
 * =over
 *
 * =item [Downgrade]
 *
 * This event is triggered each time the Turtle Boss takes damage.
 * Not it doesn't take damage for each hit. The event handler
 * gets passed two arguments: how often it took damage, and how
 * often it may take damage before it dies.
 *
 * =back
 */

using namespace TSC;
using namespace TSC::Scripting;

MRUBY_IMPLEMENT_EVENT(downgrade);

/**
 * Method: TurtleBoss::new
 *
 *   new() → a_turtle_boss
 *
 * Creates a new turtle boss with the default values.
 */
static mrb_value Initialize(mrb_state* p_state,  mrb_value self)
{
    cTurtleBoss* p_turtle = new cTurtleBoss(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_turtle;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    // This is a generated object
    p_turtle->Set_Spawned(true);

    // Let TSC manage the memory
    pActive_Level->m_sprite_manager->Add(p_turtle);

    return self;
}

/**
 * Method: TurtleBoss#walking?
 *
 *   walking?() → true or false
 *
 * Checks whether or not the turtle is walking currently.
 */
static mrb_value Is_Walking(mrb_state* p_state, mrb_value self)
{
    cTurtleBoss* p_turtle = Get_Data_Ptr<cTurtleBoss>(p_state, self);
    return p_turtle->m_turtle_state == TURTLEBOSS_WALK ? mrb_true_value() : mrb_false_value();
}

/**
 * Method: TurtleBoss#shell_standing?
 *
 *   shell_standing?() → true or false
 *
 * Checks whether or not the turtle is in its shell and not moving.
 */
static mrb_value Is_Shell_Standing(mrb_state* p_state, mrb_value self)
{
    cTurtleBoss* p_turtle = Get_Data_Ptr<cTurtleBoss>(p_state, self);
    return p_turtle->m_turtle_state == TURTLEBOSS_SHELL_STAND ? mrb_true_value() : mrb_false_value();
}

/**
 * Method: TurtleBoss#shell_moving?
 *
 *   shell_moving?() → true or false
 *
 * Checks whether or not the turtle is in its shell and moving.
 */
static mrb_value Is_Shell_Moving(mrb_state* p_state, mrb_value self)
{
    cTurtleBoss* p_turtle = Get_Data_Ptr<cTurtleBoss>(p_state, self);
    return p_turtle->m_turtle_state == TURTLEBOSS_SHELL_RUN ? mrb_true_value() : mrb_false_value();
}

/**
 * Method: TurtleBoss#shelled?
 *
 *   shelled?() → true or false
 *
 * Checks whether or not the turtle is in its shell, regardless of
 * whether the shell is moving or not.
 */
static mrb_value Is_Shelled(mrb_state* p_state, mrb_value self)
{
    cTurtleBoss* p_turtle = Get_Data_Ptr<cTurtleBoss>(p_state, self);
    return p_turtle->m_turtle_state == TURTLEBOSS_SHELL_RUN || p_turtle->m_turtle_state == TURTLEBOSS_SHELL_STAND ? mrb_true_value() : mrb_false_value();
}

/**
 * Method: TurtleBoss#angry?
 *
 *   angry?() → true or false
 *
 * Checks if the turtle boss is angry, and if so, returns
 * C<true>, otherwise returns C<false>.
 */
static mrb_value Is_Angry(mrb_state* p_state, mrb_value self)
{
    cTurtleBoss* p_turtle = Get_Data_Ptr<cTurtleBoss>(p_state, self);
    return p_turtle->m_turtle_state == TURTLEBOSS_STAND_ANGRY ? mrb_true_value() : mrb_false_value();
}

/**
 * Method: TurtleBoss#stand_up
 *
 *   stand_up()
 *
 * If the turtle is in its shell, make it come out and walk again.
 * If the turtle is not in its shell, does nothing.
 */
static mrb_value Stand_Up(mrb_state* p_state, mrb_value self)
{
    cTurtleBoss* p_turtle = Get_Data_Ptr<cTurtleBoss>(p_state, self);
    p_turtle->Stand_Up();

    return mrb_nil_value();
}

/**
 * Method: TurtleBoss#throw_fireballs
 *
 *   throw_fireballs( [ amount ] )
 *
 * Throw fireballs in all directions.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [amount (6)] The number of fireballs to throw.
 *
 * =back
 */
static mrb_value Throw_Fireballs(mrb_state* p_state, mrb_value self)
{
    mrb_int amount = -1;
    mrb_get_args(p_state, "|i", &amount);

    cTurtleBoss* p_turtle = Get_Data_Ptr<cTurtleBoss>(p_state, self);

    if (amount < 0)
        p_turtle->Throw_Fireballs();
    else
        p_turtle->Throw_Fireballs(amount);

    return mrb_nil_value();
}

/**
 * Method: TurtleBoss#max_hits=
 *
 *   max_hits=( hits ) → hits
 *
 * Specify the maximum number of hits this turtle
 * boss needs before it gets a single (!) downgrade.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [hits] The new number of maximum hits.
 *
 * =back
 */
static mrb_value Set_Max_Hits(mrb_state* p_state, mrb_value self)
{
    mrb_int hits;
    mrb_get_args(p_state, "i", &hits);

    if (hits <= 0) {
        mrb_raise(p_state, MRB_RANGE_ERROR(p_state), "Turtle boss maximum hits must be > 0.");
        return mrb_nil_value();
    }

    cTurtleBoss* p_turtle = Get_Data_Ptr<cTurtleBoss>(p_state, self);
    p_turtle->Set_Max_Hits(hits);

    return mrb_fixnum_value(hits);
}

/**
 * Method: TurtleBoss#max_hits
 *
 *   max_hits() → an_integer
 *
 * The maximum number of hits this turtle boss needs before
 * it is downgraded.
 */
static mrb_value Get_Max_Hits(mrb_state* p_state, mrb_value self)
{
    cTurtleBoss* p_turtle = Get_Data_Ptr<cTurtleBoss>(p_state, self);
    return mrb_fixnum_value(p_turtle->m_max_hits);
}

/**
 * Method: TurtleBoss#max_downgrade_count=
 *
 *   max_downgrade_count=( count ) → count
 *
 * Set the maximum number of downgrades this turtle
 * boss needs before it dies.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [:count] The new maximum number of downgrades.
 *
 * =back
 */
static mrb_value Set_Max_Downgrade_Count(mrb_state* p_state, mrb_value self)
{
    mrb_int downgrades;
    mrb_get_args(p_state, "i", &downgrades);

    if (downgrades < 0) {
        mrb_raise(p_state, MRB_RANGE_ERROR(p_state), "Maximum turtle boss downgrade count must be >= 0.");
        return mrb_nil_value();
    }

    cTurtleBoss* p_turtle = Get_Data_Ptr<cTurtleBoss>(p_state, self);
    p_turtle->Set_Max_Downgrade_Counts(downgrades);

    return mrb_fixnum_value(downgrades);
}

/**
 * Method: TurtleBoss#shell_time=
 *
 *   shell_time=( val ) → val
 *
 * Set the time the turtle boss stays in the shell
 * before coming out again.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [val] The new in-shell-stay time.
 *
 * =back
 */
static mrb_value Set_Shell_Time(mrb_state* p_state, mrb_value self)
{
    mrb_float time;
    mrb_get_args(p_state, "f", &time);

    if (time < 0) {
        mrb_raise(p_state, MRB_RANGE_ERROR(p_state), "Turtle boss shell time must be >= 0.");
        return mrb_nil_value();
    }

    cTurtleBoss* p_turtle = Get_Data_Ptr<cTurtleBoss>(p_state, self);
    p_turtle->Set_Shell_Time(time);

    return mrb_float_value(p_state, time);
}

/**
 * Method: TurtleBoss#shell_time
 *
 *   shell_time() → a_float
 *
 * Returns the time the turtle boss stays in the shell.
 */
static mrb_value Get_Shell_Time(mrb_state* p_state, mrb_value self)
{
    cTurtleBoss* p_turtle = Get_Data_Ptr<cTurtleBoss>(p_state, self);
    return mrb_float_value(p_state, p_turtle->Get_Shell_Time());
}

/**
 * Method: TurtleBoss#max_downgrade_count
 *
 *   max_downgrade_count() → an_integer
 *
 * Return the maximum number of downgrades this turtle
 * boss needs before it dies.
 */
static mrb_value Get_Max_Downgrade_Count(mrb_state* p_state, mrb_value self)
{
    cTurtleBoss* p_turtle = Get_Data_Ptr<cTurtleBoss>(p_state, self);
    return mrb_fixnum_value(p_turtle->Get_Max_Downgrade_Count());
}

/**
 * Method: TurtleBoss#level_ends_if_killed=
 *
 *   level_ends_if_killed=( bool ) → bool
 *
 * Set to true to have the level finish when the turtle
 * boss dies (the default).
 *
 * =head4 Parameter
 *
 * =over
 *
 * =item [bool]
 *
 * C<true> to end the level on death, C<false> to do nothing.
 *
 * =back
 */
static mrb_value Set_Level_Ends_If_Killed(mrb_state* p_state, mrb_value self)
{
    mrb_bool ends;
    mrb_get_args(p_state, "b", &ends);

    cTurtleBoss* p_turtle = Get_Data_Ptr<cTurtleBoss>(p_state, self);
    p_turtle->Set_Level_Ends_If_Killed(ends);

    return mrb_bool_value(ends);
}

/**
 * Method: TurtleBoss#level_ends_if_killed?
 *
 *   level_ends_if_killed? → true or false
 *
 * Checks if the level ends when this turtle boss gets
 * killed, and returns C<true> if so, C<false> otherwise.
 */
static mrb_value Does_Level_End_If_Killed(mrb_state* p_state, mrb_value self)
{
    cTurtleBoss* p_turtle = Get_Data_Ptr<cTurtleBoss>(p_state, self);
    return mrb_bool_value(p_turtle->Get_Level_Ends_If_Killed());
}

/**
 * Method: TurtleBoss#downgrade_count
 *
 *   downgrade_count() → an_integer
 *
 * Returns the number of times this turtle boss has already
 * been downgraded.
 */
static mrb_value Get_Downgrade_Count(mrb_state* p_state, mrb_value self)
{
    cTurtleBoss* p_turtle = Get_Data_Ptr<cTurtleBoss>(p_state, self);
    return mrb_fixnum_value(p_turtle->Get_Downgrade_Count());
}

void TSC::Scripting::Init_TurtleBoss(mrb_state* p_state)
{
    struct RClass* p_rcTurtleBoss = mrb_define_class(p_state, "TurtleBoss", mrb_class_get(p_state, "Enemy"));
    MRB_SET_INSTANCE_TT(p_rcTurtleBoss, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcTurtleBoss, "initialize", Initialize, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcTurtleBoss, "walking?", Is_Walking, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcTurtleBoss, "shell_standing?", Is_Shell_Standing, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcTurtleBoss, "shell_moving?", Is_Shell_Moving, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcTurtleBoss, "shelled?", Is_Shelled, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcTurtleBoss, "angry?", Is_Angry, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcTurtleBoss, "stand_up", Stand_Up, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcTurtleBoss, "throw_fireballs", Throw_Fireballs, MRB_ARGS_OPT(1));
    mrb_define_method(p_state, p_rcTurtleBoss, "max_hits=", Set_Max_Hits, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcTurtleBoss, "max_hits", Get_Max_Hits, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcTurtleBoss, "max_downgrade_count=", Set_Max_Downgrade_Count, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcTurtleBoss, "max_downgrade_count", Get_Max_Downgrade_Count, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcTurtleBoss, "shell_time=", Set_Shell_Time, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcTurtleBoss, "shell_time", Get_Shell_Time, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcTurtleBoss, "level_ends_if_killed=", Set_Level_Ends_If_Killed, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcTurtleBoss, "level_ends_if_killed?", Does_Level_End_If_Killed, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcTurtleBoss, "downgrade_count", Get_Downgrade_Count, MRB_ARGS_NONE());

    mrb_define_method(p_state, p_rcTurtleBoss, "on_downgrade", MRUBY_EVENT_HANDLER(downgrade), MRB_ARGS_NONE());
}

/***************************************************************************
 * mrb_timer.cpp
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

#include <cstdio>
#include "../../../level/level.hpp"
#include "mrb_timer.hpp"

/**
 * Class: Timer
 *
 * _Timers_ are an easy and efficient way to execute MRuby code based on a
 * time span. They can either be periodic, meaning that they will
 * continue to be active for an unspecified amount of time, or
 * non-periodic aka one-shot, meaning that they will only be active for a
 * definite timespan. For both types of timers, you have to call the
 * [#start](#start) method to activate them, and it’s possible to
 * interrupt a timer by means of its [#stop](#stop) method (you can also
 * abort a one-shot timer this way if it has not yet fired).
 *
 * Both types of timers employ a _callback_ concept similar to how events
 * work. When creating a timer with [::new](#new) or one of the
 * convenience methods [::after](#after) and [::every](#every), you pass
 * a _callback_ to the method which will be invoked whenever the timer
 * fires.
 *
 * A _periodic_ timer enters an infine loop when [#start](#start) is
 * called on it. It will then wait the inverval specified when creating
 * the timer and execute the callback. That process is repeated until you
 * either call one of the stop methods or end the level.
 *
 * A _non-periodic_ or _one-shot_ timer doesn’t loop. When [#start](#start) is
 * called, it waits the amount of time it is configured for (just like a periodic
 * timer does) and then executes the callback. However, the
 * timer will not continue to do anything beyond this. No looping is
 * done, nor any cleanup.
 *
 * Timers of any type do *not* run in parallel. Although the actual
 * marking of callbacks for run indeed *is* asynchronous, the callback itself
 * is executed while evaluating the game’s regular mainloop (a consequence
 * of this is that your callback won’t be called with 100% accuracy
 * regarding the timespan, it will be cropped to the next
 * frame). Therefore it is recommended to not put very time-consuming
 * actions into a timer’s callback function as it will slow down the
 * entire game. For example, you do _not_ want to calculate π inside your
 * timer’s callback function. Moving objects around on the other hand
 * should be OK.
 *
 * Note a particularity with objects of this class: Even when a timer
 * goes out of scope, it doesn’t cease to exist (instead, the instances
 * are remembered in an internal class-instance variable). So, if you
 * create a periodic timer like this:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ruby
 * def create_timer
 *   timer = Timer.new(1000, true){puts "Hi there"}
 *   timer.start
 * end
 *
 * create_timer
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * You might expect the periodic timer to stop working when the `timer`
 * variable goes out of scope and the MRuby DATA it references gets
 * garbage-collected, deallocating the C++ instance. This his however not
 * the case; to prevent you from having to create a massive amount of global
 * objects for your timers, a C++ timer, once started, will continue to work
 * even after its MRuby counterpart has gone out of your reach. The bottomside of this
 * is that you can’t influence the timer anymore after it has gone out of your scope,
 * there is no way to get a reference to it again. The only thing you can
 * do to force the C++ timer to stop is to end the level, which is
 * probably not what you want :-).
 *
 * Having that said, I want to enourage you to be brave and let your
 * timers go out of scope. You do not have to call [#stop](#stop)
 * manually when ending the level, this is done automatically for you. So
 * no reason to clutter the global scope with timers.
 *
 * Last but not least you shouldn’t use [::new](#new) directly. Use the
 * [::after](#after) and [::every](#every) class methods instead, as they
 * make your intention more clear and are more readable than a `true` or
 * `false` passed to [::new](#new) and they also call [#start](#start)
 * automatically for you.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ruby
 * Timer.every(1000) do
 *   puts "Callback"
 * end
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

//////////////////////////////////////////////
// C++ developer’s info                     //
//////////////////////////////////////////////

/**** How the timers and their callbacks work ****
 * Creating an active timer is a two-step process. The first
 * step is to instanciate the cTimer class, which remembers
 * the values needed for adjusting the timer -- most
 * importantly the waiting time and the callback. The
 * callback is an MRuby proc which must have been stored
 * somewhere permanently by the caller of cTimer’s constructor,
 * because it mustn’t go out of scope in MRuby land while the
 * timer is ticking.
 *
 * You then call the timer’s Start() method which creates a
 * new thread (boost::thread) that waits the time specified
 * by the timer initialisation. Rather than immediately exe-
 * cuting the callback in the thread’s context (which is a
 * very complex task due to MRuby not having native multi-
 * threading facilities) it calls
 * MRuby_Interpreter::Register_Callback_Index(), which adds
 * (protected by a mutex) the callback to a list of pending
 * callbacks (m_callbacks).
 * This list is iterated once a frame in cLevel::Update()
 * and executes the callbacks, of course protected by the same mutex
 * around the m_callbacks variable access. After all pending
 * callbacks have been executed, the list is cleared and normal
 * gameplay resumes. This way the callbacks are executed synchronous
 * to the rest of the TSC and MRuby stuff, while still allowing
 * the callback "injection" to remain asynchronous and
 * therefore high-precision. The payoff is that although the
 * actual "injection" is (nearly) asynchronous (there is this
 * mutex preventing race conditions around the list of
 * pending callbacks), the actual *execution* is
 * a bit delayed, as it will only happen when the normal
 * mainloop comes over cLevel::Update(), which is usually
 * once a frame for normal gameplay (i.e. not for
 * an active editor or the menu).
 *
 * Calling Stop() on a timer ends the underlying thread as soon as
 * possible, but not necessarily immediately, because the termination
 * condition is only checked after the callback execution.  The thread
 * is then deleted, and Stop() returns.
 *
 * To forcibly terminate a timer, call Interrupt(). This will throw a
 * C++ exception in the timer thread, which makes the thread
 * immediately halt (actually, the exception is rescued and some
 * cleanup is performed so that the `m_stopped' member is set
 * correctly). If a timer instance is deleted some way or another,
 * it’s destructor automatically calls Interrupt() for a running timer.
 *
 * The timers created from the MRuby code a user supplies
 * are automatically (in their #initialize method) stored
 * in a class-instance variable `instances' of the Timer
 * MRuby class, so they can’t go out of scope and get
 * garbage-collected. Also, the callbacks are stored
 * in an instance variable of the Timer instances, which
 * as the instances can’t get GC’ed will not get GC’ed
 * either. This allows the C++ timer to go on ticking even
 * if the referencing MRuby DATA object has gone out of scope.
 * There is no way to trigger the C++ timer’s deletion from the MRuby
 * side except for ending the level (because the MRuby instances
 * holding the C++ pointers don’t get GC’ed and the C++ pointers
 * are freed in the MRuby objects’ respective deallocation functions). */

using namespace TSC;
using namespace TSC::Scripting;


/***************************************
 * C++ part
 ***************************************/

// Note this method is ever and only called from Timer.new on the
// Mruby side, hence we don’t need to secure `callback' for the GC
// here. This is already done in Timer.new.
cTimer::cTimer(cMRuby_Interpreter* p_mruby, unsigned int interval, mrb_value callback, bool is_periodic /* = false */)
{
    mp_mruby            = p_mruby;
    m_interval          = interval;
    m_is_periodic       = is_periodic;
    m_callback          = callback;
    m_halt              = false;
    m_stopped           = true;
    mp_thread           = NULL;
}

cTimer::~cTimer()
{
    // If the timer is ticking currently, stop it.
    // This automatically deletes the thread.
    if (mp_thread)
        Interrupt();
}

void cTimer::Start()
{
    if (mp_thread)
        return;

    m_halt = false;
    m_stopped = false;

    mp_thread = new boost::thread(Threading_Function, this);
}

void cTimer::Stop()
{
    if (!mp_thread)
        return;

    m_halt = true;

    // A oneshot-timer thread may has ended when Stop()
    // gets called, but terminated threads can still be joined.
    mp_thread->join();

    delete mp_thread;
    mp_thread = NULL;
}

bool cTimer::Shall_Halt()
{
    return m_halt;
}

void cTimer::Interrupt()
{
    if (!mp_thread)
        return;

    /* A oneshot-timer may has ended when Interrupt()
     * gets called, but terminated threads can still be
     * interrupted and joined -- these method then just
     * do nothing. */
    mp_thread->interrupt();
    mp_thread->join();

    delete mp_thread;
    mp_thread = NULL;
}

bool cTimer::Is_Active()
{
    return !m_stopped;
}

bool cTimer::Is_Periodic()
{
    return m_is_periodic;
}

void cTimer::Set_Stopped() // Private API
{
    m_stopped = true;
}

unsigned int cTimer::Get_Interval()
{
    return m_interval;
}

boost::thread* cTimer::Get_Thread()
{
    return mp_thread;
}

mrb_value cTimer::Get_Callback()
{
    return m_callback;
}

cMRuby_Interpreter* cTimer::Get_MRuby_Interpreter()
{
    return mp_mruby;
}

void cTimer::Threading_Function(cTimer* timer)
{
    try {
        if (timer->Is_Periodic()) {
            while (true) {
                boost::this_thread::sleep_for(boost::chrono::milliseconds(timer->Get_Interval()));

                timer->Get_MRuby_Interpreter()->Register_Callback(timer->Get_Callback()); // This method is threadsafe

                // If soft stop was requested, now terminate.
                if (timer->Shall_Halt())
                    break;
            }
        }
        else { // One-shot timer
            boost::this_thread::sleep_for(boost::chrono::milliseconds(timer->Get_Interval()));

            timer->Get_MRuby_Interpreter()->Register_Callback(timer->Get_Callback());

            // Soft stop does not make sense for oneshot timers, so no code here.
        }
    }
    catch (boost::thread_interrupted& e) {
        // That exception is shown when boost::thread::interrupt() is called
        // (which we do in cTimer::Interrupt()).
        debug_print("boost::thread_interrupted received, terminating timer thread.\n");
    }

    // Mark the timer as stopped.
    timer->Set_Stopped();
}

/***************************************
 * MRuby side
 ***************************************/

/**
 * Method: Timer::new
 *
 *   new( interval [, is_periodic ] ){...} → a_timer
 *
 * Creates a new `Timer` instance, either periodic or non-periodic
 * depending on the last parameter’s value.
 *
 * #### Parameters
 *
 * interval
 * : The timespan to configure the timer for, in milliseconds. With a
 *   periodic timer, this is the waiting time between calls to your
 *   callback function, with a non-periodic timer this is the time to
 *   wait before the one and only call to your callback.
 *
 * is_periodic (false)
 * : If this is a truth value, create a periodic (repeating) timer
 *   instead of a non-repeating (one-shot) timer.
 *
 * #### Return value
 *
 * The newly created instance.
 */
static mrb_value Initialize(mrb_state* p_state,  mrb_value self)
{
    mrb_int interval;
    mrb_value is_periodic = mrb_false_value();
    mrb_value block;
    mrb_get_args(p_state, "i|o&", &interval, &is_periodic, &block);

    // The cTimer constructor needs the currently active cMRuby_Interpreter
    // instance which is not reachable via the mrb_state*, hence we retrieve
    // it from the currently active level which at this point is always the
    // same MRuby instance as `p_state'.
    cTimer* p_timer = new cTimer(pActive_Level->m_mruby, interval, block, mrb_test(is_periodic));
    DATA_PTR(self) = p_timer;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    // Prevent the GC from collecting the objects by a) adding ourselves
    // to the class-instance variable instances and b) adding the callback
    // to ourselves (the GC doesn’t see the reference in the C++ cTimer
    // instance).
    mrb_value klass = mrb_obj_value(mrb_obj_class(p_state, self));
    mrb_ary_push(p_state, mrb_iv_get(p_state, klass, mrb_intern_cstr(p_state, "instances")), self);
    mrb_iv_set(p_state, self, mrb_intern_cstr(p_state, "callback"), block);

    return self;
}

/**
 * Method: Timer::every
 *
 *   every( interval ){...} → a_timer
 *
 * Shortcut for calling [new()](#new) with `is_periodic = true` followed
 * by a call to [#start](#start).
 *
 * #### Parameters
 * interval
 * : The interval at which to fire the callback.
 *
 * #### Return value
 *
 * The newly created instance.
 */
static mrb_value Every(mrb_state* p_state,  mrb_value self)
{
    mrb_int interval;
    mrb_value block;
    mrb_get_args(p_state, "i&", &interval, &block);

    cTimer* p_timer = new cTimer(pActive_Level->m_mruby, interval, block, true);

    mrb_value instance = mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "Timer"), &rtTSC_Scriptable, p_timer));

    // Prevent mruby timer from getting out of scope
    mrb_ary_push(p_state, mrb_iv_get(p_state, self, mrb_intern_cstr(p_state, "instances")), instance);
    mrb_iv_set(p_state, instance, mrb_intern_cstr(p_state, "callback"), block);

    p_timer->Start();
    return instance;
}

/**
 * Method: Timer::after
 *
 *   after( secs ){...} → a_timer
 *
 * Shortcut for calling [new()](#new) with `is_periodic = false` followed
 * by a call to [#start](#start)..
 *
 * #### Parameters
 * secs
 * : The number of seconds to wait before the callback gets
 *   executed.
 *
 * #### Return value
 *
 * The newly created instance.
 */
static mrb_value After(mrb_state* p_state,  mrb_value self)
{
    mrb_int secs;
    mrb_value block;
    mrb_get_args(p_state, "i&", &secs, &block);

    cTimer* p_timer = new cTimer(pActive_Level->m_mruby, secs, block);
    mrb_value instance = mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "Timer"), &rtTSC_Scriptable, p_timer));

    // Prevent mruby timer from getting out of scope
    mrb_ary_push(p_state, mrb_iv_get(p_state, self, mrb_intern_cstr(p_state, "instances")), instance);
    mrb_iv_set(p_state, instance, mrb_intern_cstr(p_state, "callback"), block);

    p_timer->Start();
    return instance;
}

/**
 * Method: Timer#start
 *
 *   start()
 *
 * Set the timer active.
 */
static mrb_value Start(mrb_state* p_state,  mrb_value self)
{
    cTimer* p_timer = Get_Data_Ptr<cTimer>(p_state, self);

    p_timer->Start();
    return mrb_nil_value();
}

/**
 * Method: Timer#stop
 *
 *   stop()
 *
 * Soft-stop the timer. Note this usually doesn’t mean the
 * timer is stopped immediately, but instead will wait until the
 * callback is executed once more (unless you call this in the very
 * nanosecond after the callback registration and before the checking
 * of the soft-stop condition).
 *
 * This method **blocks** until the timer has stopped. Think twice before
 * using this method, because this means the game’s main loop is completely
 * paused until this method returns. The user won’t be able to do any
 * interaction with the game in this time. The exact time this blocks is
 * determined by how far through the configured interval the timer has
 * already stepped; that is, in the worst case for a repeating one-hour
 * timer you have to wait one hour if `stop` is called immediately after
 * the callback execution.
 *
 * Raises a RuntimeError if you call this on a oneshot timer, where
 * it is useless.
 */
static mrb_value Stop(mrb_state* p_state,  mrb_value self)
{
    cTimer* p_timer = Get_Data_Ptr<cTimer>(p_state, self);

    // Does not make sense for oneshot timers -- they already do execute only once.
    if (!p_timer->Is_Periodic())
        mrb_raise(p_state, MRB_RUNTIME_ERROR(p_state), "Can't usefully soft-stop a oneshot timer!");

    p_timer->Stop();
    return mrb_nil_value();
}
/**
 * Method: Timer#stop!
 *
 *   stop!()
 *   interrupt()
 *
 * Forcibly interrupt the timer _now_. In contrast to #stop, this method
 * does not block; it tells the timer thread to terminate as soon as possible
 * and immediately returns. The timer thread will immediately terminate, unless
 * it is currently registering your callback into TSC’s main loop, in which case
 * it will terminate after that.
 */
static mrb_value Interrupt(mrb_state* p_state, mrb_value self)
{
    cTimer* p_timer = Get_Data_Ptr<cTimer>(p_state, self);

    p_timer->Interrupt();
    return mrb_nil_value();
}

/**
 * Method: Timer#inspect
 *
 *   inspect()
 *
 * Human-readable description.
 */
static mrb_value Inspect(mrb_state* p_state,  mrb_value self)
{
    cTimer* p_timer = Get_Data_Ptr<cTimer>(p_state, self);
    char buffer[256];

    int num = sprintf(buffer,
                      "#<%s interval=%dms (%s, %s)>",
                      mrb_obj_classname(p_state, self),
                      p_timer->Get_Interval(),
                      p_timer->Is_Periodic() ? "periodic" : "oneshot",
                      p_timer->Is_Active() ? "running" : "stopped");

    if (num < 0)
        mrb_raisef(p_state, MRB_RUNTIME_ERROR(p_state), "Couldn't format string, sprintf() returned %d", num);

    return mrb_str_new(p_state, buffer, num);
}

/**
 * Method: Timer#shall_halt?
 *
 *   shall_halt?()
 *
 * Returns `true` if [#stop](#stop) was called on a running
 * timer.
 */
static mrb_value Shall_Halt(mrb_state* p_state,  mrb_value self)
{
    cTimer* p_timer = Get_Data_Ptr<cTimer>(p_state, self);
    if (p_timer->Shall_Halt())
        return mrb_true_value();
    else
        return mrb_false_value();
}

/**
 * Method: Timer#active?
 *
 *   active?()
 *
 * Returns `true` if the timer is running, `false` otherwise.
 * An already fired one-shot timer is considered stopped for
 * this matter.
 *
 * Also note this method is subject to a little race condition,
 * because the timer ticks in a different C++ thread. It may be
 * the case that the thread exits the very moment after you called
 * this method, but before your next code instruction, so be careful.
 * Probably you should only use this method while debugging.
 */
static mrb_value Is_Active(mrb_state* p_state,  mrb_value self)
{
    cTimer* p_timer = Get_Data_Ptr<cTimer>(p_state, self);

    if (p_timer->Is_Active())
        return mrb_true_value();
    else
        return mrb_false_value();
}


/**
 * Method: Timer#interval
 *
 *   interval() → an_integer
 *
 * Returns the time interval for this timer, in milliseconds.
 */
static mrb_value Get_Interval(mrb_state* p_state,  mrb_value self)
{
    cTimer* p_timer = Get_Data_Ptr<cTimer>(p_state, self);
    return mrb_fixnum_value(p_timer->Get_Interval());
}


void TSC::Scripting::Init_Timer(mrb_state* p_state)
{
    struct RClass* p_rcTimer = mrb_define_class(p_state, "Timer", p_state->object_class);
    MRB_SET_INSTANCE_TT(p_rcTimer, MRB_TT_DATA);

    // Invisible (for MRuby) class instance variable for storing the
    // Timer instances so they don’t get GC’ed.
    mrb_iv_set(p_state, mrb_obj_value(p_rcTimer), mrb_intern_cstr(p_state, "instances"), mrb_ary_new(p_state));

    mrb_define_class_method(p_state, p_rcTimer, "after", After, MRB_ARGS_REQ(1) | MRB_ARGS_BLOCK());
    mrb_define_class_method(p_state, p_rcTimer, "every", Every, MRB_ARGS_REQ(1) | MRB_ARGS_BLOCK());
    mrb_define_method(p_state, p_rcTimer, "initialize", Initialize, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1) | MRB_ARGS_BLOCK());
    mrb_define_method(p_state, p_rcTimer, "start", Start, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcTimer, "stop", Stop, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcTimer, "stop!", Interrupt, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcTimer, "inspect", Inspect, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcTimer, "shall_halt?", Shall_Halt, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcTimer, "interval", Get_Interval, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcTimer, "active?", Is_Active, MRB_ARGS_NONE());

    mrb_define_alias(p_state, p_rcTimer, "interrupt", "stop!");
}

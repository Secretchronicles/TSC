/***************************************************************************
 * mrb_timer.hpp - Timer class for scripting API.
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

#ifndef TSC_SCRIPTING_TIMER_HPP
#define TSC_SCRIPTING_TIMER_HPP
#include "../../scripting.hpp"

namespace TSC {
    namespace Scripting {

        // C++ side of the MRuby Timer class.
        class cTimer {
        public:
            /* Constructor. Pass the MRuby interpreter state to register
             * the timer for, the time you want the timer
             * to fire (in milliseconds) and the callback to register for firing.
             * If `is_periodic' is true, the timer loops instead of
             * just waiting a single time. */
            cTimer(cMRuby_Interpreter* p_mruby, unsigned int interval, mrb_value callback, bool is_periodic = false);
            ~cTimer();

            // Start ticking, the timer does nothing until
            // you call this. You can start a timer again
            // after you called Stop() (this applies to
            // periodic timers as well). Does nothing if the
            // timer is already running.
            void Start();
            // Returns true if the timer shall stop
            // as soon as possible. This is a private API,
            // don't use this.
            bool Shall_Halt();
            // Stop the timer, without waiting for
            // it to execute the callback once more. This method
            // blocks until the timer's thread cheks the halting
            // condition (which it does several times a second,
            // so there should not be a noticable wait).
            void Stop();
            // Returns true if the timer is running currently.
            // This may still return true if a call to Stop()
            // has not yet been honoured.
            bool Is_Active();
            // Marks the timer as stopped. This is private API,
            // don’t use this.
            void Set_Stopped();
            // Pause this timer. It will not tick, but is not stopped
            // either. Calling Continue() will start ticking from the
            // point it was Pause()d. No-op if already paused.
            // Stopping a paused timer is possible, it is cancelled.
            // This is a private API. Do not use.
            void Pause();
            // Continue the timer after it was Pause()d. No-op if not
            // Paused. This is a private API. Do not use.
            void Continue();
            // Is the timer currently paused? This is a private API.
            // do not use.
            bool Is_Paused();

            // Attribute getters
            bool                Is_Periodic();
            unsigned int        Get_Interval();
            boost::thread*      Get_Thread();
            mrb_value           Get_Callback();
            cMRuby_Interpreter* Get_MRuby_Interpreter();
        private:
            // This is the body of the thread used by the
            // timer. Contains an endless loop for waiting
            // and registering. Note one cannot use non-static
            // members as the thread body, hence this static-with-`this'-pointer
            // construct.
            static void Threading_Function(cTimer* timer);

            // True if this is a repeating timer.
            bool            m_is_periodic;
            // Time interval.
            unsigned int    m_interval;
            // The callback to register.
            mrb_value       m_callback;
            // The thread. Only set after calling start().
            boost::thread*  mp_thread;
            // The MRuby instance we’re attaching the callbacks to.
            cMRuby_Interpreter* mp_mruby;
            // If set, stops the timer as soon as possible.
            bool m_halt;
            // If set, the auxiliary thread is not running.
            bool m_stopped;
            // If set the timer has started, but is not ticking.
            bool m_paused;
        };

        // Usual function for initialising the binding
        void Init_Timer(mrb_state* p_state);
    }
}

#endif

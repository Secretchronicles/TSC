/***************************************************************************
 * scripting.cpp - Glue for the scripting implementation.
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

#ifndef TSC_SCRIPTING_HPP
#define TSC_SCRIPTING_HPP
#include "../core/global_basic.hpp"
#include "../core/global_game.hpp"
#include "objects/mrb_tsc.hpp"

// Some defines to ease use of mruby
#define MRB_ARGUMENT_ERROR(mrb) (mrb_class_get(mrb, "ArgumentError"))
#define MRB_RUNTIME_ERROR(mrb) (mrb_class_get(mrb, "RuntimeError"))
#define MRB_TYPE_ERROR(mrb) (mrb_class_get(mrb, "TypeError"))
#define MRB_NOTIMP_ERROR(mrb) (mrb_class_get(mrb, "NotImplementedError"))
#define MRB_RANGE_ERROR(mrb) (mrb_class_get(mrb, "RangeError"))

namespace TSC {
    namespace Scripting {

        // We don’t use mruby’s C typechecks, but mruby wants
        // an mrb_data_type nevertheless from us. So we set
        // it for all our objects to this one.
        extern struct mrb_data_type rtTSC_Scriptable;

        // Takes a C(++) string and directly returns an MRuby
        // symbol object (not an mrb_sym!) for it.
        inline mrb_value str2sym(mrb_state* mrb, std::string str)
        {
            return mrb_symbol_value(mrb_intern_cstr(mrb, str.c_str()));
        }

        /**
         * Shorthand for doing
         *   DATA_GET_PTR(p_state, obj, &rtTSC_Scriptable)
         * over and over with a security NULL check.
         */
        template<typename T>
        T* Get_Data_Ptr(mrb_state* p_state, mrb_value obj)
        {
            T* p_result = static_cast<T*>(mrb_data_get_ptr(p_state, obj, &rtTSC_Scriptable));
            if (!p_result) {
                mrb_raise(p_state, MRB_TYPE_ERROR(p_state), "Unexpected NULL pointer. This is most likely an TSC bug.");
                return NULL; // Not reached
            }

            return p_result;
        }

        class cMRuby_Interpreter {
        public:
            // Create a new MRuby instance for the given level.
            cMRuby_Interpreter(cLevel* p_level);
            // Destructor
            ~cMRuby_Interpreter();

            // Execute MRuby code. If an exception occurs
            // (including syntax errors), false is returned,
            // true otherwise. `contextname' is purely informational
            // and only ever used in exception messages.
            // This method prints exceptions to standard error.
            bool Run_Code(const std::string& code, const std::string& contextname);
            // Execute MRuby code found in a file, using the filename
            // as the context name. Otherwise has the same
            // semantics as Run_Code().
            bool Run_File(const boost::filesystem::path& filepath);
            // Execute MRuby code in the given parsing context.
            // This method only does raw code execution, no
            // exception inspection is done for you. It’s basically
            // a wrapper around mrb_load_nstring_cxt().
            mrb_value Run_Code_In_Context(const std::string& code, mrbc_context* p_context);
            // Registers an MRuby callback to be called on the next
            // call to Evaluate_Timer_Callbacks(). `callback'
            // is an MRuby proc.
            // This method is threadsafe.
            void Register_Callback(mrb_value callback);
            // Runs all callbacks whose timers have fired.
            // This method is threadsafe.
            void Evaluate_Timer_Callbacks();
            // Returns the underlying mrb_state*.
            mrb_state* Get_MRuby_State();
            // Returns the cLevel* we’re associated with.
            cLevel* Get_Level();
            // Ensure an object doesn't get GC'ed.
            mrb_int Protect_From_GC(mrb_value obj);
            // Release the protection for an object created with Protect_From_GC().
            void Unprotect_From_GC(mrb_int index);

            // Retrieve an mruby class object.
            inline struct RClass* Get_MRuby_Class(const std::string& name)
            {
                return m_classes[name];
            }
            // Set an mruby class object. Only use inside
            // the Init_* functions that set up the mruby
            // class hierarchy.
            inline void Set_MRuby_Class(const std::string& name, struct RClass* klass)
            {
                m_classes[name] = klass;
            }
        private:
            mrb_state* mp_mruby;
            cLevel* mp_level;
            std::vector<mrb_value> m_callbacks;
            boost::mutex m_callback_mutex;
            std::map<std::string, struct RClass*> m_classes;

            // Load all MRuby wrapper classes for the C++ classes
            // into the given mruby state.
            void Load_Wrappers();
            // Executes the main.rb file for custom startup scripts.
            void Load_Scripts();
        };
    };
};

#endif

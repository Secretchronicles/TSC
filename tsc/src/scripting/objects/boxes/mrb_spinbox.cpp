/***************************************************************************
 * mrb_spinbox.cpp
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

#include "mrb_spinbox.hpp"
#include "../../../objects/spinbox.hpp"
#include "../../../level/level.hpp"
#include "../../../core/sprite_manager.hpp"
#include "mrb_box.hpp"

/**
 * Class: SpinBox
 *
 * Parent: L<Box>
 *
 * The I<Spin Box> is an empty box that gets passable
 * for a short time if jumped against it from below
 * (or if a shell is thrown against it from the side).
 */

using namespace TSC;
using namespace TSC::Scripting;


/**
 * Method: SpinBox::new
 *
 *   new() → a_spin_box
 *
 * Creates a new SpinBox with the default values.
 */
static mrb_value Initialize(mrb_state* p_state, mrb_value self)
{
    cSpinBox* p_box = new cSpinBox(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_box;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    // This is a generated object
    p_box->Set_Spawned(true);

    // Let TSC manage the memory
    pActive_Level->m_sprite_manager->Add(p_box);

    return self;
}

/**
 * Method: SpinBox#stop
 *
 *   stop()
 *
 * Stop the spinning started by Box#activate.
 */
static mrb_value Stop(mrb_state* p_state, mrb_value self)
{
    cSpinBox* p_box = Get_Data_Ptr<cSpinBox>(p_state, self);
    p_box->Stop();
    return mrb_nil_value();
}

/**
 * Method: SpinBox#spinning?
 *
 *   spinning?() → true or false
 *
 * Checks whether the box is spinning currently, and if so,
 * returns C<true>, otherwise returns C<false>.
 */
static mrb_value Is_Spinning(mrb_state* p_state, mrb_value self)
{
    cSpinBox* p_box = Get_Data_Ptr<cSpinBox>(p_state, self);
    return mrb_bool_value(p_box->m_spin);
}

void TSC::Scripting::Init_SpinBox(mrb_state* p_state)
{
    struct RClass* p_rcSpin_Box = mrb_define_class(p_state, "SpinBox", mrb_class_get(p_state, "Box"));
    MRB_SET_INSTANCE_TT(p_rcSpin_Box, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcSpin_Box, "initialize", Initialize, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcSpin_Box, "stop", Stop, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcSpin_Box, "spinning?", Is_Spinning, MRB_ARGS_NONE());
}

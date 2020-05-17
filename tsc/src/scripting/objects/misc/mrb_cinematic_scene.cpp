/***************************************************************************
 * mrb_cinematic_scene.cpp
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

#include "mrb_cinematic_scene.hpp"

using namespace TSC;
using namespace TSC::Scripting;

static mrb_value Initialize(mrb_state* p_state, mrb_value self)
{
    mrb_value block;
    mrb_get_args(p_state, "&", &block);

    // Store block for later retrieval
    mrb_iv_set(p_state, self, mrb_intern_cstr(p_state, "@block"), block);

    // DATA_PTR(self) = Something c++ish
    DATA_TYPE(self) = &rtTSC_Scriptable;

    return self;
}

// The following does not work for some reason (segfault in
// mrb_funcall_with_block()). Instead some raw Ruby is evaluated
// in the Init() function below.
//static mrb_value Play(mrb_state* p_state, mrb_value self)
//{
//    mrb_value block = mrb_iv_get(p_state, self, mrb_intern_cstr(p_state, "@block"));
//    mrb_funcall_with_block(p_state, self, mrb_intern_lit(p_state, "instance_eval"), 0, NULL, block);
//    return mrb_nil_value();
//}

void TSC::Scripting::Init_Cinematic_Scene(mrb_state* p_state)
{
    struct RClass* p_rcCinematicScene = mrb_define_class(p_state, "CinematicScene", p_state->object_class);
    MRB_SET_INSTANCE_TT(p_rcCinematicScene, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcCinematicScene, "initialize", Initialize, MRB_ARGS_NONE()|MRB_ARGS_BLOCK());
    //mrb_define_method(p_state, p_rcCinematicScene, "play", Play, MRB_ARGS_NONE());

    // Since I didn't get it working with mrb_funcall_with_block() in Play() above,
    // just do it directly in Ruby.
    mrb_load_string(p_state, "class CinematicScene; def play; instance_eval(&@block); nil; end; end");
}

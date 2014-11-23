/***************************************************************************
 * mrb_thromp.cpp
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

#include "../../../enemies/thromp.hpp"
#include "../../../level/level.hpp"
#include "../../../core/sprite_manager.hpp"
#include "../../../core/property_helper.hpp"
#include "mrb_enemy.hpp"
#include "mrb_thromp.hpp"

/**
 * Class: Thromp
 *
 * Parent: [Enemy](enemy.html)
 * {: .superclass}
 *
 * The _Thromp_ falls down on you and squashes you. Occasionally, it
 * doesn’t come from atop, but rather from the sides or even from
 * the bottom. The thromp itself is immune to gravity.
 */

using namespace TSC;
using namespace TSC::Scripting;


/**
 * Method: Thromp::new
 *
 *   new() → a_thromp
 *
 * Creates a new thromp with the default values.
 */
static mrb_value Initialize(mrb_state* p_state,  mrb_value self)
{
    cThromp* p_thromp = new cThromp(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_thromp;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    // This is a generated object
    p_thromp->Set_Spawned(true);

    // Let TSC manage the memory
    pActive_Level->m_sprite_manager->Add(p_thromp);

    return self;
}

/**
 * Method: Thromp#image_dir=
 *
 *   image_dir=( path ) → path
 *
 * Specify the image directory for the thromp.
 *
 * #### Parameters
 * path
 * : The path to the thromp’s new image directory, relative
 *   to the `pixmaps` directory.
 */
static mrb_value Set_Image_Dir(mrb_state* p_state, mrb_value self)
{
    char* cdir = NULL;
    mrb_get_args(p_state, "z", &cdir);

    cThromp* p_thromp = Get_Data_Ptr<cThromp>(p_state, self);
    p_thromp->Set_Image_Dir(utf8_to_path(cdir));

    return mrb_str_new_cstr(p_state, cdir);
}

/**
 * Method: Thromp#image_dir
 *
 *   image_dir() → a_string
 *
 * Returns the thromp’s current image directory, relative to
 * the `pixmaps` directory.
 */
static mrb_value Get_Image_Dir(mrb_state* p_state,  mrb_value self)
{
    cThromp* p_thromp = Get_Data_Ptr<cThromp>(p_state, self);
    return mrb_str_new_cstr(p_state, path_to_utf8(p_thromp->m_img_dir).c_str());
}

/**
 * Method: Thromp#max_distance=
 *
 *   max_distance=( distance ) → distance
 *
 * Specify the maximum distance the thromp may fall towarads Maryo.
 *
 * #### Parameters
 * distance
 * : The maximum distance in pixels. A float.
 */
static mrb_value Set_Max_Distance(mrb_state* p_state, mrb_value self)
{
    mrb_float distance;
    mrb_get_args(p_state, "f", &distance);

    if (distance < 0) {
        mrb_raise(p_state, MRB_RANGE_ERROR(p_state), "Thromp maximum distance must be >= 0");
        return mrb_nil_value(); // Not reached
    }

    cThromp* p_thromp = Get_Data_Ptr<cThromp>(p_state, self);
    p_thromp->Set_Max_Distance(distance);

    return mrb_float_value(p_state, distance);
}

/**
 * Method: Thromp#max_distance
 *
 *   max_distance() → a_float
 *
 * Returns the current maximum distance the thromp may fall towards
 * Maryo, in pixels.
 */
static mrb_value Get_Max_Distance(mrb_state* p_state, mrb_value self)
{
    cThromp* p_thromp = Get_Data_Ptr<cThromp>(p_state, self);
    return mrb_float_value(p_state, p_thromp->m_max_distance);
}

/**
 * Method: Thromp#speed=
 *
 *   speed=( val ) → val
 *
 * Specify the thromp’s new speed.
 *
 * #### Parameters
 * val
 * : The thromp’s new speed. A float.
 */
static mrb_value Set_Speed(mrb_state* p_state, mrb_value self)
{
    mrb_float speed;
    mrb_get_args(p_state, "f", &speed);

    if (speed < 0) {
        mrb_raise(p_state, MRB_RANGE_ERROR(p_state), "Thromp fall speed must be >= 0");
        return mrb_nil_value(); // Not reached
    }

    cThromp* p_thromp = Get_Data_Ptr<cThromp>(p_state, self);
    p_thromp->Set_Speed(speed);

    return mrb_float_value(p_state, speed);
}

/**
 * Method: Thromp#speed
 *
 *   speed() → a_float
 *
 * Returns the thromp’s speed.
 */
static mrb_value Get_Speed(mrb_state* p_state, mrb_value self)
{
    cThromp* p_thromp = Get_Data_Ptr<cThromp>(p_state, self);
    return mrb_float_value(p_state, p_thromp->m_speed);
}

/**
 * Method: Thromp#moving_backwards?
 *
 *   moving_backwards?() → true or false
 *
 * Returns `true` if the thromp is moving backwards to its original
 * position, `false` otherwise.
 */
static mrb_value Is_Moving_Backwards(mrb_state* p_state, mrb_value self)
{
    cThromp* p_thromp = Get_Data_Ptr<cThromp>(p_state, self);
    return mrb_bool_value(p_thromp->m_move_back);
}

void TSC::Scripting::Init_Thromp(mrb_state* p_state)
{
    struct RClass* p_rcThromp = mrb_define_class(p_state, "Thromp", mrb_class_get(p_state, "Enemy"));
    MRB_SET_INSTANCE_TT(p_rcThromp, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcThromp, "initialize", Initialize, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcThromp, "image_dir", Get_Image_Dir, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcThromp, "image_dir=", Set_Image_Dir, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcThromp, "max_distance", Get_Max_Distance, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcThromp, "max_distance=", Set_Max_Distance, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcThromp, "speed", Get_Speed, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcThromp, "speed=", Set_Speed, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcThromp, "moving_backwards?", Is_Moving_Backwards, MRB_ARGS_NONE());
}

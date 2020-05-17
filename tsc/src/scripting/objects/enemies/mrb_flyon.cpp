/***************************************************************************
 * mrb_flyon.cpp
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

#include "../../../enemies/flyon.hpp"
#include "../../../level/level.hpp"
#include "../../../core/sprite_manager.hpp"
#include "../../../core/property_helper.hpp"
#include "mrb_enemy.hpp"
#include "mrb_flyon.hpp"

/**
 * Class: Flyon
 *
 * Parent: L<Enemy>
 *
 * The I<Flyon> jumps out of its hiding and attacks you. It then quickly
 * returns into its typically unreachable hiding and waits a number of
 * seconds before it tries again.
 *
 * Note the flyon is one of the rare objects that makes use of its
 * I<starting position> coordinates. They define the actual position
 * the flyon is returning to after a jump; the normal coordinates
 * merely determine where it is I<currently>.
 */

using namespace TSC;
using namespace TSC::Scripting;


/**
 * Method: Flyon::new()
 *
 *   new() → a_flyon
 *
 * Creates a new Fylon object with the default values.
 */
static mrb_value Initialize(mrb_state* p_state, mrb_value self)
{

    cFlyon* p_flyon = new cFlyon(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_flyon;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    // This is a generated object
    p_flyon->Set_Spawned(true);

    // Let TSC manager the memory
    pActive_Level->m_sprite_manager->Add(p_flyon);

    return self;
}

/**
 * Method: Flyon#image_dir=
 *
 *   image_dir=( path ) → path
 *
 * Set the directory used to retrieve the pictures for the flyon.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [path]
 *
 * Path for the flyon image directory, relative to F<pixmaps> directory.
 * Note the only real alternative to the default (F<pixmaps/flyon/orange>)
 * is F<pixmaps/flyon/blue>.
 */
static mrb_value Set_Image_Dir(mrb_state* p_state, mrb_value self)
{
    char* cdir = NULL;
    mrb_get_args(p_state, "z", &cdir);

    cFlyon* p_flyon = Get_Data_Ptr<cFlyon>(p_state, self);
    p_flyon->Set_Image_Dir(utf8_to_path(cdir));

    return mrb_str_new_cstr(p_state, cdir);
}

/**
 * Method: Flyon#image_dir
 *
 *   image_dir() → a_string
 *
 * Retrieve the path to the directory where the pictures for the
 * flyon are taken from, relative to the F<pixmaps> directory.
 */
static mrb_value Get_Image_Dir(mrb_state* p_state, mrb_value self)
{
    cFlyon* p_flyon = Get_Data_Ptr<cFlyon>(p_state, self);
    return mrb_str_new_cstr(p_state, path_to_utf8(p_flyon->m_img_dir).c_str());
}

/**
 * Method: Flyon#max_distance=
 *
 *   max_distance=( distance ) → distance
 *
 * Specify the maximum distance the flyon may travel.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [distance]
 *
 * The new maximum distance the flyon may travel, in pixels. A float.
 *
 * =back
 */
static mrb_value Set_Max_Distance(mrb_state* p_state, mrb_value self)
{
    mrb_float maxdistance;
    mrb_get_args(p_state, "f", &maxdistance);

    cFlyon* p_flyon = Get_Data_Ptr<cFlyon>(p_state, self);
    p_flyon->Set_Max_Distance(maxdistance);

    return mrb_float_value(p_state, maxdistance);
}

/**
 * Method: Flyon#max_distance
 *
 *   max_distance() → a_float
 *
 * Retrieve the maximum number of pixels the flyon may travel.
 */
static mrb_value Get_Max_Distance(mrb_state* p_state, mrb_value self)
{
    cFlyon* p_flyon = Get_Data_Ptr<cFlyon>(p_state, self);
    return mrb_float_value(p_state, p_flyon->m_max_distance);
}

/**
 * Method: Flyon#speed=
 *
 *   speed=( speed ) → speed
 *
 * TODO: Docs.
 */
static mrb_value Set_Speed(mrb_state* p_state, mrb_value self)
{
    mrb_float speed;
    mrb_get_args(p_state, "f", &speed);

    cFlyon* p_flyon = Get_Data_Ptr<cFlyon>(p_state, self);
    p_flyon->Set_Speed(speed);

    return mrb_float_value(p_state, speed);
}

/**
 * Method: Flyon#speed
 *
 *   speed() → a_float
 *
 * TODO: Docs.
 */
static mrb_value Get_Speed(mrb_state* p_state, mrb_value self)
{
    cFlyon* p_flyon = Get_Data_Ptr<cFlyon>(p_state, self);
    return mrb_float_value(p_state, p_flyon->m_speed);
}

/**
 * Method: Flyon#wait_time=
 *
 *   wait_time=( secs ) → secs
 *
 * Specify the time to wait between jumping.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [secs]
 *
 * Number of seconds to wait between jumping out again.
 * A float.
 *
 * =back
 */
static mrb_value Set_Wait_Time(mrb_state* p_state, mrb_value self)
{
    mrb_float time;
    mrb_get_args(p_state, "f", &time);

    if (time < 0) {
        mrb_raise(p_state, MRB_RANGE_ERROR(p_state), "Wait time must be > 0.");
        return mrb_nil_value(); // Not reached
    }

    cFlyon* p_flyon = Get_Data_Ptr<cFlyon>(p_state, self);
    p_flyon->m_wait_time = time;

    return mrb_float_value(p_state, time);
}

/**
 * Method: Flyon#wait_time
 *
 *   wait_time() → a_float
 *
 * Returns the number of seconds to wait betwee jumping, as a
 * float.
 */
static mrb_value Get_Wait_Time(mrb_state* p_state, mrb_value self)
{
    cFlyon* p_flyon = Get_Data_Ptr<cFlyon>(p_state, self);
    return mrb_float_value(p_state, p_flyon->m_wait_time);
}

/**
 * Method: Flyon#distance_to_end
 *
 *   distance_to_end() → a_float
 *
 * Current distance to the end position, in pixels.
 */
static mrb_value Get_End_Distance(mrb_state* p_state, mrb_value self)
{
    cFlyon* p_flyon = Get_Data_Ptr<cFlyon>(p_state, self);
    return mrb_float_value(p_state, p_flyon->Get_End_Distance());
}

/**
 * Method: Flyon#moving_backwards?
 *
 *   moving_backwards? → true or false
 *
 * Whether or not the flyon is moving backwards to its original position.
 */
static mrb_value Is_Moving_Backwards(mrb_state* p_state, mrb_value self)
{
    cFlyon* p_flyon = Get_Data_Ptr<cFlyon>(p_state, self);
    return mrb_bool_value(p_flyon->m_move_back);
}

void TSC::Scripting::Init_Flyon(mrb_state* p_state)
{
    struct RClass* p_rcFlyon = mrb_define_class(p_state, "Flyon", mrb_class_get(p_state, "Enemy"));
    MRB_SET_INSTANCE_TT(p_rcFlyon, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcFlyon, "initialize", Initialize, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcFlyon, "image_dir=", Set_Image_Dir, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcFlyon, "image_dir", Get_Image_Dir, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcFlyon, "max_distance=", Set_Max_Distance, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcFlyon, "max_distance", Get_Max_Distance, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcFlyon, "speed=", Set_Speed, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcFlyon, "speed", Get_Speed, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcFlyon, "wait_time=", Set_Wait_Time, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcFlyon, "wait_time", Get_Wait_Time, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcFlyon, "distance_to_end", Get_End_Distance, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcFlyon, "moving_backwards?", Is_Moving_Backwards, MRB_ARGS_NONE());
}

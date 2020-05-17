/***************************************************************************
 * mrb_moving_platform.cpp
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

#include "../../../objects/moving_platform.hpp"
#include "mrb_moving_platform.hpp"
#include "../../../level/level.hpp"
#include "../../../core/sprite_manager.hpp"
#include "../../../core/global_basic.hpp"

/**
 * Class: MovingPlatform
 *
 * Parent: L<MovingSprite>
 *
 * TODO: Docs.
 */

using namespace TSC;
using namespace TSC::Scripting;
using namespace std;

/**
 * Method: MovingPlatform::new
 *
 *   new() → a_moving_platform
 *
 * Creates a new instance of this class.
 */
static mrb_value Initialize(mrb_state* p_state, mrb_value self)
{
    cMoving_Platform* p_plat = new cMoving_Platform(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_plat;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    p_plat->Set_Spawned(true);
    pActive_Level->m_sprite_manager->Add(p_plat);

    return self;
}

/**
 * Method: MovingPlatform#move_type
 *
 *   move_type() → a_symbol
 *
 * Returns the platforms current move type. One of the following symbols:
 * C<:line>, C<:circle>, C<:path>, C<:path_backwards>.
 */
static mrb_value Get_Move_Type(mrb_state* p_state, mrb_value self)
{
    cMoving_Platform* p_plat = Get_Data_Ptr<cMoving_Platform>(p_state, self);

    switch (p_plat->m_move_type) {
    case MOVING_PLATFORM_TYPE_LINE:
        return str2sym(p_state, "line");
    case MOVING_PLATFORM_TYPE_CIRCLE:
        return str2sym(p_state, "circle");
    case MOVING_PLATFORM_TYPE_PATH:
        return str2sym(p_state, "path");
    case MOVING_PLATFORM_TYPE_PATH_BACKWARDS:
        return str2sym(p_state, "path_backwards");
    default:
        cerr << "Warning: Unknown moving platform type '" << p_plat->m_move_type << "'." << endl;
        return mrb_nil_value();
    }
}

/**
 * Method: MovingPlatform#move_type=
 *
 *   move_type=( symbol ) → symbol
 *
 * TODO: Docs.
 */
static mrb_value Set_Move_Type(mrb_state* p_state, mrb_value self)
{
    mrb_sym typesym;
    mrb_get_args(p_state, "n", &typesym);
    std::string typestr(mrb_sym2name(p_state, typesym));

    Moving_Platform_Type type;
    if (typestr == "line")
        type = MOVING_PLATFORM_TYPE_LINE;
    else if (typestr == "circle")
        type = MOVING_PLATFORM_TYPE_CIRCLE;
    else if (typestr == "path")
        type = MOVING_PLATFORM_TYPE_PATH;
    else if (typestr == "path_backwards")
        type = MOVING_PLATFORM_TYPE_PATH_BACKWARDS;
    else {
        mrb_raisef(p_state, MRB_ARGUMENT_ERROR(p_state), "Invalid platform moving type %s", typestr.c_str());
        return mrb_nil_value(); // Not reached
    }

    cMoving_Platform* p_plat = Get_Data_Ptr<cMoving_Platform>(p_state, self);
    p_plat->Set_Move_Type(type);

    return mrb_symbol_value(typesym);
}

/**
 * Method: MovingPlatform#path_identifier=
 *
 *   path_identifier=( str ) → str
 *
 * TODO: Docs.
 */
static mrb_value Set_Path_Identifier(mrb_state* p_state, mrb_value self)
{
    char* str = NULL;
    mrb_get_args(p_state, "z", &str);

    cMoving_Platform* p_plat = Get_Data_Ptr<cMoving_Platform>(p_state, self);
    p_plat->Set_Path_Identifier(str);

    return mrb_str_new_cstr(p_state, str);
}

/**
 * Method: MovingPlatform#path_identifier
 *
 *   path_identifier() → str
 *
 * TODO: Docs.
 */
static mrb_value Get_Path_Identifier(mrb_state* p_state, mrb_value self)
{
    cMoving_Platform* p_plat = Get_Data_Ptr<cMoving_Platform>(p_state, self);
    return mrb_str_new_cstr(p_state, p_plat->m_path_state.m_path_identifier.c_str());
}

/**
 * Method: MovingPlatform#speed=
 *
 *   speed=( float ) → float
 *
 * TODO: Docs.
 */
static mrb_value Set_Speed(mrb_state* p_state, mrb_value self)
{
    mrb_float speed;
    mrb_get_args(p_state, "f", &speed);

    cMoving_Platform* p_plat = Get_Data_Ptr<cMoving_Platform>(p_state, self);
    p_plat->Set_Speed(speed);

    return mrb_float_value(p_state, speed);
}

/**
 * Method: MovingPlatform#speed
 *
 *   speed() → a_float
 *
 * TODO: Docs.
 */
static mrb_value Get_Speed(mrb_state* p_state, mrb_value self)
{
    cMoving_Platform* p_plat = Get_Data_Ptr<cMoving_Platform>(p_state, self);
    return mrb_float_value(p_state, p_plat->m_speed);
}

/**
 * Method: MovingPlatform#middle_count=
 *
 *   middle_count=( int ) → int
 *
 * TODO: Docs.
 */
static mrb_value Set_Middle_Count(mrb_state* p_state, mrb_value self)
{
    mrb_int count;
    mrb_get_args(p_state, "i", &count);

    cMoving_Platform* p_plat = Get_Data_Ptr<cMoving_Platform>(p_state, self);
    p_plat->Set_Middle_Count(count);

    return mrb_fixnum_value(count);
}

/**
 * Method: MovingPlatform#middle_count
 *
 *   middle_count() → int
 *
 * TODO: Docs.
 */
static mrb_value Get_Middle_Count(mrb_state* p_state, mrb_value self)
{
    cMoving_Platform* p_plat = Get_Data_Ptr<cMoving_Platform>(p_state, self);
    return mrb_fixnum_value(p_plat->m_middle_count);
}

void TSC::Scripting::Init_Moving_Platform(mrb_state* p_state)
{
    struct RClass* p_rcMovingPlatform = mrb_define_class(p_state, "MovingPlatform", mrb_class_get(p_state, "MovingSprite"));
    MRB_SET_INSTANCE_TT(p_rcMovingPlatform, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcMovingPlatform, "initialize", Initialize, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcMovingPlatform, "move_type", Get_Move_Type, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcMovingPlatform, "move_type=", Set_Move_Type, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcMovingPlatform, "path_identifier", Get_Path_Identifier, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcMovingPlatform, "path_identifier=", Set_Path_Identifier, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcMovingPlatform, "speed", Get_Speed, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcMovingPlatform, "speed=", Set_Speed, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcMovingPlatform, "middle_count", Get_Middle_Count, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcMovingPlatform, "middle_count=", Set_Middle_Count, MRB_ARGS_REQ(1));
}

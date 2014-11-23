/***************************************************************************
 * mrb_path.cpp
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

#include "mrb_path.hpp"
#include "../sprites/mrb_sprite.hpp"
#include "../../../objects/sprite.hpp"
#include "../../../objects/path.hpp"
#include "../../../core/sprite_manager.hpp"
#include "../../../level/level.hpp"

/**
 * Class: Path
 * Parent: [Sprite](sprite.html)
 *
 * A _path_ is a usually invisible line objects in the level
 * (e.g. platforms) follow. While the movement could usually
 * be achieved with pure scripting using timers, paths are
 * implemented purely in C++ and as such perform better. On
 * the other hands, paths are limited to a defined number of
 * movements, while scripting your movement allows all kinds of
 * crazy things to be done.
 */

using namespace TSC;
using namespace TSC::Scripting;

// forward declare
static void PS_Free(mrb_state* p_state, void* ptr);

struct mrb_data_type TSC::Scripting::rtTSC_Path_Segment = {"TscPathSegment", PS_Free};

/***************************************
 * Class Path
 ***************************************/

/**
 * Method: Path::new
 *
 *   new() → a_path
 *
 * Creates a new path with the default values.
 */
static mrb_value Initialize(mrb_state* p_state, mrb_value self)
{
    cPath* p_path = new cPath(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_path;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    // This is a generated object
    p_path->Set_Spawned(true);

    // Let TSC manage the memory
    pActive_Level->m_sprite_manager->Add(p_path);

    return self;
}

/**
 * Method: Path#identifier=
 *
 *   identifier=( str ) → str
 *
 * Specify this path’s identifier.
 *
 * #### Parameters
 * str
 * : The path’s new identifier.
 */
static mrb_value Set_Identifier(mrb_state* p_state, mrb_value self)
{
    char* ident = NULL;
    mrb_get_args(p_state, "z", &ident);

    cPath* p_path = Get_Data_Ptr<cPath>(p_state, self);
    p_path->Set_Identifier(ident);

    return mrb_str_new_cstr(p_state, ident);
}

/**
 * Method: Path#identifier
 *
 *   identifier() → a_string or nil
 *
 * Returns the path’s current identifier as a string. If no
 * identifier has been set, returns `nil`.
 */
static mrb_value Get_Identifier(mrb_state* p_state, mrb_value self)
{
    cPath* p_path = Get_Data_Ptr<cPath>(p_state, self);

    if (p_path->m_identifier.empty())
        return mrb_nil_value();
    else
        return mrb_str_new_cstr(p_state, p_path->m_identifier.c_str());
}

/**
 * Method: Path#show_line=
 *
 *   show_line=( bool ) → bool
 *
 * Specify whether or not the path should have its moving line
 * drawn (not to draw is the default). This feature is EXPERIMENTAL
 * and usually does not work as expected.
 *
 * #### Parameters
 * bool
 * : Whether or not to draw the line.
 */
static mrb_value Set_Show_Line(mrb_state* p_state, mrb_value self)
{
    mrb_bool show;
    mrb_get_args(p_state, "b", &show);

    cPath* p_path = Get_Data_Ptr<cPath>(p_state, self);
    p_path->Set_Show_Line(show);

    return mrb_bool_value(show);
}

/**
 * Method: Path#show_line?
 *
 *   show_line?() → true or false
 *
 * Returns `true` or `false` depending on whether the path’s
 * movement line should be drawn.
 */
static mrb_value Does_Show_Line(mrb_state* p_state, mrb_value self)
{
    cPath* p_path = Get_Data_Ptr<cPath>(p_state, self);
    return mrb_bool_value(p_path->m_show_line);
}

/**
 * Method: Path#rewind=
 *
 *   rewind=( bool ) → bool
 *
 * Specify whether or not to rewind when the object moving along
 * the path reached the paths end. The default is not to rewind,
 * i.e. to mirror and move the path backwards.
 *
 * #### Parameters
 * bool
 * : Whether or not to rewind.
 */
static mrb_value Set_Rewind(mrb_state* p_state, mrb_value self)
{
    mrb_bool rewind;
    mrb_get_args(p_state, "b", &rewind);

    cPath* p_path = Get_Data_Ptr<cPath>(p_state, self);
    p_path->Set_Rewind(rewind);

    return mrb_bool_value(rewind);
}

/**
 * Method: Path#rewind?
 *
 *   rewind?() → true or false
 *
 * Returns `true` or `false` depending on whether this path
 * mirrors (`false`) or rewinds (`true`).
 */
static mrb_value Does_Rewind(mrb_state* p_state, mrb_value self)
{
    cPath* p_path = Get_Data_Ptr<cPath>(p_state, self);
    return mrb_bool_value(p_path->m_rewind);
}

/**
 * Method: Path#add_segment
 *
 *   add_segment( segment )
 *
 * Adds the given segment to the end of the path.
 *
 * #### Parameters
 * segment
 * : The [Path::Segment](path_segment.html) instance to add to the path.
 */
static mrb_value Add_Segment(mrb_state* p_state, mrb_value self)
{
    mrb_value segment;
    mrb_get_args(p_state, "o", &segment);

    if (!mrb_obj_is_kind_of(p_state, segment, mrb_class_get_under(p_state, mrb_class_get(p_state, "Path"), "Segment"))) {
        mrb_raise(p_state, MRB_TYPE_ERROR(p_state), "This is not a Path::Segment.");
        return mrb_nil_value(); // Not reached
    }

    cPath*         p_path    = Get_Data_Ptr<cPath>(p_state, self);
    cPath_Segment* p_segment = Get_Data_Ptr<cPath_Segment>(p_state, segment);

    p_path->Add_Segment(*p_segment);

    return mrb_nil_value();
}

/**
 * Method: Path#each_segment
 *
 *   each_segment(){|segment| ...}
 *
 * Iterates over all segments in the path and yields them
 * into the given block one-by-one.
 *
 * #### Parameters
 * segment (**bock**)
 * : The currently iterated path segment. A [Path::Segment](path_segment.html)
 *   instance.
 */
static mrb_value Each_Segment(mrb_state* p_state, mrb_value self)
{
    mrb_value block;
    mrb_get_args(p_state, "&", &block);

    cPath* p_path = Get_Data_Ptr<cPath>(p_state, self);

    cPath::PathList::const_iterator iter;
    struct RClass* p_rcPath_Segment = mrb_class_get_under(p_state, mrb_class_get(p_state, "Path"), "Segment");
    for (iter = p_path->m_segments.begin(); iter != p_path->m_segments.end(); iter++) {
        cPath_Segment segment = *iter;
        cPath_Segment* p_segment = new cPath_Segment;

        // We need to copy the cPath_Segment instance to a persistant pointer,
        // because `segment' goes out of scope at the end of the for loop!
        // The mruby Path::Segment class properly `delete's that pointer.
        p_segment->Set_Pos(segment.m_x1, segment.m_y1, segment.m_x2, segment.m_y2);
        mrb_value rsegment = mrb_obj_value(Data_Wrap_Struct(p_state, p_rcPath_Segment, &rtTSC_Path_Segment, p_segment));

        mrb_yield(p_state, block, rsegment);
    }

    return mrb_nil_value();
}

/**
 * Method: Path#segments
 *
 *   segments() → an_array
 *
 * Returns all segments of this path as an array.
 *
 * #### Return value
 * An array of [Path::Segment](path_segment.html) instances.
 */
static mrb_value Segments(mrb_state* p_state, mrb_value self)
{
    mrb_value ary;

    cPath* p_path = Get_Data_Ptr<cPath>(p_state, self);

    cPath::PathList::const_iterator iter;
    struct RClass* p_rcPath_Segment = mrb_class_get_under(p_state, mrb_class_get(p_state, "Path"), "Segment");
    for (iter = p_path->m_segments.begin(); iter != p_path->m_segments.end(); iter++) {
        cPath_Segment segment = *iter;
        cPath_Segment* p_segment = new cPath_Segment;

        // We need to copy the cPath_Segment instance to a persistant pointer,
        // because `segment' goes out of scope at the end of the for loop!
        // The mruby Path::Segment class properly `delete's that pointer.
        p_segment->Set_Pos(segment.m_x1, segment.m_y1, segment.m_x2, segment.m_y2);
        mrb_value rsegment = mrb_obj_value(Data_Wrap_Struct(p_state, p_rcPath_Segment, &rtTSC_Path_Segment, p_segment));

        mrb_ary_push(p_state, ary, rsegment);
    }

    return ary;
}

/***************************************
 * Class Path::Segment
 ***************************************/

/**
 * Class: Path::Segment
 *
 * Instances of this class represent a single segment of a path.
 * It’s a purely informational object that serves no purpose
 * beside containing the two endpoints of a path segment.
 */

/**
 * Method: Path::Segment::new
 *
 *   new( startx, starty, targetx, targety ) → a_path_segment
 *
 * Creates a new path segment describing the given positions and
 * their connection.
 *
 * #### Parameters
 * All parameters are float values.
 *
 * startx
 * : The start X coordinate.
 *
 * starty
 * : The start Y coordinate.
 *
 * targetx
 * : The target X coordinate.
 *
 * targety
 * : The target Y coordinate.
 */
static mrb_value PS_Initialize(mrb_state* p_state, mrb_value self)
{
    float startx, starty, targetx, targety;
    mrb_get_args(p_state, "ffff", &startx, &starty, &targetx, &targety);

    cPath_Segment* p_segment = new cPath_Segment();
    DATA_PTR(self) = p_segment;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    p_segment->Set_Pos(startx, starty, targetx, targety);

    return self;
}

// GC callback
static void PS_Free(mrb_state* p_state, void* ptr)
{
    cPath_Segment* p_segment = (cPath_Segment*) ptr;

    delete p_segment;
}

/**
 * Method: Path::Segment#start_x
 *
 *   start_x() → a_float
 *
 * Returns the start X coordinate.
 */
static mrb_value PS_Get_Start_X(mrb_state* p_state, mrb_value self)
{
    cPath_Segment* p_segment = static_cast<cPath_Segment*>(mrb_data_get_ptr(p_state, self, &rtTSC_Path_Segment));

    return mrb_float_value(p_state, p_segment->m_x1);
}

/**
 * Method: Path::Segment#start_y
 *
 *   start_y() → a_float
 *
 * Returns the start Y coordinate.
 */
static mrb_value PS_Get_Start_Y(mrb_state* p_state, mrb_value self)
{
    cPath_Segment* p_segment = static_cast<cPath_Segment*>(mrb_data_get_ptr(p_state, self, &rtTSC_Path_Segment));

    return mrb_float_value(p_state, p_segment->m_y1);
}

/**
 * Method: Path::Segment#target_x
 *
 *   target_x() → a_float
 *
 * Returns the target X coordinate.
 */
static mrb_value PS_Get_Target_X(mrb_state* p_state, mrb_value self)
{
    cPath_Segment* p_segment = static_cast<cPath_Segment*>(mrb_data_get_ptr(p_state, self, &rtTSC_Path_Segment));

    return mrb_float_value(p_state, p_segment->m_x2);
}

/**
 * Method: Path::Segment#target_y
 *
 *   target_y() → a_float
 *
 * Returns the target Y coordinate.
 */
static mrb_value PS_Get_Target_Y(mrb_state* p_state, mrb_value self)
{
    cPath_Segment* p_segment = static_cast<cPath_Segment*>(mrb_data_get_ptr(p_state, self, &rtTSC_Path_Segment));

    return mrb_float_value(p_state, p_segment->m_y2);
}

/***************************************
 * Binding
 ***************************************/

void TSC::Scripting::Init_Path(mrb_state* p_state)
{
    struct RClass* p_rcPath = mrb_define_class(p_state, "Path", mrb_class_get(p_state, "Sprite"));
    struct RClass* p_rcPath_Segment = mrb_define_class_under(p_state, p_rcPath, "Segment", p_state->object_class);
    MRB_SET_INSTANCE_TT(p_rcPath, MRB_TT_DATA);
    MRB_SET_INSTANCE_TT(p_rcPath_Segment, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcPath, "initialize", Initialize, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcPath, "identifier=", Set_Identifier, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcPath, "identifier", Get_Identifier, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcPath, "show_line=", Set_Show_Line, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcPath, "show_line?", Does_Show_Line, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcPath, "rewind=", Set_Rewind, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcPath, "rewind?", Does_Rewind, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcPath, "add_segment", Add_Segment, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcPath, "each_segment", Each_Segment, MRB_ARGS_BLOCK());
    mrb_define_method(p_state, p_rcPath, "segments", Segments, MRB_ARGS_NONE());

    mrb_define_method(p_state, p_rcPath_Segment, "initialize", PS_Initialize, MRB_ARGS_REQ(4));
    mrb_define_method(p_state, p_rcPath_Segment, "start_x", PS_Get_Start_X, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcPath_Segment, "start_y", PS_Get_Start_Y, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcPath_Segment, "target_x", PS_Get_Target_X, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcPath_Segment, "target_y", PS_Get_Target_Y, MRB_ARGS_NONE());
}

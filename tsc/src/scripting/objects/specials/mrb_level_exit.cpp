/***************************************************************************
 * mrb_level_exit.cpp
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

#include "../../../objects/level_exit.hpp"
#include "../../../core/sprite_manager.hpp"
#include "../../../level/level.hpp"
#include "mrb_level_exit.hpp"
#include "../sprites/mrb_moving_sprite.hpp"
#include "../../events/event.hpp"

/**
 * Class: LevelExit
 *
 * Parent: L<MovingSprite>
 *
 * A I<level exit> is usually a location where Alex can regularily
 * win the level, without any scripting intervention. Level exits
 * however can also be used to warp Alex to other points in the
 * same level or even into a sublevel of the current one.
 *
 * =head2 Return level stack
 *
 * Level exits can make use of a return level stack that allows
 * to return to a specific level when a sublevel’s finish is
 * reached. To activate this feature, you first have to specify
 * a sublevel a level exit will warp Alex into as usual (see
 * L<#level=>). Utilizing the methods L<#return_level=> and
 * L<#return_entry=> you can now dynamically specify where Alex
 * should return when B<the sublevel’s> default finish is reached
 * (without setting one of these, the sublevel’s finish will be
 * taken for the overall finish and end all current levels, advancing
 * Alex to the next waypoint on the overworld).
 *
 * This feature can be useful for bonus levels or labyrinths, probably
 * for other complex level setups. Note it works fine recursively, i.e.
 * you can employ the same technique in the destination sublevel again.
 *
 * =head2 Events
 *
 * =over
 *
 * =item [Exit]
 *
 * Called when this level exit is activated. If this is a warping
 * level exit, the event handler is executed after Alex’s movements
 * are completed (but before the camera move and before the target
 * level entry is read from this object).
 *
 * =back
 */

using namespace TSC;
using namespace TSC::Scripting;


MRUBY_IMPLEMENT_EVENT(exit);

/**
 * Method: LevelExit::new
 *
 *   new() → a_level_exit
 *
 * Creates a new level exit with the default values.
 */
static mrb_value Initialize(mrb_state* p_state, mrb_value self)
{
    cLevel_Exit* p_exit = new cLevel_Exit(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_exit;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    // This is a generated object
    p_exit->Set_Spawned(true);

    // Let TSC manage the memory
    pActive_Level->m_sprite_manager->Add(p_exit);

    return self;
}

/**
 * Method: LevelExit#type=
 *
 *   type=( val ) → val
 *
 * Specifies the level exit’s new type.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [val]
 *
 * A symbol denoting the new type. C<:beam> directly beams the
 * player to the target without any animation, whereas C<:warp>
 * makes the player slowly move into the destined direction
 * of this level exit (typically used for pipes).
 *
 * =back
 */
static mrb_value Set_Type(mrb_state* p_state, mrb_value self)
{
    mrb_sym type;
    mrb_get_args(p_state, "n", &type);
    std::string typestr(mrb_sym2name(p_state, type));

    Level_Exit_type let;
    if (typestr == "beam")
        let = LEVEL_EXIT_BEAM;
    else if (typestr == "wrap")
        let = LEVEL_EXIT_WARP;
    else {
        mrb_raisef(p_state, MRB_ARGUMENT_ERROR(p_state), "Invalid level exit type %s", typestr.c_str());
        return mrb_nil_value();
    }

    cLevel_Exit* p_exit = Get_Data_Ptr<cLevel_Exit>(p_state, self);
    p_exit->Set_Type(let);

    return mrb_symbol_value(type);
}

/**
 * Method: LevelExit#type
 *
 *   type() → a_symbol
 *
 * The level exit’s type. See L<#type=> for a list of possible
 * return values.
 */
static mrb_value Get_Type(mrb_state* p_state, mrb_value self)
{
    cLevel_Exit* p_exit = Get_Data_Ptr<cLevel_Exit>(p_state, self);
    switch (p_exit->m_exit_type) {
    case LEVEL_EXIT_BEAM:
        return str2sym(p_state, "beam");
    case LEVEL_EXIT_WARP:
        return str2sym(p_state, "wrap");
    default:
        return mrb_nil_value();
    }
}

/**
 * Method: LevelExit#camera_motion=
 *
 *   camera_motion=( motion ) → motion
 *
 * Specify what happens to the camera when the level exit
 * is activated.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [motion]
 *
 * One of the following symbols:
 *
 * =over
 *
 * =item [none]
 *
 * Directly set the camera to the new position. This is the default.
 *
 * =item [fly]
 *
 * Slowly move the camera over to the new position.
 *
 * =item [blink]
 *
 * Fade out at the current position and fade back in at the new position.
 *
 * =item [path_forward]
 *
 * Make the camera follow a path. You need to use L<#path=> to specify
 * the target path for this.
 *
 * =item [path_backward]
 *
 * Make the camera follow a path backwards. You need to use L<#path=> to
 * specify the target path for this.
 *
 * =back
 *
 * =back
 */
static mrb_value Set_Camera_Motion(mrb_state* p_state, mrb_value self)
{
    mrb_sym camera;
    mrb_get_args(p_state, "n", &camera);
    std::string camerastr(mrb_sym2name(p_state, camera));

    Camera_movement mov;
    if (camerastr == "none")
        mov = CAMERA_MOVE_NONE;
    else if (camerastr == "fly")
        mov = CAMERA_MOVE_FLY;
    else if (camerastr == "blink")
        mov = CAMERA_MOVE_BLINK;
    else if (camerastr == "path_forward")
        mov = CAMERA_MOVE_ALONG_PATH;
    else if (camerastr == "path_backward")
        mov = CAMERA_MOVE_ALONG_PATH_BACKWARDS;
    else {
        mrb_raisef(p_state, MRB_ARGUMENT_ERROR(p_state), "Invalid camera movement %s", camerastr.c_str());
        return mrb_nil_value(); // Not reached
    }

    cLevel_Exit* p_exit = Get_Data_Ptr<cLevel_Exit>(p_state, self);
    p_exit->Set_Camera_Motion(mov);

    return mrb_symbol_value(mov);
}

/**
 * Method: LevelExit#camera_motion
 *
 *   camera_motion() → a_symbol
 *
 * Return the level exit’s camera motion. See L<#camera_motion=> for
 * a list of possible return values.
 */
static mrb_value Get_Camera_Motion(mrb_state* p_state, mrb_value self)
{
    cLevel_Exit* p_exit = Get_Data_Ptr<cLevel_Exit>(p_state, self);
    switch (p_exit->m_exit_motion) {
    case CAMERA_MOVE_NONE:
        return str2sym(p_state, "none");
    case CAMERA_MOVE_FLY:
        return str2sym(p_state, "fly");
    case CAMERA_MOVE_BLINK:
        return str2sym(p_state, "blink");
    case CAMERA_MOVE_ALONG_PATH:
        return str2sym(p_state, "path_forward");
    case CAMERA_MOVE_ALONG_PATH_BACKWARDS:
        return str2sym(p_state, "path_backward");
    default:
        return mrb_nil_value();
    }
}

/**
 * Method: LevelExit#level=
 *
 *   level=( name ) → name
 * Set the destination level for this exit. If you want to
 * warp inside the same level, use L<#entry=> to denote a
 * target level entry instead.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [name]
 *
 * The name of the new level (without path and without C<.tsclvl> extension).
 *
 * =back
 */
static mrb_value Set_Level(mrb_state* p_state, mrb_value self)
{
    char* level = NULL;
    mrb_get_args(p_state, "z", &level);

    cLevel_Exit* p_exit = Get_Data_Ptr<cLevel_Exit>(p_state, self);
    p_exit->Set_Level(level);

    return mrb_str_new_cstr(p_state, level);
}

/**
 * Method: LevelExit#level
 *
 *   level() → a_string
 *
 * Returns the destination level for this level exit as a string
 * without path and without C<.tsclvl> extension.
 */
static mrb_value Get_Level(mrb_state* p_state, mrb_value self)
{
    cLevel_Exit* p_exit = Get_Data_Ptr<cLevel_Exit>(p_state, self);
    return mrb_str_new_cstr(p_state, p_exit->m_dest_level.c_str());
}

/**
 * Method: LevelExit#entry=
 *
 *   entry=( name ) → name
 *
 * The target level entry for this level exit. If called I<without>
 * a prior call to L<#level=>, allows you to warp inside the current
 * level.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [name] The name of the target level entry.
 *
 * =back
 */
static mrb_value Set_Entry(mrb_state* p_state, mrb_value self)
{
    char* entry = NULL;
    mrb_get_args(p_state, "z", &entry);

    cLevel_Exit* p_exit = Get_Data_Ptr<cLevel_Exit>(p_state, self);
    p_exit->Set_Entry(entry);

    return mrb_str_new_cstr(p_state, entry);
}

/**
 * Method: LevelExit#entry
 *
 *   entry() → a_string
 *
 * Returns the target level entry’s identifier for this level exit.
 */
static mrb_value Get_Entry(mrb_state* p_state, mrb_value self)
{
    cLevel_Exit* p_exit = Get_Data_Ptr<cLevel_Exit>(p_state, self);
    return mrb_str_new_cstr(p_state, p_exit->m_dest_entry.c_str());
}

/**
 * Method: LevelExit#return_level=
 *
 *   return_level=( str ) → str
 *
 * Specifies the name of the level that is placed on the stack when
 * Alex enters this level exit. An empty string means to push the
 * current level onto the stack.
 *
 * Note you have to set a destination sublevel via L<#level=> in
 * order for this setting to have any effect.
 */
static mrb_value Set_Return_Level(mrb_state* p_state, mrb_value self)
{
    char* level = NULL;
    mrb_get_args(p_state, "z", &level);

    cLevel_Exit* p_exit = Get_Data_Ptr<cLevel_Exit>(p_state, self);
    p_exit->Set_Return_Level(level);

    return mrb_str_new_cstr(p_state, level);
}

/**
 * Method: LevelExit#return_level
 *
 *   return_level() → a_string
 *
 * Returns the name of the level that is placed on the stack when
 * Alex enters this level exit. If the returned string is empty, the
 * current level will be pushed onto the stack.
 *
 * Note that this setting only takes effect if a destination
 * sublevel has been specified (see L<#level=>).
 */
static mrb_value Get_Return_Level(mrb_state* p_state, mrb_value self)
{
    cLevel_Exit* p_exit = Get_Data_Ptr<cLevel_Exit>(p_state, self);
    return mrb_str_new_cstr(p_state, p_exit->m_return_level.c_str());
}

/**
 * Method: LevelExit#return_entry
 *
 * Returns the name of the level entry that is associated with the
 * level pushed onto the return stack (see L<#return_level>). If
 * this returns an empty string, the default level starting position
 * will be used. If you specify no return level, the entry will be
 * assumed to be from the current level.
 */
static mrb_value Get_Return_Entry(mrb_state* p_state, mrb_value self)
{
    cLevel_Exit* p_exit = Get_Data_Ptr<cLevel_Exit>(p_state, self);
    return mrb_str_new_cstr(p_state, p_exit->m_return_entry.c_str());
}

/**
 * Method: LevelExit#entry=
 *
 *   entry=( str ) → str
 *
 * Specifies the name of the level entry that is associated with
 * the level pushed onto the return stack (see L<#return_level>). Set
 * this to an empty string for requesting the default starting position
 * of the return level. If no return level has been specified,
 * the entry will be assumed to belong to the current level.
 */
static mrb_value Set_Return_Entry(mrb_state* p_state, mrb_value self)
{
    char* entry = NULL;
    mrb_get_args(p_state, "z", &entry);

    cLevel_Exit* p_exit = Get_Data_Ptr<cLevel_Exit>(p_state, self);
    p_exit->Set_Return_Entry(entry);

    return mrb_str_new_cstr(p_state, entry);
}

/**
 * Method: LevelExit#exit_name
 *
 *   exit_name() → a_string
 *
 * Returns the name of this level exit. The name is used to determine
 * which path to go on the overworld map.
 */
static mrb_value Get_Exit_Name(mrb_state* p_state, mrb_value self)
{
    cLevel_Exit* p_exit = Get_Data_Ptr<cLevel_Exit>(p_state, self);
    return mrb_str_new_cstr(p_state, p_exit->m_exit_name.c_str());
}

/**
 * Method: LevelExit#exit_name=
 *
 *   exit_name=( str ) → str
 *
 * Set the name of this level exit. The name is used to determine
 * which path to go on the overworld map.
 */
static mrb_value Set_Exit_Name(mrb_state* p_state, mrb_value self)
{
    char* name = NULL;
    mrb_get_args(p_state, "z", &name);

    cLevel_Exit* p_exit = Get_Data_Ptr<cLevel_Exit>(p_state, self);
    p_exit->Set_Exit_Name(name);

    return mrb_str_new_cstr(p_state, name);
}

/**
 * Method: LevelExit#path=
 *
 *   path=( ident ) → ident
 *
 * Specify the identifier for the camera movement path if
 * you set L<#camera_motion=> to one of the C<path> options.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [ident] The name of path you want to move the camera along.
 *
 * =back
 */
static mrb_value Set_Path(mrb_state* p_state, mrb_value self)
{
    char* identifier = NULL;
    mrb_get_args(p_state, "z", &identifier);

    cLevel_Exit* p_exit = Get_Data_Ptr<cLevel_Exit>(p_state, self);
    p_exit->Set_Path_Identifier(identifier);

    return mrb_str_new_cstr(p_state, identifier);
}

/**
 * Method: LevelExit#path
 *
 *   path() → a_string
 *
 * Returns the name of the camera motion path.
 */
static mrb_value Get_Path(mrb_state* p_state, mrb_value self)
{
    cLevel_Exit* p_exit = Get_Data_Ptr<cLevel_Exit>(p_state, self);
    return mrb_str_new_cstr(p_state, p_exit->m_path_identifier.c_str());
}

/**
 * Method: LevelExit#activate
 *
 *   activate()
 *
 * Activates this level exit, warping Alex to the target or
 * ending the level.
 */
static mrb_value Activate(mrb_state* p_state, mrb_value self)
{
    cLevel_Exit* p_exit = Get_Data_Ptr<cLevel_Exit>(p_state, self);
    p_exit->Activate();

    return mrb_nil_value();
}

void TSC::Scripting::Init_LevelExit(mrb_state* p_state)
{
    struct RClass* p_rcLevel_Exit = mrb_define_class(p_state, "LevelExit", mrb_class_get(p_state, "MovingSprite"));
    MRB_SET_INSTANCE_TT(p_rcLevel_Exit, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcLevel_Exit, "initialize", Initialize, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel_Exit, "type=", Set_Type, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcLevel_Exit, "type", Get_Type, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel_Exit, "camera_motion=", Set_Camera_Motion, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcLevel_Exit, "camera_motion", Get_Camera_Motion, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel_Exit, "level=", Set_Level, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcLevel_Exit, "level", Get_Level, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel_Exit, "entry=", Set_Entry, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcLevel_Exit, "entry", Get_Entry, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel_Exit, "path=", Set_Path, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcLevel_Exit, "path", Get_Path, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel_Exit, "activate", Activate, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel_Exit, "return_level", Get_Return_Level, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel_Exit, "return_level=", Set_Return_Level, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcLevel_Exit, "return_entry", Get_Return_Entry, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel_Exit, "return_entry=", Set_Return_Entry, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcLevel_Exit, "exit_name", Get_Exit_Name, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel_Exit, "exit_name=", Set_Exit_Name, MRB_ARGS_REQ(1));

    mrb_define_method(p_state, p_rcLevel_Exit, "on_exit", MRUBY_EVENT_HANDLER(exit), MRB_ARGS_NONE());
}

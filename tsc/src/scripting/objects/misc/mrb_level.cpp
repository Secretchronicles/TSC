/***************************************************************************
 * mrb_level.cpp
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

#include "../../../level/level.hpp"
#include "../../../level/level_player.hpp"
#include "../../../user/savegame/savegame.hpp"
#include "../../../gui/hud.hpp"
#include "../../../core/property_helper.hpp"
#include "../../events/event.hpp"
#include "../mrb_eventable.hpp"
#include "mrb_level.hpp"

/**
 * Class: LevelClass
 *
 * C<LevelClass> exposes it’s sole instance through the C<Level> singleton,
 * which always points to the currently active level. It is a mostly
 * informational object allowing you to access a level’s global settings,
 * but does not permit you to change them, because this either wouldn’t
 * make much sense in the first place (why change the author name from
 * within the script?) or could even cause severe confusion for the game
 * (such as changing the filename).
 *
 * This class allows you to register handlers for a very special
 * event: The B<save_load> event. This event is not
 * fired during regular gameplay, but instead when the player creates a
 * new savegame or restores an existing one. By
 * filling a special object (L<SaveSerializer>) from the event handler, you can
 * advertise TSC to store arbitrary data in the savegame; later, when the user loads
 * this savegame again, the data is deserialised from the savegame and
 * passed back as an argument to the event handler of the B<save_load>
 * event. This way you can store information on your level from within
 * the scripting API that will persist between saves and loads of a
 * level.
 *
 * There only is a single B<save_load> event which is used for both
 * saving and loading a savegame. The event handler gets passed
 * an argument that is C<true> on a save, and C<false> on a load
 * operation. This way the API ensures that it cannot accidentally
 * happen that there is an unbalanced amount of save/load event
 * handlers, which would cause problems.
 *
 * Consider this example:
 *
 *     # Say, you have a number of switches in your
 *     # level. Their state is stored inside this
 *     # global table.
 *     switches = {
 *       :blue  => false,
 *       :red   => false,
 *       :green => false
 *     }
 *
 *     # The player may activate your switches,
 *     # causing the respective entry in the
 *     # global `switches' table to change.
 *     UIDS[114].on_touch do |collidor|
 *       switches[:red] = true if collidor.player?
 *     end
 *
 *     # Now, if the player jumps on your switch and
 *     # then saves and reloads, the switches’ states
 *     # get lost. To prevent this, we define a handler
 *     # for the `save_load' event that persists
 *     # the state of the global `switches' table on saving,
 *     # and restores it on loading. See below for why
 *     # strings are used as keys on the `store'.
 *     Level.on_save_load do |store, is_save|
 *       if is_save
 *         store["blue"]  = switches[:blue]
 *         store["red"]   = switches[:red]
 *         store["green"] = switches[:green]
 *       else
 *         switches["blue"]  = store[:blue]
 *         switches["red"]   = store[:red]
 *         switches["green"] = store[:green]
 *       end
 *     end
 *
 *     # This way the switches will remain in their
 *     # respective state even after saving/reloading
 *     # a game. If you change graphics for pressed
 *     # switches, you still have to do this manually
 *     # in your event handlers, though.
 *
 * The storage object passed to the B<save_load> event handler is an
 * instance of L<SaveSerializer>, which wraps an underlying C++
 * structure that allows serialisaton of the data stored into XML
 * format. It's a consequence of this design that it is not possible
 * to persist arbitrary mruby objects. It is only supported to store
 * strings, symbols, nil, true and false, and numerics (both fixnums
 * and floats) in the storage object. The attempt to store anything
 * else is going to raise a TypeError mruby exception.
 *
 * Keys used with the storage object have to be strings. Using any
 * other object as a key results in an exception. Keys starting
 * with the string "__" (two consecutive underscores) are reserved
 * for TSC's internal use and may not be used by level authors.
 *
 * When writing a B<save_load> event handler, it is recommended to
 * extensively test it. Any exceptions raised during save and
 * load operations will be printed to the game console (which
 * can be made visible by pressing [F7] in a level).
 *
 * Handlers for the B<save_load> event B<must not> be defined in a
 * conditional clause of any kind. To comply with this restriction,
 * simply define any handlers on the level script's toplevel. If
 * you need conditional saving/loading, place the conditional
 * I<inside> the B<save_load> event handler.
 *
 * The reason for this restriction is that the game assumes that
 * one entry in the savegame's script data section corresponds to
 * exactly one B<save_load> handler. On savegame loading, it
 * deconstructs the savegame's script data entries one by one and
 * passes each deserialised entry to the next event handler defined.
 * If B<save_load> handlers are defined under a condition, the
 * assumption breaks and it is not foreseeable which event handler
 * receives which storage entry.
 *
 * It is recommended to not register more than one event handler
 * for the B<save_load> event. It is possible, but complicates
 * compliance with the above restrictions. Registration of additional
 * event handlers for B<save_load> is mostly useful for scripts that
 * are part of the SSL.
 *
 * =head2 Internal note
 *
 * You will most likely neither notice nor need it, but the mruby C<Level>
 * singleton actually doesn’t wrap TSC’s notion of the currently running
 * level, C<pActive_Level>, but rather the pointer to the savegame
 * mechanism, C<pSavegame>. This facilitates the handling of the event
 * table for levels. Also, it is more intuitively to have the C<Save>
 * and C<Load> events defined on the Level rather than on a separate
 * Savegame object.
 *
 * =head2 Events
 *
 * =over
 *
 * =item [save_load]
 *
 * Called when the user saves or loads a savegame containing this
 * level. The event handler gets passed a storage object
 * (L<SaveSerializer>) and a boolean indicator that indicates whether
 * a save (true) or load (false) operation is in progress. Do not
 * assume your level is active when this is called, the player may be
 * in a sublevel. However, usually this has no impact on what you want
 * to restore, but don’t execute actions on the level player (like
 * warping it) or on any object managed by the HUD (game timer, amount
 * of collected jewels, etc.), it will result in undefined behaviour
 * probably leading TSC to crash.
 *
 * =back
 *
 * =head2 See Also
 *
 * L<LevelPlayer>
 */

using namespace TSC;
using namespace TSC::Scripting;


/***************************************
 * Events
 ***************************************/

MRUBY_IMPLEMENT_EVENT(save_load);

/***************************************
 * Methods
 ***************************************/

/**
 * Method: LevelClass#author
 *
 *   author() → a_string
 *
 * Returns the content of the level’s I<Author> info field.
 */
static mrb_value Get_Author(mrb_state* p_state,  mrb_value self)
{
    return mrb_str_new_cstr(p_state, pActive_Level->m_author.c_str());
}

/**
 * Method: LevelClass#description
 *
 *   description() → a_string
 *
 * Returns the content of the level’s I<Description> info field.
 */
static mrb_value Get_Description(mrb_state* p_state, mrb_value self)
{
    return mrb_str_new_cstr(p_state, pActive_Level->m_description.c_str());
}

/**
 * Method: LevelClass#difficulty
 *
 *   difficulty() → an_integer
 *
 * Returns the content of the level’s I<Difficulty> info field.
 * This reaches from 0 (undefined) over 1 (very easy) to 100
 * ((mostly) uncompletable),
 */
static mrb_value Get_Difficulty(mrb_state* p_state, mrb_value self)
{
    return mrb_fixnum_value(pActive_Level->m_difficulty);
}

/**
 * Method: LevelClass#engine_version
 *
 *   engine_version() → an_integer
 *
 * Returns the TSC engine version used to create the level.
 */
static mrb_value Get_Engine_Version(mrb_state* p_state, mrb_value self)
{
    return mrb_fixnum_value(pActive_Level->m_engine_version);
}

/**
 * Method: LevelClass#filename
 *
 *   filename() → a_string
 *
 * Returns the level’s filename.
 */
static mrb_value Get_Filename(mrb_state* p_state, mrb_value self)
{
    return mrb_str_new_cstr(p_state, path_to_utf8(pActive_Level->m_level_filename).c_str());
}

/**
 * Method: LevelClass#music_filename
 *
 *   music_filename( [ format [, with_ext ] ] ) → a_string
 *
 * Returns the default level music’s filename, relative to
 * the C<music/> directory.
 */
static mrb_value Get_Music_Filename(mrb_state* p_state, mrb_value self)
{
    return mrb_str_new_cstr(p_state, path_to_utf8(pActive_Level->Get_Music_Filename()).c_str());
}

/**
 * Method: LevelClass#script
 *
 *   script() → a_string
 *
 * Returns the MRuby code associated with this level.
 */
static mrb_value Get_Script(mrb_state* p_state, mrb_value self)
{
    return mrb_str_new_cstr(p_state, pActive_Level->m_script.c_str());
}

/**
 * Method: LevelClass#next_level_filename
 *
 *   next_level_filename() → a_string
 *
 * If a new level shall automatically be loaded when this level
 * completes, this returns the filename of the target level. Otherwise
 * the return value is undefined, but most likely an empty string.
 */
static mrb_value Get_Next_Level_Filename(mrb_state* p_state, mrb_value self)
{
    return mrb_str_new_cstr(p_state, path_to_utf8(pActive_Level->m_next_level_filename).c_str());
}

/**
 * Method: LevelClass#finish
 *
 *   finish( [ win_music ] )
 *
 * The player immediately wins the level and the game resumes to the
 * world overview, advancing to the next level point. If the level was
 * loaded using the level menu directly (and hence there is no
 * overworld), returns to the level menu.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [win_music (false)]
 *
 * If set, plays the level win music.
 *
 * =item [exit_name ("")]
 *
 * Name of the level exit taken (used in the overworld
 * to determine which path to take).
 *
 * =back
 */
static mrb_value Finish(mrb_state* p_state,  mrb_value self)
{
    mrb_value obj;
    char* exit_name = NULL;
    mrb_get_args(p_state, "|oz", &obj, &exit_name);

    if (exit_name) {
        pLevel_Manager->Finish_Level(mrb_test(obj), exit_name);
    }
    else {
        pLevel_Manager->Finish_Level(mrb_test(obj));
    }

    return mrb_nil_value();
}

/**
 * Method: LevelClass#display_info_message
 *
 *   display_info_message( message )
 *
 * Shows a B<short>, informative message on the screen. This is achieved
 * by displaying a prominent sprite covering the full width of the
 * game window containing your message for a few seconds, before the
 * entire construction (i.e. sprite plus message) is then slowly faded
 * out to invisibility.
 *
 * This method is not meant to display larger passages of text to the
 * user; use the C<Message> class from the SSL for that. No line breaking
 * is done (and only a single line of text is supported).
 *
 * This method is intended for displaying merely optional pieces of
 * information; for instance, if you built a large tower level, you
 * may use this method to display the floor the player just entered to
 * give him more orientation.
 *
 * Do not overuse this method. If you use it, stick to one usage scheme;
 * don’t use it for too many different kinds of information, that would
 * confuse the player probably.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [message]
 *
 * The message to display. A short oneliner.
 *
 * =back
 *
 * =head4 Example
 *
 *     # Say the object with UID 14 is a warp point that
 *     # warps you to the tower’s 3rd floor when touched.
 *     # To make the player aware, write your code like this:
 *     UIDS[14].on_touch do |collidor|
 *       next unless collidor.player? # Only react on the player
 *
 *       Level.display_info_message("3rd floor")
 *       collidor.warp(400, -620)
 *     end
 */
static mrb_value Display_Info_Message(mrb_state* p_state, mrb_value self)
{
    char* message = NULL;
    mrb_get_args(p_state, "z", &message);

    gp_hud->Set_Text(message);
    return mrb_nil_value();
}

/**
 * Method: LevelClass#push_return
 *
 *   push_return( stackentry )
 *
 * Push a C<Level::StackEntry> onto the return level stack.
 *
 * See L<LevelExit> for explanations on the return stack.
 */
static mrb_value Push_Return(mrb_state* p_state, mrb_value self)
{
    mrb_value stackentry;
    mrb_get_args(p_state, "o", &stackentry);

    mrb_value level = mrb_iv_get(p_state, stackentry, mrb_intern_cstr(p_state, "@level"));
    mrb_value entry = mrb_iv_get(p_state, stackentry, mrb_intern_cstr(p_state, "@entry"));

    // Note that `nil.to_s' gives an empty string.
    pLevel_Player->Push_Return(mrb_string_value_ptr(p_state, level), mrb_string_value_ptr(p_state, entry));

    return mrb_nil_value();
}

/**
 * Method: LevelClass#pop_entry
 *
 *   pop_entry() → a_stackentry or nil
 *
 * Pops the next available C<Level::StackEntry> object from the
 * level return stack and returns it. If there is none, returns
 * C<nil>.
 *
 * See L<LevelExit> for explanations on the return stack.
 */
static mrb_value Pop_Return(mrb_state* p_state, mrb_value self)
{
    std::string level, entry;

    if (pLevel_Player->Pop_Return(level, entry)) {
        struct RClass* p_klass = mrb_class_get_under(p_state, mrb_class_get(p_state, "Level"), "StackEntry");
        mrb_value args[2];
        args[0] = mrb_str_new_cstr(p_state, level.c_str());
        args[1] = mrb_str_new_cstr(p_state, entry.c_str());

        return mrb_obj_new(p_state, p_klass, 2, args);
    }
    else {
        return mrb_nil_value();
    }
}

/**
 * Method: LevelClass#clear_return
 *
 *   clear_return()
 *
 * Empties the level return stack.
 *
 * See L<LevelExit> for explanations on the return stack.
 */
static mrb_value Clear_Return(mrb_state* p_state, mrb_value self)
{
    pLevel_Player->Clear_Return();
    return mrb_nil_value();
}

/**
 * Method: LevelClass#return_stack
 *
 *   return_stack() → an_array
 *
 * Returns the current return stack as an array of Level::StackEntry
 * instances.
 *
 * See L<LevelExit> for explanations on the return stack.
 */
static mrb_value Get_Return_Stack(mrb_state* p_state, mrb_value self)
{
    mrb_value ary = mrb_ary_new(p_state);

    std::vector<cLevel_Player_Return_Entry>::const_iterator iter;
    struct RClass* p_klass = mrb_class_get_under(p_state, mrb_class_get(p_state, "Level"), "StackEntry");
    for (iter=pLevel_Player->m_return_stack.begin(); iter != pLevel_Player->m_return_stack.end(); iter++) {
        cLevel_Player_Return_Entry entry = *iter;
        mrb_value args[2];
        args[0] = mrb_str_new_cstr(p_state, entry.level.c_str());
        args[1] = mrb_str_new_cstr(p_state, entry.entry.c_str());

        mrb_value stackentry = mrb_obj_new(p_state, p_klass, 2, args);
        mrb_ary_push(p_state, ary, stackentry);
    }

    return ary;
}

/**
 * Method: LevelClass#boundaries
 *
 *   boundaries() → a_rect
 *
 * Returns the level's boundaries as a Rect instance (struct with C<x>,
 * C<y>, C<width>, C<height> members). X and Y will always be zero; note
 * that towards the upper edge the coordinates are lower, which is why
 * you usually have a negative height in a level.
 */
static mrb_value Get_Boundaries(mrb_state* p_state, mrb_value self)
{
    struct RClass* p_class = mrb_class_get(p_state, "Rect");

    mrb_value args[4];
    args[0] = mrb_float_value(p_state, pActive_Level->m_camera_limits.m_x);
    args[1] = mrb_float_value(p_state, pActive_Level->m_camera_limits.m_y);
    args[2] = mrb_float_value(p_state, pActive_Level->m_camera_limits.m_w);
    args[3] = mrb_float_value(p_state, pActive_Level->m_camera_limits.m_h);

    return mrb_obj_new(p_state, p_class, 4, args);
}

/**
 * Method: LevelClass#start_position
 *
 *   start_position() → a_point
 *
 * Returns the position Alex starts when the level is entered either
 * from the world map or from the level menu (not via a sublevel entry).
 * Return value is a Point instance (struct with members C<x> and C<y>).
 */
static mrb_value Get_Start_Position(mrb_state* p_state, mrb_value self)
{
    struct RClass* p_class = mrb_class_get(p_state, "Point");

    mrb_value args[2];
    args[0] = mrb_float_value(p_state, pActive_Level->m_player_start_pos_x);
    args[1] = mrb_float_value(p_state, pActive_Level->m_player_start_pos_y);

    return mrb_obj_new(p_state, p_class, 2, args);
}

/**
 * Method: LevelClass#fixed_horizontal_velocity
 *
 *   fixed_horizontal_velocity() → a_number
 *
 * Returns the fixed horizontal scrolling velocity. This is usually 0,
 * as the feature is only used by a handful of levels (it makes the
 * camera move horizontally automatically and kills Alex if he falls
 * behind too much).
 */
static mrb_value Get_Fixed_Hor_Vel(mrb_state* p_state, mrb_value self)
{
    return mrb_float_value(p_state, pActive_Level->m_fixed_camera_hor_vel);
}

/********************* StackEntry ********************/

/**
 * Class: LevelClass::StackEntry
 *
 * Instances of this class serve a purely informational
 * purpose, they have no real methods that actually do
 * something. They are used to represent the entries
 * in the level return stack, as explained in
 * L<LevelExit>.
 */

/**
 * Method: Level::StackEntry::new
 *
 *   new( [ level [, entry ] ] ) → a_stack_entry
 *
 * Creates a new stack entry that refers to the given
 * level/entry combination.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [level ("")]
 *
 * Name of the level to return to. An empty string means
 * to return to the current level.
 *
 * =item [entry ("")]
 *
 * Name of the level entry to return to. An empty string
 * means to return the default starting position.
 *
 * =back
 */
static mrb_value SE_Initialize(mrb_state* p_state, mrb_value self)
{
    mrb_value level;
    mrb_value entry;
    mrb_get_args(p_state, "|oo", &level, &entry);

    mrb_iv_set(p_state, self, mrb_intern_cstr(p_state, "@level"), level);
    mrb_iv_set(p_state, self, mrb_intern_cstr(p_state, "@entry"), entry);

    return self;
}

/**
 * Method: Level::StackEntry#level
 *
 *   level() → a_string
 *
 * Return the return level’s name.
 */
static mrb_value SE_Get_Level(mrb_state* p_state, mrb_value self)
{
    return mrb_iv_get(p_state, self, mrb_intern_cstr(p_state, "@level"));
}

/**
 * Method: Level::StackEntry#entry
 *
 *   entry() → a_string
 *
 * Return the return level exit.
 */
static mrb_value SE_Get_Entry(mrb_state* p_state, mrb_value self)
{
    return mrb_iv_get(p_state, self, mrb_intern_cstr(p_state, "@entry"));
}

/********************* SaveSerializer ********************/

/**
 * Class: SaveSerializer
 *
 * This is an internal class. Instances of it are created
 * from the C++ code. It's the class for the storage object
 * passed to L<LevelClass>' C<save_load> event handler.
 *
 * This class includes mruby's C<Enumerable> module. Note
 * however that no particular order of the elements is
 * guaranteed.
 */

// Leave this undocumented
static mrb_value SL_Initialize(mrb_state* p_state, mrb_value self)
{
    Script_Data* p_storage = new Script_Data();
    DATA_PTR(self) = p_storage;
    DATA_TYPE(self) = &rtTSC_Scriptable;
    return self;
}

/**
 * Method: SaveSerializer#[]=
 *
 *   serializer[str] = value → value
 *
 * Stores the object C<value> in the savegame data under
 * the given C<str>. C<str> has to be a string.
 */
static mrb_value SL_Set(mrb_state* p_state, mrb_value self)
{
    Script_Data* p_storage = Get_Data_Ptr<Script_Data>(p_state, self);
    char* key;
    mrb_value obj;
    mrb_get_args(p_state, "zo", &key, &obj);

    std::string type;
    std::stringstream value;
    if (mrb_nil_p(obj)) { // There is no MRB_TT_NIL for the below `switch'
        type = "nil";
        value << "nil";
    }
    else {
        switch (mrb_type(obj)) {
        case MRB_TT_FIXNUM:
            type = "fixnum";
            value << mrb_int(p_state, obj);
            break;
        case MRB_TT_SYMBOL:
            type = "symbol";
            value << mrb_sym2name(p_state, mrb_symbol(obj));
            break;
        case MRB_TT_FLOAT:
            type = "float";
            value << mrb_float(obj);
            break;
        case MRB_TT_STRING:
            type = "string";
            value << mrb_string_value_ptr(p_state, obj);
            break;
        case MRB_TT_FALSE:
            type = "boolean";
            value << "false";
            break;
        case MRB_TT_TRUE:
            type = "boolean";
            value << "true";
            break;
        default:
            mrb_raise(p_state, MRB_TYPE_ERROR(p_state), "Unsupported type for script save storage");
            return mrb_nil_value();
        }
    }

    (*p_storage)[key] = std::make_pair(type, value.str());
    return obj;
}

// Helper function to convert the C++ish string representation to
// an actual mruby object.
static mrb_value sl_convert_cpp_to_mrb(mrb_state* p_state, std::string type, std::string value)
{
    if (type == "fixnum")
        return mrb_fixnum_value(std::stoi(value));
    else if (type == "symbol")
        return str2sym(p_state, value);
    else if (type == "float")
        return mrb_float_value(p_state, std::stof(value));
    else if (type == "string")
        return mrb_str_new(p_state, value.data(), value.length());
    else if (type == "nil")
        return mrb_nil_value();
    else if (type == "boolean")
            return value == "true" ? mrb_true_value() : mrb_false_value();
    else {
        std::cerr << "Unsupported type found in level script storage, returning nil." << std::endl;
        return mrb_nil_value();
    }
}

/**
 * Method: SaveSerializer#[]
 *
 *   serializer[str] → an_object
 *
 * Retrieves the object stored under C<str> from the savegame.
 * If there is no object under that key, returns nil.
 */
static mrb_value SL_Get(mrb_state* p_state, mrb_value self)
{
    Script_Data* p_storage = Get_Data_Ptr<Script_Data>(p_state, self);
    char* key;
    mrb_get_args(p_state, "z", &key);

    if (p_storage->count(key) > 0) {
        std::string type = std::get<0>((*p_storage)[key]);
        std::string value = std::get<1>((*p_storage)[key]);
        return sl_convert_cpp_to_mrb(p_state, type, value);
    }
    else {
        return mrb_nil_value();
    }
}

/**
 * Method: SaveSerializer#each
 *
 *   each() { |ary| ... }
 *
 * Iterates all values stored in the serializer. The block
 * receives a two-argument array of form [key, value].
 */
static mrb_value SL_Each(mrb_state* p_state, mrb_value self)
{
    mrb_value block;
    mrb_get_args(p_state, "&!", &block);

    Script_Data* p_storage = Get_Data_Ptr<Script_Data>(p_state, self);

    for(auto iter=p_storage->begin(); iter != p_storage->end(); iter++) {
        std::string key   = iter->first;
        std::string type  = std::get<0>(iter->second);
        std::string value = std::get<1>(iter->second);

        mrb_value ary = mrb_ary_new(p_state);
        mrb_ary_push(p_state, ary, mrb_str_new_cstr(p_state, key.c_str()));
        mrb_ary_push(p_state, ary, sl_convert_cpp_to_mrb(p_state, type, value));
        mrb_yield(p_state, block, ary);
    }

    return self;
}

/**
 * Method: SaveSerializer#inspect
 *
 *     inspect() → a_string
 *
 * Returns a human-readable description of this object.
 */
static mrb_value SL_Inspect(mrb_state* p_state, mrb_value self)
{
    std::stringstream ss;
    Script_Data* p_storage = Get_Data_Ptr<Script_Data>(p_state, self);

    ss << "#<SaveSerializer with "
       << p_storage->size()
       << " elements>";

    return mrb_str_new_cstr(p_state, ss.str().c_str());
}

void TSC::Scripting::Init_Level(mrb_state* p_state)
{
    struct RClass* p_rcLevel = mrb_define_class(p_state, "LevelClass", p_state->object_class);
    mrb_include_module(p_state, p_rcLevel, mrb_module_get(p_state, "Eventable"));
    MRB_SET_INSTANCE_TT(p_rcLevel, MRB_TT_DATA);

    // Make the Level constant the only instance of LevelClass
    mrb_define_const(p_state, p_state->object_class, "Level", pSavegame->Create_MRuby_Object(p_state));

    // Forbid creating instances of Level
    mrb_undef_class_method(p_state, p_rcLevel, "new");

    mrb_define_method(p_state, p_rcLevel, "author", Get_Author, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel, "description", Get_Description, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel, "difficulty", Get_Difficulty, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel, "engine_version", Get_Engine_Version, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel, "filename", Get_Filename, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel, "music_filename", Get_Music_Filename, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel, "script", Get_Script, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel, "next_level_filename", Get_Next_Level_Filename, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel, "finish", Finish, MRB_ARGS_OPT(1));
    mrb_define_method(p_state, p_rcLevel, "display_info_message", Display_Info_Message, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcLevel, "push_return", Push_Return, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcLevel, "pop_return", Pop_Return, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel, "clear_return", Clear_Return, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel, "return_stack", Get_Return_Stack, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel, "boundaries", Get_Boundaries, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel, "start_position", Get_Start_Position, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel, "fixed_horizontal_velocity", Get_Fixed_Hor_Vel, MRB_ARGS_NONE());

    mrb_define_method(p_state, p_rcLevel, "on_save_load", MRUBY_EVENT_HANDLER(save_load), MRB_ARGS_NONE());

    struct RClass* p_rcLevel_StackEntry = mrb_define_class_under(p_state, p_rcLevel, "StackEntry", p_state->object_class);

    mrb_define_method(p_state, p_rcLevel_StackEntry, "initialize", SE_Initialize, MRB_ARGS_OPT(2));
    mrb_define_method(p_state, p_rcLevel_StackEntry, "level", SE_Get_Level, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel_StackEntry, "entry", SE_Get_Entry, MRB_ARGS_NONE());

    struct RClass* p_rcSaveSerializer = mrb_define_class(p_state, "SaveSerializer", p_state->object_class);
    MRB_SET_INSTANCE_TT(p_rcSaveSerializer, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcSaveSerializer, "initialize", SL_Initialize, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcSaveSerializer, "[]=", SL_Set, MRB_ARGS_REQ(2));
    mrb_define_method(p_state, p_rcSaveSerializer, "[]", SL_Get, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcSaveSerializer, "each", SL_Each, MRB_ARGS_BLOCK());
    mrb_define_method(p_state, p_rcSaveSerializer, "inspect", SL_Inspect, MRB_ARGS_NONE());
    mrb_include_module(p_state, p_rcSaveSerializer, mrb_module_get(p_state, "Enumerable"));
}

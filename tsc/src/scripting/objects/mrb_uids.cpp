/***************************************************************************
 * mrb_uids.cpp - mruby UIDS module
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

#include "mrb_uids.hpp"
#include "../../level/level.hpp"
#include "../../objects/sprite.hpp"
#include "../../core/sprite_manager.hpp"

/*****************************************************************************
 * Be sure to read docs/pages/mruby_sprite_management.md!
 *****************************************************************************/

/**
 * Module: UIDS
 *
 * The C<UIDS> module (yes, really, it’s a module) is a simple way to
 * refer to existing instances of class C<Sprite> and its subclasses.
 * It basically just offers the module method C<[]> that allows you
 * to retrieve any sprite you wish from the level, identified by its
 * unique identifier (UID):
 *
 *     # Move the sprite with the UID 25 away
 *     UIDS[25].warp(-100, 0)
 *
 * The C<UIDS> module maintains a cache for the sprite objects so that it
 * doesn’t have to create MRuby objects for all the sprites right at the
 * beginning of a level, but rather when you first access them. This means
 * that while level loading is fast, referencing a bunch of not-yet-seen
 * sprites will probably cause a noticable pause in the gameplay, so be
 * careful when doing this. After a sprite has first been mapped to MRuby
 * land, referencing it will just cause a lookup in the internal cache
 * and therefore is quite fast.
 */

using namespace TSC;


// Try to retrieve the given index UID from the cache, and if
// that doesn’t work, do the long shot and insert that sprite
// into the cache, then return the mruby object for it.
// p_state: mruby state
// cache: The UID-sprite cache
// ruid: The mruby fixnum index
static mrb_value _Index(mrb_state* p_state, mrb_value cache, mrb_value ruid)
{
    // Try to retrieve the sprite from the cache
    mrb_value sprite = mrb_hash_get(p_state, cache, ruid);

    // If we already have an object for this UID in the
    // cache, return it.
    if (!mrb_nil_p(sprite))
        return sprite;

    // Otherwise, allocate a new MRuby object for it and store
    // that new object in the cache.
    cSprite_List objs = pActive_Level->m_sprite_manager->objects; // Shorthand
    mrb_int uid = mrb_fixnum(ruid);
    for (cSprite_List::const_iterator iter = objs.begin(); iter != objs.end(); iter++) {
        if ((*iter)->m_uid == uid) {
            // Ask the sprite to create the correct type of MRuby object
            // so we don’t have to maintain a static C++/MRuby type mapping table
            mrb_value obj = (*iter)->Create_MRuby_Object(p_state);
            // Store it in the cache
            mrb_hash_set(p_state, cache, ruid, obj);

            return obj;
        }
    }

    return mrb_nil_value();
}

/**
 * Method: UIDS::[]
 *
 *   [uid]   → a_sprite
 *   [range] → an_array
 *   [ary]   → an_array
 *
 * Retrieve an MRuby object for the sprite with the unique identifier
 * C<uid>. The first time you call this method with a given UID, it
 * will cycle through I<all> sprite objects in the level, so it will
 * take relatively long. The sprite object is then cached internally,
 * causing later lookups to be fast.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [uid]
 *
 * The unique identifier of the sprite you want to retrieve. You can
 * look this up in the TSC editor. May also be a range.
 *
 * =item [range]
 *
 * Instead of requesting a single sprite, request a list of sprites
 * corresponding to the given range of UIDs.
 *
 * =item [ary]
 *
 * Instead of requesting a single sprite, request a list of sprites
 * corresponding to the given UIDs.
 *
 * =back
 *
 * =head4 Return value
 *
 * Returns an instance of class C<Sprite> or one of its subclasses, as
 * required. If the requested UID can’t be found, returns C<nil>.
 *
 * If you passed a range or array, you’ll get an array containing the
 * requested C<Sprite> subclass instances instead. The array may
 * contain C<nil> values for sprites that could not be found.
 *
 * =head4 Example
 *
 *     # Request a single sprite
 *     sprite = UIDS[14]
 *
 *     # Request all sprites between UID 10 and 20
 *     ary = UIDS[10..20]
 *
 *     # Request the sprites 14, 16, 20, and 400
 *     ary = UIDS[14, 16, 20, 400]
 */
static mrb_value Index(mrb_state* p_state, mrb_value self)
{
    mrb_value* args = NULL;
    int count = 0;
    mrb_get_args(p_state, "*", &args, &count);

    mrb_value cache = mrb_iv_get(p_state, self, mrb_intern_cstr(p_state, "cache"));

    if (count == 0) { // No arguments, error
        mrb_raise(p_state, MRB_ARGUMENT_ERROR(p_state), "Wrong number of arguments, expected 1..n, got 0.");
        return mrb_nil_value(); // Not reached
    }
    else if (count == 1) { // One argument
        mrb_value arg = args[0];

        // C++ does not allow us to declare variables inside a `case:' :-(
        mrb_value start = mrb_nil_value();
        mrb_value end   = mrb_nil_value();
        mrb_value ary   = mrb_nil_value();

        switch (mrb_type(arg))  {
        case MRB_TT_FIXNUM: // Single UID requested
            return _Index(p_state, cache, arg);
        case MRB_TT_RANGE: // UID range requested
            start = mrb_funcall(p_state, arg, "first", 0);
            end   = mrb_funcall(p_state, arg, "last", 0);

            // Ensure we get an integer range, and not string or so
            if (mrb_type(start) != MRB_TT_FIXNUM || mrb_type(end) != MRB_TT_FIXNUM) {
                mrb_raise(p_state, MRB_TYPE_ERROR(p_state), "Invalid UID range type.");
                return mrb_nil_value(); // Not reached
            }

            ary = mrb_ary_new(p_state);
            for (mrb_int i=mrb_fixnum(start); i <= mrb_fixnum(end); i++)
                mrb_ary_push(p_state, ary, _Index(p_state, cache, mrb_fixnum_value(i)));

            return ary;
        default:
            mrb_raise(p_state, MRB_TYPE_ERROR(p_state), "Invalid UID type.");
            return mrb_nil_value(); // Not reached
        }
    }
    else { // n arguments -- list of UIDs requested
        mrb_value ary = mrb_ary_new(p_state);

        for (mrb_int i=0; i < count; i++) {
            mrb_value item = args[i];
            mrb_ary_push(p_state, ary, _Index(p_state, cache, mrb_to_int(p_state, item)));
        }

        return ary;
    }
}

/**
 * Method: UIDS::cache_size
 *
 *   cache_size() → an_integer
 *
 * The current size of the UID cache. This method is mainly
 * useful for debugging purposes.
 */
static mrb_value Cache_Size(mrb_state* p_state, mrb_value self)
{
    mrb_value keys = mrb_hash_keys(p_state, mrb_iv_get(p_state, self, mrb_intern_cstr(p_state, "cache")));
    return mrb_fixnum_value(RARRAY_LEN(keys));
}

/**
 * Method: UIDS::cached_uids
 *
 *   cached_uids() → an_array
 *
 * Returns an unsorted array of all UIDs currently cached.
 * This method is mainly useful for debugging purposes.
 */
static mrb_value Cached_UIDs(mrb_state* p_state, mrb_value self)
{
    return mrb_hash_keys(p_state, mrb_iv_get(p_state, self, mrb_intern_cstr(p_state, "cache")));
}

// FIXME: Call Scripting::Delete_UID_From_Cache for sprites
// being removed from a level’s cSprite_Manager!
void TSC::Scripting::Delete_UID_From_Cache(mrb_state* p_state, int uid)
{
    mrb_value cache = mrb_iv_get(p_state, mrb_obj_value(mrb_class_get(p_state, "UIDS")), mrb_intern_cstr(p_state, "cache"));
    mrb_hash_delete_key(p_state, cache, mrb_fixnum_value(uid));
}

void TSC::Scripting::Init_UIDS(mrb_state* p_state)
{
    struct RClass* p_rmUIDS = mrb_define_module(p_state, "UIDS");

    // Create a `cache' instance variable invisible from Ruby.
    // This is where the cached sprite instances will be stored,
    // visible for the GC.
    mrb_value cache = mrb_hash_new(p_state);
    mrb_iv_set(p_state, mrb_obj_value(p_rmUIDS), mrb_intern_cstr(p_state, "cache"), cache);

    // UID 0 is always the player
    mrb_hash_set(p_state, cache, mrb_fixnum_value(0), mrb_const_get(p_state, mrb_obj_value(p_state->object_class), mrb_intern_cstr(p_state, "Player")));

    mrb_define_class_method(p_state, p_rmUIDS, "[]", Index, MRB_ARGS_REQ(1));
    mrb_define_class_method(p_state, p_rmUIDS, "cache_size", Cache_Size, MRB_ARGS_NONE());
    mrb_define_class_method(p_state, p_rmUIDS, "cached_uids", Cached_UIDs, MRB_ARGS_NONE());
}

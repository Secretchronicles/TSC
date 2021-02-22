/***************************************************************************
 * mrb_sprite.cpp
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

#include "../../scripting.hpp"
#include "mrb_sprite.hpp"
#include "../mrb_eventable.hpp"
#include "../../events/event.hpp"
#include "../../../level/level.hpp"
#include "../../../core/sprite_manager.hpp"
#include "../../../core/property_helper.hpp"
#include "../../../level/level_player.hpp"
#include "../../../video/gl_surface.hpp"

/**
 * Class: Sprite
 *
 * A I<Sprite> is one of the most generic objects available in the MRuby
 * API. Anything shown on the screen is somehow a sprite, and the methods
 * defined in this class are therefore available to nearly all objects
 * exposed to the MRuby API.
 *
 * All sprites created by the regular TSC editor can be references by
 * indexing the global L<UIDS> table, see
 * the section “Unique Identifiers” on the main page for more
 * information on this topic.
 *
 *     UIDS[38].x
 *
 * =head2 About coordinates
 *
 * Instances of C<Sprite> and its subclasses have two kinds of
 * coordinates, the normal ones (usually called just "coordinates") and
 * the starting position’s coordinates (usually called "initial
 * coordinates"). Most sprites don’t really care about the initial
 * coordinates, but some instances of subclasses of C<Sprite> do, e.g. the
 * L<Flyon> remembers its starting position, i.e. where to
 * return after jumping out, in its initial coordinates. Note that the
 * method for specifying the initial coordinates I<also> set the
 * "normal" coordinates at the same time to the same value (so you
 * usually don’t need to set both pairs manually).
 *
 * =head2 Events
 *
 * =over
 *
 * =item [Touch]
 *
 * This event is fired when the sprite collides with another
 * sprite. Note that such a collision actually creates two Touch
 * events, one for each sprite. This may be useful if you don’t want to
 * use a long C<if-elsif-elsif-elsif...> construct inside an event
 * handler choosing an action depending on the collided sprite.
 *
 * The event handler gets passed an instance of this class (or one of
 * its subclasses) representing the other collision "partner".
 *
 * =back
 *
 * =head2 Constants
 *
 * =over
 *
 * =item [PASSIVE_Z_START]
 *
 * Lowest valid Z coordinate for passive sprites.
 *
 * =item [MASSIVE_Z_START]
 *
 * Lowest valid Z coordinate for massive sprites.
 *
 * =item [FRONTPASSIVE_Z_START]
 *
 * Lowest valid Z coordinate for front-passive sprites.
 *
 * =item [HALFMASSIVE_Z_START]
 *
 * Lowest valid Z coordinate for half-massive sprites.
 *
 * =back
 */

using namespace TSC;
using namespace TSC::Scripting;


MRUBY_IMPLEMENT_EVENT(touch);

/**
 * Method: Sprite::new
 *
 *   new( [path [, uid ] ] ) → a_sprite
 *
 * Adds a sprite to the level.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [path]
 *
 * The path to the sprite’s image, relative to the F<pixmaps/> directory.
 *
 * =item [uid]
 *
 * The UID for the sprite. If not given, a free one will be used.
 *
 * =back
 *
 * =head4 Return value
 *
 * The newly created instance.
 */

static mrb_value Initialize(mrb_state* p_state, mrb_value self)
{
    mrb_int uid = -1;
    char* path = NULL;
    mrb_get_args(p_state, "|zi", &path, &uid);

    // Insert a new sprite instance into the MRuby object
    cSprite* p_sprite = new cSprite(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_sprite;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    // Arguments
    if (path)
        p_sprite->Set_Image(pVideo->Get_Surface(utf8_to_path(path)), true);
    if (uid != -1) {
        if (pActive_Level->m_sprite_manager->Is_UID_In_Use(uid))
            mrb_raisef(p_state, MRB_ARGUMENT_ERROR(p_state), "UID %d is already used.", uid);

        p_sprite->m_uid = uid;
    }

    // Default massivity type is front passive
    p_sprite->Set_Massive_Type(MASS_FRONT_PASSIVE);

    // Hidden by default
    p_sprite->Set_Active(false);

    // This is a generated object that should neither be saved
    // nor should it be editable in the editor.
    p_sprite->Set_Spawned(true);

    // Add to the sprite manager for automatic memory management by TSC
    pActive_Level->m_sprite_manager->Add(p_sprite);

    return self;
}

/**
 * Method: Sprite#show
 *
 *   show()
 *
 * Makes a sprite visible. See also L<#hide>.
 */
mrb_value Show(mrb_state* p_state, mrb_value self)
{
    cSprite* p_sprite = Get_Data_Ptr<cSprite>(p_state, self);
    p_sprite->Set_Active(true);
    return mrb_nil_value();
}

/**
 * Method: Sprite#hide
 *
 *   hide()
 *
 * Makes a sprite invisible. See also L<#show>.
 */
mrb_value Hide(mrb_state* p_state, mrb_value self)
{
    cSprite* p_sprite = Get_Data_Ptr<cSprite>(p_state, self);
    p_sprite->Set_Active(false);
    return mrb_nil_value();
}

/**
 * Method: Sprite#uid
 *
 *   uid() → an_integer
 *
 * Returns the UID for the sprite.
 */
mrb_value Get_UID(mrb_state* p_state, mrb_value self)
{
    cSprite* p_sprite = Get_Data_Ptr<cSprite>(p_state, self);

    return mrb_fixnum_value(p_sprite->m_uid);
}

/**
 * Method: Sprite#massive_type=
 *
 *   massive_type=( type )
 *
 * Set the massivity of a sprite.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [type]
 *
 * One of the following symbols. Their meaning is identical to the one
 * in the TSC editor.
 *
 * =over
 *
 * =item C<:passive>
 *
 * =item C<:front_passive> or C<:frontpassive>
 *
 * =item C<:massive>
 *
 * =item C<:half_massive> or C<:halfmassive>
 *
 * =item C<:climbable>
 *
 * =back
 *
 * Invalid types will cause an error.
 *
 * =back
 */
static mrb_value Set_Massive_Type(mrb_state* p_state,  mrb_value self)
{
    cSprite* p_sprite = Get_Data_Ptr<cSprite>(p_state, self);
    mrb_sym typesym;
    std::string type;
    mrb_get_args(p_state, "n", &typesym);
    type = mrb_sym2name(p_state, typesym);

    if (type == "passive")
        p_sprite->Set_Massive_Type(MASS_PASSIVE);
    else if (type == "frontpassive" || type == "front_passive") // Official: "front_passive"
        p_sprite->Set_Massive_Type(MASS_FRONT_PASSIVE);
    else if (type == "massive")
        p_sprite->Set_Massive_Type(MASS_MASSIVE);
    else if (type == "halfmassive" || type == "half_massive") // Official: "halfmassive"
        p_sprite->Set_Massive_Type(MASS_HALFMASSIVE);
    else if (type == "climbable")
        p_sprite->Set_Massive_Type(MASS_CLIMBABLE);
    else // Non-standard types are not allowed here
        mrb_raisef(p_state, MRB_ARGUMENT_ERROR(p_state), "Invalid massive type '%s'.", type.c_str());

    return mrb_symbol_value(typesym);
}

/**
 * Method: Sprite#massive_type
 *
 *   massive_type() → a_symbol
 *
 * Returns the sprite’s current massive type. See L<#massive_type=> for
 * a list of possible return values; front passive will always be
 * returned as C<:frontpassive>, half massive will always be returned
 * as C<:half_massive>.
 */
static mrb_value Get_Massive_Type(mrb_state* p_state, mrb_value self)
{
    cSprite* p_sprite = Get_Data_Ptr<cSprite>(p_state, self);
    switch (p_sprite->m_massive_type) {
    case MASS_PASSIVE:
        return str2sym(p_state, "passive");
    case MASS_FRONT_PASSIVE:
        return str2sym(p_state, "frontpassive");
    case MASS_MASSIVE:
        return str2sym(p_state, "massive");
    case MASS_CLIMBABLE:
        return str2sym(p_state, "climbable");
    default:
        return mrb_nil_value();
    }
}

/**
 * Method: Sprite#x
 *
 *   x() → an_integer
 *
 * The current X coordinate.
 */
mrb_value Get_X(mrb_state* p_state, mrb_value self)
{
    cSprite* p_sprite = Get_Data_Ptr<cSprite>(p_state, self);

    return mrb_fixnum_value(p_sprite->m_pos_x);
}

/**
 * Method: Sprite#y
 *
 *   y() → an_integer
 *
 * The current Y coordinate.
 */
mrb_value Get_Y(mrb_state* p_state, mrb_value self)
{
    cSprite* p_sprite = Get_Data_Ptr<cSprite>(p_state, self);

    return mrb_fixnum_value(p_sprite->m_pos_y);
}

/**
 * Method: Sprite#x=
 *
 *   x=( val )
 *
 * Sets a new X coordinate.
 */
mrb_value Set_X(mrb_state* p_state, mrb_value self)
{
    cSprite* p_sprite = Get_Data_Ptr<cSprite>(p_state, self);
    mrb_int x;
    mrb_get_args(p_state, "i", &x);

    p_sprite->Set_Pos_X(x);

    return mrb_fixnum_value(x);
}

/**
 * Method: Sprite#y=
 *
 *   y=( val )
 *
 * Sets a new Y coordinate.
 */
mrb_value Set_Y(mrb_state* p_state, mrb_value self)
{
    cSprite* p_sprite = Get_Data_Ptr<cSprite>(p_state, self);
    mrb_int y;
    mrb_get_args(p_state, "i", &y);

    p_sprite->Set_Pos_Y(y);

    return mrb_fixnum_value(y);
}

/**
 * Method: Sprite#start_x
 *
 *   start_x()
 *
 * Returns the sprite’s initial X coordinate.
 */
static mrb_value Get_Start_X(mrb_state* p_state,  mrb_value self)
{
    cSprite* p_sprite = Get_Data_Ptr<cSprite>(p_state, self);

    return mrb_fixnum_value(p_sprite->m_start_pos_x);
}

/**
 * Method: Sprite#start_y
 *
 *   start_y()
 *
 * Returns the sprite’s initial Y coordinate.
 */
static mrb_value Get_Start_Y(mrb_state* p_state,  mrb_value self)
{
    cSprite* p_sprite = Get_Data_Ptr<cSprite>(p_state, self);

    return mrb_fixnum_value(p_sprite->m_start_pos_y);
}

/**
 * Method: Sprite#start_x=
 *
 *   start_x=( val )
 *
 * Like L<#x=>, but also sets the sprite’s initial X coordinate.
 */
static mrb_value Set_Start_X(mrb_state* p_state, mrb_value self)
{
    cSprite* p_sprite = Get_Data_Ptr<cSprite>(p_state, self);
    mrb_int start_x;
    mrb_get_args(p_state, "i", &start_x);

    p_sprite->Set_Pos_X(start_x, true);

    return mrb_fixnum_value(start_x);
}

/**
 * Method: Sprite#start_y=
 *
 *   start_y=( val )
 *
 * Like L<#y=>, but also sets the sprite’s initial Y coordinate.
 */
static mrb_value Set_Start_Y(mrb_state* p_state, mrb_value self)
{
    cSprite* p_sprite = Get_Data_Ptr<cSprite>(p_state, self);
    mrb_int start_y;
    mrb_get_args(p_state, "i", &start_y);

    p_sprite->Set_Pos_Y(start_y, true);

    return mrb_fixnum_value(start_y);
}

/**
 * Method: Sprite#z
 *
 *   z() → an_integer
 *
 * Returns the current Z coordinate. Note you cannot set the Z
 * coordinate.
 */
static mrb_value Get_Z(mrb_state* p_state, mrb_value self)
{
    cSprite* p_sprite = Get_Data_Ptr<cSprite>(p_state, self);

    return mrb_fixnum_value(p_sprite->m_pos_z);
}

/**
 * Method: Sprite#pos
 *
 *   pos() → [an_integer, another_integer]
 *
 * Returns the sprite’s current X and Y coordinates.
 *
 * =head4 Return value
 *
 * An array of the current coordinates, both in pixels.
 */
static mrb_value Pos(mrb_state* p_state, mrb_value self)
{
    cSprite* p_sprite = Get_Data_Ptr<cSprite>(p_state, self);

    mrb_value result = mrb_ary_new(p_state);
    mrb_ary_push(p_state, result, mrb_fixnum_value(p_sprite->m_pos_x));
    mrb_ary_push(p_state, result, mrb_fixnum_value(p_sprite->m_pos_y));

    return result;
}

/**
 * Method: Sprite#start_pos
 *
 *   start_pos() → [an_integer, another_integer]
 *
 * Returns the initial coordinates for this sprite.
 *
 * =head4 Return value
 *
 * An array of the initial coordinates, both in pixels.
 */
static mrb_value Start_Pos(mrb_state* p_state, mrb_value self)
{
    cSprite* p_sprite = Get_Data_Ptr<cSprite>(p_state, self);

    mrb_value result = mrb_ary_new(p_state);
    mrb_ary_push(p_state, result, mrb_fixnum_value(p_sprite->m_start_pos_x));
    mrb_ary_push(p_state, result, mrb_fixnum_value(p_sprite->m_start_pos_y));

    return result;
}

/**
 * Method: Sprite#rect
 *
 *   rect() → [an_integer, another_integer, yetanother_integer, andafourthinteger]
 *
 * The sprite’s full image rectangle. See also L<#collision_rect>.
 */
static mrb_value mRect(mrb_state* p_state, mrb_value self) // Note: Calling this function Rect() causes a compilation error on MacOS due to a symbol conflict, thus name it mRect()
{
    cSprite* p_sprite = Get_Data_Ptr<cSprite>(p_state, self);

    mrb_value result = mrb_ary_new(p_state);
    mrb_ary_push(p_state, result, mrb_fixnum_value(p_sprite->m_rect.m_x));
    mrb_ary_push(p_state, result, mrb_fixnum_value(p_sprite->m_rect.m_y));
    mrb_ary_push(p_state, result, mrb_fixnum_value(p_sprite->m_rect.m_w));
    mrb_ary_push(p_state, result, mrb_fixnum_value(p_sprite->m_rect.m_h));

    return result;
}

/**
 * Method: Sprite#collision_rect
 *
 *   collision_rect() → [an_integer, another_integer, yetanother_integer, andafourthinteger]
 *
 * The sprite’s collision rectangle. See also L<#rect>.
 */
static mrb_value Collision_Rect(mrb_state* p_state, mrb_value self)
{
    cSprite* p_sprite = Get_Data_Ptr<cSprite>(p_state, self);

    mrb_value result = mrb_ary_new(p_state);
    mrb_ary_push(p_state, result, mrb_fixnum_value(p_sprite->m_col_rect.m_x));
    mrb_ary_push(p_state, result, mrb_fixnum_value(p_sprite->m_col_rect.m_y));
    mrb_ary_push(p_state, result, mrb_fixnum_value(p_sprite->m_col_rect.m_w));
    mrb_ary_push(p_state, result, mrb_fixnum_value(p_sprite->m_col_rect.m_h));

    return result;
}

/**
 * Method: Sprite#warp
 *
 *   warp( new_x , new_y )
 *
 * Warp the sprite somewhere. Note you are responsible for ensuring the
 * coordinates are valid, this method behaves exactly as a level entry
 * (i.e. doesn’t check coordinate validness).
 *
 * You can easily get the coordinates by moving around the cursor in
 * the TSC level editor and hovering over objects placed near the
 * location where you want to warp to.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [x] The new X coordinate.
 *
 * =item [y] The new Y coordinate.
 *
 * =back
 */
static mrb_value Warp(mrb_state* p_state, mrb_value self)
{
    cSprite* p_sprite = Get_Data_Ptr<cSprite>(p_state, self);
    mrb_int x;
    mrb_int y;
    mrb_get_args(p_state, "ii", &x, &y);

    p_sprite->Set_Pos(x, y);

    return mrb_nil_value();
}

/**
 * Method: Sprite#start_at
 *
 *   start_at( xpos, ypos )
 *
 * Sets both the initial X and Y coordinates at once.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [xpos] The initial X coordinate in pixels.
 *
 * =item [ypos] The initial Y coordinate in pixels.
 *
 * =back
 */
static mrb_value Start_At(mrb_state* p_state, mrb_value self)
{
    cSprite* p_sprite = Get_Data_Ptr<cSprite>(p_state, self);
    mrb_int start_x;
    mrb_int start_y;
    mrb_get_args(p_state, "ii", &start_x, &start_y);

    p_sprite->Set_Pos(start_x, start_y, true);

    return mrb_nil_value();
}

/**
 * Method: Sprite#suppress_save=
 *
 *   suppress_save=(a_boolean) → a_boolean
 *
 * Enable this to prevent this sprite from being saved in
 * savegame files; it is disabled by default. This functionality
 * is useful in certain scripting situations, where without it
 * generates sprites would be saved into the savegame that should
 * actually only ever be inserted by the game's script.
 *
 * =head4 Return value
 *
 * Returns its argument.
 */
static mrb_value Set_Suppress_Save(mrb_state* p_state, mrb_value self)
{
    cSprite* p_sprite = Get_Data_Ptr<cSprite>(p_state, self);
    mrb_bool suppress;
    mrb_get_args(p_state, "b", &suppress);

    p_sprite->Set_Suppress_Save(suppress);
    return mrb_bool_value(suppress);
}

/**
 * Method: Sprite#suppress_save
 *
 *   suppress_save() → a_boolean
 *
 * Queries whether this sprite is saved in savegame files.
 * See L<#suppress_save=>.
 *
 * =head4 Return value
 *
 * True or false.
 */
static mrb_value Get_Suppress_Save(mrb_state* p_state, mrb_value self)
{
    cSprite* p_sprite = Get_Data_Ptr<cSprite>(p_state, self);

    if (p_sprite->m_suppress_save)
        return mrb_true_value();
    else
        return mrb_false_value();
}

/**
 * Method: Sprite#player?
 *
 *   player?() → a_boolean
 *
 * Checks whether this sprite is the player.
 *
 * =head4 Return value
 *
 * If this sprite is the player, returns C<true>. Otherwise, returns
 * C<false>.
 */
static mrb_value Is_Player(mrb_state* p_state,  mrb_value self)
{
    cSprite* p_sprite = Get_Data_Ptr<cSprite>(p_state, self);

    if (p_sprite == pLevel_Player)
        return mrb_true_value();
    else
        return mrb_false_value();
}

/**
 * Method: Sprite#image=
 *
 *   image=( path ) → path
 *
 * Change the sprite’s image to the given one.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [path]
 *
 * The path to the new image, relative to the F<pixmaps/> directory.
 *
 * =back
 */
static mrb_value Set_Image(mrb_state* p_state, mrb_value self)
{
    char* path = NULL;
    mrb_get_args(p_state, "z", &path);

    cSprite* p_sprite = Get_Data_Ptr<cSprite>(p_state, self);
    p_sprite->Set_Image(pVideo->Get_Surface(utf8_to_path(path), true));

    return mrb_str_new_cstr(p_state, path);
}

/**
 * Method: Sprite#image
 *
 *   image() → a_string or nil
 *
 * Returns the path to the sprite’s current image. If the sprite was
 * not constructed from a file, returns C<nil>.
 */
static mrb_value Get_Image(mrb_state* p_state, mrb_value self)
{
    cSprite* p_sprite = Get_Data_Ptr<cSprite>(p_state, self);
    boost::filesystem::path imgpath = p_sprite->m_start_image->Get_Path();

    if (imgpath.empty())
        return mrb_nil_value();
    else
        return mrb_str_new_cstr(p_state, path_to_utf8(imgpath).c_str());
}

/**
 * Method: Sprite#active=
 *
 *   active=( bool ) → bool
 *
 * TODO: Docs
 */
static mrb_value Set_Active(mrb_state* p_state, mrb_value self)
{
    mrb_bool status;
    mrb_get_args(p_state, "b", &status);
    cSprite* p_sprite = Get_Data_Ptr<cSprite>(p_state, self);
    p_sprite->Set_Active(status);

    return mrb_bool_value(status);
}

/**
 * Method: Sprite#active?
 *
 *   active?() → true or false
 *
 * TODO: Docs
 */
static mrb_value Is_Active(mrb_state* p_state, mrb_value self)
{
    cSprite* p_sprite = Get_Data_Ptr<cSprite>(p_state, self);
    return mrb_bool_value(p_sprite->m_active);
}

void TSC::Scripting::Init_Sprite(mrb_state* p_state)
{
    struct RClass* p_rcSprite = mrb_define_class(p_state, "Sprite", p_state->object_class);
    mrb_include_module(p_state, p_rcSprite, mrb_module_get(p_state, "Eventable"));
    MRB_SET_INSTANCE_TT(p_rcSprite, MRB_TT_DATA);

    mrb_define_const(p_state, p_rcSprite, "PASSIVE_Z_START", mrb_float_value(p_state, cSprite::m_pos_z_passive_start));
    mrb_define_const(p_state, p_rcSprite, "MASSIVE_Z_START", mrb_float_value(p_state, cSprite::m_pos_z_massive_start));
    mrb_define_const(p_state, p_rcSprite, "FRONTPASSIVE_Z_START", mrb_float_value(p_state, cSprite::m_pos_z_front_passive_start));
    mrb_define_const(p_state, p_rcSprite, "HALFMASSIVE_Z_START", mrb_float_value(p_state, cSprite::m_pos_z_halfmassive_start));

    mrb_define_method(p_state, p_rcSprite, "initialize", Initialize, MRB_ARGS_OPT(2));
    mrb_define_method(p_state, p_rcSprite, "show", Show, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcSprite, "hide", Hide, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcSprite, "uid", Get_UID, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcSprite, "massive_type=", Set_Massive_Type, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcSprite, "massive_type", Get_Massive_Type, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcSprite, "x", Get_X, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcSprite, "y", Get_Y, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcSprite, "x=", Set_X, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcSprite, "y=", Set_Y, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcSprite, "z", Get_Z, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcSprite, "start_x", Get_Start_X, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcSprite, "start_y", Get_Start_Y, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcSprite, "start_x=", Set_Start_X, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcSprite, "start_y=", Set_Start_Y, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcSprite, "pos", Pos, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcSprite, "start_pos", Start_Pos, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcSprite, "rect", mRect, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcSprite, "collision_rect", Collision_Rect, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcSprite, "warp", Warp, MRB_ARGS_REQ(2));
    mrb_define_method(p_state, p_rcSprite, "start_at", Start_At, MRB_ARGS_REQ(2));
    mrb_define_method(p_state, p_rcSprite, "player?", Is_Player, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcSprite, "image", Get_Image, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcSprite, "image=", Set_Image, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcSprite, "active=", Set_Active, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcSprite, "active?", Is_Active, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcSprite, "suppress_save=", Set_Suppress_Save, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcSprite, "suppress_save", Get_Suppress_Save, MRB_ARGS_NONE());

    mrb_define_method(p_state, p_rcSprite, "on_touch", MRUBY_EVENT_HANDLER(touch), MRB_ARGS_NONE());
}

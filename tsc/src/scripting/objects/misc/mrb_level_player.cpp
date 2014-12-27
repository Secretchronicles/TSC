/***************************************************************************
 * mrb_level_player.cpp
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

#include "mrb_level_player.hpp"
#include "../sprites/mrb_animated_sprite.hpp"
#include "../../../level/level_player.hpp"
#include "../../../gui/hud.hpp"
#include "../../events/event.hpp"
#include "../../../core/global_basic.hpp"

using namespace TSC;
using namespace TSC::Scripting;
using namespace std;

/**
 * Class: LevelPlayer
 * Parent: [AnimatedSprite](animatedsprite.html)
 * {: .superclass}
 *
 * The sole instance of this class, the singleton `Player`, represents
 * Alex himself. Naturally you can’t instanciate this class (TSC isn’t a
 * multiplayer game), but otherwise this class is your interface to doing
 * all kinds of evil things with Alex. You should, however, be careful,
 * because the powerful methods exposed by this class allow you to
 * control nearly every aspect of Alex--if you exaggerate, the player
 * will get annoyed, probably stopping to play your level.
 *
 * This class’ documentation uses two words that you’re better off not
 * mixing up:
 *
 * Alex (or just "the level player")
 * : The sprite of Alex walking around and jumping on enemies.
 *
 * Player
 * : The actual user sitting in front of some kind of monitor. Don’t
 *   confuse him with Alex, because a) he will probably get angry, and b)
 *   you’ll get funny meanings on sentences like "the player presses the
 *   _jump_ key". The only exception to this rule is the `Player`
 *   constant in Mruby, which obviously represents Alex, not the guy
 *   playing the game.
 *
 * Note that the level player is just a normal sprite all its way up
 * through the class hierarchy. So be sure to check the superclasses’
 * methods if you don’t find what you’re looking for here.
 *
 * Events
 * ------
 *
 * Downgrade
 * : Whenever Alex gets hit (but not killed), this event is triggered.
 *   The event handler gets passed Alex’s current downgrade count
 *   (which is always 1) and Alex’s maximum downgrade count (which
 *   is always 2). As you can see, the arguments passed are not really
 *   useful and are just there for symmetry with some enemies’
 *   _Downgrade_ event handlers.
 *
 * Jewel_100
 * : After Alex has collected 100 jewels, this event
 *   is triggered. The event handler isn’t passed anything, but note
 *   that it is highly discouraged to alter Alex’s amount of jewels from
 *   within the event handler; this may lead to unexpected behaviour
 *   such as Alex having more than 100 jewels after all operations
 *   regarding the amount of jewels have finished or even endless
 *   loops as altering the jewel amount may cause subsequent events
 *   of this type to be triggered.
 *
 * Jump
 * : This event is issued when the Alex does a valid jump, i.e. the
 *   player presses the _Jump_ key and Alex is currently in a state that
 *   actually allows him to jump. The event is triggered immediately
 *   before starting the jump.
 *
 * Shoot
 * : Fired when Alex executes a valid shoot, either fireball or
 *   iceball. As with the _Jump_ event, this is only triggered when the
 *   player presses the _Shoot_ key and Alex is currently in a state
 *   that allows him to shoot. Likewise, the event is triggered just
 *   prior to the actual shot. The event handler gets passed either the
 *   string `"ice"` when the player fired an iceball, or `"fire"` when it
 *   was a fireball.
 */

/***************************************
 * Events
 ***************************************/

MRUBY_IMPLEMENT_EVENT(jewel_100);
MRUBY_IMPLEMENT_EVENT(downgrade);
MRUBY_IMPLEMENT_EVENT(jump);
MRUBY_IMPLEMENT_EVENT(shoot);

/***************************************
 * Methods
 ***************************************/

/**
 * Method: LevelPlayer#jump
 *
 *   jump( [ deaccel ] )
 *
 * Makes Alex jump.
 *
 * #### Parameter
 * deaccel
 * : Negative acceleration to apply, i.e. defines how high Alex will
 *   jump. Note that this isn’t necessarily the height in pixels as the
 *   force of gravity will be applied to the value while jumping.
 */
static mrb_value Jump(mrb_state* p_state,  mrb_value self)
{
    mrb_int deaccel = -1;
    mrb_get_args(p_state, "|i", &deaccel);

    if (deaccel > 0)
        pLevel_Player->Start_Jump(deaccel);
    else
        pLevel_Player->Start_Jump();

    return mrb_nil_value();
}

/**
 * Method: LevelPlayer#type
 *
 *   type() → a_symbol or nil
 *
 * Returns Alex’s current type. See [#type=](#type-1) for a list of
 * possible symbols to be returned. Returns nil if Alex’s state can’t
 * be detected for some reason (and prints a warning on stderr).
 */
static mrb_value Get_Type(mrb_state* p_state,  mrb_value self)
{
    switch (pLevel_Player->m_maryo_type) {
    case MARYO_DEAD:
        return str2sym(p_state, "dead");
    case MARYO_SMALL:
        return str2sym(p_state, "small");
    case MARYO_BIG:
        return str2sym(p_state, "big");
    case MARYO_FIRE:
        return str2sym(p_state, "fire");
    case MARYO_ICE:
        return str2sym(p_state, "ice");
    //case MARYO_CAPE:
    //  return str2sym(p_state, "cape"); // Not implemented officially in TSC
    case MARYO_GHOST:
        return str2sym(p_state, "ghost");
    default:
        std::cerr << "Warning: Invalid Alex state: " << pLevel_Player->m_maryo_type << std::endl;
        return mrb_nil_value();
    }
}

/**
 * Method: LevelPlayer#type=
 *
 *   type=(type)
 *
 * Forcibly applies a powerup/powerdown to the level player. Note this method
 * bypasses any Alex state checks, i.e. you can directly apply `:ice` to
 * small Alex or force Fire Alex back to Normal Big Alex by applying
 * `:big`. This check bypassing is the reason why you shouldn’t use this
 * method for downgrading or killing the player; there might however be
 * situations in which calling this method is more appropriate.
 *
 * Using this method never affects the rescue item (that one shown on top
 * of the screen in the box).
 *
 * #### Parameter
 * type
 * : The powerup or powerdown to apply. One of the following symbols:
 *
 *   `:dead`
 *    : Please use the [kill()](#kill) method instead.
 *
 *   `:small`
 *    : Please use the [downgrade()](#downgrade) method instead.
 *
 *   `:big`
 *   : Apply the normal mushroom.
 *
 *   `:fire`
 *   : Apply the fireplant.
 *
 *   `:ice`
 *   : Apply the ice mushroom.
 *
 *   `:ghost`
 *   : Apply the ghost mushroom.
 *
 *   Specifying an invalid type causes an error.
 *
 */
static mrb_value Set_Type(mrb_state* p_state,  mrb_value self)
{
    mrb_sym sym;
    mrb_get_args(p_state, "n", &sym);
    const char* typestr = mrb_sym2name(p_state, sym);
    Maryo_type type;

    if (strcmp(typestr, "dead") == 0)
        type = MARYO_DEAD;
    else if (strcmp(typestr, "small") == 0)
        type = MARYO_SMALL;
    else if (strcmp(typestr, "big") == 0)
        type = MARYO_BIG;
    else if (strcmp(typestr, "fire") == 0)
        type = MARYO_FIRE;
    else if (strcmp(typestr, "ice") == 0)
        type = MARYO_ICE;
    //else if (strcmp(typestr, "cape") == 0) // Not implemented officially by TSC
    //  type = MARYO_CAPE;
    else if (strcmp(typestr, "ghost") == 0)
        type = MARYO_GHOST;
    else {
        mrb_raisef(p_state, MRB_ARGUMENT_ERROR(p_state), "Invalid Alex type '%s'.", typestr);
        return mrb_nil_value();
    }

    pLevel_Player->Set_Type(type);
    return mrb_symbol_value(sym);
}

/**
 * Method: LevelPlayer#get_points
 *
 *   points() → an_integer
 *
 * Returns the number of points the player currently has.
 */
static mrb_value Get_Points(mrb_state* p_state,  mrb_value self)
{
    return mrb_fixnum_value(pLevel_Player->m_points);
}

/**
 * Method: LevelPlayer#set_points
 *
 *   points=(points)
 *
 * Reset the player’s points to the given value. You probably don’t want
 * to do this.
 */
static mrb_value Set_Points(mrb_state* p_state,  mrb_value self)
{
    mrb_int points;
    mrb_get_args(p_state, "i", &points);

    pHud_Points->Set_Points(points);
    return mrb_fixnum_value(points);
}

/**
 * Method: LevelPlayer#add_points
 *
 *   add_points( points ) → an_integer
 *
 * Adds more points to the amount of points the player already has.
 *
 * #### Parameter
 * points
 * : The number of points to add.
 *
 * #### Return value
 * The new number of points.
 */
static mrb_value Add_Points(mrb_state* p_state,  mrb_value self)
{
    mrb_int points;
    mrb_get_args(p_state, "i", &points);

    /* X and Y positions, multipliers, etc. are intended to be used
     * with enemies, not direct point increasing, so I don’t provide
     * MRuby bindings for those parameters here. */
    pHud_Points->Add_Points(points);
    return mrb_fixnum_value(pLevel_Player->m_points);
}


/**
 * Method: LevelPlayer#kill
 *
 *   kill()
 *
 * Forcibly kill the level player. This method kills Alex
 * regardless of being big, fire, etc, but still honours
 * star and other invincibility effects. If you urgently
 * want to kill Alex despite of those, use `#kill!`.
 */
static mrb_value Kill(mrb_state* p_state, mrb_value self)
{
    pLevel_Player->DownGrade_Player(true, true);
    return mrb_nil_value();
}

/**
 * Method: LevelPlayer#kill!
 *
 *   kill!()
 *
 * Like `#kill`, but also ignore any invincibility status that
 * Alex may be in. That is, even kill Alex if he has just been
 * hurt and is thus invincible or despite star being in effect.
 */
static mrb_value Forced_Kill(mrb_state* p_state, mrb_value self)
{
    pLevel_Player->DownGrade_Player(true, true, true);
    return mrb_nil_value();
}

/**
 * Method: LevelPlayer#jewels
 *
 *   jewels() → an_integer
 *
 * The current amount of jewels Alex has collected so
 * far. This is always smaller than 100.
 */
static mrb_value Get_Jewels(mrb_state* p_state,  mrb_value self)
{
    return mrb_fixnum_value(pLevel_Player->m_goldpieces);
}

/**
 * Method: LevelPlayer#jewels=
 *
 *   jewels=(num)
 *
 * Reset the number of collected jewels to the given
 * value. If you set a value greater than 100, a `Jewel_100` event is
 * triggered.
 *
 * #### Parameter
 * num
 * : The new number of jewels. This value obeys the same
 *   100-rule as the parameter to [add_jewels()](#addjewels).
 */
static mrb_value Set_Jewels(mrb_state* p_state,  mrb_value self)
{
    mrb_int jewels;
    mrb_get_args(p_state, "i", &jewels);

    pHud_Goldpieces->Set_Gold(jewels);
    return mrb_fixnum_value(jewels);
}

/**
 * Method: LevelPlayer#add_jewels
 *
 *   add_jewels(num)    → an_integer
 *
 * Add to the player’s current amount of jewels. If the number of
 * jewels passes 100, a `Jewel_100` event is triggered.
 *
 * #### Parameter
 * num
 * : The number of jewels to add. If Alex’s resulting
 *   amount of jewels (i.e. the current amount plus `num`)
 *   is greater than 99, Alex gains a life and 100 is subtracted
 *   from the resulting amount. This process is repeated until the total
 *   resulting amount of jewels is not greater than 99.
 *
 * #### Return value
 * The new amount of jewels (after the 100-rule described
 * above has been applied as often as necessary).
 */
static mrb_value Add_Jewels(mrb_state* p_state,  mrb_value self)
{
    mrb_int jewels;
    mrb_get_args(p_state, "i", &jewels);

    pHud_Goldpieces->Add_Gold(jewels);
    return mrb_fixnum_value(pLevel_Player->m_goldpieces);
}

/**
 * Method: LevelPlayer#lives
 *
 *   lives() → an_integer
 *
 * Returns the number of lives the player has. Note this may
 * be zero.
 */
static mrb_value Get_Lives(mrb_state* p_state,  mrb_value self)
{
    return mrb_fixnum_value(pLevel_Player->m_lives);
}

/**
 * Method: LevelPlayer#lives=
 *
 *   lives=(lives)
 *
 * Reset Alex’s number of lives to the given value.
 *
 * #### Parameter
 * lives
 * : The new number of lives. This number may be negative, but note that
 *   setting lives to 0 or less doesn’t kill the player immediately as
 *   this number is only checked when the player gets killed by some other
 *   force.
 */
static mrb_value Set_Lives(mrb_state* p_state,  mrb_value self)
{
    mrb_int lives;
    mrb_get_args(p_state, "i", &lives);

    pHud_Lives->Set_Lives(lives);

    return mrb_fixnum_value(lives);
}


/**
 * Method: LevelPlayer#add_lives
 *
 *   add_lives( lives ) → an_integer
 *
 * Add to the player’s current number of lives.
 *
 * #### Parameter
 * lives
 * : The lives to add. This number may be negative, but note that setting
 *   lives to 0 or less doesn’t kill the player immediately as this
 *   number is only checked when the player gets killed by some other
 *   force.
 *
 * #### Return value
 * The number of lives Alex now has.
 */
static mrb_value Add_Lives(mrb_state* p_state, mrb_value self)
{
    mrb_int lives;
    mrb_get_args(p_state, "i", &lives);

    pHud_Lives->Add_Lives(lives);
    return mrb_fixnum_value(pLevel_Player->m_lives);
}

/**
 *  Method: LevelPlayer#invincible?
 *
 *   invincible?() → true or false
 *
 * Checks whether Alex currently cannot be defeated. This
 * usually means star is in effect.
 */
static mrb_value Is_Invincible(mrb_state* p_state, mrb_value self)
{
    if (pLevel_Player->m_invincible >= 0.1f)
        return mrb_true_value();
    else
        return mrb_false_value();
}

/**
 * Method: LevelPlayer#release_item
 *
 *   release_item()
 *
 * If Alex currently carries something (shell), he will drop it.
 * Otherwise does nothing.
 */
static mrb_value Release_Item(mrb_state* p_state, mrb_value self)
{
    pLevel_Player->Release_Item();
    return mrb_nil_value();
}

/***************************************
 * Entry point
 ***************************************/

void TSC::Scripting::Init_Level_Player(mrb_state* p_state)
{
    struct RClass* p_rcLevel_Player = mrb_define_class(p_state, "LevelPlayer", mrb_class_get(p_state, "AnimatedSprite"));
    MRB_SET_INSTANCE_TT(p_rcLevel_Player, MRB_TT_DATA);

    // Make the Player global the only instance of LevelPlayer
    mrb_define_const(p_state, p_state->object_class, "Player", pLevel_Player->Create_MRuby_Object(p_state));

    // Forbid creating new instances of LevelPlayer
    mrb_undef_class_method(p_state, p_rcLevel_Player, "new");

    // Normal methods
    mrb_define_method(p_state, p_rcLevel_Player, "type", Get_Type, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel_Player, "jump", Jump, MRB_ARGS_OPT(1));
    mrb_define_method(p_state, p_rcLevel_Player, "type=", Set_Type, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcLevel_Player, "points", Get_Points, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel_Player, "points=", Set_Points, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcLevel_Player, "add_points", Add_Points, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcLevel_Player, "kill", Kill, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel_Player, "kill!", Forced_Kill, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel_Player, "jewels", Get_Jewels, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel_Player, "jewels=", Set_Jewels, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcLevel_Player, "add_jewels", Add_Jewels, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcLevel_Player, "lives", Get_Lives, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel_Player, "lives=", Set_Lives, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcLevel_Player, "add_lives", Add_Lives, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcLevel_Player, "invincible?", Is_Invincible, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel_Player, "release_item", Release_Item, MRB_ARGS_NONE());

    // Event handlers
    mrb_define_method(p_state, p_rcLevel_Player, "on_jewel_100", MRUBY_EVENT_HANDLER(jewel_100), MRB_ARGS_BLOCK());
    mrb_define_method(p_state, p_rcLevel_Player, "on_downgrade", MRUBY_EVENT_HANDLER(downgrade), MRB_ARGS_BLOCK());
    mrb_define_method(p_state, p_rcLevel_Player, "on_jump", MRUBY_EVENT_HANDLER(jump), MRB_ARGS_BLOCK());
    mrb_define_method(p_state, p_rcLevel_Player, "on_shoot", MRUBY_EVENT_HANDLER(shoot), MRB_ARGS_BLOCK());
}

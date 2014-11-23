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

using namespace TSC;
using namespace TSC::Scripting;


/**
 * Class: LevelPlayer
 * Parent: [AnimatedSprite](animatedsprite.html)
 * {: .superclass}
 *
 * The sole instance of this class, the singleton `Player`, represents
 * Maryo himself. Naturally you can’t instanciate this class (TSC isn’t a
 * multiplayer game), but otherwise this class is your interface to doing
 * all kinds of evil things with Maryo. You should, however, be careful,
 * because the powerful methods exposed by this class allow you to
 * control nearly every aspect of Maryo--if you exaggerate, the player
 * will get annoyed, probably stopping to play your level.
 *
 * This class’ documentation uses two words that you’re better off not
 * mixing up:
 *
 * Maryo (or just "the level player")
 * : The sprite of Maryo walking around and jumping on enemies.
 *
 * Player
 * : The actual user sitting in front of some kind of monitor. Don’t
 *   confuse him with Maryo, because a) he will probably get angry, and b)
 *   you’ll get funny meanings on sentences like "the player presses the
 *   _jump_ key". The only exception to this rule is the `Player`
 *   constant in Mruby, which obviously represents Maryo, not the guy
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
 * : Whenever Maryo gets hit (but not killed), this event is triggered.
 *   The event handler gets passed Maryo’s current downgrade count
 *   (which is always 1) and Maryo’s maximum downgrade count (which
 *   is always 2). As you can see, the arguments passed are not really
 *   useful and are just there for symmetry with some enemies’
 *   _Downgrade_ event handlers.
 *
 * Gold_100
 * : After Maryo has collected 100 gold pieces/waffles, this event
 *   is triggered. The event handler isn’t passed anything, but note
 *   that it is highly discouraged to alter Maryo’s amount of gold from
 *   within the event handler; this may lead to unexpected behaviour
 *   such as Maryo having more than 100 gold pieces after all operations
 *   regarding the amount of gold/waffles have finished or even endless
 *   loops as altering the gold/waffle amount may cause subsequent events
 *   of this type to be triggered.
 *
 * Jump
 * : This event is issued when the Maryo does a valid jump, i.e. the
 *   player presses the _Jump_ key and Maryo is currently in a state that
 *   actually allows him to jump. The event is triggered immediately
 *   before starting the jump.
 *
 * Shoot
 * : Fired when Maryo executes a valid shoot, either fireball or
 *   iceball. As with the _Jump_ event, this is only triggered when the
 *   player presses the _Shoot_ key and Maryo is currently in a state
 *   that allows him to shoot. Likewise, the event is triggered just
 *   prior to the actual shot. The event handler gets passed either the
 *   string `"ice"` when the player fired an iceball, or `"fire"` when it
 *   was a fireball.
 *
 * Waffles_100
 * : Synonym for the `Gold_100` event.
 */

/***************************************
 * Events
 ***************************************/

MRUBY_IMPLEMENT_EVENT(gold_100);
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
 * Makes Maryo jump.
 *
 * #### Parameter
 * deaccel
 * : Negative acceleration to apply, i.e. defines how high Maryo will
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
 * Returns Maryo’s current type. See [#type=](#type-1) for a list of
 * possible symbols to be returned. Returns nil if Maryo’s state can’t
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
        std::cerr << "Warning: Invalid Maryo state: " << pLevel_Player->m_maryo_type << std::endl;
        return mrb_nil_value();
    }
}

/**
 * Method: LevelPlayer#type=
 *
 *   type=(type)
 *
 * Forcibly applies a powerup/powerdown to the level player. Note this method
 * bypasses any Maryo state checks, i.e. you can directly apply `:ice` to
 * small Maryo or force Fire Maryo back to Normal Big Maryo by applying
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
        mrb_raisef(p_state, MRB_ARGUMENT_ERROR(p_state), "Invalid Maryo type '%s'.", typestr);
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
 * Forcibly kill the level player. This method kills Maryo
 * regardless of being big, fire, etc, but still honours
 * star and other invincibility effects. If you urgently
 * want to kill Maryo despite of those, use `#kill!`.
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
 * Maryo may be in. That is, even kill Maryo if he has just been
 * hurt and is thus invincible or despite star being in effect.
 */
static mrb_value Forced_Kill(mrb_state* p_state, mrb_value self)
{
    pLevel_Player->DownGrade_Player(true, true, true);
    return mrb_nil_value();
}

/**
 * Method: LevelPlayer#gold
 *
 *   gold() → an_integer
 *   waffles() an_integer
 *
 * The current amount of gold pieces/waffles Maryo has collected so
 * far. This is always smaller than 100.
 */
/**
 * Method: LevelPlayer#waffles
 *
 *   gold() → an_integer
 *   waffles() an_integer
 *
 * Alias for [#gold](#gold).
 */
static mrb_value Get_Gold(mrb_state* p_state,  mrb_value self)
{
    return mrb_fixnum_value(pLevel_Player->m_goldpieces);
}

/**
 * Method: LevelPlayer#gold=
 *
 *   gold=(num)
 *   waffles=(num)
 *
 * Reset the number of collected gold pieces/waffles to the given
 * value. If you set a value greater than 100, a `Gold_100` event is
 * triggered.
 *
 * #### Parameter
 * num
 * : The new number of gold pieces/waffles. This value obeys the same
 *   100-rule as the parameter to [add_gold()](#addgold).
 */
/**
 * Method: LevelPlayer#waffles=
 *
 *   gold=(num)
 *   waffles=(num)
 *
 * Alias for [#gold=](#gold-1).
 */
static mrb_value Set_Gold(mrb_state* p_state,  mrb_value self)
{
    mrb_int gold;
    mrb_get_args(p_state, "i", &gold);

    pHud_Goldpieces->Set_Gold(gold);
    return mrb_fixnum_value(gold);
}

/**
 * Method: LevelPlayer#add_gold
 *
 *   add_gold(num)    → an_integer
 *   add_waffles(num) → an_integer
 *
 * Add to the player’s current amount of gold/waffles. If the number of
 * gold passes 100, a `Gold_100` event is triggered.
 *
 * #### Parameter
 * num
 * : The number of gold pieces/waffles to add. If Maryo’s resulting
 *   amount of gold pieces/waffles (i.e. the current amount plus `num`)
 *   is greater than 99, Maryo gains a life and 100 is subtracted
 *   from the resulting amount. This process is repeated until the total
 *   resulting amount of gold pieces/waffles is not greater than 99.
 *
 * #### Return value
 * The new amount of gold pieces/waffles (after the 100-rule described
 * above has been applied as often as necessary).
 */
/**
 * Method:  LevelPlayer#add_waffles
 *
 *   add_gold(num)    → an_integer
 *   add_waffles(num) → an_integer
 *
 * Alias for [#add_gold](#addgold)
 */
static mrb_value Add_Gold(mrb_state* p_state,  mrb_value self)
{
    mrb_int gold;
    mrb_get_args(p_state, "i", &gold);

    pHud_Goldpieces->Add_Gold(gold);
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
 * Reset Maryo’s number of lives to the given value.
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
 * The number of lives Maryo now has.
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
 * Checks whether Maryo currently cannot be defeated. This
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
 * If Maryo currently carries something (shell), he will drop it.
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
    mrb_define_method(p_state, p_rcLevel_Player, "gold", Get_Gold, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel_Player, "gold=", Set_Gold, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcLevel_Player, "add_gold", Add_Gold, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcLevel_Player, "lives", Get_Lives, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel_Player, "lives=", Set_Lives, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcLevel_Player, "add_lives", Add_Lives, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcLevel_Player, "invincible?", Is_Invincible, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLevel_Player, "release_item", Release_Item, MRB_ARGS_NONE());

    // Event handlers
    mrb_define_method(p_state, p_rcLevel_Player, "on_gold_100", MRUBY_EVENT_HANDLER(gold_100), MRB_ARGS_BLOCK());
    mrb_define_method(p_state, p_rcLevel_Player, "on_downgrade", MRUBY_EVENT_HANDLER(downgrade), MRB_ARGS_BLOCK());
    mrb_define_method(p_state, p_rcLevel_Player, "on_jump", MRUBY_EVENT_HANDLER(jump), MRB_ARGS_BLOCK());
    mrb_define_method(p_state, p_rcLevel_Player, "on_shoot", MRUBY_EVENT_HANDLER(shoot), MRB_ARGS_BLOCK());

    // Aliases
    mrb_define_alias(p_state, p_rcLevel_Player, "waffles", "gold");
    mrb_define_alias(p_state, p_rcLevel_Player, "waffles=", "gold=");
    mrb_define_alias(p_state, p_rcLevel_Player, "add_waffles", "add_gold");
    mrb_define_alias(p_state, p_rcLevel_Player, "on_waffles_100", "on_gold_100");
}

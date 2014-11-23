/***************************************************************************
 * mrb_audio.cpp
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

#include "../../../audio/audio.hpp"
#include "../mrb_eventable.hpp"
#include "mrb_audio.hpp"

/**
 * Class: AudioClass
 *
 * The `Audio` singleton, which is the sole instance of `AudioClass`,
 * allows you to interact with TSC’s sound system. You can play any sound
 * from TSC’s sound collection and any music that could also be used as a
 * background music in a level. Namely, paths to sound files are relative
 * to the TSC `sounds/` directory and music paths relative to the TSC
 * `music/` directory. The following table lists some examples (of
 * course you have to adapt the paths to your local setup):
 *
 * |-----------------------+---------------------------------------+--------------------------------------|
 * | TSC installation path | Sound path                            | Music path                           |
 * |-----------------------+---------------------------------------+--------------------------------------|
 * |/usr/local             | /usr/local/share/tsc/sounds           | /usr/local/share/tsc/music           |
 * |-----------------------+---------------------------------------+--------------------------------------|
 * |C:\Program files\TSC   | C:\Program files\TSC\share\tsc\sounds | C:\Program files\TSC\share\tsc\music |
 * |-----------------------+---------------------------------------+--------------------------------------|
 *
 * TODO: Check the Windows path.
 *
 * So, if you want to play the star music, you first have to find where
 * the music file is located. So, assuming your TSC is installed at
 * `/usr/local`, you’d find the star music at
 * `/usr/local/share/tsc/music/game/star.ogg`. To play it, you’d take
 * the path relative to `/usr/local/share/tsc/music/`,
 * i.e. `game/star.ogg`. This is what you pass on to a method such as
 * [play_music](#playmusic):
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ruby
 * Audio.play_music("game/star.ogg")
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * Note that path elements are always separated with a forward slash /,
 * even if the native path separation on some platforms is another (such
 * as \ on Windows).
*/

using namespace TSC;
using namespace TSC::Scripting;


static mrb_value Initialize(mrb_state* p_state,  mrb_value self)
{
    mrb_raise(p_state, MRB_NOTIMP_ERROR(p_state), "Cannot create instances of this class.");
    return self; // Not reached
}

/**
 * Method: AudioClass#play_sound
 *
 *   play_sound( filename [, volume [, loops [, resid ] ] ] ) → a_bool
 *
 * Plays a sound.
 *
 * #### Parameters
 *
 * filename
 * : Path to the sound file to play, relative to the `sounds/`
 *   directory.
 *
 * volume (-1)
 * : Volume the sound shall have. Between 0 and 100.
 *
 * loops (0)
 * : Number of times to repeat the sound _after_ it has been played once,
 *   i.e. the number of sounds played equals `1 + loops`.
 *
 * resid (-1)
 * : Special identifier to prevent a sound from being
 *   played while another instance of this sound is already being
 *   played. TSC ensures that no two sounds with the same resource
 *   ID are played at the same time, i.e. the running sound will
 *   be stopped and discarded before your sound is played. You can
 *   define your own IDs, but there is a number of IDs predefined
 *   by TSC:
 *
 *   1. Maryo jump sound.
 *   2. Maryo wall hit sound
 *   3. Maryo powerdown sound
 *   4. Maryo ball sound
 *   5. Maryo death sound
 *   6. Fireplant, blue mushroom, ghost mushroom and feather
 *      sound
 *   7. 1-Up mushroom and moon sound
 *   8. Maryo Au! (not used currently)
 *   9. Maryo stop sound
 *
 *   Specifying -1 for this parameter allows the given sound to be played
 *   multiple times.
 *
 * #### Return value
 *
 * True on success, false otherwise. Possible failure reasons include
 * incorrect filenames or the sound may simply have been muted by
 * the user in TSC’s preferences, so you probably shouldn’t give
 * too much on this.
 */
static mrb_value Play_Sound(mrb_state* p_state,  mrb_value self)
{
    char* filename = NULL;
    mrb_int volume = -1;
    mrb_int loops = 0;
    mrb_int resid = -1;
    mrb_get_args(p_state,"z|iii", &filename, &volume, &loops, &resid);

    if (pAudio->Play_Sound(filename, resid, volume, loops))
        return mrb_true_value();
    else
        return mrb_false_value();
}

/**
 * Method: AudioClass#play_music
 *
 *   play_music( filename [, loops [, force [, fadein_ms = 0 ] ] ] ) → a_bool
 *
 * Plays a music (optionally replacing the currently played one, if any).
 *
 * #### Parameters
 *
 * filename
 * : Name of the music file to play, relative to the `music/`
 *   directory.
 *
 * loops (0)
 * : Number of times to repeat the music _after_ it has been played once,
 *   i.e. the number of musics played equals `1 + loops`.
 *
 * force (true)
 * : Enforces the new music to run even if another music
 *   is already running (the running music is stopped and discarded).
 *   Note this behaviour is enabled by default, you have to explicitely
 *   set this to `false`.
 *
 * fadein_ms (0)
 * : Number of milliseconds to fade the music in.
 *
 * #### Return value
 *
 * True on success, false otherwise. Possible failure reasons include
 * incorrect filenames or the music may simply have been muted by
 * the user in TSC’s preferences, so you probably shouldn’t give
 * too much on this.
 */
static mrb_value Play_Music(mrb_state* p_state,  mrb_value self)
{
    char* filename = NULL;
    mrb_int loops = 0;
    mrb_value force = mrb_true_value();
    mrb_int fadein_ms = 0;

    mrb_get_args(p_state, "z|ioi", &filename, &loops, &force, &fadein_ms);

    if (pAudio->Play_Music(filename, loops, mrb_test(force), fadein_ms))
        return mrb_true_value();
    else
        return mrb_false_value();
}



void TSC::Scripting::Init_Audio(mrb_state* p_state)
{
    struct RClass* p_rcAudio = mrb_define_class(p_state, "AudioClass", p_state->object_class);
    mrb_include_module(p_state, p_rcAudio, mrb_class_get(p_state, "Eventable"));
    MRB_SET_INSTANCE_TT(p_rcAudio, MRB_TT_DATA);

    // Make the Audio constant the only instance of AudioClass
    mrb_define_const(p_state, p_state->object_class, "Audio", pAudio->Create_MRuby_Object(p_state));

    mrb_define_method(p_state, p_rcAudio, "initialize", Initialize, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcAudio, "play_sound", Play_Sound, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(3));
    mrb_define_method(p_state, p_rcAudio, "play_music", Play_Music, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(3));
}

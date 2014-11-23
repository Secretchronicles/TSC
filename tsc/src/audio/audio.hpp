/***************************************************************************
 * audio.h
 *
 * Copyright © 2003 - 2011 Florian Richter
 * Copyright © 2013 - 2014 The TSC Contributors
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TSC_AUDIO_HPP
#define TSC_AUDIO_HPP

#include "../core/global_basic.hpp"
#include "../audio/sound_manager.hpp"
#include "../scripting/scriptable_object.hpp"
#include "../scripting/objects/misc/mrb_audio.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** Sound Resource ID's  *** *** *** *** *** *** *** *** *** *** */

// sounds which shouldn't be played multiple times at the same time
    enum AudioChannel {
        RID_MARYO_JUMP      = 1,
        RID_MARYO_WALL_HIT  = 2,
        RID_MARYO_POWERDOWN = 3,
        RID_MARYO_DEATH     = 5,
        RID_MARYO_BALL      = 4,
        RID_MARYO_AU        = 8,
        RID_MARYO_STOP      = 9,

        RID_FIREPLANT       = 6,
        RID_MUSHROOM_BLUE   = 6,
        RID_MUSHROOM_GHOST  = 6,
        RID_MUSHROOM        = 6,
        RID_FEATHER         = 6,
        RID_1UP_MUSHROOM    = 7,
        RID_MOON            = 7
    };

    /* *** *** *** *** *** *** *** Audio Sound object *** *** *** *** *** *** *** *** *** *** */

// Callback for a sound finished playing
    void Finished_Sound(const int channel);

    class cAudio_Sound {
    public:
        cAudio_Sound(void);
        virtual ~cAudio_Sound(void);

        // Load the data
        void Load(cSound* data);
        // Free the data
        void Free(void);
        // Finished playing
        void Finished(void);

        /* Play the Sound
         * use_res_id: if set stops all sounds using the same resource id.
         * loops : if set to -1 loops indefinitely or if greater than zero, loop the sound that many times.
        */
        int Play(int use_res_id = -1, int loops = 0);
        // Stop the Sound if playing
        void Stop(void);

        // sound object
        cSound* m_data;

        // channel if playing else -1
        int m_channel;
        // the last used resource id
        int m_resource_id;
    };

    typedef vector<cAudio_Sound*> AudioSoundList;

    /* *** *** *** *** *** *** *** Audio class *** *** *** *** *** *** *** *** *** *** */

    class cAudio: public Scripting::cScriptable_Object {
    public:
        cAudio(void);
        ~cAudio(void);

        // Initialize Audio Engine
        bool Init(void);
        // De-initializes Audio Engine
        void Close(void);

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            // See docs in mrb_level.cpp for why we associate ourself
            // with the Level class here instead of a savegame class.
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "AudioClass"), &Scripting::rtTSC_Scriptable, this));
        }

        // Set the maximum number of sounds playable at once
        void Set_Max_Sounds(unsigned int limit = 10);

        /* Check if the sound was already loaded and returns a pointer to it else it will be loaded.
         * The returned sound should not be deleted or modified.
         */
        cSound* Get_Sound_File(boost::filesystem::path filename) const;

        // Play the given sound. `filename' should be relative to the sounds/ directory.
        bool Play_Sound(boost::filesystem::path filename, int res_id = -1, int volume = -1, int loops = 0);
        // If no forcing it will be played after the current music
        bool Play_Music(boost::filesystem::path filename, int loops = 0, bool force = 1, unsigned int fadein_ms = 0);

        /* Returns a pointer to the sound if it is active.
         * The returned sound should not be deleted or modified.
         */
        cAudio_Sound* Get_Playing_Sound(boost::filesystem::path filename);

        /* Returns true if a free channel for the sound is available
        */
        cAudio_Sound* Create_Sound_Channel(void);

        // Toggle Music on/off
        void Toggle_Music(void);
        // Toggle Sounds on/off
        void Toggle_Sounds(void);

        // Pause Music
        void Pause_Music(void) const;

        /* Resume halted sound
         * if channel is -1 all halted sounds will be resumed
        */
        void Resume_Sound(int channel = -1) const;
        // Resume Music
        void Resume_Music(void) const;

        /* Fade out Sound(s)
         * ms : the time to fade out
         * channel : if set only fade this channel out or if -1 all channels
         * overwrite_fading : overwrite an already existing fade out
        */
        void Fadeout_Sounds(unsigned int ms = 200, int channel = -1, bool overwrite_fading = 0) const;
        /* Fade out Sound(s)
         * ms : the time to fade out
         * filename : fade all sounds with this filename out
         * overwrite_fading : overwrite an already existing fade out
        */
        void Fadeout_Sounds(unsigned int ms, boost::filesystem::path filename, bool overwrite_fading = 0);
        /* Fade out Music
         * ms : the time to fade out
         * overwrite_fading : overwrite an already existing fade out
        */
        void Fadeout_Music(unsigned int ms = 500, bool overwrite_fading = 0) const;

        // Set the Music position ( if .ogg in seconds )
        void Set_Music_Position(float position) const;

        /* Returns
        * 0 if not fading
        * 1 if fading out
        * 2 if fading in
        */
        Mix_Fading Is_Music_Fading(void) const;
        /* Returns
        * 0 if not fading
        * 1 if fading out
        * 2 if fading in
        */
        Mix_Fading Is_Sound_Fading(int sound_channel) const;

        // Returns true if the Music is paused
        bool Is_Music_Paused(void) const;
        // Returns true if the Music is playing
        bool Is_Music_Playing(void) const;

        // Halt the given sounds
        void Halt_Sounds(int channel = -1) const;
        // Halt the Music
        void Halt_Music(void) const;

        // Stop all sounds
        void Stop_Sounds(void) const;

        // Set the Sound Volume
        void Set_Sound_Volume(Uint8 volume, int channel = -1) const;
        // Set the Music Volume
        void Set_Music_Volume(Uint8 volume) const;

        // Update
        void Update(void);

        // is the audio engine initialized
        bool m_initialised;
        // is sound enabled
        bool m_sound_enabled;
        // is music enabled
        bool m_music_enabled;
        // is the debug mode enabled
        bool m_debug;

        // current music and sound volume
        Uint8 m_sound_volume, m_music_volume;

        // current playing music filename
        boost::filesystem::path m_music_filename;
        // current playing music pointer
        Mix_Music* m_music;
        // if new music should play after the current this is the old data
        Mix_Music* m_music_old;

        // The current sounds pointer array
        AudioSoundList m_active_sounds;

        // maximum sounds allowed at once
        unsigned int m_max_sounds;

        // initialization information
        int m_audio_buffer, m_audio_channels;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

/// Audio Handler
    extern cAudio* pAudio;

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

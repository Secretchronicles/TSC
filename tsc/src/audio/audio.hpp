/***************************************************************************
 * audio.h
 *
 * Copyright © 2003 - 2011 Florian Richter
 * Copyright © 2012-2020 The TSC Contributors
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

#define MAX_VOLUME 100

#include "../core/global_basic.hpp"
#include "../audio/sound_manager.hpp"
#include "../scripting/scriptable_object.hpp"
#include "../scripting/objects/misc/mrb_audio.hpp"
#include <queue>
#ifdef _WIN32
#undef IN
#undef OUT
#endif


namespace TSC {

    /* *** *** *** *** *** *** *** Sound Resource ID's  *** *** *** *** *** *** *** *** *** *** */

// sounds which shouldn't be played multiple times at the same time
    enum AudioChannel {
        RID_ALEX_JUMP      = 1,
        RID_ALEX_WALL_HIT  = 2,
        RID_ALEX_POWERDOWN = 3,
        RID_ALEX_DEATH     = 5,
        RID_ALEX_BALL      = 4,
        RID_ALEX_AU        = 8,
        RID_ALEX_STOP      = 9,

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
    /* void Finished_Sound(const int channel); */

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
         * loops : if set to true, loops indefinitely.
        */
        bool Play(int use_res_id = -1, bool loops = false);
        // Stop the Sound if playing
        void Stop(void);

        // sound object
        cSound* m_data;

        // the current sound
        sf::Sound m_sound;
        // the last used resource id
        int m_resource_id;
    };

    typedef vector<cAudio_Sound*> AudioSoundList;

    /* *** *** *** *** *** *** *** Audio class *** *** *** *** *** *** *** *** *** *** */

    class cAudio: public Scripting::cScriptable_Object {
    public:
        // Information to use in the queue of songs to play next.
        struct NextMusicInfo {
            NextMusicInfo(boost::filesystem::path filename, bool loops,
                          unsigned int fadein_ms):
                filename{filename}, loops{loops}, fadein_ms{fadein_ms} {}
            boost::filesystem::path filename;
            bool loops;
            unsigned int fadein_ms;
        };

        cAudio(void);
        ~cAudio(void);

        // Initialize Audio Engine
        bool Init(void);
        // De-initializes Audio Engine
        void Close(bool close_sound=true, bool close_music=true);

        // Create the MRuby object for this
        virtual mrb_value Create_MRuby_Object(mrb_state* p_state)
        {
            // See docs in mrb_level.cpp for why we associate ourself
            // with the Level class here instead of a savegame class.
            return mrb_obj_value(Data_Wrap_Struct(p_state, mrb_class_get(p_state, "AudioClass"), &Scripting::rtTSC_Scriptable, this));
        }

        /* Check if the sound was already loaded and returns a pointer to it else it will be loaded.
         * The returned sound should not be deleted or modified.
         */
        cSound* Get_Sound_File(boost::filesystem::path filename) const;

        // Play the given sound. `filename' should be relative to the sounds/ directory.
        bool Play_Sound(boost::filesystem::path filename, int res_id = -1, int volume = -1, bool loops = false);
        // If no forcing it will be played after the current music
        bool Play_Music(boost::filesystem::path filename, bool loops = false, bool force = 1, unsigned int fadein_ms = 0);

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
        void Pause_Music(void);
        // Resume Music
        void Resume_Music(void);

        /* Fade out Music
         * ms : the time to fade out
        */
        void Fadeout_Music(unsigned int ms = 500);

        // Set the Music position ( if .ogg in seconds )
        void Set_Music_Position(float position);

        // Returns true if the Music is paused
        bool Is_Music_Paused(void) const;
        // Returns true if the Music is playing
        bool Is_Music_Playing(void) const;

        // Halt the Music
        void Halt_Music(void);

        // Stop all sounds
        void Stop_Sounds(void) const;

        // Set the Sound Volume
        void Set_Sound_Volume(uint8_t volume);
        // Set the Music Volume
        void Set_Music_Volume(uint8_t volume);

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
        uint8_t m_sound_volume, m_music_volume;

        // current playing music filename
        boost::filesystem::path m_music_filename;
        // current playing music
        sf::Music m_music;

        enum class FadeDirection { NONE, IN, OUT };
        FadeDirection m_fade_direction;
        // fading time start
        std::chrono::time_point<std::chrono::high_resolution_clock> m_fade_time_start;
        // fading time total
        unsigned int m_fade_time_total;

        // next music to play
        std::stack<NextMusicInfo> m_next_music;

        // The current sounds pointer array
        AudioSoundList m_active_sounds;

        // maximum sounds allowed at once
        unsigned int m_max_sounds;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

/// Audio Handler
    extern cAudio* pAudio;

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

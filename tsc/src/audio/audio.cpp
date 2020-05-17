/***************************************************************************
 * audio.cpp  -  Audio class
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

#include "../audio/audio.hpp"
#include "../core/game_core.hpp"
#include "../level/level.hpp"
#include "../overworld/overworld.hpp"
#include "../user/preferences.hpp"
#include "../core/i18n.hpp"
#include "../core/filesystem/filesystem.hpp"
#include "../core/filesystem/resource_manager.hpp"
#include "../core/global_basic.hpp"

using namespace std;

namespace fs = boost::filesystem;

namespace TSC {
/* *** *** *** *** *** *** *** *** Audio Sound *** *** *** *** *** *** *** *** *** */

cAudio_Sound::cAudio_Sound(void)
{
    m_data = NULL;
    m_resource_id = -1;
}

cAudio_Sound::~cAudio_Sound(void)
{
    Free();
}

void cAudio_Sound::Load(cSound* data)
{
    Free();

    m_data = data;
}

void cAudio_Sound::Free(void)
{
    Stop();


    if (m_data) {
        m_data = NULL;
    }

    m_resource_id = -1;
}

bool cAudio_Sound::Play(int use_res_id /* = -1 */, bool loops /* = false */)
{
    if (!m_data) {
        return 0;
    }

    if (use_res_id >= 0) {
        for (AudioSoundList::iterator itr = pAudio->m_active_sounds.begin(); itr != pAudio->m_active_sounds.end(); ++itr) {
            // get object pointer
            cAudio_Sound* obj = (*itr);

            // skip self
            if (obj == this) {
                continue;
            }

            // stop Sounds using the given resource id
            if (obj->m_resource_id == use_res_id) {
                obj->Stop();
            }
        }
    }

    m_resource_id = use_res_id;
    // play sound
    m_sound.setBuffer(m_data->m_buffer);
    m_sound.play();

    return 1;
}

void cAudio_Sound::Stop(void)
{
    // if not loaded
    if (!m_data) {
        return;
    }

    m_sound.stop();
}

/* *** *** *** *** *** *** *** *** Audio *** *** *** *** *** *** *** *** *** */

cAudio::cAudio(void)
{
    m_initialised = 0;
    m_sound_enabled = 0;
    m_music_enabled = 0;

    m_debug = 0;

    m_sound_volume = cPreferences::m_sound_volume_default;
    m_music_volume = cPreferences::m_music_volume_default;
}

cAudio::~cAudio(void)
{
    Close();
}

bool cAudio::Init(void)
{
    bool sound = pPreferences->m_audio_sound;
    bool music = pPreferences->m_audio_music;

    // if no change
    if (m_music_enabled == music && m_sound_enabled == sound) {
        return 1;
    }

    Close(!sound, !music);

    // if no audio
    if (!music && !sound) {
        return 1;
    }

    m_initialised = 1;

    // music initialization
    if (music) {
        m_music_enabled = 1;

        // set music volume
        Set_Music_Volume(m_music_volume);
    }
    // music de-initialization
    else {
        m_music_enabled = 0;
    }

    // sound initialization
    if (sound) {
        m_sound_enabled = 1;

        // set sound volume
        Set_Sound_Volume(m_sound_volume);
    }
    // sound de-initialization
    else {
        m_sound_enabled = 0;
    }

    m_max_sounds = 100; // XXX: what???

    return 1;
}

void cAudio::Close(bool close_sound/*=true*/, bool close_music/*=true*/)
{
    if (m_initialised) {
        if (m_debug) {
            cout << "Closing Audio System" << endl;
        }

        if (m_sound_enabled && close_sound) {
            Stop_Sounds();

            // clear sounds
            for (AudioSoundList::iterator itr = m_active_sounds.begin(); itr != m_active_sounds.end(); ++itr) {
                delete *itr;
            }

            m_active_sounds.clear();

            m_max_sounds = 0;
            m_sound_enabled = 0;
        }

        if (m_music_enabled && close_music) {
            Halt_Music();

            m_music_enabled = 0;
        }

        m_initialised = 0;
    }

    m_fade_direction = FadeDirection::NONE;
}

cSound* cAudio::Get_Sound_File(fs::path filename) const
{
    if (!m_initialised || !m_sound_enabled) {
        return NULL;
    }

    // not available
    if (!File_Exists(filename)) {
        // add sound directory if required
        if (!filename.is_absolute())
            filename = pResource_Manager->Get_Game_Sounds_Directory() / filename;
    }

    cSound* sound = pSound_Manager->Get_Pointer(filename);

    // if not already cached
    if (!sound) {
        sound = new cSound();

        // loaded sound
        if (sound->Load(filename)) {
            pSound_Manager->Add(sound);

            if (m_debug) {
                cout << "Loaded sound file : " << filename.c_str() << endl;
            }
        }
        // failed loading
        else {
            delete sound;
            return NULL;
        }
    }

    return sound;
}

bool cAudio::Play_Sound(fs::path filename, int res_id /* = -1 */, int volume /* = -1 */, bool loops /* = false */)
{
    if (!m_initialised || !m_sound_enabled) {
        return 0;
    }

    // not available
    if (!File_Exists(filename)) {
        // add sound directory
        if (!filename.is_absolute())
            filename = pResource_Manager->Get_Game_Sounds_Directory() / filename;

        // not found
        if (!File_Exists(filename)) {
            cerr << "Warning: Could not find sound file '" << path_to_utf8(filename) << "'" << endl;
            return false;
        }
    }

    cSound* sound_data = Get_Sound_File(filename);

    // failed loading
    if (!sound_data) {
        cerr << "Warning: Could not load sound file '" << path_to_utf8(filename) << "'" << endl;
        return false;
    }

    // create channel
    cAudio_Sound* sound = Create_Sound_Channel();

    if (!sound) {
        // no free channel available
        return 0;
    }

    // load data
    sound->Load(sound_data);

    // failed to play
    if (!sound->Play(res_id, loops)) {
        debug_print("Could not play sound file : %s\n", path_to_utf8(filename).c_str());
        return 0;
    }
    // playing successfully
    else {
        // volume is out of range
        if (volume > MAX_VOLUME) {
            cerr << "PlaySound Volume is out of range : " << volume << endl;
            volume = m_sound_volume;
        }
        // no volume is given
        else if (volume < 0) {
            volume = m_sound_volume;
        }

        // set volume
        sound->m_sound.setVolume(volume);
    }

    return 1;
}

bool cAudio::Play_Music(fs::path filename, bool loops /* = false */, bool force /* = 1 */, unsigned int fadein_ms /* = 0 */)
{
    if (!filename.is_absolute())
        filename = pResource_Manager->Get_Game_Music_Directory() / filename;

    // no valid file
    if (!File_Exists(filename)) {
        cerr << "Warning: Couldn't find music file '" << path_to_utf8(filename) << "'" << endl;
        return 0;
    }

    // save music filename
    m_music_filename = filename;

    if (!m_music_enabled || !m_initialised) {
        return 0;
    }

    // if music is stopped resume it
    Resume_Music();

    // if no music is playing or force to play the given music
    if (!Is_Music_Playing() || force) {
        // stop current music
        Halt_Music();

        // load the given music
        if (!m_music.openFromFile(path_to_utf8(filename).c_str())) {
            debug_print("Couldn't load music file : %s\n", path_to_utf8(filename).c_str());

            // failed to play
            return false;
        }

        m_music.setLoop(loops);
        // set up fade in
        if (fadein_ms) {
            m_music.setVolume(0);
            m_fade_direction = FadeDirection::IN;
            m_fade_time_start = std::chrono::high_resolution_clock::now();
            m_fade_time_total = fadein_ms;
        }
        m_music.play();
    }
    // music is playing and is not forced
    else {
        // put it in the queue of music to play next
        m_next_music.emplace(filename, loops, fadein_ms);
    }

    return true;
}

cAudio_Sound* cAudio::Get_Playing_Sound(fs::path filename)
{
    if (!m_sound_enabled || !m_initialised) {
        return NULL;
    }

    // add sound directory
    if (!filename.is_absolute())
        filename = pResource_Manager->Get_Game_Sounds_Directory() / filename;

    // get all sounds
    for (AudioSoundList::const_iterator itr = m_active_sounds.begin(); itr != m_active_sounds.end(); ++itr) {
        // get object pointer
        cAudio_Sound* obj = (*itr);

        // if not playing
        if (obj->m_sound.getStatus() != sf::SoundSource::Playing) {
            continue;
        }

        // found it
        if (obj->m_data->m_filename.compare(filename) == 0) {
            // return first found
            return obj;
        }
    }

    // not found
    return NULL;
}

cAudio_Sound* cAudio::Create_Sound_Channel(void)
{
    assert(m_max_sounds > 0);

    // get all sounds
    for (AudioSoundList::iterator itr = m_active_sounds.begin(); itr != m_active_sounds.end(); ++itr) {
        // get object pointer
        cAudio_Sound* obj = (*itr);

        // if not playing
        if (obj->m_sound.getStatus() != sf::SoundSource::Playing) {
            // found a free channel
            obj->Free();
            return obj;
        }
    }

    // if not maximum sounds
    if (m_active_sounds.size() < m_max_sounds) {
        cAudio_Sound* sound = new cAudio_Sound();
        m_active_sounds.push_back(sound);
        return sound;
    }

    // none found
    return NULL;
}

void cAudio::Toggle_Music(void)
{
    pPreferences->m_audio_music = !pPreferences->m_audio_music;
    Init();

    // play music
    if (m_music_enabled && !m_music_filename.empty()) {
        Play_Music(m_music_filename, true, true, 2000);
    }
}

void cAudio::Toggle_Sounds(void)
{
    pPreferences->m_audio_sound = !pPreferences->m_audio_sound;
    Init();

    // play test sound
    if (m_sound_enabled) {
        Play_Sound("audio_on.ogg");
    }
}

void cAudio::Pause_Music(void)
{
    if (!m_music_enabled || !m_initialised) {
        return;
    }

    // if music is playing
    m_music.pause();
}

void cAudio::Resume_Music(void)
{
    if (!m_music_enabled || !m_initialised) {
        return;
    }

    if (Is_Music_Paused()) {
        m_music.play();
    }
}

void cAudio::Fadeout_Music(unsigned int ms /* = 500 */)
{
    if (!m_music_enabled || !m_initialised) {
        return;
    }

    // if music is currently not playing
    if (!Is_Music_Playing()) {
        return;
    }

    m_fade_direction = FadeDirection::OUT;
    m_fade_time_start = std::chrono::high_resolution_clock::now();
    m_fade_time_total = ms;
}

void cAudio::Set_Music_Position(float position)
{
    if (!m_music_enabled || !m_initialised) {
        return;
    }

    m_music.setPlayingOffset(sf::seconds(position));
}

bool cAudio::Is_Music_Paused(void) const
{
    if (!m_music_enabled || !m_initialised) {
        return 0;
    }

    return m_music.getStatus() == sf::SoundSource::Paused;
}

bool cAudio::Is_Music_Playing(void) const
{
    if (!m_music_enabled || !m_initialised) {
        return 0;
    }

    return m_music.getStatus() == sf::SoundSource::Playing;
}

void cAudio::Halt_Music(void)
{
    if (!m_initialised) {
        return;
    }

    m_music.stop();
    m_fade_direction = FadeDirection::NONE;
}

void cAudio::Stop_Sounds(void) const
{
    if (!m_initialised) {
        return;
    }

    for (AudioSoundList::iterator itr = pAudio->m_active_sounds.begin(); itr != pAudio->m_active_sounds.end(); ++itr) {
        // get object pointer
        cAudio_Sound* obj = (*itr);

        // stop sound
        obj->Stop();
    }
}

void cAudio::Set_Sound_Volume(uint8_t volume)
{
    // not active
    if (!m_initialised) {
        return;
    }

    // out of range
    if (volume > MAX_VOLUME) {
        volume = MAX_VOLUME;
    }

    for (AudioSoundList::iterator itr = pAudio->m_active_sounds.begin(); itr != pAudio->m_active_sounds.end(); ++itr) {
        // get object pointer
        cAudio_Sound* obj = (*itr);

        // set volume
        obj->m_sound.setVolume(volume);
    }
}

void cAudio::Set_Music_Volume(uint8_t volume)
{
    // not active
    if (!m_initialised || !m_music_enabled) {
        return;
    }

    // out of range
    if (volume > MAX_VOLUME) {
        volume = MAX_VOLUME;
    }

    m_music_volume = volume;
}

void cAudio::Update(void)
{
    if (!m_initialised || !m_music_enabled) {
        return;
    }

    // if music is enabled but nothing is playing
    if (!Is_Music_Playing() && !m_next_music.empty()) {
        // play the next song in the queue
        NextMusicInfo next = m_next_music.top();
        m_next_music.pop();
        Play_Music(next.filename, next.loops, true, next.fadein_ms);
    }
    // if we are currently fading music
    else if (m_fade_direction != FadeDirection::NONE) {
        auto passed = std::chrono::high_resolution_clock::now() - m_fade_time_start;
        unsigned int passed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(passed).count();
        if (passed_ms >= m_fade_time_total) {
            // done fading; reset volume
            if (m_fade_direction == FadeDirection::OUT) {
                Halt_Music();
            }
            m_music.setVolume(m_music_volume);
            m_fade_direction = FadeDirection::NONE;
        } else {
            // update volume
            float new_volume = m_music_volume * (static_cast<float>(passed_ms) / m_fade_time_total);
            if (m_fade_direction == FadeDirection::OUT) {
                new_volume = MAX_VOLUME - new_volume;
            }
            m_music.setVolume(new_volume);
        }
    } else {
        // make sure current volume is up-to-date
        m_music.setVolume(m_music_volume);
    }
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

cAudio* pAudio = NULL;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

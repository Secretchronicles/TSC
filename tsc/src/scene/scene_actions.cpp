/***************************************************************************
 * scene_actions.cpp  -  Individual possible actions in a cScene
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

#include "scene_actions.hpp"
#include "../core/framerate.hpp"
#include "../audio/audio.hpp"
#include "../user/preferences.hpp"
#include "scene.hpp"

namespace fs = boost::filesystem;
using namespace std;
using namespace TSC;
using namespace TSC::SceneActions;

Action::Action(cScene* p_scene)
    : mp_scene(p_scene)
{
}

Action::~Action()
{
}

ImageChange::ImageChange(cScene* p_scene, std::string scene_image)
    : Action(p_scene),
      m_scene_image(scene_image)
{
}

bool ImageChange::Execute()
{
    mp_scene->Set_Scene_Image(m_scene_image);
    return true;
}

MusicChange::MusicChange(cScene* p_scene, std::string music)
    : Action(p_scene),
      m_music(music)
{
}

bool MusicChange::Execute()
{
    pAudio->Play_Music(m_music, true, 0, 1000);
    return true;
}

WaitReturn::WaitReturn(cScene* p_scene)
    : Action(p_scene),
      m_return_pressed(false)
{
}

bool WaitReturn::Execute()
{
    return m_return_pressed;
}

bool WaitReturn::Key_Down(const sf::Event& evt)
{
    // React on some sensible keys
    if (evt.key.code == pPreferences->m_key_jump || evt.key.code == pPreferences->m_key_shoot || evt.key.code == sf::Keyboard::Return) {
        m_return_pressed = true;
        return true;
    }

    return false;
}

WaitTime::WaitTime(cScene* p_scene, float seconds)
    : Action(p_scene),
      m_wait_counter(speedfactor_fps * seconds)
{
}

bool WaitTime::Execute()
{
    m_wait_counter -= pFramerate->m_speed_factor;
    return m_wait_counter <= 0.0f;
}

Narration::Narration(cScene* p_scene, std::initializer_list<std::string> messages)
    : Action(p_scene),
      m_messages(messages),
      m_read(true)
{
}

bool Narration::Execute()
{
    // If the user read the last message, advance to the next
    // one (or terminate if no messages remain to be shown).
    if (!m_read)
        return false;

    if (m_messages.empty()) {
        // Signal end of this action.
        mp_scene->Hide_Story_Box();
        return true;
    }
    else {
        // Get next message and remove it from message list.
        std::string text = m_messages.front();
        m_messages.erase(m_messages.begin());

        // Display it to the user.
        mp_scene->Set_Story_Text(text);
        mp_scene->Show_Story_Box();

        // Mark message as unread.
        m_read = false;

        // Signal this action is not finished.
        return false;
    }
}

bool Narration::Key_Down(const sf::Event& evt)
{
    // Mark message as read on keypress
    if (evt.key.code == pPreferences->m_key_jump || evt.key.code == pPreferences->m_key_shoot || evt.key.code == sf::Keyboard::Return) {
        m_read = true;
        return true;
    }

    return false;
}

NextUp::NextUp(cScene* p_scene, enum GameAction ga, std::string name, std::string arg)
    : Action(p_scene),
      m_ga(ga),
      m_name(name),
      m_arg(arg)
{
}

bool NextUp::Execute()
{
    mp_scene->Set_Next_Game_Action(m_ga, m_name, m_arg);

    return true;
}

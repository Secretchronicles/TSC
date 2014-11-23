/***************************************************************************
 * random_sound.cpp  -  random sounds support
 *
 * Copyright © 2008 - 2011 Florian Richter
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

#include "../audio/random_sound.hpp"
#include "../core/game_core.hpp"
#include "../core/framerate.hpp"
#include "../audio/audio.hpp"
#include "../video/renderer.hpp"
#include "../input/mouse.hpp"
#include "../core/math/utilities.hpp"
#include "../core/i18n.hpp"
#include "../core/xml_attributes.hpp"

namespace TSC {

/* *** *** *** *** *** cRandom_Sound *** *** *** *** *** *** *** *** *** *** *** */

cRandom_Sound::cRandom_Sound(cSprite_Manager* sprite_manager)
    : cSprite(sprite_manager, "sound")
{
    // Set defaults
    cRandom_Sound::Init();
}

cRandom_Sound::cRandom_Sound(XmlAttributes& attributes, cSprite_Manager* sprite_manager)
    : cSprite(sprite_manager, "sound")
{
    cRandom_Sound::Init();

    // filename
    Set_Filename(attributes["file"]);

    // position
    Set_Pos(string_to_float(attributes["posx"]), string_to_float(attributes["posy"]), true);

    //
    Set_Continuous(attributes.fetch<bool>("continuous", m_continuous));

    // delay
    Set_Delay_Min(attributes.fetch<int>("delay_min", m_delay_min));
    Set_Delay_Min(attributes.fetch<int>("delay_max", m_delay_max));

    // volume
    Set_Volume_Min(attributes.fetch<float>("volume_min", m_volume_min));
    Set_Volume_Max(attributes.fetch<float>("volume_min", m_volume_max));

    // volume reduction
    Set_Volume_Reduction_Begin(attributes.fetch<float>("volume_reduction_begin", m_volume_reduction_begin));
    Set_Volume_Reduction_End(attributes.fetch<float>("volume_reduction_end", m_volume_reduction_end));
}

cRandom_Sound::~cRandom_Sound(void)
{

}

void cRandom_Sound::Init(void)
{
    m_sprite_array = ARRAY_ACTIVE;
    m_type = TYPE_SOUND;
    m_massive_type = MASS_PASSIVE;
    m_editor_pos_z = 0.111f;
    m_camera_range = 0;
    m_name = "Sound";

    m_rect.m_w = 10.0f;
    m_rect.m_h = 10.0f;
    m_col_rect.m_w = m_rect.m_w;
    m_col_rect.m_h = m_rect.m_h;
    m_start_rect.m_w = m_rect.m_w;
    m_start_rect.m_h = m_rect.m_h;

    // default values
    m_continuous = 0;
    m_delay_min = 1000;
    m_delay_max = 5000;
    m_volume_min = 90.0f;
    m_volume_max = 100.0f;
    m_volume_reduction_begin = 400.0f;
    m_volume_reduction_end = 1000.0f;

    m_distance_to_camera = 0.0f;
    m_next_play_delay = 0.0f;
    m_volume_update_counter = 0.0f;

    m_editor_color_volume_reduction_begin = Color(0.1f, 0.5f, 0.1f, 0.2f);
    m_editor_color_volume_reduction_end = Color(0.2f, 0.4f, 0.1f, 0.2f);
}

cRandom_Sound* cRandom_Sound::Copy(void) const
{
    cRandom_Sound* random_sound = new cRandom_Sound(m_sprite_manager);
    random_sound->Set_Pos(m_start_pos_x, m_start_pos_y, 1);
    random_sound->Set_Filename(m_filename.c_str());
    random_sound->Set_Continuous(m_continuous);
    random_sound->Set_Delay_Min(m_delay_min);
    random_sound->Set_Delay_Max(m_delay_max);
    random_sound->Set_Volume_Min(m_volume_min);
    random_sound->Set_Volume_Max(m_volume_max);
    random_sound->Set_Volume_Reduction_Begin(m_volume_reduction_begin);
    random_sound->Set_Volume_Reduction_End(m_volume_reduction_end);
    return random_sound;
}

std::string cRandom_Sound::Get_XML_Type_Name()
{
    return "";
}

xmlpp::Element* cRandom_Sound::Save_To_XML_Node(xmlpp::Element* p_element)
{
    xmlpp::Element* p_node = cSprite::Save_To_XML_Node(p_element);


    // filename
    Add_Property(p_node, "file", m_filename);
    // continuous
    Add_Property(p_node, "continuous", m_continuous);
    // delay
    Add_Property(p_node, "delay_min", m_delay_min);
    Add_Property(p_node, "delay_max", m_delay_max);
    // volume
    Add_Property(p_node, "volume_min", m_volume_min);
    Add_Property(p_node, "volume_max", m_volume_max);
    // volume reduction
    Add_Property(p_node, "volume_reduction_begin", m_volume_reduction_begin);
    Add_Property(p_node, "volume_reduction_end", m_volume_reduction_end);

    return p_node;
}

void cRandom_Sound::Set_Filename(const std::string& str)
{
    // stop playing sounds
    for (unsigned int i = 0; i < 100; i++) {
        cAudio_Sound* sound = pAudio->Get_Playing_Sound(m_filename);

        if (!sound) {
            break;
        }

        sound->Stop();
    }

    m_filename = str;
}

std::string cRandom_Sound::Get_Filename(void) const
{
    return m_filename;
}

void cRandom_Sound::Set_Continuous(bool continuous)
{
    m_continuous = continuous;
}

void cRandom_Sound::Set_Delay_Min(unsigned int delay)
{
    if (delay < 20) {
        delay = 20;
    }

    m_delay_min = delay;
}

unsigned int cRandom_Sound::Get_Delay_Min(void) const
{
    return m_delay_min;
}

void cRandom_Sound::Set_Delay_Max(unsigned int delay)
{
    if (delay < 20) {
        delay = 20;
    }
    else if (delay < m_delay_min) {
        delay = m_delay_min;
    }

    m_delay_max = delay;
}

unsigned int cRandom_Sound::Get_Delay_Max(void) const
{
    return m_delay_max;
}

void cRandom_Sound::Set_Volume_Min(float volume)
{
    if (volume < 0.1f) {
        volume = 0.1f;
    }
    else if (volume > m_volume_max) {
        volume = m_volume_max;
    }
    else if (volume > 100.0f) {
        volume = 100.0f;
    }

    m_volume_min = volume;
}

float cRandom_Sound::Get_Volume_Min(void) const
{
    return m_volume_min;
}

void cRandom_Sound::Set_Volume_Max(float volume)
{
    if (volume < 0.1f) {
        volume = 0.1f;
    }
    else if (volume < m_volume_min) {
        volume = m_volume_min;
    }
    else if (volume > 100.0f) {
        volume = 100.0f;
    }

    m_volume_max = volume;
}

float cRandom_Sound::Get_Volume_Max(void) const
{
    return m_volume_max;
}

void cRandom_Sound::Set_Volume_Reduction_Begin(float distance)
{
    if (distance < 1.0f) {
        distance = 1.0f;
    }
    else if (distance > m_volume_reduction_end) {
        distance = m_volume_reduction_end;
    }

    m_volume_reduction_begin = distance;
}

float cRandom_Sound::Get_Volume_Reduction_Begin(void) const
{
    return m_volume_reduction_begin;
}

void cRandom_Sound::Set_Volume_Reduction_End(float distance)
{
    if (distance < 1.0f) {
        distance = 1.0f;
    }
    else if (distance < m_volume_reduction_begin) {
        distance = m_volume_reduction_begin;
    }

    m_volume_reduction_end = distance;
}

float cRandom_Sound::Get_Volume_Reduction_End(void) const
{
    return m_volume_reduction_end;
}

float cRandom_Sound::Get_Distance_Volume_Mod(void) const
{
    // if in volume reduction range
    if (m_distance_to_camera > m_volume_reduction_begin) {
        const float distance = m_distance_to_camera - m_volume_reduction_begin;
        return 1.0f - distance / (m_volume_reduction_end - m_volume_reduction_begin);
    }

    // no reduction
    return 1.0f;
}

void cRandom_Sound::Update(void)
{
    Update_Valid_Update();

    if (!m_valid_update) {
        return;
    }

    bool play = 0;

    if (m_continuous) {
        m_volume_update_counter -= pFramerate->m_elapsed_ticks;

        // get the sound
        cAudio_Sound* sound = pAudio->Get_Playing_Sound(m_filename);

        // if not playing
        if (!sound) {
            // play it
            play = 1;
        }
        // update volume
        else if (m_volume_update_counter <= 0.0f) {
            // volume based on maximum
            float sound_volume = m_volume_max * 0.01f;
            // apply distance modifier
            sound_volume *= Get_Distance_Volume_Mod();
            // set to mixer volume
            sound_volume *= static_cast<float>(MIX_MAX_VOLUME);
            // set volume
            pAudio->Set_Sound_Volume(static_cast<Uint8>(sound_volume), sound->m_channel);

            // update volume every 100 ms
            m_volume_update_counter = 100.0f;
        }
    }
    else {
        // subtract duration of this frame in milliseconds
        m_next_play_delay -= pFramerate->m_elapsed_ticks;

        // play it
        if (m_next_play_delay <= 0.0f) {
            play = 1;

            // set next delay
            m_next_play_delay = static_cast<float>(m_delay_min);

            if (m_delay_max > m_delay_min) {
                m_next_play_delay += Get_Random_Float(0.0f, static_cast<float>(m_delay_max - m_delay_min));
            }
        }
    }

    if (play) {
        float sound_volume = 0.0f;

        // always maximum volume
        if (m_continuous) {
            sound_volume = m_volume_max;
        }
        // set random volume
        else {
            // random
            if (m_volume_max > m_volume_min) {
                sound_volume = Get_Random_Float(m_volume_min, m_volume_max);
            }
            // static
            else {
                sound_volume = m_volume_max;
            }
        }

        sound_volume *= 0.01f;

        // adjust volume based on distance
        sound_volume *= Get_Distance_Volume_Mod();

        int loops = 0;

        if (m_continuous) {
            // unlimited
            loops = -1;
        }

        sound_volume *= static_cast<float>(MIX_MAX_VOLUME);

        // play sound
        pAudio->Play_Sound(m_filename, -1, static_cast<int>(sound_volume), loops);
    }
}

void cRandom_Sound::Draw(cSurface_Request* request /* = NULL */)
{
    if (!m_valid_draw) {
        return;
    }

    // draw color rect
    pVideo->Draw_Rect(m_col_rect.m_x - pActive_Camera->m_x, m_col_rect.m_y - pActive_Camera->m_y, m_col_rect.m_w, m_col_rect.m_h, m_editor_pos_z, &greenyellow);
    // volume reduction begin
    cCircle_Request* circle_request = new cCircle_Request();
    pVideo->Draw_Circle(m_col_rect.m_x - pActive_Camera->m_x, m_col_rect.m_y - pActive_Camera->m_y, m_volume_reduction_begin, m_editor_pos_z - 0.0001f, &m_editor_color_volume_reduction_begin, circle_request);
    circle_request->m_line_width = 3;
    // add request
    pRenderer->Add(circle_request);
    // volume reduction end
    circle_request = new cCircle_Request();
    pVideo->Draw_Circle(m_col_rect.m_x - pActive_Camera->m_x, m_col_rect.m_y - pActive_Camera->m_y, m_volume_reduction_end, m_editor_pos_z - 0.0002f, &m_editor_color_volume_reduction_end, circle_request);
    circle_request->m_line_width = 3;
    // add request
    pRenderer->Add(circle_request);
}

bool cRandom_Sound::Is_Update_Valid()
{
    // if destroyed
    if (m_auto_destroy) {
        return 0;
    }

    // get distance from camera center position
    float dx = fabs(pActive_Camera->m_x + (game_res_w * 0.5f) - m_pos_x);
    float dy = fabs(pActive_Camera->m_y + (game_res_h * 0.5f) - m_pos_y);

    m_distance_to_camera = sqrt(dx * dx + dy * dy);

    // if outside the range
    if (m_distance_to_camera >= m_volume_reduction_end) {
        Event_Out_Of_Range();
        return 0;
    }

    return 1;
}

bool cRandom_Sound::Is_Draw_Valid(void)
{
    // if editor not enabled
    if (!editor_enabled) {
        return 0;
    }

    // if not active on the screen or not mouse object
    if (!m_active || (!Is_Visible_On_Screen() && pMouseCursor->m_active_object != this)) {
        return 0;
    }

    return 1;
}

void cRandom_Sound::Event_Out_Of_Range(void) const
{
    // fade out sounds if out of range
    pAudio->Fadeout_Sounds(500, m_filename);
}

void cRandom_Sound::Editor_Activate(void)
{
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    // filename
    CEGUI::Editbox* editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "editor_sound_filename"));
    Editor_Add(UTF8_("Filename"), UTF8_("Sound filename"), editbox, 200);

    editbox->setText(m_filename.c_str());
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cRandom_Sound::Editor_Filename_Text_Changed, this));

    // continuous
    CEGUI::Checkbox* checkbox = static_cast<CEGUI::Checkbox*>(wmgr.createWindow("TaharezLook/Checkbox", "editor_sound_continuous"));
    Editor_Add(UTF8_("Continuous"), UTF8_("Check if the sound should be played continuously instead of randomly"), checkbox, 50);

    checkbox->setSelected(m_continuous);
    checkbox->subscribeEvent(CEGUI::Checkbox::EventCheckStateChanged, CEGUI::Event::Subscriber(&cRandom_Sound::Editor_Continuous_Changed, this));

    // delay min
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "editor_sound_delay_min"));
    Editor_Add(UTF8_("Delay Minimum"), UTF8_("Minimal delay until played again"), editbox, 90);

    editbox->setValidationString("^[+]?\\d*$");
    editbox->setText(int_to_string(m_delay_min));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cRandom_Sound::Editor_Delay_Min_Text_Changed, this));

    // delay max
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "editor_sound_delay_max"));
    Editor_Add(UTF8_("Maximum"), UTF8_("Maximal delay until played again"), editbox, 90, 28, 0);

    editbox->setValidationString("^[+]?\\d*$");
    editbox->setText(int_to_string(m_delay_max));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cRandom_Sound::Editor_Delay_Max_Text_Changed, this));

    // volume min
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "editor_sound_volume_min"));
    Editor_Add(UTF8_("Volume Minimum"), UTF8_("Minimal random volume for each play"), editbox, 90);

    editbox->setValidationString("^[+]?\\d*$");
    editbox->setText(int_to_string(static_cast<int>(m_volume_min)));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cRandom_Sound::Editor_Volume_Min_Text_Changed, this));

    // volume max
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "editor_sound_volume_max"));
    Editor_Add(UTF8_("Maximum"), UTF8_("Maximal random volume for each play"), editbox, 90, 28, 0);

    editbox->setValidationString("^[+]?\\d*$");
    editbox->setText(int_to_string(static_cast<int>(m_volume_max)));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cRandom_Sound::Editor_Volume_Max_Text_Changed, this));

    // volume reduction begin
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "editor_sound_volume_reduction_begin"));
    Editor_Add(UTF8_("Volume Reduction Begin"), UTF8_("Volume reduction begins gradually at this distance"), editbox, 90);

    editbox->setValidationString("^[+]?\\d*$");
    editbox->setText(int_to_string(static_cast<int>(m_volume_reduction_begin)));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cRandom_Sound::Editor_Volume_Reduction_Begin_Text_Changed, this));

    // volume reduction end
    editbox = static_cast<CEGUI::Editbox*>(wmgr.createWindow("TaharezLook/Editbox", "editor_sound_volume_reduction_end"));
    Editor_Add(UTF8_("End"), UTF8_("Volume reduction ends at this distance. Sound is not played beyond this."), editbox, 90, 28, 0);

    editbox->setValidationString("^[+]?\\d*$");
    editbox->setText(int_to_string(static_cast<int>(m_volume_reduction_end)));
    editbox->subscribeEvent(CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber(&cRandom_Sound::Editor_Volume_Reduction_End_Text_Changed, this));

    // init
    Editor_Init();
}

bool cRandom_Sound::Editor_Filename_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    Set_Filename(static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str());

    return 1;
}

bool cRandom_Sound::Editor_Continuous_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    bool enabled = static_cast<CEGUI::Checkbox*>(windowEventArgs.window)->isSelected();

    Set_Continuous(enabled);

    return 1;
}

bool cRandom_Sound::Editor_Delay_Min_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Delay_Min(string_to_int(str_text));

    return 1;
}

bool cRandom_Sound::Editor_Delay_Max_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Delay_Max(string_to_int(str_text));

    return 1;
}

bool cRandom_Sound::Editor_Volume_Min_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Volume_Min(string_to_float(str_text));

    return 1;
}

bool cRandom_Sound::Editor_Volume_Max_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Volume_Max(string_to_float(str_text));

    return 1;
}

bool cRandom_Sound::Editor_Volume_Reduction_Begin_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Volume_Reduction_Begin(string_to_float(str_text));

    return 1;
}

bool cRandom_Sound::Editor_Volume_Reduction_End_Text_Changed(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    std::string str_text = static_cast<CEGUI::Editbox*>(windowEventArgs.window)->getText().c_str();

    Set_Volume_Reduction_End(string_to_float(str_text));

    return 1;
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

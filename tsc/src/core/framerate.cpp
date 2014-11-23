/***************************************************************************
 * framerate.cpp  -  Framerate independant motion control
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

#include "../core/global_basic.hpp"
#include "../core/framerate.hpp"
#include "../core/math/utilities.hpp"

namespace TSC {

/* *** *** *** *** *** *** cPerformance_Timer *** *** *** *** *** *** *** *** *** *** *** */

cPerformance_Timer::cPerformance_Timer(void)
{
    Reset();
}

cPerformance_Timer::~cPerformance_Timer(void)
{

}

void cPerformance_Timer::Reset(void)
{
    frame_counter = 0;
    ms_counter = 0;
    ms = 0;
}

void cPerformance_Timer::Update(void)
{
    // count frame
    frame_counter++;

    // add milliseconds
    Uint32 new_ticks = SDL_GetTicks();
    ms_counter += new_ticks - pFramerate->m_perf_last_ticks;
    pFramerate->m_perf_last_ticks = new_ticks;

    // counted 100 frames
    if (frame_counter >= 100) {
        ms = ms_counter;
        frame_counter = 0;
        ms_counter = 0;
    }
}


/* *** *** *** *** *** *** cFramerate *** *** *** *** *** *** *** *** *** *** *** */

cFramerate::cFramerate(void)
{
    m_fps_target = 0;
    m_fps = 0.0f;
    m_fps_best = 0.0f;
    m_fps_worst = 100000.0f;
    m_fps_average = 0;
    m_fps_average_framedelay = 0;
    m_frames_counted = 0;
    m_last_ticks = 0;
    m_elapsed_ticks = 1;
    m_max_elapsed_ticks = 100;
    m_speed_factor = 0.1f;
    m_force_speed_factor = 0.0f;

    // create performance timers
    for (unsigned int i = 0; i < 24; i++) {
        m_perf_timer.push_back(new cPerformance_Timer());
    }
}

cFramerate::~cFramerate(void)
{
    // clear performance timer
    for (Performance_Timer_List::iterator itr = m_perf_timer.begin(); itr != m_perf_timer.end(); ++itr) {
        delete *itr;
    }

    m_perf_timer.clear();
}

void cFramerate::Init(const float target_fps /* = speedfactor_fps */)
{
    m_fps_target = target_fps;
    m_max_elapsed_ticks = 100;
    m_force_speed_factor = 0.0f;

    Reset();
}

void cFramerate::Update(void)
{
    const Uint32 current_ticks = SDL_GetTicks();

    // if speed factor is forced
    if (!Is_Float_Equal(m_force_speed_factor, 0.0f)) {
        m_speed_factor = m_force_speed_factor;
        m_elapsed_ticks = static_cast<Uint32>((m_force_speed_factor * 1000) / m_fps_target);

        // change to minimum
        if (m_elapsed_ticks == 0) {
            m_elapsed_ticks = 1;
        }
    }
    // measure speed factor
    else {
        // set elapsed ticks
        m_elapsed_ticks = current_ticks - m_last_ticks;

        // minimum
        if (m_elapsed_ticks == 0) {
            m_elapsed_ticks = 1;
        }
        // maximum
        else if (m_elapsed_ticks > m_max_elapsed_ticks) {
            m_elapsed_ticks = m_max_elapsed_ticks;
        }

        // speed factor calculation for this frame
        m_speed_factor = static_cast<float>(m_elapsed_ticks / (1000 / m_fps_target));
    }

    // speed factor based fps
    m_fps = m_fps_target / m_speed_factor;

    // calculate average fps every second
    if (current_ticks - m_fps_average_framedelay > 1000) {
        m_fps_average = m_frames_counted;

        m_fps_average_framedelay += 1000;
        m_frames_counted = 0;
    }
    // count a fps
    else {
        m_frames_counted++;
    }

    // best fps
    if (m_fps > m_fps_best) {
        m_fps_best = m_fps;
    }
    // worst fps
    else if (m_fps < m_fps_worst) {
        m_fps_worst = m_fps;
    }

    m_last_ticks = current_ticks;
}

void cFramerate::Reset(void)
{
    m_last_ticks = SDL_GetTicks();
    m_elapsed_ticks = 1;
    m_speed_factor = 0.001f;
    m_fps_best = 0;
    m_fps_worst = 100000.0f;
    m_fps_average = 0;
    m_fps_average_framedelay = m_last_ticks;
    m_frames_counted = 0;

    // reset performance timer
    for (Performance_Timer_List::iterator itr = m_perf_timer.begin(); itr != m_perf_timer.end(); ++itr) {
        (*itr)->Reset();
    }
}

void cFramerate::Set_Max_Elapsed_Ticks(const Uint32 ticks)
{
    m_max_elapsed_ticks = ticks;
}

void cFramerate::Set_Fixed_Speedfacor(const float val)
{
    m_force_speed_factor = val;
}

/* *** *** *** *** *** *** *** helper functions *** *** *** *** *** *** *** *** *** *** */

void Correct_Frame_Time(const unsigned int fps)
{
    while (!Is_Frame_Time(fps)) {
        SDL_Delay(1);
    }
}

bool Is_Frame_Time(const unsigned int fps)
{
    static Uint32 static_time = 0;

    if (SDL_GetTicks() - static_time < 1000 / fps) {
        return 0;
    }

    static_time = SDL_GetTicks();
    return 1;
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

cFramerate* pFramerate = NULL;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

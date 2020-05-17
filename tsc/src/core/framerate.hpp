/***************************************************************************
 * framerate.h
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

#ifndef TSC_FRAMERATE_HPP
#define TSC_FRAMERATE_HPP

#include "../core/global_game.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** cPerformance_Timer *** *** *** *** *** *** *** *** *** *** */

// counts milliseconds for 100 frames and sets them to ms
    class cPerformance_Timer {
    public:
        cPerformance_Timer(void);
        ~cPerformance_Timer(void);

        // reset
        void Reset(void);

        // Update and set new framerate ticks
        void Update(void);

        // current frame counter
        uint32_t frame_counter;
        // current milliseconds per frames counted
        uint32_t ms_counter;
        // milliseconds per 100 frames
        uint32_t ms;
    };

    /* *** *** *** *** *** *** *** cFramerate *** *** *** *** *** *** *** *** *** *** */

    /* Framerate class
     * speed factor is a float with that you multiply all your motions. For instance, if the targetfps is 100,
     * and the actual fps is 80, the speed factor will be set to 100/80, or 1.25.
    */
    class cFramerate {
    public:
        cFramerate(void);
        ~cFramerate(void);

        // Initialize with the given target fps
        void Init(const float target_fps = speedfactor_fps);
        // update speed factor
        void Update(void);
        // reset speed factor and worst/best fps statistic
        void Reset(void);

        // set maximum allowed elapsed ticks
        void Set_Max_Elapsed_Ticks(const uint32_t ticks);

        /* Set the given fixed speed factor
         * if value is 0 no fixed speed factor will be used
        */
        void Set_Fixed_Speedfacor(const float val);

        // target fps for speed factor calculations
        float m_fps_target;
        // current fps
        float m_fps;
        // best fps
        float m_fps_best;
        // worst fps
        float m_fps_worst;
        // average fps in the last second
        unsigned int m_fps_average;
        // delay since last average fps calculation
        uint32_t m_fps_average_framedelay;
        // amount of frames counted
        unsigned int m_frames_counted;

        /* last update ticks
         * used for speed factor calculation
         */
        uint32_t m_last_ticks;
        // elapsed ticks since last frame
        uint32_t m_elapsed_ticks;
        // maximum elapsed ticks
        uint32_t m_max_elapsed_ticks;

        /* current factor
         * based on target fps
         */
        float m_speed_factor;
        // fixed speed factor value
        float m_force_speed_factor;

        // ## performance values ##
        // ticks since last section
        uint32_t m_perf_last_ticks;

        typedef vector<cPerformance_Timer*> Performance_Timer_List;
        Performance_Timer_List m_perf_timer;
    };

    /* *** *** *** *** *** *** *** helper functions *** *** *** *** *** *** *** *** *** *** */

    /* Fixed framerate method
     * if next frame is not ready wait until it is
    */
    void Correct_Frame_Time(const unsigned int fps);
// Return true if the next frame is ready for the given framerate
    bool Is_Frame_Time(const unsigned int fps);

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

// Framerate class
    extern cFramerate* pFramerate;

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

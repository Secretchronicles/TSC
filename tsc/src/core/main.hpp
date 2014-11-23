/***************************************************************************
 * main.h
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

#ifndef TSC_MAIN_HPP
#define TSC_MAIN_HPP

namespace TSC {

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

// Initialize everything, called before everything else
// Seed random numbers, init SDL, OpenGL, CEGUI, load preferences, and create globals
    void Init_Game(void);

// Save preferences, delete globals, and closes SDL
    void Exit_Game(void);

    /* Top-level input function.
     * Calls either KeyDown, KeyUp, or passes control to pMouseCursor or pJoystick
     * Returns true if the event was handled.
    */
    bool Handle_Input_Global(SDL_Event* ev);

    /* Update current game state
     * Should be called continuously from Game Loop.
    */
    void Update_Game(void);

    /* Draw current game state
     * Should be called continuously from Game Loop.
    */
    void Draw_Game(void);

    extern const std::string g_credits;

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

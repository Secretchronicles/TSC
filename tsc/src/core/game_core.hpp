/***************************************************************************
 * game_core.h
 *
 * Copyright © 2005 - 2011 Florian Richter
 * Copyright © 2013 - 2014 The TSC Contributors
 ***************************************************************************/
/*
 *   This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TSC_GAME_CORE_HPP
#define TSC_GAME_CORE_HPP

#include "../core/property_helper.hpp"
#include "../objects/sprite.hpp"
#include "../core/camera.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** *** Variables *** *** *** *** *** *** *** *** *** */

// quit game if true
    extern bool game_exit;
// current Game Mode
    extern GameMode Game_Mode;
// current Game Mode Type
    extern GameModeType Game_Mode_Type;
// next global Game Action
    extern GameAction Game_Action;
// Game Action data
    extern CEGUI::XMLAttributes Game_Action_Data_Start;
    extern CEGUI::XMLAttributes Game_Action_Data_Middle;
    extern CEGUI::XMLAttributes Game_Action_Data_End;
// Game Action pointer
    extern void* Game_Action_ptr;

// internal game resolution and is used for global scaling
    extern int game_res_w;
    extern int game_res_h;

// global debugging
    extern bool game_debug;
    extern bool game_debug_performance;

// Game Input event
    extern SDL_Event input_event;

// global up scale ( f.e. default image scale )
    extern float global_upscalex;
    extern float global_upscaley;
// global down scale ( f.e. mouse/CEGUI scale )
    extern float global_downscalex;
    extern float global_downscaley;

// The global editor enabled variables prevent including additional editor header files
// if editor is enabled for the current game mode
    extern bool editor_enabled;
// if level editor is active
    extern bool editor_level_enabled;
// if world editor is active
    extern bool editor_world_enabled;

// Active camera class
    extern cCamera* pActive_Camera;
// Active player
    extern cSprite* pActive_Player;


    /* *** *** *** *** *** *** *** Functions *** *** *** *** *** *** *** *** *** *** */

// Handle game events
    void Handle_Game_Events(void);
// Handle generic game events
    void Handle_Generic_Game_Events(const CEGUI::XMLAttributes& Current_Game_Action_Data);
    /* Leave the current game mode state
     * mostly to prepare for a new game mode
    */
    void Leave_Game_Mode(const GameMode next_mode);
    /* Enter the given game mode
     * handles/changes game mode specific managers and objects
    */
    void Enter_Game_Mode(const GameMode new_mode);

// Clear the complete input event queue
    void Clear_Input_Events(void);

    /* Preload the common images into the image manager
     * draw_gui : if set use the loading screen gui for drawing
     */
    void Preload_Images(bool draw_gui = 0);

    /* Preload the common sounds into the sound manager
     * draw_gui : if set use the loading screen gui for drawing
     */
    void Preload_Sounds(bool draw_gui = 0);

/// Add a <property> node below the given XML node.
    void Add_Property(xmlpp::Element* p_element, const Glib::ustring& name, const Glib::ustring& value);

    inline void Add_Property(xmlpp::Element* p_element, const Glib::ustring& name, int value)
    {
        Add_Property(p_element, name, int_to_string(value));
    }
    inline void Add_Property(xmlpp::Element* p_element, const Glib::ustring& name, Uint64 value)
    {
        Add_Property(p_element, name, int64_to_string(value));
    }
    inline void Add_Property(xmlpp::Element* p_element, const Glib::ustring& name, long value)
    {
        Add_Property(p_element, name, long_to_string(value));
    }
    inline void Add_Property(xmlpp::Element* p_element, const Glib::ustring& name, float value)
    {
        Add_Property(p_element, name, float_to_string(value));
    }
    inline void Add_Property(xmlpp::Element* p_element, const Glib::ustring& name, bool value)
    {
        Add_Property(p_element, name, bool_to_string(value));
    }
    inline void Add_Property(xmlpp::Element* p_element, const Glib::ustring& name, unsigned int value)
    {
        Add_Property(p_element, name, uint_to_string(value));
    }

// Replace a property if it exists (or add it if it does not exist)
    void Replace_Property(xmlpp::Element* p_element, const Glib::ustring& name, const Glib::ustring& value);

    inline void Replace_Property(xmlpp::Element* p_element, const Glib::ustring& name, int value)
    {
        Replace_Property(p_element, name, int_to_string(value));
    }
    inline void Replace_Property(xmlpp::Element* p_element, const Glib::ustring& name, Uint64 value)
    {
        Replace_Property(p_element, name, int64_to_string(value));
    }
    inline void Replace_Property(xmlpp::Element* p_element, const Glib::ustring& name, long value)
    {
        Replace_Property(p_element, name, long_to_string(value));
    }
    inline void Replace_Property(xmlpp::Element* p_element, const Glib::ustring& name, float value)
    {
        Replace_Property(p_element, name, float_to_string(value));
    }
    inline void Replace_Property(xmlpp::Element* p_element, const Glib::ustring& name, bool value)
    {
        Replace_Property(p_element, name, bool_to_string(value));
    }
    inline void Replace_Property(xmlpp::Element* p_element, const Glib::ustring& name, unsigned int value)
    {
        Replace_Property(p_element, name, uint_to_string(value));
    }

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

/***************************************************************************
 * menu.h
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

#ifndef TSC_MENU_HPP
#define TSC_MENU_HPP

#include "../core/global_basic.hpp"
#include "../video/animation.hpp"
#include "../video/video.hpp"
#include "../core/camera.hpp"
#include "hud.hpp"

namespace TSC {

    /* *** *** *** *** *** *** cMenuHandler *** *** *** *** *** *** *** *** *** *** *** */

    /*
    * handle dynamic Menu-Items
    */
    class cMenuHandler {
    public:
        cMenuHandler(void);
        ~cMenuHandler(void);

        // Adds a Menu item to the menu
        int Add_Menu_Item(sf::FloatRect p_rect, void* p_item);
        //void Add_Menu_Item(cMenu_Item* item, float shadow_pos = 0, Color shadow_color = static_cast<uint8_t>(0));

        // Unloads all items
        void Reset(void);

        /* Sets the Active Menu Item
        * if set to -1 nothing is active
        */
        void Set_Active(int num);

        // Updates the Menu Mouse Collision detection
        void Update_Mouse(void);

        // Update
        void Update(void);
        // Draw
        void Draw(bool with_background = 1);

        inline cHudSprite* Get_TSC_Logo(){ return mp_tsc_logo; }

        // Returns the currently active Menu Item
        void* Get_Active_Item(void);
        // Returns the rect of the currently active menu item
        sf::FloatRect Get_Active_Item_Rect(void);
        // Returns the number of loaded Menus
        unsigned int Get_Size(void) const;

        // menu camera
        cCamera* m_camera;
        // menu level
        cLevel* m_level;
        // menu player (currently only used to set the pActive_Player)
        cSprite* m_player;

        /* The currently active Menu Item
        * if set to -1 nothing is active
        */
        int m_active;

    private:
        struct MenuItem {
            sf::FloatRect m_rect;
            void* mp_item;
        };
        typedef vector<struct MenuItem> MenuList;

        // Returns the path to the menu level, taking user config into account.
        boost::filesystem::path menu_level_path();

        MenuList m_items;
        cHudSprite* mp_tsc_logo;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

    class cMenuCore {
    public:
        cMenuCore(void);
        ~cMenuCore(void);

        // Handle Input event
        bool Handle_Event(const sf::Event& ev);
        /* handle key down event
         * returns true if processed
        */
        bool Key_Down(const sf::Event& evt);
        /* handle key up event
         * returns true if processed
        */
        bool Key_Up(const sf::Event& evt);
        /* handle mouse button down event
         * returns true if processed
        */
        bool Mouse_Down(sf::Mouse::Button button);
        /* handle mouse button up event
         * returns true if processed
        */
        bool Mouse_Up(sf::Mouse::Button button);
        /* handle joystick button down event
         * returns true if processed
        */
        bool Joy_Button_Down(unsigned int button);
        /* handle joystick button up event
         * returns true if processed
        */
        bool Joy_Button_Up(unsigned int button);


        /* Load the given Menu
         * exit_gamemode : return to this game mode on exit
        */
        void Load(const MenuID menu = MENU_MAIN, const GameMode exit_gamemode = MODE_NOTHING);
        // Unload
        void Unload(void);

        // Enter game mode
        void Enter(const GameMode old_mode = MODE_NOTHING);
        // Leave game mode
        void Leave(const GameMode next_mode = MODE_NOTHING);

        // Update current Menu
        void Update(void);
        // Draw current Menu
        void Draw(void);

        // current menu id
        MenuID m_menu_id;

        // Menu class
        cMenu_Base* m_menu_data;

        // Menu handler
        cMenuHandler* m_handler;
        // Menu Animation Manager
        cAnimation_Manager* m_animation_manager;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

// The Menu
    extern cMenuCore* pMenuCore;

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

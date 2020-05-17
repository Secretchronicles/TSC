/***************************************************************************
 * scene.hpp
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

#ifndef TSC_SCENE_HPP
#define TSC_SCENE_HPP

#include "../core/global_basic.hpp"
#include "../core/global_game.hpp"

namespace TSC {

    // forward-declare
    namespace SceneActions {
        class Action;
    }

    class cScene {
    public:
        cScene(void);
        virtual ~cScene(void);

        /* Allocates the appropriate instance of a subclass of cScene
         * for the given scene name and returns a pointer to the
         * allocated storage. Do not use directly; this function is
         * only meant to be called from the main game loop.
         * Returns NULL if the scene name does not exist. */
        static cScene* Load_Scene(std::string name);

        void Enter(const GameMode old_mode = MODE_NOTHING);
        void Leave(const GameMode next_mode = MODE_NOTHING);

        void Update(void);
        void Draw(void);

        /* When the scene finishes, initiate the given GameAction.
         * Supported are:
         * * GA_ENTER_LEVEL (default). Loads the level `name` and places
         *   Alex at the level entry `arg`. If `name` is empty, resume
         *   the current level. You can still use `arg` to transfer
         *   Alex to another level entry in the current level. If there
         *   is no current level, crashes if `name` is empty.
         * * GA_ENTER_WORLD: Loads the overworld `name` and places Alex
         *   at the initial waypoint. If `name` is empty, resumes the
         *   active overworld (crashes if there is none). If `arg` is
         *   not empty, advance to the next waypoint via the waypoint
         *   exit named `arg`. `arg` is ignored if `name` is not empty.
         * * GA_ENTER_MENU: Show the credits screen. `name` and `arg`
         *   arguments are ignored.
         * * GA_ENTER_SCENE: Switch to another scene denoted with
         *   `name`. `arg` is ignored.
         */
        void Set_Next_Game_Action(enum GameAction next, std::string name = "", std::string arg = "");

        // Set the background image of the scene
        // scene_image is relative to the pixmaps/ directory.
        void Set_Scene_Image(std::string scene_image);

        /* These functions control the story box shown at the
         * bottom of the screen. Note that calling Set_Story_Text()
         * does not automatically show the box. Set_Story_Text()'s
         * argument will automatically be broken when the text hits
         * the box' right edge. It can also make use of colours
         * and all other formatting directives documented at
         * <http://cegui.org.uk/wiki/Formatting_Tags_in_CEGUI>. */
        void Show_Story_Box();
        void Hide_Story_Box();
        void Set_Story_Text(std::string text);

        /* handle key down event
         * returns true processed
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

        /* Name of the scene, set by subclasses as `m_name' */
        std::string Get_Name();

    private:
        void End_Scene();

        cSprite_Manager* mp_sprite_manager;
        cCamera* mp_camera;

        // The scene's background image, i.e. the scene image that is in use.
        cSprite* mp_scene_image;
        // The story narration box shown in the lower third of the window.
        CEGUI::Window* mp_story_box;

        // Next mode to start
        enum GameAction m_next_game_action;
        std::string m_next_name;
        std::string m_next_arg;

    protected:
        // The actions to play in this scene, one after the other.
        vector<SceneActions::Action*> m_action_sequence;
        std::string m_name;
    };

    extern cScene* pActive_Scene;
}

#endif /* TSC_SCENE_HPP */

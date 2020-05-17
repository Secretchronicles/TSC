/***************************************************************************
 * scene_actions.hpp
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

#ifndef TSC_SCENE_ACTIONS_HPP
#define TSC_SCENE_ACTIONS_HPP
#include "../core/global_basic.hpp"
#include "../core/global_game.hpp"

namespace TSC {

    // forward-declare
    class cScene;

    namespace SceneActions {

        /// Parent class of all scene actions.
        class Action {
        public:
            Action(cScene* p_scene);
            virtual ~Action(void);

            /* This method is called once the action is active. If
             * it returns true, the action is treated as terminated.
             * If it returns false, the action is assumed to be still
             * running. If false is returned, Execute() will be called
             * again once a frame (in cScene::Update()). Prepare for
             * that. Execute() is mostly Update() for actions. */
            virtual bool Execute() = 0;

            /* Only if Execute() returned false: the following event
             * handlers will be called when this action is active.
             * They do nothing by default. Please return true from a
             * handler if you handled the event (otherwise it will get
             * passed by the event loop to other parts of TSC). */
            virtual bool Key_Down(const sf::Event& evt) {return false;}
            virtual bool Key_Up(const sf::Event& evt) {return false;}
            virtual bool Joy_Button_Down(unsigned int button) {return false;}
            virtual bool Joy_Button_Up(unsigned int button) {return false;}
            virtual bool Mouse_Down(sf::Mouse::Button button) {return false;}
            virtual bool Mouse_Up(sf::Mouse::Button button) {return false;}

            cScene* mp_scene;
        };

        /// Change the full-screen background image to the given image.
        /// Path needs to be relative to pixmaps/.
        class ImageChange: public Action {
        public:
            ImageChange(cScene* p_scene, std::string scene_image);
            virtual bool Execute();
            std::string m_scene_image;
        };

        /// Change background music to the specified music.
        class MusicChange: public Action {
        public:
            MusicChange(cScene* p_scene, std::string music);
            virtual bool Execute();
            std::string m_music;
        };

        /// Wait until the user presses Return or Action or Shoot.
        class WaitReturn: public Action {
        public:
            WaitReturn(cScene* p_scene);
            virtual bool Execute();
            virtual bool Key_Down(const sf::Event& evt);
        private:
            bool m_return_pressed;
        };

        /// Wait approximiately the given amount of seconds.
        class WaitTime: public Action {
        public:
            WaitTime(cScene* p_scene, float seconds);
            virtual bool Execute();
        private:
            float m_wait_counter;
        };

        /**
         * Narrate story. Each element in `messages` in the constructor is one
         * story box content (and thus needs to fit in there -- consider
         * translations may take up more space than English). Lines are
         * broken automatically by CEGUI.
         */
        class Narration: public Action {
        public:
            Narration(cScene* p_scene, std::initializer_list<std::string> messages);
            virtual bool Execute();
            virtual bool Key_Down(const sf::Event& evt);
        private:
            std::vector<std::string> m_messages;
            bool m_read;
        };

        /**
         * Configure what happens after the scene. The arguments are
         * forwarded to cScene::Set_Next_Level().
         */
        class NextUp: public Action {
        public:
            NextUp(cScene* p_scene, enum GameAction ga, std::string name = "", std::string arg = "");
            virtual bool Execute();
        private:
            enum GameAction m_ga;
            std::string m_name;
            std::string m_arg;
        };

    }

}

#endif /* TSC_SCENE_ACTIONS_HPP */

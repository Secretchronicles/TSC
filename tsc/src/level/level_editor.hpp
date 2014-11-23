/***************************************************************************
 * level_editor.h
 *
 * Copyright © 2006 - 2011 Florian Richter
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

#ifndef TSC_LEVEL_EDITOR_HPP
#define TSC_LEVEL_EDITOR_HPP

#include "../core/editor/editor.hpp"
#include "../level/level_settings.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** cEditor_Level *** *** *** *** *** *** *** *** *** *** */

    class cEditor_Level : public cEditor {
    public:
        cEditor_Level(cSprite_Manager* sprite_manager, cLevel* level);
        virtual ~cEditor_Level(void);

        // Initialize
        virtual void Init(void);

        // Enable
        virtual void Enable(void);
        /* Disable
         * native_mode : if unset the current game mode isn't altered
        */
        virtual void Disable(bool native_mode = 0);


        /* handle key down event
         * returns true if the key was processed
        */
        virtual bool Key_Down(SDLKey key);

        // Set the parent level
        void Set_Level(cLevel* level);
        // Set the parent sprite manager
        virtual void Set_Sprite_Manager(cSprite_Manager* sprite_manager);

        // Set Active Menu Entry
        virtual void Activate_Menu_Item(cEditor_Menu_Object* entry);

        // #### editor Functions
        /* switch the object state of the given object
         * returns true if successful
        */
        bool Switch_Object_State(cSprite* obj) const;

        // Menu functions
        virtual bool Function_New(void);
        virtual void Function_Load(void);
        virtual void Function_Save(bool with_dialog = 0);
        virtual void Function_Save_as(void);
        virtual void Function_Delete(void);
        virtual void Function_Reload(void);
        virtual void Function_Settings(void);

        // parent level
        cLevel* m_level;
        // Level Settings
        cLevel_Settings* m_settings_screen;
    protected:
        static std::vector<cSprite*> items_loader_callback(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager, void* p_data);
        virtual void Parse_Items_File(boost::filesystem::path filename);
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

// The Level Editor
    extern cEditor_Level* pLevel_Editor;

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

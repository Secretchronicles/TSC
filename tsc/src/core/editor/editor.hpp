/***************************************************************************
 * editor.hpp
 *
 * Copyright © 2006 - 2011 Florian Richter
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

#ifndef TSC_EDITOR_HPP
#define TSC_EDITOR_HPP

namespace TSC {
    class cEditor_Menu_Entry {
    public:
        cEditor_Menu_Entry(std::string name);
        ~cEditor_Menu_Entry();

        void Add_Item(cSprite* p_template_sprite, std::string cegui_img_ident, std::string name, CEGUI::Quaternion rotation); // FIXME: Must take std::vector<cSprite*> due to multi-sprite objects
        void Activate(CEGUI::TabControl* p_tabcontrol);

        inline void Set_Color(Color color){ m_color = color; }
        inline Color Get_Color(){ return m_color; }

        inline std::string Get_Name(){ return m_name; }
        inline const CEGUI::utf8* Get_CEGUI_Name() { return reinterpret_cast<const CEGUI::utf8*>(m_name.c_str()); }

        inline void Set_Header(bool is_header){ m_is_header = is_header; }
        inline void Set_Function(bool is_function){ m_is_function = is_function; }
        inline bool Is_Header() { return m_is_header; }
        inline bool Is_Function() { return m_is_function; }

        inline void Set_Required_Tags(std::vector<std::string> tags) { m_required_tags = tags; }
        inline std::vector<std::string>& Get_Required_Tags() { return m_required_tags; }

        inline CEGUI::ScrollablePane* Get_CEGUI_Pane() { return mp_tab_pane; }

    private:
        std::string m_name;
        Color m_color;
        bool m_is_header;
        bool m_is_function;
        std::vector<std::string> m_required_tags;
        CEGUI::ScrollablePane* mp_tab_pane;
        int m_element_y;

        bool on_image_mouse_down(const CEGUI::EventArgs& ev);
    };

    class cEditor {
    public:
        cEditor();
        virtual ~cEditor(void);

        virtual void Init(void);
        virtual void Unload(void);

        void Toggle(cSprite_Manager* p_sprite_manager);
        virtual void Enable(cSprite_Manager* p_edited_sprite_manager);
        virtual void Disable(void);

        // These methods are for interacting with the object config panel on
        // the right side of the editor.

        /// Add a label widget and its corresponding value widget to the panel.
        void Add_Config_Widget(const CEGUI::String& name, const CEGUI::String& tooltip, CEGUI::Window* p_settings_widget, float obj_height = 28.0f);
        /// Show the panel to the user, showing all widgets added with Add_Config_Widget().
        void Show_Config_Panel();
        /// Hide the panel from the user. This method destroys all widgets in the panel so you'll
        /// need to use Add_Config_Widget() again.
        void Hide_Config_Panel();
        /// Is the config panel shown to the user?
        inline bool Is_Config_Panel_Shown(){ return m_object_config_pane_shown; }

        bool Try_Add_Image_Item(boost::filesystem::path settings_path, const cImage_Settings_Data* p_settings);
        bool Try_Add_Special_Item(cSprite* p_sprite); // FIXME: Must take std::vector<cSprite*> due to multi-sprite objects
        void Select_Same_Object_Types(const cSprite* obj);

        void Process_Input(void);

        // This method has to return an identifier for display in
        // the status bar about what is being edited at this moment.
        virtual std::string Status_Bar_Ident() const = 0;

        virtual void Update(void);
        virtual void Draw(void);

        virtual bool Mouse_Down(sf::Mouse::Button button);
        virtual bool Mouse_Up(sf::Mouse::Button button);
        virtual bool Mouse_Move(const sf::Event& evt);
        virtual bool Key_Down(const sf::Event& evt);

        bool m_enabled;
        bool m_object_config_pane_shown;
    protected:
        std::string m_editor_item_tag;
        boost::filesystem::path m_menu_filename;
        // This sprite manager is a reference to the edited level's/world's
        // sprite manager and used when interacting with the level.
        cSprite_Manager* mp_edited_sprite_manager;
        // This sprite manager holds the template objects from which sprites
        // are created when an object is dragged from the sidebar into the level.
        // I.e. it holds the objects in the sidebar.
        cSprite_Manager m_sprite_manager;

        // Menu functions
        void Activate_Function_Entry(cEditor_Menu_Entry* p_function_entry);
        void Function_Exit(void);
        virtual bool Function_New(void) { return 0; };
        virtual void Function_Load(void) {};
        virtual void Function_Save(bool with_dialog = 0) {};
        virtual void Function_Save_as(void) {};
        virtual void Function_Delete(void) {};
        virtual void Function_Settings(void) {};

        // Loads the editor/*_items.xml file that corresponds to this
        // editor. Override in a subclass and employ cEditorItemsLoader
        // to parse the file and return its result. Use the sprite
        // manager available in `m_sprite_manager' to store the resulting
        // template cSprite objects.
        virtual vector<cSprite*> Parse_Items_File() = 0;

    private:
        CEGUI::Window* mp_editor_root;
        CEGUI::TabControl* mp_editor_tabpane;
        CEGUI::Listbox* mp_menu_listbox;
        CEGUI::RadioButton* mp_selector_radio_selection;
        CEGUI::RadioButton* mp_selector_radio_secretarea;
        CEGUI::Window* mp_object_config_pane;
        CEGUI::Window* mp_status_bar;
        float m_visibility_timer;
        CEGUI::UDim m_tabpane_target_x_position;
        CEGUI::UDim m_object_config_pane_target_x_position;
        bool m_rested;
        bool m_mouse_inside;
        std::vector<CEGUI::Window*> m_editor_items;
        std::vector<cEditor_Menu_Entry*> m_menu_entries;
        bool m_help_window_visible;
        const int CAMERA_SPEED = 35;

        void parse_menu_file();
        void populate_menu();
        void load_image_items();
        void load_special_items();
        std::vector<cEditor_Menu_Entry*> find_target_menu_entries_for(const std::vector<std::string>& available_tags);
        cSprite_List copy_direction(const cSprite_List& objects, const ObjectDirection dir) const;
        cSprite* copy_direction(const cSprite* obj, const ObjectDirection dir, int offset /* = 0 */) const;
        void replace_sprites(void);
        std::string load_cegui_image(boost::filesystem::path);
        void update_status_bar();
        bool on_mouse_enter(const CEGUI::EventArgs& event);
        bool on_mouse_leave(const CEGUI::EventArgs& event);
        bool on_menu_selection_changed(const CEGUI::EventArgs& event);
        bool on_help_window_exit_clicked(const CEGUI::EventArgs& args);
        bool on_selector_radio_changed(const CEGUI::EventArgs& event);
    };
}

#endif

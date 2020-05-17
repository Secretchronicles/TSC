/***************************************************************************
 * save_level.cpp - Handler for saving a cLevel instance to a savegame.
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

#include "save_level.hpp"
#include "../../core/game_core.hpp"

using namespace TSC;

/* *** *** *** *** *** *** *** cSave_Level_Object *** *** *** *** *** *** *** *** *** *** */
cSave_Level_Object_Property::cSave_Level_Object_Property(const std::string& new_name /* = "" */, const std::string& new_value /* = "" */)
{
    m_name = new_name;
    m_value = new_value;
}

/* *** *** *** *** *** *** *** cSave_Level_Object *** *** *** *** *** *** *** *** *** *** */
cSave_Level_Object::cSave_Level_Object(void)
{
    m_type = TYPE_UNDEFINED;
}

cSave_Level_Object::~cSave_Level_Object(void)
{
    m_properties.clear();
}

bool cSave_Level_Object::exists(const std::string& val_name)
{
    for (Save_Level_Object_ProprtyList::iterator itr = m_properties.begin(); itr != m_properties.end(); ++itr) {
        cSave_Level_Object_Property obj = (*itr);
        if (obj.m_name.compare(val_name) == 0) {
            // found
            return 1;
        }
    }
    // not found
    return 0;
}

std::string cSave_Level_Object::Get_Value(const std::string& val_name)
{
    for (Save_Level_Object_ProprtyList::iterator itr = m_properties.begin(); itr != m_properties.end(); ++itr) {
        cSave_Level_Object_Property obj = (*itr);
        if (obj.m_name.compare(val_name) == 0) {
            // found
            return obj.m_value;
        }
    }
    // not found
    return "";
}

/* *** *** *** *** *** *** *** cSave_Level *** *** *** *** *** *** *** *** *** *** */

cSave_Level::cSave_Level(void)
{
    // level
    m_level_pos_x = 0.0f;
    m_level_pos_y = 0.0f;
}

cSave_Level::~cSave_Level(void)
{
    m_regular_objects.clear();
    m_spawned_objects.clear();
}

void cSave_Level::Save_To_Node(xmlpp::Element* p_parent_node)
{
    // <level>
#ifdef USE_LIBXMLPP3
    xmlpp::Element* p_node = p_parent_node->add_child_element("level");
    Add_Property(p_node, "level_name", m_name);
#else
    xmlpp::Element* p_node = p_parent_node->add_child("level");
    Add_Property(p_node, "level_name", m_name);
#endif

    // Player position. Only save that for the active level.
    if (!Is_Float_Equal(m_level_pos_x, 0.0f) && !Is_Float_Equal(m_level_pos_y, 0.0f)) {
        Add_Property(p_node, "player_posx", m_level_pos_x);
        Add_Property(p_node, "player_posy", m_level_pos_y);
    }

    /* Custom data a script writer wants to store; empty if the
     * script writer didn’t hook into the on_load and on_save
     * events. */
#ifdef USE_LIBXMLPP3
    if (!m_script_datas.empty()) {
        xmlpp::Element* p_datas_node = p_node->add_child_element("mruby_data");
        for(const Script_Data& data: m_script_datas) {
            xmlpp::Element* p_data_node = p_datas_node->add_child_element("script_data");

            for(auto iter=data.begin(); iter != data.end(); iter++) {
                xmlpp::Element* p_entry_node = p_data_node->add_child_element("script_data_entry");
                p_entry_node->set_attribute("name", iter->first);
                p_entry_node->set_attribute("type", std::get<0>(iter->second));
                p_entry_node->set_attribute("value", std::get<1>(iter->second));
            }
        }
    }
#else
    if (!m_script_datas.empty()) {
        xmlpp::Element* p_datas_node = p_node->add_child("mruby_data");
        for(const Script_Data& data: m_script_datas) {
            xmlpp::Element* p_data_node = p_datas_node->add_child("script_data");

            for(auto iter=data.begin(); iter != data.end(); iter++) {
                xmlpp::Element* p_entry_node = p_data_node->add_child("script_data_entry");
                p_entry_node->set_attribute("name", iter->first);
                p_entry_node->set_attribute("type", std::get<0>(iter->second));
                p_entry_node->set_attribute("value", std::get<1>(iter->second));
            }
        }
    }
#endif

    // The regular objects.
    // <objects_data>
#ifdef USE_LIBXMLPP3
    xmlpp::Element* p_objects_data_node = p_node->add_child_element("objects_data");
#else
    xmlpp::Element* p_objects_data_node = p_node->add_child("objects_data");
#endif
    std::vector<const cSprite*>::const_iterator iter;
    for(iter=m_regular_objects.begin(); iter != m_regular_objects.end(); iter++) {
        xmlpp::Document subdoc;
        xmlpp::Element* p_object_node = subdoc.create_root_node("object");
        const cSprite* p_sprite = (*iter);

        /* Let the sprite itself decide whether it wants to be saved.
         * If the virtual method Save_To_Savegame_XML_Node() returns false,
         * no saving shall be done, the created XML node is ignored and not
         * used. If the method returns true, we add in the created node. */
        if (p_sprite->Save_To_Savegame_XML_Node(p_object_node)) {
            p_objects_data_node->import_node(p_object_node);
        }
    }
    // </objects_data>

    // The spawned objects. These have always to be saved.
    // <spawned_objects>
#ifdef USE_LIBXMLPP3
    xmlpp::Element* p_spawned_node = p_node->add_child_element("spawned_objects");
#else
    xmlpp::Element* p_spawned_node = p_node->add_child("spawned_objects");
#endif
    cSprite_List::iterator iter2; // TODO: Should be const_iterator
    for(iter2=m_spawned_objects.begin(); iter2 != m_spawned_objects.end(); iter2++) {
        cSprite* p_sprite = (*iter2);
        p_sprite->Save_To_XML_Node(p_spawned_node);
    }
    // </spawned_objects>

    //</level>
}

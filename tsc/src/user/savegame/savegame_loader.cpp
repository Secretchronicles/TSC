/***************************************************************************
 * savegame_loader.cpp - Loading savegame XML
 *
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

#include "../../core/i18n.hpp"
#include "../../level/level_loader.hpp"
#include "../../overworld/world_manager.hpp"
#include "savegame_loader.hpp"
#include "savegame.hpp"
#include "../../core/global_basic.hpp"
#include "../../level/level_player.hpp"

// Maximum number of waypoint exits is 4. One for each direction.
#define MAX_WAYPOINT_EXITS 4

namespace fs = boost::filesystem;
using namespace std;
using namespace TSC;

cSavegameLoader::cSavegameLoader()
    : xmlpp::SaxParser()
{
    mp_save = NULL;
}

cSavegameLoader::~cSavegameLoader()
{
    // Do not delete the cSave instance — it is
    // used by the caller and deleted by him.
    mp_save = NULL;
}

cSave* cSavegameLoader::Get_Save()
{
    return mp_save;
}

/***************************************
 * SAX parser callbacks
 ***************************************/

void cSavegameLoader::parse_file(fs::path filename)
{
    m_savefile = filename;

    //Note: If changes are made to the .tscsav format but not the .smcsav format, the below logic will need to change
    m_is_old_format = m_savefile.extension() == utf8_to_path(".tscsav") || m_savefile.extension() == utf8_to_path(".smcsav") ? false : true;

    xmlpp::SaxParser::parse_file(path_to_utf8(filename));
}

void cSavegameLoader::on_start_document()
{
    if (mp_save)
        throw("Restarted XML parser after already starting it."); // FIXME: Proper exception

    mp_save = new cSave();
}

void cSavegameLoader::on_end_document()
{
    // if no description is set
    if (mp_save->m_description.empty())
        mp_save->m_description = _("No description");
}

void cSavegameLoader::on_start_element(const Glib::ustring& name, const xmlpp::SaxParser::AttributeList& properties)
{
    if (name == "property" || name == "Property") {
        std::string key;
        std::string value;

        /* Collect all the <property> elements for the surrounding
         * mayor element (like <settings> or <sprite>). When the
         * surrounding element is closed, the results are handled
         * in on_end_element(). */
        for (xmlpp::SaxParser::AttributeList::const_iterator iter = properties.begin(); iter != properties.end(); iter++) {
            xmlpp::SaxParser::Attribute attr = *iter;

            if (attr.name == "name" || attr.name == "Name")
                key = attr.value;
            else if (attr.name == "value" || attr.name == "Value")
                value = attr.value;
        }

        m_current_properties[key] = value;
    }
    else if (name == "script_data_entry") {
        std::string key;
        std::string type;
        std::string value;

        for (xmlpp::SaxParser::AttributeList::const_iterator iter = properties.begin(); iter != properties.end(); iter++) {
            xmlpp::SaxParser::Attribute attr = *iter;

            if (attr.name == "name")
                key = attr.value;
            else if (attr.name == "type")
                type = attr.value;
            else if (attr.name == "value")
                value = attr.value;
        }

        m_current_script_data[key] = std::make_pair(type, value);
    }
}

void cSavegameLoader::on_end_element(const Glib::ustring& name)
{
    // Already handled
    if (name == "property" || name == "Property")
        return;

    // Ignore the root element
    if (name == "savegame" || name == "Savegame")
        return;

    if (name == "information" || name == "Information")
        handle_information();
    else if (name == "level" || name == "Level")
        handle_level();
    else if (name == "objects_data")
        return; // don’t clear attributes
    else if (name == "object" || name == "Level_Object") {
        handle_level_object();
        return; // don’t clear attributes
    }
    else if (name == "spawned_objects")
        return; // don't clear attributes
    else if (name == "mruby_data")
        return; // idon't clear attributes for <level>
    else if (name == "script_data") {
        handle_script_data();
        return; // don't clear attributes for <level>
    }
    else if (name == "script_data_entry")
        return; // don't clear attributes for <level>
    else if (name != "player" && cLevel::Is_Level_Object_Element(std::string(name))) { // Glib::ustring is not autoconverted to CEGUI::String
        handle_level_spawned_object(name);
        return; // don't clear attributes
    }
    else if (name == "player" || name == "Player")
        handle_player();
    else if (name == "return")
        handle_return();
    else if (m_is_old_format && name == "Overworld_Data")
        handle_old_format_overworld_data();
    else if (name == "overworld" || name == "Overworld")
        handle_overworld();
    else if (name == "waypoint" || name == "Overworld_Level") {
        handle_overworld_waypoint();
        return; // don’t clear attributes to keep the world "name"
    }
    else
        cerr << "Warning: Unknown savegame element '" << name << "'" << endl;

    m_current_properties.clear();
}

void cSavegameLoader::handle_information()
{
    mp_save->m_version              = m_current_properties.retrieve<int>("version");
    mp_save->m_level_engine_version = m_current_properties.fetch<int>("level_engine_version", mp_save->m_level_engine_version);
    mp_save->m_save_time            = string_to_int64(m_current_properties["save_time"]);
    mp_save->m_description          = m_current_properties["description"];
}

void cSavegameLoader::handle_level()
{
    cSave_Level* p_savelevel = new cSave_Level();

    // Restore the general attributes.
    p_savelevel->m_name         = m_current_properties["level_name"];
    p_savelevel->m_level_pos_x  = m_current_properties.fetch<float>("player_posx", 0);
    p_savelevel->m_level_pos_y  = m_current_properties.fetch<float>("player_posy", 0);

    /* Restore object lists. Note the lists in `p_savelevel' are
     * currently empty (it’s a new object) and hence swapping
     * with them consequently means clearing the swapping
     * partner. */
    // set level objects, clear object list for the next level
    p_savelevel->m_level_objects.swap(m_level_objects);
    // set level spawned objects, clear object list for the next level
    p_savelevel->m_spawned_objects.swap(m_level_spawned_objects);
    // Copy all storages into the savelevel object (there's one storage
    // per registerd save_load handler). Clear data list for next level.
    p_savelevel->m_script_datas.swap(m_script_datas);

    // Add this level to the list of levels for this
    // savegame.
    mp_save->m_levels.push_back(p_savelevel);
}

void cSavegameLoader::handle_script_data()
{
    // Place a copy of all the deserialised storage into the script
    // data list, then clear the working list for the next storage
    // (each save_load handler has its own storage).
    m_script_datas.push_back(m_current_script_data);
    m_current_script_data.clear();
}

void cSavegameLoader::handle_level_object()
{
    int type = m_current_properties.retrieve<int>("type");
    if (type <= 0) {
        cerr << "Warning: Unknown level object type '" << type << "'" << endl;
        return;
    }

    cSave_Level_Object* p_object = new cSave_Level_Object();

    // type
    p_object->m_type = static_cast<SpriteType>(type);

    // Get properties
    XmlAttributes::const_iterator iter;
    for (iter = m_current_properties.begin(); iter != m_current_properties.end(); iter++) {

        // Ignore level attributes
        if (iter->first == "level_name" || iter->first == "player_posx" || iter->first == "player_posy" || iter->first == "mruby_data")
            continue;

        p_object->m_properties.push_back(cSave_Level_Object_Property(iter->first, iter->second));
    }

    /* We are in a nested tag situation (<object> in <objects_data>).
     * We now clear only the properties specific to <object> so the
     * <property>s found in the upper <objects_data> node are preserved
     * for the next <object>. */
    m_current_properties.erase("type");
    // remove used properties
    Save_Level_Object_ProprtyList::const_iterator iter2;
    for (iter2 = p_object->m_properties.begin(); iter2 != p_object->m_properties.end(); iter2++) {
        cSave_Level_Object_Property prop = (*iter2);
        m_current_properties.erase(prop.m_name);
    }

    // add object for handling in handle_level()
    m_level_objects.push_back(p_object);
}

void cSavegameLoader::handle_level_spawned_object(const Glib::ustring& name)
{
    std::vector<cSprite*> sprites = cLevelLoader::Create_Level_Objects_From_XML_Tag(name, m_current_properties, mp_save->m_level_engine_version, pActive_Level->m_sprite_manager);

    // add objects for later handling in handle_level()
    std::vector<cSprite*>::iterator iter;
    for (iter=sprites.begin(); iter != sprites.end(); iter++)
        m_level_spawned_objects.push_back(*iter);
}

void cSavegameLoader::handle_player()
{
    //Note: fetch defaults to a value if an xml element is not found rather than throw an exception as retrieve does
    //This allows reverse compatibility with old save formats
    mp_save->m_lives                    = m_current_properties.retrieve<int>("lives");
    mp_save->m_points                   = m_current_properties.fetch<long>("points", 0);
    mp_save->m_goldpieces               = m_current_properties.retrieve<int>("goldpieces");
    mp_save->m_player_type              = m_current_properties.fetch<int>("type", ALEX_BIG);
    mp_save->m_player_type_temp_power   = m_current_properties.fetch<int>("type_temp_power", ALEX_SMALL);
    mp_save->m_invincible               = m_current_properties.fetch<float>("invincible", 0);
    mp_save->m_invincible_star          = m_current_properties.fetch<float>("invincible_star", 0);
    //Pre Version 2.0 files did not save the ghost time.  For such files, default the ghost time to 10 seconds if the saved player type is a ghost type and 0 it not
    //Properly setting this prevents a weird display color for Alex for old save files that saved with ghost Alex
    mp_save->m_ghost_time               = m_current_properties.fetch<float>("ghost_time", (mp_save ->m_player_type == ALEX_GHOST ? speedfactor_fps * 10 : 0));
    mp_save->m_ghost_time_mod           = m_current_properties.fetch<float>("ghost_time_mod", 0);
    mp_save->m_player_state             = m_current_properties.retrieve<int>("state");
    mp_save->m_itembox_item             = m_current_properties.retrieve<int>("itembox_item");

    // New in V.11
    if (m_current_properties.exists("level_time"))
        mp_save->m_level_time = m_current_properties.retrieve<int>("level_time");

    // See handle_old_format_overworld_data() for the old format handler
    if (!m_is_old_format) {
        mp_save->m_overworld_active = m_current_properties["overworld_active"];
        mp_save->m_overworld_current_waypoint = m_current_properties.retrieve<int>("overworld_current_waypoint");
    }
}

void cSavegameLoader::handle_return()
{
    cSave_Player_Return_Entry e(m_current_properties["level"], m_current_properties["entry"]);

    mp_save->m_return_entries.push_back(e);
}

// Handles savegame format V.10 and lower. See also format check
// in handle_player().
void cSavegameLoader::handle_old_format_overworld_data()
{
    mp_save->m_overworld_active = m_current_properties["active"];
    mp_save->m_overworld_current_waypoint = m_current_properties.retrieve<int>("current_waypoint");
}

void cSavegameLoader::handle_overworld()
{
    std::string name = m_current_properties["name"];

    // is overworld available? We can probably ignore this for in-level saves
    if (!pOverworld_Manager->Get_from_Name(name))
        cerr << "Warning: Overworld '" << name << "' in savegame '" << mp_save->m_description << "' could not be found. Trying to continue anyway..." << endl;

    // Create savegame overworld
    cSave_Overworld* p_saveoverworld = new cSave_Overworld();
    p_saveoverworld->m_name = name;
    p_saveoverworld->m_waypoints.swap(m_waypoints); // Clears m_waypoints for the next overworld
    // save
    mp_save->m_overworlds.push_back(p_saveoverworld);
}

void cSavegameLoader::handle_overworld_waypoint()
{
    cSave_Overworld_Waypoint* p_waypoint = new cSave_Overworld_Waypoint();

    // destination ( level_name and world_name is pre V.0.99.6 )
    if (m_current_properties.exists("world_name"))
        p_waypoint->m_destination = m_current_properties["world_name"];
    else if (m_current_properties.exists("level_name"))
        p_waypoint->m_destination = m_current_properties["level_name"];
    else // default
        p_waypoint->m_destination = m_current_properties["destination"];

    p_waypoint->m_access = m_current_properties.retrieve<bool>("access");

    // Since 2.1.0: alternate path lock state
    for(int i=0; i < MAX_WAYPOINT_EXITS; i++) {
        std::string locked_attr = "waypoint_exit_" + int_to_string(i) + "_locked";
        if (m_current_properties.exists(locked_attr)) {
            waypoint_exit ex;
            ex.locked = m_current_properties.fetch<bool>(locked_attr, true);
            p_waypoint->m_exits.push_back(ex);

            // Clear attribute for the next waypoint (nested XML we need to take care of)
            m_current_properties.erase(locked_attr);
        }
    }

    m_waypoints.push_back(p_waypoint);

    // Clear attributes for the next waypoint (nested XML we need to take care of)
    m_current_properties.erase("destination");
    m_current_properties.erase("level_name");
    m_current_properties.erase("world_name");
    m_current_properties.erase("access");
}

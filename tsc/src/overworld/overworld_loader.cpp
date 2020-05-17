/***************************************************************************
 * overworld_loader.cpp - Loading overworld XML
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

#include "../video/gl_surface.hpp"
#include "../core/filesystem/resource_manager.hpp"
#include "overworld_loader.hpp"
#include "overworld_description_loader.hpp"
#include "overworld.hpp"
#include "../core/global_basic.hpp"

namespace fs = boost::filesystem;
using namespace TSC;

using namespace std;

cOverworldLoader::cOverworldLoader()
    : xmlpp::SaxParser()
{
    mp_overworld = NULL;
}

cOverworldLoader::~cOverworldLoader()
{
    // mp_overworld must be deleted by caller
    mp_overworld = NULL;
}

cOverworld* cOverworldLoader::Get_Overworld()
{
    return mp_overworld;
}

/***************************************
 * SAX parser callbacks
 ***************************************/

void cOverworldLoader::parse_file(fs::path filename)
{
    m_worldfile = filename;
    xmlpp::SaxParser::parse_file(path_to_utf8(m_worldfile));
}

void cOverworldLoader::on_start_document()
{
    if (mp_overworld)
        throw("Restarted XML parser after already starting it."); // FIXME: proper exception

    mp_overworld = new cOverworld();
}

void cOverworldLoader::on_end_document()
{
    // engine version entry not set
    if (mp_overworld->m_engine_version < 0)
        mp_overworld->m_engine_version = 0;
}

void cOverworldLoader::on_start_element(const Glib::ustring& name, const xmlpp::SaxParser::AttributeList& properties)
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
}

void cOverworldLoader::on_end_element(const Glib::ustring& name)
{
    // <property> tags are parsed cumulatively in on_start_element()
    // so all have been collected when the surrounding element
    // terminates here.
    if (name == "property" || name == "Property")
        return;
    // Ignore the root tag itself
    if (name == "overworld")
        return;

    if (name == "information")
        Parse_Tag_Information();
    else if (name == "settings")
        Parse_Tag_Settings();
    else if (name == "player")
        Parse_Tag_Player();
    else if (name == "background")
        Parse_Tag_Background();
    else {
        cSprite* p_object = Create_World_Object_From_XML(name, m_current_properties, mp_overworld->m_engine_version, mp_overworld->m_sprite_manager, mp_overworld);

        // valid
        if (p_object)
            mp_overworld->m_sprite_manager->Add(p_object);
        else
            cerr << "Warning: Unknown overworld element '" << name << "'" << endl;
    }

    // Everything handled, so we can now safely clear the
    // collected <property> element values for the next
    // tag.
    m_current_properties.clear();
}

/***************************************
 * Main tag parsers
 ***************************************/

void cOverworldLoader::Parse_Tag_Information()
{
    mp_overworld->m_engine_version = string_to_int(m_current_properties["engine_version"]);
    mp_overworld->m_last_saved = string_to_int64(m_current_properties["save_time"]);
}

void cOverworldLoader::Parse_Tag_Settings()
{
    // Author
    //mp_overworld->author = m_current_properties["author"];
    // Version
    //mp_overworld->version = m_current_properties["version"];
    // Music
    mp_overworld->m_musicfile = m_current_properties["music"];
    // Camera Limits
    //pOverworldManager->camera->Set_Limits(GL_rect(    m_current_properties.fetch<int>("cam_limit_x"),
    //                                              m_current_properties.fetch<int>("cam_limit_y"),
    //                                              m_current_properties.fetch<int>("cam_limit_w"),
    //                                              m_current_properties.fetch<int>("cam_limit_h")));
}

void cOverworldLoader::Parse_Tag_Player()
{
    // Start Waypoint
    mp_overworld->m_player_start_waypoint = m_current_properties.retrieve<int>("waypoint");
    // Moving state
    mp_overworld->m_player_moving_state = static_cast<Moving_state>(m_current_properties.retrieve<int>("moving_state"));
}

void cOverworldLoader::Parse_Tag_Background()
{
    mp_overworld->m_background_color = Color(static_cast<uint8_t>(m_current_properties.retrieve<int>("color_red")),
                                       m_current_properties.retrieve<int>("color_green"),
                                       m_current_properties.retrieve<int>("color_blue"));
}

/***************************************
 * Create_World_Object_From_XML()
 ***************************************/

cSprite* cOverworldLoader::Create_World_Object_From_XML(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager, cOverworld* p_overworld)
{
    if (name == "sprite")
        return Create_Sprite_From_XML_Tag(attributes, engine_version, p_sprite_manager, p_overworld);
    else if (name == "waypoint")
        return Create_Waypoint_From_XML_Tag(attributes, engine_version, p_sprite_manager, p_overworld);
    else if (name == "sound")
        return Create_Sound_From_XML_Tag(attributes, engine_version, p_sprite_manager, p_overworld);
    else if (name == "line")
        return Create_Line_From_XML_Tag(attributes, engine_version, p_sprite_manager, p_overworld);
    else
        cerr << "Warning: Unknown world object XML tag '" << name << "'" << endl;

    return NULL;
}

cSprite* cOverworldLoader::Create_Sprite_From_XML_Tag(XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager, cOverworld* p_overworld)
{
    if (engine_version < 2) {
        // old version: change file and position name
        if (attributes.exists("filename")) {
            attributes["image"] = attributes["filename"];
            attributes["posx"] = attributes["pos_x"];
            attributes["posy"] = attributes["pos_y"];
        }

        // If V.1.9 and lower: move y coordinate bottom to 0
        if (attributes.exists("posy"))
            attributes["posy"] = float_to_string(attributes.retrieve<float>("posy") - 600.0f);
    }

    // If V.1.9 and lower: change old bridge to bridge 1 vertical
    if (engine_version < 3)
        attributes.relocate_image("world/objects/bridge/bridge_1.png", "world/objects/bridge/bridge_1_ver_start.png");

    // If V.1.9 and lower: green pipe has been removed in favour for red pipe
    if (engine_version < 5) {
        attributes.relocate_image("world/objects/pipe/green_big.png", "world/objects/pipe/red_big.png");
        attributes.relocate_image("world/objects/pipe/green_small.png", "world/objects/pipe/red_small.png");
    }

    // Create sprite
    cSprite* p_sprite = new cSprite(attributes, p_sprite_manager);
    // Set sprite type
    p_sprite->Set_Massive_Type(MASS_PASSIVE);

    // needs image. If V.1.9 and lower: change old bridge to bridge 1 vertical
    if (p_sprite->m_image && engine_version < 3 && p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("world/objects/bridge/bridge_1_ver_start.png")) == 0) {
        // Move a bit to the left
        p_sprite->Move(-7.0f, 0.0f, true);
        p_sprite->m_start_pos_x = p_sprite->m_pos_x;

        // create other tiles now
        cSprite* p_copy = p_sprite->Copy();

        // middle
        p_copy->Set_Image(pVideo->Get_Surface(utf8_to_path("world/objects/bridge/bridge_1_ver_middle.png")), true);
        p_copy->Set_Pos_Y(p_copy->m_start_pos_y + 32, true);
        p_sprite_manager->Add(p_copy); // HACK: Should be done by the caller! (unexpected parameter modification)

        // end
        p_copy = p_copy->Copy();
        p_copy->Set_Image(pVideo->Get_Surface(utf8_to_path("world/objects/bridge/bridge_1_ver_end.png")), true);
        p_copy->Set_Pos_Y(p_copy->m_start_pos_y + 32, true);
        p_sprite_manager->Add(p_copy); // HACK: Should be done by the caller! (unexpected parameter modification)
    }

    /* Actually, this is the only one of 3 sprites that will be added to p_sprite_manager
     * by the caller (see HACKs above). cLevelLoader solves this problem by returning
     * a std::vector of cSprite instances instead, but retrospectively this seems
     * overkill for a problem that will be gone when backward compatibility is
     * finally dropped. */
    return p_sprite;
}

cSprite* cOverworldLoader::Create_Waypoint_From_XML_Tag(XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager, cOverworld* p_overworld)
{
    // if V.1.9 and lower : move y coordinate bottom to 0
    if (engine_version < 2 && attributes.exists("y"))
        attributes["y"] = float_to_string(attributes.retrieve<float>("y") - 600.0f);

    return new cWaypoint(attributes, p_sprite_manager);
}

cSprite* cOverworldLoader::Create_Sound_From_XML_Tag(XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager, cOverworld* p_overworld)
{
    // if V.1.9 and lower : move y coordinate bottom to 0
    if (engine_version < 2 && attributes.exists("pos_y"))
        attributes["pos_y"] = float_to_string(attributes.retrieve<float>("pos_y") - 600.0f);

    return new cRandom_Sound(attributes, p_sprite_manager);
}

cSprite* cOverworldLoader::Create_Line_From_XML_Tag(XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager, cOverworld* p_overworld)
{
    // if V.1.9 and lower : move y coordinate bottom to 0
    if (engine_version < 2) {
        if (attributes.exists("Y1"))
            attributes["Y1"] = float_to_string(attributes.retrieve<float>("Y1") - 600.0f);

        if (attributes.exists("Y2"))
            attributes["Y2"] = float_to_string(attributes.retrieve<float>("Y2") - 600.0f);
    }

    return new cLayer_Line_Point_Start(attributes, p_sprite_manager, p_overworld);
}

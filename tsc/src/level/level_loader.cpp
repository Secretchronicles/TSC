/***************************************************************************
 * level_loader.cpp - loading level XML
 *
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

#include "level_loader.hpp"
#include "level_player.hpp"
#include "../core/sprite_manager.hpp"
#include "../core/property_helper.hpp"
#include "../core/filesystem/resource_manager.hpp"
#include "../video/font.hpp"
#include "../objects/enemystopper.hpp"
#include "../objects/level_exit.hpp"
#include "../objects/bonusbox.hpp"
#include "../objects/spinbox.hpp"
#include "../objects/text_box.hpp"
#include "../objects/goldpiece.hpp"
#include "../objects/powerup.hpp"
#include "../objects/star.hpp"
#include "../objects/moving_platform.hpp"
#include "../enemies/eato.hpp"
#include "../enemies/furball.hpp"
#include "../enemies/turtle.hpp"
#include "../enemies/shell.hpp"
#include "../enemies/bosses/turtle_boss.hpp"
#include "../enemies/flyon.hpp"
#include "../enemies/thromp.hpp"
#include "../enemies/rokko.hpp"
#include "../enemies/krush.hpp"
#include "../enemies/gee.hpp"
#include "../enemies/spika.hpp"
#include "../enemies/static.hpp"
#include "../enemies/spikeball.hpp"
#include "../enemies/pip.hpp"
#include "../enemies/beetle_barrage.hpp"
#include "../enemies/beetle.hpp"
#include "../enemies/larry.hpp"
#include "../audio/random_sound.hpp"
#include "../video/animation.hpp"
#include "../core/game_core.hpp"
#include "../objects/ball.hpp"
#include "../objects/lava.hpp"
#include "../objects/crate.hpp"

namespace fs = boost::filesystem;
using namespace TSC;

cLevelLoader::cLevelLoader()
    : xmlpp::SaxParser()
{
    mp_level    = NULL;
}

cLevelLoader::~cLevelLoader()
{
    // Do not delete the cLevel instance — it is used by the
    // caller and deleted by him.
    mp_level = NULL;
}

cLevel* cLevelLoader::Get_Level()
{
    return mp_level;
}

/***************************************
 * SAX parser callbacks
 ***************************************/

void cLevelLoader::parse_file(boost::filesystem::path filename)
{
    m_levelfile = filename;
    xmlpp::SaxParser::parse_file(path_to_utf8(filename));
}

void cLevelLoader::on_start_document()
{
    if (mp_level)
        throw("Restarted XML parser after already starting it."); // FIXME: proper exception

    mp_level = new cLevel();
    m_in_script_tag = false;
}

void cLevelLoader::on_end_document()
{
    mp_level->m_level_filename = m_levelfile;

    // engine version entry not set
    if (mp_level->m_engine_version < 0)
        mp_level->m_engine_version = 0;
}

void cLevelLoader::on_start_element(const Glib::ustring& name, const xmlpp::SaxParser::AttributeList& properties)
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

            if (attr.name == "name")
                key = attr.value;
            else if (attr.name == "value")
                value = attr.value;
        }

        m_current_properties[key] = value;
    }
    else if (name == "script") {
        // Indicate a script tag has opened, so we can retrieve
        // its and only its text.
        m_in_script_tag = true;
    }
}

void cLevelLoader::on_end_element(const Glib::ustring& name)
{
    // <property> tags are parsed cumulatively in on_start_element()
    // so all have been collected when the surrounding element
    // terminates here.
    if (name == "property" || name == "Property")
        return;

    // Now for the real, cumbersome parsing process
    if (name == "information")
        Parse_Tag_Information();
    else if (name == "settings")
        Parse_Tag_Settings();
    else if (name == "background")
        Parse_Tag_Background();
    else if (name == "player")
        Parse_Tag_Player();
    else if (cLevel::Is_Level_Object_Element(std::string(name))) // CEGUI doesn’t like Glib::ustring
        Parse_Level_Object_Tag(name);
    else if (name == "level") {
        /* Ignore the root <level> tag */
    }
    else if (name == "script")
        m_in_script_tag = false; // Indicate the <script> tag has ended
    else
        std::cerr << "Warning: Unknown XML tag '" << name << "'on level parsing." << std::endl;

    // Everything handled, so we can now safely clear the
    // collected <property> element values for the next
    // tag.
    m_current_properties.clear();
}

void cLevelLoader::on_characters(const Glib::ustring& text)
{
    /* If we’re currently in the <script> tag, read its
     * text (may be called multiple times for each token,
     * so append rather then set directly). */
    if (m_in_script_tag)
        mp_level->m_script.append(text);
}

/***************************************
 * Parsers for mayor XML tags
 ***************************************/

void cLevelLoader::Parse_Tag_Information()
{
    // Support V1.7 and lower which used float
    float engine_version_float = string_to_float(m_current_properties["engine_version"]);

    // if float engine version
    if (engine_version_float < 3)
        engine_version_float *= 10; // change to new format

    mp_level->m_engine_version = static_cast<int>(engine_version_float);
    mp_level->m_last_saved     = string_to_int64(m_current_properties["save_time"]);
}

void cLevelLoader::Parse_Tag_Settings()
{
    // If V1.9 and lower: Move Y coordinate bottom to 0
    if (mp_level->m_engine_version < 35 && m_current_properties.count("cam_limit_h") > 0) {
        float y = string_to_float(m_current_properties["cam_limit_h"]);
        m_current_properties["cam_lmit_h"] = float_to_string(y - 600.0f);
    }

    mp_level->Set_Author(m_current_properties["lvl_author"]);
    mp_level->Set_Version(m_current_properties["lvl_version"]);
    mp_level->Set_Difficulty(string_to_int(m_current_properties["lvl_difficulty"]));
    mp_level->Set_Description(xml_string_to_string(m_current_properties["lvl_description"]));
    mp_level->Set_Music(utf8_to_path(m_current_properties["lvl_music"]));
    mp_level->Set_Land_Type(Get_Level_Land_Type_Id(m_current_properties["lvl_land_type"]));

    mp_level->m_fixed_camera_hor_vel = string_to_float(m_current_properties["cam_fixed_hor_vel"]);

    mp_level->m_camera_limits = GL_rect(string_to_float(m_current_properties["cam_limit_x"]),
                                        string_to_float(m_current_properties["cam_limit_y"]),
                                        string_to_float(m_current_properties["cam_limit_w"]),
                                        string_to_float(m_current_properties["cam_limit_h"]));

    mp_level->m_unload_after_exit = static_cast<bool>(string_to_int(m_current_properties["unload_after_exit"]));
}

void cLevelLoader::Parse_Tag_Background()
{
    BackgroundType bg_type = static_cast<BackgroundType>(string_to_int(m_current_properties["type"]));

    // Use gradient background
    if (bg_type == BG_GR_HOR || bg_type == BG_GR_VER)
        mp_level->m_background_manager->Get_Pointer(0)->Load_From_Attributes(m_current_properties);
    else // default background
        mp_level->m_background_manager->Add(new cBackground(m_current_properties, mp_level->m_sprite_manager));
}

void cLevelLoader::Parse_Tag_Player()
{
    // Defaults
    mp_level->m_player_start_pos_x = cLevel_Player::m_default_pos_x;
    mp_level->m_player_start_pos_y = cLevel_Player::m_default_pos_y;

    if (m_current_properties.count("posx"))
        mp_level->m_player_start_pos_x = string_to_float(m_current_properties["posx"]);
    if (m_current_properties.count("posy"))
        mp_level->m_player_start_pos_y = string_to_float(m_current_properties["posy"]);

    // If V1.9 and lower: Move Y coordinate bottom to 0
    if (mp_level->m_engine_version < 35)
        mp_level->m_player_start_pos_y -= 600.0f;

    // Adjust with Y offsets other versions had
    if (mp_level->m_engine_version <= 10)
        mp_level->m_player_start_pos_y += 58.0f;
    else if (mp_level->m_engine_version <= 20)
        mp_level->m_player_start_pos_y -= 48.0f;

    // direction, set to default if invalid
    mp_level->m_player_start_direction = Get_Direction_Id(m_current_properties["direction"]);
    if (mp_level->m_player_start_direction != DIR_LEFT && mp_level->m_player_start_direction != DIR_RIGHT)
        mp_level->m_player_start_direction = DIR_RIGHT;
}

void cLevelLoader::Parse_Level_Object_Tag(const std::string& name)
{
    // create sprite
    std::vector<cSprite*> sprites = Create_Level_Objects_From_XML_Tag(name, m_current_properties, mp_level->m_engine_version, mp_level->m_sprite_manager);

    // valid
    if (sprites.size() > 0) {
        /* If a static UID has been set, assign it to the sprite. -1
         * (cSprite’s default) means "no UID set" and instructs
         * cSprite_Manager to assign a new, free one to the
         * object. Note that in the unlikely case of intermixed static
         * and dynamic UIDs (i.e.  some sprites have UIDs assigned,
         * others not — this can only be achieved by editing the level
         * XML by hand, the editor does not allow this) it could
         * happen that cSprite_Manager’s UID generator generates a UID
         * (and assignes it to the object) that is later set as a
         * static UID for another sprite — causing this sprite to have
         * the same UID as the one with the generated UID (this will
         * print a warning to the console if TSC is compiled in debug
         * mode). We cannot know this in advance, but as said you have
         * to edit the XML by hand and therefore we can ignore this
         * case safely. */
        if (m_current_properties.count("uid"))
            sprites[0]->m_uid = string_to_int(m_current_properties["uid"]); // The 98% case is that we get only one sprite back, the other 2% are backward compatibility

        for (std::vector<cSprite*>::iterator iter = sprites.begin(); iter != sprites.end(); iter++)
            mp_level->m_sprite_manager->Add(*iter);
    }
}

/***************************************
 * Create_Level_Objects_From_XML_Tag()
 ***************************************/

std::vector<cSprite*> cLevelLoader::Create_Level_Objects_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager)
{
    if (name == "sprite")
        return Create_Sprites_From_XML_Tag(name, attributes, engine_version, p_sprite_manager);
    else if (name == "enemystopper")
        return Create_Enemy_Stoppers_From_XML_Tag(name, attributes, engine_version, p_sprite_manager);
    else if (name == "levelexit")
        return Create_Level_Exits_From_XML_Tag(name, attributes, engine_version, p_sprite_manager);
    else if (name == "level_entry")
        return Create_Level_Entries_From_XML_Tag(name, attributes, engine_version, p_sprite_manager);
    else if (name == "box")
        return Create_Boxes_From_XML_Tag(name, attributes, engine_version, p_sprite_manager);
    else if (name == "item" || name == "powerup") // powerup is pre V.0.99.5
        return Create_Items_From_XML_Tag(name, attributes, engine_version, p_sprite_manager);
    else if (name == "moving_platform")
        return Create_Moving_Platforms_From_XML_Tag(name, attributes, engine_version, p_sprite_manager);
    else if (name == "falling_platform") // falling platform is pre V.1.5
        return Create_Falling_Platforms_From_XML_Tag(name, attributes, engine_version, p_sprite_manager);
    else if (name == "enemy")
        return Create_Enemies_From_XML_Tag(name, attributes, engine_version, p_sprite_manager);
    else if (name == "sound")
        return Create_Sounds_From_XML_Tag(name, attributes, engine_version, p_sprite_manager);
    else if (name == "particle_emitter")
        return Create_Particle_Emitters_From_XML_Tag(name, attributes, engine_version, p_sprite_manager);
    else if (name == "path")
        return Create_Paths_From_XML_Tag(name, attributes, engine_version, p_sprite_manager);
    else if (name == "global_effect") // global_effect is V.1.9 and lower
        return Create_Global_Effects_From_XML_Tag(name, attributes, engine_version, p_sprite_manager);
    else if (name == "ball")
        return Create_Balls_From_XML_Tag(name, attributes, engine_version, p_sprite_manager);
    else if (name == "lava")
        return Create_Lavas_From_XML_Tag(name, attributes, engine_version, p_sprite_manager);
    else if (name == "crate")
        return Create_Crates_From_XML_Tag(name, attributes, engine_version, p_sprite_manager);
    else
        std::cerr << "Warning: Unknown level object element '" << name << "'. Is cLevelLoader::Create_Level_Objects_From_XML_Tag() in sync with cLevel::Is_Level_Object_Element()?" << std::endl;

    // keep above list sync with cLevel::Is_Level_Object_Element()

    // This is not a level object tag, return empty list
    return std::vector<cSprite*>();
}

std::vector<cSprite*> cLevelLoader::Create_Sprites_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager)
{
    std::vector<cSprite*> result;

    // V1.4 and lower: change some image paths
    if (engine_version < 25) {
        attributes.relocate_image("game/box/stone8.png", "blocks/metal/stone_2_violet.png");
        attributes.relocate_image("ground/jungle_1/tree_type_1.png", "ground/jungle_1/tree/1.png");
        attributes.relocate_image("ground/jungle_1/tree_type_1_front.png", "ground/jungle_1/tree/1_front.png");
        attributes.relocate_image("ground/jungle_1/tree_type_2.png", "ground/jungle_1/tree/2.png");
        attributes.relocate_image("ground/yoshi_1/extra_1_blue.png", "ground/jungle_2/hedge/1_blue.png");
        attributes.relocate_image("ground/yoshi_1/extra_1_green.png", "ground/jungle_2/hedge/1_green.png");
        attributes.relocate_image("ground/yoshi_1/extra_1_red.png", "ground/jungle_2/hedge/1_red.png");
        attributes.relocate_image("ground/yoshi_1/extra_1_yellow.png", "ground/jungle_2/hedge/1_yellow.png");
        attributes.relocate_image("ground/yoshi_1/rope_1_leftright.png", "ground/jungle_2/rope_1_hor.png");
    }
    // V1.5 and lower: change pipe connection image paths
    if (engine_version < 28) {
        attributes.relocate_image("blocks/pipe/connection_left_down.png", "blocks/pipe/connection/plastic_1/orange/right_up.png");
        attributes.relocate_image("blocks/pipe/connection_left_up.png", "blocks/pipe/connection/plastic_1/orange/right_down.png");
        attributes.relocate_image("blocks/pipe/connection_right_down.png", "blocks/pipe/connection/plastic_1/orange/left_up.png");
        attributes.relocate_image("blocks/pipe/connection_right_up.png", "blocks/pipe/connection/plastic_1/orange/left_down.png");
        attributes.relocate_image("blocks/pipe/metal_connector.png", "blocks/pipe/connection/metal_1/grey/middle.png");
    }
    // V1.7 and lower: change yoshi_1 hill_up to jungle_1 slider image paths
    if (engine_version < 31) {
        attributes.relocate_image("ground/yoshi_1/hill_up_1.png", "ground/jungle_1/slider/2_green_left.png");
        attributes.relocate_image("ground/yoshi_1/hill_up_2.png", "ground/jungle_1/slider/2_blue_left.png");
        attributes.relocate_image("ground/yoshi_1/hill_up_3.png", "ground/jungle_1/slider/2_brown_left.png");
    }
    // V1.7.x and lower: change green_1 ground to green_3 ground image paths
    if (engine_version < 34) {
        // normal
        attributes.relocate_image("ground/green_1/ground/left_up.png", "ground/green_3/ground/top/left.png");
        attributes.relocate_image("ground/green_1/ground/left_down.png", "ground/green_3/ground/bottom/left.png");
        attributes.relocate_image("ground/green_1/ground/right_up.png", "ground/green_3/ground/top/right.png");
        attributes.relocate_image("ground/green_1/ground/right_down.png", "ground/green_3/ground/bottom/right.png");
        attributes.relocate_image("ground/green_1/ground/up.png", "ground/green_3/ground/top/1.png");
        attributes.relocate_image("ground/green_1/ground/down.png", "ground/green_3/ground/bottom/1.png");
        attributes.relocate_image("ground/green_1/ground/right.png", "ground/green_3/ground/middle/right.png");
        attributes.relocate_image("ground/green_1/ground/left.png", "ground/green_3/ground/middle/left.png");
        attributes.relocate_image("ground/green_1/ground/middle.png", "ground/green_3/ground/middle/1.png");

        // hill (not available)
        //attributes.relocate_image( "ground/green_1/ground/hill_left_up.png", "ground/green_3/ground/" );
        //attributes.relocate_image( "ground/green_1/ground/hill_right_up.png", "ground/green_3/ground/" );
        //attributes.relocate_image( "ground/green_1/ground/hill_right.png", "ground/green_3/ground/" );
        //attributes.relocate_image( "ground/green_1/ground/hill_left.png", "ground/green_3/ground/" );
    }
    // V1.9 and lower: move y coordinate bottom to 0
    if (engine_version < 35) {
        if (attributes.count("posy") > 0)
            attributes["posy"] = float_to_string(string_to_float(attributes["posy"]) - 600.0f);
    }
    // V1.9.x and lower: change fire-1 animation to particles
    if (engine_version < 37) {
        attributes.relocate_image("animation/fire_1/1.png", "animation/particles/fire_1.png");
        attributes.relocate_image("animation/fire_1/2.png", "animation/particles/fire_2.png");
        attributes.relocate_image("animation/fire_1/3.png", "animation/particles/fire_3.png");
        attributes.relocate_image("animation/fire_1/4.png", "animation/particles/fire_4.png");
    }
    // always: fix sprite with undefined massive-type
    if (attributes.count("type") > 0 && attributes["type"] == "undefined") {
        std::cerr << "Warning: Fixing type 'undefined' by forcing it to 'passive'" << std::endl;
        attributes["type"] = "passive"; // So it doesn’t hinder gameplay
    }

    cSprite* p_sprite = new cSprite(attributes, p_sprite_manager);

    // If image not available display its filename
    if (!p_sprite->m_start_image) {
        std::string text = attributes["image"];
        if (text.empty())
            text = "Invalid image here";

        cGL_Surface* p_text_image = pFont->Render_Text(pFont->m_font_small, text);
        p_text_image->m_path = utf8_to_path(text);
        p_sprite->Set_Image(p_text_image, true, true);
        p_sprite->Set_Massive_Type(MASS_PASSIVE); // It shouldn't hinder gameplay
        p_sprite->Set_Active(false); // Only display it in the editor
    }

    // needs image
    if (p_sprite->m_image) {
        // If V1.2 and lower: change pipe position
        if (engine_version < 22) {
            if (p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("pipes/green/up.png")) == 0 ||
                    p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("pipes/green/ver.png")) == 0 ||
                    p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("pipes/green/down.png")) == 0 ||
                    p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("pipes/blue/up.png")) == 0 ||
                    p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("pipes/blue/ver.png")) == 0 ||
                    p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("pipes/blue/down.png")) == 0 ||
                    p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("pipes/yellow/up.png")) == 0 ||
                    p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("pipes/yellow/ver.png")) == 0 ||
                    p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("pipes/yellow/down.png")) == 0 ||
                    p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("pipes/grey/up.png")) == 0 ||
                    p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("pipes/grey/ver.png")) == 0 ||
                    p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("pipes/grey/down.png")) == 0) {
                p_sprite->Move(-6, 0, 1);
                p_sprite->m_start_pos_x = p_sprite->m_pos_x;
            }
            else if (p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("pipes/green/right.png")) == 0 ||
                     p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("pipes/green/hor.png")) == 0 ||
                     p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("pipes/green/left.png")) == 0 ||
                     p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("pipes/blue/right.png")) == 0 ||
                     p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("pipes/blue/hor.png")) == 0 ||
                     p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("pipes/blue/left.png")) == 0 ||
                     p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("pipes/yellow/right.png")) == 0 ||
                     p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("pipes/yellow/hor.png")) == 0 ||
                     p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("pipes/yellow/left.png")) == 0 ||
                     p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("pipes/grey/right.png")) == 0 ||
                     p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("pipes/grey/hor.png")) == 0 ||
                     p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("pipes/grey/left.png")) == 0) {
                p_sprite->Move(0, -6, 1);
                p_sprite->m_start_pos_y = p_sprite->m_pos_y;
            }
        } // engine_version < 22

        // If V1.2.x and lower: change some hill positions
        if (engine_version < 23) {
            if (p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("hills/green_1/head.png")) ||
                    p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("hills/light_blue_1/head.png")) == 0) {
                p_sprite->Move(0, -6, 1);
                p_sprite->m_start_pos_y = p_sprite->m_pos_y;
            }
        } // engine_version < 23

        // If V1.7 and lower: change yoshi_1 hill_up to jungle_1 slider image paths
        if (engine_version < 31) {
            // This is one of the XML tags that explodes into multiple sprites.
            // image filename is already changed but we need to add the middle and right tiles
            if (p_sprite_manager && (p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("ground/jungle_1/slider/2_green_left.png")) == 0 ||
                                     p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("ground/jungle_1/slider/2_blue_left.png")) == 0 ||
                                     p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("ground/jungle_1/slider/2_brown_left.png")) == 0)) {
                std::string color;
                // green
                if (p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("ground/jungle_1/slider/2_green_left.png")) == 0)
                    color = "green";
                // blue
                else if (p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("ground/jungle_1/slider/2_blue_left.png")) == 0)
                    color = "blue";
                // brown
                else
                    color = "brown";

                cSprite* p_copy = p_sprite;

                // add middle tiles
                for (unsigned int i = 0; i < 4; i++) {
                    p_copy = p_copy->Copy();
                    p_copy->Set_Image(pVideo->Get_Surface(utf8_to_path("ground/jungle_1/slider/2_" + color + "_middle.png")), 1);
                    p_copy->Set_Pos_X(p_copy->m_start_pos_x + 22, 1);
                    //p_sprite_manager->Add( p_copy );
                    result.push_back(p_copy);
                }

                // add end tile
                p_copy = p_copy->Copy();
                p_copy->Set_Image(pVideo->Get_Surface(utf8_to_path("ground/jungle_1/slider/2_" + color + "_right.png")), 1);
                p_copy->Set_Pos_X(p_copy->m_start_pos_x + 22, 1);
                //p_sprite_manager->Add( p_copy );
                result.push_back(p_copy);
            }
        } // engine_version < 31

        // If V1.7 and lower: change slider grey_1 to green_1 brown slider image paths
        if (engine_version < 32) {
            // image filename is already changed but we need to add an additional middle tile for left and right
            if (p_sprite_manager && (p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("ground/green_1/slider/1/brown/left.png")) == 0 ||
                                     p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("ground/green_1/slider/1/brown/right.png")) == 0)) {
                // add middle tile
                cSprite* p_copy = p_sprite->Copy();
                p_copy->Set_Image(pVideo->Get_Surface(utf8_to_path("ground/green_1/slider/1/brown/middle.png")), 1);

                // if from left tile it must be moved
                if (p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("ground/green_1/slider/1/brown/left.png")) == 0)
                    p_copy->Set_Pos_X(p_copy->m_start_pos_x + 18, 1);

                //p_sprite_manager->Add( p_copy );
                result.push_back(p_copy);
            }
            // move right tile
            if (p_sprite->m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("ground/green_1/slider/1/brown/right.png")) == 0) {
                p_sprite->Move(18, 0, 1);
                p_sprite->m_start_pos_x = p_sprite->m_pos_x;
            }
        } // engine_version < 32
    } // p_sprite->m_image

    result.push_back(p_sprite);
    return result;
}

std::vector<cSprite*> cLevelLoader::Create_Enemy_Stoppers_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager)
{
    std::vector<cSprite*> result;

    // If V1.9 and lower: Move Y coordinate bottom to 0
    if (engine_version < 35 && attributes.count("posy") > 0)
        attributes["posy"] = float_to_string(string_to_float(attributes["posy"]) - 600.0f);

    result.push_back(new cEnemyStopper(attributes, p_sprite_manager));
    return result;
}

std::vector<cSprite*> cLevelLoader::Create_Level_Exits_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager)
{
    std::vector<cSprite*> result;

    // If V1.9 and lower: Move Y coordinate bottom to 0
    if (engine_version < 35 && attributes.count("posy") > 0)
        attributes["posy"] = float_to_string(string_to_float(attributes["posy"]) - 600.0f);

    // If V1.9.x and lower: change "motion" to "camera_motion"
    if (engine_version < 36 && attributes.count("motion") > 0) {
        attributes["camera_motion"] = int_to_string(string_to_int(attributes["motion"]) + 1);
        attributes.erase("motion");
    }

    result.push_back(new cLevel_Exit(attributes, p_sprite_manager));
    return result;
}

std::vector<cSprite*> cLevelLoader::Create_Level_Entries_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager)
{
    std::vector<cSprite*> result;

    // If V1.9 and lower: Move Y coordinate bottom to 0
    if (engine_version < 35 && attributes.count("posy") > 0)
        attributes["posy"] = float_to_string(string_to_float(attributes["posy"]) - 600.0f);

    result.push_back(new cLevel_Entry(attributes, p_sprite_manager));
    return result;
}

std::vector<cSprite*> cLevelLoader::Create_Boxes_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager)
{
    std::vector<cSprite*> result;

    // If V1.9 and lower: Move Y coordinate bottom to 0
    if (engine_version < 35 && attributes.exists("posy"))
        attributes["posy"] = float_to_string(string_to_float(attributes["posy"]) - 600.0f);

    if (attributes["type"] == "bonus")
        result.push_back(new cBonusBox(attributes, p_sprite_manager));
    else if (attributes["type"] == "gold") { // `gold' is somewhere pre V0.99.5
        // Update old values
        attributes["type"]      = "bonus";
        attributes["animation"] = "Default";
        attributes["item"]      = int_to_string(TYPE_GOLDPIECE);

        // Renamed old values
        if (attributes.exists("color")) {
            attributes["gold_color"] = attributes["color"];
            attributes.erase("color");
        }

        result.push_back(new cBonusBox(attributes, p_sprite_manager));
    }
    else if (attributes["type"] == "spin")
        result.push_back(new cSpinBox(attributes, p_sprite_manager));
    else if (attributes["type"] == "text")
        result.push_back(new cText_Box(attributes, p_sprite_manager));
    else if (attributes["type"] == "empty") { // pre V0.99.4
        // Update old values
        attributes["type"] = "bonus";
        attributes["item"] = "0";

        result.push_back(new cBonusBox(attributes, p_sprite_manager));
    }
    else if (attributes["type"] == "invisible") { // pre V0.99.4
        // Update old values
        attributes["type"] = "bonus";
        attributes["item"] = "0";
        attributes["invisible"] = "1";

        result.push_back(new cBonusBox(attributes, p_sprite_manager));
    }
    else // if attributes["type"] == X
        std::cerr << "Warning: Unknown level box type: " << attributes["type"] << std::endl;

    return result;
}

std::vector<cSprite*> cLevelLoader::Create_Items_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager)
{
    std::vector<cSprite*> result;

    // if V.1.9 and lower : move y coordinate bottom to 0
    if (engine_version < 35 && attributes.exists("posy"))
        attributes["posy"] = float_to_string(string_to_float(attributes["posy"]) - 600.0f);

    std::string type = attributes["type"];
    if (type == "goldpiece")
        result.push_back(new cGoldpiece(attributes, p_sprite_manager));
    else if (type == "mushroom")
        result.push_back(new cMushroom(attributes, p_sprite_manager));
    else if (type == "fireplant")
        result.push_back(new cFirePlant(attributes, p_sprite_manager));
    else if (type == "jstar")
        result.push_back(new cjStar(attributes, p_sprite_manager));
    else if (type == "moon")
        result.push_back(new cMoon(attributes, p_sprite_manager));
    else // type == "X"
        std::cerr << "Warning: Unknown level item type '" << type << "'" << std::endl;

    return result;
}

std::vector<cSprite*> cLevelLoader::Create_Moving_Platforms_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager)
{
    std::vector<cSprite*> result;

    // if V.1.7 and lower : change slider grey_1 to green_1 brown slider image paths
    if (engine_version < 32) {
        attributes.relocate_image("slider/grey_1/slider_left.png", "ground/green_1/slider/1/brown/left.png", "image_top_left");
        attributes.relocate_image("slider/grey_1/slider_middle.png", "ground/green_1/slider/1/brown/middle.png", "image_top_middle");
        attributes.relocate_image("slider/grey_1/slider_right.png", "ground/green_1/slider/1/brown/right.png", "image_top_right");
    }
    // if V.1.9 and lower : move y coordinate bottom to 0
    if (engine_version < 35 && attributes.exists("posy"))
        attributes["posy"] = float_to_string(string_to_float(attributes["posy"]) - 600.0f);

    cMoving_Platform* p_moving_platform = new cMoving_Platform(attributes, p_sprite_manager);

    // if V.1.7 and lower : change new slider middle count because start and end image is now half the width
    if (engine_version < 32) {
        if (p_moving_platform->m_images[0].m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("ground/green_1/slider/1/brown/left.png")) == 0)
            p_moving_platform->Set_Middle_Count(p_moving_platform->m_middle_count + 1);
        else if (p_moving_platform->m_images[0].m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("ground/green_1/slider/1/brown/right.png")) == 0)
            p_moving_platform->Set_Middle_Count(p_moving_platform->m_middle_count + 1);
    }

    result.push_back(p_moving_platform);
    return result;
}

std::vector<cSprite*> cLevelLoader::Create_Falling_Platforms_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager)
{
    std::vector<cSprite*> result;

    // it's not moving
    attributes["speed"] = "0";

    // renamed time_fall to touch_time and change to the new value
    if (attributes.exists("time_fall")) {
        attributes["touch_time"] = float_to_string(string_to_float(attributes["time_fall"]) * speedfactor_fps);
        attributes.erase("time_fall");
    }
    else
        attributes["touch_time"] = "48";

    // enable falling
    attributes["shake_time"] = "12";

    // if V.1.7 and lower : change slider grey_1 to green_1 brown slider image paths
    if (engine_version < 32) {
        attributes.relocate_image("slider/grey_1/slider_left.png", "ground/green_1/slider/1/brown/left.png", "image_top_left");
        attributes.relocate_image("slider/grey_1/slider_middle.png", "ground/green_1/slider/1/brown/middle.png", "image_top_middle");
        attributes.relocate_image("slider/grey_1/slider_right.png", "ground/green_1/slider/1/brown/right.png", "image_top_right");
    }

    // if V.1.9 and lower : move y coordinate bottom to 0
    if (engine_version < 35 && attributes.exists("posy"))
        attributes["posy"] = float_to_string(string_to_float(attributes["posy"]) - 600.0f);

    cMoving_Platform* p_moving_platform = new cMoving_Platform(attributes, p_sprite_manager);

    // if V.1.7 and lower : change new slider middle count because start and end image is now half the width
    if (engine_version < 32) {
        if (p_moving_platform->m_images[0].m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("ground/green_1/slider/1/brown/left.png")) == 0)
            p_moving_platform->Set_Middle_Count(p_moving_platform->m_middle_count + 1);
        else if (p_moving_platform->m_images[0].m_image->m_path.compare(pResource_Manager->Get_Game_Pixmap("ground/green_1/slider/1/brown/right.png")) == 0)
            p_moving_platform->Set_Middle_Count(p_moving_platform->m_middle_count + 1);
    }

    result.push_back(p_moving_platform);
    return result;
}

std::vector<cSprite*> cLevelLoader::Create_Paths_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager)
{
    std::vector<cSprite*> result;

    // if V.1.9 and lower : move y coordinate bottom to 0
    if (engine_version < 35 && attributes.exists("posy"))
        attributes["posy"] = float_to_string(string_to_float(attributes["posy"]) - 600.0f);

    result.push_back(new cPath(attributes, p_sprite_manager));
    return result;
}

std::vector<cSprite*> cLevelLoader::Create_Enemies_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager)
{
    std::vector<cSprite*> result;
    std::string type = attributes["type"];

    // if V.1.5 and lower
    if (engine_version < 26) {
        // change gumba to furball
        if (type == "gumba") {
            // change type
            type = "furball";
            attributes["type"] = "furball";
            // fix color : red was used in pre 1.0 but later became blue
            if (attributes.exists("color") && attributes["color"] == "red")
                attributes["color"] = "blue";
        }

        // change rex to krush
        else if (type == "rex") {
            // change type
            type = "krush";
            attributes["type"] = "krush";
        }
    } // engine_version < 26

    // if V.1.7 and lower
    if (engine_version < 29) {
        if (type == "jpiranha") {
            // change type
            type = "flyon";
            attributes["type"] = "flyon";

            // change image dir
            if (attributes.exists("image_dir")) {
                std::string img_dir = attributes["image_dir"];
                std::string::size_type pos = img_dir.find("jpiranha");

                // change if found
                if (pos != std::string::npos) {
                    img_dir.replace(pos, 8, "flyon");
                    attributes["image_dir"] = img_dir;
                }
            }
        }
    } // engine_version < 29

    // if V.1.9 and lower : move y coordinate bottom to 0
    if (engine_version < 35 && attributes.exists("posy"))
        attributes["posy"] = float_to_string(string_to_float(attributes["posy"]) - 600.0f);

    // Now for the real enemy loading after all the backward compatibility stuff
    if (type == "eato")
        result.push_back(new cEato(attributes, p_sprite_manager));
    else if (type == "furball")
        result.push_back(new cFurball(attributes, p_sprite_manager));
    else if (type == "turtle")
        result.push_back(new cTurtle(attributes, p_sprite_manager));
    else if (type == "shell")
        result.push_back(new cShell(attributes, p_sprite_manager));
    else if (type == "turtleboss") {
        // if V.1.5 and lower : max_downgrade_time changed to shell_time
        if (engine_version < 27 && attributes.exists("max_downgrade_time")) {
            attributes["shell_time"] = attributes["max_downgrade_time"];
            attributes.erase("max_downgrade_time");
        }
        result.push_back(new cTurtleBoss(attributes, p_sprite_manager));
    }
    else if (type == "flyon")
        result.push_back(new cFlyon(attributes, p_sprite_manager));
    else if (type == "thromp") {
        cThromp* p_thromp = new cThromp(attributes, p_sprite_manager);

        // if V.1.4 and lower : fix thromp distance was smaller
        if (engine_version < 25)
            p_thromp->Set_Max_Distance(p_thromp->m_max_distance + 36);

        result.push_back(p_thromp);
    }
    else if (type == "rokko")
        result.push_back(new cRokko(attributes, p_sprite_manager));
    else if (type == "krush")
        result.push_back(new cKrush(attributes, p_sprite_manager));
    else if (type == "gee")
        result.push_back(new cGee(attributes, p_sprite_manager));
    else if (type == "spika")
        result.push_back(new cSpika(attributes, p_sprite_manager));
    else if (type == "static")
        result.push_back(new cStaticEnemy(attributes, p_sprite_manager));
    else if (type == "spikeball")
        result.push_back(new cSpikeball(attributes, p_sprite_manager));
    else if (type == "pip")
        result.push_back(new cPip(attributes, p_sprite_manager));
    else if (type == "beetle_barrage")
        result.push_back(new cBeetleBarrage(attributes, p_sprite_manager));
    else if (type == "beetle")
        result.push_back(new cBeetle(attributes, p_sprite_manager));
    else if (type == "larry")
        result.push_back(new cLarry(attributes, p_sprite_manager));
    else // type == "X"
        std::cerr << "Warning: Unknown level enemy type: " << type << std::endl;

    return result;
}

std::vector<cSprite*> cLevelLoader::Create_Sounds_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager)
{
    std::vector<cSprite*> result;

    // if V.1.9 and lower : move y coordinate bottom to 0
    if (engine_version < 35 && attributes.exists("posy"))
        attributes["posy"] = float_to_string(string_to_float(attributes["posy"]) - 600.0f);

    result.push_back(new cRandom_Sound(attributes, p_sprite_manager));

    return result;
}

std::vector<cSprite*> cLevelLoader::Create_Particle_Emitters_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager)
{
    std::vector<cSprite*> result;

    // Note : If you relocate images don't forget the global effect

    // if V.1.9 and lower : move y coordinate bottom to 0
    if (engine_version < 35 && attributes.exists("posy"))
        attributes["posy"] = float_to_string(string_to_float(attributes["posy"]) - 600.0f);

    // if V.1.9 and lower : change fire_1 animation to particles
    if (engine_version < 37) {
        attributes.relocate_image("animation/fire_1/1.png", "animation/particles/fire_1.png", "file");
        attributes.relocate_image("animation/fire_1/2.png", "animation/particles/fire_2.png", "file");
        attributes.relocate_image("animation/fire_1/3.png", "animation/particles/fire_3.png", "file");
        attributes.relocate_image("animation/fire_1/4.png", "animation/particles/fire_4.png", "file");
    }

    // if V.1.9 and lower : change file to image
    if (engine_version < 38 && attributes.exists("file")) {
        attributes["image"] = attributes["file"];
        attributes.erase("file");
    }

    cParticle_Emitter* p_emitter = new cParticle_Emitter(attributes, p_sprite_manager);
    // set to not spawned
    p_emitter->Set_Spawned(false);

    result.push_back(p_emitter);

    return result;
}

// if V.1.9 and lower : convert global effect to an advanced particle emitter
std::vector<cSprite*> cLevelLoader::Create_Global_Effects_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager)
{
    std::vector<cSprite*> result;

    // if V.0.99.4 and lower : change lieftime mod to time to live
    if (engine_version < 21 && !attributes.exists("time_to_live")) {
        attributes["time_to_live"] = float_to_string(attributes.fetch<float>("lifetime_mod", 20.0f) * 0.3f);
        attributes.erase("lifetime_mod");
    }

    // if V.0.99.7 and lower : change creation speed to emitter iteration interval
    if (engine_version < 22 && !attributes.exists("emitter_iteration_interval")) {
        attributes["emitter_iteration_interval"] = float_to_string((1.0f / attributes.fetch<float>("creation_speed", 0.3f)) * 0.032f);
        attributes.erase("creation_speed");
    }

    // if V.1.9 and lower : change fire_1 animation to particles
    if (engine_version < 37) {
        attributes.relocate_image("animation/fire_1/1.png", "animation/particles/fire_1.png", "image");
        attributes.relocate_image("animation/fire_1/2.png", "animation/particles/fire_2.png", "image");
        attributes.relocate_image("animation/fire_1/3.png", "animation/particles/fire_3.png", "image");
        attributes.relocate_image("animation/fire_1/4.png", "animation/particles/fire_4.png", "image");
    }

    // change disabled type to quota 0
    if (attributes.exists("type") && attributes["type"] == "0") {
        attributes["quota"] = "0";
        attributes.erase("type");
    }

    // rename attributes
    attributes["pos_x"]         = int_to_string(attributes.fetch<int>("rect_x", 0));
    attributes["pos_y"]         = int_to_string(attributes.fetch<int>("rect_y", 0) - 600);
    attributes["size_x"]        = int_to_string(attributes.fetch<int>("rect_w", game_res_w));
    attributes["size_y"]        = int_to_string(attributes.fetch<int>("rect_h", 0));
    attributes["pos_z"]         = float_to_string(attributes.fetch<float>("z", 0.12f));
    attributes["pos_z_rand"]    = float_to_string(attributes.fetch<float>("z_rand", 0.0f));
    attributes["emitter_time_to_live"] = "-1.0";
    if (!attributes.exists("time_to_live"))
        attributes["time_to_live"] = "0.7";
    attributes["emitter_interval"]  = float_to_string(attributes.fetch<float>("emitter_iteration_interval", 0.3f));
    attributes["size_scale"]        = float_to_string(attributes.fetch<float>("speed", 0.2f));
    attributes["size_scale_rand"]   = float_to_string(attributes.fetch<float>("scale_rand", 0.2f));
    attributes["vel"]               = float_to_string(attributes.fetch<float>("speed", 0.2f));
    attributes["vel_rand"]          = float_to_string(attributes.fetch<float>("speed_rand", 8.0f));
    attributes["angle_start"]       = float_to_string(attributes.fetch<float>("dir_range_start", 0.0f));
    attributes["angle_range"]       = float_to_string(attributes.fetch<float>("dir_range_size", 90.0f));
    attributes["const_rot_z"]       = float_to_string(attributes.fetch<float>("const_rotz", -5.0f));
    attributes["const_rot_z_rand"]  = float_to_string(attributes.fetch<float>("const_rotz_rand", 10.0f));

    cParticle_Emitter* p_emitter = new cParticle_Emitter(attributes, p_sprite_manager);
    p_emitter->Set_Spawned(false);

    // clip to the camera
    p_emitter->Set_Clip_Rect(GL_rect(0.0f, 0.0f, static_cast<float>(game_res_w), static_cast<float>(game_res_h) + (attributes.fetch<int>("rect_y", 0) * -1)));
    p_emitter->Set_Based_On_Camera_Pos(1);

    result.push_back(p_emitter);
    return result;
}

std::vector<cSprite*> cLevelLoader::Create_Balls_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager)
{
    std::vector<cSprite*> result;
    result.push_back(new cBall(attributes, p_sprite_manager));
    return result;
}

std::vector<cSprite*> cLevelLoader::Create_Lavas_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager)
{
    std::vector<cSprite*> result;
    result.push_back(new cLava(attributes, p_sprite_manager));
    return result;
}

std::vector<cSprite*> cLevelLoader::Create_Crates_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager)
{
    std::vector<cSprite*> result;
    result.push_back(new cCrate(attributes, p_sprite_manager));
    return result;
}

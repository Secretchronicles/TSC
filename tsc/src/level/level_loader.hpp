/***************************************************************************
 * level_loader.hpp - loading level XML
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

#ifndef TSC_LEVEL_LOADER_HPP
#define TSC_LEVEL_LOADER_HPP
#include "../core/global_game.hpp"
#include "../core/xml_attributes.hpp"
#include "level.hpp"

namespace TSC {

    /**
     * This class is used to construct a level from a given XML file.
     * While technically all its code could be included in cLevel directly,
     * having it as a separate class is much cleaner and doesn’t clutter cLevel
     * with all the parsing stuff which is for the actual work as a Level
     * represenative quite unimportant.
     *
     * Note a cLevelLoader can only be used to parse a given file once.
     * As it internally allocates a cLevel object and exposes it after
     * parsing, we cannot be sure if we can securely delete it before
     * a second run, which is therefore simply forbidden and will result
     * in an XML_Double_Parsing exception to be thrown.
     *
     * Note that the cLevel instance returned by Get_Level() is NOT destroyed
     * when the cLevelLoader gets destroyed. It is handed to you for further
     * processing instead.
     */
    class cLevelLoader: public xmlpp::SaxParser {
    public:
        // Takes the sprite’s main XML tag name, a list of parsed <property> elements
        // and the level’s engine version and creates a cSprite instance from that.
        // That is, the return value usually is a std::vector with a single cSprite
        // in it, but sometimes (notably on backward compatibility), a single XML
        // element breaks up into multiple sprites you then get back all in that vector.
        // This method is static, because it must be accessible from the savegame loader
        // as well.
        static std::vector<cSprite*> Create_Level_Objects_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager);

        cLevelLoader();
        virtual ~cLevelLoader();

        // Parse the given filename. Use this function instead of bare xmlpp’s
        // parse_file() that accepts a Glib::ustring — this function sets
        // some internal members.
        virtual void parse_file(boost::filesystem::path filename);
        // After finishing parsing, contains a pointer to a cLevel instance.
        // This pointer must be freed by you. Returns NULL before parsing.
        cLevel* Get_Level();

    protected: // SAX parser callbacks
        virtual void on_start_document();
        virtual void on_end_document();
        virtual void on_start_element(const Glib::ustring& name, const xmlpp::SaxParser::AttributeList& properties);
        virtual void on_end_element(const Glib::ustring& name);
        virtual void on_characters(const Glib::ustring& text);

    private:
        static std::vector<cSprite*> Create_Sprites_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager);
        static std::vector<cSprite*> Create_Enemy_Stoppers_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager);
        static std::vector<cSprite*> Create_Level_Exits_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager);
        static std::vector<cSprite*> Create_Level_Entries_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager);
        static std::vector<cSprite*> Create_Boxes_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager);
        static std::vector<cSprite*> Create_Items_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager);
        static std::vector<cSprite*> Create_Moving_Platforms_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager);
        static std::vector<cSprite*> Create_Falling_Platforms_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager);
        static std::vector<cSprite*> Create_Enemies_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager);
        static std::vector<cSprite*> Create_Sounds_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager);
        static std::vector<cSprite*> Create_Particle_Emitters_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager);
        static std::vector<cSprite*> Create_Paths_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager);
        static std::vector<cSprite*> Create_Global_Effects_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager);
        static std::vector<cSprite*> Create_Balls_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager);
        static std::vector<cSprite*> Create_Lavas_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager);
        static std::vector<cSprite*> Create_Crates_From_XML_Tag(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager);

        void Parse_Tag_Information();
        void Parse_Tag_Settings();
        void Parse_Tag_Background();
        void Parse_Tag_Player();
        void Parse_Level_Object_Tag(const std::string& name);

        // The cLevel instance we’re building
        cLevel* mp_level;
        // The file we’re parsing
        boost::filesystem::path m_levelfile;
        // The <property> results we found before the current tag. The
        // value of the `name' attribute is mapped to the value of the
        // `value' attribute. on_end_element() must clear this at its end.
        XmlAttributes m_current_properties;
        // True if we’re currently parsing a <script> tag.
        bool m_in_script_tag;
    };

}

#endif

/***************************************************************************
 * overworld_loader.hpp - Loading overworld XML
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

#ifndef TSC_WORLD_LOADER_HPP
#define TSC_WORLD_LOADER_HPP
#include "../core/global_game.hpp"
#include "../core/xml_attributes.hpp"

namespace TSC {

    /**
     * Parser for a world file. This class is used
     * in conjunction with the loaders for layers
     * and description XML files for worlds by
     * the cOverworld constructor.
     *
     * Note this means that the cOverworld instance you
     * get from this parser is **incomplete**! Without
     * employing the two other XML parsers you can’t use
     * the cOverworld for playing; therefore, use cOverworld’s
     * methods for instanciating cOverworld rather than this
     * class directly, which should be considered as a private API.
     */
    class cOverworldLoader: public xmlpp::SaxParser {
    public:
        static cSprite* Create_World_Object_From_XML(const std::string& name, XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager, cOverworld* p_overworld);

        cOverworldLoader();
        virtual ~cOverworldLoader();

        // Parse the given world file. Use this function instead of bare xmlpp’s
        // parse_file() that accepts a Glib::ustring — this function sets
        // some internal members.
        virtual void parse_file(boost::filesystem::path filename);

        cOverworld* Get_Overworld();
    protected: // SAX parser callbacks
        virtual void on_start_document();
        virtual void on_end_document();
        virtual void on_start_element(const Glib::ustring& name, const xmlpp::SaxParser::AttributeList& properties);
        virtual void on_end_element(const Glib::ustring& name);

        // Main tag parsers
        void Parse_Tag_Information();
        void Parse_Tag_Settings();
        void Parse_Tag_Player();
        void Parse_Tag_Background();

    private:
        static cSprite* Create_Sprite_From_XML_Tag(XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager, cOverworld* p_overworld);
        static cSprite* Create_Waypoint_From_XML_Tag(XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager, cOverworld* p_overworld);
        static cSprite* Create_Sound_From_XML_Tag(XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager, cOverworld* p_overworld);
        static cSprite* Create_Line_From_XML_Tag(XmlAttributes& attributes, int engine_version, cSprite_Manager* p_sprite_manager, cOverworld* p_overworld);

        // The cOverworld instance this parser builds up.
        cOverworld* mp_overworld;
        // The world file we’re parsing
        boost::filesystem::path m_worldfile;
        // The <property> results we found before the current tag.
        XmlAttributes m_current_properties;
    };

}

#endif

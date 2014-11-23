/***************************************************************************
 * editor_items_loader.hpp - XML loader for editor items
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

#ifndef TSC_EDITOR_ITEMS_LOADER_HPP
#define TSC_EDITOR_ITEMS_LOADER_HPP
#include "../global_game.hpp"
#include "../xml_attributes.hpp"

namespace TSC {

    class cEditorItemsLoader: public xmlpp::SaxParser {
    public:
        cEditorItemsLoader();
        virtual ~cEditorItemsLoader();

        // Parse the given filename. Use this function instead of bare xmlpp’s
        // parse_file() that accepts a Glib::ustring — this function sets
        // some internal members. `cb' is used to retrieve the sprites;
        // it should point to cLevelLoader::Create_Level_Objects_From_XML_Tag
        // or a similar function. `p_data' is passed as-is to your callback.
        virtual void parse_file(boost::filesystem::path filename, cSprite_Manager* p_sm, void* p_data, std::vector<cSprite*> (*cb)(const std::string&, XmlAttributes&, int, cSprite_Manager*, void*));
        // Retrieve the tagged cSprite instances found by the parser.
        vector<cSprite*> get_tagged_sprites();

    protected: // SAX parser callbacks
        virtual void on_start_document();
        virtual void on_end_document();
        virtual void on_start_element(const Glib::ustring& name, const xmlpp::SaxParser::AttributeList& properties);
        virtual void on_end_element(const Glib::ustring& name);

    private:
        // The file we’re parsing
        boost::filesystem::path m_items_file;
        // The <property> results we found before the current tag. The
        // value of the `name' attribute is mapped to the value of the
        // `value' attribute. on_end_element() must clear this at its end.
        XmlAttributes m_current_properties;

        vector<cSprite*> m_tagged_sprites;
        std::vector<cSprite*> (*mfp_callback)(const std::string&, XmlAttributes&, int, cSprite_Manager*, void*);
        cSprite_Manager* mp_sprite_manager;

        // User data for callback
        void* mp_data;
    };

}

#endif

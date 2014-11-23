/***************************************************************************
 * overworld_description_loader.hpp - Loading overworld description XML
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

#ifndef TSC_WORLD_DESCRIPTION_LOADER_HPP
#define TSC_WORLD_DESCRIPTION_LOADER_HPP
#include "../core/global_game.hpp"
#include "../core/xml_attributes.hpp"
#include "overworld.hpp"

namespace TSC {

    class cOverworldDescriptionLoader: public xmlpp::SaxParser {
    public:
        cOverworldDescriptionLoader();
        virtual ~cOverworldDescriptionLoader();

        // Parse the given filename.
        virtual void parse_file(boost::filesystem::path filename);

        cOverworld_description* Get_Overworld_Description();

    protected:
        // SAX parser callbacks
        virtual void on_start_document();
        virtual void on_end_document();
        virtual void on_start_element(const Glib::ustring& name, const xmlpp::SaxParser::AttributeList& properties);
        virtual void on_end_element(const Glib::ustring& name);

        void handle_world();

    private:
        // The description we’re building.
        cOverworld_description* mp_description;
        // The file we’re parsing.
        boost::filesystem::path m_descfile;
        // The <property> results we found before the current tag.
        XmlAttributes m_current_properties;
    };

}

#endif

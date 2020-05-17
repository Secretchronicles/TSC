/***************************************************************************
 * campaign_loader.hpp - Loading campaign XML
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

#ifndef TSC_CAMPAIGN_LOADER_HPP
#define TSC_CAMPAIGN_LOADER_HPP
#include "../core/global_game.hpp"
#include "../core/xml_attributes.hpp"
#include "campaign_manager.hpp"

namespace TSC {

    class cCampaignLoader: public xmlpp::SaxParser {
    public:
        cCampaignLoader();
        virtual ~cCampaignLoader();

        // Parse the given filename. Use this function instead of
        // bare xmlpp’s parse_file() that accepts a Glib::ustring —
        // this function sets some internal members.
        virtual void parse_file(boost::filesystem::path filename);
        // After finishing parsing, contains a pointer to a cCampaign
        // instance. This pointer must be freed by you. Returns NULL
        // before parsing.
        cCampaign* Get_Campaign();

    protected: // SAX parser callbacks
        virtual void on_start_document();
        virtual void on_end_document();
        virtual void on_start_element(const Glib::ustring& name, const xmlpp::SaxParser::AttributeList& properties);
        virtual void on_end_element(const Glib::ustring& name);

        void Handle_Information();
        void Handle_Target();

        cCampaign* mp_campaign;
        boost::filesystem::path m_campaignfile;
        XmlAttributes m_current_properties;
    };

}
#endif

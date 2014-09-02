#include "../core/filesystem/vfs.hpp"
#include "campaign_loader.hpp"
#include "../core/global_basic.hpp"

namespace fs = boost::filesystem;
using namespace SMC;

using namespace std;

cCampaignLoader::cCampaignLoader()
    : xmlpp::SaxParser()
{
    mp_campaign = NULL;
}

cCampaignLoader::~cCampaignLoader()
{
    // Do not delete the cCampaign instance — it is used by the
    // caller and deleted by him.
    mp_campaign = NULL;
}

cCampaign* cCampaignLoader::Get_Campaign()
{
    return mp_campaign;
}

/***************************************
 * SAX parser callbacks
 ***************************************/

void cCampaignLoader::parse_file(boost::filesystem::path filename)
{
    m_campaignfile = filename;
    std::istream* s = pVfs->Open_Stream(filename);
    if(s)
    {
        xmlpp::SaxParser::parse_stream(*s);
        delete s;
    }
}

void cCampaignLoader::on_start_document()
{
    if (mp_campaign)
        throw(RestartedXmlParserError());

    mp_campaign = new cCampaign();
}

void cCampaignLoader::on_end_document()
{
    //
}

void cCampaignLoader::on_start_element(const Glib::ustring& name, const xmlpp::SaxParser::AttributeList& properties)
{
    if (name == "property") {
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
}

void cCampaignLoader::on_end_element(const Glib::ustring& name)
{
    // <property> tags are parsed cumulatively in on_start_element()
    // so all have been collected when the surrounding element
    // terminates here.
    if (name == "property")
        return;

    if (name == "information")
        Handle_Information();
    else if (name == "target")
        Handle_Target();
    else if (name == "campaign") {
        /* Ignore */
    }
    else
        cerr << "Warning: Campaign unknown element '" << name << "'." << endl;

    m_current_properties.clear();
}

void cCampaignLoader::Handle_Information()
{
    mp_campaign->m_name         = m_current_properties["name"];
    mp_campaign->m_description  = m_current_properties["description"];
    mp_campaign->m_last_saved   = string_to_int64(m_current_properties["save_time"]);
}

void cCampaignLoader::Handle_Target()
{
    mp_campaign->m_target = m_current_properties["name"];
    mp_campaign->m_is_target_level = m_current_properties.retrieve<bool>("is_level");
}

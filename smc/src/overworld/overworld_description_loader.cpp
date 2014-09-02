#include "../core/filesystem/vfs.hpp"
#include "overworld_description_loader.hpp"
#include "overworld.hpp"
#include "../core/global_basic.hpp"

namespace fs = boost::filesystem;
using namespace SMC;

using namespace std;

cOverworldDescriptionLoader::cOverworldDescriptionLoader()
    : xmlpp::SaxParser()
{
    mp_description = NULL;
}

cOverworldDescriptionLoader::~cOverworldDescriptionLoader()
{
    // Do not delete the cOverworld_description instance — it
    // is used by the caller and deleted by him.
    mp_description = NULL;
}

cOverworld_description* cOverworldDescriptionLoader::Get_Overworld_Description()
{
    return mp_description;
}

/***************************************
 * SAX parser callbacks
 ***************************************/

void cOverworldDescriptionLoader::parse_file(fs::path filename)
{
    m_descfile = filename;
    std::istream* s = pVfs->Open_Stream(filename);
    if(s) {
        xmlpp::SaxParser::parse_stream(*s);
        delete s;
    }
}

void cOverworldDescriptionLoader::on_start_document()
{
    if (mp_description)
        throw("Restarted XML parser after already starting it."); // FIXME: proper exception

    mp_description = new cOverworld_description();
}

void cOverworldDescriptionLoader::on_end_document()
{

}

void cOverworldDescriptionLoader::on_start_element(const Glib::ustring& name, const xmlpp::SaxParser::AttributeList& properties)
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

void cOverworldDescriptionLoader::on_end_element(const Glib::ustring& name)
{
    // Already handled
    if (name == "property" || name == "Property")
        return;

    if (name == "world" || name == "World")
        handle_world();
    else if (name == "description" || name == "Description") {
        /* Ignore */
    }
    else
        cerr << "Warning: Unknown World Description element '" << name << "'." << endl;

    m_current_properties.clear();
}

void cOverworldDescriptionLoader::handle_world()
{
    mp_description->m_name = m_current_properties.fetch("name", mp_description->m_name);
    mp_description->m_visible = m_current_properties.fetch<bool>("visible", true);
}

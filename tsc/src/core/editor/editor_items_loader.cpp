/***************************************************************************
 * editor_items_loader.cpp - XML loader for editor items
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

#include "../global_basic.hpp"
#include "editor_items_loader.hpp"
#include "../../objects/sprite.hpp"

using namespace std;

using namespace TSC;
namespace fs = boost::filesystem;

cEditorItemsLoader::cEditorItemsLoader()
{
    //
}

cEditorItemsLoader::~cEditorItemsLoader()
{
    //
}

void cEditorItemsLoader::parse_file(fs::path filename, cSprite_Manager* p_sm, void* p_data, std::vector<cSprite*> (*cb)(const std::string&, XmlAttributes&, int, cSprite_Manager*, void*))
{
    m_items_file = filename;
    mp_sprite_manager = p_sm;
    mfp_callback = cb;
    mp_data = p_data;
    xmlpp::SaxParser::parse_file(path_to_utf8(filename));
}

void cEditorItemsLoader::on_start_document()
{
    //
}

void cEditorItemsLoader::on_end_document()
{
    //
}

void cEditorItemsLoader::on_start_element(const Glib::ustring& name, const xmlpp::SaxParser::AttributeList& properties)
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
}

void cEditorItemsLoader::on_end_element(const Glib::ustring& name)
{
    // <property> tags are parsed cumulatively in on_start_element()
    // so all have been collected when the surrounding element
    // terminates here.
    if (name == "property")
        return;
    if (name == "items") // ignore root element
        return;

    std::string objname = m_current_properties["object_name"];
    std::string tags = m_current_properties["object_tags"];

    /* The orindary level loaders suffer from a problem that the editor items
     * loader does not have: legacy objects. Since it is possible that a level
     * contains tags that the parser bursts into *multiple* sprite objects the
     * level loading code must take care of adding an entire group of sprites for
     * a single XML object to the level. However, the level_items.xml and world_items.xml
     * files *never* contain legacy tags of this nature, because they're part of the
     * TSC distribution itself and the devs keep them up to date. In other words:
     * the editor does not even allow the user to place legacy objects in a level.
     * Thus, each tag in the *_items.xml file is guaranteed to create exactly one
     * sprite on parsing. Hence it is okay if we reduce the std::vector<cSprite*>
     * vector returned by the level parser to its first element. There cannot,
     * ever, be more than one element in this vector if the *_items.xml files are
     * maintained properly by the TSC devs and do not contain legacy tags. */
    std::vector<cSprite*> sprites = mfp_callback(objname, m_current_properties, level_engine_version, mp_sprite_manager, mp_data);

    if (sprites.empty()) {
        cerr << "Warning: Editor item could not be created: " << objname << endl;
        return;
    }
    else if (sprites.size() > 1) {
        std::string str = "Legacy tag '";
        str += objname;
        str += "' in editor items XML file '";
        str += path_to_utf8(m_items_file);
        str += "' detected! This is a bug. The editor items XML file needs to be updated to the current set of tags!";
        throw(std::runtime_error(str));
    }

    // Cf. above why we can reduce the vector to its first element.
    // Once the parser does not produce legacy output with multi-sprite
    // elements anymore, simplify this code accordingly.
    sprites[0]->m_editor_tags = tags.c_str();
    m_tagged_sprites.push_back(sprites[0]);

    // Prepare for next element
    m_current_properties.clear();
}

vector<cSprite*> cEditorItemsLoader::get_tagged_sprites()
{
    return m_tagged_sprites;
}

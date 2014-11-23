/***************************************************************************
 * editor_items_loader.cpp - XML loader for editor items
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

#include "editor_items_loader.hpp"
#include "../../objects/sprite.hpp"

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

    std::string objname = m_current_properties["object_name"];
    std::string tags = m_current_properties["object_tags"];

    // TODO: When backward compatibility is removed, reduce to ONE
    // cSprite* instead of this stupid vector. Currently backward
    // compatibility may cause a single XML tag to explode to multiple
    // sprites.
    std::vector<cSprite*> sprites = mfp_callback(objname, m_current_properties, level_engine_version, mp_sprite_manager, mp_data);

    if (sprites.empty()) {
        std::cerr << "Warning: Editor item could not be created: " << objname << std::endl;
        return;
    }

    // Also, the tags would then only be set on this one sprite.
    // This really looks stupid this way. Really. I mean really.
    sprites[0]->m_editor_tags = tags.c_str();

    std::vector<cSprite*>::iterator iter;
    for (iter=sprites.begin(); iter != sprites.end(); iter++)
        m_tagged_sprites.push_back(*iter);

    m_current_properties.clear();
}

vector<cSprite*> cEditorItemsLoader::get_tagged_sprites()
{
    return m_tagged_sprites;
}

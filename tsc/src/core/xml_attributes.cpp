/***************************************************************************
 * xml_attributes.cpp - XML attributes editing
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

#include "xml_attributes.hpp"
#include "filesystem/resource_manager.hpp"
#include "property_helper.hpp"

namespace TSC {

void XmlAttributes::relocate_image(const std::string& filename_old, const std::string& filename_new, const std::string& attribute_name /* = "image" */)
{
    std::string current_value = (*this)[attribute_name];
    std::string filename_old_full = path_to_utf8(pResource_Manager->Get_Game_Pixmaps_Directory() / filename_old);

    if (current_value == filename_old || current_value == filename_old_full)
        (*this)[attribute_name] = filename_new;
}

bool XmlAttributes::exists(const std::string& key)
{
    if (count(key) > 0)
        return true;
    else
        return false;
}
}

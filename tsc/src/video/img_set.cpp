/***************************************************************************
 * img_set.cpp  -  Image set container
 *
 * Copyright © 2003 - 2011 Florian Richter
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

#include "../core/game_core.hpp"
#include "../core/framerate.hpp"
#include "../video/gl_surface.hpp"
#include "../video/renderer.hpp"
#include "../core/i18n.hpp"
#include "../core/file_parser.hpp"
#include "../core/filesystem/resource_manager.hpp"
#include "../core/filesystem/package_manager.hpp"
#include "../core/math/utilities.hpp"
#include "../core/global_basic.hpp"

using namespace std;

namespace fs = boost::filesystem;

namespace TSC {

/* *** *** *** *** *** *** *** cImageSet_FrameInfo *** *** *** *** *** *** *** *** *** *** */
cImageSet_FrameInfo::cImageSet_FrameInfo()
{
    m_time_min = 0;
    m_time_max = 0;
}

/* *** *** *** *** *** *** *** cImageSet_Parser *** *** *** *** *** *** *** *** *** *** */
cImageSet_Parser::cImageSet_Parser(Uint32 time)
    : m_time_min(time), m_time_max(time)
{
}

bool cImageSet_Parser::HandleMessage(const std::string* parts, unsigned int count, unsigned int line)
{
    if(count == 2 && parts[0] == "time") {
        // set min and max times to same thing
        m_time_min = m_time_max = string_to_int(parts[1]);
    }
    else if(count == 3 && parts[0] == "time") {
        // set min and max times
        m_time_min = string_to_int(parts[1]);
        m_time_max = string_to_int(parts[2]);
    }
    else if(count > 0) {
        cImageSet_FrameInfo info;

        // initial values, combine filename with imageset path
        info.m_filename = data_file.parent_path() / utf8_to_path(parts[0]);
        info.m_time_min = m_time_min;
        info.m_time_max = m_time_max;

        // parse the rest
        for(unsigned int idx = 1; idx < count; idx++)
        {
            if(parts[idx] == "time") {
                if(idx + 2 < count) {
                    // first is min time, second is max time
                    info.m_time_min = string_to_int(parts[idx + 1]);
                    info.m_time_max = string_to_int(parts[idx + 2]);
                }
                idx += 2;
            }
            else if(parts[idx] == "branch") {
                if(idx + 2 < count) {
                    // first is frame number (starting at 0), second is percentage likely to branch
                    cImageSet_FrameInfo::Entry_Type branch(string_to_int(parts[idx + 1]), string_to_int(parts[idx + 2]));
                    info.m_branches.push_back(branch);
                }
                idx += 2;
            }
        }

        // add to list
        m_images.push_back(info);
    }

    return 1;
}

/* *** *** *** *** *** *** *** cImageSet_Surface *** *** *** *** *** *** *** *** *** *** */

cImageSet_Surface::cImageSet_Surface(void)
{
    m_image = NULL;
    m_time = 0;
}

cImageSet_Surface::~cImageSet_Surface(void)
{
    //
}

void cImageSet_Surface::Enter(void)
{
    // set random time for this frame
    m_time = m_info.m_time_min + rand() % (m_info.m_time_max - m_info.m_time_min + 1);
}

int cImageSet_Surface::Leave(void)
{
    // determine any branching to other frames
    if(m_info.m_branches.size() == 0)
        return -1;

    int rnd = rand() % 101;
    for(cImageSet_FrameInfo::List_Type::iterator it = m_info.m_branches.begin(); it != m_info.m_branches.end(); ++it)
    {
        // first is frame number, second is percentage
        if(rnd <= it->second) {
            return it->first;
        }

        // remove that from rnd
        rnd = rnd - it->second;
    }

    // no branch
    return -1;
}

/* *** *** *** *** *** *** *** cImageSet *** *** *** *** *** *** *** *** *** *** */

cImageSet::cImageSet()
{
    // animation data
    m_curr_img = -1;
    m_anim_enabled = 0;
    m_anim_img_start = 0;
    m_anim_img_end = 0;
    m_anim_time_default = 1000;
    m_anim_counter = 0;
    m_anim_last_ticks = pFramerate->m_last_ticks - 1;
    m_anim_mod = 1.0f;
}

cImageSet::~cImageSet()
{
}

void cImageSet::Add_Image(cGL_Surface* image, Uint32 time /* = 0 */)
{
    // set to default time
    if (time == 0) {
        time = m_anim_time_default;
    }

    cImageSet_Surface obj;
    obj.m_image = image;
    obj.m_time = time;

    // we may not be adding from an image set, so set up some initial information
    obj.m_info.m_time_min = time;
    obj.m_info.m_time_max = time;

    m_images.push_back(obj);
}

bool cImageSet::Add_Image_Set(const std::string& name, boost::filesystem::path path, Uint32 time /* = 0 */, int* start_num /* = NULL */, int* end_num /* = NULL */)
{
    int start, end;
    boost::filesystem::path filename;

    // Set to -1 until added
    if(start_num)
        *start_num = -1;

    if(end_num)
        *end_num = -1;

    start = m_images.size();
    if(path.extension() == utf8_to_path(".png")) {
        // Adding a single image
        Add_Image(pVideo->Get_Package_Surface(path), time);
        filename = path;
    }
    else {
        // Parse the animation file
        filename = pPackage_Manager->Get_Pixmap_Reading_Path(path_to_utf8(path));
        if(filename == boost::filesystem::path()) {
            cerr << "Warning: Unable to load image set: " << name << Get_Identity() << endl;
            return false;
        }

        cImageSet_Parser parser(time);
        if(!parser.Parse(path_to_utf8(filename))) {
            cerr << "Warning: Unable to parse image set: " << filename << endl;
            return false;
        }

        if(parser.m_images.size() == 0) {
            cerr << "Warning: Empty image set: " << filename << endl;
            return false;
        }

        // Add images
        for(cImageSet_Parser::List_Type::iterator itr = parser.m_images.begin(); itr != parser.m_images.end(); ++itr) {
            Add_Image(pVideo->Get_Package_Surface(itr->m_filename), itr->m_time_min);

            // update info
            m_images.back().m_info = *itr;
        }
    }
    end = m_images.size() - 1;

    // Add the item
    if(end >= start) {
        m_named_ranges[name] = std::pair<int, int>(start, end);

        if(start_num)
            *start_num = start;

        if(end_num)
            *end_num = end;

        return true;
    }
    else {
        cerr << "Warning: No image set added: " << filename << endl;
        return false;
    }
}

bool cImageSet::Set_Image_Set(const std::string& name, bool new_startimage /* =0 */)
{
    cImageSet_Name_Map::iterator it = m_named_ranges.find(name);

    if(it == m_named_ranges.end()) {
        cerr << "Warning: Named image set not found: " << name << Get_Identity() << endl;
        Set_Image_Num(-1, new_startimage);
        Set_Animation(0);
        return false;
    }
    else {
        int start = it->second.first;
        int end = it->second.second;

        Set_Image_Num(start, new_startimage);
        Set_Animation_Image_Range(start, end);
        Set_Animation((end > start)); // True if more than one image
        Reset_Animation();

        return true;
    }
}

void cImageSet::Set_Image_Num(const int num, bool new_startimage /* = 0 */)
{
    if (m_curr_img == num) {
        return;
    }

    m_curr_img = num;

    if (m_curr_img < 0) {
        // BUG: Setting m_image to NULL may crash if code direclty access m_image such as m_image->m_w
        // This may happen if a loaded image set does not exist but the code still calls Set_Image_Set
        // then tries to acces m_image. Perhaps this should set some type of blank image instead.
        Set_Image_Set_Image(NULL, new_startimage);
    }
    else if (m_curr_img < static_cast<int>(m_images.size())) {
        Set_Image_Set_Image(m_images[m_curr_img].m_image, new_startimage);
    }
    else {
        debug_print("Warning : Object image number %d bigger as the array size %u%s", m_curr_img, static_cast<unsigned int>(m_images.size()), Get_Identity().c_str());
    }
}

cGL_Surface* cImageSet::Get_Image(const unsigned int num) const
{
    if (num >= m_images.size()) {
        return NULL;
    }

    return m_images[num].m_image;
}

void cImageSet::Clear_Images(void)
{
    m_curr_img = -1;
    m_images.clear();
    m_named_ranges.clear();
}

void cImageSet::Update_Animation(void)
{
    // prevent calling twice within the same update cycle
    if (m_anim_last_ticks == pFramerate->m_last_ticks) {
        return;
    }
    m_anim_last_ticks = pFramerate->m_last_ticks;

    // if not valid
    if (!m_anim_enabled || m_anim_img_end == 0) {
        return;
    }

    m_anim_counter += pFramerate->m_elapsed_ticks;

    // out of range
    if (m_curr_img < 0 || m_curr_img >= static_cast<int>(m_images.size())) {
        cerr << "Warning: Animation image " << m_curr_img << " for " << Get_Identity() << " out of range (max " << (m_images.size() - 1) << "). Forcing start image." << endl;
        Set_Image_Num(m_anim_img_start);
        return;
    }

    cImageSet_Surface& image = m_images[m_curr_img];

    if (static_cast<Uint32>(m_anim_counter * m_anim_mod) >= image.m_time) {
        // leave old image
        int branch_target = image.Leave();
        if (branch_target >= 0) {
            // add target for next image to the image set starting point
            branch_target += m_anim_img_start;
        }

        // branch if needed
        if(branch_target >= m_anim_img_start && branch_target <= m_anim_img_end) {
            Set_Image_Num(branch_target);
        }
        else if (m_curr_img >= m_anim_img_end) {
            Set_Image_Num(m_anim_img_start);
        }
        else {
            Set_Image_Num(m_curr_img + 1);
        }

        // enter new image after updating animation counter
        m_anim_counter = static_cast<Uint32>(m_anim_counter * m_anim_mod) - image.m_time;
        m_images[m_curr_img].Enter();
    }

    return;
}

void cImageSet::Set_Time_All(const Uint32 time, const bool default_time /* = 0 */)
{
    for (cImageSet_Surface_List::iterator itr = m_images.begin(); itr != m_images.end(); ++itr) {
        cImageSet_Surface& obj = (*itr);
        obj.m_time = time;
        obj.m_info.m_time_min = time;
        obj.m_info.m_time_max = time;
    }

    if (default_time) {
        Set_Default_Time(time);
    }
}

/* *** *** *** *** *** *** cSimpleImageSet *** *** *** *** *** *** *** *** *** */
cSimpleImageSet::cSimpleImageSet()
    : m_image(NULL)
{
}

cSimpleImageSet::~cSimpleImageSet()
{
}

std::string cSimpleImageSet::Get_Identity(void)
{
    return m_identity;
}

void cSimpleImageSet::Set_Image_Set_Image(cGL_Surface* new_image, bool new_startimage /*=0*/)
{
    m_image = new_image;
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

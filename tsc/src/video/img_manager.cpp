/***************************************************************************
 * img_manager.cpp  -  Image Handler/Manager
 *
 * Copyright © 2003 - 2011 Florian Richter
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

#include "../video/img_manager.hpp"
#include "../video/renderer.hpp"
#include "../video/loading_screen.hpp"
#include "../core/i18n.hpp"
#include "../core/global_basic.hpp"
#include "../core/property_helper.hpp"

using namespace std;

namespace fs = boost::filesystem;

namespace TSC {

/* *** *** *** *** *** cSaved_Texture *** *** *** *** *** *** *** *** *** *** *** *** */

cSaved_Texture::cSaved_Texture(void)
{
    m_base = NULL;
    m_pixels = NULL;

    m_width = 0;
    m_height = 0;
    m_format = 0;

    m_min_filter = 0;
    m_mag_filter = 0;
    m_wrap_s = 0;
    m_wrap_t = 0;
}

cSaved_Texture::~cSaved_Texture(void)
{
    if (m_pixels) {
        delete[] m_pixels;
    }
}

/* *** *** *** *** *** *** cImage_Manager *** *** *** *** *** *** *** *** *** *** *** */

cImage_Manager::cImage_Manager(void)
    : cObject_Manager<cGL_Surface>()
{
    m_high_texture_id = 0;
}

cImage_Manager::~cImage_Manager(void)
{
    cImage_Manager::Delete_All();
}

void cImage_Manager::Add(cGL_Surface* obj)
{
    if (!obj) {
        return;
    }

    // it is now managed
    obj->m_managed = 1;

    // Add and remember index where it was stored
    cObject_Manager<cGL_Surface>::Add(obj);
    m_index_table[path_to_utf8(obj->m_path)] = objects.size() - 1;
}

cGL_Surface* cImage_Manager::Get_Pointer(const fs::path& path)
{
    std::unordered_map<std::string, size_t>::iterator iter =
        m_index_table.find(path_to_utf8(path));

    if (iter == m_index_table.end()) {
        // not found
        return NULL;
    }
    else {
        return objects[iter->second];
    }
}

cGL_Surface* cImage_Manager::Copy(const fs::path& path)
{
    std::unordered_map<std::string, size_t>::iterator iter =
        m_index_table.find(path_to_utf8(path));

    if (iter == m_index_table.end()) {
        // not found
        return NULL;
    }
    else {
        return objects[iter->second]->Copy();
    }
}

// Must be called on the loading screen, i.e. after Loading_Screen_Init() and
// before Loading_Screen_Exit().
void cImage_Manager::Grab_Textures(bool from_file /* = 0 */, bool draw_gui /* = 0 */)
{
    // progress bar
    CEGUI::ProgressBar* progress_bar = NULL;

    if (draw_gui) {
        // get progress bar
        progress_bar = static_cast<CEGUI::ProgressBar*>(CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("loadingroot/progress_bar"));
        progress_bar->setProgress(0);
        // set loading screen text
        Loading_Screen_Draw_Text(_("Saving Textures"));
    }

    unsigned int loaded_files = 0;
    unsigned int file_count = objects.size();

    // save all textures
    for (GL_Surface_List::iterator itr = objects.begin(); itr != objects.end(); ++itr) {
        // get surface
        cGL_Surface* obj = (*itr);

        // skip surfaces with an already deleted texture
        if (!glIsTexture(obj->m_image)) {
            continue;
        }

        // get software texture and save it to software memory
        m_saved_textures.push_back(obj->Get_Software_Texture(from_file));
        // delete hardware texture
        if (glIsTexture(obj->m_image)) {
            glDeleteTextures(1, &obj->m_image);
        }

        // count files
        loaded_files++;

        // draw
        if (draw_gui) {
            // update progress
            progress_bar->setProgress(static_cast<float>(loaded_files) / static_cast<float>(file_count));

            Loading_Screen_Draw();
        }
    }
}

void cImage_Manager::Restore_Textures(bool draw_gui /* = 0 */)
{
    // progress bar
    CEGUI::ProgressBar* progress_bar = NULL;

    if (draw_gui) {
        // get progress bar
        progress_bar = static_cast<CEGUI::ProgressBar*>(CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("loadingroot/progress_bar"));
        progress_bar->setProgress(0);
        // set loading screen text
        Loading_Screen_Draw_Text(_("Restoring Textures"));
    }

    unsigned int loaded_files = 0;
    unsigned int file_count = m_saved_textures.size();

    // load back into hardware textures
    for (Saved_Texture_List::iterator itr = m_saved_textures.begin(); itr != m_saved_textures.end(); ++itr) {
        // get saved texture
        cSaved_Texture* soft_tex = (*itr);

        // load it
        soft_tex->m_base->Load_Software_Texture(soft_tex);
        // delete
        delete soft_tex;

        // count files
        loaded_files++;

        // draw
        if (draw_gui) {
            // update progress
            progress_bar->setProgress(static_cast<float>(loaded_files) / static_cast<float>(file_count));

            Loading_Screen_Draw();
        }
    }

    m_saved_textures.clear();
}

void cImage_Manager::Delete_Image_Textures(void)
{
    for (GL_Surface_List::iterator itr = objects.begin(); itr != objects.end(); ++itr) {
        // get object
        cGL_Surface* obj = (*itr);

        if (obj->m_auto_del_img && glIsTexture(obj->m_image)) {
            glDeleteTextures(1, &obj->m_image);
        }
    }
}

bool cImage_Manager::Delete(size_t array_num, bool delete_data)
{
    if (array_num < objects.size()) {
        std::string filepath = path_to_utf8(objects[array_num]->m_path);
        objects.erase(objects.begin() + array_num);
        m_index_table.erase(filepath);
    }

    if (delete_data) {
        delete objects[array_num];
    }

    return 1;
}

bool cImage_Manager::Delete(cGL_Surface* obj, bool delete_data)
{
    std::string filepath = path_to_utf8(obj->m_path);
    if (cObject_Manager::Delete(obj, delete_data)) {
        m_index_table.erase(filepath);
        return true;
    }
    else {
        return false;
    }
}

void cImage_Manager::Delete_Hardware_Textures(void)
{
    // delete all hardware surfaces
    for (GLuint i = 0; i < m_high_texture_id; i++) {
        if (glIsTexture(i)) {
            cout << "ImageManager : deleting texture " << i << endl;
            glDeleteTextures(1, &i);
        }
    }

    m_high_texture_id = 0;
}

void cImage_Manager::Delete_All(void)
{
    // stops cGL_Surface destructor from checking if GL texture id still in use
    Delete_Image_Textures();
    cObject_Manager<cGL_Surface>::Delete_All();
    m_index_table.clear();
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

cImage_Manager* pImage_Manager = NULL;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

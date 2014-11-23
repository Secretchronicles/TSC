/***************************************************************************
 * font.cpp  -  internal font functions
 *
 * Copyright © 2006 - 2011 Florian Richter
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

#include "../video/font.hpp"
#include "../video/gl_surface.hpp"
#include "../core/property_helper.hpp"
#include "../core/filesystem/resource_manager.hpp"

namespace TSC {

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

void Font_Delete_Ref(cGL_Surface* surface)
{
    pFont->Delete_Ref(surface);
}

/* *** *** *** *** *** *** *** Font Manager class *** *** *** *** *** *** *** *** *** *** */

cFont_Manager::cFont_Manager(void)
{
    m_font_normal = NULL;
    m_font_small = NULL;
    m_font_very_small = NULL;
}

cFont_Manager::~cFont_Manager(void)
{
    // if not initialized
    if (!TTF_WasInit()) {
        return;
    }

    if (m_font_normal) {
        TTF_CloseFont(m_font_normal);
        m_font_normal = NULL;
    }

    if (m_font_small) {
        TTF_CloseFont(m_font_small);
        m_font_small = NULL;
    }

    if (m_font_very_small) {
        TTF_CloseFont(m_font_very_small);
        m_font_very_small = NULL;
    }

    TTF_Quit();
}

void cFont_Manager::Init(void)
{
    // if already initialised
    if (TTF_WasInit()) {
        return;
    }

    // init ttf
    if (TTF_Init() == -1) {
        printf("Error : SDL_TTF initialization failed\nReason : %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    // open fonts
    m_font_normal           = TTF_OpenFont(path_to_utf8(pResource_Manager->Get_Gui_Font_Directory() / utf8_to_path("default_bold.ttf")).c_str(), 18);
    m_font_small            = TTF_OpenFont(path_to_utf8(pResource_Manager->Get_Gui_Font_Directory() / utf8_to_path("default_bold.ttf")).c_str(), 11);
    m_font_very_small = TTF_OpenFont(path_to_utf8(pResource_Manager->Get_Gui_Font_Directory() / utf8_to_path("default_bold.ttf")).c_str(), 9);

    // if loading failed
    if (!m_font_normal || !m_font_small || !m_font_very_small) {
        // FIXME: Throw a proper exception
        throw "Font loading failed";
    }
}

void cFont_Manager::Add_Ref(cGL_Surface* surface)
{
    if (!surface) {
        return;
    }

    m_active_fonts.push_back(surface);
}

void cFont_Manager::Delete_Ref(cGL_Surface* surface)
{
    for (ActiveFontList::iterator itr = m_active_fonts.begin(); itr != m_active_fonts.end(); ++itr) {
        cGL_Surface* obj = (*itr);

        // delete reference if found
        if (obj == surface) {
            m_active_fonts.erase(itr);
            return;
        }
    }
}

cGL_Surface* cFont_Manager::Render_Text(TTF_Font* font, const std::string& text, const Color color)
{
    // get SDL Color
    SDL_Color sdlcolor = color.Get_SDL_Color();
    // create text surface
    cGL_Surface* surface = pVideo->Create_Texture(TTF_RenderUTF8_Blended(font, text.c_str(), sdlcolor));

    if (!surface) {
        return NULL;
    }

    surface->m_path = utf8_to_path(text);

    // set function if font gets deleted
    surface->Set_Destruction_Function(&Font_Delete_Ref);
    // add font to active fonts
    Add_Ref(surface);

    return surface;
}

void cFont_Manager::Grab_Textures(void)
{
    // save to software memory
    for (ActiveFontList::iterator itr = m_active_fonts.begin(); itr != m_active_fonts.end(); ++itr) {
        cGL_Surface* obj = (*itr);

        // get software texture and save it
        m_software_textures.push_back(obj->Get_Software_Texture());
        // delete hardware texture
        if (glIsTexture(obj->m_image)) {
            glDeleteTextures(1, &obj->m_image);
        }
        obj->m_image = 0;
    }
}

void cFont_Manager::Restore_Textures(void)
{
    // load back into hardware textures
    for (Saved_Texture_List::iterator itr = m_software_textures.begin(); itr != m_software_textures.end(); ++itr) {
        // get saved texture
        cSaved_Texture* soft_tex = (*itr);
        // load it
        soft_tex->m_base->Load_Software_Texture(soft_tex);
    }

    // delete software textures
    for (Saved_Texture_List::iterator itr = m_software_textures.begin(); itr != m_software_textures.end(); ++itr) {
        delete *itr;
    }

    m_software_textures.clear();
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

cFont_Manager* pFont = NULL;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

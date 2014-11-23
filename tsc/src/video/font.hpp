/***************************************************************************
 * font.h
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

#ifndef TSC_FONT_HPP
#define TSC_FONT_HPP

#include "../core/global_basic.hpp"
#include "../video/img_manager.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** Font Manager class *** *** *** *** *** *** *** *** *** *** */

// Deletes an active Font Surface
    void Font_Delete_Ref(cGL_Surface* surface);

    class cFont_Manager {
    public:
        cFont_Manager(void);
        ~cFont_Manager(void);

        // initialization
        void Init(void);

        // Adds an allocated Font surface
        void Add_Ref(cGL_Surface* surface);
        // Deletes an active Font Surface
        void Delete_Ref(cGL_Surface* surface);

        // Renders the given text into a new surface
        cGL_Surface* Render_Text(TTF_Font* font, const std::string& text, const Color color = static_cast<Uint8>(0));

        /* Saves hardware textures in software memory
        */
        void Grab_Textures(void);

        /* Loads the saved software textures back into hardware textures
        */
        void Restore_Textures(void);

        // TTF loaded fonts
        TTF_Font* m_font_normal;
        TTF_Font* m_font_small;
        TTF_Font* m_font_very_small;

        // current active loaded font list
        typedef vector<cGL_Surface*> ActiveFontList;
        ActiveFontList m_active_fonts;

        // saved software textures only used for reloading
        Saved_Texture_List m_software_textures;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

// Font Handler
    extern cFont_Manager* pFont;

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

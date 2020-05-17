/***************************************************************************
 * img_manager.h
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

#ifndef TSC_IMG_MANAGER_HPP
#define TSC_IMG_MANAGER_HPP

#include "../core/global_basic.hpp"
#include "../video/video.hpp"
#include "../core/obj_manager.hpp"
#include "../video/gl_surface.hpp"

namespace TSC {

    /* *** *** *** *** *** cSaved_Texture *** *** *** *** *** *** *** *** *** *** *** *** */

// software texture data
    class cSaved_Texture {
    public:
        cSaved_Texture(void);
        ~cSaved_Texture(void);

        // base surface
        cGL_Surface* m_base;

        // pixel data
        GLubyte* m_pixels;

        // size
        GLint m_width;
        GLint m_height;
        // format
        GLint m_format;

        // settings
        GLint m_min_filter;
        GLint m_mag_filter;
        GLint m_wrap_s;
        GLint m_wrap_t;
    };

    typedef vector<cSaved_Texture*> Saved_Texture_List;
    typedef vector<cGL_Surface*> GL_Surface_List;

    /* *** *** *** *** *** *** cImage_Manager *** *** *** *** *** *** *** *** *** *** *** */

//  Keeps track of all the images in memory
//
// Operators:
//  - cImage_Manager [path]
//  - cImage_Manager [identifier]
    class cImage_Manager : public cObject_Manager<cGL_Surface> {
    public:
        cImage_Manager(void);
        virtual ~cImage_Manager(void);

        // Add a surface
        virtual void Add(cGL_Surface* obj);

        // Return the surface by path
        cGL_Surface* Get_Pointer(const boost::filesystem::path& path);

        // Return the copied image
        cGL_Surface* Copy(const boost::filesystem::path& path);

        cGL_Surface* operator [](unsigned int identifier)
        {
            return cObject_Manager<cGL_Surface>::Get_Pointer(identifier);
        }

        cGL_Surface* operator [](const std::string& path)
        {
            return Get_Pointer(path);
        }

        /* Save hardware textures in software memory
         * from_file: if set don't store in software memory but load again from file
         * draw_gui : if set use the loading screen gui for drawing
        */
        void Grab_Textures(bool from_file = 0, bool draw_gui = 0);

        /* Load the saved software textures back into hardware textures or
         * load textures from file if set
         * draw_gui : if set use the loading screen gui for drawing
        */
        void Restore_Textures(bool draw_gui = 0);

        // Delete all surface textures, but keep object vector entries
        void Delete_Image_Textures(void);

        // Delete all hardware surfaces
        void Delete_Hardware_Textures(void);

        // Delete all Surfaces
        virtual void Delete_All(void);

        virtual bool Delete(size_t array_num, bool delete_data = 1);
        virtual bool Delete(cGL_Surface* obj, bool delete_data = 1);

        // highest opengl texture id found
        GLuint m_high_texture_id;

    private:
        // saved textures for reloading
        Saved_Texture_List m_saved_textures;

        std::unordered_map<std::string, size_t> m_index_table;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

// Image Manager
    extern cImage_Manager* pImage_Manager;

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

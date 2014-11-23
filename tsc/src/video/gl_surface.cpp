/***************************************************************************
 * gl_surface.cpp  -  OpenGL Surface class
 *
 * Copyright © 2005 - 2011 Florian Richter
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

#include "../video/gl_surface.hpp"
#include "../video/video.hpp"
#include "../video/renderer.hpp"
#include "../video/img_manager.hpp"
#include "../objects/sprite.hpp"
#include "../core/property_helper.hpp"

namespace fs = boost::filesystem;

namespace TSC {

/* *** *** *** *** *** *** *** *** cGL_Surface *** *** *** *** *** *** *** *** *** */

cGL_Surface::cGL_Surface(void)
{
    m_image = 0;

    m_int_x = 0;
    m_int_y = 0;
    m_start_w = 0;
    m_start_h = 0;
    m_w = 0;
    m_h = 0;
    m_tex_w = 0;
    m_tex_h = 0;

    // internal rotation data
    m_base_rot_x = 0;
    m_base_rot_y = 0;
    m_base_rot_z = 0;

    // collision data
    m_col_pos.m_x = 0;
    m_col_pos.m_y = 0;
    m_col_w = 0;
    m_col_h = 0;

    m_auto_del_img = 1;
    m_managed = 0;
    m_obsolete = 0;

    // default massive type is passive
    m_massive_type = MASS_PASSIVE;

    m_ground_type = GROUND_NORMAL;

    destruction_function = NULL;
}

cGL_Surface::~cGL_Surface(void)
{
    // don't delete a managed OpenGL image if still in use by another managed cGL_Surface
    if (m_auto_del_img && glIsTexture(m_image) && (!m_managed || !Is_Texture_Use_Multiple())) {
        glDeleteTextures(1, &m_image);
    }

    if (destruction_function) {
        destruction_function(this);
    }
}

cGL_Surface* cGL_Surface::Copy(void) const
{
    // create copy image
    cGL_Surface* new_surface = new cGL_Surface();

    // data
    new_surface->m_image = m_image;
    new_surface->m_int_x = m_int_x;
    new_surface->m_int_y = m_int_y;
    new_surface->m_start_w = m_start_w;
    new_surface->m_start_h = m_start_h;
    new_surface->m_w = m_w;
    new_surface->m_h = m_h;
    new_surface->m_tex_h = m_tex_h;
    new_surface->m_tex_w = m_tex_w;
    new_surface->m_base_rot_x = m_base_rot_x;
    new_surface->m_base_rot_y = m_base_rot_y;
    new_surface->m_base_rot_z = m_base_rot_z;
    new_surface->m_col_pos = m_col_pos;
    new_surface->m_col_w = m_col_w;
    new_surface->m_col_h = m_col_h;
    new_surface->m_path = m_path;

    // settings
    new_surface->m_obsolete = m_obsolete;
    new_surface->m_editor_tags = m_editor_tags;
    new_surface->m_name = m_name;
    new_surface->m_massive_type = m_massive_type;
    new_surface->Set_Ground_Type(m_ground_type);

    return new_surface;
}

void cGL_Surface::Blit(float x, float y, float z, cSurface_Request* request /* = NULL */) const
{
    bool create_request = 0;

    if (!request) {
        create_request = 1;
        // create request
        request = new cSurface_Request();
    }

    Blit_Data(request);

    // position
    request->m_pos_x += x;
    request->m_pos_y += y;
    request->m_pos_z = z;

    if (create_request) {
        // add request
        pRenderer->Add(request);
    }
}

void cGL_Surface::Blit_Data(cSurface_Request* request) const
{
    // texture id
    request->m_texture_id = m_image;

    // position
    request->m_pos_x += m_int_x;
    request->m_pos_y += m_int_y;

    // size
    request->m_w = m_start_w;
    request->m_h = m_start_h;

    // rotation
    request->m_rot_x += m_base_rot_x;
    request->m_rot_y += m_base_rot_y;
    request->m_rot_z += m_base_rot_z;
}

void cGL_Surface::Save(const std::string& filename)
{
    if (!m_image) {
        printf("Couldn't save cGL_Surface : No Image Texture ID set\n");
        return;
    }

    // bind the texture
    glBindTexture(GL_TEXTURE_2D, m_image);

    // create image data
    GLubyte* data = new GLubyte[m_tex_w * m_tex_h * 4];
    // read texture
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, static_cast<GLvoid*>(data));
    // save
    pVideo->Save_Surface(filename, data, m_tex_w, m_tex_h);
    // clear data
    delete[] data;
}

void cGL_Surface::Set_Ground_Type(GroundType gtype)
{
    m_ground_type = gtype;
}

bool cGL_Surface::Is_Texture_Use_Multiple(void) const
{
    for (GL_Surface_List::iterator itr = pImage_Manager->objects.begin(); itr != pImage_Manager->objects.end(); ++itr) {
        cGL_Surface* obj = (*itr);

        if (obj == this) {
            continue;
        }

        if (obj->m_image == m_image) {
            return 1;
        }
    }

    return 0;
}

cSaved_Texture* cGL_Surface::Get_Software_Texture(bool only_filename /* = 0 */)
{
    cSaved_Texture* soft_tex = new cSaved_Texture();

    // hardware texture to software texture
    if (!only_filename) {
        // bind the texture
        glBindTexture(GL_TEXTURE_2D, m_image);

        // texture settings
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &soft_tex->m_width);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &soft_tex->m_height);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &soft_tex->m_format);

        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &soft_tex->m_wrap_s);
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &soft_tex->m_wrap_t);
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &soft_tex->m_min_filter);
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &soft_tex->m_mag_filter);

        unsigned int bpp;

        if (soft_tex->m_format == GL_RGBA) {
            bpp = 4;
        }
        else if (soft_tex->m_format == GL_RGB) {
            bpp = 3;
        }
        else {
            bpp = 4;
            printf("Warning: cGL_Surface::Get_Software_Texture : Unknown format\n");
        }

        // texture data
        soft_tex->m_pixels = new GLubyte[soft_tex->m_width * soft_tex->m_height * bpp];

        glGetTexImage(GL_TEXTURE_2D, 0, soft_tex->m_format, GL_UNSIGNED_BYTE, soft_tex->m_pixels);
    }

    // surface pointer
    soft_tex->m_base = this;

    return soft_tex;
}

void cGL_Surface::Load_Software_Texture(cSaved_Texture* soft_tex)
{
    if (!soft_tex) {
        return;
    }

    // software texture
    if (soft_tex->m_pixels) {
        GLuint tex_id;
        glGenTextures(1, &tex_id);

        glBindTexture(GL_TEXTURE_2D, tex_id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, soft_tex->m_wrap_s);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, soft_tex->m_wrap_t);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, soft_tex->m_min_filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, soft_tex->m_mag_filter);

        // check if mipmaps are enabled
        bool mipmaps = 0;

        // if mipmaps are enabled
        if (soft_tex->m_min_filter == GL_LINEAR_MIPMAP_LINEAR) {
            mipmaps = 1;
        }

        // Create Hardware Texture
        pVideo->Create_GL_Texture(soft_tex->m_width, soft_tex->m_height, soft_tex->m_pixels, mipmaps);

        m_image = tex_id;
    }
    // load from file
    else {
        cGL_Surface* surface_copy = pVideo->Load_GL_Surface(m_path);

        if (!surface_copy) {
            printf("Warning: cGL_Surface::Load_Software_Texture %s loading failed\n", m_path.c_str());
            return;
        }

        // get image
        m_image = surface_copy->m_image;
        m_tex_w = surface_copy->m_tex_w;
        m_tex_h = surface_copy->m_tex_h;
        // keep hardware texture
        surface_copy->m_auto_del_img = 0;
        // delete copy
        delete surface_copy;
    }
}

fs::path cGL_Surface::Get_Path()
{
    return m_path;
}

void cGL_Surface::Set_Destruction_Function(void (*nfunction)(cGL_Surface*))
{
    destruction_function = nfunction;
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

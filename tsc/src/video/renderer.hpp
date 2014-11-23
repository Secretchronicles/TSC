/***************************************************************************
 * renderer.h
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

#ifndef TSC_RENDERER_HPP
#define TSC_RENDERER_HPP

#include "../video/video.hpp"
#include "../core/math/line.hpp"
#include "../core/math/rect.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** RenderType *** *** *** *** *** *** *** *** *** *** */

    enum RenderType {
        REND_NOTHING = 0,
        REND_CLEAR = 1,
        REND_RECT = 2,
        REND_GRADIENT = 3,
        REND_SURFACE = 4,
        REND_TEXT = 5, // todo
        REND_LINE = 6,
        REND_CIRCLE = 7
    };

    /* *** *** *** *** *** *** cRender_Request *** *** *** *** *** *** *** *** *** *** *** */

    class cRender_Request {
    public:
        cRender_Request(void);
        virtual ~cRender_Request(void);

        // draw
        virtual void Draw(void);

        // render type
        RenderType m_type;
        // Z position
        float m_pos_z;
        // times to render until deletion
        int m_render_count;
    };

    typedef vector<cRender_Request*> RenderList;

    /* *** *** *** *** *** *** cClear_Request *** *** *** *** *** *** *** *** *** *** *** */

    class cClear_Request : public cRender_Request {
    public:
        cClear_Request(void);
        virtual ~cClear_Request(void);

        // draw
        virtual void Draw(void);
    };

    /* *** *** *** *** *** *** cRender_Request_Advanced *** *** *** *** *** *** *** *** *** *** *** */

    class cRender_Request_Advanced : public cRender_Request {
    public:
        cRender_Request_Advanced(void);
        virtual ~cRender_Request_Advanced(void);

        // render basic state
        void Render_Basic(void);
        // clear basic render state
        void Render_Basic_Clear(void) const;

        // render advanced state
        void Render_Advanced(void);
        // clear advanced render state
        void Render_Advanced_Clear(void) const;

        // global scale
        bool m_global_scale;
        // if not set camera position is subtracted
        bool m_no_camera;

        // rotation
        float m_rot_x;
        float m_rot_y;
        float m_rot_z;
        // blending
        GLenum m_blend_sfactor;
        GLenum m_blend_dfactor;
        // shadow position
        float m_shadow_pos;
        // shadow color
        Color m_shadow_color;

        // combine type
        GLint m_combine_type;
        // combine color
        float m_combine_color[3];
    };

    /* *** *** *** *** *** *** cLine_Request *** *** *** *** *** *** *** *** *** *** *** */

    class cLine_Request : public cRender_Request_Advanced {
    public:
        cLine_Request(void);
        virtual ~cLine_Request(void);

        // draw
        virtual void Draw(void);

        // color
        Color m_color;
        // position
        GL_line m_line;
        // width
        float m_line_width;
        // stipple pattern
        GLushort m_stipple_pattern;
    };

    /* *** *** *** *** *** *** cRect_Request *** *** *** *** *** *** *** *** *** *** *** */

    class cRect_Request : public cRender_Request_Advanced {
    public:
        cRect_Request(void);
        virtual ~cRect_Request(void);

        // draw
        virtual void Draw(void);
        // color
        Color m_color;
        // rect
        GL_rect m_rect;
        // rect is filled
        bool m_filled;
        // scale
        float m_scale_x;
        float m_scale_y;
        float m_scale_z;
        // line width (only used if not filled)
        float m_line_width;
        // stipple pattern (only used if not filled)
        GLushort m_stipple_pattern;
    };

    /* *** *** *** *** *** *** cGradient_Request *** *** *** *** *** *** *** *** *** *** *** */

    class cGradient_Request : public cRender_Request_Advanced {
    public:
        cGradient_Request(void);
        virtual ~cGradient_Request(void);

        // draw
        virtual void Draw(void);

        // rect
        GL_rect m_rect;
        // direction
        ObjectDirection m_dir;
        // colors
        Color m_color_1;
        Color m_color_2;
    };

    /* *** *** *** *** *** *** cCircle_Request *** *** *** *** *** *** *** *** *** *** *** */

    class cCircle_Request : public cRender_Request_Advanced {
    public:
        cCircle_Request(void);
        virtual ~cCircle_Request(void);

        // draw
        virtual void Draw(void);
        // color
        Color m_color;
        // position
        GL_point m_pos;
        // radius
        float m_radius;
        // if set circle is not filled
        float m_line_width;
    };

    /* *** *** *** *** *** *** cSurface_Request *** *** *** *** *** *** *** *** *** *** *** */

    class cSurface_Request : public cRender_Request_Advanced {
    public:
        cSurface_Request(void);
        virtual ~cSurface_Request(void);

        // Draw
        virtual void Draw(void);

        // texture id
        GLuint m_texture_id;
        // position
        float m_pos_x;
        float m_pos_y;
        // scale
        float m_scale_x;
        float m_scale_y;
        float m_scale_z;
        // size
        float m_w;
        float m_h;

        // color
        Color m_color;

        // delete texture after request finished
        bool m_delete_texture;
    };

    /* *** *** *** *** *** *** cRenderQueue *** *** *** *** *** *** *** *** *** *** *** */

    class cRenderQueue {
    public:
        cRenderQueue(unsigned int reserve_items);
        ~cRenderQueue(void);

        /* Add a Render Request
        */
        void Add(cRender_Request* obj);

        /* Render current data
         * clear: if set clear the finished data after rendering
        */
        void Render(bool clear = 1);

        /* Reduce the render count
         * amount : render count decrease amount
         * clear: if set clear the finished data after rendering
        */
        void Fake_Render(unsigned int amount = 1, bool clear = 1);

        /* clear the render data
         * if force is given all objects will be removed
        */
        void Clear(bool force = 1);

        // render data array
        RenderList m_render_data;

        // Z position sort
        struct zpos_sort {
            bool operator()(const cRender_Request* a, const cRender_Request* b) const
            {
                return a->m_pos_z < b->m_pos_z;
            }
        };
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

// Renderer class
    extern cRenderQueue* pRenderer;
    extern cRenderQueue* pRenderer_current;

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

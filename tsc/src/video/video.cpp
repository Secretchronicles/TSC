/***************************************************************************
 * video.cpp  -  General video functions
 *
 * Copyright © 2005 - 2011 Florian Richter
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

#include "../core/global_basic.hpp"
#include "loading_screen.hpp"
#include "../user/preferences.hpp"
#include "../core/framerate.hpp"
#include "../core/game_core.hpp"
#include "img_settings.hpp"
#include "img_manager.hpp"
#include "../input/mouse.hpp"
#include "../input/joystick.hpp"
#include "../video/renderer.hpp"
#include "../core/main.hpp"
#include "../core/math/utilities.hpp"
#include "../core/i18n.hpp"
#include "../core/math/size.hpp"
#include "../core/filesystem/filesystem.hpp"
#include "../core/filesystem/resource_manager.hpp"
#include "../core/filesystem/relative.hpp"
#include "../gui/hud.hpp"
#include "video.hpp"

using namespace std;

namespace fs = boost::filesystem;

namespace TSC {

/* *** *** *** *** *** *** *** Video class *** *** *** *** *** *** *** *** *** *** */

cVideo::cVideo(void)
{
    mp_window = new sf::RenderWindow();
    m_opengl_version = 0;

    m_double_buffer = 0;

    m_rgb_size[0] = 0;
    m_rgb_size[1] = 0;
    m_rgb_size[2] = 0;

    m_default_buffer = GL_BACK;
    m_max_texture_size = 512;

    m_audio_init_failed = 0;
    m_joy_init_failed = 0;
    m_geometry_quality = cPreferences::m_geometry_quality_default;
    m_texture_quality = cPreferences::m_texture_quality_default;

#ifdef __unix__
    glx_context = NULL;
#endif
    m_render_thread = boost::thread();

    mp_cegui_renderer = NULL;
    mp_default_tooltip = NULL;

    m_initialised = 0;
}

cVideo::~cVideo(void)
{
    if (mp_default_tooltip) {
        CEGUI::WindowManager::getSingleton().destroyWindow(mp_default_tooltip);
        CEGUI::System::getSingleton().getDefaultGUIContext().setDefaultTooltipObject(0);
        mp_default_tooltip = NULL;
    }

    CEGUI::System::destroy();
    CEGUI::OpenGLRenderer::destroy(*mp_cegui_renderer);
    mp_cegui_renderer = NULL;

    delete mp_cegui_xmlparser;
    mp_cegui_xmlparser = NULL;

    delete mp_cegui_imgcodec;
    mp_cegui_imgcodec = NULL;

    if (mp_window) {
        delete mp_window;
        mp_window = NULL;
    }
}

void cVideo::Init_CEGUI(void)
{
    std::string utf8_logpath = path_to_utf8(pResource_Manager->Get_User_CEGUI_Logfile());
    debug_print("CEGUI log file is at '%s'.\n", utf8_logpath.c_str());

    // create CEGUI renderer and system objects
    mp_cegui_renderer = &CEGUI::OpenGLRenderer::create();
#ifdef CEGUI_USE_EXPAT
    mp_cegui_xmlparser = new CEGUI::ExpatParser();
#else
    mp_cegui_xmlparser = new CEGUI::LibxmlParser();
#endif
    mp_cegui_imgcodec = new CEGUI::DevILImageCodec();
    CEGUI::System::create(*mp_cegui_renderer, NULL, mp_cegui_xmlparser, mp_cegui_imgcodec, NULL, "", utf8_logpath);

    // Retrieve default resource provider for the OpenGLRenderer
    CEGUI::DefaultResourceProvider* p_rp
        = static_cast<CEGUI::DefaultResourceProvider*>(CEGUI::System::getSingleton().getResourceProvider());

    // set Resource Provider directories
    p_rp->setResourceGroupDirectory("schemes", path_to_utf8(pResource_Manager->Get_Gui_Scheme_Directory()));
    p_rp->setResourceGroupDirectory("imagesets", path_to_utf8(pResource_Manager->Get_Gui_Imageset_Directory()));
    p_rp->setResourceGroupDirectory("fonts", path_to_utf8(pResource_Manager->Get_Gui_Font_Directory()));
    p_rp->setResourceGroupDirectory("looknfeels", path_to_utf8(pResource_Manager->Get_Gui_LookNFeel_Directory()));
    p_rp->setResourceGroupDirectory("layouts", path_to_utf8(pResource_Manager->Get_Gui_Layout_Directory()));
    p_rp->setResourceGroupDirectory("xmlschemas", path_to_utf8(pResource_Manager->Get_Gui_XmlSchema_Directory()));
    // These resource groups are used for displaying the editor images.
    p_rp->setResourceGroupDirectory("ingame-images", path_to_utf8(pResource_Manager->Get_Game_Pixmaps_Directory()));
    p_rp->setResourceGroupDirectory("cache-images", path_to_utf8(pResource_Manager->Get_User_Pixmaps_Directory()));
    // This group is for using the background images in the editor's background tab's preview
    p_rp->setResourceGroupDirectory("backgrounds", path_to_utf8(pResource_Manager->Get_Game_Pixmaps_Directory() / "game" / "background"));

    // set the default resource groups to be used
    CEGUI::Scheme::setDefaultResourceGroup("schemes");
    CEGUI::ImageManager::setImagesetDefaultResourceGroup("imagesets");
    CEGUI::Font::setDefaultResourceGroup("fonts");
    CEGUI::WidgetLookManager::setDefaultResourceGroup("looknfeels");
    CEGUI::WindowManager::setDefaultResourceGroup("layouts");

    // For those XML parsers CEGUI wants to to XML validation with
    CEGUI::XMLParser* p_parser = CEGUI::System::getSingleton().getXMLParser();
    if (p_parser->isPropertyPresent("SchemaDefaultResourceGroup")) {
        p_parser->setProperty("SchemaDefaultResourceGroup", "xmlschemas");
    }

    // Load our CEGUI theme
    CEGUI::SchemeManager::getSingleton().createFromFile("TSCLook256.scheme");

    // Have CEGUI draw a mouse cursor
    CEGUI::GUIContext& gui_context = CEGUI::System::getSingleton().getDefaultGUIContext();
    gui_context.getMouseCursor().setDefaultImage("TSCLook256/MouseArrow");

    // set initial mouse position
    sf::Vector2i mousepos = sf::Mouse::getPosition(*pVideo->mp_window);
    CEGUI::MouseCursor::setInitialMousePosition(CEGUI::Vector2f(mousepos.x, mousepos.y));

    // Create the invisible root window
    CEGUI::Window* p_rootwindow = CEGUI::WindowManager::getSingleton().createWindow("DefaultWindow", "root");
    p_rootwindow->setMousePassThroughEnabled(true); // Ensure CEGUI doesn't eat mouse events not hitting any CEGUI window (return value of injector function)
    gui_context.setRootWindow(p_rootwindow);

    // Create default tooltip widget (see http://static.cegui.org.uk/docs/0.8.7/classCEGUI_1_1Tooltip.html#details),
    // where CEGUI::System::setTooltip() has been renamed to CEGUI::GUIContext::setDefaultTooltipObject()
    // (documentation bug).
    mp_default_tooltip = static_cast<CEGUI::Tooltip*>(CEGUI::WindowManager::getSingleton().createWindow("TSCLook256/Tooltip", "default_tooltip"));
    gui_context.setDefaultTooltipObject(mp_default_tooltip);
}

void cVideo::Init_Video(bool reload_textures_from_file /* = false */, bool use_preferences /* = true */)
{
    Render_Finish();

    sf::VideoMode videomode(800, 600, 16); // defaults
    sf::VideoMode desktopmode(sf::VideoMode::getDesktopMode());
    if (use_preferences) {
        videomode.width        = pPreferences->m_video_screen_w;
        videomode.height       = pPreferences->m_video_screen_h;
        videomode.bitsPerPixel = pPreferences->m_video_screen_bpp;

        // Emulate old SDL behaviour for user preferences of value 0
        if (videomode.width == 0)
            videomode.width = desktopmode.width;
        if (videomode.height == 0)
            videomode.height = desktopmode.height;
    }

    // test screen mode
    if (!videomode.isValid()) {
        cerr << "Warning : Video Resolution " << videomode.width << "x" << videomode.height << " is not supported" << endl;
        cerr << "Falling back to lowest available settings." << endl;

        // set lowest available settings
        videomode.width = 640;
        videomode.height = 480;
        videomode.bitsPerPixel = 16;

        // overwrite user settings
        if (use_preferences) {
            pPreferences->m_video_screen_w = videomode.width;
            pPreferences->m_video_screen_h = videomode.height;
        }
    }

    uint32_t style;

    if (pPreferences->m_video_fullscreen)
        style = sf::Style::Fullscreen;
    else
        style = sf::Style::Default;

    mp_window->create(videomode, CAPTION, style);
    mp_window->setMouseCursorVisible(false);

    if (use_preferences && pPreferences->m_video_vsync) {
        mp_window->setVerticalSyncEnabled(true);
    }

    // TODO: Icon? Icon is available as pResource_Manager->Get_Game_Icon("window_icon.png");

    // if reinitialization
    if (m_initialised) {
        Loading_Screen_Init();

        // save textures
        pImage_Manager->Grab_Textures(reload_textures_from_file, 1);
        mp_cegui_renderer->grabTextures();
        pImage_Manager->Delete_Hardware_Textures();

        // exit loading screen
        Loading_Screen_Exit();
    }

    // For backward compatibility with old SDL. SFML is always
    // double-buffered.
    m_double_buffer = true;

    // remember default buffer
    glGetIntegerv(GL_DRAW_BUFFER, &m_default_buffer);
    // get maximum texture size
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m_max_texture_size);

#ifdef __unix__
    // get context
    glx_context = glXGetCurrentContext();
#endif

    // initialize opengl
    Init_OpenGL();

    // if reinitialization
    if (m_initialised) {
        // reset highest texture id
        pImage_Manager->m_high_texture_id = 0;

        /* restore GUI textures
         * must be the first CEGUI call after the grabTextures function
        */
        mp_cegui_renderer->restoreTextures();

        // send new size to CEGUI
        CEGUI::System::getSingleton().notifyDisplaySizeChanged(CEGUI::Sizef(static_cast<float>(videomode.width), static_cast<float>(videomode.height)));

        Loading_Screen_Init();

        // initialize new image cache
        if (reload_textures_from_file) {
            Init_Image_Cache(0);
        }

        // restore textures
        pImage_Manager->Restore_Textures(1);

        // Tell the HUD about the size change so it can adapt
        gp_hud->Screen_Size_Changed();

        Loading_Screen_Exit();
    }
    // finishing first initialization
    else {
        // get opengl version
        std::string version_str = reinterpret_cast<const char*>(glGetString(GL_VERSION));
        // erase everything after X.X
        version_str.erase(3);

        m_opengl_version = string_to_float(version_str);

        // if below optimal version
        if (m_opengl_version < 1.4f) {
            if (m_opengl_version >= 1.3f) {
                cout << "Info : OpenGL Version " << fixed << setprecision(1) << m_opengl_version << " is below the optimal version 1.4 and higher" << endl;

                //Restore cout format settings (so that we don't change them for the entire game)
                cout.unsetf(ios::fixed);
                cout << setprecision(6); //6 is the default in C++
            }
            else {
                cerr << "Warning : OpenGL Version " << fixed << setprecision(1) << m_opengl_version << " is below version 1.3" << endl;

                //Restore cerr format settings
                cerr.unsetf(ios::fixed);
                cerr << setprecision(6);
            }

        }

        // Init CEGUI
        Init_CEGUI();

        // On game startup, Loading_Screen_Init() etc. are called from
        // the Init_Game() function. Only on recreation (see above) they're
        // called from here.
        // TODO: That's bad code design.

        m_initialised = 1;
    }
}

void cVideo::Init_OpenGL(void)
{
    // viewport should cover the whole screen
    glViewport(0, 0, pPreferences->m_video_screen_w, pPreferences->m_video_screen_h);

    // select the projection matrix
    glMatrixMode(GL_PROJECTION);
    // clear it
    glLoadIdentity();
    // Set up the orthographic projection matrix
    glOrtho(0, static_cast<float>(pPreferences->m_video_screen_w), static_cast<float>(pPreferences->m_video_screen_h), 0, -1, 1);

    // select the orthographic projection matrix
    glMatrixMode(GL_MODELVIEW);
    // clear it
    glLoadIdentity();

    // set the smooth shading model
    glShadeModel(GL_SMOOTH);

    // set clear color to black
    glClearColor(0, 0, 0, 1);

    // Z-Buffer
    glEnable(GL_DEPTH_TEST);

    // Depth function
    glDepthFunc(GL_LEQUAL);
    // Depth Buffer Setup
    glClearDepth(1);

    // Blending
    glEnable(GL_BLEND);
    // Blending function
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Alpha
    glEnable(GL_ALPHA_TEST);
    // Alpha function
    glAlphaFunc(GL_GREATER, 0.01f);

    // Geometry
    Init_Geometry();
    // texture detail
    Init_Texture_Detail();
    // Resolution Scale
    Init_Resolution_Scale();

    // clear screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Swap buffers
    mp_window->display();
}

void cVideo::Init_Geometry(void)
{
    Render_Finish();

    // Geometry Anti-Aliasing
    if (m_geometry_quality > 0.5f) {
        // Point
        glEnable(GL_POINT_SMOOTH);
        glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
        // Line
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        // Polygon - does not display correctly with open source ATi drivers ( 18.2.2008 )
        //glEnable( GL_POLYGON_SMOOTH );
        // Geforce 4 440 MX hangs if enabled
        //glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );
    }
    else {
        // Point
        glEnable(GL_POINT_SMOOTH);
        glHint(GL_POINT_SMOOTH_HINT, GL_FASTEST);
        // Line
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);
    }

    /* Perspective Correction
     * The quality of color, texture coordinate, and fog coordinate interpolation
    */
    if (m_geometry_quality > 0.25f) {
        // high quality
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    }
    else {
        // low quality
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    }
}

void cVideo::Init_Texture_Detail(void)
{
    Render_Finish();

    /* filter quality of generated mipmap images
     * only available if OpenGL version is 1.4 or greater
    */
    if (m_opengl_version >= 1.4f) {
        if (m_texture_quality > 0.2f) {
            glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
        }
        else {
            glHint(GL_GENERATE_MIPMAP_HINT, GL_FASTEST);
        }
    }
}

void cVideo::Init_Resolution_Scale(void) const
{
    // up scale
    global_upscalex = static_cast<float>(pPreferences->m_video_screen_w) / static_cast<float>(game_res_w);
    global_upscaley = static_cast<float>(pPreferences->m_video_screen_h) / static_cast<float>(game_res_h);
    // down scale
    global_downscalex = static_cast<float>(game_res_w) / static_cast<float>(pPreferences->m_video_screen_w);
    global_downscaley = static_cast<float>(game_res_h) / static_cast<float>(pPreferences->m_video_screen_h);
}

/**
 * Create the cache of downscaled images. This function
 * expects to be run while the loading screen is active,
 * i.e. the caller must make sure to have called Loading_Screen_Init()
 * prior to this function and has to call Loading_Screen_Exit() when
 * he is done with this function (and everything else he wants to
 * do while the loading screen is active).
 *
 * \param recreate
 * If this is true (it's false by default), recreate the image cache even
 * if it already exists.
 */
void cVideo::Init_Image_Cache(bool recreate /* = 0 */)
{
    m_imgcache_dir = pResource_Manager->Get_User_Imgcache_Directory();
    fs::path imgcache_dir_active = m_imgcache_dir / utf8_to_path(int_to_string(pPreferences->m_video_screen_w) + "x" + int_to_string(pPreferences->m_video_screen_h));

    // if cache is disabled
    if (!pPreferences->m_image_cache_enabled) {
        return;
    }

    // if not the same game version
    if (recreate || pPreferences->m_game_version != tsc_version) {
        // delete all caches
        if (Dir_Exists(m_imgcache_dir)) {
            try {
                fs::remove_all(m_imgcache_dir);
            }
            // could happen if a file is locked or we have no write rights
            catch (const std::exception& ex) {
                cerr << ex.what() << endl;

                Loading_Screen_Draw_Text(_("Caching Images failed : Could not remove old images"));
                //sleep(2);
            }
        }

        fs::create_directories(m_imgcache_dir);
    }

    // no cache available
    if (!Dir_Exists(imgcache_dir_active)) {
        fs::create_directories(imgcache_dir_active / utf8_to_path(GAME_PIXMAPS_DIR));
    }
    // cache available
    else {
        m_imgcache_dir = imgcache_dir_active;
        return;
    }

    // texture detail should be maximum for caching
    float real_texture_detail = m_texture_quality;
    m_texture_quality = 1;

    // set loading screen text
    Loading_Screen_Draw_Text(_("Caching Images"));

    // get all files
    vector<fs::path> image_files = Get_Directory_Files(pResource_Manager->Get_Game_Pixmaps_Directory(), ".settings", true);

    unsigned int loaded_files = 0;
    unsigned int file_count = image_files.size();

    // create directories, load images and save to cache
    for (vector<fs::path>::iterator itr = image_files.begin(); itr != image_files.end(); ++itr) {
        // get filenames
        fs::path filename = (*itr);
        fs::path cache_filename = imgcache_dir_active / fs_relative(pResource_Manager->Get_Game_Data_Directory(), filename);

        // if directory
        if (fs::is_directory(filename)) {
            if (!fs::is_directory(cache_filename)) {
                fs::create_directory(cache_filename);
            }

            loaded_files++;
            continue;
        }

        bool settings_file = false;

        // Don't use .settings file type directly for image loading
        if (filename.extension() == fs::path(".settings")) {
            settings_file = true;
            filename.replace_extension(".png");
        }

        // load software image
        cSoftware_Image software_image = Load_Image(filename);
        sf::Image* p_sf_image = software_image.m_sf_image;
        cImage_Settings_Data* settings = software_image.m_settings;

        // failed to load image
        if (!p_sf_image) {
            continue;
        }

        /* don't cache if no image settings or images without the width and height set
         * as there is currently no support to get the old and real image size
         * and thus the scaled down (cached) image size is used which is wrong
        */
        if (!settings || !settings->m_width || !settings->m_height) {
            if (settings) {
                debug_print("Info : %s has no image settings image size set and will not get cached\n", cache_filename.c_str());
            }
            else {
                debug_print("Info : %s has no image settings and will not get cached\n", cache_filename.c_str());
            }
            delete p_sf_image;
            continue;
        }

        // create final image
        p_sf_image = Convert_To_Final_Software_Image(p_sf_image);

        // get final size for this resolution
        cSize_Int size = settings->Get_Surface_Size(p_sf_image);
        delete settings;
        int new_width = size.m_width;
        int new_height = size.m_height;

        // apply maximum texture size
        Apply_Max_Texture_Size(new_width, new_height);

        // does not need to be downsampled
        if (new_width >= p_sf_image->getSize().x && new_height >= p_sf_image->getSize().y) {
            delete p_sf_image;
            p_sf_image = NULL;
            continue;
        }

        // calculate block reduction
        int reduce_block_x = p_sf_image->getSize().x / new_width;
        int reduce_block_y = p_sf_image->getSize().y / new_height;

        // create downsampled image
        /* Old SDL TSC queried SDL for a "bytes per pixels" value, see
         * <https://wiki.libsdl.org/SDL_PixelFormat>.  This is simply
         * the number of bytes required to store all info about one
         * pixel.  It can easily be calculated without SDL: If yor
         * image has a depth of 8 *bits* per colour, then a pixel
         * consists of 3x8 = 24 bits (RGB) or 4x8 = 32 bits
         * (RGBA). For 24 bits you need 3 bytes to store, for 32 bits
         * 4 bytes. SFML guarantees in the documentation of
         * sf::Image::getPixelPtr() that RGBA data is returned with a
         * colour depth of 8 bit (resulting in 32 bits per pixel as
         * per the above). If SFML ever supports other colour depths,
         * the required bytes-per-pixel storage value can easily be
         * calculated with:
         *   ceil(bits-per-pixel * 4 / 8.0)
         * Where 4
         * stands for RGBA. For plain RGB you'd need to insert 3
         * instead. For now, relying on SFML's docs, we just hardcode
         * 4 bytes as that is what SFML returns to us. */
        unsigned int image_bpp = 4; // 8 bits-per-color x 4 colors (RGBA) = 32 bits. 32 bits / 8 bits = 4 bytes.
        unsigned char* image_downsampled = new unsigned char[new_width * new_height * image_bpp];
        bool downsampled = Downscale_Image(static_cast<const unsigned char*>(p_sf_image->getPixelsPtr()), p_sf_image->getSize().x, p_sf_image->getSize().y, image_bpp, image_downsampled, reduce_block_x, reduce_block_y);

        delete p_sf_image;

        // if image is available
        if (downsampled) {
            // save as png
            if (settings_file) {
                cache_filename.replace_extension(".png");
            }

            // save image
            Save_Surface(cache_filename, image_downsampled, new_width, new_height, image_bpp);
        }

        delete[] image_downsampled;

        // count files
        loaded_files++;

        // draw
        // update progress
        Loading_Screen_Set_Progress(static_cast<float>(loaded_files) / static_cast<float>(file_count));

// OLD #ifdef _DEBUG
// OLD             // update filename
// OLD             cGL_Surface* surface_filename = pFont->Render_Text(pFont->m_font_small, path_to_utf8(filename), white);
// OLD             // draw filename
// OLD             surface_filename->Blit(game_res_w * 0.2f, game_res_h * 0.8f, 0.1f);
// OLD #endif
            Loading_Screen_Draw();
// OLD #ifdef _DEBUG
// OLD             // delete
// OLD             delete surface_filename;
// OLD #endif
    }

    // set back texture detail
    m_texture_quality = real_texture_detail;
    // set directory after surfaces got loaded from Load_GL_Surface()
    m_imgcache_dir = imgcache_dir_active;
}

int cVideo::Test_Video(int width, int height, int bpp, int flags /* = 0 */) const
{
    return sf::VideoMode(width, height, bpp).isValid();
}

vector<cSize_Int> cVideo::Get_Supported_Resolutions(int flags /* = 0 */) const
{
    vector<cSize_Int> valid_resolutions;

    const std::vector<sf::VideoMode>& valid_modes = sf::VideoMode::getFullscreenModes();

    if (valid_modes.empty()) {
        valid_resolutions.push_back(cSize_Int(2048, 1536));
        valid_resolutions.push_back(cSize_Int(1600, 1200));
        valid_resolutions.push_back(cSize_Int(1280, 1024));
        valid_resolutions.push_back(cSize_Int(1024, 768));
        valid_resolutions.push_back(cSize_Int(800, 600));
        valid_resolutions.push_back(cSize_Int(640, 480));
    }
    else {
        std::vector<sf::VideoMode>::const_iterator iter;
        for(iter=valid_modes.begin(); iter != valid_modes.end(); iter++) {
            valid_resolutions.push_back(cSize_Int((*iter).width, (*iter).height));
        }
    }

    return valid_resolutions;
}

void cVideo::Make_GL_Context_Current(void)
{
// Not ported to SFML yet, because unused function
// Part of the unfinished separate rendering thread implementation in TSC
// (TSC_RENDER_THREAD_TEST macro).
    throw(std::runtime_error("Make_GL_Context_Current() not implemented"));

// scoped context lock here
//#ifdef _WIN32
//    if (wglGetCurrentContext() != wm_info.hglrc) {
//        wglMakeCurrent(GetDC(wm_info.window), wm_info.hglrc);
//    }
//#elif __unix__
//    if (glx_context != NULL) {
//        glXMakeCurrent(wm_info.info.x11.gfxdisplay, wm_info.info.x11.window, glx_context);
//    }
//#elif __APPLE__
//    // party time
//#endif
}

void cVideo::Make_GL_Context_Inactive(void)
{
#ifdef _WIN32
    wglMakeCurrent(NULL, NULL);
#elif __unix__
    // OLD glXMakeCurrent(wm_info.info.x11.gfxdisplay, None, NULL);
#elif __APPLE__
    // party time
#endif
}

void cVideo::Render_From_Thread(void)
{
    Make_GL_Context_Current();

    pRenderer_current->Render();
    // under linux with sofware mesa 7.9 it only showed the rendered output with SDL_GL_SwapBuffers()

    // update performance timer
    //pFramerate->m_perf_timer[PERF_RENDER_GAME]->Update();

    Make_GL_Context_Inactive();
}

void cVideo::Render(bool threaded /* = 0 */)
{
    Render_Finish();

    if (threaded) {
        CEGUI::System::getSingleton().renderAllGUIContexts();

        // update performance timer
        pFramerate->m_perf_timer[PERF_RENDER_GUI]->Update();

        mp_window->display();

        // update performance timer
        pFramerate->m_perf_timer[PERF_RENDER_BUFFER]->Update();

        // switch active renderer
        cRenderQueue* new_render = pRenderer;
        pRenderer = pRenderer_current;
        pRenderer_current = new_render;

        // move objects that should render more than once
        if (!pRenderer->m_render_data.empty()) {
            pRenderer_current->m_render_data.insert(pRenderer_current->m_render_data.begin(), pRenderer->m_render_data.begin(), pRenderer->m_render_data.end());
            pRenderer->m_render_data.clear();
        }

        // make main thread inactive
        Make_GL_Context_Inactive();
        // start render thread
        m_render_thread = boost::thread(&cVideo::Render_From_Thread, this);
    }
    // single thread mode
    else {
        pRenderer->Render();

        // update performance timer
        pFramerate->m_perf_timer[PERF_RENDER_GAME]->Update();

        // Render GUI after everything else, i.e. on top of everything
        CEGUI::System::getSingleton().renderAllGUIContexts();

        // update performance timer
        pFramerate->m_perf_timer[PERF_RENDER_GUI]->Update();

        mp_window->display();

        // update performance timer
        pFramerate->m_perf_timer[PERF_RENDER_BUFFER]->Update();
    }
}

void cVideo::Render_Finish(void)
{
#ifndef TSC_RENDER_THREAD_TEST
    return;
#endif
    if (m_render_thread.joinable()) {
        m_render_thread.join();
    }

    // todo : use opengl in only one thread
    Make_GL_Context_Current();
}

void cVideo::Toggle_Fullscreen(void)
{
    Render_Finish();

    // toggle fullscreen
    pPreferences->m_video_fullscreen = !pPreferences->m_video_fullscreen;

    // save clear color
    GLclampf clear_color[4];
    glGetFloatv(GL_COLOR_CLEAR_VALUE, clear_color);

    // Video must be reinitialized
    Init_Video();

    // set back clear color
    glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
}

cGL_Surface* cVideo::Get_Surface(fs::path filename, bool print_errors /* = true */)
{
    // .settings file type can't be used directly
    if (filename.extension() == fs::path(".settings"))
        filename.replace_extension(".png");

    // pixmaps dir must be given
    if (!filename.is_absolute()) {
        filename = pResource_Manager->Get_Game_Pixmaps_Directory() / filename;
    }

    // check if already loaded
    cGL_Surface* image = pImage_Manager->Get_Pointer(path_to_utf8(filename));
    // already loaded
    if (image) {
        return image;
    }

    // load new image
    image = Load_GL_Surface(path_to_utf8(filename), 1, print_errors);
    // add new image
    if (image) {
        pImage_Manager->Add(image);
    }

    return image;
}

cVideo::cSoftware_Image cVideo::Load_Image(boost::filesystem::path filename, bool load_settings /* = 1 */, bool print_errors /* = 1 */) const
{
    // pixmaps dir must be given
    if (!filename.is_absolute()) {
        filename = fs::absolute(filename, pResource_Manager->Get_Game_Pixmaps_Directory());
    }

    cSoftware_Image software_image = cSoftware_Image();
    sf::Image* p_sf_image = new sf::Image();
    bool successfully_loaded = false;
    cImage_Settings_Data* settings = NULL;
    fs::path final_png_path;

    // load settings if available
    if (load_settings) {
        fs::path settings_file = fs::path(filename);

        // If not already set
        if (settings_file.extension() != fs::path(".settings"))
            settings_file.replace_extension(".settings");

        if (fs::exists(settings_file) && fs::is_regular_file(settings_file)) {
            settings = pSettingsParser->Get(settings_file);

            // add cache dir and remove data dir
            fs::path img_filename_cache = m_imgcache_dir / fs_relative(pResource_Manager->Get_Game_Data_Directory(), filename);

            // check if image cache file exists
            if (fs::exists(img_filename_cache) && fs::is_regular_file(img_filename_cache)) {
                successfully_loaded = p_sf_image->loadFromFile(path_to_utf8(img_filename_cache));

                if (successfully_loaded) {
                    final_png_path = img_filename_cache;
                }
            }
            // image given in base settings
            else if (!settings->m_base.empty()) {
                // use current directory
                fs::path img_filename = filename.parent_path() / settings->m_base;

                if (!exists(img_filename)) {
                    // use data dir
                    img_filename = settings->m_base;

                    // pixmaps dir must be given
                    if (!img_filename.is_absolute()) {
                        img_filename = fs::absolute(img_filename, pResource_Manager->Get_Game_Pixmaps_Directory());
                    }
                }

                successfully_loaded = p_sf_image->loadFromFile(path_to_utf8(img_filename));

                if (successfully_loaded) {
                    final_png_path = img_filename;
                }
            }
        }
    }

    // if not set in image settings and file exists
    if (!successfully_loaded && exists(filename) && (!settings || settings->m_base.empty())) {
        successfully_loaded = p_sf_image->loadFromFile(path_to_utf8(filename));

        if (successfully_loaded) {
            final_png_path = filename;
        }
    }

    if (!successfully_loaded) {
        if (settings) {
            delete settings;
            settings = NULL;
        }

        if (print_errors) {
            cerr << "Error loading image : " << path_to_utf8(filename) << endl << endl;
        }

        delete p_sf_image;
        return software_image;
    }

    software_image.m_sf_image = p_sf_image;
    software_image.m_settings = settings;
    software_image.m_real_png_path = final_png_path;
    return software_image;
}

cGL_Surface* cVideo::Load_GL_Surface(boost::filesystem::path filename, bool use_settings /* = 1 */, bool print_errors /* = 1 */)
{
    // pixmaps dir must be given
    if (!filename.is_absolute()) {
        filename = fs::absolute(filename, pResource_Manager->Get_Game_Pixmaps_Directory());
    }

    // load software image
    cSoftware_Image software_image = Load_Image(filename, use_settings, print_errors);
    sf::Image* p_sf_image = software_image.m_sf_image;
    cImage_Settings_Data* settings = software_image.m_settings;

    // final surface
    cGL_Surface* image = NULL;

    // with settings
    if (settings) {
        // get the size
        cSize_Int size = settings->Get_Surface_Size(p_sf_image);
        Apply_Max_Texture_Size(size.m_width, size.m_height);
        // get basic settings surface
        image = pVideo->Create_Texture(p_sf_image, settings->m_mipmap, size.m_width, size.m_height);
        // apply settings
        settings->Apply(image);
        delete settings;
    }
    // without settings
    else {
        image = Create_Texture(p_sf_image);
    }
    // set filenames
    if (image) {
        image->m_path = filename;
        image->m_real_png_path = software_image.m_real_png_path;
    }
    // print error
    else if (print_errors) {
        cerr << "Error loading GL surface image" << endl;
    }

    return image;
}

/**
 * OpenGL only understands textures whose edges each have a length
 * that is a power of 2. This function ensures that our images fulfill
 * this requirement; if they don’t already have such edges, `p_sf_image'
 * is expanded to a size that fits the power-of-2 rule; the newly created
 * pixels are set to transparency.
 */
sf::Image* cVideo::Convert_To_Final_Software_Image(sf::Image* p_sf_image) const
{
    // get power of two size
    sf::Vector2u cursize = p_sf_image->getSize();
    const unsigned int width = Get_Power_of_2(cursize.x);
    const unsigned int height = Get_Power_of_2(cursize.y);

    // if it needs to be changed
    if (width != cursize.x || height != cursize.y) {
        // create power of 2 surface
        sf::Image* new_image = new sf::Image();
        new_image->create(width, height, sf::Color::Transparent);
        // copy over the old image into the new one (i.e., blit it onto it)
        new_image->copy(*p_sf_image, 0, 0);
        // delete original surface
        delete p_sf_image;
        // set new surface
        p_sf_image = new_image;
    }

    return p_sf_image;
}

cGL_Surface* cVideo::Create_Texture(sf::Image* p_sf_image, bool mipmap /* = 0 */, unsigned int force_width /* = 0 */, unsigned int force_height /* = 0 */) const
{
    if (!p_sf_image) {
        return NULL;
    }

    // create final image
    p_sf_image = Convert_To_Final_Software_Image(p_sf_image);

    /* todo : Make this a render request because it forces an early thread render finish as opengl commands are used directly.
     * Reduces performance if the render thread is on. It's usually called from the text rendering in cTimeDisplay::Update.
    */
    pVideo->Render_Finish();

    // create one texture
    GLuint image_num = 0;
    glGenTextures(1, &image_num);

    // if image id is 0 it failed
    if (!image_num) {
        cerr << "Error : GL image generation failed" << endl;
        delete p_sf_image;
        return NULL;
    }

    // set highest texture id
    if (pImage_Manager->m_high_texture_id < image_num) {
        pImage_Manager->m_high_texture_id = image_num;
    }

    int width = p_sf_image->getSize().x;
    int height = p_sf_image->getSize().y;

    // forced size is set
    if (force_width > 0 && force_height > 0) {
        // get power of two size
        force_width = Get_Power_of_2(force_width);
        force_height = Get_Power_of_2(force_height);

        // apply forced size
        if (force_width != width || force_height != height) {
            width = force_width;
            height = force_height;
        }
    }

    // texture size
    int texture_width = width;
    int texture_height = height;
    // check if the image size is greater than the maximum texture size
    Apply_Max_Texture_Size(texture_width, texture_height);

    // scale to new size
    if (texture_width != p_sf_image->getSize().x || texture_height != p_sf_image->getSize().y) {
        int reduce_block_x = p_sf_image->getSize().x / texture_width;
        int reduce_block_y = p_sf_image->getSize().y / texture_height;

        // create scaled image
        unsigned char* new_pixels = static_cast<unsigned char*>(malloc(texture_width * texture_height * 4));
        Downscale_Image(static_cast<const unsigned char*>(p_sf_image->getPixelsPtr()), p_sf_image->getSize().x, p_sf_image->getSize().y, 8 /* getPixelsPtr() guarantees 8 BPP */, new_pixels, reduce_block_x, reduce_block_y);

        sf::Image* p_new_image = new sf::Image();
        p_new_image->create(texture_width, texture_height, static_cast<const uint8_t*>(new_pixels));

        delete p_sf_image;
        p_sf_image = p_new_image;

        free(new_pixels);
    }

    // use the generated texture
    glBindTexture(GL_TEXTURE_2D, image_num);

    // set texture wrap modes which control how to interpret texture coordinates
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // set texture magnification function
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // upload to OpenGL texture
    Create_GL_Texture(texture_width, texture_height, p_sf_image->getPixelsPtr(), mipmap);

    // unset pixel store mode
    // OLD (see corresponding call further above) glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    delete p_sf_image;

    // create OpenGL surface class
    cGL_Surface* image = new cGL_Surface();
    image->m_image = image_num;
    image->m_tex_w = texture_width;
    image->m_tex_h = texture_height;
    image->m_start_w = static_cast<float>(width);
    image->m_start_h = static_cast<float>(height);
    image->m_w = image->m_start_w;
    image->m_h = image->m_start_h;
    image->m_col_w = image->m_w;
    image->m_col_h = image->m_h;

    // if debug build check for errors
#ifdef _DEBUG
    // glGetError only saves one error flag
    GLenum error = glGetError();

    if (error != GL_NO_ERROR) {
        cerr << "CreateTexture : GL Error found : " << gluErrorString(error) << endl;
    }
#endif

    return image;
}

void cVideo::Create_GL_Texture(unsigned int width, unsigned int height, const void* pixels, bool mipmap /* = 0 */) const
{
    // unsigned byte is an unsigned 8-bit integer (1 byte)
    // create mipmaps
    if (mipmap) {
        // enable mipmap filter
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        // if OpenGL 1.4 or higher
        if (m_opengl_version >= 1.4f) {
            // use glTexImage2D to create Mipmaps
            glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, 1);
            // copy the software bitmap into the opengl texture
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        }
        // OpenGL below 1.4
        else {
            // use glu to create Mipmaps
            gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        }
    }
    // no mipmaps
    else {
        // default texture minifying function
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        // copy the software bitmap into the opengl texture
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    }
}

Color cVideo::Get_Pixel(int x, int y) const
{
    GLubyte* pixel = new GLubyte[3];
    // read it
    glReadPixels(x, y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);

    // convert to color
    Color color = Color(pixel[0], pixel[1], pixel[2]);
    // delete data
    delete[] pixel;

    return color;
}

void cVideo::Clear_Screen(void) const
{
    pRenderer->Add(new cClear_Request());
}

void cVideo::Draw_Rect(const GL_rect* rect, float z, const Color* color, cRect_Request* request /* = NULL */) const
{
    if (!rect) {
        Draw_Rect(0, 0, static_cast<float>(game_res_w), static_cast<float>(game_res_h), z, color, request);
    }
    else {
        Draw_Rect(rect->m_x, rect->m_y, rect->m_w, rect->m_h, z, color, request);
    }
}

void cVideo::Draw_Rect(float x, float y, float width, float height, float z, const Color* color, cRect_Request* request /* = NULL */) const
{
    if (!color || height == 0 || width == 0) {
        return;
    }

    bool create_request = 0;

    if (!request) {
        create_request = 1;
        // create request
        request = new cRect_Request();
    }

    // rect
    request->m_rect.m_x = x;
    request->m_rect.m_y = y;
    request->m_rect.m_w = width;
    request->m_rect.m_h = height;

    // z position
    request->m_pos_z = z;

    // color
    request->m_color = *color;

    if (create_request) {
        // add request
        pRenderer->Add(request);
    }
}

void cVideo::Draw_Gradient(const GL_rect* rect, float z, const Color* color_1, const Color* color_2, ObjectDirection direction, cGradient_Request* request /* = NULL */) const
{
    if (!rect) {
        Draw_Gradient(0, 0, static_cast<float>(game_res_w), static_cast<float>(game_res_h), z, color_1, color_2, direction, request);
    }
    else {
        Draw_Gradient(rect->m_x, rect->m_y, rect->m_w, rect->m_h, z, color_1, color_2, direction, request);
    }
}

void cVideo::Draw_Gradient(float x, float y, float width, float height, float z, const Color* color_1, const Color* color_2, ObjectDirection direction, cGradient_Request* request /* = NULL */) const
{
    if (!color_1 || !color_2 || height == 0 || width == 0) {
        return;
    }

    bool create_request = 0;

    if (!request) {
        create_request = 1;
        // create request
        request = new cGradient_Request();
    }

    // rect
    request->m_rect.m_x = x;
    request->m_rect.m_y = y;
    request->m_rect.m_w = width;
    request->m_rect.m_h = height;

    // z position
    request->m_pos_z = z;

    // color
    request->m_color_1 = *color_1;
    request->m_color_2 = *color_2;

    // direction
    request->m_dir = direction;

    if (create_request) {
        // add request
        pRenderer->Add(request);
    }
}

void cVideo::Draw_Circle(float x, float y, float radius, float z, const Color* color, cCircle_Request* request /* = NULL */) const
{
    if (!color || radius <= 0) {
        return;
    }

    bool create_request = 0;

    if (!request) {
        create_request = 1;
        // create request
        request = new cCircle_Request();
    }

    // position
    request->m_pos.m_x = x;
    request->m_pos.m_y = y;
    // radius
    request->m_radius = radius;

    // z position
    request->m_pos_z = z;

    // color
    request->m_color = *color;

    if (create_request) {
        // add request
        pRenderer->Add(request);
    }
}

void cVideo::Draw_Line(const GL_line* line, float z, const Color* color, cLine_Request* request /* = NULL */) const
{
    if (!line) {
        return;
    }

    Draw_Line(line->m_x1, line->m_y1, line->m_x2, line->m_y2, z, color, request);
}

void cVideo::Draw_Line(float x1, float y1, float x2, float y2, float z, const Color* color, cLine_Request* request /* = NULL */) const
{
    if (!color) {
        return;
    }

    bool create_request = 0;

    if (!request) {
        create_request = 1;
        // create request
        request = new cLine_Request();
    }

    // line
    request->m_line.m_x1 = x1;
    request->m_line.m_y1 = y1;
    request->m_line.m_x2 = x2;
    request->m_line.m_y2 = y2;

    // z position
    request->m_pos_z = z;

    // color
    request->m_color = *color;

    if (create_request) {
        // add request
        pRenderer->Add(request);
    }
}

float cVideo::Get_Scale(const cGL_Surface* image, float width, float height, bool only_downscale /* = 1 */) const
{
    if (!image) {
        return 0;
    }

    // change size
    if (!(only_downscale && image->m_h <= height && image->m_w <= width)) {
        float zoom = width / image->m_w;

        if (height / image->m_h < zoom) { // if height is smaller
            zoom = height / image->m_h;
        }

        return zoom;
    }

    return 1;
}

void cVideo::Apply_Max_Texture_Size(int& width, int& height) const
{
    if (width > m_max_texture_size) {
        // change height to keep aspect ratio
        int scale_down = width / m_max_texture_size;

        if (scale_down < 1) {
            debug_print("Warning : image height scale down %d is invalid\n", scale_down);
            scale_down = 1;
        }

        height = height / scale_down;
        width = m_max_texture_size;
    }
    if (height > m_max_texture_size) {
        // change width to keep aspect ratio
        int scale_down = height / m_max_texture_size;

        if (scale_down < 1) {
            debug_print("Warning : image width scale down %d is invalid\n", scale_down);
            scale_down = 1;
        }

        width = width / scale_down;
        height = m_max_texture_size;
    }
}

/* function from Jonathan Dummer
 * from image helper functions
 * MIT license
*/
bool cVideo::Downscale_Image(const unsigned char* const orig, int width, int height, int channels, unsigned char* resampled, int block_size_x, int block_size_y) const
{
    // error check
    if (width <= 0 || height <= 0 || channels <= 0 || orig == NULL || resampled == NULL || block_size_x <= 0 || block_size_y <= 0) {
        // invalid argument
        return 0;
    }

    int mip_width = width / block_size_x;
    int mip_height = height / block_size_y;

    // check size
    if (mip_width < 1) {
        mip_width = 1;
    }
    if (mip_height < 1) {
        mip_height = 1;
    }

    int j, i, c;

    for (j = 0; j < mip_height; ++j) {
        for (i = 0; i < mip_width; ++i) {
            for (c = 0; c < channels; ++c) {
                const int index = (j * block_size_y) * width * channels + (i * block_size_x) * channels + c;
                int sum_value;
                int u,v;
                int u_block = block_size_x;
                int v_block = block_size_y;
                int block_area;

                /* do a bit of checking so we don't over-run the boundaries
                 * necessary for non-square textures!
                 */
                if (block_size_x * (i + 1) > width) {
                    u_block = width - i * block_size_y;
                }
                if (block_size_y * (j + 1) > height) {
                    v_block = height - j * block_size_y;
                }
                block_area = u_block * v_block;

                /* for this pixel, see what the average
                 * of all the values in the block are.
                 * note: start the sum at the rounding value, not at 0
                 */
                sum_value = block_area >> 1;
                for (v = 0; v < v_block; ++v) {
                    for (u = 0; u < u_block; ++u) {
                        sum_value += orig[index + v * width * channels + u * channels];
                    }
                }

                resampled[j * mip_width * channels + i * channels + c] = sum_value / block_area;
            }
        }
    }

    return 1;
}

void cVideo::Save_Screenshot(void)
{
    Render_Finish();

    fs::path filename;

    for (unsigned int i = 1; i < 1000; i++) {
        filename = pResource_Manager->Get_User_Screenshot_Directory() / utf8_to_path(int_to_string(i) + ".png");

        if (!File_Exists(filename)) {
            // create image data
            GLubyte* data = new GLubyte[pPreferences->m_video_screen_w * pPreferences->m_video_screen_h * 3];
            // read opengl screen
            glReadPixels(0, 0, pPreferences->m_video_screen_w, pPreferences->m_video_screen_h, GL_RGB, GL_UNSIGNED_BYTE, static_cast<GLvoid*>(data));
            // save
            Save_Surface(filename, data, pPreferences->m_video_screen_w, pPreferences->m_video_screen_h, 3, 1);
            // clear data
            delete[] data;

            // show info
            gp_hud->Set_Text("Screenshot " + int_to_string(i) + _(" saved"));

            // finished
            return;
        }
    }
}

void cVideo::Save_Surface(const fs::path& filename, const unsigned char* data, unsigned int width, unsigned int height, unsigned int bpp /* = 4 */, bool reverse_data /* = 0 */) const
{
    FILE* fp = NULL;

    // This is bad. libpng directly wants a FILE pointer and there’s no way
    // to use boost’s much better fs::ofstream. So we have to resort to getting
    // the underlying platform-specific representation of `filename' (which is
    // an UTF-16 string on Windows and UTF-8 everywhere else) and then call the
    // platform-specific function to open a unicode-filename file with that platform-
    // specific string. Uah.
#ifdef _WIN32
    fp = _wfopen(filename.native().c_str(), L"wb");
#else
    fp = fopen(filename.native().c_str(), "wb");
#endif

    if (!fp) {
        cerr << "Warning: cVideo :: Save_Surface : Could not create file " << path_to_utf8(filename) << " for writing" << endl;
        return;
    }

    int png_color_type;

    if (bpp == 4) {
        png_color_type = PNG_COLOR_TYPE_RGBA;
    }
    else if (bpp == 3) {
        png_color_type = PNG_COLOR_TYPE_RGB;
    }
    else {
        cerr << "Warning: cVideo :: Save_Surface : " << filename.c_str() << " Unknown bytes per pixel " << bpp << endl;
        fclose(fp);
        return;
    }

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info_ptr = png_create_info_struct(png_ptr);

    png_init_io(png_ptr, fp);

    png_set_IHDR(png_ptr, info_ptr,
                 width, height, 8 /* bit depth */, png_color_type,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_ptr, info_ptr);

    png_uint_32 png_height = height;
    png_uint_32 row_bytes = width * bpp;

    png_byte* image = new png_byte[png_height * row_bytes];
    png_bytep* row_pointers = new png_bytep[png_height];

    // create image data
    int img_size = png_height * row_bytes;
    for (int i = 0; i < img_size; ++i) {
        image[i] = data[i];
    }

    // create row pointers
    if (reverse_data) {
        for (unsigned int i = 0; i < png_height; i++) {
            // reverse direction because of opengl glReadPixels
            row_pointers[png_height - 1 - i] = image + (i * row_bytes);
        }
    }
    else {
        for (unsigned int i = 0; i < png_height; i++) {
            row_pointers[i] = image + (i * row_bytes);
        }
    }

    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);

    delete []image;
    delete []row_pointers;

    fclose(fp);
}

static void Handle_Important_Events(const sf::Event& event)
{
    switch (event.type) {
    case sf::Event::JoystickConnected:
    case sf::Event::JoystickDisconnected:
        pJoystick->Init();
        break;
    default: break;
    }
}

bool cVideo::PollEvent(sf::Event& event) {
    if (mp_window->pollEvent(event)) {
        Handle_Important_Events(event);
        return true;
    } else {
        return false;
    }
}

bool cVideo::WaitEvent(sf::Event& event) {
    if (mp_window->waitEvent(event)) {
        Handle_Important_Events(event);
        return true;
    } else {
        return false;
    }
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

void Draw_Effect_Out(Effect_Fadeout effect /* = EFFECT_OUT_RANDOM */, float speed /* = 1 */)
{
    if (effect == EFFECT_OUT_RANDOM) {
        effect = static_cast<Effect_Fadeout>((rand() % (EFFECT_OUT_AMOUNT - 1)) + 1);
    }

    switch (effect) {
    case EFFECT_OUT_BLACK: {
        Color color = static_cast<uint8_t>(0);

        for (float i = 1; i > 0; i -= (speed / 30) * pFramerate->m_speed_factor) {
            color.alpha = static_cast<uint8_t>(45 - (45 * i));

            // create request
            cRect_Request* request = new cRect_Request();
            pVideo->Draw_Rect(NULL, 0.9f, &color, request);

            request->m_render_count = 2;

            // add request
            pRenderer->Add(request);

            pVideo->Render();

            pFramerate->Update();
            // maximum fps
            Correct_Frame_Time(100);
        }

        break;
    }
    case EFFECT_OUT_HORIZONTAL_VERTICAL: {
        int hor = (rand() % 2) - 1;
        float pos = 0;
        float pos_end = 0;

        // horizontal
        if (hor) {
            pos = static_cast<float>(game_res_w);
        }
        // vertical
        else {
            pos = static_cast<float>(game_res_h);
        }

        Color color = Color(static_cast<uint8_t>(0), 0, 0, 0);
        float alpha = 10.0f;

        while (pos > pos_end * 0.5f) {
            // fade alpha in
            if (alpha < 255.0f) {
                alpha += 10 * pFramerate->m_speed_factor;

                if (alpha > 255.0f) {
                    alpha = 255.0f;
                }

                color.alpha = static_cast<uint8_t>(alpha);
            }

            // draw gradient
            cGradient_Request* gradient_request = new cGradient_Request();

            // horizontal
            if (hor) {
                pos -= 20 * pFramerate->m_speed_factor;
                pos_end = static_cast<float>(game_res_w) - pos;
                // left
                pVideo->Draw_Gradient(0, 0, pos_end, static_cast<float>(game_res_h), 0.9f, &color, &black, DIR_HORIZONTAL, gradient_request);
            }
            // vertical
            else {
                pos -= 15 * pFramerate->m_speed_factor;
                pos_end = static_cast<float>(game_res_h) - pos;
                // top
                pVideo->Draw_Gradient(0, 0, static_cast<float>(game_res_w), pos_end, 0.9f, &color, &black, DIR_VERTICAL, gradient_request);
            }

            gradient_request->m_render_count = 2;
            // add request
            pRenderer->Add(gradient_request);

            gradient_request = new cGradient_Request();

            // horizontal
            if (hor) {
                // right
                pVideo->Draw_Gradient(static_cast<float>(game_res_w) - pos_end, 0, pos_end, static_cast<float>(game_res_h), 0.9f, &color, &black, DIR_HORIZONTAL, gradient_request);
            }
            // vertical
            else {
                // down
                pVideo->Draw_Gradient(0, static_cast<float>(game_res_h) - pos_end, static_cast<float>(game_res_w), pos_end, 0.9f, &color, &black, DIR_VERTICAL, gradient_request);
            }

            gradient_request->m_render_count = 2;
            // add request
            pRenderer->Add(gradient_request);

            // draw game
            Draw_Game();

            pVideo->Render();

            pFramerate->Update();
            // maximum fps
            Correct_Frame_Time(100);
        }
        break;
    }
    case EFFECT_OUT_BIG_ITEM: {
        float f = 0.1f;
        cGL_Surface* image = NULL;

        // item based on the camera x position
        if (pActive_Camera->m_x < 2000) {
            image = pVideo->Get_Surface("game/items/mushroom_red.png");
        }
        else if (pActive_Camera->m_x < 5000) {
            image = pVideo->Get_Surface("game/items/fireberry_1.png");
        }
        else if (pActive_Camera->m_x < 10000) {
            image = pVideo->Get_Surface("game/items/mushroom_green.png");
        }
        else if (pActive_Camera->m_x < 20000) {
            image = pVideo->Get_Surface("game/items/lemon_1.png");
        }
        else {
            image = pVideo->Get_Surface("game/items/moon_1.png");
        }

        Color color = white;

        while (f < 50) {
            Draw_Game();

            f += 0.9f * pFramerate->m_speed_factor * speed * (f / 7);

            color = Color(static_cast<uint8_t>(255 - (f * 4)), 255 - static_cast<uint8_t>(f * 4), 255 - static_cast<uint8_t>(f * 4), 200 - static_cast<uint8_t>(f * 4));

            // ## item
            // create request
            cSurface_Request* request = new cSurface_Request();
            image->Blit((game_res_w * 0.5f) - ((image->m_w * f) / 2) , game_res_h * 0.5f - ((image->m_h * f) / 2), 0.9f, request);

            request->m_blend_sfactor = GL_SRC_ALPHA;
            request->m_blend_dfactor = GL_ONE;

            request->m_color = color;

            // scale
            request->m_scale_x = f;
            request->m_scale_y = f;

            // add request
            pRenderer->Add(request);


            // ## additional black fadeout
            color = Color(0, 0, 0, static_cast<uint8_t>(50 + (f * 4)));

            // create request
            cRect_Request* rect_request = new cRect_Request();
            pVideo->Draw_Rect(NULL, 0.901f, &color, rect_request);

            // add request
            pRenderer->Add(rect_request);

            pVideo->Render();
            pFramerate->Update();
        }

        break;
    }
    case EFFECT_OUT_RANDOM_COLOR_BOOST: {
        unsigned int rand_color_num = (rand() % 4);

        Color rand_color;

        // red
        if (rand_color_num == 0) {
            rand_color = Color(static_cast<uint8_t>(1), 20, 20, 4);
        }
        // green
        else if (rand_color_num == 1) {
            rand_color = Color(static_cast<uint8_t>(20), 1, 20, 4);
        }
        // blue
        else if (rand_color_num == 2) {
            rand_color = Color(static_cast<uint8_t>(20), 20, 1, 4);
        }
        // yellow
        else {
            rand_color = Color(static_cast<uint8_t>(1), 1, 40, 4);
        }

        // rect size
        float rect_size = 1.0f;

        while (rect_size < 200.0f) {
            rect_size += 4.0f * pFramerate->m_speed_factor;

            for (unsigned int g = 0; g < 50; g++) {
                // create request
                cRect_Request* request = new cRect_Request();
                pVideo->Draw_Rect(Get_Random_Float(-rect_size * 0.5f, game_res_w - rect_size * 0.5f), Get_Random_Float(-rect_size * 0.5f, game_res_h - rect_size * 0.5f), rect_size, rect_size, 0.9f, &rand_color, request);

                request->m_render_count = 2;

                request->m_blend_sfactor = GL_SRC_ALPHA;
                request->m_blend_dfactor = GL_ONE_MINUS_SRC_COLOR;

                // add request
                pRenderer->Add(request);
            }

            pVideo->Render();
            pFramerate->Update();
        }
        break;
    }
    case EFFECT_OUT_BLACK_TILED_RECTS: {
        // grid settings
        const unsigned int tiles_num_hor = 8;
        const unsigned int tiles_num_ver = 6;
        const unsigned int tiles_num = tiles_num_hor * tiles_num_ver;

        // the grid
        float grid[tiles_num_ver][tiles_num_hor];

        // init grid
        for (unsigned int i = 0; i < tiles_num_ver; i++) {
            for (unsigned int j = 0; j < tiles_num_hor; j++) {
                grid[i][j] = 0.0f;
            }
        }

        unsigned int selected_tile_x = 0;
        unsigned int selected_tile_y = 0;

        unsigned int activated_tiles = 0;

        GL_rect dest(0, 0, static_cast<float>(game_res_w) / tiles_num_hor, static_cast<float>(game_res_h) / tiles_num_ver);
        Color color = black;

        // update until the latest tile did fade in
        while (grid[selected_tile_y][selected_tile_x] < 60.0f) {
            // if not all activated
            if (activated_tiles < tiles_num) {
                // find an unused rect
                while (grid[selected_tile_y][selected_tile_x] > 0.1f) {
                    unsigned int temp = rand() % tiles_num;

                    selected_tile_y = temp / tiles_num_hor;
                    selected_tile_x = temp % tiles_num_hor;
                }

                // activate it
                grid[selected_tile_y][selected_tile_x] = 0.2f;
                activated_tiles++;
            }

            // fade in and draw all activated tiles
            for (unsigned int x = 0; x < tiles_num_hor; x++) {
                for (unsigned int y = 0; y < tiles_num_ver; y++) {
                    // not activated
                    if (grid[y][x] < 0.1f) {
                        continue;
                    }

                    // fade in
                    if (grid[y][x] < 120.0f) {
                        grid[y][x] += pFramerate->m_speed_factor;

                        if (grid[y][x] > 120.0f) {
                            grid[y][x] = 120.0f;
                        }
                    }

                    // set position
                    dest.m_x = x * dest.m_w;
                    dest.m_y = y * dest.m_h;
                    // set alpha
                    color.alpha = static_cast<uint8_t>(grid[y][x] * 0.4f);

                    // create request
                    cRect_Request* request = new cRect_Request();
                    pVideo->Draw_Rect(&dest, 0.9f, &color, request);

                    // rotation
                    request->m_rot_z = grid[y][x] * 5.0f;

                    // scale
                    request->m_scale_x = 0.1f + (grid[y][x] * 0.02f);
                    request->m_scale_y = request->m_scale_x;
                    request->m_rect.m_x -= (dest.m_w * 0.5f) * (request->m_scale_x - 1.0f);
                    request->m_rect.m_y -= (dest.m_h * 0.5f) * (request->m_scale_y - 1.0f);


                    request->m_render_count = 2;
                    // add request
                    pRenderer->Add(request);
                }
            }

            pVideo->Render();
            pFramerate->Update();
            // correction needed
            Correct_Frame_Time(speedfactor_fps * 2);
        }
        break;
    }
    case EFFECT_OUT_FIXED_COLORBOX: {
        Color start_color;

        const unsigned int rand_color = (rand() % 2);

        // green
        if (rand_color == 0) {
            start_color = Color(static_cast<uint8_t>(10), 55, 10, 250);
        }
        // blue
        else {
            start_color = Color(static_cast<uint8_t>(10), 10, 55, 250);
        }

        Color color = start_color;
        float color_mod = 1.0f;

        // position
        float pos_x = 0;
        float pos_y = 0;
        // size
        float rect_size = 20;
        // rect used for grid drawing
        GL_rect rect;

        // animate until size reached the limit
        while (rect_size < 35) {
            // add size
            rect_size += 0.3f * pFramerate->m_speed_factor;

            // change color modification
            if (color_mod > 0.0f) {
                color_mod -= 0.04f * pFramerate->m_speed_factor;

                if (color_mod < 0.0f) {
                    color_mod = 0.0f;
                }

                // darken color
                color.red = static_cast<uint8_t>(start_color.red * color_mod);
                color.green = static_cast<uint8_t>(start_color.green * color_mod);
                color.blue = static_cast<uint8_t>(start_color.blue * color_mod);
                color.alpha = static_cast<uint8_t>(rect_size * 0.3f);
            }

            // continuous random position advance
            float random = Get_Random_Float(2.0f, 3.0f);
            pos_x -= random;
            pos_y -= random + Get_Random_Float(0.1f, 0.1f);

            // draw rects as a net
            // horizontal
            for (rect.m_x = pos_x; rect.m_x < game_res_w; rect.m_x += 20 + (rect_size * color_mod)) {
                // vertical
                for (rect.m_y = pos_y; rect.m_y < game_res_h; rect.m_y += 20 + (rect_size * color_mod)) {
                    rect.m_w = Get_Random_Float(1.0f, 0.2f + (rect_size * 1.5f));
                    rect.m_h = Get_Random_Float(1.0f, 0.2f + (rect_size * 1.5f));

                    // create request
                    cRect_Request* request = new cRect_Request();
                    pVideo->Draw_Rect(&rect, 0.9f, &color, request);

                    request->m_render_count = 2;

                    // add request
                    pRenderer->Add(request);
                }
            }

            Color rect_color;
            rect_color.red = static_cast<uint8_t>(start_color.red * 0.1f * color_mod);
            rect_color.green = static_cast<uint8_t>(start_color.green * 0.1f * color_mod);
            rect_color.blue = static_cast<uint8_t>(start_color.blue * 0.1f * color_mod);
            rect_color.alpha = static_cast<uint8_t>(rect_size * 3);
            // create request
            cRect_Request* request = new cRect_Request();
            pVideo->Draw_Rect(NULL, 0.9f, &rect_color, request);

            request->m_render_count = 2;

            request->m_blend_sfactor = GL_SRC_ALPHA;
            request->m_blend_dfactor = GL_ONE_MINUS_SRC_COLOR;

            // add request
            pRenderer->Add(request);

            pVideo->Render();
            pFramerate->Update();
            // correction needed
            Correct_Frame_Time(speedfactor_fps * 2);
        }
        break;
    }
    default:
        break;  // be happy
    }

    pFramerate->Update();
}

void Draw_Effect_In(Effect_Fadein effect /* = EFFECT_IN_RANDOM */, float speed /* = 1 */)
{
    // Clear render cache
    pRenderer->Clear(1);

    if (effect == EFFECT_IN_RANDOM) {
        effect = static_cast<Effect_Fadein>((rand() % (EFFECT_IN_AMOUNT - 1)) + 1);
    }

    switch (effect) {
    case EFFECT_IN_BLACK: {
        Color color = static_cast<uint8_t>(0);

        for (float i = 1; i > 0; i -= (speed / 30) * pFramerate->m_speed_factor) {
            color.alpha = static_cast<uint8_t>(255 * i);

            // create request
            cRect_Request* request = new cRect_Request();
            pVideo->Draw_Rect(NULL, 0.9f, &color, request);

            // add request
            pRenderer->Add(request);

            Draw_Game();

            pVideo->Render();

            pFramerate->Update();
            // maximum fps
            Correct_Frame_Time(100);
        }

        break;
    }
    default:
        break;  // be happy
    }

    pFramerate->Update();
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

cVideo* pVideo = NULL;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

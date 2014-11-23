/***************************************************************************
 * video.cpp  -  General video functions
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

#include "../video/video.hpp"
#include "../gui/hud.hpp"
#include "../user/preferences.hpp"
#include "../core/framerate.hpp"
#include "../video/font.hpp"
#include "../core/game_core.hpp"
#include "../video/img_settings.hpp"
#include "../input/mouse.hpp"
#include "../video/renderer.hpp"
#include "../core/main.hpp"
#include "../core/math/utilities.hpp"
#include "../core/i18n.hpp"
#include "../core/math/size.hpp"
#include "../core/filesystem/filesystem.hpp"
#include "../core/filesystem/resource_manager.hpp"
#include "../gui/spinner.hpp"

namespace fs = boost::filesystem;

namespace TSC {

/* *** *** *** *** *** *** *** Video class *** *** *** *** *** *** *** *** *** *** */

cVideo::cVideo(void)
{
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

    SDL_VERSION(&wm_info.version);
#ifdef __unix__
    glx_context = NULL;
#endif
    m_render_thread = boost::thread();

    m_initialised = 0;
}

cVideo::~cVideo(void)
{

}

void cVideo::Init_CEGUI(void) const
{
    // create renderer
    try {
        pGuiRenderer = &CEGUI::OpenGLRenderer::create(CEGUI::Size(screen->w, screen->h));
    }
    // catch CEGUI Exceptions
    catch (CEGUI::Exception& ex) {
        printf("CEGUI Exception occurred : %s\n", ex.getMessage().c_str());
        exit(EXIT_FAILURE);
    }

    pGuiRenderer->enableExtraStateSettings(1);

    // create Resource Provider
    CEGUI::DefaultResourceProvider* rp = new CEGUI::DefaultResourceProvider();

    // set Resource Provider directories
    rp->setResourceGroupDirectory("schemes", path_to_utf8(pResource_Manager->Get_Gui_Scheme_Directory()));
    rp->setResourceGroupDirectory("imagesets", path_to_utf8(pResource_Manager->Get_Gui_Imageset_Directory()));
    rp->setResourceGroupDirectory("fonts", path_to_utf8(pResource_Manager->Get_Gui_Font_Directory()));
    rp->setResourceGroupDirectory("looknfeels", path_to_utf8(pResource_Manager->Get_Gui_LookNFeel_Directory()));
    rp->setResourceGroupDirectory("layouts", path_to_utf8(pResource_Manager->Get_Gui_Layout_Directory()));

    if (CEGUI::System::getDefaultXMLParserName().compare("XercesParser") == 0) {
        // Needed for Xerces to specify the schemas location
        rp->setResourceGroupDirectory("schemas", path_to_utf8(pResource_Manager->Get_Game_Schema_Directory()).c_str());
    }

    // create logger
    CEGUI::Logger* logger = new CEGUI::DefaultLogger();
    // set logging level
#ifdef _DEBUG
    logger->setLoggingLevel(CEGUI::Informative);
#else
    logger->setLoggingLevel(CEGUI::Errors);
#endif

    // set initial mouse position
    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);
    CEGUI::MouseCursor::setInitialMousePosition(CEGUI::Point(mouse_x, mouse_y));
    // add custom widgets
    CEGUI::WindowFactoryManager::addFactory<CEGUI::TSC_SpinnerFactory>();

    // create system
    try {
        debug_print("CEGUI log file is at '%s'.\n", path_to_utf8(pResource_Manager->Get_User_CEGUI_Logfile()).c_str());
        // fixme : Workaround for std::string to CEGUI::String utf8 conversion. Check again if CEGUI 0.8 works with std::string utf8
#ifdef _WIN32
        pGuiSystem = &CEGUI::System::create(*pGuiRenderer, rp, NULL, NULL, NULL, "", (const CEGUI::utf8*)(path_to_utf8(pResource_Manager->Get_User_CEGUI_Logfile()).c_str()));
#else
        pGuiSystem = &CEGUI::System::create(*pGuiRenderer, rp, NULL, NULL, NULL, "", path_to_utf8(pResource_Manager->Get_User_CEGUI_Logfile()));
#endif
    }
    // catch CEGUI Exceptions
    catch (CEGUI::Exception& ex) {
        printf("CEGUI Exception occurred : %s\n", ex.getMessage().c_str());
        exit(EXIT_FAILURE);
    }
}

void cVideo::Init_CEGUI_Data(void) const
{
    // set the default resource groups to be used
    CEGUI::Scheme::setDefaultResourceGroup("schemes");
    CEGUI::Imageset::setDefaultResourceGroup("imagesets");
    CEGUI::Font::setDefaultResourceGroup("fonts");
    CEGUI::WidgetLookManager::setDefaultResourceGroup("looknfeels");
    CEGUI::WindowManager::setDefaultResourceGroup("layouts");

    // load the scheme file, which auto-loads the imageset
    try {
        CEGUI::SchemeManager::getSingleton().create("TaharezLook.scheme");
    }
    // catch CEGUI Exceptions
    catch (CEGUI::Exception& ex) {
        printf("CEGUI Scheme Exception occurred : %s\n", ex.getMessage().c_str());
        exit(EXIT_FAILURE);
    }

    // default mouse cursor
    pGuiSystem->setDefaultMouseCursor("TaharezLook", "MouseArrow");
    // force new mouse image
    CEGUI::MouseCursor::getSingleton().setImage(&CEGUI::ImagesetManager::getSingleton().get("TaharezLook").getImage("MouseArrow"));
    // default tooltip
    pGuiSystem->setDefaultTooltip("TaharezLook/Tooltip");

    // create default root window
    CEGUI::Window* window_root = CEGUI::WindowManager::getSingleton().loadWindowLayout("default.layout");
    pGuiSystem->setGUISheet(window_root);
    window_root->activate();
}

void cVideo::Init_SDL(void)
{
    if (SDL_Init(SDL_INIT_VIDEO) == -1) {
        printf("Error : SDL initialization failed\nReason : %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    atexit(SDL_Quit);

    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) == -1) {
        printf("Warning : SDL Joystick initialization failed\nReason : %s\n", SDL_GetError());
        m_joy_init_failed = 1;
    }
    else {
        m_joy_init_failed = 0;
    }

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) == -1) {
        printf("Warning : SDL Audio initialization failed\nReason : %s\n", SDL_GetError());
        m_audio_init_failed = 1;
    }
    else {
        m_audio_init_failed = 0;
    }

    // preload the sdl_image png library
    IMG_Init(IMG_INIT_PNG);

    SDL_EnableUNICODE(1);
    // hide by default
    SDL_ShowCursor(SDL_DISABLE);
}

void cVideo::Init_Video(bool reload_textures_from_file /* = 0 */, bool use_preferences /* = 1 */)
{
    Render_Finish();

    // set the video flags
    int flags = SDL_OPENGL | SDL_SWSURFACE;

    // only enter fullscreen if set in preferences
    if (use_preferences && pPreferences->m_video_fullscreen) {
        flags |= SDL_FULLSCREEN;
    }

    int screen_w, screen_h, screen_bpp;

    // full initialization
    if (use_preferences) {
        screen_w = pPreferences->m_video_screen_w;
        screen_h = pPreferences->m_video_screen_h;
        screen_bpp = pPreferences->m_video_screen_bpp;
    }
    // initialization with SDL defaults
    else {
        screen_w = 800;
        screen_h = 600;
        screen_bpp = 16;
    }

    // first initialization
    if (!m_initialised) {
        // Set Caption
        SDL_WM_SetCaption(CAPTION, NULL);
        // Set Icon
        boost::filesystem::path filename_icon = pResource_Manager->Get_Game_Icon("window_32.png");
        if (File_Exists(filename_icon)) {
            SDL_Surface* icon = IMG_Load(path_to_utf8(filename_icon).c_str());
            SDL_WM_SetIcon(icon, NULL);
            SDL_FreeSurface(icon);
        }
        else {
            std::cerr << "Warning: Window icon '" << path_to_utf8(filename_icon) << "' does not exist" << std::endl;
        }
    }

    // test screen mode
    int screen_test = Test_Video(screen_w, screen_h, screen_bpp, flags);

    // failed
    if (screen_test == 0) {
        printf("Warning : Video Resolution %dx%d is not supported\n", screen_w, screen_h);

        // set lowest available settings
        screen_w = 640;
        screen_h = 480;
        screen_bpp = 0;

        // overwrite user settings
        if (use_preferences) {
            pPreferences->m_video_screen_w = screen_w;
            pPreferences->m_video_screen_h = screen_h;
        }
    }
    // can not handle bits per pixel
    else if (screen_test > 1 && screen_bpp > 0 && screen_test < screen_bpp) {
        printf("Warning : Video Bpp %d is not supported but %d is\n", screen_bpp, screen_test);
        // set closest supported bpp
        screen_bpp = screen_test;

        // overwrite user settings
        if (use_preferences) {
            pPreferences->m_video_screen_bpp = screen_bpp;
        }
    }

    int screen_rgb_size[3];

    // set bit per pixel sizes
    if (screen_bpp == 8) {
        screen_rgb_size[0] = 3;
        screen_rgb_size[1] = 3;
        screen_rgb_size[2] = 2;
    }
    else if (screen_bpp == 15) {
        screen_rgb_size[0] = 5;
        screen_rgb_size[1] = 5;
        screen_rgb_size[2] = 5;
    }
    else if (screen_bpp == 24) {
        screen_rgb_size[0] = 8;
        screen_rgb_size[1] = 8;
        screen_rgb_size[2] = 8;
    }
    // same as 24...
    else if (screen_bpp == 32) {
        screen_rgb_size[0] = 8;
        screen_rgb_size[1] = 8;
        screen_rgb_size[2] = 8;
    }
    else { // 16 and default
        screen_rgb_size[0] = 5;
        screen_rgb_size[1] = 6;
        screen_rgb_size[2] = 5;
    }

    // request settings
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, screen_rgb_size[0]);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, screen_rgb_size[1]);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, screen_rgb_size[2]);
    // hangs on 16 bit
    //SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
    // not yet needed
    //SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    // if vertical synchronization is enabled
    if (use_preferences && pPreferences->m_video_vsync) {
        SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);
    }

    // if reinitialization
    if (m_initialised) {
        // check if CEGUI is initialized
        bool cegui_initialized = pGuiSystem->getGUISheet() != NULL;

        // show loading screen
        if (cegui_initialized) {
            Loading_Screen_Init();
        }

        // save textures
        pImage_Manager->Grab_Textures(reload_textures_from_file, cegui_initialized);
        pFont->Grab_Textures();
        pGuiRenderer->grabTextures();
        pImage_Manager->Delete_Hardware_Textures();

        // exit loading screen
        if (cegui_initialized) {
            Loading_Screen_Exit();
        }
    }

    // Note: As of SDL 1.2.10, if width and height are both 0, SDL_SetVideoMode will use the desktop resolution.
    screen = SDL_SetVideoMode(screen_w, screen_h, screen_bpp, flags);

    if (!screen) {
        printf("Error : Screen mode creation failed\nReason : %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    // check if fullscreen got set
    if (use_preferences && pPreferences->m_video_fullscreen) {
        bool is_fullscreen = ((screen->flags & SDL_FULLSCREEN) == SDL_FULLSCREEN);

        if (!is_fullscreen) {
            printf("Warning : Fullscreen mode could not be set\n");
        }
    }

    // check if double buffering got set
    int is_double_buffer;
    SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &is_double_buffer);
    m_double_buffer = is_double_buffer > 0;

    if (!m_double_buffer) {
        // only important on full initialization
        if (use_preferences) {
            printf("Warning : Double Buffering could not be set\n");
        }
    }

    // check if vertical synchronization got set
    if (use_preferences && pPreferences->m_video_vsync) {
        int is_vsync;
        // seems to return always true even if not available
        SDL_GL_GetAttribute(SDL_GL_SWAP_CONTROL, &is_vsync);

        if (!is_vsync) {
            printf("Warning : VSync could not be set\n");
        }
    }

    // get color bit size
    SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &m_rgb_size[0]);
    SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &m_rgb_size[1]);
    SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &m_rgb_size[2]);

    // check if color bit size is set as wanted
    if (use_preferences) {
        if (m_rgb_size[0] < screen_rgb_size[0]) {
            printf("Warning : smaller red bit size %d as requested %d\n", m_rgb_size[0], screen_rgb_size[0]);
        }

        if (m_rgb_size[1] < screen_rgb_size[1]) {
            printf("Warning : smaller green bit size %d as requested %d\n", m_rgb_size[1], screen_rgb_size[1]);
        }

        if (m_rgb_size[2] < screen_rgb_size[2]) {
            printf("Warning : smaller blue bit size %d as requested %d\n", m_rgb_size[2], screen_rgb_size[2]);
        }
    }

    // remember default buffer
    glGetIntegerv(GL_DRAW_BUFFER, &m_default_buffer);
    // get maximum texture size
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m_max_texture_size);

    /* check if accelerated visual
    int accelerated = 0;
    SDL_GL_GetAttribute( SDL_GL_ACCELERATED_VISUAL, &accelerated );
    printf( "accel %d\n", accelerated );*/

    // get window manager information
    if (!SDL_GetWMInfo(&wm_info)) {
        printf("Error: SDL_GetWMInfo not implemented\n");
    }
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
        pGuiRenderer->restoreTextures();
        pFont->Restore_Textures();

        // send new size to CEGUI
        pGuiSystem->notifyDisplaySizeChanged(CEGUI::Size(static_cast<float>(screen_w), static_cast<float>(screen_h)));

        // check if CEGUI is initialized
        bool cegui_initialized = pGuiSystem->getGUISheet() != NULL;

        // show loading screen
        if (cegui_initialized) {
            Loading_Screen_Init();
        }

        // initialize new image cache
        if (reload_textures_from_file) {
            Init_Image_Cache(0, cegui_initialized);
        }

        // restore textures
        pImage_Manager->Restore_Textures(cegui_initialized);

        // exit loading screen
        if (cegui_initialized) {
            Loading_Screen_Exit();
        }
    }
    // finished first initialization
    else {
        // get opengl version
        std::string version_str = reinterpret_cast<const char*>(glGetString(GL_VERSION));
        // erase everything after X.X
        version_str.erase(3);

        m_opengl_version = string_to_float(version_str);

        // if below optimal version
        if (m_opengl_version < 1.4f) {
            if (m_opengl_version >= 1.3f) {
                printf("Info : OpenGL Version %.1f is below the optimal version 1.4 and higher\n", m_opengl_version);
            }
            else {
                printf("Warning : OpenGL Version %.1f is below version 1.3\n", m_opengl_version);
            }

        }

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

    SDL_GL_SwapBuffers();
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

void cVideo::Init_Image_Cache(bool recreate /* = 0 */, bool draw_gui /* = 0 */)
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
                printf("%s\n", ex.what());

                if (draw_gui) {
                    // caching failed
                    Loading_Screen_Draw_Text(_("Caching Images failed : Could not remove old images"));
                    SDL_Delay(2000);
                }
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

    CEGUI::ProgressBar* progress_bar = NULL;

    if (draw_gui) {
        // get progress bar
        progress_bar = static_cast<CEGUI::ProgressBar*>(CEGUI::WindowManager::getSingleton().getWindow("progress_bar"));
        progress_bar->setProgress(0);

        // set loading screen text
        Loading_Screen_Draw_Text(_("Caching Images"));
    }

    // get all files
    vector<fs::path> image_files = Get_Directory_Files(pResource_Manager->Get_Game_Pixmaps_Directory(), ".settings", true);

    unsigned int loaded_files = 0;
    unsigned int file_count = image_files.size();

    // create directories, load images and save to cache
    for (vector<fs::path>::iterator itr = image_files.begin(); itr != image_files.end(); ++itr) {
        // get filenames
        fs::path filename = (*itr);
        fs::path cache_filename = imgcache_dir_active / fs::relative(pResource_Manager->Get_Game_Data_Directory(), filename);

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
        SDL_Surface* sdl_surface = software_image.m_sdl_surface;
        cImage_Settings_Data* settings = software_image.m_settings;

        // failed to load image
        if (!sdl_surface) {
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
            SDL_FreeSurface(sdl_surface);
            continue;
        }

        // create final image
        sdl_surface = Convert_To_Final_Software_Image(sdl_surface);

        // get final size for this resolution
        cSize_Int size = settings->Get_Surface_Size(sdl_surface);
        delete settings;
        int new_width = size.m_width;
        int new_height = size.m_height;

        // apply maximum texture size
        Apply_Max_Texture_Size(new_width, new_height);

        // does not need to be downsampled
        if (new_width >= sdl_surface->w && new_height >= sdl_surface->h) {
            SDL_FreeSurface(sdl_surface);
            continue;
        }

        // calculate block reduction
        int reduce_block_x = sdl_surface->w / new_width;
        int reduce_block_y = sdl_surface->h / new_height;

        // create downsampled image
        unsigned int image_bpp = sdl_surface->format->BytesPerPixel;
        unsigned char* image_downsampled = new unsigned char[new_width * new_height * image_bpp];
        bool downsampled = Downscale_Image(static_cast<unsigned char*>(sdl_surface->pixels), sdl_surface->w, sdl_surface->h, image_bpp, image_downsampled, reduce_block_x, reduce_block_y);

        SDL_FreeSurface(sdl_surface);

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
        if (draw_gui) {
            // update progress
            progress_bar->setProgress(static_cast<float>(loaded_files) / static_cast<float>(file_count));

#ifdef _DEBUG
            // update filename
            cGL_Surface* surface_filename = pFont->Render_Text(pFont->m_font_small, path_to_utf8(filename), white);
            // draw filename
            surface_filename->Blit(game_res_w * 0.2f, game_res_h * 0.8f, 0.1f);
#endif
            Loading_Screen_Draw();
#ifdef _DEBUG
            // delete
            delete surface_filename;
#endif
        }
    }

    // set back texture detail
    m_texture_quality = real_texture_detail;
    // set directory after surfaces got loaded from Load_GL_Surface()
    m_imgcache_dir = imgcache_dir_active;
}

int cVideo::Test_Video(int width, int height, int bpp, int flags /* = 0 */) const
{
    // auto set the video flags
    if (!flags) {
        flags = SDL_OPENGL | SDL_SWSURFACE;

        // if fullscreen is set
        if (pPreferences->m_video_fullscreen) {
            flags |= SDL_FULLSCREEN;
        }
    }

    return SDL_VideoModeOK(width, height, bpp, flags);
}

vector<cSize_Int> cVideo::Get_Supported_Resolutions(int flags /* = 0 */) const
{
    vector<cSize_Int> valid_resolutions;

    // auto set the video flags
    if (!flags) {
        // always set fullscreen
        flags = SDL_OPENGL | SDL_SWSURFACE | SDL_FULLSCREEN;
    }

    SDL_Rect** modes = SDL_ListModes(NULL, flags);
    bool create_default_list = 0;

    // no dimension is available
    if (modes == NULL) {
        create_default_list = 1;
    }
    // any dimension is allowed
    else if (modes == (SDL_Rect**)-1) {
        create_default_list = 1;
    }
    else {
        for (int i = 0; modes[i]; ++i) {
            valid_resolutions.push_back(cSize_Int(modes[i]->w, modes[i]->h));
        }
    }

    if (create_default_list) {
        valid_resolutions.push_back(cSize_Int(2048, 1536));
        valid_resolutions.push_back(cSize_Int(1600, 1200));
        valid_resolutions.push_back(cSize_Int(1280, 1024));
        valid_resolutions.push_back(cSize_Int(1024, 768));
        valid_resolutions.push_back(cSize_Int(800, 600));
        valid_resolutions.push_back(cSize_Int(640, 480));
    }

    return valid_resolutions;
}

void cVideo::Make_GL_Context_Current(void)
{
    // scoped context lock here
#ifdef _WIN32
    if (wglGetCurrentContext() != wm_info.hglrc) {
        wglMakeCurrent(GetDC(wm_info.window), wm_info.hglrc);
    }
#elif __unix__
    if (glx_context != NULL) {
        glXMakeCurrent(wm_info.info.x11.gfxdisplay, wm_info.info.x11.window, glx_context);
    }
#elif __APPLE__
    // party time
#endif

    // update info (needed?)
    SDL_GetWMInfo(&wm_info);
}

void cVideo::Make_GL_Context_Inactive(void)
{
#ifdef _WIN32
    wglMakeCurrent(NULL, NULL);
#elif __unix__
    glXMakeCurrent(wm_info.info.x11.gfxdisplay, None, NULL);
#elif __APPLE__
    // party time
#endif

    // update info (needed?)
    SDL_GetWMInfo(&wm_info);
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
        pGuiSystem->renderGUI();

        // update performance timer
        pFramerate->m_perf_timer[PERF_RENDER_GUI]->Update();

        SDL_GL_SwapBuffers();

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

        pGuiSystem->renderGUI();

        // update performance timer
        pFramerate->m_perf_timer[PERF_RENDER_GUI]->Update();

        SDL_GL_SwapBuffers();

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

#ifdef _WIN32
    // windows needs reinitialization
    Init_Video();
#else
    // works only for X11 platforms
    SDL_WM_ToggleFullScreen(screen);
#endif

    // set back clear color
    glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
}

cGL_Surface* cVideo::Get_Surface(fs::path filename, bool print_errors /* = true */)
{
    // .settings file type can't be used directly
    if (filename.extension() == fs::path(".settings"))
        filename.replace_extension(".png");

    // pixmaps dir must be given
    if (!filename.is_absolute())
        filename = pResource_Manager->Get_Game_Pixmaps_Directory() / filename;

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
    filename = fs::absolute(filename, pResource_Manager->Get_Game_Pixmaps_Directory());

    cSoftware_Image software_image = cSoftware_Image();
    SDL_Surface* sdl_surface = NULL;
    cImage_Settings_Data* settings = NULL;

    // load settings if available
    if (load_settings) {
        fs::path settings_file = fs::path(filename);

        // If not already set
        if (settings_file.extension() != fs::path(".settings"))
            settings_file.replace_extension(".settings");

        if (fs::exists(settings_file) && fs::is_regular_file(settings_file)) {
            settings = pSettingsParser->Get(settings_file);

            fs::path img_filename_cache = m_imgcache_dir / fs::relative(pResource_Manager->Get_Game_Data_Directory(), settings_file); // Why add .png here? Should be in the return value of fs::relative() anyway.
            // check if image cache file exists
            if (fs::exists(img_filename_cache) && fs::is_regular_file(img_filename_cache))
                sdl_surface = IMG_Load(path_to_utf8(img_filename_cache).c_str());
            // image given in base settings
            else if (!settings->m_base.empty()) {
                // use current directory
                fs::path img_filename = filename.parent_path() / settings->m_base;

                if (!exists(img_filename)) {
                    // use data dir
                    img_filename = settings->m_base;

                    // pixmaps dir must be given
                    img_filename = fs::absolute(img_filename, pResource_Manager->Get_Game_Pixmaps_Directory());
                }

                sdl_surface = IMG_Load(path_to_utf8(img_filename).c_str());
            }
        }
    }

    // if not set in image settings and file exists
    if (!sdl_surface && exists(filename) && (!settings || settings->m_base.empty())) {
        sdl_surface = IMG_Load(path_to_utf8(filename).c_str());
    }

    if (!sdl_surface) {
        if (settings) {
            delete settings;
            settings = NULL;
        }

        if (print_errors) {
            std::cerr << "Error loading image : " << path_to_utf8(filename) << std::endl << "Reason : " << SDL_GetError() << std::endl;
        }

        return software_image;
    }

    software_image.m_sdl_surface = sdl_surface;
    software_image.m_settings = settings;
    return software_image;
}

cGL_Surface* cVideo::Load_GL_Surface(boost::filesystem::path filename, bool use_settings /* = 1 */, bool print_errors /* = 1 */)
{
    using namespace boost::filesystem;

    // pixmaps dir must be given
    filename = fs::absolute(filename, pResource_Manager->Get_Game_Pixmaps_Directory());

    // load software image
    cSoftware_Image software_image = Load_Image(filename, use_settings, print_errors);
    SDL_Surface* sdl_surface = software_image.m_sdl_surface;
    cImage_Settings_Data* settings = software_image.m_settings;

    // final surface
    cGL_Surface* image = NULL;

    // with settings
    if (settings) {
        // get the size
        cSize_Int size = settings->Get_Surface_Size(sdl_surface);
        Apply_Max_Texture_Size(size.m_width, size.m_height);
        // get basic settings surface
        image = pVideo->Create_Texture(sdl_surface, settings->m_mipmap, size.m_width, size.m_height);
        // apply settings
        settings->Apply(image);
        delete settings;
    }
    // without settings
    else {
        image = Create_Texture(sdl_surface);
    }
    // set filename
    if (image) {
        image->m_path = filename;
    }
    // print error
    else if (print_errors) {
        std::cerr << "Error loading GL surface image : " << path_to_utf8(filename) << std::endl << "Reason : " << SDL_GetError() << std::endl;
    }

    return image;
}

SDL_Surface* cVideo::Convert_To_Final_Software_Image(SDL_Surface* surface) const
{
    // get power of two size
    const unsigned int width = Get_Power_of_2(surface->w);
    const unsigned int height = Get_Power_of_2(surface->h);

    // if it needs to be changed
    if (width != surface->w || height != surface->h || surface->format->BitsPerPixel != 32) {
        // create power of 2 and 32 bits per pixel surface
        SDL_Surface* final = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 32,
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
                             0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
#else
                             0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
#endif

        // set the entire surface alpha to 0
        SDL_SetAlpha(surface, 0, SDL_ALPHA_TRANSPARENT);
        // blit to 32 bit surface
        SDL_BlitSurface(surface, NULL, final, NULL);
        // delete original surface
        SDL_FreeSurface(surface);
        // set new surface
        surface = final;
    }

    return surface;
}

cGL_Surface* cVideo::Create_Texture(SDL_Surface* surface, bool mipmap /* = 0 */, unsigned int force_width /* = 0 */, unsigned int force_height /* = 0 */) const
{
    if (!surface) {
        return NULL;
    }

    // create final image
    surface = Convert_To_Final_Software_Image(surface);

    /* todo : Make this a render request because it forces an early thread render finish as opengl commands are used directly.
     * Reduces performance if the render thread is on. It's usually called from the text rendering in cTimeDisplay::Update.
    */
    pVideo->Render_Finish();

    // create one texture
    GLuint image_num = 0;
    glGenTextures(1, &image_num);

    // if image id is 0 it failed
    if (!image_num) {
        printf("Error : GL image generation failed\n");
        SDL_FreeSurface(surface);
        return NULL;
    }

    // set highest texture id
    if (pImage_Manager->m_high_texture_id < image_num) {
        pImage_Manager->m_high_texture_id = image_num;
    }

    int width = surface->w;
    int height = surface->h;

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
    if (texture_width != surface->w || texture_height != surface->h) {
        int reduce_block_x = surface->w / texture_width;
        int reduce_block_y = surface->h / texture_height;

        // create scaled image
        unsigned char* new_pixels = static_cast<unsigned char*>(SDL_malloc(texture_width * texture_height * 4));
        Downscale_Image(static_cast<unsigned char*>(surface->pixels), surface->w, surface->h, surface->format->BytesPerPixel, new_pixels, reduce_block_x, reduce_block_y);
        SDL_free(surface->pixels);
        surface->pixels = new_pixels;
    }
    // set SDL_image pixel store mode
    else {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, surface->pitch / surface->format->BytesPerPixel);
    }

    // use the generated texture
    glBindTexture(GL_TEXTURE_2D, image_num);

    // set texture wrap modes which control how to interpret texture coordinates
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // set texture magnification function
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // upload to OpenGL texture
    Create_GL_Texture(texture_width, texture_height, surface->pixels, mipmap);

    // unset pixel store mode
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    SDL_FreeSurface(surface);

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
        printf("CreateTexture : GL Error found : %s\n", gluErrorString(error));
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
            pHud_Debug->Set_Text("Screenshot " + int_to_string(i) + _(" saved"), speedfactor_fps * 2.5f);

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
        std::cerr << "Warning: cVideo::Save_Surface : Could not create file " << path_to_utf8(filename) << " for writing" << std::endl;
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
        printf("Warning: cVideo::Save_Surface : %s Unknown bytes per pixel %d\n", filename.c_str(), bpp);
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

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

void Draw_Effect_Out(Effect_Fadeout effect /* = EFFECT_OUT_RANDOM */, float speed /* = 1 */)
{
    if (effect == EFFECT_OUT_RANDOM) {
        effect = static_cast<Effect_Fadeout>((rand() % (EFFECT_OUT_AMOUNT - 1)) + 1);
    }

    switch (effect) {
    case EFFECT_OUT_BLACK: {
        Color color = static_cast<Uint8>(0);

        for (float i = 1; i > 0; i -= (speed / 30) * pFramerate->m_speed_factor) {
            color.alpha = static_cast<Uint8>(45 - (45 * i));

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

        Color color = Color(static_cast<Uint8>(0), 0, 0, 0);
        float alpha = 10.0f;

        while (pos > pos_end * 0.5f) {
            // fade alpha in
            if (alpha < 255.0f) {
                alpha += 10 * pFramerate->m_speed_factor;

                if (alpha > 255.0f) {
                    alpha = 255.0f;
                }

                color.alpha = static_cast<Uint8>(alpha);
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
            image = pVideo->Get_Surface("game/items/star.png");
        }
        else {
            image = pVideo->Get_Surface("game/items/moon_1.png");
        }

        Color color = white;

        while (f < 50) {
            Draw_Game();

            f += 0.9f * pFramerate->m_speed_factor * speed * (f / 7);

            color = Color(static_cast<Uint8>(255 - (f * 4)), 255 - static_cast<Uint8>(f * 4), 255 - static_cast<Uint8>(f * 4), 200 - static_cast<Uint8>(f * 4));

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
            color = Color(0, 0, 0, static_cast<Uint8>(50 + (f * 4)));

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
            rand_color = Color(static_cast<Uint8>(1), 20, 20, 4);
        }
        // green
        else if (rand_color_num == 1) {
            rand_color = Color(static_cast<Uint8>(20), 1, 20, 4);
        }
        // blue
        else if (rand_color_num == 2) {
            rand_color = Color(static_cast<Uint8>(20), 20, 1, 4);
        }
        // yellow
        else {
            rand_color = Color(static_cast<Uint8>(1), 1, 40, 4);
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
                    color.alpha = static_cast<Uint8>(grid[y][x] * 0.4f);

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
            start_color = Color(static_cast<Uint8>(10), 55, 10, 250);
        }
        // blue
        else {
            start_color = Color(static_cast<Uint8>(10), 10, 55, 250);
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
                color.red = static_cast<Uint8>(start_color.red * color_mod);
                color.green = static_cast<Uint8>(start_color.green * color_mod);
                color.blue = static_cast<Uint8>(start_color.blue * color_mod);
                color.alpha = static_cast<Uint8>(rect_size * 0.3f);
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
            rect_color.red = static_cast<Uint8>(start_color.red * 0.1f * color_mod);
            rect_color.green = static_cast<Uint8>(start_color.green * 0.1f * color_mod);
            rect_color.blue = static_cast<Uint8>(start_color.blue * 0.1f * color_mod);
            rect_color.alpha = static_cast<Uint8>(rect_size * 3);
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
        Color color = static_cast<Uint8>(0);

        for (float i = 1; i > 0; i -= (speed / 30) * pFramerate->m_speed_factor) {
            color.alpha = static_cast<Uint8>(255 * i);

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

void Loading_Screen_Init(void)
{
    if (CEGUI::WindowManager::getSingleton().isWindowPresent("loading")) {
        printf("Warning: Loading Screen already initialized.");
        return;
    }

    CEGUI::Window* guisheet = pGuiSystem->getGUISheet();

    // hide all windows
    for (unsigned int i = 0, gui_windows = guisheet->getChildCount(); i < gui_windows; i++) {
        guisheet->getChildAtIdx(i)->hide();
    }

    // Create loading window
    CEGUI::Window* loading_window = CEGUI::WindowManager::getSingleton().loadWindowLayout("loading.layout");
    guisheet->addChildWindow(loading_window);

    // Set license info as translatable string
    CEGUI::Window* license_text = static_cast<CEGUI::Window*>(CEGUI::WindowManager::getSingleton().getWindow("text_gpl"));
    // TRANS: Be careful with the length of this line, if
    // TRANS: it is much longer than the English version,
    // TRANS: it will be cut off.
    license_text->setText(UTF8_("This program is distributed under the terms of the GPLv3"));

    // set info text
    CEGUI::Window* text_default = static_cast<CEGUI::Window*>(CEGUI::WindowManager::getSingleton().getWindow("text_loading"));
    text_default->setText(_("Loading"));
}

void Loading_Screen_Draw_Text(const std::string& str_info /* = "Loading" */)
{
    // set info text
    CEGUI::Window* text_default = static_cast<CEGUI::Window*>(CEGUI::WindowManager::getSingleton().getWindow("text_loading"));
    if (!text_default) {
        printf("Warning: Loading Screen not initialized.");
        return;
    }
    text_default->setText(reinterpret_cast<const CEGUI::utf8*>(str_info.c_str()));

    Loading_Screen_Draw();
}

void Loading_Screen_Draw(void)
{
    // limit fps or vsync will slow down the loading
    if (!Is_Frame_Time(60)) {
        pRenderer->Fake_Render();
        return;
    }

    // clear screen
    pVideo->Clear_Screen();
    pVideo->Draw_Rect(NULL, 0.00001f, &black);

    // Render
    pRenderer->Render();
    pGuiSystem->renderGUI();
    SDL_GL_SwapBuffers();
}

void Loading_Screen_Exit(void)
{
    CEGUI::Window* loading_window = CEGUI::WindowManager::getSingleton().getWindow("loading");

    // loading window is present
    if (loading_window) {
        CEGUI::Window* guisheet = pGuiSystem->getGUISheet();

        // delete loading window
        guisheet->removeChildWindow(loading_window);
        CEGUI::WindowManager::getSingleton().destroyWindow(loading_window);

        // show windows again
        // fixme : this should only show the hidden windows again
        for (unsigned int i = 0, gui_windows = guisheet->getChildCount(); i < gui_windows; i++) {
            guisheet->getChildAtIdx(i)->show();
        }
    }
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

cVideo* pVideo = NULL;

CEGUI::OpenGLRenderer* pGuiRenderer = NULL;
CEGUI::System* pGuiSystem = NULL;

SDL_Surface* screen = NULL;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

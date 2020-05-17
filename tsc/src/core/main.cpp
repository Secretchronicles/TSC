/***************************************************************************
 * main.cpp  -  main routines and initialization
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

#include "../core/global_basic.hpp"
#include "../core/game_core.hpp"
#include "../core/main.hpp"
#include "../core/filesystem/resource_manager.hpp"
#include "../core/filesystem/filesystem.hpp"
#include "../level/level.hpp"
#include "../scene/scene.hpp"
#include "../gui/menu.hpp"
#include "../core/framerate.hpp"
#include "../user/preferences.hpp"
#include "../audio/sound_manager.hpp"
#include "../audio/audio.hpp"
#include "sprite_manager.hpp"
#include "../level/level_settings.hpp"
#include "../level/level_editor.hpp"
#include "../overworld/world_editor.hpp"
#include "../input/joystick.hpp"
#include "../overworld/world_manager.hpp"
#include "../overworld/overworld.hpp"
#include "../campaign/campaign_manager.hpp"
#include "../input/mouse.hpp"
#include "../user/savegame/savegame.hpp"
#include "../input/keyboard.hpp"
#include "../video/renderer.hpp"
#include "../video/loading_screen.hpp"
#include "../video/img_settings.hpp"
#include "../video/img_manager.hpp"
#include "../core/i18n.hpp"
#include "../gui/generic.hpp"
#include "../gui/game_console.hpp"
#include "../gui/debug_window.hpp"

using namespace std;

// TSC namespace is set later to exclude main() from it
using namespace TSC;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

int main(int argc, char** argv)
{
// todo : remove this apple hack
#ifdef __APPLE__
    // dynamic datapath detection for OS X
    // change CWD to point inside bundle so it finds its data (if necessary)
    char path[1024];
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    assert(mainBundle);
    CFURLRef mainBundleURL = CFBundleCopyBundleURL(mainBundle);
    assert(mainBundleURL);
    CFStringRef cfStringRef = CFURLCopyFileSystemPath(mainBundleURL, kCFURLPOSIXPathStyle);
    assert(cfStringRef);
    CFStringGetCString(cfStringRef, path, 1024, kCFStringEncodingASCII);
    CFRelease(mainBundleURL);
    CFRelease(cfStringRef);

    std::string contents = std::string(path) + std::string("/Contents");
    std::string datapath;

    if (contents.find(".app") != std::string::npos) {
        // executable is inside an app bundle, use app bundle-relative paths
        datapath = contents + std::string("/Resources/data/");
    }
    else if (contents.find("/bin") != std::string::npos) {
        // executable is installed Unix-way
        datapath = contents.substr(0, contents.find("/bin")) + "/share/tsc";
    }
    else {
        cerr << "Warning: Could not determine installation type\n";
    }

    if (!datapath.empty()) {
        cout << "setting CWD to " << datapath.c_str() << endl;
        if (chdir(datapath.c_str()) != 0) {
            cerr << "Warning: Failed changing CWD\n";
        }
    }
#endif

    // convert arguments to a vector string
    vector<std::string> arguments(argv, argv + argc);

    if (argc >= 2) {
        for (unsigned int i = 1; i < arguments.size(); i++) {
            // help
            if (arguments[i] == "--help" || arguments[i] == "-h") {
                cout << "Usage: " << arguments[0] << " [OPTIONS]" << endl;
                cout << "Where OPTIONS is one of the following:" << endl;
                cout << "-h, --help\tDisplay this message" << endl;
                cout << "-v, --version\tShow the version of " << CAPTION << endl;
                cout << "-d, --debug\tEnable debug modes with the options : game performance" << endl;
                cout << "-l, --level\tLoad the given level" << endl;
                cout << "-w, --world\tLoad the given world" << endl;
                return EXIT_SUCCESS;
            }
            // version
            else if (arguments[i] == "--version" || arguments[i] == "-v") {
                std::cout << "This is " << CAPTION << " version " << TSC_VERSION_MAJOR << "." << TSC_VERSION_MINOR << "." << TSC_VERSION_PATCH;
#ifdef TSC_VERSION_POSTFIX
                std::cout << "-" << TSC_VERSION_POSTFIX << "." << std::endl;
                std::cout << " --- This is a DEVELOPMENT built! It may eat your hamster! ---" << std::endl;
#else
                std::cout << "." << std::endl;
#endif
#ifdef TSC_VERSION_GIT
                std::cout << "It was compiled from commit " << TSC_VERSION_GIT << "." << std::endl;
#endif
                return EXIT_SUCCESS;
            }
            // debug
            else if (arguments[i] == "--debug" || arguments[i] == "-d") {
                // no value
                if (i + 1 >= arguments.size()) {
                    cerr << arguments[i] << " requires a value" << endl;
                    return EXIT_FAILURE;
                }
                // with value
                else {
                    for (unsigned int option = i + i; i < arguments.size(); i++) {
                        std::string option_str = arguments[option];

                        if (option_str == "game") {
                            game_debug = 1;
                        }
                        else if (option_str == "performance") {
                            game_debug_performance = 1;
                        }
                        else {
                            cerr << "Unknown debug option " << option_str << endl;
                            return EXIT_FAILURE;
                        }
                    }
                }
            }
            // level loading is handled later
            else if (arguments[i] == "--level" || arguments[i] == "-l") {
                // skip
            }
            // world loading is handled later
            else if (arguments[1] == "--world" || arguments[1] == "-w") {
                // skip
            }
            // unknown argument
            else if (arguments[i].substr(0, 1) == "-") {
                cerr << "Unknown argument " << arguments[i] << endl << "Use -h to list all possible arguments" << endl;
                return EXIT_FAILURE;
            }
        }
    }

    do {
        game_reset = false;
        game_exit = false;

        // initialize everything
        Init_Game();

        // command line level entering
        if (argc > 2 && (arguments[1] == "--level" || arguments[1] == "-l") && !arguments[2].empty()) {
            Game_Action = GA_ENTER_LEVEL;
            Game_Mode_Type = MODE_TYPE_LEVEL_CUSTOM;
            Game_Action_Data_Middle.add("load_level", arguments[2]);
        }
        // command line world entering
        else if (argc > 2 && (arguments[1] == "--world" || arguments[1] == "-w") && !arguments[2].empty()) {
            Game_Action = GA_ENTER_WORLD;
            Game_Action_Data_Middle.add("enter_world", arguments[2]);
        }
        // enter main menu
        else {
            Game_Action = GA_ENTER_MENU;
            Game_Action_Data_Middle.add("load_menu", int_to_string(MENU_MAIN));
        }

        Game_Action_Data_Start.add("screen_fadeout", int_to_string(EFFECT_OUT_BLACK));
        Game_Action_Data_Start.add("screen_fadeout_speed", "3");
        Game_Action_Data_End.add("screen_fadein", int_to_string(EFFECT_IN_BLACK));
        Game_Action_Data_End.add("screen_fadein_speed", "3");

        // game loop
#ifndef _DEBUG
        try {
#endif
            while (!game_exit and !game_reset) {
                // update
                Update_Game();
                // draw
                Draw_Game();

                // render
#ifdef TSC_RENDER_THREAD_TEST
                pVideo->Render(1);
#else
                pVideo->Render();
#endif

                // update speedfactor
                pFramerate->Update();
            }
#ifndef _DEBUG
        }
        catch (...) {
            /* Cleanup and exit with non-success status. This is done
             * only in release mode, because the try/catch statement
             * unwinds the stack and confuses GDB. Running GDB's
             * "backtrace" command on an exception re-thrown like this
             * causes it to print the backtrace to the re-throw
             * statement below, which is not useful at all. In debug
             * mode, the original exception needs to terminate the
             * programme so debugging the problem is easier. */
            std::cerr << "Uncought exception. You might want to file a bug; see <https://secretchronicles.org/>." << std::endl;
            Exit_Game();
            throw;
        }
#endif

        Exit_Game();

        // reset should start fresh, so reset level and world
        argc = 0;

    } while (game_reset);
    return EXIT_SUCCESS;
}

// namespace is set here to exclude main() from it
namespace TSC {

void Init_Game(void)
{
    // init random number generator
    srand(static_cast<unsigned int>(time(NULL)));

    // Init Stage 1 - core classes
    debug_print("Initializing resource manager and core classes\n");
    pResource_Manager = new cResource_Manager();
    pVideo = new cVideo();
    pAudio = new cAudio();
    pFramerate = new cFramerate();
    pRenderer = new cRenderQueue(200);
    pRenderer_current = new cRenderQueue(200);
    pImage_Manager = new cImage_Manager();
    pSound_Manager = new cSound_Manager();
    pSettingsParser = new cImage_Settings_Parser();

    // Init Stage 2 - set preferences and init audio and the video screen

    // load user data
    pPreferences = cPreferences::Load_From_File(pResource_Manager->Get_Preferences_File());
    debug_print("Configuration file is '%s'.\n", path_to_utf8(pPreferences->m_config_filename).c_str());

    // set game language
    I18N_Set_Language(pPreferences->m_language);
    // init translation support
    I18N_Init();
    // init user dir directory
    pResource_Manager->Init_User_Directory();
    // framerate init
    pFramerate->Init();
    // audio init
    pAudio->Init();
    // video init
    pVideo->Init_Video();

    debug_print("Loading campaigns\n");
    pCampaign_Manager = new cCampaign_Manager();

    debug_print("Setting up level player\n");
    pLevel_Player = new cLevel_Player(NULL);
    pLevel_Player->m_disallow_managed_delete = 1;
    // set the first active player available
    pActive_Player = pLevel_Player;

    debug_print("Loading levels\n");
    pLevel_Manager = new cLevel_Manager();
    // set the first animation manager available
    pActive_Animation_Manager = pActive_Level->m_animation_manager;
    // set the first active sprite manager available
    pLevel_Player->Set_Sprite_Manager(pActive_Level->m_sprite_manager);

    // apply preferences
    debug_print("Applying preferences\n");
    pPreferences->Apply();

    // draw generic loading screen
    Loading_Screen_Init();
    // initialize image cache
    pVideo->Init_Image_Cache(0);

    // Init Stage 3 - game classes
    gp_hud = new cHud();
    pLevel_Player->Init();

    gp_game_console = new cGame_Console();

    pLevel_Editor = new cEditor_Level();
    pLevel_Editor->Init();

    pWorld_Editor = new cEditor_World();
    pWorld_Editor->Init();

    pMouseCursor = new cMouseCursor(pActive_Level->m_sprite_manager);
    pKeyboard = new cKeyboard();
    pJoystick = new cJoystick();
    pLevel_Manager->Init();
    // note : set any sprite manager as cOverworld_Manager::Load sets it again
    pOverworld_Player = new cOverworld_Player(pActive_Level->m_sprite_manager, NULL);
    pOverworld_Manager = new cOverworld_Manager(pActive_Level->m_sprite_manager);
    gp_debug_window = new cDebug_Window(pActive_Level->m_sprite_manager);
    // set default overworld active
    pOverworld_Player->Set_Overworld(pOverworld_Manager->Get("World 1"));
    pOverworld_Manager->Set_Active("World 1");
    pMenuCore = new cMenuCore();
    pSavegame = new cSavegame();

    // cache
    debug_print("Preloading images and sounds...\n");
    Preload_Images(1);
    Preload_Sounds(1);
    debug_print("Done preloading images and sounds.\n");
    Loading_Screen_Exit();
}

// Note: This function must not throw exceptions! It is called in main()'s
// global try/catch construct's catch{} clause.
void Exit_Game(void)
{
    if (pPreferences) {
        pPreferences->Save();
    }

    pLevel_Manager->Unload();
    pMenuCore->m_handler->m_level->Unload();

    if (pAudio) {
        delete pAudio;
        pAudio = NULL;
    }

    if (pLevel_Player) {
        delete pLevel_Player;
        pLevel_Player = NULL;
    }

    if (gp_hud) {
        delete gp_hud;
        gp_hud = NULL;
    }

    if (pSound_Manager) {
        delete pSound_Manager;
        pSound_Manager = NULL;
    }

    if (pLevel_Editor) {
        delete pLevel_Editor;
        pLevel_Editor = NULL;
    }

    if (pWorld_Editor) {
        delete pWorld_Editor;
        pWorld_Editor = NULL;
    }

    if (gp_debug_window) {
        delete gp_debug_window;
        gp_debug_window = NULL;
    }

    if (gp_game_console) {
        delete gp_game_console;
        gp_game_console = NULL;
    }

    if (pPreferences) {
        delete pPreferences;
        pPreferences = NULL;
    }

    if (pSavegame) {
        delete pSavegame;
        pSavegame = NULL;
    }

    if (pMouseCursor) {
        delete pMouseCursor;
        pMouseCursor = NULL;
    }

    if (pJoystick) {
        delete pJoystick;
        pJoystick = NULL;
    }

    if (pKeyboard) {
        delete pKeyboard;
        pKeyboard = NULL;
    }

    if (pCampaign_Manager) {
        delete pCampaign_Manager;
        pCampaign_Manager = NULL;
    }

    if (pOverworld_Manager) {
        delete pOverworld_Manager;
        pOverworld_Manager = NULL;
    }

    if (pOverworld_Player) {
        delete pOverworld_Player;
        pOverworld_Player = NULL;
    }

    if (pLevel_Manager) {
        delete pLevel_Manager;
        pLevel_Manager = NULL;
    }

    if (pMenuCore) {
        delete pMenuCore;
        pMenuCore = NULL;
    }

    if (pRenderer) {
        delete pRenderer;
        pRenderer = NULL;
    }

    if (pRenderer_current) {
        delete pRenderer_current;
        pRenderer_current = NULL;
    }

    if (pVideo) {
        delete pVideo;
        pVideo = NULL;
    }

    if (pImage_Manager) {
        delete pImage_Manager;
        pImage_Manager = NULL;
    }

    if (pSettingsParser) {
        delete pSettingsParser;
        pSettingsParser = NULL;
    }

    if (pResource_Manager) {
        delete pResource_Manager;
        pResource_Manager = NULL;
    }
}

bool Handle_Input_Global(const sf::Event& ev)
{
    switch (ev.type) {
    case sf::Event::Closed: {
        game_exit = 1;
        Clear_Input_Events();

        // handle on all handlers ?
        return 0;
    }
    case sf::Event::Resized: {
        CEGUI::System::getSingleton().notifyDisplaySizeChanged(CEGUI::Sizef(static_cast<float>(ev.size.width), static_cast<float>(ev.size.height)));
        break;
    }
    case sf::Event::TextEntered: {
        if (pKeyboard->Text_Entered(ev)) {
            return 1;
        }
        break;
    }
    case sf::Event::KeyPressed: {
        if (pKeyboard->Key_Down(ev)) {
            return 1;
        }
        break;
    }
    case sf::Event::KeyReleased: {
        if (pKeyboard->Key_Up(ev)) {
            return 1;
        }
        break;
    }
    case sf::Event::JoystickButtonPressed: {
        if (pJoystick->Handle_Button_Down_Event(ev)) {
            return 1;
        }
        break;
    }
    case sf::Event::JoystickButtonReleased: {
        if (pJoystick->Handle_Button_Up_Event(ev)) {
            return 1;
        }
        break;
    }
    case sf::Event::JoystickMoved: {
        pJoystick->Handle_Motion(ev);
        break;
    }
    case sf::Event::LostFocus: {
        // lost visibility
        bool music_paused = false;
        // pause music
        if (pAudio->Is_Music_Playing()) {
            pAudio->Pause_Music();
            music_paused = true;
        }

        // Wait until we get focus again. This “freezes” the
        // game instead of updating it further.
        sf::Event focusin_event;
        while (true) {
            pVideo->WaitEvent(focusin_event);
            if (focusin_event.type == sf::Event::GainedFocus) {
                break;
            }
            else if (focusin_event.type == sf::Event::Closed) {
                // Window was closed while it didn't have the focus; exit game now
                game_exit = 1;
                Clear_Input_Events();
                return 0;
            }
        }
        // resume if music got paused
        if (music_paused) {
            pAudio->Resume_Music();
        }
        return 1;

    }
    default: { // other events (event type filters in the respective subhandlers)
        // mouse
        if (pMouseCursor->Handle_Event(ev)) {
            return 1;
        }

        // send events
        if (Game_Mode == MODE_MENU) {
            if (pMenuCore->Handle_Event(ev)) {
                return 1;
            }
        }
        break;
    }
    }

    return 0;
}

void Update_Game(void)
{
    // do not update if exiting
    if (game_exit) {
        return;
    }

    // if in menu and vsync is disabled then limit the fps to reduce the load for CPU/GPU
    if (Game_Mode == MODE_MENU && !pPreferences->m_video_vsync) {
        Correct_Frame_Time(100);
    }
    // if fps limit is set
    else if (pPreferences->m_video_fps_limit) {
        Correct_Frame_Time(pPreferences->m_video_fps_limit);
    }

    if (Game_Action != GA_NONE) {
        pVideo->Render_Finish();
    }

    // ## game events
    Handle_Game_Events();

    // ## input
    // Actually `input_event' is a global variable that is also queried elsewhere
    // in the code (uaaah, poor design).
    while (pVideo->PollEvent(input_event)) {
        // handle
        Handle_Input_Global(input_event);
    }

    pMouseCursor->Update();

    // ## audio
    pAudio->Resume_Music();
    pAudio->Update();

    // performance measuring
    pFramerate->m_perf_last_ticks = TSC_GetTicks();

    // ## hud
    gp_hud->Update();

    // ## game console
    gp_game_console->Update();

    // ## debug window
    gp_debug_window->Update();

    // ## update
    if (Game_Mode == MODE_LEVEL) {
        pLevel_Manager->Update();
    }
    else if (Game_Mode == MODE_OVERWORLD) {
        pActive_Overworld->Update();
    }
    else if (Game_Mode == MODE_MENU) {
        pMenuCore->Update();
    }
    else if (Game_Mode == MODE_LEVEL_SETTINGS) {
        pLevel_Editor->m_settings_screen.Update();
    }
    else if (Game_Mode == MODE_SCENE) {
        pActive_Scene->Update();
    }

    // gui
    Gui_Handle_Time();
}

void Draw_Game(void)
{
    // don't draw if exiting
    if (game_exit) {
        return;
    }

    // performance measuring
    pFramerate->m_perf_last_ticks = TSC_GetTicks();

    if (Game_Mode == MODE_LEVEL) {
        pLevel_Manager->Draw();
    }
    else if (Game_Mode == MODE_OVERWORLD) {
        pActive_Overworld->Draw();
    }
    else if (Game_Mode == MODE_MENU) {
        pMenuCore->Draw();
    }
    else if (Game_Mode == MODE_LEVEL_SETTINGS) {
        pLevel_Editor->m_settings_screen.Draw();
    }
    else if (Game_Mode == MODE_SCENE) {
        pActive_Scene->Draw();
    }

    // Mouse
    pMouseCursor->Draw();

    // update performance timer
    pFramerate->m_perf_timer[PERF_DRAW_MOUSE]->Update();
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

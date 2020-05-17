/***************************************************************************
 * global_game.h
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

#ifndef TSC_GLOBAL_GAME_HPP
#define TSC_GLOBAL_GAME_HPP
#include "global_basic.hpp"

namespace TSC {

    /* Turns the version numbers into a numeric value:
     * (1,2,3) -> (10203)
     * Assumes that there will never be more than 100 minor or patch versions
     */
#define TSC_VERSION_NUM(X, Y, Z) ((X)*10000 + (Y)*100 + (Z))

// Caption
#define CAPTION "The Secret Chronicles of Dr. M."
#define TSC_GETTEXT_DOMAIN "Secretchronicles"

    static const unsigned int tsc_version = TSC_VERSION_NUM(TSC_VERSION_MAJOR, TSC_VERSION_MINOR, TSC_VERSION_PATCH);

    /* *** *** *** *** *** *** *** Object Direction *** *** *** *** *** *** *** *** *** *** */

    enum ObjectDirection {
        // undefined
        DIR_UNDEFINED   = -1,

        // default
        DIR_LEFT    = 0,
        DIR_RIGHT   = 1,
        DIR_UP      = 2,
        DIR_DOWN    = 3,

        DIR_TOP     = 2,
        DIR_BOTTOM  = 3,

        // multi
        DIR_TOP_LEFT = 4,
        DIR_TOP_RIGHT = 5,
        DIR_BOTTOM_LEFT = 6,
        DIR_BOTTOM_RIGHT = 7,
        DIR_LEFT_TOP = 21,
        DIR_LEFT_BOTTOM = 22,
        DIR_RIGHT_TOP = 23,
        DIR_RIGHT_BOTTOM = 24,

        DIR_UP_LEFT = 4,
        DIR_UP_RIGHT = 5,
        DIR_DOWN_LEFT = 6,
        DIR_DOWN_RIGHT = 7,
        DIR_LEFT_UP = 21,
        DIR_LEFT_DOWN = 22,
        DIR_RIGHT_UP = 23,
        DIR_RIGHT_DOWN = 24,

        // extra
        DIR_HORIZONTAL  = 10,   // left or right
        DIR_VERTICAL    = 11,   // up or down
        DIR_ALL         = 20,   // all directions

        // special
        DIR_FIRST       = 100,  // Overworld first waypoint
        DIR_LAST        = 101   // Overworld last waypoint
    };

    /* *** *** *** *** *** *** *** Default Color *** *** *** *** *** *** *** *** *** *** */

    enum DefaultColor {
        COL_DEFAULT = -1,
        COL_WHITE   = 0,
        COL_BLACK   = 1,
        COL_RED     = 2,
        COL_ORANGE  = 3,
        COL_YELLOW  = 4,
        COL_GREEN   = 5,
        COL_BLUE    = 6,
        COL_BROWN   = 7,
        COL_GREY    = 8,
        COL_VIOLET  = 9
    };

    /* *** *** *** *** *** *** *** Game Mode *** *** *** *** *** *** *** *** *** *** */

    enum GameMode {
        MODE_NOTHING        = 0,
        MODE_LEVEL          = 1,
        MODE_OVERWORLD      = 2,
        MODE_MENU           = 3,
        MODE_LEVEL_SETTINGS = 4,
        MODE_SCENE          = 5
    };

    enum GameModeType {
        MODE_TYPE_DEFAULT       = 0,
        MODE_TYPE_LEVEL_CUSTOM  = 1
    };

    /* *** *** *** *** *** *** *** Game Action *** *** *** *** *** *** *** *** *** *** */

    enum GameAction {
        GA_NONE             = 0,
        GA_ENTER_MENU       = 1,
        GA_ENTER_WORLD      = 2,
        GA_ENTER_LEVEL      = 3,
        GA_DOWNGRADE_PLAYER = 4,
        GA_ACTIVATE_LEVEL_EXIT  = 5,
        GA_ENTER_LEVEL_SETTINGS = 6,
        GA_RESET                = 7,
        GA_ENTER_SCENE          = 8
    };

    /* *** *** *** *** *** Level draw type *** *** *** *** *** *** *** *** *** *** *** *** */

    enum LevelDrawType {
        LVL_DRAW        = 0,    // only draw
        LVL_DRAW_NO_BG  = 1,    // only draws and without background gradient and image
        LVL_DRAW_BG     = 2     // only draws the background gradient and image
    };

    /* *** *** *** *** *** Level land type *** *** *** *** *** *** *** *** *** *** *** *** */

    enum LevelLandType {
        LLT_UNDEFINED = 0,
        LLT_GREEN = 1,
        LLT_JUNGLE = 2,
        LLT_ICE = 3,
        LLT_SNOW = 4,
        LLT_WATER = 5,
        LLT_CANDY = 6,
        LLT_DESERT = 7,
        LLT_SAND = 8,
        LLT_CASTLE = 9,
        LLT_UNDERGROUND = 10,
        LLT_CRYSTAL = 11,
        LLT_GHOST = 12,
        LLT_MUSHROOM = 13,
        LLT_SKY = 14,
        LLT_PLASTIC = 15,
        LLT_LAST = 16
    };

    /* *** *** *** *** *** *** *** Paths *** *** *** *** *** *** *** *** *** *** */

// Core
#define GAME_OVERWORLD_DIR "worlds"
#define GAME_LEVEL_DIR "levels"
#define GAME_CAMPAIGN_DIR "campaigns"
#define GAME_PIXMAPS_DIR "pixmaps"
#define GAME_SOUNDS_DIR "sounds"
#define GAME_MUSIC_DIR "music"
#define GAME_EDITOR_DIR "editor"
#define GAME_ICON_DIR "icon"
#define GAME_SCHEMA_DIR "schema"
#define GAME_TRANSLATION_DIR "translations"
#define GAME_SCRIPTING_DIR "scripting"
// GUI
#define GUI_SCHEME_DIR "gui/schemes"
#define GUI_IMAGESET_DIR "gui/imagesets"
#define GUI_FONT_DIR "gui/fonts"
#define GUI_LAYOUT_DIR "gui/layouts"
#define GUI_LOOKNFEEL_DIR "gui/looknfeels"
#define GUI_XMLSCHEMA_DIR "gui/xmlschemas"
// User
#define USER_SAVEGAME_DIR "savegames"
#define USER_SCREENSHOT_DIR "screenshots"
#define USER_LEVEL_DIR "levels"
#define USER_WORLD_DIR "worlds"
#define USER_CAMPAIGN_DIR "campaigns"
#define USER_IMGCACHE_DIR "images"
#define USER_SCRIPTING_DIR "scripting"

    /* *** *** *** *** *** *** *** forward declarations *** *** *** *** *** *** *** *** *** *** */

// Allows use of pointers in header files without including individual headers
// which decreases dependencies between files

    /* *** speedfactor framerate *** */
    static const int speedfactor_fps = 32;

    /* *** level engine version *** */
    static const int level_engine_version = 48;
    /* *** world engine version *** */
    static const int world_engine_version = 6;

    /* *** Sprite Types *** */

    enum SpriteType {
        TYPE_UNDEFINED = 0,
        // global
        TYPE_SPRITE = 1,
        TYPE_ENEMY = 2,
        TYPE_PLAYER = 3,
        TYPE_ACTIVE_SPRITE = 4,
        // game
        TYPE_MOUSECURSOR = 100,
        // overworld
        TYPE_OW_WAYPOINT = 55,
        TYPE_OW_LINE_START = 57,
        TYPE_OW_LINE_END = 58,
        // level
        TYPE_LEVEL_EXIT = 18,
        TYPE_LEVEL_ENTRY = 54,
        TYPE_ENEMY_STOPPER = 20,
        TYPE_BONUS_BOX = 26,
        TYPE_SPIN_BOX = 27,
        TYPE_TEXT_BOX = 59,
        TYPE_MOVING_PLATFORM = 38,
        TYPE_CRATE = 71,
        TYPE_SECRET_AREA = 74,
        // enemy
        TYPE_FURBALL = 10,
        TYPE_FURBALL_BOSS = 62,
        TYPE_ARMY = 19,
        TYPE_TURTLE_BOSS = 56,
        TYPE_FLYON = 29,
        TYPE_ROKKO = 30,
        TYPE_KRUSH = 36,
        TYPE_THROMP = 41,
        TYPE_EATO = 42,
        TYPE_GEE = 43,
        TYPE_SPIKA = 31,
        TYPE_STATIC_ENEMY = 50,
        TYPE_SPIKEBALL = 64,
        TYPE_PIP = 67,
        TYPE_BEETLE_BARRAGE = 68,
        TYPE_BEETLE = 69,
        TYPE_SHELL = 70,
        TYPE_LARRY = 72,
        TYPE_DOOM_LARRY = 73,
        // items
        TYPE_POWERUP = 23,
        TYPE_MUSHROOM_DEFAULT = 25,
        TYPE_MUSHROOM_LIVE_1 = 35,
        TYPE_MUSHROOM_POISON = 49,
        TYPE_MUSHROOM_BLUE = 51,
        TYPE_MUSHROOM_GHOST = 52,
        TYPE_FIREPLANT = 24,
        TYPE_JUMPING_GOLDPIECE = 22,
        TYPE_FALLING_GOLDPIECE = 48,
        TYPE_GOLDPIECE = 8,
        TYPE_MOON = 37,
        TYPE_STAR = 39,
        // special
        TYPE_BALL = 28,
        TYPE_SOUND = 60,
        TYPE_ANIMATION = 61,
        TYPE_PARTICLE_EMITTER = 65,
        TYPE_PATH = 63,
        // HUD
        TYPE_HUD_POINTS = 12,
        TYPE_HUD_TIME = 13,
        TYPE_HUD_DEBUG = 14,
        TYPE_HUD_LIFE = 15,
        TYPE_HUD_GOLD = 16,
        TYPE_HUD_BACKGROUND = 17,
        TYPE_HUD_ITEMBOX = 32,
        TYPE_HUD_INFOMESSAGE = 66
    };

    /* *** Massive Types *** */
// Massive types primaryly define the Z coordinate of a sprite.
// Collision checking is done with the array types below.

    enum MassiveType {
        MASS_PASSIVE = 0,
        MASS_MASSIVE = 1,
        MASS_HALFMASSIVE = 2,
        MASS_CLIMBABLE = 3,
        MASS_FRONT_PASSIVE = 4
    };

    /* *** Ground Types *** */

    enum GroundType {
        GROUND_NORMAL = 0,
        GROUND_EARTH = 1,
        GROUND_ICE = 2,
        GROUND_SAND = 3,
        GROUND_STONE = 4,
        GROUND_PLASTIC = 5
    };

    /* *** Array Types *** */
// Array types define how an object behaves on collisions.

    enum ArrayType {
        ARRAY_UNDEFINED = 0,
        // normal blocking object (level default)
        ARRAY_MASSIVE = 1,
        // normal passive object
        ARRAY_PASSIVE = 2,
        // enemy
        ARRAY_ENEMY = 3,
        // special object
        ARRAY_ACTIVE = 4,
        // hud
        ARRAY_HUD = 5,
        // animation
        ARRAY_ANIM = 6,
        // player
        ARRAY_PLAYER = 7,
        // lava
        ARRAY_LAVA = 8
    };

    /* *** collision validation types *** */

    enum Col_Valid_Type {
        // not valid
        COL_VTYPE_NOT_VALID = 0,
        // internal
        COL_VTYPE_INTERNAL = 1,
        // blocking
        COL_VTYPE_BLOCKING = 2,
        // no validation possible for a sub-function
        COL_VTYPE_NOT_POSSIBLE = 3
    };

    /* *** Input identifier *** */

    enum input_identifier {
        INP_UNKNOWN = 0,
        INP_UP = 1,
        INP_DOWN = 2,
        INP_LEFT = 3,
        INP_RIGHT = 4,
        INP_JUMP = 5,
        INP_SHOOT = 6,
        INP_ACTION = 7,
        // Request Item
        INP_ITEM = 8,
        // General Exit/Leave/Cancel
        INP_EXIT = 9
    };

    /* *** Menu Types *** */

    enum MenuID {
        MENU_NOTHING = 0,
        MENU_MAIN = 1,
        MENU_START = 5,
        MENU_OPTIONS = 2,
        MENU_LOAD = 3,
        MENU_SAVE = 4,
        MENU_CREDITS = 6
    };

    /* *** Ball Effect types *** */

    enum ball_effect {
        FIREBALL_DEFAULT = 1,
        FIREBALL_EXPLOSION = 2,
        ICEBALL_DEFAULT = 3,
        ICEBALL_EXPLOSION = 4
    };

    /* *** Performance timer types *** */

    enum performance_timer_type {
        // update
        PERF_UPDATE_PROCESS_INPUT = 0,
        PERF_UPDATE_LEVEL = 1,
        PERF_UPDATE_LEVEL_EDITOR = 2,
        PERF_UPDATE_HUD = 3,
        PERF_UPDATE_PLAYER = 4,
        PERF_UPDATE_PLAYER_COLLISIONS = 23,
        PERF_UPDATE_LATE_LEVEL = 22,
        PERF_UPDATE_LEVEL_COLLISIONS = 5,
        PERF_UPDATE_CAMERA = 6,
        // update overworld
        PERF_UPDATE_OVERWORLD = 17,
        // update menu
        PERF_UPDATE_MENU = 18,
        // update level settings
        PERF_UPDATE_LEVEL_SETTINGS = 19,
        // draw level
        PERF_DRAW_LEVEL_LAYER1 = 7,
        PERF_DRAW_LEVEL_PLAYER = 8,
        PERF_DRAW_LEVEL_LAYER2 = 9,
        PERF_DRAW_LEVEL_HUD = 10,
        PERF_DRAW_LEVEL_EDITOR = 11,
        // draw overworld
        PERF_DRAW_OVERWORLD = 16,
        // draw menu
        PERF_DRAW_MENU = 14,
        // draw level settings
        PERF_DRAW_LEVEL_SETTINGS = 15,
        // draw
        PERF_DRAW_MOUSE = 12,
        // rendering
        PERF_RENDER_GAME = 13,
        PERF_RENDER_GUI = 20,
        PERF_RENDER_BUFFER = 21
    };

    /* *** Classes *** */

    class cCamera;
    class cCircle_Request;
    class cEditor_Object_Settings_Item;
    class cGL_Surface;
    class cGradient_Request;
    class cImage_Settings_Data;
    class cLayer_Line_Point_Start;
    class cLevel;
    class cLine_collision;
    class cLine_Request;
    class cLevel_Settings;
    class cScene;
    class cMenu_Base;
    class cObjectCollisionType;
    class cObjectCollision;
    class cOverworld;
    class cOverworld_Player;
    class cParticle_Emitter;
    class cPath;
    class cPath_State;
    class cRect_Request;
    class cSave_Level_Object;
    class cSaved_Texture;
    class cSize_Float;
    class cSize_Int;
    class cSprite_Manager;
    class cSurface_Request;
    class cSprite;
    class cBackground_Manager;
    class cWorld_Sprite_Manager;
    class Color;
    class GL_rect;
    class GL_line;
    class GL_point;
    class cLevelLoader;
    class XmlAttributes;

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

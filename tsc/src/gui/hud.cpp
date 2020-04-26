#include "../core/global_basic.hpp"
#include "../core/global_game.hpp"
#include "../core/game_core.hpp"
#include "../video/color.hpp"
#include "../objects/sprite.hpp"
#include "../objects/movingsprite.hpp"
#include "../core/i18n.hpp"
#include "../audio/audio.hpp"
#include "../video/gl_surface.hpp"
#include "../level/level.hpp"
#include "../level/level_player.hpp"
#include "../objects/powerup.hpp"
#include "../objects/rescue_item.hpp"
#include "../scripting/events/gold_100_event.hpp"
#include "../core/property_helper.hpp"
#include "../core/framerate.hpp"
#include "../core/filesystem/resource_manager.hpp"
#include "../core/sprite_manager.hpp"
#include "hud.hpp"

// 35 is the number of pixels set in berry's .settings file.
#define BERRY_WIDTH  35 * global_upscalex
#define BERRY_HEIGHT 35 * global_upscaley

// 48px is just slightly bigger.
#define ITEMBOX_WIDTH 48 * global_upscalex
#define ITEMBOX_HEIGHT 48 * global_upscaley

// Sizing found by trial&error.
#define JEWEL_WIDTH 16 * global_upscalex
#define JEWEL_HEIGHT 16 * global_upscaley

// Sizing found by trial&error.
#define ALEX_HEAD_WIDTH 48 * global_upscalex
#define ALEX_HEAD_HEIGHT 24 * global_upscaley

// Number of seconds to display HUD messages, times the speedfactor
#define TEXT_DISPLAY_TIME 2.0f * speedfactor_fps

// Number of seconds to display the mini points (the points shown
// e.g. next to a killed enemy), times the speedfactor.
#define MINIPOINTS_DISPLAY_TIME 2.0f * speedfactor_fps

// Number of lives the player has at start.
#define PLAYER_DEFAULT_LIVES 3

// extern variables
TSC::cHud* TSC::gp_hud = NULL;

using namespace TSC;
namespace fs = boost::filesystem;

cHud::cHud()
    : m_points(0), m_jewels(0), m_lives(PLAYER_DEFAULT_LIVES),
      mp_display_item(NULL), m_rescue_item_type(TYPE_UNDEFINED),
      m_elapsed_time(0), m_last_time(std::chrono::system_clock::now()),
      m_text_counter(0.0f), mp_hud_root(NULL), mp_points_label(NULL), mp_time_label(NULL),
      mp_jewels_label(NULL), mp_lives_label(NULL),
      mp_waypoint_label(NULL), mp_world_label(NULL), mp_message_text(NULL), mp_item_image(NULL)
{
    load_hud_images_into_cegui();

    mp_hud_root = static_cast<CEGUI::FrameWindow*>(
        CEGUI::WindowManager::getSingleton()
        .loadLayoutFromFile("hud.layout"));

    mp_points_label   = mp_hud_root->getChild("points");
    mp_time_label     = mp_hud_root->getChild("time");
    mp_jewels_label   = mp_hud_root->getChild("jewels");
    mp_lives_label    = mp_hud_root->getChild("lives");
    mp_waypoint_label = mp_hud_root->getChild("world_waypoint");
    mp_world_label    = mp_hud_root->getChild("world_name");
    mp_message_text   = mp_hud_root->getChild("message");
    mp_item_image     = mp_hud_root->getChild("itembox_image/item_image");

    mp_hud_root->hide();
    CEGUI::System::getSingleton()
        .getDefaultGUIContext()
        .getRootWindow()
        ->addChild(mp_hud_root);

    // Hide by default
    mp_message_text->hide();
    mp_item_image->hide();

    Screen_Size_Changed();

    // Set initial values. Using methods rather than bare assignment
    // so translations can kick in for the HUD elements.
    Set_Points(0);
    Set_Jewels(0);
    Set_Elapsed_Time(0);
    Set_Lives(PLAYER_DEFAULT_LIVES);
}

cHud::~cHud()
{
    // Delete all instances of unfinished minipoints
    std::vector<cMiniPoints*>::iterator iter;
    for(iter=m_active_mini_points.begin(); iter != m_active_mini_points.end(); iter++) {
        delete (*iter);
    }
    m_active_mini_points.clear();

    CEGUI::System::getSingleton()
        .getDefaultGUIContext()
        .getRootWindow()
        ->removeChild(mp_hud_root);

    CEGUI::WindowManager::getSingleton().destroyWindow(mp_hud_root);
}

void cHud::Show()
{
    mp_hud_root->show();

    // Show/Hide the mode-specific widgets.
    if (Game_Mode == MODE_LEVEL) {
        mp_time_label->show();
        mp_hud_root->getChild("itembox_image")->show();
        mp_waypoint_label->hide();
        mp_world_label->hide();
    }
    else if (Game_Mode == MODE_OVERWORLD) {
        mp_time_label->hide();
        mp_hud_root->getChild("itembox_image")->hide();
        mp_waypoint_label->show();
        mp_world_label->show();
    }
}

void cHud::Hide()
{
    mp_hud_root->hide();
}

void cHud::Update()
{
    static char timestr[32];
    static int seconds;

    // Update elapsed time
    if (Game_Mode == MODE_LEVEL) {
        // Do not add to elapsed time while in editor. m_last_time
        // still needs updating, because the subtraction in `else'
        // below would otherwise add the in-editor time in one big step.
        if (editor_level_enabled) {
            m_last_time = std::chrono::system_clock::now();
        }
        else {
            // Increase elapsed time by taking the differentw to m_last_time.
            std::chrono::system_clock::time_point time_now = std::chrono::system_clock::now();
            std::chrono::milliseconds time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_now - m_last_time);

            m_elapsed_time += time_elapsed.count();
            m_last_time     = time_now;
            seconds         = m_elapsed_time / 1000;

            sprintf(timestr, _("Time %02d:%02d"), seconds / 60, seconds % 60);
            mp_time_label->setText(timestr);
        }
    }

    // Update all minipoints
    std::vector<cMiniPoints*>::iterator iter;
    for(iter=m_active_mini_points.begin(); iter != m_active_mini_points.end();) {
        if ((*iter)->Update()) {
            // If they're done displaying, delete them and remove them from the vector.
            delete (*iter);
            iter = m_active_mini_points.erase(iter);
        }
        else {
            iter++;
        }
    }

    // Update text counter if a message is displayed
    if (mp_message_text->isVisible()) {
        m_text_counter -= pFramerate->m_speed_factor;
        if (m_text_counter <= 0) {
            mp_message_text->hide();
            m_text_counter = 0;
        }
    }
}

void cHud::Set_Points(long points)
{
    m_points = points;

    char str[32];
    memset(str, '\0', 32);
    sprintf(str, _("Points %08ld"), m_points);
    mp_points_label->setText(str);
}

void cHud::Add_Points(long points, float x /* = 0.0f */, float y /* = 0.0f */, std::string strtext /* = "" */, const Color& color /* = 255 */, bool allow_multiplier /* = false */)
{
    if (allow_multiplier) {
        points = static_cast<unsigned int>(pLevel_Player->m_kill_multiplier * static_cast<float>(points));
    }

    Set_Points(m_points + points);

    if (Is_Float_Equal(x, 0.0f) || Is_Float_Equal(y, 0.0f) || m_active_mini_points.size() > 50) {
        return;
    }

    // if empty set the points as text
    if (strtext.empty()) {
        strtext = int_to_string(points);
    }

    m_active_mini_points.push_back(new cMiniPoints(strtext, x, y, color));
}

void cHud::Reset_Points()
{
    Set_Points(0);
}

long cHud::Get_Points()
{
    return m_points;
}

void cHud::Set_Jewels(int jewels)
{
    m_jewels = jewels;

    while (m_jewels >= 100) {
        m_jewels -= 100;
        pAudio->Play_Sound("item/live_up_2.ogg", RID_1UP_MUSHROOM);
        Add_Lives(1);

        // Display "1UP"
        Add_Points(0, pLevel_Player->m_pos_x + pLevel_Player->m_image->m_w/3, pLevel_Player->m_pos_y + 5, "1UP", lightred);

        // Fire the Gold100 event
        Scripting::cGold_100_Event evt;
        evt.Fire(pActive_Level->m_mruby, pLevel_Player);
    }

    char str[8];
    memset(str, '\0', 8);
    sprintf(str, "%02d", m_jewels);
    mp_jewels_label->setText(str);

    // Change text colour with more and more jewels collected
    // OLD Color color = Color(static_cast<uint8_t>(255), 255, 255 - (gold * 2));
}

void cHud::Add_Jewels(int jewels)
{
    Set_Jewels(m_jewels + jewels);
}

void cHud::Reset_Jewels()
{
    Set_Jewels(0);
}

int cHud::Get_Jewels()
{
    return m_jewels;
}

void cHud::Set_Lives(int lives)
{
    m_lives = lives;

    if (m_lives > 99)
        m_lives = 99;

    char str[32];
    memset(str, '\0', 32);
    sprintf(str, "[colour='FF00FF00']%02d x", m_lives);
    mp_lives_label->setText(str);
}

void cHud::Add_Lives(int lives)
{
    Set_Lives(m_lives + lives);
}

void cHud::Reset_Lives()
{
    Set_Lives(PLAYER_DEFAULT_LIVES);
}

int cHud::Get_Lives()
{
    return m_lives;
}

void cHud::Set_Elapsed_Time(uint32_t milliseconds)
{
    m_elapsed_time = milliseconds;
}

void cHud::Reset_Elapsed_Time()
{
    m_elapsed_time = 0;
    m_last_time = std::chrono::system_clock::now();
}

uint32_t cHud::Get_Elapsed_Time()
{
    return m_elapsed_time;
}

void cHud::Set_Item(SpriteType item_type, bool sound /* = true */)
{
    // play sound (savegame load is not in MODE_LEVEL)
    if (Game_Mode == MODE_LEVEL && sound) {
        pAudio->Play_Sound("itembox_set.ogg");
    }

    m_rescue_item_type = item_type;

    switch(m_rescue_item_type) {
    case TYPE_MUSHROOM_DEFAULT:
        mp_item_image->setProperty("Image", m_normal_berry_img);
        break;
    case TYPE_MUSHROOM_BLUE:
        mp_item_image->setProperty("Image", m_ice_berry_img);
        break;
    case TYPE_FIREPLANT:
        mp_item_image->setProperty("Image", m_fire_berry_img);
        break;
    case TYPE_UNDEFINED: // Request to empty the item box
        mp_item_image->setProperty("Image", "");
        break;
    default:
        std::cerr << "Warning: Unsupported item type stored in HUD item box, changing to normal berry" << std::endl;
        m_rescue_item_type = TYPE_MUSHROOM_DEFAULT;
        mp_item_image->setProperty("Image", m_normal_berry_img);
        break;
    }

    mp_item_image->show();
}

void cHud::Request_Item(void)
{
    if (Game_Mode != MODE_LEVEL)
        throw(std::runtime_error("cHud::Reqeust_Item() may only be called in level mode!"));

    // Currently no rescue item
    if (m_rescue_item_type == TYPE_UNDEFINED)
        return;

    GL_rect cam_rect = pLevel_Manager->m_camera->Get_Rect();

    // Spawn rescue item
    cRescueItem* p_item = new cRescueItem(pActive_Level->m_sprite_manager);
    p_item->Set_Item_Type(m_rescue_item_type);
    p_item->Set_Pos(cam_rect.m_x + (cam_rect.m_w / 2.0f) - (BERRY_WIDTH / 2.0f),
                    cam_rect.m_y);
    pActive_Level->m_sprite_manager->Add(p_item);

    pAudio->Play_Sound("itembox_get.ogg");

    // Clear rescue item
    m_rescue_item_type = TYPE_UNDEFINED;
    mp_item_image->hide();
}

void cHud::Reset_Item(void)
{
    m_rescue_item_type = TYPE_UNDEFINED;
    mp_item_image->hide();
}

SpriteType cHud::Get_Item(void)
{
    return m_rescue_item_type;
}

void cHud::Reset()
{
    Reset_Points();
    Reset_Jewels();
    Reset_Lives();
    Reset_Elapsed_Time();
    Reset_Item();
}

void cHud::Set_Waypoint_Name(std::string name, Color color)
{
    // TODO: Apply color
    mp_waypoint_label->setText(name);
}

void cHud::Set_World_Name(std::string name)
{
    mp_world_label->setText(std::string("[colour='FFFFFF00']") + name);
}

void cHud::Set_Text(std::string message)
{
    mp_message_text->setText(reinterpret_cast<const CEGUI::utf8*>(message.c_str())); // Caller has to translate it
    mp_message_text->show();
    m_text_counter = TEXT_DISPLAY_TIME;
}

void cHud::load_hud_images_into_cegui()
{
    CEGUI::ImageManager& imgmanager = CEGUI::ImageManager::getSingleton();

    imgmanager.addFromImageFile("hud_jewel",   "game/hud_jewel.png",   "ingame-images");
    imgmanager.addFromImageFile("hud_alex",    "game/hud_alex.png",    "ingame-images");
    imgmanager.addFromImageFile("hud_itembox", "game/hud_itembox.png", "ingame-images");

    m_normal_berry_img = path_to_utf8(pResource_Manager->Get_User_Pixmap("game/items/mushroom_red.png"));
    m_fire_berry_img   = path_to_utf8(pResource_Manager->Get_User_Pixmap("game/items/fireberry_1.png"));
    m_ice_berry_img    = path_to_utf8(pResource_Manager->Get_User_Pixmap("game/items/mushroom_blue.png"));

    // Convert the pathes to the names they are stored under in CEGUI
    // (see cEditor::load_cegui_image(), which is executed on game startup).
    string_replace_all(m_normal_berry_img, "/", "+");
    string_replace_all(m_fire_berry_img,   "/", "+");
    string_replace_all(m_ice_berry_img,    "/", "+");
}

/**
 * Call this whenever the screen's resolution changes. It adjusts the
 * sizing of the HUD images according to global_upscalex and global_upscaley,
 * which need to be set previously to calling this function.
 */
void cHud::Screen_Size_Changed()
{
    // Size of the jewel image
    mp_hud_root->getChild("jewel_image")->setSize(CEGUI::USize(CEGUI::UDim(0, JEWEL_WIDTH),
                                                               CEGUI::UDim(0, JEWEL_HEIGHT)));

    // Size of Alex head image
    mp_hud_root->getChild("alex_image")->setSize(CEGUI::USize(CEGUI::UDim(0, ALEX_HEAD_WIDTH),
                                                              CEGUI::UDim(0, ALEX_HEAD_HEIGHT)));


    // Size & position the item box and its contents
    mp_hud_root->getChild("itembox_image")->setSize(CEGUI::USize(CEGUI::UDim(0, ITEMBOX_WIDTH),
                                                                 CEGUI::UDim(0, ITEMBOX_HEIGHT)));

    mp_item_image->setArea(CEGUI::UDim(0.5, -(0.5*BERRY_WIDTH)),
                           CEGUI::UDim(0.5, -(0.5*BERRY_HEIGHT)),
                           CEGUI::UDim(0, BERRY_WIDTH),
                           CEGUI::UDim(0, BERRY_HEIGHT));
}

cHudSprite::cHudSprite(cSprite_Manager* sprite_manager)
    : cSprite(sprite_manager)
{
    m_camera_range = 0;
    m_pos_z = 0.13f;

    Set_Ignore_Camera(1);
}

cHudSprite::~cHudSprite(void)
{

}

cHudSprite* cHudSprite::Copy(void) const
{
    cHudSprite* hud_sprite = new cHudSprite(m_sprite_manager);
    hud_sprite->Set_Image(m_start_image);
    hud_sprite->Set_Pos(m_start_pos_x, m_start_pos_y, 1);
    hud_sprite->m_type = m_type;
    hud_sprite->m_sprite_array = m_sprite_array;
    hud_sprite->m_massive_type = m_massive_type;
    hud_sprite->Set_Ignore_Camera(m_no_camera);
    hud_sprite->Set_Shadow_Pos(m_shadow_pos);
    hud_sprite->Set_Shadow_Color(m_shadow_color);
    return hud_sprite;
}

cMiniPoints::cMiniPoints(std::string pointstext, float x, float y, Color color)
    : mp_label(NULL), m_counter(MINIPOINTS_DISPLAY_TIME),
      m_x(x), m_y(y)
{
    CEGUI::Window* p_root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    // TODO: Apply color

    mp_label = wmgr.createWindow("TSCLook256/Label");
    mp_label->setText(pointstext);
    p_root->addChild(mp_label);
}

cMiniPoints::~cMiniPoints()
{
    CEGUI::Window* p_root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    p_root->removeChild(mp_label);
    wmgr.destroyWindow(mp_label);
    mp_label = NULL;
}

/**
 * Decreases the internal show counter, updates the position of the text as
 * required to adjust to camera movements, and returns true if the text is
 * not displayed anymore and can thus be deleted. Otherwise returns false,
 * which means you need to keep the object around.
 *
 * Call this once per frame.
 */
bool cMiniPoints::Update()
{
    if (m_counter <= 0)
        return true;
    else {
        // Have it move upwards a little with each frame
        m_y -= 0.4f;

        // Now resolve the position (CEGUI expects coordinates always
        // relative to the current screen, it does not now the level
        // coordinates that are stored in m_x, m_y).
        float x = m_x - pLevel_Manager->m_camera->m_x;
        float y = m_y - pLevel_Manager->m_camera->m_y;

        mp_label->setPosition(CEGUI::UVector2(CEGUI::UDim(0, x), CEGUI::UDim(0, y)));
        mp_label->setAlpha(static_cast<float>(m_counter) / static_cast<float>(MINIPOINTS_DISPLAY_TIME));

        // Decrease display counter
        m_counter -= pFramerate->m_speed_factor;
        return false;
    }
}

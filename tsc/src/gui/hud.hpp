/***************************************************************************
 * hud.h
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

#ifndef TSC_HUD_HPP
#define TSC_HUD_HPP

namespace TSC {

    class cMiniPoints
    {
    public:
        cMiniPoints(std::string pointstext, float x, float y, Color color);
        ~cMiniPoints();

        bool Update();

    private:
        CEGUI::Window* mp_label;
        float m_counter;
        float m_x;
        float m_y;
    };

    /**
     * The HUD (Head-Up Display) is the collection of UI elements at the
     * top of the screen (time passed, jewels collected, etc.). It is
     * implemented as a number of stacked CEGUI windows and can act
     * either in world or level mode depending on the value of the
     * Game_Mode global variable, which makes it show slightly different
     * things.
     *
     * The HUD instance is global (`gp_hud` variable) and is not destroyed
     * when a level ends. It is always the same instance.
     */
    class cHud
    {
    public:
        cHud();
        ~cHud();

        void Show();
        void Hide();

        void Set_Points(long points);
        void Add_Points(long points, float x = 0.0f, float y = 0.0f, std::string strtext = "", const Color& color = static_cast<uint8_t>(255), bool allow_multiplier = false);
        void Reset_Points(void);
        long Get_Points(void);

        void Set_Jewels(int jewels);
        void Add_Jewels(int jewels);
        void Reset_Jewels(void);
        int Get_Jewels(void);

        void Set_Lives(int lives);
        void Add_Lives(int lives);
        void Reset_Lives(void);
        int Get_Lives(void);

        void Set_Elapsed_Time(uint32_t milliseconds);
        void Reset_Elapsed_Time(void);
        uint32_t Get_Elapsed_Time();

        void Set_Item(SpriteType item_type, bool sound = true);
        void Request_Item(void);
        void Reset_Item(void);
        SpriteType Get_Item();

        /// Reset everything (calls all Reset* methods)
        void Reset();

        /// Displays a short message to the user.
        void Set_Text(std::string message);

        void Set_Waypoint_Name(std::string name, Color color);
        void Set_World_Name(std::string name);

        void Update();
        void Screen_Size_Changed();

    private:
        long m_points;
        int m_jewels;
        int m_lives;
        cMovingSprite* mp_display_item; // Only for display
        SpriteType m_rescue_item_type; // This is what is stored across level switches
        uint32_t m_elapsed_time;
        std::chrono::system_clock::time_point m_last_time;
        float m_text_counter;

        CEGUI::FrameWindow* mp_hud_root;
        CEGUI::Window* mp_points_label;
        CEGUI::Window* mp_time_label;
        CEGUI::Window* mp_jewels_label;
        CEGUI::Window* mp_lives_label;
        CEGUI::Window* mp_waypoint_label;
        CEGUI::Window* mp_world_label;
        CEGUI::Window* mp_message_text;
        CEGUI::Window* mp_item_image;

        // Names the berries are available under in CEGUI
        // as images.
        std::string m_normal_berry_img;
        std::string m_fire_berry_img;
        std::string m_ice_berry_img;

        std::vector<cMiniPoints*> m_active_mini_points;

        void load_hud_images_into_cegui();
    };

    /* *** *** *** *** *** *** *** cHudSprite *** *** *** *** *** *** *** *** *** *** */

    /// Base class for image-showing HUD elements.
    /// This class is only kept for backwards-compatibility.
    class cHudSprite : public cSprite {
    public:
        cHudSprite(cSprite_Manager* sprite_manager);
        virtual ~cHudSprite(void);

        // copy this sprite
        virtual cHudSprite* Copy(void) const;
    };

    typedef vector<cHudSprite*> HudSpriteList;

    extern cHud* gp_hud;

} // namespace TSC

#endif

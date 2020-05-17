/***************************************************************************
 * random_sound.h
 *
 * Copyright © 2008 - 2011 Florian Richter
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

#ifndef TSC_RANDOM_SOUND_HPP
#define TSC_RANDOM_SOUND_HPP

#include "../core/global_basic.hpp"
#include "../objects/sprite.hpp"

namespace TSC {

    /* *** *** *** *** *** cRandom_Sound *** *** *** *** *** *** *** *** *** *** *** */

    class cRandom_Sound : public cSprite {
    public:
        // constructor
        cRandom_Sound(cSprite_Manager* sprite_manager);
        // create from stream
        cRandom_Sound(XmlAttributes& attributes, cSprite_Manager* sprite_manager);
        // destructor
        virtual ~cRandom_Sound(void);

        // initialize defaults
        void Init(void);
        // copy
        virtual cRandom_Sound* Copy(void) const;

        // Set filename
        void Set_Filename(const std::string& str);
        // Get filename
        std::string Get_Filename(void) const;

        // Set if it is played continuously
        void Set_Continuous(bool continuous);
        // Set minimal delay
        void Set_Delay_Min(unsigned int delay);
        // Get minimal delay
        unsigned int Get_Delay_Min(void) const;
        // Set maximum delay
        void Set_Delay_Max(unsigned int delay);
        // Get maximum delay
        unsigned int Get_Delay_Max(void) const;

        // Set minimal volume
        void Set_Volume_Min(float volume);
        // Get minimal volume
        float Get_Volume_Min(void) const;
        // Set maximum volume
        void Set_Volume_Max(float volume);
        // Get maximum volume
        float Get_Volume_Max(void) const;

        // Set start of gradual volume reduction
        void Set_Volume_Reduction_Begin(float distance);
        // Get start of gradual volume reduction
        float Get_Volume_Reduction_Begin(void) const;
        // Set end of gradual volume reduction
        void Set_Volume_Reduction_End(float distance);
        // Get end of gradual volume reduction
        float Get_Volume_Reduction_End(void) const;

        // Returns the volume modifier (0.0 - 1.0) for the current distance
        float Get_Distance_Volume_Mod(void) const;

        // update
        virtual void Update(void);
        // draw
        virtual void Draw(cSurface_Request* request = NULL);

        // if update is valid for the current state
        virtual bool Is_Update_Valid();
        // if draw is valid for the current state and position
        virtual bool Is_Draw_Valid(void);

        // editor activation
        virtual void Editor_Activate(void);
        // editor filename text changed event
        bool Editor_Filename_Text_Changed(const CEGUI::EventArgs& event);
        // editor continuous changed event
        bool Editor_Continuous_Changed(const CEGUI::EventArgs& event);
        // editor delay min text changed event
        bool Editor_Delay_Min_Text_Changed(const CEGUI::EventArgs& event);
        // editor delay max text changed event
        bool Editor_Delay_Max_Text_Changed(const CEGUI::EventArgs& event);
        // editor volume min text changed event
        bool Editor_Volume_Min_Text_Changed(const CEGUI::EventArgs& event);
        // editor volume max text changed event
        bool Editor_Volume_Max_Text_Changed(const CEGUI::EventArgs& event);
        // editor volume reduction begin text changed event
        bool Editor_Volume_Reduction_Begin_Text_Changed(const CEGUI::EventArgs& event);
        // editor volume reduction end text changed event
        bool Editor_Volume_Reduction_End_Text_Changed(const CEGUI::EventArgs& event);

        // Save to XML node
        virtual xmlpp::Element* Save_To_XML_Node(xmlpp::Element* p_element);

    protected:
        virtual std::string Get_XML_Type_Name();

    private:
        // the audio filename to play
        std::string m_filename;
        // is it played continuous
        bool m_continuous;
        // delay in milliseconds
        unsigned int m_delay_min;
        unsigned int m_delay_max;
        // volume in percent
        float m_volume_min;
        float m_volume_max;

        // volume reduction begin
        float m_volume_reduction_begin;
        // volume reduction end
        float m_volume_reduction_end;

        // distance to camera
        float m_distance_to_camera;

        // time until next play
        float m_next_play_delay;
        // time until next volume update
        float m_volume_update_counter;

        // editor color volume reduction begin
        Color m_editor_color_volume_reduction_begin;
        // editor color volume reduction end
        Color m_editor_color_volume_reduction_end;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif


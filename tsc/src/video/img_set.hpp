/***************************************************************************
 * img_set.h
 *
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

#ifndef TSC_IMG_SET_HPP
#define TSC_IMG_SET_HPP

#include "../core/global_game.hpp"
#include "../core/global_basic.hpp"
#include "../core/math/rect.hpp"
#include "../core/file_parser.hpp"
#include "../video/video.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** cImageSet *** *** *** *** *** *** *** *** *** *** */
    class cImageSet {
    public:
        /* *** *** *** *** *** *** *** FrameInfo *** *** *** *** *** *** *** *** *** *** */
        struct FrameInfo {
            FrameInfo();

            boost::filesystem::path m_filename;
            uint32_t m_time_min;
            uint32_t m_time_max;

            typedef std::pair<int, int> Entry_Type;
            typedef std::vector<Entry_Type> List_Type;

            List_Type m_branches;
        };

        /* *** *** *** *** *** *** *** Parser *** *** *** *** *** *** *** *** *** *** */
        class Parser : public cFile_parser
        {
        public:
            typedef std::vector<FrameInfo> List_Type;

            Parser(uint32_t time);
            bool HandleMessage(const std::string* parts, unsigned int count, unsigned int line);

            List_Type m_images;
            uint32_t m_time_min;
            uint32_t m_time_max;
        };

        /* *** *** *** *** *** *** *** Surface *** *** *** *** *** *** *** *** *** *** */

        class Surface {
        public:
            Surface(void);
            ~Surface(void);

            // enter the frame
            void Enter(void);
            // leave a frame, return next frame for branching or -1 
            int Leave(void);

            // the image
            cGL_Surface* m_image;
            // time to display in milliseconds
            uint32_t m_time;
            // information
            FrameInfo m_info;
        };


        /* *** *** *** *** *** *** *** cImageSet Contents *** *** *** *** *** *** *** *** *** *** */

        // constructor
        cImageSet();
        // destructor
        virtual ~cImageSet(void);

        /* Add an image to the animation
         * NULL image is allowed
         * time: if not set uses the default display time
        */
        void Add_Image(cGL_Surface* image, uint32_t time = 0);

        // Add an image set
        bool Add_Image_Set(const std::string& name, boost::filesystem::path path, uint32_t time = 0, int* start_num = NULL, int* end_num = NULL);

        // Set an active image set
        bool Set_Image_Set(const std::string& name, bool new_startimage = 0);

        // Set the animation start and end image
        inline void Set_Animation_Image_Range(const int start, const int end)
        {
            m_anim_img_start = start;
            m_anim_img_end = end;
        };
        /* Set the image using the given array number
        */
        void Set_Image_Num(const int num, bool new_startimage = 0);
        // Get an array image
        cGL_Surface* Get_Image(const unsigned int num) const;
        // Clear the image list
        void Clear_Images(bool reset_image=false, bool reset_startimage=false);

        /* Set if the animation is enabled
         * default : disabled
        */
        inline void Set_Animation(const bool enabled = 0)
        {
            m_anim_enabled = enabled;
        };
        // Reset animation back to the first image
        inline void Reset_Animation(void)
        {
            m_anim_counter = 0;
        };

        // update animation, return true on image change
        void Update_Animation(void);

        // Set default image display time
        inline void Set_Default_Time(const uint32_t time = 1000)
        {
            m_anim_time_default = time;
        };
        /* Set display time for all images
         * default_time: if set also make it the default time
        */
        void Set_Time_All(const uint32_t time, const bool default_time = 0);
        /* Set the animation speed modifier
         * 1.0 is the normal speed
        */
        inline void Set_Animation_Speed(const float anim_mod)
        {
            m_anim_mod = anim_mod;

            if (m_anim_mod < 0.0f) {
                m_anim_mod = 0.0f;
            }
        };

        /* Fetch a single image from another image set. */
        static cGL_Surface* Fetch_Single_Image(const boost::filesystem::path& path, int idx = 0);

        // currently set image array number
        int m_curr_img;
        // if animation is enabled
        bool m_anim_enabled;
        // animation start image
        int m_anim_img_start;
        // animation end image
        int m_anim_img_end;
        // default animation time
        uint32_t m_anim_time_default;
        // animation counter
        uint32_t m_anim_counter;
        uint32_t m_anim_last_ticks;
        // animation speed modifier
        float m_anim_mod;
    
        // Required overrides
        virtual std::string Get_Identity(void) { return std::string(); }
        virtual void Set_Image_Set_Image(cGL_Surface* new_image, bool new_startimage = 0) = 0;


        // Surface list
        typedef vector<Surface> Surface_List;
        Surface_List m_images;

        // Image set names
        typedef std::map<std::string, std::pair<int, int> > Name_Map;
        Name_Map m_named_ranges;

    };

    /* *** *** *** *** *** *** cSimpleImageSet *** *** *** *** *** *** *** *** *** */

    class cSimpleImageSet : public cImageSet {
    public:
        // ctor
        cSimpleImageSet();
        // dtor
        virtual ~cSimpleImageSet();

        // store identity
        inline void Set_Identity(const std::string& identity) {
            m_identity = identity;
        }

        // return identity
        virtual std::string Get_Identity(void);
        // set the current imageset image
        virtual void Set_Image_Set_Image(cGL_Surface* new_image, bool new_startimage = 0);

        // identity
        std::string m_identity;
        // image
        cGL_Surface* m_image;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

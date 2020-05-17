/***************************************************************************
 * campaign_manager.h
 *
 * Copyright © 2010 - 2011 Florian Richter
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

#ifndef TSC_CAMPAIGN_MANAGER_HPP
#define TSC_CAMPAIGN_MANAGER_HPP

#include "../core/global_basic.hpp"
#include "../core/global_game.hpp"
#include "../core/obj_manager.hpp"

namespace TSC {

    /* *** *** *** *** *** cCampaign *** *** *** *** *** *** *** *** *** *** *** *** */

    enum CampaignTargetType {
        CMP_TARGET_LEVEL,
        CMP_TARGET_WORLD,
        CMP_TARGET_SCENE
    };

    class cCampaign {
    public:
        cCampaign(void);
        ~cCampaign(void);

        // Save. Raises xmlpp::exception on error.
        void Save_To_File(const boost::filesystem::path& filename);

        // name
        std::string m_name;
        // target
        std::string m_target;
        // Type of target
        enum CampaignTargetType m_target_type;
        // description
        std::string m_description;
        // last save time
        time_t m_last_saved;
        /* 0 if only in game directory
         * 1 if only in user directory
         * 2 if in both
        */
        int m_user;
    };

    /* *** *** *** *** *** cCampaign_Manager *** *** *** *** *** *** *** *** *** *** *** *** */

    class cCampaign_Manager : public cObject_Manager<cCampaign> {
    public:
        cCampaign_Manager(void);
        virtual ~cCampaign_Manager(void);

        // load all campaigns
        void Load(void);
        // load a campaign
        cCampaign* Load_Campaign(const boost::filesystem::path& filename);

        // Get campaign from name
        cCampaign* Get_from_Name(const std::string& name);
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

// Campaign Manager
    extern cCampaign_Manager* pCampaign_Manager;

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

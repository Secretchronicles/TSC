/***************************************************************************
 * campaign_manager.cpp  -  class for handling campaigns
 *
 * Copyright © 2010 - 2011 The SMC Contributors
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "campaign_manager.hpp"
#include "campaign_loader.hpp"
#include "../gui/hud.hpp"
#include "../core/game_core.hpp"
#include "../core/filesystem/filesystem.hpp"
#include "../core/filesystem/resource_manager.hpp"
#include "../core/filesystem/package_manager.hpp"
#include "../core/filesystem/vfs.hpp"
#include "../core/i18n.hpp"
#include "../core/global_basic.hpp"

namespace fs = boost::filesystem;

using namespace std;

namespace SMC {

/* *** *** *** *** *** *** *** cCampaign *** *** *** *** *** *** *** *** *** *** */

cCampaign :: cCampaign(void)
{
    m_is_target_level = 0;
    m_user = 0;
}

cCampaign :: ~cCampaign(void)
{

}

void cCampaign :: Save_To_File(const fs::path& filename)
{
    xmlpp::Document doc;
    xmlpp::Element* p_root = doc.create_root_node("campaign");
    xmlpp::Element* p_node = NULL;

    // <information>
    p_node = p_root->add_child("information");
    Add_Property(p_node, "name", m_name);
    Add_Property(p_node, "description", m_description);
    Add_Property(p_node, "save_time", static_cast<Uint64>(time(NULL)));
    // </information>

    // <target>
    p_node = p_root->add_child("target");
    Add_Property(p_node, "name", m_target);
    Add_Property(p_node, "is_level", m_is_target_level);
    // </target>

    doc.write_to_file_formatted(Glib::filename_from_utf8(path_to_utf8(filename)));
    debug_print("Wrote campaign file '%s'.\n", path_to_utf8(filename).c_str());
}

/* *** *** *** *** *** *** *** cCampaign_Manager *** *** *** *** *** *** *** *** *** *** */

cCampaign_Manager :: cCampaign_Manager(void)
{
    Load();
}

cCampaign_Manager :: ~cCampaign_Manager(void)
{
    Delete_All();
}

void cCampaign_Manager :: Load(void)
{
    // if already loaded
    if (!objects.empty()) {
        Delete_All();
    }

    vector<fs::path> user_files = pVfs->Get_Directory_Files(pPackage_Manager->Get_User_Campaign_Path(), ".smccpn", false, false);
    vector<fs::path> game_files = pVfs->Get_Directory_Files(pPackage_Manager->Get_Game_Campaign_Path(), ".smccpn", false, false);

    for (vector<fs::path>::iterator itr = user_files.begin(); itr != user_files.end(); ++itr) {
        fs::path user_campaign_filename = (*itr);
        cCampaign* campaign = Load_Campaign(user_campaign_filename);

        // remove base directory
        user_campaign_filename = user_campaign_filename.filename();

        if (campaign) {
            Add(campaign);
            campaign->m_user = 1;

            // remove name from game files
            for (vector<fs::path>::iterator game_itr = game_files.begin(); game_itr != game_files.end(); ++game_itr) {
                fs::path game_campaign_filename = (*game_itr);

                // remove base directory
                game_campaign_filename = game_campaign_filename.filename();

                if (user_campaign_filename.compare(game_campaign_filename) == 0) {
                    campaign->m_user = 2;
                    game_files.erase(game_itr);
                    break;
                }
            }
        }
    }

    for (vector<fs::path>::iterator itr = game_files.begin(); itr != game_files.end(); ++itr) {
        fs::path campaign_filename = (*itr);
        cCampaign* campaign = Load_Campaign(campaign_filename);

        if (campaign) {
            Add(campaign);
        }
    }
}

cCampaign* cCampaign_Manager :: Load_Campaign(const fs::path& filename)
{
    if (!pVfs->File_Exists(filename)) {
        cerr << "Error : Campaign loading failed : " << path_to_utf8(filename) << endl;
        return NULL;
    }

    cCampaignLoader parser;
    parser.parse_file(filename);

    debug_print("Loaded campaign file: %s\n", path_to_utf8(filename).c_str());
    return parser.Get_Campaign();
}

cCampaign* cCampaign_Manager :: Get_from_Name(const std::string& name)
{
    for (vector<cCampaign*>::iterator itr = objects.begin(); itr != objects.end(); ++itr) {
        cCampaign* obj = (*itr);

        if (obj->m_name.compare(name) == 0) {
            return obj;
        }
    }

    return NULL;
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

cCampaign_Manager* pCampaign_Manager = NULL;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace SMC

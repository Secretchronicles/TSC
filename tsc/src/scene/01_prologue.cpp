/***************************************************************************
 * 01_prologue.cpp - Prologue scene
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

#include "01_prologue.hpp"
#include "../core/i18n.hpp"
#include "scene_actions.hpp"

namespace fs = boost::filesystem;
using namespace std;
using namespace TSC;

cPrologueScene::cPrologueScene(void)
    : cScene()
{
    namespace TSA = TSC::SceneActions;

    m_name = "Prologue";

    m_action_sequence.push_back(new TSA::ImageChange(this, "scene_images/placeholder.png"));
    m_action_sequence.push_back(new TSA::MusicChange(this, "story/theme_1.ogg"));

    m_action_sequence.push_back(new TSA::Narration(this, {
        _("The sinking sun sent out its last colourful beams over the Eastern Desert when a single individual was making his way through the ups and downs of the sandy dunes."),
        _("When he reached the top of the highest dune, he gazed around, but to only see sand followed by more sand, by even more [colour='FFFFF000']sand[colour='FFFFFFFF']."),
        _("He took out a map, but with the stars not yet visible on the darkening sky there was no means of orientation in this infinite sea of equally hostile sand dunes.")
    }));

    m_action_sequence.push_back(new TSA::NextUp(this, GA_ENTER_WORLD, "world_1"));
}

cPrologueScene::~cPrologueScene(void)
{
}

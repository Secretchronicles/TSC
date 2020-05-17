/***************************************************************************
 * mrb_scene.cpp
 *
 * Copyright © 2012-2020 The TSC Contributors
 ***************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "mrb_scene.hpp"
#include "../../../scene/scene.hpp"
#include "../../../core/game_core.hpp"

/**
 * Module: Scene
 *
 * Module for playing the visual novel scenes.
 */

using namespace TSC;
using namespace TSC::Scripting;

// Helper method
static void Play_Scene(std::string scene)
{
    Game_Action = GA_ENTER_SCENE;
    Game_Action_Data_Start.add("screen_fadeout", int_to_string(EFFECT_OUT_HORIZONTAL_VERTICAL));
    Game_Action_Data_Start.add("screen_fadeout_speed", "1.5");
    Game_Action_Data_Middle.add("load_scene", scene.c_str());
    Game_Action_Data_End.add("screen_fadein", int_to_string(EFFECT_IN_RANDOM));
    Game_Action_Data_End.add("screen_fadein_speed", "3");
}

/**
 * Method: Scene::play
 *
 *   play( scene )
 *
 * Calling this method plays the scene with the name C<scene>, which
 * has to be a string, e.g. C<"01_prologue"> (to play the prologue scene).
 * The canonical scene name mapping can be found in the source code
 * of the C<cScene::Load_Scene()> function.
 *
 * This method returns, but on the next frame update after return
 * the game mode switches to scene mode. Once the scene has completed,
 * the game mode returns to level mode in the level that was active
 * when scene play was started.
 *
 * An exception is raised if the requested scene does not exist.
 *
 * B<Remark>: This means that this method does not block until the
 * scene has finished!
 */
static mrb_value Play(mrb_state* p_state, mrb_value self)
{
    char* scene_name = NULL;
    mrb_get_args(p_state, "z", &scene_name);

    // In order to give early feedback on nonexisting scenes,
    // try to load the scene here temporaryly. If that fails,
    // raise. Otherwise register the scene for regular play.
    cScene* p_scene = cScene::Load_Scene(scene_name);
    if (p_scene) {
        delete p_scene;
        Play_Scene(scene_name);
    }
    else {
        mrb_raisef(p_state, MRB_ARGUMENT_ERROR(p_state), "Invalid scene '%s'.", scene_name);
    }

    return mrb_nil_value();
}

void TSC::Scripting::Init_Scene(mrb_state* p_state)
{
    struct RClass* p_rmScene = mrb_define_module(p_state, "Scene");

    mrb_define_module_function(p_state, p_rmScene, "play", Play, MRB_ARGS_REQ(1));
}

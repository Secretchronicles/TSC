/***************************************************************************
 * mrb_eato.cpp
 *
 * Copyright © 2013-2014 The TSC Contributors
 ***************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "../../../enemies/eato.hpp"
#include "../../../level/level.hpp"
#include "../../../core/sprite_manager.hpp"
#include "../../../core/property_helper.hpp"
#include "mrb_enemy.hpp"
#include "mrb_eato.hpp"

/**
 * Class: Eato
 *
 * Parent: [Enemy](enemy.html)
 * {: .superclass}
 *
 * The small _Eato_ is a plant glued to a wall, ceiling or even the bottom
 * and just sits there. It doesn’t move, but if Maryo gets in its reach,
 * it will bite.
 */

using namespace TSC;
using namespace TSC::Scripting;


/**
 * Method: Eato::new
 *
 *   new() → an_eato
 *
 * TODO: Docs.
 */
static mrb_value Initialize(mrb_state* p_state,  mrb_value self)
{
    cEato* p_eato = new cEato(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_eato;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    // This is a generated object
    p_eato->Set_Spawned(true);

    // Let TSC manage the memory
    pActive_Level->m_sprite_manager->Add(p_eato);

    return self;
}

/**
 * Method: Eato#image_dir=
 *
 *   image_dir=( path ) → path
 *
 * TODO: Docs.
 */
static mrb_value Set_Image_Dir(mrb_state* p_state, mrb_value self)
{
    char* cdir = NULL;
    mrb_get_args(p_state, "z", &cdir);

    cEato* p_eato = Get_Data_Ptr<cEato>(p_state, self);
    p_eato->Set_Image_Dir(utf8_to_path(cdir));

    return mrb_str_new_cstr(p_state, cdir);
}

/**
 * Method: Eato#image_dir
 *
 *   image_dir() → a_string
 *
 * TODO: Docs.
 */
static mrb_value Get_Image_Dir(mrb_state* p_state,  mrb_value self)
{
    cEato* p_eato = Get_Data_Ptr<cEato>(p_state, self);
    return mrb_str_new_cstr(p_state, path_to_utf8(p_eato->m_img_dir).c_str());
}

void TSC::Scripting::Init_Eato(mrb_state* p_state)
{
    struct RClass* p_rcEato = mrb_define_class(p_state, "Eato", mrb_class_get(p_state, "Enemy"));
    MRB_SET_INSTANCE_TT(p_rcEato, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcEato, "initialize", Initialize, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcEato, "image_dir", Get_Image_Dir, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcEato, "image_dir=", Set_Image_Dir, MRB_ARGS_REQ(1));
}

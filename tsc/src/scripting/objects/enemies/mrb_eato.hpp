/***************************************************************************
 * mrb_eato.hpp
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

#ifndef TSC_SCRIPTING_EATO_HPP
#define TSC_SCRIPTING_EATO_HPP
#include "../../scripting.hpp"

namespace TSC {
    namespace Scripting {
        void Init_Eato(mrb_state* p_state);
    }
}
#endif

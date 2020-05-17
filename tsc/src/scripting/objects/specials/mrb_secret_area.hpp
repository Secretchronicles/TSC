/***************************************************************************
 * mrb_secret_area.hpp
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

#ifndef TSC_SCRIPTING_SECRET_AREA_HPP
#define TSC_SCRIPTING_SECRET_AREA_HPP

namespace TSC {
    namespace Scripting {
        void Init_SecretArea(mrb_state* p_state);
    }
}

#endif /* TSC_SCRIPTING_SECRET_AREA_HPP */

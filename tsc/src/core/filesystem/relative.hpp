/***************************************************************************
 * relative.hpp - Implementation of relative path detector
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

/*
 * This file adds a function fs_relative() to boost::filesystem that
 * allows us to create a relative path from a given “root”, i.e. the
 * common parts of both paths are missing in the returned path object.
 *
 * Boost >= 1.60.0 includes such a function, but for the sake of
 * supporting older versions as well we provide our own version
 * here.
 */

#ifndef TSC_RELATIVE_HPP
#define TSC_RELATIVE_HPP

namespace TSC {

    /// Find the relative path from start_path to target_path.
    boost::filesystem::path fs_relative(boost::filesystem::path start_path, boost::filesystem::path target_path);
}
#endif

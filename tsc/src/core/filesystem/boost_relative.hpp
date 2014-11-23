/***************************************************************************
 * boost_relative.hpp - Implementation of boost::filesystem::relvative()
 *
 * Copyright © 2013 - 2014 The TSC Contributors
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
 * This file adds a function make_relative() to boost::filesystem that
 * allows us to create a relative path from a given “root”, i.e. the
 * common parts of both paths are missing in the returned path object.
 */

#ifndef TSC_BOOST_RELATIVE_HPP
#define TSC_BOOST_RELATIVE_HPP
#include <boost/filesystem.hpp>

namespace boost {

    namespace filesystem {
        /// Find the relative path from start_path to target_path.
        boost::filesystem::path relative(boost::filesystem::path start_path, boost::filesystem::path target_path);
    }
}
#endif

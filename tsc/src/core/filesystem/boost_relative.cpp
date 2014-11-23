/***************************************************************************
 * boost_relative.cpp - Implementation of boost::filesystem::relvative()
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

#include "boost_relative.hpp"

namespace boost {
namespace filesystem {

/**
 * Returns the path you need to walk in order to go from `start_path' to
 * `target_path'. Examples:
 *
 * /foo/bar/baz to /foo/blubb/zubb/xx => ../../blubb/zubb/xx
 * /foo/bar to /foo => ..
 * /foo to /foo/bar => bar
 * /foo/bar to /foo/bar => .
 *
 * Only works with absolute pathes. If relative ones are passed, boost::filesystem::absolute()
 * is called on them previously.
 */
boost::filesystem::path relative(boost::filesystem::path start_path, boost::filesystem::path target_path)
{
    start_path  = boost::filesystem::absolute(start_path);
    target_path = boost::filesystem::absolute(target_path);

    if (start_path == target_path)
        return boost::filesystem::path(".");

    boost::filesystem::path result;
    boost::filesystem::path::iterator startpath_iter = start_path.begin();
    boost::filesystem::path::iterator targetpath_iter = target_path.begin();

    while(true) {
        if (targetpath_iter == target_path.end()) {
            /* /foo/bar
             * /foo
             */
            for(; startpath_iter != start_path.end(); startpath_iter++) {
                result /= "..";
            }
            break;
        }
        else if (startpath_iter == start_path.end()) {
            /* /foo
             * /foo/bar
             */
            for(; targetpath_iter != target_path.end(); targetpath_iter++) {
                result /= (*targetpath_iter);
            }
            break;
        }
        else if (*startpath_iter != *targetpath_iter) {
            // Both are inequal at this part, but not terminal
            result /= "..";
        }
        else {
            // Both are equal at this part of the path (part of common root), skip to next one
            // (ignore)
        }

        startpath_iter++;
        targetpath_iter++;
    }

    return result;
}

}
}

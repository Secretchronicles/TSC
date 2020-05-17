/***************************************************************************
 * filesystem.h
 *
 * Copyright © 2005 - 2011 Florian Richter
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

#ifndef TSC_FILESYSTEM_HPP
#define TSC_FILESYSTEM_HPP

#include "../../core/global_basic.hpp"

namespace TSC {

    /* *** *** *** *** *** filesystem functions *** *** *** *** *** *** *** *** *** *** *** *** */

// Return the trimmed filename with the given options
    boost::filesystem::path Trim_Filename(boost::filesystem::path filename, bool keep_dir = true, bool keep_end = true);

// Check if the file exists
    bool File_Exists(const boost::filesystem::path& filename);
// Check if the directory exists
    bool Dir_Exists(const boost::filesystem::path& dir);

    /* Deletes the given directory, which must be empty.
     * Use with caution.
     * Returns true on success
    */
    bool Delete_Dir(const std::string& dir);

    /* Get the file size in bytes.
    * returns 0 if the file does not exist
    */
    size_t Get_File_Size(const std::string& filename);

// Converts "\" and "!" to "/"
    void Convert_Path_Separators(std::string& str);
    void Convert_Path_Separators(boost::filesystem::path& path);

    /* Get all files from the directory.
     * dir : the directory to scan
     * file_type : if set only this files with this file extension (with dot) are returned
     * with_directories : if set adds directories to the returned objects
     * search_in_sub_directories : searches in every sub-directory
    */
    vector<boost::filesystem::path> Get_Directory_Files(const boost::filesystem::path& dir, const std::string& file_type = "", bool with_directories = false, bool search_in_sub_directories = true);

// Return the operating system temporary files directory
    boost::filesystem::path Get_Temp_Directory(void);

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

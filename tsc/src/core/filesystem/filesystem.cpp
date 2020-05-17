/***************************************************************************
 * filesystem.cpp  -  File System
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

#include "../../core/filesystem/filesystem.hpp"
#include "../../core/game_core.hpp"
#include "../../core/global_basic.hpp"

using namespace std;

namespace fs = boost::filesystem;

namespace TSC {

/* *** *** *** *** *** *** cResource_Manager *** *** *** *** *** *** *** *** *** *** *** */

fs::path Trim_Filename(fs::path filename, bool keep_dir /* = 1 */, bool keep_end /* = 1 */)
{
    if (!keep_dir)
        filename = filename.filename();
    if (!keep_end)
        filename.replace_extension("");

    return filename;
}

bool File_Exists(const fs::path& filename)
{
    fs::file_type type = fs::status(filename).type();

    return type == fs::regular_file || type == fs::symlink_file;
}

bool Dir_Exists(const fs::path& dir)
{
    fs::file_type type = fs::status(dir).type();

    return type == fs::directory_file || type == fs::symlink_file;
}

bool Delete_Dir(const std::string& dir)
{
// fixme : boost should use a codecvt_facet but for now we convert to UCS-2
#ifdef _WIN32
    return RemoveDirectoryW(utf8_to_ucs2(dir).c_str()) != 0;
#else
    return rmdir(dir.c_str()) == 0;
#endif
}

size_t Get_File_Size(const std::string& filename)
{
    struct stat file_info;

    // if file exists
    if (stat(filename.c_str(), &file_info) == 0) {
        // if regular file
#ifdef _WIN32
        if ((file_info.st_mode & S_IFREG) > 0)
#else
        if (!S_ISDIR(file_info.st_mode))
#endif
        {
            // return size
            return file_info.st_size;
        }
    }

    return 0;
}

void Convert_Path_Separators(std::string& str)
{
    for (std::string::iterator itr = str.begin(); itr != str.end(); ++itr) {
        // convert it
        if (*itr == '\\' || *itr == '!') {
            *itr = '/';
        }
    }
}

void Convert_Path_Separators(fs::path& path)
{
    std::string str = path_to_utf8(path);
    Convert_Path_Separators(str);
    path = utf8_to_path(str);
}

vector<fs::path> Get_Directory_Files(const fs::path& dir, const std::string& file_type /* = "" */, bool with_directories /* = false */, bool search_in_sub_directories /* = true */)
{
    vector<fs::path> valid_files;
    fs::path extension = utf8_to_path(file_type);

    if (!Dir_Exists(dir))
        return valid_files;

    fs::directory_iterator end_iter; // No-args constructor makes an end-iter according to docs

    // load all available objects
    for (fs::directory_iterator dir_itr(dir); dir_itr != end_iter; ++dir_itr) {
        try {
            const fs::path this_path = dir_itr->path();

            // if directory
            if (fs::is_directory(*dir_itr)) {
                // ignore hidden directories and . and ..
                if (path_to_utf8(this_path.filename()).find(".") == 0) {
                    continue;
                }

                if (with_directories) {
                    valid_files.push_back(this_path);
                }

                // load all items from the sub-directory
                if (search_in_sub_directories) {
                    vector<fs::path> new_valid_files = Get_Directory_Files(this_path, file_type, with_directories);
                    valid_files.insert(valid_files.end(), new_valid_files.begin(), new_valid_files.end());
                }
            }
            // valid file
            else if (extension.empty() || this_path.extension() == extension) {
                valid_files.push_back(this_path);
            }
        }
        catch (const std::exception& ex) {
            cerr << dir_itr->path().string().c_str() << " " << ex.what() << endl;
        }
    }

    return valid_files;
}

boost::filesystem::path Get_Temp_Directory(void)
{
    return boost::filesystem::temp_directory_path();
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

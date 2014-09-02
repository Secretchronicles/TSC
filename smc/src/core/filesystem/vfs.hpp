/***************************************************************************
 * vfs.hpp
 *
 * Copyright © 2009 - 2014 The SMC Contributors
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SMC_VFS_HPP
#define SMC_VFS_HPP

#include <SDL_rwops.h>

#include "../../core/global_basic.hpp"
#include "../../core/global_game.hpp"
#include "../../core/xml_attributes.hpp"

namespace SMC
{

/* *** *** *** *** *** cVfs *** *** *** *** *** *** *** *** *** *** *** *** */

class cVfs
{
public:
	cVfs( const char* argv0 );
	~cVfs( void );

	std::string Mount(boost::filesystem::path dir, bool mount=true);

	bool Exists(boost::filesystem::path path);
	bool File_Exists(boost::filesystem::path path);
	bool Directory_Exists(boost::filesystem::path path);

	std::vector<boost::filesystem::path> Get_Directory_Files(const boost::filesystem::path& dir, const std::string& file_type="", bool with_directories=false, bool search_in_subdirectories=true);

	std::istream* Open_Stream(boost::filesystem::path file);
    SDL_RWops* Open_RWops(boost::filesystem::path file);

private:
	std::string Find(boost::filesystem::path path);
	
	static std::string path_to_physfs(boost::filesystem::path path);
	static boost::filesystem::path physfs_to_path(std::string physfs);


	std::map<boost::filesystem::path, std::string> m_mount_points;
	int m_mount_index;
};

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

// VFS
extern cVfs *pVfs;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace SMC

#endif


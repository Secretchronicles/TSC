/***************************************************************************
 * vfs.cpp  -  Virtual File System
 *
 * Copyright © 2009 - 2011 The SMC Contributors
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux)
#include <limits.h>
#endif

#include <physfs.h>

#include "vfs.hpp"
#include "resource_manager.hpp"
#include "filesystem.hpp"
#include "../../user/preferences.hpp"
#include "../property_helper.hpp"
#include "../errors.hpp"

namespace fs = boost::filesystem;
namespace errc = boost::system::errc;

namespace SMC
{

/* *** *** *** *** *** cVfs_streambuf *** *** *** *** *** *** *** *** *** *** */
class cVfs_streambuf : public std::streambuf
{
public:
	explicit cVfs_streambuf(PHYSFS_File* f, size_t size=2048, size_t putback=1) :
		m_file(f),
		m_putback(std::max(putback, size_t(1))),
		m_buffer(std::max(size, putback) + putback)
	{
		char* end = &m_buffer.front() + m_buffer.size();
		setg(end, end, end);
	}

	~cVfs_streambuf()
	{
		PHYSFS_close(m_file);
	}

private:
	cVfs_streambuf(const cVfs_streambuf&);
	cVfs_streambuf& operator=(const cVfs_streambuf&);

	PHYSFS_File* m_file;
	size_t m_putback;
	std::vector<char> m_buffer;

protected:
	std::streambuf::int_type underflow()
	{
		if(gptr() < egptr())
			return traits_type::to_int_type(*gptr());

		char* base = &m_buffer.front();
		char* start = base;

		if(eback() == base)
		{
			size_t diff = std::min(m_putback, static_cast<size_t>(egptr() - base));
			std::memmove(base, egptr() - diff, diff);
			start += diff;
		}

		// Read data from handle
		PHYSFS_sint64 n = PHYSFS_read(m_file, start, 1, m_buffer.size() - (start - base));
		if(n <= 0) // todo: -1 is an error
			return traits_type::eof();

		setg(base, start, start + static_cast<size_t>(n));
		return traits_type::to_int_type(*gptr());
	}
};

class cVfs_istream : public std::istream
{
public:
	explicit cVfs_istream(cVfs_streambuf* buf) :
		std::istream(buf), m_buf(buf)
	{
	}

	~cVfs_istream()
	{
		delete m_buf;
	}

private:
	cVfs_istream(const cVfs_istream&);
	cVfs_istream& operator=(const cVfs_istream&);

	cVfs_streambuf* m_buf;
};

/* *** *** *** *** *** cVfs_RWops *** *** *** *** *** *** *** *** *** *** */
class cVfs_RWops
{
public:
	cVfs_RWops(PHYSFS_File* file) :
		m_file(file), m_offset(0)
	{
	}

	~cVfs_RWops()
	{
		PHYSFS_close(m_file);
	}

	static SDL_RWops* Create(PHYSFS_File* f)
	{
		SDL_RWops* ops = SDL_AllocRW();

		ops->seek = sdl_seek;
		ops->read = sdl_read;
		ops->write = sdl_write;
		ops->close = sdl_close;
		ops->type = 0;
		ops->hidden.unknown.data1 = new cVfs_RWops(f);

		return ops;
	}

	static int sdl_seek(SDL_RWops* context, int offset, int whence)
	{
		cVfs_RWops* data = static_cast<cVfs_RWops*>(context->hidden.unknown.data1);

		int target = data->m_offset;
		switch(whence)
		{
			// TODO: default?
			default:
			case RW_SEEK_SET:
				target = offset;
				break;

			case RW_SEEK_CUR:
				target += offset;
				break;

			case RW_SEEK_END:
				// TODO: what if fileLength returns in error
				PHYSFS_sint64 l = PHYSFS_fileLength(data->m_file);
				target = l + offset;
				break;
		}

		// todo: handle error
		PHYSFS_seek(data->m_file, target);
		data->m_offset = PHYSFS_tell(data->m_file);

		return data->m_offset;
	}

	static int sdl_read(SDL_RWops *context, void *ptr, int size, int maxnum)
	{
		cVfs_RWops* data = static_cast<cVfs_RWops*>(context->hidden.unknown.data1);

		PHYSFS_sint64 n = PHYSFS_read(data->m_file, ptr, size, maxnum);
		if(n <= 0) // todo: -1 is an error
			return 0;

		data->m_offset += (n * size);
		return n;
	}

	static int sdl_write(SDL_RWops *context, const void *ptr, int size, int num)
	{
		return 0;
	}

	static int sdl_close(SDL_RWops *context)
	{
		cVfs_RWops* data = static_cast<cVfs_RWops*>(context->hidden.unknown.data1);
		delete data;

        SDL_FreeRW(context);
		return 0;
	}

	PHYSFS_File* m_file;
	int m_offset;
};



/* *** *** *** *** *** *** cVfs *** *** *** *** *** *** *** *** *** *** *** */

cVfs :: cVfs( const char *argv0 ) : m_mount_index(0)
{
	std::cout << "Initializing VFS" << std::endl;
	PHYSFS_init(argv0);
}

cVfs :: ~cVfs( void )
{
	PHYSFS_deinit();
}

std::string cVfs :: Mount(fs::path dir, bool mount/*=true*/)
{
	// First check if this path has already been mounted, and if so, return the mount point
	if(m_mount_points.find(dir) != m_mount_points.end())
		return m_mount_points[dir];

	if(!mount)
		return std::string();

	// Otherwise, create the mount point in PhysFS
	std::string mount_point = "/mounts/" + int_to_string(m_mount_index);
	m_mount_index++;

	PHYSFS_mount(path_to_utf8(dir).c_str(), mount_point.c_str(), 1);
	m_mount_points[dir] = mount_point;

	return mount_point;
}

bool cVfs :: Exists(fs::path path)
{
	std::string physfs_path = Find(path);
	if(!physfs_path.empty())
	{
		return PHYSFS_exists(physfs_path.c_str()) != 0;
	}

	return false;
}

bool cVfs :: File_Exists(fs::path path)
{
	std::string physfs_path = Find(path);
	if(!physfs_path.empty())
	{
		return PHYSFS_exists(physfs_path.c_str()) != 0 && PHYSFS_isDirectory(physfs_path.c_str()) == 0;
	}

	return false;
}

bool cVfs :: Directory_Exists(fs::path path)
{
	std::string physfs_path = Find(path);
	if(!physfs_path.empty())
	{
		return PHYSFS_exists(physfs_path.c_str()) != 0 && PHYSFS_isDirectory(physfs_path.c_str()) != 0;
	}

	return false;
}
	
std::vector<fs::path> cVfs :: Get_Directory_Files(const fs::path& dir, const std::string& file_type/*=""*/, bool with_directories/*=false*/, bool search_in_subdirectories/*=true*/)
{
	std::vector<fs::path> valid_files;
	fs::path extension = utf8_to_path(file_type);

	std::string physfs_path = Find(dir);
	if(physfs_path.empty())
		return valid_files;

	char** results = PHYSFS_enumerateFiles(physfs_path.c_str());
	if(results == NULL)
		return valid_files;

	// read all available entries
	for(char** i = results; *i != NULL; i++)
	{
		const fs::path this_path = dir / physfs_to_path(*i);
		const std::string this_physfs_path = physfs_path + "/" + *i;

		if(PHYSFS_isDirectory(this_physfs_path.c_str()))
		{
			// ignore hidden directories and '.' and '..'
			// TODO: should this be moved outside of the directory check to ignore all
			// hidden items including hidden files?
			if(path_to_utf8(this_path.filename()).find(".") == 0)
			{
				continue;
			}

			if(with_directories)
			{
				valid_files.push_back(this_path);
			}

			// load all items from the subdirectory
			if(search_in_subdirectories)
			{
				std::vector<fs::path> new_valid_files = Get_Directory_Files(this_path, file_type, with_directories, search_in_subdirectories);
				valid_files.insert(valid_files.end(), new_valid_files.begin(), new_valid_files.end());
			}
		}
		else if(extension.empty() || this_path.extension() == extension)
		{
			valid_files.push_back(this_path);
		}
	}

	PHYSFS_freeList(results);
	return valid_files;
}

std::istream* cVfs :: Open_Stream(fs::path file)
{
	std::string physfs_path = Find(file);

	if(!physfs_path.empty())
	{
		PHYSFS_File* f = PHYSFS_openRead(physfs_path.c_str());
		if(f)
		{
			return new cVfs_istream(new cVfs_streambuf(f));
		}
	}

	return NULL;
}

SDL_RWops* cVfs :: Open_RWops(fs::path file)
{
    std::string physfs_path = Find(file);

    if(!physfs_path.empty())
    {
        PHYSFS_File* f = PHYSFS_openRead(physfs_path.c_str());
        if(f)
        {
            return cVfs_RWops::Create(f);
        }
    }

    return NULL;
}

std::string cVfs :: Find(fs::path path)
{
	fs::path internal;
	std::string result;
	
	path = fs::absolute(path);
	while(!(path.empty() || fs::exists(path)))
	{
		internal = path.filename() / internal;
		path = path.parent_path();
	}

	if(path.empty())
		return result;

	// To avoid certain inconsistencies, this code no long auto-mounts zip files,
	// so they should be mounted if needed beforehand.  Regular reads and writes
	// still work due to mounting of root directories as needed.

	// check if mount point already exists, if not mount root path
	result = Mount(path, false);
	if(result.empty())
		result = Mount(path.root_path()) + "/" + path_to_physfs(path.relative_path());

	if(!internal.empty())
		result = result + "/" + path_to_physfs(internal);

	return result;
}

/* static */
std::string cVfs :: path_to_physfs(fs::path path)
{
	// TODO: ensure result are in UTF8 for physfs and needs to use '/' for separators
	return path.generic_string();
}

/* static */
fs::path cVfs :: physfs_to_path(std::string physfs_path)
{
	return utf8_to_path(physfs_path);
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

cVfs *pVfs = NULL;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace SMC


#include "components/vfs/unix.hpp"

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <fstream>
#include <limits>

namespace vfs
{
RootFileSystem &instance(void *context)
{
	static vfs::RootFileSystem fs;

	static bool first_time = true;
	if (first_time)
	{
		char buf[256];
		getcwd(buf, 256);

		std::string cwd{buf};
		fs.mount("/", std::make_shared<vfs::UnixFileSystem>(cwd));
		fs.mount("/scenes", std::make_shared<vfs::UnixFileSystem>(cwd + "/assets/scenes"));
		fs.mount("/textures", std::make_shared<vfs::UnixFileSystem>(cwd + "/assets/textures"));
		fs.mount("/fonts", std::make_shared<vfs::UnixFileSystem>(cwd + "/assets/fonts"));
		fs.mount("/temp", std::make_shared<vfs::UnixTempFileSystem>());

		first_time = false;
	}

	return fs;
}

inline const std::string get_temp_path_from_environment()
{
	char const *temp_dir = getenv("TMPDIR");
	if (temp_dir == nullptr)
	{
		temp_dir = "/tmp";
	}

	return std::string(temp_dir);
}

UnixFileSystem::UnixFileSystem(const std::string &base_path) :
    FileSystem{},
    m_base_path{base_path}
{
}

UnixTempFileSystem::UnixTempFileSystem() :
    UnixFileSystem{get_temp_path_from_environment()}
{}

bool UnixFileSystem::folder_exists(const std::string &file_path)
{
	auto full_path = m_base_path + file_path;

	struct stat info;
	if (stat(full_path.c_str(), &info) != 0)
	{
		return false;
	}
	return (info.st_mode & S_IFDIR) == S_IFDIR;
}

bool UnixFileSystem::file_exists(const std::string &file_path)
{
	auto full_path = m_base_path + file_path;

	struct stat info;
	if (stat(full_path.c_str(), &info) != 0)
	{
		return false;
	}
	return (info.st_mode & S_IFREG) == S_IFREG;
}

status::status UnixFileSystem::read_file(const std::string &file_path, std::shared_ptr<Blob> *blob)
{
	auto full_path = m_base_path + file_path;

	std::fstream stream;
	stream.open(full_path, std::ios::in | std::ios::binary);

	if (!stream.good())
	{
		return status::FailedToRead;
	}

	if (!blob)
	{
		return status::Success;
	}

	stream.ignore(std::numeric_limits<std::streamsize>::max());
	std::streamsize size = stream.gcount();
	stream.clear();
	stream.seekg(0, std::ios_base::beg);

	auto w_blob = std::make_shared<StdBlob>();
	w_blob->buffer.resize(static_cast<size_t>(size), 0);

	if (!stream.read(reinterpret_cast<char *>(w_blob->buffer.data()), size))
	{
		return status::FailedToRead;
	}

	*blob = w_blob;

	return status::Success;
}

status::status UnixFileSystem::read_chunk(const std::string &file_path, const size_t offset, const size_t count, std::shared_ptr<Blob> *blob)
{
	auto full_path = m_base_path + file_path;

	std::fstream stream;
	stream.open(full_path, std::ios::in | std::ios::binary);

	if (!stream.good())
	{
		return status::FailedToRead;
	}

	if (!blob)
	{
		return status::Success;
	}

	stream.ignore(std::numeric_limits<std::streamsize>::max());
	std::streamsize size = stream.gcount();
	stream.clear();

	if (offset + count > size)
	{
		return status::MemoryOutOfRange;
	}

	stream.seekg(offset, std::ios_base::beg);

	auto w_blob = std::make_shared<StdBlob>();
	w_blob->buffer.resize(static_cast<size_t>(count), 0);

	if (!stream.read(reinterpret_cast<char *>(w_blob->buffer.data()), count))
	{
		return status::FailedToRead;
	}

	*blob = w_blob;

	return status::Success;
}

size_t UnixFileSystem::file_size(const std::string &file_path)
{
	auto full_path = m_base_path + file_path;

	std::fstream stream;
	stream.open(full_path, std::ios::in | std::ios::binary);
	if (!stream.good())
	{
		return 0;
	}

	stream.ignore(std::numeric_limits<std::streamsize>::max());
	return stream.gcount();
}

bool UnixFileSystem::write_file(const std::string &file_path, const void *data, size_t size)
{
	auto full_path = m_base_path + file_path;

	std::fstream stream;
	stream.open(full_path, std::ios::out | std::ios::binary);
	if (!stream.good() || !stream.write(reinterpret_cast<const char *>(data), size))
	{
		return false;
	}
	return true;
}

status::status UnixFileSystem::enumerate_files(const std::string &file_path, std::vector<std::string> *files)
{
	std::string full_path = file_path;

	if (file_exists(file_path))
	{
		full_path = full_path.substr(0, full_path.rfind('/'));
	}

	if (!folder_exists(full_path))
	{
		// no folder exists
		return status::FileNotFound;
	}

	full_path = m_base_path + full_path;

	std::vector<std::string> _files;

	DIR *          dir;
	struct dirent *ent;
	if ((dir = opendir(full_path.c_str())) != NULL)
	{
		while ((ent = readdir(dir)) != NULL)
		{
			// check if file and not dir
			auto file_name = std::string{ent->d_name};
			if (file_name == ".." || file_name == ".")
			{
				continue;
			}
			file_name             = "/" + file_name;
			std::string file_path = full_path + file_name;

			struct stat info;
			if (stat(file_path.c_str(), &info) != 0)
			{
				continue;
			}
			if ((info.st_mode & S_IFREG) == S_IFREG)
			{
				_files.push_back(file_name);
			}
		}
		closedir(dir);
	}

	*files = _files;

	return status::Success;
}

status::status UnixFileSystem::enumerate_folders(const std::string &file_path, std::vector<std::string> *folders)
{
	std::string full_path = file_path;

	if (file_exists(file_path))
	{
		full_path = full_path.substr(0, full_path.rfind('/'));
	}
	else if (!folder_exists(file_path) && !folder_exists(full_path))
	{
		// no folder exists
		return status::FileNotFound;
	}

	full_path = m_base_path + full_path;

	std::vector<std::string> _folders;

	DIR *          dir;
	struct dirent *ent;
	if ((dir = opendir(full_path.c_str())) != NULL)
	{
		while ((ent = readdir(dir)) != NULL)
		{
			// check if file and not dir
			std::string folder_name = std::string{ent->d_name};
			if (folder_name == ".." || folder_name == ".")
			{
				continue;
			}
			folder_name             = "/" + folder_name;
			std::string folder_path = full_path + folder_name;

			struct stat info;
			if (stat(folder_path.c_str(), &info) != 0)
			{
				continue;
			}
			if ((info.st_mode & S_IFDIR) == S_IFDIR)
			{
				_folders.push_back(folder_name);
			}
		}
		closedir(dir);
	}

	*folders = _folders;

	return status::Success;
}

}        // namespace vfs
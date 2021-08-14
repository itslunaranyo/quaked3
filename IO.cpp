#include "pre.h"
#include "IO.h"
#include <filesystem>

std::string IO::GetAppPath()
{
	return std::filesystem::current_path().string();
}

bool IO::FileExists(const std::string& f)
{
	if (std::filesystem::is_directory(f))
		return false;
	return std::filesystem::exists(f);
}

bool IO::FileExists(const std::string& dir, const std::string& f)
{
	auto p = std::filesystem::path(dir) / f;
	return std::filesystem::exists(p);
}

bool IO::FileWithExtensionExists(const std::string& path)
{
	auto p = std::filesystem::path(path);
	std::string dir = p.parent_path().string();
	std::string ext = p.extension().string();
	return FileWithExtensionExists(dir,ext);
}

bool IO::FileWithExtensionExists(const std::string& dir, const std::string& ext)
{
	for (auto f : std::filesystem::directory_iterator(dir))
	{
		if (f.path().extension() == ext)
			return true;
	}
	return false;
}

bool IO::DirExists(const std::string& d)
{
	if (!std::filesystem::is_directory(d))
		return false;
	return std::filesystem::exists(d);
}

std::string IO::ChangeExtension(const std::string& path, const char* ext)
{
	auto p = std::filesystem::path(path);
	p.replace_extension(ext);
	return p.string();
}

std::string IO::FileName(const std::string& path)
{
	auto p = std::filesystem::path(path);
	return p.filename().string();
}

bool IO::IsPathAbsolute(const std::string& path)
{
	auto p = std::filesystem::path(path);
	return p.is_absolute();
}

int IO::AllWithWildcard(const std::string& path, std::list<std::string>& out)
{
	auto p = std::filesystem::path(path);
	std::string dir = p.parent_path().string();
	std::string ext = p.extension().string();

	return AllWithExtension(dir, ext, out);
}

int IO::AllWithExtension(const std::string_view& dir, const std::string_view& ext, std::list<std::string>& out)
{
	int i = 0;
	for (auto f : std::filesystem::directory_iterator(dir))
	{
		if (f.path().extension() == ext)
		{
			out.push_back(f.path().string());
			++i;
		}
	}
	return i;
}

void IO::RenameFile(const std::string& from, const std::string& to)
{
	if (FileExists(to))
		std::filesystem::remove(to);
	std::filesystem::rename(from, to);
}
#pragma once

#include <list>

namespace IO
{
    std::string GetAppPath();
    bool FileExists(const std::string& f);
    bool FileExists(const std::string& dir, const std::string& f);
	bool FileWithExtensionExists(const std::string& dir, const std::string& ext);
	bool FileWithExtensionExists(const std::string& path);
	bool DirExists(const std::string& d);
    std::string ChangeExtension(const std::string& path, const char* ext);
    std::string FileName(const std::string& path);
    bool IsPathAbsolute(const std::string& path);
	int AllWithWildcard(const std::string& path, std::list<std::string>& out);
    int AllWithExtension(const std::string_view& dir, const std::string_view& ext, std::list<std::string>& out);
    void RenameFile(const std::string& from, const std::string& to);
}


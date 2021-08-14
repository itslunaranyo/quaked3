#pragma once

#include <fstream>

class FileReader
{
public:
	FileReader();
	virtual ~FileReader();

	bool Open(const char* filename);
	bool Open(const std::string& filename);
	bool AtEnd();
	size_t Size() { return len; };
	bool Empty() { return len <= 0; }
protected:
	std::ifstream file;
	size_t len = -1;
	std::ios::openmode oflag = std::ios::in;
};


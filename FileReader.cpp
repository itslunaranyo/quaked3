#include "pre.h"
#include "FileReader.h"

FileReader::FileReader()
{
}

FileReader::~FileReader()
{
	file.close();
}

bool FileReader::Open(const char* filename)
{
	file.open(filename, oflag | std::ios::ate);
	if (file.fail())
		Error(_S("Couldn't open file %d\n") << filename);
	len = file.tellg();
	file.seekg(0);
	return true;
}
bool FileReader::Open(const std::string& filename)
{
	return Open(filename.c_str());
}

bool FileReader::AtEnd()
{
	return file.eof();
}


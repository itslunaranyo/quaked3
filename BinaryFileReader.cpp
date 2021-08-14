#include "pre.h"
#include "BinaryFileReader.h"

BinaryFileReader::BinaryFileReader()
{
	oflag |= std::ios::binary;
}

BinaryFileReader::~BinaryFileReader()
{
}

size_t BinaryFileReader::ReadAll(void* b)
{
	file.seekg(0);
	file.read((char*)b, len);
	return file.gcount();
}

size_t BinaryFileReader::Read(void* b, size_t amt)
{
	file.read((char*)b, amt);
	return file.gcount();
}

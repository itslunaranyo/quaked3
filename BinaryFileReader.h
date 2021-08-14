#pragma once

#include "FileReader.h"

class BinaryFileReader : public FileReader
{
public:
	BinaryFileReader();
	~BinaryFileReader();

	// use Size() to make your buffer the right size first
	size_t ReadAll(void* b);
	size_t Read(void* b, size_t amt);
};


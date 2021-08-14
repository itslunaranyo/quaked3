#pragma once

#include "FileReader.h"

class TextFileReader : public FileReader
{
public:
	TextFileReader();

	void ReadAll(std::string& s);
	const std::string& ReadNextLine();
	const std::string& ReadCurrentLine();

	int LineNum() { return line; }

protected:
	std::string curLine;
	int line = -1;
};

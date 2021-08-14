#include "pre.h"
#include "TextFileReader.h"


TextFileReader::TextFileReader()
{
	curLine.reserve(256);
}

void TextFileReader::ReadAll(std::string& s)
{
	char buf[4096];
	s.clear();
	s.reserve(len + 1);
		
	file.seekg(0);
	buf[4095] = 0;
	while (file.read(buf, 4095))
		s.append(buf);
	s.append(buf, file.gcount());
}

const std::string& TextFileReader::ReadCurrentLine()
{
	return curLine;
}

const std::string& TextFileReader::ReadNextLine()
{
	char buf[256];
	curLine.clear();

	do {
		file.getline(buf, 256);
		curLine.append(buf);
	} while (file.gcount() == 256);
	++line;
	return curLine;
}
#pragma once

class TextureGroup;

class WadReader
{
public:
	WadReader() {};
	~WadReader() {};

	TextureGroup* Read(const std::string& filename);
};


#pragma once

#include "TextFileReader.h"
#include "mathlib.h"
#include <map>

class ConfigReader : public TextFileReader
{
public:
	ConfigReader();

	bool ReadSection();
	const std::string& currentSection();
	bool ReadString(const std::string& key, std::string& out);
	bool ReadInt(const std::string& key, int& out);
	bool ReadBool(const std::string& key, bool& out);
	bool ReadFloat(const std::string& key, float& out);
	bool ReadVec3(const std::string& key, vec3& out);

private:
	std::string sectionName = "";
	std::map<std::string, std::string> cfgDict;
	bool onSectionTitle();
};


#include "pre.h"
#include "strlib.h"
#include "ConfigReader.h"
#include "Tokenizer.h"

ConfigReader::ConfigReader() {}

bool ConfigReader::ReadSection()
{
	while (!onSectionTitle() && !AtEnd())
	{
		ReadNextLine();
	}
	int cbracket = curLine.find(']');
	if (curLine[0] == '[' && cbracket != std::string::npos)
	{
		sectionName = curLine.substr(1, cbracket - 1);
		cfgDict.clear();

		while (!AtEnd())
		{
			ReadNextLine();
			if (onSectionTitle())
				break;
			Tokenizer tokr(curLine);
			StringViewList tokens;
			tokr.All(tokens);
			if (tokens.size() != 2)
				continue;
			cfgDict[std::string(tokens[0])] = std::string(tokens[1]);
		}
	}
	else
		Error(_S("Malformed configuration file on line %d") << line);

	return onSectionTitle();
}

const std::string& ConfigReader::currentSection()
{
	return sectionName;
}

bool ConfigReader::ReadString(const std::string& key, std::string& out)
{
	if (cfgDict[key].empty())
		return false;
	out = cfgDict[key];
	return true;
}

bool ConfigReader::ReadInt(const std::string& key, int& out)
{
	try {
		int x = std::stoi(cfgDict[key]);
		out = x;
		return true;
	}
	catch (std::exception) {
		return false;
	}
}

bool ConfigReader::ReadBool(const std::string& key, bool& out)
{
	try {
		int x = std::stoi(cfgDict[key]);
		out = (x != 0);
		return true;
	}
	catch (std::exception) {
		return false;
	}
}

bool ConfigReader::ReadFloat(const std::string& key, float& out)
{
	try {
		float x = std::stof(cfgDict[key]);
		out = x;
		return true;
	}
	catch (std::exception) {
		return false;
	}
}

bool ConfigReader::ReadVec3(const std::string& key, vec3& out)
{
	return strlib::StringToVec(cfgDict[key], out);
}

bool ConfigReader::onSectionTitle()
{
	return (!curLine.empty() && curLine[0] == '[');
}

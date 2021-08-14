#pragma once

#include "EPair.h"
#include <list>
class EntClass;

class EntityDefReader
{
public:
	EntityDefReader() {};
	~EntityDefReader() {};
	virtual void ReadFromPath(const std::string& path) {};
	virtual void GetTypes(std::map<std::string, EPairType>& epf) {};
	virtual void GetClassnames(StringViewList& clist) {};
	virtual void GetDefinition(EntClass& ec) {};

protected:
	// for minimal string copying, keep all the file contents in memory and work
	//	exclusively with string_views until the point of insertion into the eclass
	std::list<std::string> contents;

	std::map<std::string_view, std::string_view> blocks;
	virtual size_t GetEntityBlock(const std::string_view qc, size_t ofs) { return 0; };

	std::map<std::string_view, EPairType> fields;
	virtual size_t GetField(const std::string_view qc, size_t ofs) { return 0; };

};


#pragma once

#include "EntityDefReader.h"

class QcDefReader : public EntityDefReader
{
public:
	QcDefReader() {};
	~QcDefReader() {};

	void ReadFromPath(const std::string& path) override;
	void GetTypes(std::map<std::string, EPairType> & epf) override;
	void GetClassnames(StringViewList& clist) override;
	void GetDefinition(EntClass& ec) override;

private:
	size_t GetEntityBlock(const std::string_view qc, size_t ofs) override;
	size_t GetField(const std::string_view qc, size_t ofs) override;
};


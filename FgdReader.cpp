#include "pre.h"
#include "strlib.h"
#include "FgdReader.h"
#include "TextFileReader.h"
#include <map>

// while parsing fgd base() defs, multiple baseclasses will be requested from the fgdreader
	// cache these when parsed or we'll be here a while
// fgd field type discoveries are added to the list
	// int/flags/choice from fgd override float from qc (and targetname overrides string)
// gotta be able to union spawnflags types

void FgdReader::ReadFromPath(const std::string& path)
{
}

void FgdReader::GetTypes(std::map<std::string, EPairType>& epf)
{
}

void FgdReader::GetClassnames(StringViewList& clist)
{
}

void FgdReader::GetDefinition(EntClass& ec)
{
}

size_t FgdReader::GetEntityBlock(const std::string_view qc, size_t ofs)
{
	return size_t();
}

size_t FgdReader::GetField(const std::string_view qc, size_t ofs)
{
	return size_t();
}

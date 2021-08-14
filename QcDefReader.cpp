#include "pre.h"
#include "strlib.h"
#include "IO.h"
#include "QcDefReader.h"
#include "TextFileReader.h"
#include "Tokenizer.h"
#include "EntClass.h"
#include "syslib.h"
#include <list>
#include <map>
#include <regex>


void QcDefReader::ReadFromPath(const std::string& path)
{
	std::list<std::string> files;
	int star = path.find('*');
	if (star != -1)
	{
		IO::AllWithWildcard(path, files);
	}
	else
	{
		files.push_back(path);
	}

	// parse each file enough to index definitions by name as blocks of text
	for (auto file : files)
	{
		TextFileReader qcReader;
		std::string &qc = contents.emplace_back("");
		size_t mark;

		qcReader.Open(file);
		qcReader.ReadAll(qc);

		mark = 0;
		while (true)
		{
			mark = qc.find("/*QUAKED ", mark);
			if (mark == -1)
				break;
			mark = GetEntityBlock(qc, mark + 9);
		}
		
		mark = 0;
		// loop again and get field definitions too
		// FIXME: avoid otherwise legal field definitions inside block comments
		std::basic_regex fieldRE("^\\s*?\\.(float|vector|string|entity|void)[^\n]+;");
		std::smatch m;
		if (!std::regex_match(qc, m, fieldRE))
			continue;
		for (int i = 0; i < m.size(); ++i)
		{
			GetField(qc, m.position(i));
		}
	}
}

void QcDefReader::GetTypes(std::map<std::string, EPairType>& epf)
{
	for (auto fpair : fields)
	{
		std::string name(fpair.first);
		EPairType dtype = epf[name];

		if ( dtype == EPairType::UNKNOWN ||
			// float/vec from qc override string from fgd because fgd doesn't have those types
			(dtype == EPairType::STRING && (fpair.second == EPairType::VEC3 || fpair.second == EPairType::FLOAT) ) )
		{
			epf[name] = fpair.second;
		}
	}
}

void QcDefReader::GetClassnames(StringViewList& clist)
{
	clist.reserve(clist.size() + blocks.size());

	for (const auto &pair : blocks)
		clist.push_back(pair.first);
}

size_t QcDefReader::GetEntityBlock(const std::string_view qc, size_t ofs)
{
	std::string_view classname, block;
	size_t sep, end;

	sep = qc.find(' ', ofs);
	classname = qc.substr(ofs, sep - ofs);

	end = qc.find("*/", sep);
	if (end == -1)
		Error(_S("EOF reached while parsing QUAKED comment for %s") << classname);

	sep += 1;
	blocks[classname] = qc.substr(sep, end - sep);
	return end + 2;
}

size_t QcDefReader::GetField(const std::string_view qc, size_t ofs)
{
	StringViewList tokens;
	size_t start, end;
	std::string_view qcsub;
	EPairType fieldType = EPairType::STRING;
	int nameOfs = 1;

	start = qc.find('.', ofs) + 1;
	end = qc.find(';', start);

	Tokenizer tokr(qcsub);
	tokr.SplitCode();
	tokr.All(tokens);

	if (tokens[0] == ".string")
		fieldType = EPairType::STRING;
	else if (tokens[0] == ".vector")
		fieldType = EPairType::VEC3;
	else if (tokens[0] == ".float")
		fieldType = EPairType::FLOAT;
	else if (tokens[0] == ".entity")
		// entity ref fields can be set in the map to entity numbers, which is janky but does work
		fieldType = EPairType::INT;
	else
	{
		Log::Warning(_S("Couldn't parse field definition '%s', skipping") << qcsub);
		return end + 1;
	}

	if (tokens[nameOfs] == "(")
	{
		// function reference, which will be a function name for our purposes
		fieldType = EPairType::STRING;
		while (tokens[nameOfs] != ")")
		{
			++nameOfs;
			if (nameOfs == tokens.size())
			{
				Log::Warning(_S("Couldn't parse field definition '%s', skipping") << qcsub);
				return end + 1;
			}
		}
		++nameOfs;
	}

	for (nameOfs; nameOfs < tokens.size(); ++nameOfs)
	{
		if (tokens[nameOfs] == "=")
			break;	// this is a field mask, don't need names after this one
		if (tokens[nameOfs] == ",")
			continue;
		fields[std::string(tokens[nameOfs])] = fieldType;
	}

	return end + 1;
}


/*
the classname, color triple, and bounding box are parsed out of comments
A ? size means take the exact brush size.

/*QUAKED <classname> (0 0 0) ?
/*QUAKED <classname> (0 0 0) (-8 -8 -8) (8 8 8)

Flag names can follow the size description:

/*QUAKED func_door (0 .5 .8) ? START_OPEN STONE_SOUND DOOR_DONT_LINK GOLD_KEY SILVER_KEY
*/

void QcDefReader::GetDefinition(EntClass& ec)
{
	std::string_view def(blocks[ec.name]), token;
	size_t mark = 0, nl = 0;
	Tokenizer tokr(def);
	tokr.ParentheticalTokens();

	token = tokr.Next();
	if (!strlib::StringToVec(token, ec.color))
		Error(_S("Couldn't parse entity definition for %s: no color triple") << ec.name);
	ec.texdef.Set(strlib::RGBToHex(ec.color));

	token = tokr.Next();
	if (token == "?")
	{
		ec.form = EntClass::ECF_BRUSH;
	}
	else if (strlib::StringToVec(token, ec.mins))
	{
		ec.form = EntClass::ECF_POINT;
		token = tokr.Next();
		if (!strlib::StringToVec(token, ec.maxs))
			Error(_S("Couldn't parse entity definition for %s: mins but no maxs") << ec.name);
	}
	else
	{
		Error(_S("Couldn't parse entity definition for %s: expected size or ? after color") << ec.name);
	}

	int f = 0;
	nl = def.find('\n', tokr.Position());
	while (tokr.Position() < nl)
	{
		token = tokr.Next();

		// skip little trash characters because they're placeholders for empty flags
		if (token.length() == 1 && !std::isalnum(token[0]))
		{
			++f;
			continue;
		}

		ec.flagnames[f] = token;

		if (++f >= EntClass::MAX_FLAGS)
			break;
	}

	// everything after the first line is explanatory text
	ec.comments = def.substr(nl + 1);
	Sys_TranslateString(ec.comments);
}

#ifdef BALLS
/*
==============
EntClassInitializer::InitFromText
==============
*/
EntClass* EntClassInitializer::InitFromText(char* text)
{
	char* t;
	int			len;
	int			r, i;
	char* p, parms[256];
	EntClass* e;
	char		color[MAX_TEXNAME];

	e = new EntClass();

	text += strlen("/*QUAKED ");

	// grab the name
	text = COM_Parse(text);
	strcpy(e->name, g_szComToken);

	// grab the color, reformat as texture name
	r = sscanf(text, " (%f %f %f)", &e->color[0], &e->color[1], &e->color[2]);
	if (r != 3)
	{
		Sys_Printf("Error parsing: %s\n", e->name);
		delete e;
		return nullptr;
	}
	//sprintf(color, "#%1.3f %1.3f %1.3f", e->color[0], e->color[1], e->color[2]);
	//strcpy(e->texdef.name, color);
	rgbToHex(e->color, color);
	e->texdef.Set(color);	// make a solid color texture

	while (*text != ')')
	{
		if (!*text)
		{
			Sys_Printf("Error parsing: %s\n", e->name);
			delete e;
			return nullptr;
		}
		text++;
	}
	text++;

	// get the size	
	text = COM_Parse(text);
	if (g_szComToken[0] == '(')
	{	// parse the size as two vectors
		e->form = ECF_POINT;

		r = sscanf(text, "%f %f %f) (%f %f %f)",
			&e->mins[0], &e->mins[1], &e->mins[2],
			&e->maxs[0], &e->maxs[1], &e->maxs[2]);
		if (r != 6)
		{
			Sys_Printf("Error parsing: %s\n", e->name);
			delete e;
			return nullptr;
		}

		for (i = 0; i < 2; i++)
		{
			while (*text != ')')
			{
				if (!*text)
				{
					Sys_Printf("Error parsing: %s\n", e->name);
					delete e;
					return nullptr;
				}
				text++;
			}
			text++;
		}
	}
	else
	{	// use the brushes
		e->form = ECF_BRUSH;
	}

	// get the flags

	// copy to the first /n
	p = parms;
	while (*text && *text != '\n')
		*p++ = *text++;
	*p = 0;
	text++;

	// any remaining words are parm flags
	p = parms;
	for (i = 0; i < MAX_FLAGS; i++)
	{
		p = COM_Parse(p);
		if (!p)
			break;
		if (!strcmp(g_szComToken, "?"))		// lunaran - respect ? convention for unused spawnflags
		{
			e->flagnames[i][0] = 0;
			continue;
		}
		strcpy(e->flagnames[i], g_szComToken);
	}
	if (i < 8)
	{
		// .def doesn't include skill level flags, put them in manually
		strcpy(e->flagnames[8], "!Easy");
		strcpy(e->flagnames[9], "!Normal");
		strcpy(e->flagnames[10], "!Hard");
		strcpy(e->flagnames[11], "!Deathmatch");

		// TODO: specify default spawnflags in project file? ie for quoth/copper coop flags
	}

	// find the length until close comment
	for (t = text; t[0] && !(t[0] == '*' && t[1] == '/'); t++)
		;

	// copy the comment block out
	len = t - text;
	e->comments.resize(len + 1);
	memcpy(*e->comments, text, len);
#if 0
	for (i = 0; i < len; i++)
		if (text[i] == '\n')
			e->comments[i] = '\r';
		else
			e->comments[i] = text[i];
#endif
	e->comments[len] = 0;
	return e;
}
#endif
#include "pre.h"
#include "qe3.h"
#include "MapParser.h"
#include "Brush.h"
#include "Face.h"
#include "EPair.h"
#include "EntClass.h"
#include "Entity.h"

MapParser::MapParser(const std::string_view data) : 
	tokr(data),
	schema(MapSchema::STANDARD)
{
	tokr.DoubleQuotedTokens();
}

MapParser::~MapParser() {}

void MapParser::Read(Brush& blist, Entity& elist)
{
	while (tokr.Next() != "")
	{
		if (tokr.Current() != "{")
			Error(_S("Couldn't parse map file (expected { on line %d, got %s)") << tokr.CurrentLine() << tokr.Current());

		ParseEntity(blist, elist);
	}
}

void MapParser::ParseEntity(Brush& blist, Entity& elist)
{
	Entity* e = new Entity();

	while (!tokr.Next().empty())
	{
		if (tokr.Current() == "{")
		{
			// it's a brush
			ParseBrush(blist, *e);
		}
		else if (tokr.Current() == "}")
		{
			// entity is over
			e->SetSpawnflagFilter();
			e->AddToList(&elist);
			return;
		}
		else
		{
			// keyvalue
			ParseEPair(*e);
		}
	}
	Error(_S("Couldn't parse map file (EOF before closing brace on line %d)") << tokr.CurrentLine());
}

void MapParser::ParseEPair(Entity& e)
{
	// TODO: all this verification should be done by the tokenizer by passing it expectations
	std::string_view key = tokr.Current();
	if (!tokr.CurrentInQuotes())
	{
		Error(_S("Couldn't parse map file (expected quoted key on line %d, got %s)") << tokr.CurrentLine() << tokr.Current());
	}
	else if (key.empty())
	{
		Log::Warning(_S("Empty quoted key on line %d") << tokr.CurrentLine());
		return;
	}

	std::string_view val = tokr.Next();
	if (val.empty())
	{
		if (tokr.CurrentInQuotes())
		{
			Log::Warning(_S("Empty quoted value on line %d") << tokr.CurrentLine());
			return;
		}
		Error(_S("Couldn't parse map file (EOF before quoted value on line %d)") << tokr.CurrentLine());
	}
	else if (!tokr.CurrentInQuotes())
	{
		Error(_S("Couldn't parse map file (expected quoted value on line %d, got %s)") << tokr.CurrentLine() << tokr.Current());
	}

	// create directly for speed rather than use SetKeyvalue, which checks for duplicates
	EPair* ep = new EPair(key, val);
	ep->next = e.epairs;
	e.epairs = ep;
}

void MapParser::ParseBrush(Brush& blist, Entity& e)
{
	Brush* b = new Brush();

	tokr.ParentheticalTokens();
	while (!tokr.Next().empty())
	{
		if (tokr.Current() == "}")
			break;
		ParseFace(*b);
	}
	tokr.ParentheticalTokens(false);

	// add to the end of the entity chain
	e.LinkBrush(b, true);
	b->AddToList(blist);
}

void MapParser::ParseFace(Brush& b)
{
	Face* f = new Face();
	std::string ftk;

	// read the three point plane definition
	for (int i = 0; i < 3; ++i)
	{
		if (!tokr.CurrentInParens() || !strlib::StringToVec(tokr.Current(), f->plane.pts[i]))
		{
			Error(_S("Couldn't parse map (bad plane point '%s' on line %d)") << tokr.Current() << tokr.CurrentLine());
		}
		tokr.Next();
	}
	f->plane.Make();

	// current token should be a texture name
	f->texdef.SetTemp(tokr.Current());	// apply directly without setting because no wads are loaded yet

	ftk = tokr.Next();

	// SOMEDAY: test face definition first, then decide if the map is std or v220
	if (ftk == "[")
		Error("Couldn't parse map (Valve220 format is not supported yet :( )");

	f->texdef.shift[0] = std::stof(ftk);
	ftk = tokr.Next();
	f->texdef.shift[1] = std::stof(ftk);
	ftk = tokr.Next();
	f->texdef.rotate = std::stof(ftk);
	ftk = tokr.Next();
	f->texdef.scale[0] = std::stof(ftk);
	ftk = tokr.Next();
	f->texdef.scale[1] = std::stof(ftk);
	// TODO: error check any of this

	// add the brush to the end of the chain, so loading and saving a map doesn't reverse the order
	if (b.faces)
	{
		Face *scan = b.faces;
		while (scan->fnext)
		{
			assert(scan != scan->fnext);
			assert(scan->fnext != f);
			scan = scan->fnext;
		}
		scan->fnext = f;
	}
	else
		b.faces = f;
}

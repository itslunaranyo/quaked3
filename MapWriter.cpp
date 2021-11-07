#include "pre.h"
#include "qedefs.h"
#include "cfgvars.h"
#include "strlib.h"
#include "StringFormatter.h"
#include "MapWriter.h"
#include "Map.h"
#include "Entity.h"
#include "EPair.h"
#include "Brush.h"
#include "Face.h"
#include "select.h"

#include <fstream>
#include <algorithm>

// TODO: something better than ostringstream

MapWriter::MapWriter() : writtenEnts(0), writtenBrushes(0), writtenEntBrushes(0)
{
}

MapWriter::~MapWriter()
{
}

void MapWriter::WriteMap(Map& map, std::ostream& out, const int subset)
{
	int count = 0;
	if (subset == WRITE_ALL)	// ignore the test and write everything
	{
		WriteCompleteEntity(*map.world, map, out);
		for (Entity* e = map.entities.Next(); e != &map.entities; e = e->Next())
			WriteCompleteEntity(*e, map, out);
		return;
	}

	std::vector<Brush*> brToWrite;
	brToWrite.reserve(128);
	if (subset & (int)BrushListFlag::SELECTED)
	{
		for (Brush* b = g_brSelectedBrushes.Next(); b != &g_brSelectedBrushes; b = b->Next())
			brToWrite.push_back(b);
	}
	if (subset & (int)BrushListFlag::ACTIVE)
	{
		for (Brush* b = g_map.brActive.Next(); b != &g_map.brActive; b = b->Next())
			brToWrite.push_back(b);
	}
	/*
	// brushes hidden by regioning currently only need to be written during a WRITE_ALL
	if (subset & (int)BrushListFlag::REGIONED)
	{
		for (Brush* b = g_map.brRegioned.Next(); b != &g_map.brRegioned; b = b->Next())
			brToWrite.push_back(b);
	}*/

	// a brush ent can't be assumed to be contiguous because brushes are in the selected 
	// list in order of selection, so put everything in order
	std::sort(brToWrite.begin(), brToWrite.end(), 
		[](const Brush* a, const Brush* b) -> bool {return a->owner < b->owner; });
	// (stability/retaining the map brush/entity order aren't really important except
	// for the full-document save/load)

	auto brWrIt = brToWrite.begin();
	while (brWrIt != brToWrite.end())
	{
		count = 0;
		auto b = brWrIt;
		Entity* e = (*b)->owner;
		while (brWrIt != brToWrite.end() && (*brWrIt)->owner == e)
		{
			++brWrIt;
			++count;
		}
		WritePartialEntity(*e, map, out, b, count);
	}
}

void MapWriter::WriteCompleteEntity(Entity& e, Map& map, std::ostream& out)
{
	OpenEntity(e, out);

	if (e.IsBrush())
	{
		writtenEntBrushes = 0;
		for (Brush* b = e.brushes.ENext(); b != &e.brushes; b = b->ENext())
			WriteBrush(*b, out);
	}

	CloseEntity(out);
}

void MapWriter::WritePartialEntity(Entity& e, Map& map, std::ostream& out, std::vector<Brush*>::iterator brIt, int count)
{
	OpenEntity(e, out);
	if (e.IsBrush())
	{
		writtenEntBrushes = 0;
		for (int i = 0; i < count; i++)
		{
			Brush& b = **(brIt++);
			WriteBrush(b, out);
		}
	}
	CloseEntity(out);
}

void MapWriter::OpenEntity(Entity& e, std::ostream& out)
{
	if (writtenEnts > 0)
		out << "// entity " << writtenEnts << "\n{\n";
	else
		out << "{\n";
	for (EPair* ep = e.epairs; ep; ep = ep->next)
		WriteEPair(*ep, out);
}

void MapWriter::CloseEntity(std::ostream& out)
{
	out << "}\n";
	++writtenEnts;
}

void MapWriter::WriteEPair(EPair& ep, std::ostream& out)
{
	out << "\"" << ep.GetKey() << "\" \"" << ep.GetValue() << "\"\n";
}

void MapWriter::WriteBrush(Brush& b, std::ostream& out)
{
	out << "// brush " << writtenEntBrushes++ << "\n{\n";
	for (Face* f = b.faces; f; f = f->fnext)
	{
		WriteFace(*f, out);
	}
	out << "}\n";
	++writtenBrushes;
}

void MapWriter::WriteFace(Face& f, std::ostream& out)
{
	int i;
	for (i = 0; i < 3; i++)
		if (g_cfgEditor.BrushPrecision)
			out << "( " << f.plane.pts[i][0] << " " << f.plane.pts[i][1] << " " << f.plane.pts[i][2] << " ) ";
		else
			out << "( " << (int)f.plane.pts[i][0] << " " << (int)f.plane.pts[i][1] << " " << (int)f.plane.pts[i][2] << " ) ";

	out << (f.texdef.Name().empty() ? "unnamed" : f.texdef.Name());

	out << " " << f.texdef.shift[0] << " " << f.texdef.shift[1] << " " << f.texdef.rotate << " ";

	if (f.texdef.scale[0] == (int)f.texdef.scale[0])
		out << (int)f.texdef.scale[0] << " ";
	else
		out << f.texdef.scale[0] << " ";

	if (f.texdef.scale[1] == (int)f.texdef.scale[1])
		out << (int)f.texdef.scale[1] << " ";
	else
		out << f.texdef.scale[1] << " ";

	out << "\n";
}

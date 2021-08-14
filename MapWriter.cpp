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

// TODO: something better than ostringstream

MapWriter::MapWriter() : writtenEnts(0), writtenBrushes(0), writtenEntBrushes(0)
{
}

MapWriter::~MapWriter()
{
}

void MapWriter::WriteMap(Map& map, std::ostream& out, const int subset)
{
	WriteEntity(*map.world, map, out, subset);

	for (Entity* e = map.entities.Next(); e != &map.entities; e = e->Next())
		WriteEntity(*e, map, out, subset);
}

void MapWriter::WriteEntity(Entity& e, Map& map, std::ostream& out, const int subset)
{
	if (subset != WRITE_ALL)	// ignore the test and write everything
	{
		// do the currently expensive list test
		if (!ShouldWrite(e, map, subset))
			return;
	}

	if (writtenEnts > 0)
		out << "// entity " << writtenEnts << "\n{\n";
	else
		out << "{\n";
	for (EPair* ep = e.epairs; ep; ep = ep->next)
		WriteEPair(*ep, out);

	if (e.IsBrush())
	{
		writtenEntBrushes = 0;
		for (Brush* b = e.brushes.ENext(); b != &e.brushes; b = b->ENext())
		{
			if (subset == WRITE_ALL || ShouldWrite(*b, map, subset))	// NOT ideal
				WriteBrush(*b, out);
		}
	}
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

	out << (f.texdef.name[0] ? f.texdef.name : "unnamed");

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

bool MapWriter::ShouldWrite(Entity& e, Map& map, const int subset)
{
	for (Brush* b = e.brushes.Next(); b != &e.brushes; b = b->Next())
	{
		if (ShouldWrite(*b, map, subset))
			return true;
	}
	return false;
}

bool MapWriter::ShouldWrite(Brush& b, Map& map, const int subset)
{
	// TODO: this is the worst
	if (subset & (int)BrushListFlag::SELECTED)
	{
		if (b.IsOnList(g_brSelectedBrushes))
			return true;
	}
	if (subset & (int)BrushListFlag::ACTIVE)
	{
		if (b.IsOnList(map.brActive))
			return true;
	}
	if (subset & (int)BrushListFlag::REGIONED)
	{
		if (b.IsOnList(map.brRegioned))
			return true;
	}

	return false;
}



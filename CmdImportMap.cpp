//==============================
//	CmdImportMap.cpp
//==============================

#include "qe3.h"
#include "CmdImportMap.h"

CmdImportMap::CmdImportMap() : Command("Import Map")
{
	selectOnDo = true;
}

void CmdImportMap::File(const char *fname)
{
	filename.resize(strlen(fname) + 1);
	strcpy((char*)*filename, fname);
	state = LIVE;
}

//==============================

void CmdImportMap::Do_Impl()
{
	bool bSnapCheck;
	Brush *b, *next;
	Entity *e, *enext;
	qeBuffer buf;
	Brush blist;
	Entity elist;

	if (!filename.size() || filename[0] == 0)
		Error("No filename specified for import");

	// sikk---> make sure Grid Snap is off to insure complex brushes remain intact
	if (!g_qeglobals.d_savedinfo.bNoClamp)
	{
		g_qeglobals.d_savedinfo.bNoClamp = true;
		bSnapCheck = true;
	}
	// <---sikk
//	g_qeglobals.d_nParsedBrushes = 0;

	blist.CloseLinks();
	elist.CloseLinks();

	IO_LoadFile((char*)*filename, buf);
	g_map.Read((char*)*buf, blist, elist);

	for (Entity *ent = elist.next; ent != &elist; ent = ent->next)
	{
		// world brushes need to be merged into the existing worldspawn
		if (ent->eclass == EntClass::worldspawn)
		{
			ent->RemoveFromList();
			for (b = ent->brushes.onext; b != &ent->brushes; b = next)
			{
				next = b->onext;
				b->owner->UnlinkBrush(b);
				b->owner = g_map.world;
			}
			delete ent;
			break;
		}
	}

	g_map.BuildBrushData(blist);

	if (bSnapCheck)	// sikk - turn Grid Snap back on if it was on before map load
		g_qeglobals.d_savedinfo.bNoClamp = false;

	// delink the lists to feed them to CmdAddRemove
	// lunaran FIXME: this is stupid and parse should change instead
	for (b = blist.next; b != &blist; b = next)
	{
		next = b->next;
		if (b->owner->IsBrush())
			b->onext = b->oprev = nullptr;
		b->RemoveFromList();
		cmdAR.AddedBrush(b);
	}
	for (e = elist.next; e != &elist; e = enext)
	{
		enext = e->next;
		e->RemoveFromList();
		cmdAR.AddedEntity(e);
	}

	cmdAR.Do();
}

void CmdImportMap::Undo_Impl()
{
	cmdAR.Undo();
}

void CmdImportMap::Redo_Impl()
{
	cmdAR.Redo();
}

void CmdImportMap::Sel_Impl()
{
	cmdAR.Select();
}



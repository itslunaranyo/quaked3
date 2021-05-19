//==============================
//	CmdPaste.cpp
//==============================

#include "qe3.h"

CmdPaste::CmdPaste() : Command("Paste")
{
	selectOnDo = true;
	state = LIVE;
}

//==============================

void CmdPaste::Do_Impl()
{
	if (!IsClipboardFormatAvailable(CF_TEXT))
		Error("Couldn't paste from clipboard");
	if (!OpenClipboard(g_qeglobals.d_hwndMain))
		Error("Couldn't paste from clipboard");

	HGLOBAL hglb;
	char*	cbdata;
	bool bSnapCheck;

	hglb = GetClipboardData(CF_TEXT);
	if (!hglb)
	{
		CloseClipboard();
		Error("Couldn't paste from clipboard");
	}

	cbdata = (char*)GlobalLock(hglb);
	if (!cbdata || cbdata[0] != '{')	// no opening brace = definitely not map data
	{
		GlobalUnlock(hglb);
		CloseClipboard();
		Error("Couldn't paste from clipboard");
	}


	Brush *b, *next;
	Entity *e, *enext;
	Brush blist;
	Entity elist;

	// sikk---> make sure Grid Snap is off to insure complex brushes remain intact
	if (!g_qeglobals.d_savedinfo.bNoClamp)
	{
		g_qeglobals.d_savedinfo.bNoClamp = true;
		bSnapCheck = true;
	}
	// <---sikk
	g_qeglobals.d_nParsedBrushes = 0;

	blist.CloseLinks();
	elist.CloseLinks();

	g_map.Read(cbdata, blist, elist);

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

	GlobalUnlock(hglb);
	CloseClipboard();

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

void CmdPaste::Undo_Impl()
{
	cmdAR.Undo();
}

void CmdPaste::Redo_Impl()
{
	cmdAR.Redo();
}

void CmdPaste::Sel_Impl()
{
	cmdAR.Select();
}



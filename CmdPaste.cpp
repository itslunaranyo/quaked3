//==============================
//	CmdPaste.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "CmdPaste.h"
#include "map.h"

CmdPaste::CmdPaste() : Command("Paste")
{
	selectOnDo = true;
	modifiesSelection = true;
	state = LIVE;
}

//==============================

void CmdPaste::Do_Impl()
{
	if (!IsClipboardFormatAvailable(CF_TEXT))
		CmdError("Couldn't paste from clipboard");
	if (!OpenClipboard(g_hwndMain))
		CmdError("Couldn't paste from clipboard");

	HGLOBAL hglb;
	char*	cbdata;
	bool bSnapCheck;

	hglb = GetClipboardData(CF_TEXT);
	if (!hglb)
	{
		CloseClipboard();
		CmdError("Couldn't paste from clipboard");
	}

	cbdata = (char*)GlobalLock(hglb);
	if (!cbdata)
	{
		GlobalUnlock(hglb);
		CloseClipboard();
		CmdError("Couldn't paste from clipboard");
	}

	Brush *b, *next;
	Entity *e, *enext;
	Brush blist;
	Entity elist;

	// sikk---> make sure Grid Snap is off to insure complex brushes remain intact
	if (g_qeglobals.bGridSnap)
	{
		g_qeglobals.bGridSnap = false;
		bSnapCheck = true;
	}
	// <---sikk
//	g_qeglobals.d_nParsedBrushes = 0;

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
		g_qeglobals.bGridSnap = true;

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
		if (e->IsBrush())
			e->brushes.onext = e->brushes.oprev = e->brushes.next;
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



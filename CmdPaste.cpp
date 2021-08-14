//==============================
//	CmdPaste.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "MapParser.h"
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

	MapParser parser(cbdata);
	try
	{
		parser.Read(blist, elist);
	}
	catch (qe3_exception)
	{
		while (elist.Next() != &elist)
			delete elist.Next();
		GlobalUnlock(hglb);
		CloseClipboard();
		CmdError("Couldn't paste from clipboard");
	}

	for (Entity *ent = elist.Next(); ent != &elist; ent = ent->Next())
	{
		enext = ent->Next();
		bool has_brushes = (ent->brushes.ENext() != &ent->brushes);
		ent->eclass = EntClass::ForName(ent->GetKeyValue("classname"), has_brushes, false);

		if (ent->eclass->IsPointClass())
		{	// create a custom brush
			ent->MakeBrush()->AddToList(blist);
		}

		// world brushes need to be merged into the existing worldspawn
		if (ent->IsWorld())
		{
			ent->RemoveFromList();
			for (b = ent->brushes.ENext(); b != &ent->brushes; b = next)
			{
				next = b->ENext();
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
	for (b = blist.Next(); b != &blist; b = next)
	{
		next = b->Next();
		b->RemoveFromList();
		if (b->owner->IsBrush() && !b->owner->IsWorld())
		{
			Entity::UnlinkBrush(b, true);
		}
		cmdAR.AddedBrush(b);
	}
	for (e = elist.Next(); e != &elist; e = enext)
	{
		enext = e->Next();
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



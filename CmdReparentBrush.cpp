//==============================
//	CmdReparentBrush.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "map.h"
#include "select.h"
#include "CmdReparentBrush.h"

CmdReparentBrush::CmdReparentBrush() : newowner(nullptr), Command("Reparent Brush")
{
	modifiesSelection = true;
}

CmdReparentBrush::~CmdReparentBrush()
{
	if (state != DONE) return;

	for (auto entIt = entRemoved.begin(); entIt != entRemoved.end(); ++entIt)
		delete *entIt;
}

void CmdReparentBrush::Destination(Entity *dest)
{
	newowner = dest;
	// noop until actual brushes are added
}

void CmdReparentBrush::AddBrush(Brush *br)
{
	if (br->owner == newowner) return;

	if (newowner->IsPoint())
	{
		state = NOOP;
		CmdError("Can't add brushes to a fixed size entity");
	}
	if (br->owner->IsPoint())
	{
		state = NOOP;
		CmdError("Can't reparent a fixed size entity");
	}

	reparents.emplace_back(br);

	state = LIVE;
}

void CmdReparentBrush::AddBrushes(Brush *brList)
{
	for (Brush* br = brList->next; br != brList; br = br->next)
		AddBrush(br);
}

//==============================

void CmdReparentBrush::Do_Impl()
{
	for (auto rpIt = reparents.begin(); rpIt != reparents.end(); ++rpIt)
	{
		// if this is the last brush in the entity, remove the entity too
		if (rpIt->br->onext == rpIt->br->oprev && !rpIt->br->owner->IsWorld())
		{
			Entity* own = rpIt->br->owner;
			entRemoved.push_back(own);
			Warning("%s lost all its brushes and will be deleted", own->eclass->name);
		}

		Entity::UnlinkBrush(rpIt->br);
		newowner->LinkBrush(rpIt->br);
	}

	for (auto entIt = entRemoved.begin(); entIt != entRemoved.end(); ++entIt)
		(*entIt)->RemoveFromList();
}

void CmdReparentBrush::Undo_Impl()
{
	for (auto entIt = entRemoved.begin(); entIt != entRemoved.end(); ++entIt)
		(*entIt)->AddToList(&g_map.entities);
	for (auto rpIt = reparents.begin(); rpIt != reparents.end(); ++rpIt)
	{
		Entity::UnlinkBrush(rpIt->br);
		rpIt->oldowner->LinkBrush(rpIt->br);
	}
}

void CmdReparentBrush::Redo_Impl()
{
	for (auto rpIt = reparents.begin(); rpIt != reparents.end(); ++rpIt)
	{
		Entity::UnlinkBrush(rpIt->br);
		newowner->LinkBrush(rpIt->br);
	}
	for (auto entIt = entRemoved.begin(); entIt != entRemoved.end(); ++entIt)
		(*entIt)->RemoveFromList();
}

void CmdReparentBrush::Sel_Impl()
{
	for (auto rpIt = reparents.begin(); rpIt != reparents.end(); ++rpIt)
	{
		rpIt->br->RemoveFromList();
		rpIt->br->AddToList(&g_brSelectedBrushes);
	}
}



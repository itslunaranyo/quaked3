//==============================
//	CmdAddRemove.cpp
//==============================

#include "qe3.h"
#include <algorithm>

// brush entities are always delinked (even if 100% of the brushes in an entity
// were deleted) for simplicity's sake. their owner pointers are kept as markers
// for restoration.
// point entity fixedsize brushes are never unlinked from their owner entities.

CmdAddRemove::CmdAddRemove() : Command("Add/Remove")
{
}

// cmd has been killed off the end of a queue forever, clean up sequestered data
CmdAddRemove::~CmdAddRemove()
{
	// perma-delete whichever of added/removed brushes are unlinked from the map
	if (state == DONE)		// delete the removed brushes
	{
		Delete(brRemoved);
		Delete(entRemoved);
	}
	if (state == UNDONE || state == LIVE )	// delete the added brushes
	{
		Delete(brAdded);
		Delete(entAdded);
	}
}

void CmdAddRemove::RemovedBrush(Brush *br)
{
	assert(br);
	assert(std::find(brAdded.begin(), brAdded.end(), br) == brAdded.end());
	assert(std::find(brRemoved.begin(), brRemoved.end(), br) == brRemoved.end());
	assert(state == LIVE || state == NOOP);

	if (br->owner->IsPoint())
		entRemoved.push_back(br->owner);
	else
		brRemoved.push_back(br);
	state = LIVE;
}

void CmdAddRemove::RemovedBrushes(Brush *brList)
{
	for (Brush *br = brList->next; br != brList; br = br->next)
		RemovedBrush(br);
}

void CmdAddRemove::RemovedBrushes(std::vector<Brush*> &brList)
{
	for (auto brIt = brList.begin(); brIt != brList.end(); ++brIt)
		RemovedBrush(*brIt);
}


void CmdAddRemove::AddedBrush(Brush *br)
{
	assert(br);
	assert(std::find(brAdded.begin(), brAdded.end(), br) == brAdded.end());
	assert(std::find(brRemoved.begin(), brRemoved.end(), br) == brRemoved.end());
	assert(state == LIVE || state == NOOP);

	if (br->owner->IsPoint())
		entAdded.push_back(br->owner);
	else
		brAdded.push_back(br);
	state = LIVE;
}

void CmdAddRemove::AddedBrushes(std::vector<Brush*>& brList)
{
	for (auto brIt = brList.begin(); brIt != brList.end(); ++brIt)
		AddedBrush(*brIt);
}

void CmdAddRemove::RemovedEntity(Entity *ent)
{
	assert(ent);
	// TODO: not have to perform this search every time
	auto entIt = std::find(entRemoved.begin(), entRemoved.end(), ent);
	if (entIt == entRemoved.end())
		entRemoved.push_back(ent);
	state = LIVE;
}

void CmdAddRemove::AddedEntity(Entity *ent)
{
	assert(ent);
	auto entIt = std::find(entAdded.begin(), entAdded.end(), ent);
	if (entIt == entAdded.end())
		entAdded.push_back(ent);
	state = LIVE;
}

void CmdAddRemove::AddedEntities(std::vector<Entity*>& entList)
{
	for (auto eIt = entList.begin(); eIt != entList.end(); ++eIt)
		AddedEntity(*eIt);
}

// ========================================================================

void CmdAddRemove::Do_Impl()
{
	Sequester(brRemoved);

	// check if removing all the brushes we've been fed leads to any empty entities
	// and mark them for removal too
	Brush* br;
	for (auto brIt = brRemoved.begin(); brIt != brRemoved.end(); ++brIt)
	{
		br = *brIt;
		if (br->owner->brushes.next == &br->owner->brushes &&
			br->owner->GetEntClass() != EntClass::worldspawn)	// never delete worldspawn
		{
			RemovedEntity(br->owner);
		}
	}
	Sequester(entRemoved);
	Restore(entAdded);
	Restore(brAdded);
}


void CmdAddRemove::Sequester(std::vector<Brush*>& brList)
{
	Brush* br;
	Entity* own;

	for (auto brIt = brList.begin(); brIt != brList.end(); ++brIt)
	{
		br = *brIt;
		br->RemoveFromList();	// unlink from scene

		own = br->owner;
		// unlink brush from owner but don't lose the pointer, we'll need it on restore
		Entity::UnlinkBrush(br);
		br->owner = own;
	}
}

void CmdAddRemove::Sequester(std::vector<Entity*> &entList)
{
	Entity* ent;
	for (auto entIt = entList.begin(); entIt != entList.end(); ++entIt)
	{
		ent = *entIt;
		ent->RemoveFromList();
		if (ent->IsPoint())
			ent->brushes.onext->RemoveFromList();
	}
}

void CmdAddRemove::Restore(std::vector<Brush*>& brList)
{
	Brush* br;

	for (auto brIt = brList.begin(); brIt != brList.end(); ++brIt)
	{
		br = *brIt;
		br->AddToList(&g_map.brActive);	// put back in scene
		br->owner->LinkBrush(br);
	}
}

void CmdAddRemove::Restore(std::vector<Entity*> &entList)
{
	Entity* ent;
	for (auto entIt = entList.begin(); entIt != entList.end(); ++entIt)
	{
		ent = *entIt;
		ent->AddToList(&g_map.entities);
		if (ent->IsPoint())
			ent->brushes.onext->AddToList(&g_map.brActive);
	}
}

void CmdAddRemove::Delete(std::vector<Brush*>& brList)
{
	for (auto brIt = brList.begin(); brIt != brList.end(); ++brIt)
	{
		delete *brIt;
	}
}

void CmdAddRemove::Delete(std::vector<Entity*> &entList)
{
	for (auto entIt = entList.begin(); entIt != entList.end(); ++entIt)
		delete *entIt;
}

// ========================================================================

void CmdAddRemove::Undo_Impl()
{
	Sequester(brAdded);
	Sequester(entAdded);
	Restore(entRemoved);
	Restore(brRemoved);
}

void CmdAddRemove::Redo_Impl()
{
	Sequester(brRemoved);
	Sequester(entRemoved);
	Restore(entAdded);
	Restore(brAdded);
}

void CmdAddRemove::Sel_Impl()
{
	if (state == DONE)
	{
		for (auto brIt = brAdded.begin(); brIt != brAdded.end(); ++brIt)
			Selection::SelectBrush(*brIt);
		for (auto entIt = entAdded.begin(); entIt != entAdded.end(); ++entIt)
			Selection::SelectBrush((*entIt)->brushes.onext);
		return;
	}
	for (auto brIt = brRemoved.begin(); brIt != brRemoved.end(); ++brIt)
		Selection::SelectBrush(*brIt);
	for (auto entIt = entRemoved.begin(); entIt != entRemoved.end(); ++entIt)
		Selection::SelectBrush((*entIt)->brushes.onext);
}


//==============================
//	CmdImportMap.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "CmdImportMap.h"
#include "CmdSetKeyvalue.h"
#include "map.h"

CmdImportMap::CmdImportMap() : Command("Import Map")
{
	modifiesSelection = true;
	selectOnDo = true;
	mergewads = false;
	adjusttargets = false;
	mergekvs = KVM_IGNORE;
}

void CmdImportMap::MergeWads(bool merge)
{
	if (state == LIVE || state == NOOP)
		mergewads = merge;
	state = LIVE;
}

void CmdImportMap::AdjustTargets(bool inc)
{
	if (state == LIVE || state == NOOP)
		adjusttargets = inc;
	state = LIVE;
}

void CmdImportMap::MergeWorldKeys(worldKVMerge merge)
{
	if (state == LIVE || state == NOOP)
		mergekvs = merge;
	state = LIVE;
}

void CmdImportMap::File(const std::string& fname)
{
	if (state != LIVE && state != NOOP) return;

	filename = fname;
	state = LIVE;
}

//==============================

struct targetLink
{
	targetLink(const std::string& n);

	std::string name;
	std::vector<EPair*> pairs;

	void incrementName();
};

targetLink::targetLink(const std::string& n)
{
	name = n;
}

void targetLink::incrementName()
{
	int increment;
	char* num;
	num = name.data();
	num += strlen(num);
	while (*(num - 1) >= '0' && *(num - 1) <= '9' && (num - 1) != name.data())
		--num;

	if (*num == 0)
	{	// name has no number at the end, so bless it with the holy 2
		increment = 2;
	}
	else
	{
		sscanf(num, "%i", &increment);
		increment++;
	}
	sprintf(num, "%i", increment); // this may get a digit or two longer, but name has 8 characters of padding
}

// ==============================================

void CmdImportMap::Do_Impl()
{
	bool bSnapCheck;
	Brush *b, *next;
	Entity *e, *enext;
	qeBuffer buf;
	Brush blist;
	Entity elist;
	CmdSetKeyvalue *cmdKV;

	std::vector<const char*> mapnames;
	std::vector<targetLink*> collisions;

	if (!filename.size() || filename[0] == 0)
		CmdError("No filename specified for import");

	// sikk---> make sure Grid Snap is off to insure complex brushes remain intact
	if (g_qeglobals.bGridSnap)
	{
		g_qeglobals.bGridSnap = false;
		bSnapCheck = true;
	}
	// <---sikk

	g_map.LoadFromFile(filename, elist, blist);

	// list all the unique target/targetname values in the current map
	if (adjusttargets)
	{
		for (Entity *ent = g_map.entities.Next(); ent != &g_map.entities; ent = ent->Next())
		{
			for (EPair* kv = ent->epairs; kv; kv = kv->next)
			{
				if (kv->IsTarget() || kv->IsTargetName())
				{
					bool dupe = false;
					for (auto mnIt = mapnames.begin(); mnIt != mapnames.end(); ++mnIt)
					{
						if (*mnIt == kv->GetValue())
						{
							dupe = true;
							break;
						}
					}
					if (!dupe)
						mapnames.push_back(kv->GetValue().c_str());
				}
			}
		}
	}


	// process the imported entities for addition to the scene
	for (Entity *ent = elist.Next(); ent != &elist; ent = enext)
	{
		enext = ent->Next();
		bool has_brushes = (ent->brushes.ENext() != &ent->brushes);
		ent->eclass = EntClass::ForName(ent->GetKeyValue("classname"), has_brushes, false);

		if (ent->eclass->IsPointClass())
		{	// create a custom brush
			ent->MakeBrush()->AddToList(blist);
		}

		// world brushes and keyvalues need to be merged into the existing worldspawn
		if (ent->IsWorld())
		{
			ent->RemoveFromList();
			for (b = ent->brushes.ENext(); b != &ent->brushes; b = next)
			{
				next = b->ENext();
				b->owner->UnlinkBrush(b);
				b->owner = g_map.world;
			}

			// merge imported worldspawn keyvalues into main worldspawn
			for (EPair* kv = ent->epairs; kv; kv = kv->next)
			{
				// merge the wad key somewhat intelligently
				if (kv->GetKey() == "wad" && mergewads)
				{
					std::string outwadkey;
					std::string worldwads = g_map.world->GetKeyValue("wad");
					Textures::MergeWadStrings(worldwads, kv->GetValue(), outwadkey);
					cmdKV = new CmdSetKeyvalue("wad", outwadkey);
					cmdKV->AddEntity(g_map.world);
					cmdWorldKVs.push_back(cmdKV);
				}
				// do a simple keep/reject on the rest
				else 
				{
					if (mergekvs == KVM_IGNORE) continue;
					if (mergekvs == KVM_OVERWRITE ||
						mergekvs == KVM_ADD && g_map.world->GetKeyValue(kv->GetKey())[0] == 0)
					{
						cmdKV = new CmdSetKeyvalue(kv->GetKey(), kv->GetValue());
						cmdKV->AddEntity(g_map.world);
						cmdWorldKVs.push_back(cmdKV);
					}
				}
			}
			delete ent;
		}
		// find target/targetname keys in the import that collide with any in the current map
		else if (adjusttargets)
		{
			targetLink* tlink = nullptr;
			for (EPair* kv = ent->epairs; kv; kv = kv->next)
			{
				if (!kv->IsTarget() && !kv->IsTargetName())
					continue;

				// o(n2) complexity here we come
				for (auto mnIt = mapnames.begin(); mnIt != mapnames.end(); ++mnIt)
				{
					if (*mnIt == kv->GetValue())
					{
						// now find it in the collision list, or add it
						for (auto ctlIt = collisions.begin(); ctlIt != collisions.end(); ++ctlIt)
						{
							if ((*ctlIt)->name == kv->GetValue())
								tlink = *ctlIt;
						}
						if (!tlink)
						{
							tlink = new targetLink(kv->GetValue());
							collisions.push_back(tlink);
						}
						tlink->pairs.push_back(kv);
					}
				}
			}
		}
	}

	// fix up collisions
	if (adjusttargets)
	{
		for (auto ctlIt = collisions.begin(); ctlIt != collisions.end(); ++ctlIt)
		{
			// increment the name until we resolve the collision
			bool free = false;
			while (!free)
			{
				free = true;
				(*ctlIt)->incrementName();
				for (auto mnIt = mapnames.begin(); mnIt != mapnames.end(); ++mnIt)
				{
					if (strcmp((*ctlIt)->name.c_str(), *mnIt))
						continue;
					free = false;
				}
			}

			for (auto epIt = (*ctlIt)->pairs.begin(); epIt != (*ctlIt)->pairs.end(); ++epIt)
			{
				(*epIt)->SetValue((*ctlIt)->name.c_str());
			}

			// don't reuse the incremented name elsewhere
			mapnames.push_back((*ctlIt)->name.c_str());
		}
	}

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
	for (auto cmdKVit = cmdWorldKVs.begin(); cmdKVit != cmdWorldKVs.end(); ++cmdKVit)
		(*cmdKVit)->Do();

	g_map.SanityCheck();
}

void CmdImportMap::Undo_Impl()
{
	for (auto cmdKVit = cmdWorldKVs.begin(); cmdKVit != cmdWorldKVs.end(); ++cmdKVit)
		(*cmdKVit)->Undo();
	cmdAR.Undo();
}

void CmdImportMap::Redo_Impl()
{
	cmdAR.Redo();
	for (auto cmdKVit = cmdWorldKVs.begin(); cmdKVit != cmdWorldKVs.end(); ++cmdKVit)
		(*cmdKVit)->Redo();
}

void CmdImportMap::Sel_Impl()
{
	cmdAR.Select();
}

// ==============================================


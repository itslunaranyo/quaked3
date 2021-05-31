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

void CmdImportMap::File(const char *fname)
{
	if (state != LIVE && state != NOOP) return;

	filename.resize(strlen(fname) + 1);
	strcpy(filename.c_str(), fname);
	state = LIVE;
}

//==============================

struct targetLink
{
	targetLink(const char* n);

	qeBuffer name;
	std::vector<EPair*> pairs;

	void incrementName();
};

targetLink::targetLink(const char *n)
{
	name.resize(strlen(n) + 8);
	strcpy(name.c_str(), n);
}

void targetLink::incrementName()
{
	int increment;
	char* num;
	num = name.c_str();
	num += strlen(num);
	while (*(num - 1) >= '0' && *(num - 1) <= '9' && (num - 1) != name.c_str())
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

	std::vector<char*> mapnames;
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

	blist.CloseLinks();
	elist.CloseLinks();

	IO_LoadFile(filename.c_str(), buf);
	g_map.Read(buf.c_str(), blist, elist);

	// list all the unique target/targetname values in the current map
	if (adjusttargets)
	{
		for (Entity *ent = g_map.entities.next; ent != &g_map.entities; ent = ent->next)
		{
			for (EPair* kv = ent->epairs; kv; kv = kv->next)
			{
				if (kv->IsTarget() || kv->IsTargetName())
				{
					bool dupe = false;
					for (auto mnIt = mapnames.begin(); mnIt != mapnames.end(); ++mnIt)
					{
						if (!strcmp(*mnIt, kv->value.c_str()))
						{
							dupe = true;
							break;
						}
					}
					if (!dupe)
						mapnames.push_back(kv->value.c_str());
				}
			}
		}
	}


	// process the imported entities for addition to the scene
	for (Entity *ent = elist.next; ent != &elist; ent = enext)
	{
		// world brushes and keyvalues need to be merged into the existing worldspawn
		enext = ent->next;
		if (ent->eclass == EntClass::worldspawn)
		{
			ent->RemoveFromList();
			for (b = ent->brushes.onext; b != &ent->brushes; b = next)
			{
				next = b->onext;
				b->owner->UnlinkBrush(b);
				b->owner = g_map.world;
			}

			// merge imported worldspawn keyvalues into main worldspawn
			for (EPair* kv = ent->epairs; kv; kv = kv->next)
			{
				// merge the wad key somewhat intelligently
				if (!strcmp(kv->key.c_str(), "wad") && mergewads)
				{
					char outwadkey[1024];
					char* worldwads = g_map.world->GetKeyValue("wad");
					Textures::MergeWadStrings(worldwads, kv->value.c_str(), outwadkey);
					cmdKV = new CmdSetKeyvalue("wad", outwadkey);
					cmdKV->AddEntity(g_map.world);
					cmdWorldKVs.push_back(cmdKV);
				}
				// do a simple keep/reject on the rest
				else 
				{
					if (mergekvs == KVM_IGNORE) continue;
					if (mergekvs == KVM_OVERWRITE ||
						mergekvs == KVM_ADD && g_map.world->GetKeyValue(kv->key.c_str())[0] == 0)
					{
						cmdKV = new CmdSetKeyvalue(kv->key.c_str(), kv->value.c_str());
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
					if (!strcmp(*mnIt, kv->value.c_str()))
					{
						// now find it in the collision list, or add it
						for (auto ctlIt = collisions.begin(); ctlIt != collisions.end(); ++ctlIt)
						{
							if (!strcmp((*ctlIt)->name.c_str(), kv->value.c_str()))
								tlink = *ctlIt;
						}
						if (!tlink)
						{
							tlink = new targetLink(kv->value.c_str());
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
	for (b = blist.next; b != &blist; b = next)
	{
		next = b->next;
		b->RemoveFromList();
		if (b->owner->IsBrush() && !b->owner->IsWorld())
		{
			e = b->owner;
			Entity::UnlinkBrush(b);
			b->owner = e;
		}
		cmdAR.AddedBrush(b);
	}
	for (e = elist.next; e != &elist; e = enext)
	{
		enext = e->next;
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


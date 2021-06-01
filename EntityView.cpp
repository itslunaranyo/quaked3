//==============================
//	EntityView.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "EntityView.h"
#include "select.h"


EntityView::EntityView()
{
	Reset();
}


EntityView::~EntityView()
{
}

EntityView::entEpair_t* EntityView::FindKV(const char *key)
{
	for (auto epIt = vPairs.begin(); epIt != vPairs.end(); ++epIt)
	{
		if (epIt->kv.key == key)
			return &(*epIt);
	}
	return nullptr;
}

/*
=========================
EntityView::UnionKV

add a keyvalue to the pool, properly flagging it as mixed if it clashes
=========================
*/
EntityView::entEpair_t* EntityView::UnionKV(const char *key, const char *val)
{
	entEpair_t* eep;
	eep = FindKV(key);
	if (!eep)
	{
		// key not in list, add it with this value
		vPairs.emplace_back();
		eep = &(vPairs.back());
		eep->kv.SetKey(key);
		eep->kv.SetValue(val);
		return eep;
	}
	if (eep->mixed)
	{
		// key is present and as mixed as it's going to get
		return eep;
	}
	if (eep->kv.value != val)
	{
		// key is present but value doesn't match, mark as mixed
		eep->kv.SetValue(mixedkv);
		eep->mixed = true;
	}
	// otherwise key and value match, so leave it alone
	return eep;
}

void EntityView::DeleteKV(const char *key)
{
	for (auto epIt = vPairs.begin(); epIt != vPairs.end(); ++epIt)
	{
		if (epIt->kv.key == key)
		{
			vPairs.erase(epIt);
			return;
		}
	}
}

void EntityView::Reset()
{
	vPairs.clear();
	vClass = NULL;
	vClassMixed = false;
	memset(vFlags, 0, MAX_FLAGS * sizeof(entFlag_t));
}

void EntityView::Refresh()
{
	Entity *eo;
	EPair *ep;
	bool first;

	Reset();

	eo = NULL;
	first = true;
	for (Brush* b = g_brSelectedBrushes.Next(); b != &g_brSelectedBrushes; b = b->Next())
	{
		// since brushes in the selection are (mostly) sorted by entity, this is a 
		// decent enough way to shuffle through them, but it does no harm if a 
		// fragmented brush entity contributes its own values more than once
		if (b->owner == eo)
			continue;
		eo = b->owner;

		UnionFlagNames(eo->eclass);
		for (ep = eo->epairs; ep; ep = ep->next)
		{
			if (ep->key == "spawnflags")
			{
				UnionFlags(eo->GetKeyValueInt("spawnflags"), first);
			}
			UnionKV(ep->key.c_str(), ep->value.c_str());
		}
		first = false;
	}
}

void EntityView::UnionFlags(int inFlags, bool first)
{
	int f;

	for (int i = 0; i < MAX_FLAGS; i++)
	{
		if (vFlags[i].state == EFS_MIXED)
			continue;

		f = !!(inFlags & (1 << i));
		if (first)
		{
			vFlags[i].state = (entFlagState_t)f;
		}
		else if (f != vFlags[i].state)
		{
			vFlags[i].state = EFS_MIXED;
		}
	}
}

void EntityView::UnionFlagNames(EntClass *ec)
{
	for (int i = 0; i < MAX_FLAGS; i++)
	{
		if (!*ec->flagnames[i])
			continue;
		if (!*vFlags[i].name)
		{
			strncpy_s(vFlags[i].name, ec->flagnames[i], 32);
			continue;
		}
		if (_stricmp(vFlags[i].name, ec->flagnames[i]))
		{
			strncpy(vFlags[i].name, "~\0", 2);
		}
	}
}

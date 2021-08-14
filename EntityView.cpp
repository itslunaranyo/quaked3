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

EntityView::entEpair_t* EntityView::FindKV(const std::string& key)
{
	for (auto epIt = vPairs.begin(); epIt != vPairs.end(); ++epIt)
	{
		if (epIt->kv.GetKey() == key)
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
EntityView::entEpair_t* EntityView::UnionKV(const std::string& key, const std::string& val)
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
	if (eep->kv.GetValue() != val)
	{
		// key is present but value doesn't match, mark as mixed
		eep->kv.SetValue(mixedkv);
		eep->mixed = true;
	}
	// otherwise key and value match, so leave it alone
	return eep;
}

void EntityView::DeleteKV(const std::string& key)
{
	for (auto epIt = vPairs.begin(); epIt != vPairs.end(); ++epIt)
	{
		if (epIt->kv.GetKey() == key)
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
	memset(vFlags, 0, EntClass::MAX_FLAGS * sizeof(entFlag_t));
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
			if (ep->GetKey() == "spawnflags")
			{
				UnionFlags(eo->GetKeyValueInt("spawnflags"), first);
			}
			UnionKV(ep->GetKey(), ep->GetValue());
		}
		first = false;
	}
}

void EntityView::UnionFlags(int inFlags, bool first)
{
	int f;

	for (int i = 0; i < EntClass::MAX_FLAGS; i++)
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
	for (int i = 0; i < EntClass::MAX_FLAGS; i++)
	{
		if (ec->flagnames[i].empty())
			continue;
		if (vFlags[i].name.empty())
		{
			vFlags[i].name = ec->flagnames[i];
			continue;
		}
		if (vFlags[i].name != ec->flagnames[i])
		{
			vFlags[i].name = "~";
		}
	}
}

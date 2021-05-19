//==============================
//	CmdSetKeyvalue.cpp
//==============================

#include "qe3.h"

CmdSetKeyvalue::CmdSetKeyvalue(const char *key, const char *value)
{
	if (!key[0])
		Error("No key specified");
	if (!strcmp(key, "classname") && !value[0])
		Error("Cannot delete classname keyvalue (it is important)");

	newKV.key.resize(strlen(key) + 1);
	newKV.value.resize(strlen(value) + 1);

	strcpy((char*)*newKV.key, key);
	strcpy((char*)*newKV.value, value);
}

CmdSetKeyvalue::~CmdSetKeyvalue() {}

void CmdSetKeyvalue::AddEntity(Entity *e)
{
	for (auto kvcIt = kvchanges.begin(); kvcIt != kvchanges.end(); ++kvcIt)
		if (kvcIt->ent == e)
			return;	// already added

	// cache the old keyvalues for undo
	EPair* ep;
	ep = e->GetEPair((char*)*newKV.key);
	if (ep)
		kvchanges.emplace_back(e, ep->value);
	else
		kvchanges.emplace_back(e, "");

	state = LIVE;
}

CmdSetKeyvalue::keyvalue_change_s::keyvalue_change_s(Entity *_e, const qeBuffer &_v) : ent(_e)
{
	val.resize(_v.size());
	strcpy((char*)*val, (char*)*_v);
}
CmdSetKeyvalue::keyvalue_change_s::keyvalue_change_s(Entity *_e, const char* _cv) : ent(_e)
{
	val.resize(strlen(_cv) + 1);
	strcpy((char*)*val, _cv);
}

//==============================

void CmdSetKeyvalue::SetNew()
{
	// special handling for the special keyvalues
	if (!strcmp((char*)*newKV.key, "classname"))
	{
		for (auto kvcIt = kvchanges.begin(); kvcIt != kvchanges.end(); ++kvcIt)
			if (!kvcIt->ent->IsWorld())
				kvcIt->ent->ChangeClassname((char*)*newKV.value);
		return;
	}

	if (!strcmp((char*)*newKV.key, "origin"))
	{
		for (auto kvcIt = kvchanges.begin(); kvcIt != kvchanges.end(); ++kvcIt)
		{
			if (!newKV.value[0])	// origin key can be deleted from brush ents but not points
			{
				if (kvcIt->ent->IsBrush())
					kvcIt->ent->DeleteKeyValue((char*)*newKV.key);
				continue;
			}
			kvcIt->ent->SetKeyValue((char*)*newKV.key, (char*)*newKV.value);
			if (kvcIt->ent->IsPoint())
				kvcIt->ent->SetOriginFromKeyvalue();
		}
		return;
	}

	if (!newKV.value[0])	// no value specified, treat as delete
		for (auto kvcIt = kvchanges.begin(); kvcIt != kvchanges.end(); ++kvcIt)
			kvcIt->ent->DeleteKeyValue((char*)*newKV.key);
	else
		for (auto kvcIt = kvchanges.begin(); kvcIt != kvchanges.end(); ++kvcIt)
			kvcIt->ent->SetKeyValue((char*)*newKV.key, (char*)*newKV.value);
}

void CmdSetKeyvalue::SetOld()
{
	// special handling for the special keyvalues
	if (!strcmp((char*)*newKV.key, "classname"))
	{
		for (auto kvcIt = kvchanges.begin(); kvcIt != kvchanges.end(); ++kvcIt)
			if (!kvcIt->ent->IsWorld())
				kvcIt->ent->ChangeClassname((char*)*kvcIt->val);
		return;
	}

	if (!strcmp((char*)*newKV.key, "origin"))
	{
		for (auto kvcIt = kvchanges.begin(); kvcIt != kvchanges.end(); ++kvcIt)
		{
			if (!kvcIt->val[0])
			{
				kvcIt->ent->DeleteKeyValue((char*)*newKV.key);
				continue;
			}
			kvcIt->ent->SetKeyValue((char*)*newKV.key, (char*)*kvcIt->val);
			if (kvcIt->ent->IsPoint())
				kvcIt->ent->SetOriginFromKeyvalue();
		}
		return;
	}

	for (auto kvcIt = kvchanges.begin(); kvcIt != kvchanges.end(); ++kvcIt)
		if (!kvcIt->val[0])	// had no value before, delete kv again
			kvcIt->ent->DeleteKeyValue((char*)*newKV.key);
		else
			kvcIt->ent->SetKeyValue((char*)*newKV.key, (char*)*kvcIt->val);
}

void CmdSetKeyvalue::Do_Impl() { SetNew(); }
void CmdSetKeyvalue::Undo_Impl() { SetOld(); }
void CmdSetKeyvalue::Redo_Impl() { SetNew(); }
void CmdSetKeyvalue::Sel_Impl()
{
	for (auto kvcIt = kvchanges.begin(); kvcIt != kvchanges.end(); ++kvcIt)
		for (Brush *br = kvcIt->ent->brushes.onext; br != &kvcIt->ent->brushes; br = br->onext)
			Selection::SelectBrush(br);
}


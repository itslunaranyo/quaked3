//==============================
//	CmdSetKeyvalue.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "CmdSetKeyvalue.h"
#include "select.h"

CmdSetKeyvalue::CmdSetKeyvalue(const std::string& key, const std::string& value) : Command("Set Keyvalue")
{
	if (!key[0])
		CmdError("No key specified");
	if (key == "classname" && value.empty())
		CmdError("Cannot delete classname keyvalue (it is important)");

	newKV.Set(key, value);
}

CmdSetKeyvalue::~CmdSetKeyvalue() {}

void CmdSetKeyvalue::AddEntity(Entity *e)
{
	for (auto kvcIt = kvchanges.begin(); kvcIt != kvchanges.end(); ++kvcIt)
		if (kvcIt->ent == e)
			return;	// already added

	// cache the old keyvalues for undo
	EPair* ep;
	ep = e->GetEPair(newKV.GetKey());
	if (ep)
		kvchanges.emplace_back(e, ep->GetValue());
	else
		kvchanges.emplace_back(e, "");

	state = LIVE;
}

CmdSetKeyvalue::keyvalue_change_s::keyvalue_change_s(Entity* _e, const std::string& _v) : ent(_e)
{
	val = _v;
}
CmdSetKeyvalue::keyvalue_change_s::keyvalue_change_s(Entity* _e, const char* _cv) : ent(_e)
{
	val = _cv;
}

//==============================

/*
this command applies only one keyvalue to a group of entities on Do, but
because it can be overwriting many disparate values with a common one,
it has to handle reapplying these multiple unique keyvalues to each entity 
in that group on Undo
*/

void CmdSetKeyvalue::SetNew()
{
	// special handling for the special keyvalues
	if (newKV.GetKey() == "classname")
	{
		for (auto kvcIt = kvchanges.begin(); kvcIt != kvchanges.end(); ++kvcIt)
			if (!kvcIt->ent->IsWorld())
				kvcIt->ent->ChangeClassname(newKV.GetValue());
		return;
	}

	if (newKV.GetKey() == "origin")
	{
		for (auto kvcIt = kvchanges.begin(); kvcIt != kvchanges.end(); ++kvcIt)
		{
			if (newKV.GetValue().empty())	// origin key can be deleted from brush ents but not points
			{
				if (kvcIt->ent->IsBrush())
					kvcIt->ent->DeleteKeyValue("origin");
				continue;
			}
			kvcIt->ent->SetKeyValue("origin", newKV.GetValue());
			if (kvcIt->ent->IsPoint())
				kvcIt->ent->SetOriginFromKeyvalue();
		}
		return;
	}

	if (newKV.GetValue().empty())	// no value specified, treat as delete
		for (auto kvcIt = kvchanges.begin(); kvcIt != kvchanges.end(); ++kvcIt)
			kvcIt->ent->DeleteKeyValue(newKV.GetKey());
	else
		for (auto kvcIt = kvchanges.begin(); kvcIt != kvchanges.end(); ++kvcIt)
			kvcIt->ent->SetKeyValue(newKV.GetKey(), newKV.GetValue());
}

void CmdSetKeyvalue::SetOld()
{
	// special handling for the special keyvalues
	if (newKV.GetKey() == "classname")
	{
		for (auto kvcIt = kvchanges.begin(); kvcIt != kvchanges.end(); ++kvcIt)
			if (!kvcIt->ent->IsWorld())
				kvcIt->ent->ChangeClassname(kvcIt->val);
		return;
	}

	if (newKV.GetKey() == "origin")
	{
		for (auto kvcIt = kvchanges.begin(); kvcIt != kvchanges.end(); ++kvcIt)
		{
			if (kvcIt->val.empty())
			{
				kvcIt->ent->DeleteKeyValue("origin");
				continue;
			}
			kvcIt->ent->SetKeyValue("origin", kvcIt->val);
			if (kvcIt->ent->IsPoint())
				kvcIt->ent->SetOriginFromKeyvalue();
		}
		return;
	}

	for (auto kvcIt = kvchanges.begin(); kvcIt != kvchanges.end(); ++kvcIt)
		if (kvcIt->val.empty())	// had no value before, delete kv again
			kvcIt->ent->DeleteKeyValue(newKV.GetKey());
		else
			kvcIt->ent->SetKeyValue(newKV.GetKey(), kvcIt->val);
}

void CmdSetKeyvalue::Do_Impl() { SetNew(); }
void CmdSetKeyvalue::Undo_Impl() { SetOld(); }
void CmdSetKeyvalue::Redo_Impl() { SetNew(); }
void CmdSetKeyvalue::Sel_Impl()
{
	for (auto kvcIt = kvchanges.begin(); kvcIt != kvchanges.end(); ++kvcIt)
		for (Brush *br = kvcIt->ent->brushes.ENext(); br != &kvcIt->ent->brushes; br = br->ENext())
			Selection::SelectBrush(br);
}


//==============================
//	CmdSetSpawnflag.cpp
//==============================

#include "qe3.h"

CmdSetSpawnflag::CmdSetSpawnflag(int flag, bool val) : newval(val), Command("Set Spawnflag")
{
	if (flag < 1 || flag > 32)
		Error("Spawnflag out of range (valid: 1-32, passed: %i)", flag);

	flagnum = 1 << (flag - 1);
}

CmdSetSpawnflag::~CmdSetSpawnflag() {}

CmdSetSpawnflag::spawnflag_change_s::spawnflag_change_s(Entity * _e, bool ov) : ent(_e), oldval(ov) {}

void CmdSetSpawnflag::AddEntity(Entity *e)
{
	for (auto sfcIt = sfchanges.begin(); sfcIt != sfchanges.end(); ++sfcIt)
		if (sfcIt->ent == e)
			return;	// already added

	// cache the old bit state for undo
	bool ov;
	ov = !!(flagnum & e->GetKeyValueInt("spawnflags"));
	sfchanges.emplace_back(e, ov);

	state = LIVE;
}

//==============================

void CmdSetSpawnflag::SetNew()
{
	for (auto sfcIt = sfchanges.begin(); sfcIt != sfchanges.end(); ++sfcIt)
		sfcIt->ent->SetSpawnFlag(flagnum, newval);
}

void CmdSetSpawnflag::SetOld()
{
	for (auto sfcIt = sfchanges.begin(); sfcIt != sfchanges.end(); ++sfcIt)
		sfcIt->ent->SetSpawnFlag(flagnum, sfcIt->oldval);
}

void CmdSetSpawnflag::Do_Impl() { SetNew(); }
void CmdSetSpawnflag::Undo_Impl() { SetOld(); }
void CmdSetSpawnflag::Redo_Impl() { SetNew(); }

void CmdSetSpawnflag::Sel_Impl()
{
	for (auto sfcIt = sfchanges.begin(); sfcIt != sfchanges.end(); ++sfcIt)
		for (Brush *br = sfcIt->ent->brushes.onext; br != &sfcIt->ent->brushes; br = br->onext)
			Selection::SelectBrush(br);
}


//==============================
//	cmdbrushmod.cpp
//==============================

#include "qe3.h"

CmdBrushMod::CmdBrushMod()
{
}

CmdBrushMod::~CmdBrushMod()
{
	if (state == LIVE)
		Swap();	// we aren't done, so restore the geometry that was backed up

	// now manually delete whichever set of brush geometry we don't want
	for (auto brbIt = brbasisCache.begin(); brbIt != brbasisCache.end(); ++brbIt)
		brbIt->br_basis.clear();
}

void CmdBrushMod::brbasis_pair_s::swap()
{
	Brush::brbasis_s temp;

	// =assigning the basis copies pointers but doesn't clone faces
	temp = br->basis;
	br->basis = br_basis;
	br_basis = temp;

	// important: null this pointer, because the swapped faces belong to someone else
	temp.faces = nullptr;
}


// call BEFORE modifying a brush - clones its geometry into storage outside the scene
void CmdBrushMod::ModifyBrush(Brush *br)
{
	assert(state == LIVE || state == NOOP);

	for (auto brbIt = brbasisCache.begin(); brbIt != brbasisCache.end(); ++brbIt)
		if (brbIt->br == br)
			return;	// must avoid duplicates in the list, or duplicate restores will stomp each other
			
	brbasisCache.emplace_back();
	brbasis_pair_t &basispair = brbasisCache.back();

	basispair.br = br;
	br->CopyBasis(basispair.br_basis);
	state = LIVE;
}

void CmdBrushMod::ModifyBrushes(Brush *brList)
{
	for (Brush* b = brList->next; b != brList; b = b->next)
		ModifyBrush(b);
}

void CmdBrushMod::RestoreBrush(Brush *br)
{
	assert(state == LIVE || state == NOOP);

	for (auto brbIt = brbasisCache.begin(); brbIt != brbasisCache.end(); ++brbIt)
	{
		if (brbIt->br == br)
		{
			br->basis.clear();	// delete modified geometry
			br->basis = brbIt->br_basis.clone();	// recopy backup geometry
			break;
		}
	}
}

void CmdBrushMod::RestoreBrushes(Brush *brList)
{
	for (Brush* b = brList->next; b != brList; b = b->next)
		RestoreBrush(b);
}

void CmdBrushMod::RestoreAll()
{
	assert(state == LIVE || state == NOOP);

	for (auto brbIt = brbasisCache.begin(); brbIt != brbasisCache.end(); ++brbIt)
	{
		brbIt->br->basis.clear();	// delete modified geometry
		brbIt->br->basis = brbIt->br_basis.clone();	// recopy backup geometry
	}
	state = NOOP;
}

// only call if the brush hasn't been changed after all and you don't need the backup
void CmdBrushMod::UnmodifyBrush(Brush *br)
{
	assert(state == LIVE || state == NOOP);

	for (auto brbIt = brbasisCache.begin(); brbIt != brbasisCache.end(); ++brbIt)
	{
		if (brbIt->br == br)
		{
			// deletes entry as well as the cloned geometry in the basis
			brbasisCache.erase(brbIt);
			break;
		}
	}
	if (brbasisCache.size() == 0)
		state = NOOP;
}

void CmdBrushMod::UnmodifyBrushes(Brush *brList)
{
	for (Brush* b = brList->next; b != brList; b = b->next)
		UnmodifyBrush(b);
}

void CmdBrushMod::UnmodifyAll()
{
	assert(state == LIVE || state == NOOP);

	// deletes entry as well as the cloned geometry in the basis
	brbasisCache.clear();

	state = NOOP;
}

// call if you screwed up a brush and you want things to go back to the way they were
void CmdBrushMod::RevertBrush(Brush *br)
{
	assert(state == LIVE || state == NOOP);

	for (auto brbIt = brbasisCache.begin(); brbIt != brbasisCache.end(); ++brbIt)
	{
		if (brbIt->br == br)
		{
			brbIt->swap();	// switch old geometry back into the scene first
			brbasisCache.erase(brbIt);	// deletes entry as well as the swapped garbage
			break;
		}
	}
	if (brbasisCache.size() == 0)
		state = NOOP;
}

void CmdBrushMod::RevertBrushes(Brush *brList)
{
	for (Brush* b = brList->next; b != brList; b = b->next)
		RevertBrush(b);
}

void CmdBrushMod::RevertAll()
{
	assert(state == LIVE || state == NOOP);

	for (auto brbIt = brbasisCache.begin(); brbIt != brbasisCache.end(); ++brbIt)
		brbIt->swap();	// switch old geometry back into the scene first

	brbasisCache.clear();

	state = NOOP;
}

//==============================

void CmdBrushMod::Swap()
{
	for (auto brbIt = brbasisCache.begin(); brbIt != brbasisCache.end(); ++brbIt)
	{
		brbIt->swap();
		brbIt->br->Build();
	}
}

void CmdBrushMod::Do_Impl() {}	// nothing to do here, changes were already made outside this command

// undo and redo are the same: switch all sequestered geometry with the scene geometry
void CmdBrushMod::Undo_Impl() { Swap(); }
void CmdBrushMod::Redo_Impl() { Swap(); }

void CmdBrushMod::Sel_Impl()
{
	for (auto brbIt = brbasisCache.begin(); brbIt != brbasisCache.end(); ++brbIt)
		Selection::SelectBrush(brbIt->br);
}


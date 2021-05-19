//==============================
//	cmdbrushmod.cpp
//==============================

#include "qe3.h"

CmdBrushMod::CmdBrushMod() : Command("Brush Mod")
{
}

CmdBrushMod::~CmdBrushMod()
{
	if (state == LIVE)
		SwapAll();	// we aren't done, so restore the geometry that was backed up

	// now manually delete whichever set of brush geometry we don't want
	for (auto brbIt = brushCache.begin(); brbIt != brushCache.end(); ++brbIt)
		brbIt->clear();
}


CmdBrushMod::brBasis_s::brBasis_s(Brush *bOrig) :
	br(bOrig), mins(bOrig->mins), maxs(bOrig->maxs), faces(nullptr)
{
	Face *f, *nf;
	// important safety tip: this produces a face list in reverse order
	// but you weren't trying to index a linked list by position anyway, right?
	for (f = bOrig->faces; f; f = f->fnext)
	{
		nf = f->Clone();	// clones texdef and plane, but not winding
		nf->fnext = faces;
		faces = nf;
	}
}

CmdBrushMod::brBasis_s::~brBasis_s()
{
	Face *f, *fnext;
	for (f = faces; f; f = fnext)
	{
		fnext = f->fnext;
		delete f;
	}
}

void CmdBrushMod::brBasis_s::clear()
{
	// free faces
	Face *f, *fnext;
	for (f = faces; f; f = fnext)
	{
		fnext = f->fnext;
		delete f;
	}
	faces = nullptr;
}

//================================================================

void CmdBrushMod::Swap(brBasis_t &brb)
{
	/*
	vec3 tmpv;
	Face* tmpf;

	// swap pointers but don't copy faces
	tmpf = brb.br->faces;
	brb.br->faces = brb.faces;
	brb.faces = tmpf;

	tmpv = brb.br->mins;
	brb.br->mins = brb.mins;
	brb.mins = tmpv;
	tmpv = brb.br->maxs;
	brb.br->maxs = brb.maxs;
	brb.maxs = tmpv;
	*/

	std::swap(brb.br->faces, brb.faces);
	std::swap(brb.br->mins, brb.mins);
	std::swap(brb.br->maxs, brb.maxs);
}

void CmdBrushMod::SwapAll()
{
	for (auto brbIt = brushCache.begin(); brbIt != brushCache.end(); ++brbIt)
	{
		Swap(*brbIt);
	}
}

//================================================================

// call BEFORE modifying a brush - clones its geometry, then moves existing geometry
// into storage outside the scene
void CmdBrushMod::ModifyBrush(Brush *br)
{
	assert(state == LIVE || state == NOOP);

	for (auto brbIt = brushCache.begin(); brbIt != brushCache.end(); ++brbIt)
		if (brbIt->br == br)
			return;	// must avoid duplicates, or restores will stomp each other
			
	brushCache.emplace_back(br);

	// the brush in the scene must be left pointing to the cloned set of Faces
	/*
	Face *f = br->faces;
	br->faces = brushCache.back().faces;
	brushCache.back().faces = f;
	*/
	std::swap(brushCache.back().faces, br->faces);
	// the original faces are those saved in storage, so that if an undo happens the
	// faces which prior undo steps point to in memory are the ones which are restored
	// to the scene. commands and tools that wrap CmdBrushMod must be aware of this
	// and act accordingly.

	state = LIVE;
}

void CmdBrushMod::ModifyBrushes(Brush *brList)
{
	for (Brush* b = brList->next; b != brList; b = b->next)
		ModifyBrush(b);
}

//================================================================

void CmdBrushMod::RestoreBrush(Brush *br)
{
	assert(state == LIVE);

	for (auto brbIt = brushCache.begin(); brbIt != brushCache.end(); ++brbIt)
	{
		if (brbIt->br == br)
		{
			Face *f, *nf;
			br->ClearFaces();	// delete modified geometry

			// recopy backup geometry
			for (f = brbIt->faces; f; f = f->fnext)
			{
				nf = f->Clone();	// clones texdef and plane, but not winding
				nf->fnext = br->faces;
				br->faces = nf;
			}
			br->Build();
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
	assert(state == LIVE);

	for (auto brbIt = brushCache.begin(); brbIt != brushCache.end(); ++brbIt)
	{
		Face *f, *nf;
		brbIt->br->ClearFaces();	// delete modified geometry

		// recopy backup geometry
		for (f = brbIt->faces; f; f = f->fnext)
		{
			nf = f->Clone();	// clones texdef and plane, but not winding
			nf->fnext = brbIt->br->faces;
			brbIt->br->faces = nf;
		}
		brbIt->br->Build();
		break;
	}
	//state = NOOP;
}

//================================================================

// call if you screwed up a brush and you want things to go back to the way they were
void CmdBrushMod::RevertBrush(Brush *br)
{
	assert(state == LIVE);

	for (auto brbIt = brushCache.begin(); brbIt != brushCache.end(); ++brbIt)
	{
		if (brbIt->br == br)
		{
			Swap(*brbIt);	// switch original geometry back into the scene first
			br->Build();
			brushCache.erase(brbIt);	// deletes entry and takes the swapped garbage with it
			break;
		}
	}
	if (brushCache.empty())
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
	if (brushCache.empty())
		return;

	SwapAll();
	brushCache.clear();
	state = NOOP;
}

//==============================

void CmdBrushMod::Do_Impl() {}	// nothing to do here, changes were already made outside this command

// undo and redo are the same: switch all sequestered geometry with the scene geometry
void CmdBrushMod::Undo_Impl() { SwapAll(); }
void CmdBrushMod::Redo_Impl() { SwapAll(); }

void CmdBrushMod::Sel_Impl()
{
	for (auto brbIt = brushCache.begin(); brbIt != brushCache.end(); ++brbIt)
		Selection::SelectBrush(brbIt->br);
}


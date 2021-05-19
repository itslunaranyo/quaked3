//==============================
//	cmdbrushmod.cpp
//==============================

#include "qe3.h"
#include "CmdBrushMod.h"

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
	Face *of, *nf;

	// important safety tip: this produces a face list in reverse order
	for (of = bOrig->faces; of; of = of->fnext)
	{
		nf = of->Clone();	// clones texdef and plane, but not winding
		nf->d_color = of->d_color;	// or noops will turn the brush black

		// give the existing windings back, ours will need to be rebuilt on an undo anyway
		std::swap(of->face_winding, nf->face_winding);
		//f->face_winding = nullptr;

		nf->fnext = faces;
		faces = nf;
	}

	// reverse the list again
	of = faces;
	faces = nullptr;
	while (of)
	{
		nf = of->fnext;
		of->fnext = faces;
		faces = of;
		of = nf;
	}

	// the brush in the scene must be left pointing to the cloned set of Faces
	std::swap(faces, bOrig->faces);
	// the original faces are those saved in storage, so that if an undo happens the
	// faces which prior undo steps point to in memory are the ones which are restored
	// to the scene. commands and tools that wrap CmdBrushMod must be aware of this
	// and act accordingly.
}

CmdBrushMod::brBasis_s::~brBasis_s()
{
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
	std::swap(brb.br->faces, brb.faces);
	std::swap(brb.br->mins, brb.mins);
	std::swap(brb.br->maxs, brb.maxs);
	brb.br->FullBuild();
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
void CmdBrushMod::ModifyBrush(Brush *brIn)
{
	assert(state == LIVE || state == NOOP);

	for (auto brbIt = brushCache.begin(); brbIt != brushCache.end(); ++brbIt)
		if (brbIt->br == brIn)
			return;	// must avoid duplicates, or restores will stomp each other
	
	// moves br's faces into storage, gives us back clones of those faces, but
	// with the faces' original windings
	brushCache.emplace_back(brIn);
	
	state = LIVE;
}

void CmdBrushMod::ModifyBrushes(Brush *brList)
{
	for (Brush* b = brList->next; b != brList; b = b->next)
		ModifyBrush(b);
}

void CmdBrushMod::ModifyBrushes(std::vector<Brush*> brList)
{
	for (auto brbIt = brList.begin(); brbIt != brList.end(); ++brbIt)
		ModifyBrush(*brbIt);
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
			br->FullBuild();
			break;
		}
	}
}

void CmdBrushMod::RestoreBrushes(Brush *brList)
{
	for (Brush* b = brList->next; b != brList; b = b->next)
		RestoreBrush(b);
}

void CmdBrushMod::RestoreBrushes(std::vector<Brush*> brList)
{
	for (auto brbIt = brList.begin(); brbIt != brList.end(); ++brbIt)
		RestoreBrush(*brbIt);
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
		brbIt->br->FullBuild();
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
			br->FullBuild();
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

void CmdBrushMod::RevertBrushes(std::vector<Brush*> brList)
{
	for (auto brbIt = brList.begin(); brbIt != brList.end(); ++brbIt)
		RevertBrushes(*brbIt);
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


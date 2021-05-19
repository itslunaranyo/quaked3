//==============================
//	CmdBrushClip.cpp
//==============================

#include "qe3.h"

CmdBrushClip::CmdBrushClip() : side(CLIP_FRONT), pointsSet(false)
{
	selectOnDo = true;
	selectOnUndo = true;
}

CmdBrushClip::~CmdBrushClip()
{
	if (state == LIVE)
		ClearSplitLists();

	// DONE and UNDONE have moved appropriate brushes into the CmdAddRemove lists,
	// so cmdAR's destructor is already taking care of those cases
}

void CmdBrushClip::StartBrushes(Brush *brList)
{
	// only allow setting the list once; if the selection list changes, the tool
	// can destroy the command and create a new one
	assert(state == NOOP);
	for (Brush* br = brList->next; br != brList; br = br->next)
	{
		if (br->owner->IsPoint())
			continue;

		brIn.push_back(br);
		state = LIVE;
	}
}

void CmdBrushClip::SetPoints(vec3 p1, vec3 p2, vec3 p3)
{
	if (cp1 == p1 && cp2 == p2 && cp3 == p3)
		return;

	cp1 = p1;
	cp2 = p2;
	cp3 = p3;
	pointsSet = (VectorLength(CrossProduct(p1 - p2, p1 - p3)) > 0.1f);	// don't accept collinear points
	if (pointsSet)
		CreateSplitLists();
}

void CmdBrushClip::UnsetPoints()
{
	pointsSet = false;
	ClearSplitLists();
}

void CmdBrushClip::SetSide(clipside s)
{
	side = s;
}

//==============================

void CmdBrushClip::CreateSplitLists()
{
	if (!pointsSet)
		Error("Tried to 3-point clip without 3 points");

	ClearSplitLists();

	Brush *f, *b;

	Face splitf;
	splitf.texdef = g_qeglobals.d_workTexDef;
	//splitf.DEPtexture = Textures::ForName(splitf.texdef.name);
	splitf.plane.FromPoints(cp1, cp2, cp3);

	for (auto brIt = brIn.begin(); brIt != brIn.end(); ++brIt)
	{
		CSG::SplitBrushByFace(*brIt, &splitf, &f, &b);
		if (f)
			brFront.push_back(f);
		if (b)
			brBack.push_back(b);
	}
}

void CmdBrushClip::ClearSplitLists()
{
	for (auto brIt = brFront.begin(); brIt != brFront.end(); ++brIt)
		delete (*brIt);
	brFront.clear();

	for (auto brIt = brBack.begin(); brIt != brBack.end(); ++brIt)
		delete (*brIt);
	brBack.clear();
}

void CmdBrushClip::Do_Impl()
{
	cmdAR.RemovedBrushes(brIn);
	if (side == CLIP_FRONT || side == CLIP_BOTH)
		cmdAR.AddedBrushes(brFront);
	if (side == CLIP_BACK || side == CLIP_BOTH)
		cmdAR.AddedBrushes(brBack);

	cmdAR.Do();
}

void CmdBrushClip::Undo_Impl()
{
	cmdAR.Undo();
}

void CmdBrushClip::Redo_Impl()
{
	cmdAR.Redo();
}

void CmdBrushClip::Sel_Impl()
{
	cmdAR.Select();
}



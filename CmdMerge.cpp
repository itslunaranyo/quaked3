//==============================
//	CmdMerge.cpp
//==============================

#include "qe3.h"
#include "csg.h"
#include "CmdMerge.h"

CmdMerge::CmdMerge() : /*convex(false),*/ Command("CSG Merge")
{
	selectOnDo = true;
	selectOnUndo = true;
	modifiesSelection = true;
}

void CmdMerge::UseBrush(Brush *br)
{
	assert(br);
	assert(state == LIVE || state == NOOP);

	if (br->owner->IsPoint())
		Error("Can't merge a point entity");
	else if (!br->IsHidden())
		brMerged.push_back(br);

	state = LIVE;
}

void CmdMerge::UseBrushes(Brush *brList)
{
	for (Brush *br = brList->next; br != brList; br = br->next)
		UseBrush(br);
}

//==============================

void CmdMerge::Do_Impl()
{
	Brush *newbrush = CSG::DoMerge(brMerged, false);// , convex);
	if (!newbrush)
		Error("Brushes could not be merged");

	cmdAR.RemovedBrushes(brMerged);
	cmdAR.AddedBrush(newbrush);
	cmdAR.Do();
}

void CmdMerge::Undo_Impl() { cmdAR.Undo(); }
void CmdMerge::Redo_Impl() { cmdAR.Redo(); }
void CmdMerge::Sel_Impl() { cmdAR.Select(); }


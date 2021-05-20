//==============================
//	CmdBridge.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "csg.h"
#include "CmdBridge.h"
#include "select.h"

CmdBridge::CmdBridge() : Command("CSG Bridge")
{
	selectOnDo = true;
	selectOnUndo = true;
	modifiesSelection = true;
}

void CmdBridge::UseFace(Face *f)
{
	assert(f);
	assert(state == LIVE || state == NOOP);

	if (f->owner->owner->IsPoint())
		CmdError("Can't Bridge a point entity");
	else if (!f->owner->IsHidden())
		fBridged.push_back(f);

	state = LIVE;
}

void CmdBridge::UseFaces(std::vector<Face*> fList)
{
	for (auto fIt = fList.begin(); fIt != fList.end(); ++fIt)
		UseFace(*fIt);
}

//==============================

void CmdBridge::Do_Impl()
{
	Brush *newbrush = CSG::DoBridge(fBridged);
	if (!newbrush)
		CmdError("Faces could not be Bridged");

	cmdAR.AddedBrush(newbrush);
	cmdAR.Do();
}

void CmdBridge::Undo_Impl() { cmdAR.Undo(); }
void CmdBridge::Redo_Impl() { cmdAR.Redo(); }
void CmdBridge::Sel_Impl() { Selection::DeselectAllFaces(); cmdAR.Select(); }


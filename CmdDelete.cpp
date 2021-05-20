//==============================
//	CmdDelete.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "CmdDelete.h"

CmdDelete::CmdDelete(Brush* brList) : Command("Delete")
{
	selectOnUndo = true;
	modifiesSelection = true;

	Delete(brList);
	state = LIVE;
}

CmdDelete::~CmdDelete() {}

void CmdDelete::Delete(Brush *brList)
{
	cmdAR.RemovedBrushes(brList);
}

void CmdDelete::Do_Impl() { cmdAR.Do(); }
void CmdDelete::Undo_Impl() { cmdAR.Undo(); }
void CmdDelete::Redo_Impl() { cmdAR.Redo(); }
void CmdDelete::Sel_Impl() { cmdAR.Select(); }
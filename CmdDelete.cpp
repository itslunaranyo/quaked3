//==============================
//	CmdDelete.cpp
//==============================

#include "qe3.h"

CmdDelete::CmdDelete(Brush* brList)
{
	selectOnUndo = true;

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

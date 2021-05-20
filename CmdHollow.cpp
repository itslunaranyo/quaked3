//==============================
//	CmdHollow.cpp
//==============================

#include "qe3.h"
#include "csg.h"
#include "CmdHollow.h"

CmdHollow::CmdHollow() : Command("CSG Hollow")
{
	selectOnDo = true;
	selectOnUndo = true;
	modifiesSelection = true;
}

void CmdHollow::UseBrush(Brush *br)
{
	assert(br);
	assert(state == LIVE || state == NOOP);

	if (br->owner->IsPoint())
		CmdError("Can't hollow a point entity");
	else if (!br->IsHidden())
		brHollowed.push_back(br);

	state = LIVE;
}

void CmdHollow::UseBrushes(Brush *brList)
{
	for (Brush *br = brList->next; br != brList; br = br->next)
		UseBrush(br);
}

//==============================

void CmdHollow::Do_Impl()
{
	Face *f, *split;
	vec3 move;
	Brush *front;// , *back;

	for (auto brIt = brHollowed.begin(); brIt != brHollowed.end(); ++brIt)
	{
		Brush *br = *brIt;
		for (f = br->faces; f; f = f->fnext)
		{
			split = f->Clone();
			move = f->plane.normal * (double)g_qeglobals.d_nGridSize;
			split->plane.Translate(-move);

			CSG::SplitBrushByFace(br, split, &front);
			delete split;
			if (front)
				cmdAR.AddedBrush(front);
		}
		cmdAR.RemovedBrush(br);
	}
	cmdAR.Do();
}

void CmdHollow::Undo_Impl()
{
	cmdAR.Undo();
}

void CmdHollow::Redo_Impl()
{
	cmdAR.Redo();
}

void CmdHollow::Sel_Impl()
{
	cmdAR.Select();
}



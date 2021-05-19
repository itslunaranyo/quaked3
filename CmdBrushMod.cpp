#include "CmdBrushMod.h"


CmdBrushMod::CmdBrushMod()
{
}

CmdBrushMod::~CmdBrushMod()
{
}

void CmdBrushMod::ModifiedBrush(Brush * br)
{
	assert(state == LIVE || state == NOOP);
	state = LIVE;

	brbasisCache.emplace_back();
	brbasis_pair_t &basispair = brbasisCache.back();

	basispair.br = br;
	br->CopyBasis(basispair.br_basis);
}

void CmdBrushMod::ModifiedBrushes(Brush * brList)
{
	for (Brush* b = brList->next; b != brList; b = brList->next)
		ModifiedBrush(b);
}

void CmdBrushMod::Undo_Impl()
{
}

void CmdBrushMod::Redo_Impl()
{
}

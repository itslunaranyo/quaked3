//==============================
//	cmdbrushmod.h
//==============================

#ifndef __COMMAND_BRUSHMOD_H__
#define __COMMAND_BRUSHMOD_H__

#include "qe3.h"
#include <vector>

class CmdBrushMod : public Command
{
public:
	CmdBrushMod();
	~CmdBrushMod();

	typedef struct {
		Brush* br;
		Brush::brbasis_s br_basis;
	} brbasis_pair_t;

	std::vector<brbasis_pair_t> brbasisCache;

	void ModifiedBrush(Brush* br);
	void ModifiedBrushes(Brush* brList);

	void Undo_Impl();
	void Redo_Impl();
};

#endif
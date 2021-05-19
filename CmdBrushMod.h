//==============================
//	cmdbrushmod.h
//==============================

#ifndef __COMMAND_BRUSHMOD_H__
#define __COMMAND_BRUSHMOD_H__

#include <vector>

class CmdBrushMod : public Command
{
public:
	CmdBrushMod();
	~CmdBrushMod();

	typedef struct brbasis_pair_s {
		Brush* br;
		Brush::brbasis_s br_basis;
		void swap();
	} brbasis_pair_t;

	std::vector<brbasis_pair_t> brbasisCache;

	// clone off the geometry of these brushes and keep it
	void ModifyBrush(Brush* br);
	void ModifyBrushes(Brush* brList);

	// throw away cloned geometry from these brushes and stop tracking
	void UnmodifyBrush(Brush* br);
	void UnmodifyBrushes(Brush* brList);

	// restore cloned geometry from these brushes and stop tracking
	void RevertBrush(Brush* br);
	void RevertBrushes(Brush* brList);

private:
	void Swap();

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Select_Impl();
};

#endif
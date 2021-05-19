//==============================
//	cmdbrushmod.h
//==============================

#ifndef __COMMAND_BRUSHMOD_H__
#define __COMMAND_BRUSHMOD_H__

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

	// clone off the geometry of these brushes and keep it
	void ModifyBrush(Brush* br);
	void ModifyBrushes(Brush* brList);

	// restore cloned geometry from these brushes
	void RestoreBrush(Brush* br);
	void RestoreBrushes(Brush* brList);
	void RestoreAll();

	// throw away cloned geometry from these brushes and stop tracking - does NOT restore
	void UnmodifyBrush(Brush* br);
	void UnmodifyBrushes(Brush* brList);
	void UnmodifyAll();

	// restore cloned geometry from these brushes and stop tracking
	void RevertBrush(Brush* br);
	void RevertBrushes(Brush* brList);
	void RevertAll();

private:
	std::vector<brbasis_pair_t> brbasisCache;

	void Swap();

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Select_Impl();
};

#endif
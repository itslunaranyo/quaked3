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

	// clone off the geometry of these brushes and keep it
	// -> original faces wind up outside the scene <-
	void ModifyBrush(Brush* br);
	void ModifyBrushes(Brush* brList);

	// restore geometry for these brushes back to their state when Modify()ed
	// -> original faces stay outside the scene <-
	void RestoreBrush(Brush* br);
	void RestoreBrushes(Brush* brList);
	void RestoreAll();

	// restore cloned geometry from these brushes and stop tracking
	// -> original faces are put back in the scene <-
	void RevertBrush(Brush* br);
	void RevertBrushes(Brush* brList);
	void RevertAll();

private:
	typedef struct brBasis_s {
		brBasis_s(Brush *bOrig);
		~brBasis_s();
		Brush* br;
		Face *faces;
		vec3 mins, maxs;
		void clear();
	} brBasis_t;

	std::vector<brBasis_t> brushCache;

	void Swap(brBasis_t &brb);
	void SwapAll();

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Sel_Impl();
};

#endif
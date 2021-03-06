//==============================
//	cmdbrushmod.h
//==============================

#ifndef __COMMAND_BRUSHMOD_H__
#define __COMMAND_BRUSHMOD_H__

#include "Command.h"

/*
========================================================================

BRUSH MODIFICATION

For any change which modifies the geometry of any number of brushes,
without changing the number of brushes in the scene. 
- does not change the memory location of modified brushes
- replaces brush faces with duplicates to be further modified by some 
  outer command, but restores memory location of original faces on undo

========================================================================
*/

class CmdBrushMod : public Command
{
public:
	CmdBrushMod();
	~CmdBrushMod();

	// clone off the geometry of these brushes and keep it
	// -> original faces wind up outside the scene <-
	void ModifyBrush(Brush* br);
	void ModifyBrushes(Brush* brList);
	void ModifyBrushes(std::vector<Brush*> brList);

	// restore geometry for these brushes back to their state when Modify()ed
	// -> original faces stay outside the scene <-
	void RestoreBrush(Brush* br);
	void RestoreBrushes(Brush* brList);
	void RestoreBrushes(std::vector<Brush*> brList);
	void RestoreAll();

	// restore cloned geometry from these brushes and stop tracking
	// -> original faces are put back in the scene <-
	void RevertBrush(Brush* br);
	void RevertBrushes(Brush* brList);
	void RevertBrushes(std::vector<Brush*> brList);
	void RevertAll();

private:
	typedef struct brBasis_s {
		brBasis_s(Brush *bOrig);
		~brBasis_s();
		Brush *br;
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
//==============================
//	CmdMerge.h
//==============================

#ifndef __COMMAND_MERGE_H__
#define __COMMAND_MERGE_H__

#include "qe3.h"
#include "Command.h"
#include "CmdAddRemove.h"

class CmdMerge : public Command
{
public:
	CmdMerge();
	~CmdMerge() {};

	void UseBrush(Brush* br);
	void UseBrushes(Brush* brList);
	//void AllowConvex(bool acvx) { convex = acvx; }

	int BrushDelta() { return cmdAR.BrushDelta(); };
	int EntityDelta() { return cmdAR.EntityDelta(); };
private:
	CmdAddRemove cmdAR;
	std::vector<Brush*> brMerged;
	bool convex;

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Sel_Impl();

};

#endif	// __COMMAND_MERGE_H__

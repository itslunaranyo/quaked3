//==============================
//	CmdClone.h
//==============================

#ifndef __COMMAND_CLONE_H__
#define __COMMAND_CLONE_H__

#include "Command.h"
#include "CmdAddRemove.h"

class CmdClone : public Command
{
public:
	CmdClone(Brush* brList, const vec3 offset = { 0,0,0 });
	~CmdClone();

	int BrushDelta() { return cmdAR.BrushDelta(); };
	int EntityDelta() { return cmdAR.EntityDelta(); };
private:
	CmdAddRemove cmdAR;

	void Clone(Brush* brList, const vec3 offset);

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Sel_Impl();
};

#endif
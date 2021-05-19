//==============================
//	CmdClone.h
//==============================

#ifndef __COMMAND_DELETE_H__
#define __COMMAND_DELETE_H__

#include "Command.h"
#include "CmdAddRemove.h"

class Brush;

class CmdDelete : public Command
{
public:
	CmdDelete(Brush *brList);
	~CmdDelete();

	int BrushDelta() { return cmdAR.BrushDelta(); };
	int EntityDelta() { return cmdAR.EntityDelta(); };
private:
	CmdAddRemove cmdAR;

	void Delete(Brush* brList);

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Select_Impl();
};

#endif
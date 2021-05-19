//==============================
//	CmdCone.h
//==============================

#ifndef __COMMAND_CONE_H__
#define __COMMAND_CONE_H__

#include "Command.h"
#include "CmdBrushMod.h"

class CmdCone : public Command
{
public:
	CmdCone();
	~CmdCone();

	void SetSides(int s);
	void SetAxis(int ax);
	void UseBrush(Brush* br);
private:
	CmdBrushMod cmdBM;
	Brush *target;
	int sides;
	int axis;

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Sel_Impl();

};

#endif	// __COMMAND_CONE_H__

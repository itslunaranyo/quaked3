//==============================
//	CmdCylinder.h
//==============================

#ifndef __COMMAND_CYLINDER_H__
#define __COMMAND_CYLINDER_H__

#include "qe3.h"
#include "Command.h"
#include "CmdBrushMod.h"

class CmdCylinder : public Command
{
public:
	CmdCylinder();
	~CmdCylinder();

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

#endif	// __COMMAND_CYLINDER_H__

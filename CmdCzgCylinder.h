//==============================
//	CmdCzgCylinder.h
//==============================

#ifndef __COMMAND_CZG_CYLINDER_H__
#define __COMMAND_CZG_CYLINDER_H__

#include "Command.h"
#include "CmdBrushMod.h"

class Brush;

class CmdCzgCylinder : public Command
{
public:
	CmdCzgCylinder();
	~CmdCzgCylinder();

	void SetDegree(int d);
	void SetAxis(int ax);
	void UseBrush(Brush* br);
private:
	CmdBrushMod cmdBM;
	Brush *target;
	int degree;
	int axis;

	float *PatternForDegree(int d);

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Sel_Impl();

};

#endif	// __COMMAND_CZG_CYLINDER_H__

//==============================
//	CmdSphere.h
//==============================

#ifndef __COMMAND_SPHERE_H__
#define __COMMAND_SPHERE_H__

#include "qe3.h"
#include "Command.h"
#include "CmdBrushMod.h"

class CmdSphere : public Command
{
public:
	CmdSphere();
	~CmdSphere();

	void SetSides(int s);
	void UseBrush(Brush* br);
private:
	CmdBrushMod cmdBM;
	Brush *target;
	int sides;

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Sel_Impl();

};

#endif	// __COMMAND_SPHERE_H__

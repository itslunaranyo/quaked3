//==============================
//	CmdBridge.h
//==============================

#ifndef __COMMAND_Bridge_H__
#define __COMMAND_Bridge_H__

#include "qe3.h"
#include "Command.h"
#include "CmdAddRemove.h"

class CmdBridge : public Command
{
public:
	CmdBridge();
	~CmdBridge() {};

	void UseFace(Face* f);
	void UseFaces(std::vector<Face*> fList);

	int BrushDelta() { return cmdAR.BrushDelta(); };
	int EntityDelta() { return cmdAR.EntityDelta(); };
private:
	CmdAddRemove cmdAR;
	std::vector<Face*> fBridged;

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Sel_Impl();

};

#endif	// __COMMAND_Bridge_H__

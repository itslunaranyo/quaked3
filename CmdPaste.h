//==============================
//	CmdPaste.h
//==============================

#ifndef __COMMAND_PASTE_H__
#define __COMMAND_PASTE_H__

class CmdPaste : public Command
{
public:
	CmdPaste();
	~CmdPaste() {}

	int BrushDelta() { return cmdAR.BrushDelta(); };
	int EntityDelta() { return cmdAR.EntityDelta(); };
private:
	CmdAddRemove cmdAR;

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Sel_Impl();

};

#endif	// __COMMAND_PASTE_H__

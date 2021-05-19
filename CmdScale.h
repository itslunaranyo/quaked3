//==============================
//	CmdScale.h
//==============================

#ifndef __COMMAND_SCALE_H__
#define __COMMAND_SCALE_H__

class CmdScale : public Command
{
public:
	CmdScale();
	~CmdScale();

private:
	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Sel_Impl();

};

#endif	// __COMMAND_SCALE_H__

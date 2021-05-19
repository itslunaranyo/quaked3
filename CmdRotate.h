//==============================
//	CmdRotate.h
//==============================

#ifndef __COMMAND_ROTATE_H__
#define __COMMAND_ROTATE_H__

class CmdRotate : public Command
{
public:
	CmdRotate();
	~CmdRotate();

private:
	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Sel_Impl();

};

#endif	// __COMMAND_ROTATE_H__

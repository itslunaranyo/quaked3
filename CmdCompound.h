//==============================
//	CmdCompound.h
//==============================

#ifndef __COMMAND_COMPOUND_H__
#define __COMMAND_COMPOUND_H__

class CmdCompound : public Command
{
public:
	CmdCompound(const char* name);
	~CmdCompound();

	bool Complete(Command* cmd);
	int BrushDelta();
	int EntityDelta();

private:
	std::vector<Command*> series;
	bool stalled;

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Sel_Impl();

};

#endif	// __COMMAND_COMPOUND_H__

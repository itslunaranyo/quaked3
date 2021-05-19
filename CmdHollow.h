//==============================
//	CmdHollow.h
//==============================

#ifndef __COMMAND_HOLLOW_H__
#define __COMMAND_HOLLOW_H__

class CmdHollow : public Command
{
public:
	CmdHollow();
	~CmdHollow() {};

	void UseBrush(Brush* br);
	void UseBrushes(Brush* brList);

	int BrushDelta() { return cmdAR.BrushDelta(); };
	int EntityDelta() { return cmdAR.EntityDelta(); };
private:
	CmdAddRemove cmdAR;
	std::vector<Brush*> brHollowed;

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Sel_Impl();

};

#endif	// __COMMAND_HOLLOW_H__

//==============================
//	CmdHollow.h
//==============================

#ifndef __COMMAND_HOLLOW_H__
#define __COMMAND_HOLLOW_H__

class CmdHollow : public Command
{
public:
	CmdHollow() {};
	~CmdHollow() {};

	void UseBrush(Brush* br);
	void UseBrushes(Brush* brList);

private:
	CmdAddRemove cmdAR;
	std::vector<Brush*> brHollowed;

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Select_Impl();

};

#endif	// __COMMAND_HOLLOW_H__

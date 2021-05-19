//==============================
//	CmdMerge.h
//==============================

#ifndef __COMMAND_MERGE_H__
#define __COMMAND_MERGE_H__

class CmdMerge : public Command
{
public:
	CmdMerge() : convex(false) {};
	~CmdMerge() {};

	void UseBrush(Brush* br);
	void UseBrushes(Brush* brList);
	//void AllowConvex(bool acvx) { convex = acvx; }

private:
	CmdAddRemove cmdAR;
	std::vector<Brush*> brMerged;
	bool convex;

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Select_Impl();

};

#endif	// __COMMAND_MERGE_H__

//==============================
//	CmdClone.h
//==============================

#ifndef __COMMAND_CLONE_H__
#define __COMMAND_CLONE_H__

#include "qe3.h"

class CmdClone : public Command
{
public:
	CmdClone(Brush* brList, vec3_t offset = nullptr);
	~CmdClone();
private:
	CmdAddRemove cmdAR;

	void Clone(Brush* brList, vec3_t offset);

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Select_Impl();
};

#endif
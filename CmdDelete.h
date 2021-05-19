//==============================
//	CmdClone.h
//==============================

#ifndef __COMMAND_DELETE_H__
#define __COMMAND_DELETE_H__

#include "qe3.h"

class CmdDelete : public Command
{
public:
	CmdDelete(Brush *brList);
	~CmdDelete();
private:
	CmdAddRemove cmdAR;

	void Delete(Brush* brList);

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
};

#endif
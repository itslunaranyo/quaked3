//==============================
//	CmdCompound.cpp
//==============================

#include "qe3.h"
#include "CmdCompound.h"

CmdCompound::CmdCompound(const char* name) : Command(name)
{
	stalled = false;
	selectOnUndo = true;
	selectOnDo = true;
}

CmdCompound::~CmdCompound()
{
	Command* cmd;
	while (series.size())
	{
		cmd = series.back();
		series.pop_back();
		delete cmd;
	}
}

bool CmdCompound::Complete(Command *cmd)
{
	assert(state == NOOP || state == LIVE);
	if (stalled)
		Error("tried to add %s to stalled compound cmd", cmd->name);

	if (cmd->state == NOOP)
	{
		delete cmd;
		return true;
	}

	try {
		cmd->Do();
	}
	catch (...) {
		delete cmd;
		Undo_Impl();
		state = NOOP;
		return false;
	}

	if (cmd->state == NOOP)
	{
		delete cmd;
		return true;
	}

	series.push_back(cmd);
	state = LIVE;
	return true;
}

int CmdCompound::BrushDelta()
{
	int bd = 0;
	for (auto cmdIt = series.begin(); cmdIt != series.end(); ++cmdIt)
		bd += (*cmdIt)->BrushDelta();
	return bd;
}

int CmdCompound::EntityDelta()
{
	int ed = 0;
	for (auto cmdIt = series.begin(); cmdIt != series.end(); ++cmdIt)
		ed += (*cmdIt)->EntityDelta();
	return ed;
}

//==============================

void CmdCompound::Do_Impl()
{
	// all commands are already doneS
#ifdef _DEBUG
	// or are they
	for (auto cmdIt = series.begin(); cmdIt != series.end(); ++cmdIt)
		assert((*cmdIt)->state == DONE);
#endif
}

void CmdCompound::Undo_Impl()
{
	for (auto cmdIt = series.rbegin(); cmdIt != series.rend(); ++cmdIt)
		(*cmdIt)->Undo();
}

void CmdCompound::Redo_Impl()
{
	for (auto cmdIt = series.begin(); cmdIt != series.end(); ++cmdIt)
		(*cmdIt)->Redo();
}

void CmdCompound::Select_Impl()
{
	for (auto cmdIt = series.begin(); cmdIt != series.end(); ++cmdIt)
		(*cmdIt)->Select();
}



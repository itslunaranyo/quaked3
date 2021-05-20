//==============================
//	CmdCompound.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "CmdCompound.h"

/*
==============================
a compound command acts as a self-contained command queue which can be
treated as a single command.

multiple commands can be created, configured, and then completed by passing
them into the compound command, before it is then itself passed to the real
command queue. this enables all the subcommands to be collectively treated
as one action and undone/redone/canceled as a discrete unit.
==============================
*/

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
		CmdError("tried to add %s to stalled compound cmd", cmd->name);

	if (cmd->state == NOOP)
	{
		delete cmd;
		return true;
	}

	try {
		cmd->Do();
	}
	catch (qe3_cmd_exception& ex) {
		delete cmd;
		Undo_Impl();	// revert all prior commands if one fails
		state = NOOP;
		ReportError(ex);
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
	// all commands are already done
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

void CmdCompound::Sel_Impl()
{
	for (auto cmdIt = series.begin(); cmdIt != series.end(); ++cmdIt)
		(*cmdIt)->Select();
}



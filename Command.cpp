//==============================
//	command.cpp
//==============================

#include "qe3.h"

/*
========================================================================

uq.front <---> uq.back > CURRENT STATE < rq.back <---> rq.front
					<---- prev   next ---->

========================================================================
*/
std::deque<Command*> Command::undoQueue; 
std::deque<Command*> Command::redoQueue;

Command::Command() : state(BAD)
{
}

Command::~Command()
{
}

/*
==================
Command::Complete

the UI is done finalizing this command, seal it off and enqueue it
==================
*/
void Command::Complete()
{
	assert(state == LIVE);

	undoQueue.push_back(this);
	state = DONE;

	if ((int)undoQueue.size() > g_qeglobals.d_savedinfo.nUndoLevels)
		ClearOldestUndo();
}

/*
==================
Command::Undo
==================
*/
void Command::Undo()
{
	assert(state == DONE);
	assert(this == undoQueue.back());

	Undo_Impl();
	state = UNDONE;
}

/*
==================
Command::Redo
==================
*/
void Command::Redo()
{
	assert(state == UNDONE);
	assert(this == redoQueue.back());

	Redo_Impl();
	state = DONE;
}

/*
==================
Command::PerformUndo
==================
*/
void Command::PerformUndo()
{
	if (undoQueue.empty())
	{
		Sys_Printf("Nothing to undo.\n");
		return;
	}

	Command* cmd = undoQueue.back();
	undoQueue.pop_back();
	cmd->Undo();
	redoQueue.push_back(cmd);
}

/*
==================
Command::PerformRedo
==================
*/
void Command::PerformRedo()
{
	if (redoQueue.empty())
	{
		Sys_Printf("Nothing to redo.\n");
		return;
	}

	Command* cmd = redoQueue.back();
	redoQueue.pop_back();
	cmd->Redo();
	undoQueue.push_back(cmd);
}

/*
==================
Command::ClearOldestUndo
==================
*/
void Command::ClearOldestUndo()
{
	Command* cmd = undoQueue.front();
	undoQueue.pop_front();
	delete cmd;
}


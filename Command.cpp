//==============================
//	command.cpp
//==============================

#include "qe3.h"

/*
========================================================================

	COMMANDS

every action that modifies the map data (brushes, geometry, texturing,
entities, keyvalues, so on) is an instance of some subclass of Command.
the UI builds these Commands and then passes them to the CommandQueue,
which causes their changes to be applied to the scene.

the Commands remain extant in the CommandQueue to serve as undos/redos.
the back of each queue is closest to the 'present', and the front is
farthest in the past (for undos) or the future (for redos).

changes to selection or visibility, loading and unloading of wads, or
other events which do not alter what gets saved to disk are map data
agnostic, are not Commands, and cannot be undone.

========================================================================
*/

CommandQueue g_cmdQueue;

Command::Command() : state(NOOP) {}
Command::~Command() {}

/*
==================
Command::Do

called when the command object is passed into the queue and completed.
commands can set up anything they need in advance, but must not permanently
modify the scene before Do() is called. deleting any command before this
point should leave the scene exactly as it was when that command was first
instantiated.
==================
*/
void Command::Do()
{
	assert(state == LIVE);
	Do_Impl();
	state = DONE;
}

/*
==================
Command::Undo

called when the command object is asked to revert its changes. the command
can accomplish this however is best (caching before/after for complex ops,
simply reversing non-destructive ones, whatever)

the queue moves the object onto the redo queue after this, so the undo
process must also be reversible.
==================
*/
void Command::Undo()
{
	assert(state == DONE);
	Undo_Impl();
	state = UNDONE;

	g_bSelectionChanged = true;
	Sys_UpdateWindows(W_ALL);
}

/*
==================
Command::Redo

must restore original changes to the scene exactly as after a call to Do().
object is moved back to the undo queue after this is called.
==================
*/
void Command::Redo()
{
	assert(state == UNDONE);
	Redo_Impl();
	state = DONE;

	g_bSelectionChanged = true;
	Sys_UpdateWindows(W_ALL);
}

/*
==================
Command::Select

select everything that was modified by the command. commands should not
modify the selection by default, only when requested to by the UI through
this method.
==================
*/
void Command::Select()
{
	assert(state == DONE || state == UNDONE);
	Select_Impl();
}


// ========================================================================


/*
==================
CommandQueue::Complete

the UI is done finalizing this command, seal it off and make it an undo
==================
*/
void CommandQueue::Complete(Command *cmd)
{
	if (cmd->state == Command::NOOP)
	{
		// dead command that produces no changes in the scene, throw it away
		delete cmd;
		return;
	}

	ClearAllRedos();
	cmd->Do();
	undoQueue.push_back(cmd);

	if ((int)undoQueue.size() > g_qeglobals.d_savedinfo.nUndoLevels)
		ClearOldestUndo();

	g_map.modified = true;
}

/*
==================
CommandQueue::Undo
==================
*/
void CommandQueue::Undo()
{
	if (undoQueue.empty())
	{
		Sys_Printf("Nothing to undo.\n");
		Sys_Beep();
		return;
	}

	Command* cmd = undoQueue.back();
	undoQueue.pop_back();
	cmd->Undo();
	redoQueue.push_back(cmd);

	// lunaran TODO: remember on what step the map was last saved, and clear the
	//	modified flag if we undo/redo back to that exact step
	g_map.modified = true;
}

/*
==================
CommandQueue::Redo
==================
*/
void CommandQueue::Redo()
{
	if (redoQueue.empty())
	{
		Sys_Printf("Nothing to redo.\n");
		Sys_Beep();
		return;
	}

	Command* cmd = redoQueue.back();
	redoQueue.pop_back();
	cmd->Redo();
	undoQueue.push_back(cmd);
	g_map.modified = true;
}

/*
==================
CommandQueue::Clear
==================
*/
void CommandQueue::Clear()
{
	ClearAllUndos();
	ClearAllRedos();
}

/*
==================
CommandQueue::ClearOldestUndo
==================
*/
void CommandQueue::ClearOldestUndo()
{
	Command* cmd = undoQueue.front();
	undoQueue.pop_front();
	delete cmd;
}

/*
==================
CommandQueue::ClearAllRedos
==================
*/
void CommandQueue::ClearAllRedos()
{
	Command* cmd;
	while (redoQueue.size())
	{
		cmd = redoQueue.front();
		redoQueue.pop_front();
		delete cmd;
	}
}

/*
==================
CommandQueue::ClearAllUndos
==================
*/
void CommandQueue::ClearAllUndos()
{
	while (undoQueue.size())
		ClearOldestUndo();
}


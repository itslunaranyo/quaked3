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
the back of each queue is the end closest to the 'present', and the
front is farthest in the past (for undos) or the future (for redos).

changes to selection, changes to visibility, loading and unloading of 
wads, or other events which do not alter what gets saved to disk are 
map data agnostic, are not Commands, and cannot be undone.

========================================================================
*/

CommandQueue g_cmdQueue;

Command::Command() : selectOnDo(false), selectOnUndo(false), state(NOOP) {}
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
	//assert(state == LIVE);
	// sub-commands can sometimes do nothing, just check here
	if (state == NOOP)
		return;

	Do_Impl();

	// commands can decide as they're Do()ne that despite being set up and
	// receiving config, their net change to the scene was a zero, and
	// NOOP themselves as a signal
	if (state == NOOP)
		return;

	state = DONE;
	if (selectOnDo)
		Select();
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

	if (selectOnUndo)
		Select();
}

/*
==================
Command::Redo

must restore changes to the scene exactly as after a call to Do().
object is moved back to the undo queue after this is called.
==================
*/
void Command::Redo()
{
	assert(state == UNDONE);
	Redo_Impl();
	state = DONE;

	if (selectOnDo)
		Select();
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
	Sel_Impl();
	Selection::Changed();
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
		Sys_Printf("already noop command, deleting\n");
		delete cmd;
		return;
	}

	ClearAllRedos();
	try {
		cmd->Do();
	}
	catch (...)	{
		delete cmd;
		return;
	}

	if (cmd->state == Command::NOOP)
	{
		Sys_Printf("noop command after do, deleting\n");
		delete cmd;
		return;
	}

	undoQueue.push_back(cmd);
	cmd->id = ++gId;
	if (!idFirstAfterSave)
		idFirstAfterSave = cmd->id;

	if ((int)undoQueue.size() > g_qeglobals.d_savedinfo.nUndoLevels)
		ClearOldestUndo();

	Sys_UpdateWindows(W_ALL);
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

	Sys_UpdateWindows(W_ALL);
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

	Sys_UpdateWindows(W_ALL);
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
CommandQueue::SetSaved

make a note of the undo and redo steps on either side of us right now, so we know
this is the last saved state should we undo/redo across it again
==================
*/
void CommandQueue::SetSaved()
{
	if (undoQueue.size())
		idLastBeforeSave = undoQueue.back()->id;
	//else
	//	idLastBeforeSave = 0;

	if (redoQueue.size())
		idFirstAfterSave = redoQueue.back()->id;
	else
		idFirstAfterSave = 0;

	//QE_UpdateTitle();
}

/*
==================
CommandQueue::IsModified
==================
*/
bool CommandQueue::IsModified()
{
	if (undoQueue.size())
	{
		if (idLastBeforeSave == undoQueue.back()->id)
			return false;
	}
	else
	{
		if (!idLastBeforeSave)
			return false;
		// if undoqueue is empty but there is an idLastBeforeSave, we filled the undo queue and then undid all of it
		if (redoQueue.size())
		{
			if (idFirstAfterSave == redoQueue.back()->id)
				return false;
		}
	}

	return true;
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
		cmd = redoQueue.back();
		redoQueue.pop_back();
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
	Command* cmd;
	while (undoQueue.size())
	{
		cmd = undoQueue.back();
		undoQueue.pop_back();
		delete cmd;
	}
}


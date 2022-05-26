//==============================
//	command.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "Command.h"
#include "map.h"
#include "winding.h"
#include "select.h"

/*
========================================================================

COMMANDS

every action that modifies the map data (brushes, geometry, texturing,
entities, keyvalues, so on) is an instance of some subclass of Command.
the UI builds these Commands and then passes them to the CommandQueue,
which takes over handling their undo/redo status. sometimes this also 
causes their changes to be applied to the scene, in other cases it is
easier for the tool building the command to track a modification in
progress (such as during mouse drags), and only make it 'official' by 
submitting a command for it when finished.

the Commands remain extant in the CommandQueue to serve as undos/redos.
the back() of each queue is the end closest to the 'present', and the
front() is farthest in the past (for undos) or the future (for redos).

changes to selection, changes to visibility, loading and unloading of 
wads, or other events which are map data agnostic (ie do not alter what
gets saved to disk) are not Commands and cannot be undone.

all commands are free to store brush and face pointers, but in exchange
they must avoid moving brushes or faces in memory under all circumstances,
to maintain integrity across the entire length of the queue. 

========================================================================
*/

CommandQueue g_cmdQueue;

Command::Command(const char* nameIn) : name(nameIn), selectOnDo(false), selectOnUndo(false), modifiesSelection(false), state(NOOP) {}
Command::~Command() {}

/*
==================
Command::Do

called when the command object is passed into the queue and completed.

commands can set up anything they need in advance, but must not *permanently*
modify the scene before Do() is called. deleting any command before this
point should leave the scene exactly as it was when that command was first
instantiated*. deleting it after (when state == DONE) happens when the undo
drops off the far end of the list, so it should clean up all its memory
but not reverse its changes to the scene.

* commands that are modifiable after Do (like translate or texture shift, for
purposes of combining little nudges into single steps on the queue) necessarily
break this rule, in which case they take on the responsibility of reverting
themselves if deleted before being Done
==================
*/
void Command::Do()
{
	if (state == NOOP)
		return;

	Do_Impl();

	// commands can decide in their Do()s that, despite being set up and
	// receiving config, their net change to the scene was a zero, and
	// NOOP themselves to signal this
	if (state == NOOP)
		return;

	state = DONE;
	if (selectOnDo)
	{
		Select();
	}
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

	// replace the selection on undo but don't create one
	if (selectOnUndo && !Selection::IsEmpty())
	{
		Selection::DeselectAll();
		Select();
	}
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

	if (selectOnDo && !Selection::IsEmpty())
	{
		Selection::DeselectAll();
		Select();
	}
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

/*
==================
Command::CmdError

any error in configuration or other malformed input to a command object is
signalled by throwing a qe3_cmd_exception up to the encapsulating tool or
script editor
==================
*/
void Command::CmdError(char * error, ...)
{
	va_list argptr;
	char	text[1024];

	va_start(argptr, error);
	vsprintf(text, error, argptr);
	va_end(argptr);

	throw qe3_cmd_exception(text);
}


// ========================================================================

/*
==================
CommandQueue::~CommandQueue
==================
*/
CommandQueue::~CommandQueue()
{
	Clear();
}

/*
==================
CommandQueue::Complete

the UI is done finalizing this command, seal it off and make it an undo
==================
*/
bool CommandQueue::Complete(Command *cmd)
{
	if (cmd->state == Command::NOOP)
	{
		// dead command that produces no changes in the scene, throw it away
#ifdef _DEBUG
		Log::Print(_S("DEBUG: '%s' is a no-op, deleting\n") << cmd->name);
#endif
		delete cmd;
		return true;
	}

	ClearAllRedos();
	try {
		cmd->Do();
	}
	catch (qe3_cmd_exception)	{
		delete cmd;
		return false;
	}

	if (cmd->state == Command::NOOP)
	{
#ifdef _DEBUG
		Log::Print(_S("DEBUG: no-op '%s' after do, deleting\n") << cmd->name);
#endif
		delete cmd;
		return true;
	}

	// commands are individually responsible for enumerating brushes and
	// entities added and removed from the scene, to maintain the total
	g_map.numBrushes += cmd->BrushDelta();
	g_map.numEntities += cmd->EntityDelta();
	g_map.autosaveTime = 0;

	undoQueue.push_back(cmd);
	cmd->id = ++gId;
	if (!idFirstAfterSave)
		idFirstAfterSave = cmd->id;

	if (size && undoQueue.size() > size)
		ClearOldestUndo();

	if (cmd->modifiesSelection)
		Selection::Changed();

	Winding::OnCommandComplete();

	WndMain_UpdateBrushStatusBar();
	WndMain_UpdateWindows(W_ALL);
	return true;
}

/*
==================
CommandQueue::SetSize

if the user changes the undo queue size in preferences, shrink it immediately
==================
*/
void CommandQueue::SetSize(int s)
{
	size = (unsigned)s;
	if (!size) return;	// 0 == unlimited
	while (size < undoQueue.size())
	{
		ClearOldestUndo();
	}
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
		Log::Print("Nothing to undo.\n");
		Sys_Beep();
		return;
	}

	Command* cmd = undoQueue.back();
	undoQueue.pop_back();
	g_map.numBrushes -= cmd->BrushDelta();
	g_map.numEntities -= cmd->EntityDelta();
	g_map.autosaveTime = 0;
	Log::Print(_S("Undo: %s\n") << cmd->name);
	cmd->Undo();
	redoQueue.push_back(cmd);
	Selection::Changed();

	WndMain_UpdateBrushStatusBar();
	WndMain_UpdateWindows(W_ALL);
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
		Log::Print("Nothing to redo.\n");
		Sys_Beep();
		return;
	}

	Command* cmd = redoQueue.back();
	redoQueue.pop_back();
	g_map.numBrushes += cmd->BrushDelta();
	g_map.numEntities += cmd->EntityDelta();
	g_map.autosaveTime = 0;
	Log::Print(_S("Redo: %s\n") << cmd->name);
	cmd->Redo();
	undoQueue.push_back(cmd);
	Selection::Changed();

	WndMain_UpdateBrushStatusBar();
	WndMain_UpdateWindows(W_ALL);
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

	if (redoQueue.size())
		idFirstAfterSave = redoQueue.back()->id;
	else
		idFirstAfterSave = 0;
}

/*
==================
CommandQueue::IsModified
==================
*/
bool CommandQueue::IsModified()
{
	if (undoQueue.empty() && redoQueue.empty() &&
		!idLastBeforeSave && !idFirstAfterSave)
		return false;

	if (undoQueue.size() && idLastBeforeSave == undoQueue.back()->id)
		return false;
	if (redoQueue.size() && idFirstAfterSave == redoQueue.back()->id)
		return false;

	return true;
}

/*
==================
CommandQueue::ClearOldestUndo
==================
*/
void CommandQueue::ClearOldestUndo()
{
	if (undoQueue.empty())
		return;
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
	idFirstAfterSave = 0;
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
	idLastBeforeSave = 0;
}


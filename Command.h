//==============================
//	command.h
//==============================

#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <deque>

// ========================================================================

class Command
{
public:
	Command();
	virtual ~Command();

	enum { NOOP, LIVE, DONE, UNDONE } state;
	// NOOP - uninitialized/no effect on scene, will be deleted without enqueuing
	// LIVE - currently being built by the UI
	// DONE - completed, scene has been changed
	// UNDONE - completed, changes to scene have been reverted

	void Do();
	void Undo();
	void Redo();
	void Select();

protected:
	bool selectOnDo;
	bool selectOnUndo;
	virtual void Do_Impl() { return; }
	virtual void Undo_Impl() { return; }
	virtual void Redo_Impl() { return; }
	virtual void Select_Impl() { return; }
};

// ========================================================================

class CommandQueue
{
public:
	CommandQueue() {}
	~CommandQueue() {}

	void Complete(Command* cmd);
	void Undo();
	void Redo();
	void Clear();

	Command* LastUndo() { return undoQueue.back(); }
	bool UndoAvailable() { return (undoQueue.size() != 0); }
	bool RedoAvailable() { return (redoQueue.size() != 0); }

private:
	void ClearOldestUndo();
	void ClearAllRedos();
	void ClearAllUndos();
	std::deque<Command*> undoQueue, redoQueue;
};


#endif
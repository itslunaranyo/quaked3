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
	Command(const char* nameIn);
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

	unsigned id;
	const char *name;
	bool modifiesSelection;

	// net change in brushcount and entitycount on do or redo as a result of 
	// this command (the same numbers are used and negated for undo)
	virtual int BrushDelta() { return 0; };
	virtual int EntityDelta() { return 0; };

protected:
	void CmdError(char *error, ...);

	bool selectOnDo;
	bool selectOnUndo;
	virtual void Do_Impl() { return; }
	virtual void Undo_Impl() { return; }
	virtual void Redo_Impl() { return; }
	virtual void Sel_Impl() { return; }
};

// ========================================================================

class CommandQueue
{
public:
	CommandQueue() : gId(0), idLastBeforeSave(0), idFirstAfterSave(0), size(0) {}
	~CommandQueue();

	bool Complete(Command* cmd);
	void SetSize(int s);
	void Undo();
	void Redo();
	void Clear();
	void SetSaved();
	bool IsModified();

	Command* LastUndo() { return CanUndo() ? undoQueue.back() : nullptr; }
	bool CanUndo() { return (undoQueue.size() != 0); }
	bool CanRedo() { return (redoQueue.size() != 0); }

private:
	unsigned size;
	void ClearOldestUndo();
	void ClearAllRedos();
	void ClearAllUndos();

	unsigned gId, idLastBeforeSave, idFirstAfterSave;

	std::deque<Command*> undoQueue, redoQueue;
};

extern CommandQueue g_cmdQueue;

#endif
//==============================
//	command.h
//==============================

#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <deque>

class Command
{
public:
	Command();
	virtual ~Command();

	enum { BAD, LIVE, DONE, UNDONE } state;

	void Complete();
	void Undo();
	void Redo();

	static void PerformUndo();
	static void PerformRedo();

protected:
	virtual bool Undo_Impl() { return false; }
	virtual bool Redo_Impl() { return false; }

private:
	static void ClearOldestUndo();
	static std::deque<Command*> undoQueue, redoQueue;
};


#endif
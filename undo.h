//==============================
//	undo.h
//==============================
#ifndef __UNDO_H__
#define __UNDO_H__

// QERadiant Multilevel Undo/Redo

class qeUndo
{
public:
	qeUndo();
	~qeUndo();

	double		time;			//time operation was performed
	int			id;				//every undo has an unique id
	int			done;			//true when undo is build
	char		*operation;		//name of the operation
	Brush		brushlist;		//deleted brushes
	Entity		entitylist;		//deleted entities
	qeUndo	*prev, *next;	//next and prev undo in list
};

//========================================================================

namespace Undo
{
// start operation
void Start(char *operation);
// end operation
void End();
// add brush to the undo
void AddBrush(Brush *pBrush);
// add a list with brushes to the undo
void AddBrushList(Brush *brushlist);
// end a brush after the operation is performed
void EndBrush(Brush *pBrush);
// end a list with brushes after the operation is performed
void EndBrushList(Brush *brushlist);

	// allocate and initialize a new undo
	void GeneralStart(char *operation);
	// add entity to undo
	void AddEntity(Entity *entity);
	// end an entity after the operation is performed
	void EndEntity(Entity *entity);
	// returns true if brush is in undo buffer
	bool BrushInUndo(qeUndo *undo, Brush *brush);
	// returns true if entity is in undo buffer
	bool EntityInUndo(qeUndo *undo, Entity *ent);
	// undo last operation
	void Undo();
	// redo last undone operation
	void Redo();
	// returns true if there is something to be undone available
	bool UndoAvailable();
	// returns true if there is something to redo available
	bool RedoAvailable();
	// clear the undo buffer
	void Clear();
	// clear the redo buffer
	void ClearRedo();
	// free first undo as undo buffer increases past max
	void FreeFirstUndo();
	// get maximum undo size
	int  GetMaxSize();
	// set maximum undo size (default 64)
	void SetMaxSize(int size);
	// get maximum undo memory in bytes
	int  GetMaxMemorySize();
	// set maximum undo memory in bytes (default 2 MB)
	void SetMaxMemorySize(int size);
	// returns the amount of memory used by undo
	int  MemorySize();
}
#endif

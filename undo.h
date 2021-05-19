//==============================
//	undo.h
//==============================
#ifndef __UNDO_H__
#define __UNDO_H__

// QERadiant Multilevel Undo/Redo

typedef struct undo_s
{
	double		time;				//time operation was performed
	int			id;					//every undo has an unique id
	int			done;				//true when undo is build
	char	   *operation;			//name of the operation
	brush_t		brushlist;			//deleted brushes
	entity_t	entitylist;			//deleted entities
	struct undo_s	*prev, *next;	//next and prev undo in list
} undo_t;

//========================================================================

// 
void Undo_GeneralStart (char *operation);
// start operation
void Undo_Start (char *operation);
// end operation
void Undo_End ();
// add brush to the undo
void Undo_AddBrush (brush_t *pBrush);
// add a list with brushes to the undo
void Undo_AddBrushList (brush_t *brushlist);
// end a brush after the operation is performed
void Undo_EndBrush (brush_t *pBrush);
// end a list with brushes after the operation is performed
void Undo_EndBrushList (brush_t *brushlist);
// add entity to undo
void Undo_AddEntity (entity_t *entity);
// end an entity after the operation is performed
void Undo_EndEntity (entity_t *entity);
// returns true if brush is in undo buffer
bool Undo_BrushInUndo (undo_t *undo, brush_t *brush);
// returns true if entity is in undo buffer
bool Undo_EntityInUndo (undo_t *undo, entity_t *ent);
// undo last operation
void Undo_Undo ();
// redo last undone operation
void Undo_Redo ();
// returns true if there is something to be undone available
bool Undo_UndoAvailable ();
// returns true if there is something to redo available
bool Undo_RedoAvailable ();
// clear the undo buffer
void Undo_Clear ();
// clear the redo buffer
void Undo_ClearRedo ();
// free first undo as undo buffer increases past max
void Undo_FreeFirstUndo ();
// get maximum undo size
int  Undo_GetMaxSize ();
// set maximum undo size (default 64)
void Undo_SetMaxSize (int size);
// get maximum undo memory in bytes
int  Undo_GetMaxMemorySize ();
// set maximum undo memory in bytes (default 2 MB)
void Undo_SetMaxMemorySize (int size);
// returns the amount of memory used by undo
int  Undo_MemorySize ();

#endif

//==============================
//	undo.c
//==============================

//==================================================================
//	QERadiant Undo/Redo
//
//	basic setup:
//
//	<-g_undolist---------g_lastundo> <---map data---> <-g_lastredo---------g_redolist->
//
//
//	undo/redo on the g_map.world is special, only the epair changes
//	are remembered and the world entity never gets deleted.
//	
//	FIXME: maybe reset the Undo system at map load
//		   maybe also reset the entityId at map load
//==================================================================

#include "qe3.h"
#include <malloc.h>	// sikk - Undo/Redo


undo_t *g_undolist;					//first undo in the list
undo_t *g_lastundo;					//last undo in the list
undo_t *g_redolist;					//first redo in the list
undo_t *g_lastredo;					//last redo in list
int g_undoMaxSize = 64;				//maximum number of undos
int g_undoSize = 0;					//number of undos in the list
int g_undoMaxMemorySize = 2097152;	//maximum undo memory (default 2 MB) (2 * 1024 * 1024 bytes)
int g_undoMemorySize = 0;			//memory size of undo buffer
int g_undoId = 1;					//current undo ID (zero is invalid id)
int g_redoId = 1;					//current redo ID (zero is invalid id)

/*
=============
Undo_GeneralStart
=============
*/
void Undo_GeneralStart (char *operation)
{
	undo_t		*undo;
	Brush		*pBrush;
	Entity	*pEntity;

	if (g_lastundo)
		if (!g_lastundo->done)
			Sys_Printf("WARNING: Undo_Start: Last undo not finished.\n");

	undo = (undo_t *)malloc(sizeof(undo_t));

	if (!undo) 
		return;

	memset(undo, 0, sizeof(undo_t));
	undo->brushlist.next = &undo->brushlist;
	undo->brushlist.prev = &undo->brushlist;
	undo->entitylist.next = &undo->entitylist;
	undo->entitylist.prev = &undo->entitylist;

	if (g_lastundo) 
		g_lastundo->next = undo;
	else 
		g_undolist = undo;

	undo->prev = g_lastundo;
	undo->next = NULL;
	g_lastundo = undo;
	undo->time = Sys_DoubleTime();
	
	if (g_undoId > g_undoMaxSize * 2) 
		g_undoId = 1;
	if (g_undoId <= 0) 
		g_undoId = 1;

	undo->id = g_undoId++;
	undo->done = false;
	undo->operation = operation;

	// reset the undo IDs of all brushes using the new ID
	for (pBrush = g_map.brActive.next; pBrush != NULL && pBrush != &g_map.brActive; pBrush = pBrush->next)
		if (pBrush->undoId == undo->id)
			pBrush->undoId = 0;

	for (pBrush = g_brSelectedBrushes.next; pBrush != NULL && pBrush != &g_brSelectedBrushes; pBrush = pBrush->next)
		if (pBrush->undoId == undo->id)
			pBrush->undoId = 0;

	// reset the undo IDs of all entities using thew new ID
	for (pEntity = g_map.entities.next; pEntity != NULL && pEntity != &g_map.entities; pEntity = pEntity->next)
		if (pEntity->undoId == undo->id)
			pEntity->undoId = 0;

	g_undoMemorySize += sizeof(undo_t);
	g_undoSize++;

	// undo buffer is bound to a max
	if (g_undoSize > g_undoMaxSize)
		Undo_FreeFirstUndo();
}

/*
=============
Undo_Start
=============
*/
void Undo_Start (char *operation)
{
	Undo_ClearRedo();
	Undo_GeneralStart(operation);
}

/*
=============
Undo_End
=============
*/
void Undo_End ()
{
	if (!g_lastundo)
	{
//		Sys_Printf("MSG: Undo_End: Nothing left to undo.\n");
		return;
	}

	if (g_lastundo->done)
	{
//		Sys_Printf("WARNING: Undo_End: Last undo already finished.\n");
		return;
	}

	g_lastundo->done = true;

	// undo memory size is bound to a max
	while (g_undoMemorySize > g_undoMaxMemorySize)
	{
		// always keep one undo
		if (g_undolist == g_lastundo) 
			break;
		Undo_FreeFirstUndo();
	}
	
//	Sys_Printf("undo size = %d, undo memory = %d\n", g_undoSize, g_undoMemorySize);
}

/*
=============
Undo_AddBrush
=============
*/
void Undo_AddBrush (Brush *pBrush)
{
	Brush *pClone;
	
	if (!g_lastundo)
	{
		Sys_Printf("MSG: Undo_AddBrushList: Nothing left to undo.\n");
		return;
	}

	if (g_lastundo->entitylist.next != &g_lastundo->entitylist)
		Sys_Printf("WARNING: Undo_AddBrushList: Adding brushes after entity.\n");

	// if the brush is already in the undo
	if (Undo_BrushInUndo(g_lastundo, pBrush))
		return;

	// clone the brush
	pClone = pBrush->FullClone();
	
	// save the ID of the owner entity
	pClone->ownerId = pBrush->owner->entityId;

	// save the old undo ID for previous undos
	pClone->undoId = pBrush->undoId;

	pClone->AddToList(&g_lastundo->brushlist);
	g_undoMemorySize += pClone->MemorySize();
}

/*
=============
Undo_AddBrushList

TTimo: some brushes are just there for UI, and the information is 
somewhere else for patches it's in the patchMesh_t structure, so when 
we clone the brush we get that information (brush_t::pPatch) but: 
models are stored in pBrush->owner->md3Class, and owner epairs and 
origin parameters are important so, we detect models and push the 
entity in the undo session (as well as it's BBox brush) same for 
other items like weapons and ammo etc.
=============
*/
void Undo_AddBrushList (Brush *brushlist)
{
	Brush *pBrush;
	Brush *pClone;

	if (!g_lastundo)
	{
		Sys_Printf("MSG: Undo_AddBrushList: Nothing left to undo.\n");
		return;
	}

	if (g_lastundo->entitylist.next != &g_lastundo->entitylist)
		Sys_Printf("WARNING: Undo_AddBrushList: Adding brushes after entity.\n");

	// copy the brushes to the undo
	for (pBrush = brushlist->next; pBrush != NULL && pBrush != brushlist; pBrush = pBrush->next)
	{
		// if the brush is already in the undo
		// ++timo FIXME: when does this happen?
		if (Undo_BrushInUndo(g_lastundo, pBrush))
			continue;

		// do we need to store this brush's entity in the undo?
		// if it's a fixed size entity, the brush that reprents it is not really relevant, it's used for selecting and moving around
		// what we want to store for undo is the owner entity, epairs and origin/angle stuff
		// ++timo FIXME: if the entity is not fixed size I don't know, so I don't do it yet
		if (pBrush->owner->eclass->IsFixedSize())
			Undo_AddEntity(pBrush->owner);

		// clone the brush
		pClone = pBrush->FullClone();

		// save the ID of the owner entity
		pClone->ownerId = pBrush->owner->entityId;

		// save the old undo ID from previous undos
		pClone->undoId = pBrush->undoId;

		pClone->AddToList(&g_lastundo->brushlist);
		g_undoMemorySize += pClone->MemorySize();
	}
}

/*
=============
Undo_EndBrush
=============
*/
void Undo_EndBrush (Brush *pBrush)
{
	if (!g_lastundo)
	{
//		Sys_Printf("MSG: Undo_End: Nothing left to undo.\n");
		return;
	}

	if (g_lastundo->done)
	{
//		Sys_Printf("WARNING: Undo_End: Last undo already finished.\n");
		return;
	}

	pBrush->undoId = g_lastundo->id;
}

/*
=============
Undo_EndBrushList
=============
*/
void Undo_EndBrushList (Brush *brushlist)
{
	Brush *pBrush;

	if (!g_lastundo)
	{
//		Sys_Printf("MSG: Undo_End: Nothing left to undo.\n");
		return;
	}

	if (g_lastundo->done)
	{
//		Sys_Printf("WARNING: Undo_End: Last undo already finished.\n");
		return;
	}

	for (pBrush = brushlist->next; pBrush != NULL && pBrush != brushlist; pBrush = pBrush->next)
		pBrush->undoId = g_lastundo->id;
}

/*
=============
Undo_AddEntity
=============
*/
void Undo_AddEntity (Entity *entity)
{
	Entity *pClone;

	if (!g_lastundo)
	{
		Sys_Printf("MSG: Undo_AddEntity: Nothing left to undo.\n");
		return;
	}

	// if the entity is already in the undo
	if (Undo_EntityInUndo(g_lastundo, entity))
		return;

	// clone the entity
	pClone = entity->Clone();

	// NOTE: Entity_Clone adds the entity to the entity list
	//		 so we remove it from that list here
	pClone->RemoveFromList();

	// save the old undo ID for previous undos
	pClone->undoId = entity->undoId;

	// save the entity ID (we need a full clone)
	pClone->entityId = entity->entityId;
	
	pClone->AddToList(&g_lastundo->entitylist);
	g_undoMemorySize += pClone->MemorySize();
}

/*
=============
Undo_EndEntity
=============
*/
void Undo_EndEntity (Entity *entity)
{
	if (!g_lastundo)
	{
#ifdef _DEBUG
		Sys_Printf("MSG: Undo_End: Nothing left to undo.\n");
#endif
		return;
	}
	if (g_lastundo->done)
	{
#ifdef _DEBUG
		Sys_Printf("WARNING: Undo_End: Last undo already finished.\n");
#endif
		return;
	}
	if (entity == g_map.world)
	{
//		Sys_Printf("WARNING: Undo_AddEntity: Undo on world entity.\n");
		// NOTE: we never delete the world entity when undoing an operation
		//		 we only transfer the epairs
		return;
	}
	entity->undoId = g_lastundo->id;
}

/*
=============
Undo_BrushInUndo
=============
*/
bool Undo_BrushInUndo (undo_t *undo, Brush *brush)
{
	Brush *b;

	for (b = undo->brushlist.next; b != &undo->brushlist; b = b->next)
		if (b == brush) 
			return true;

	return false;
}

/*
=============
Undo_EntityInUndo
=============
*/
bool Undo_EntityInUndo (undo_t *undo, Entity *ent)
{
	Entity *e;

	for (e = undo->entitylist.next; e != &undo->entitylist; e = e->next)
		if (e == ent) 
			return true;

	return false;
}

/*
=============
Undo_Undo
=============
*/
void Undo_Undo ()
{
	undo_t		*undo, *redo;
	Brush		*pBrush, *pNextBrush;
	Entity	*pEntity, *pNextEntity, *pUndoEntity;

	if (!g_lastundo)
	{
		Sys_Printf("MSG: Nothing left to undo.\n");
		return;
	}

	if (!g_lastundo->done)
	{
		Sys_Printf("WARNING: Undo_Undo: Last undo not finished.\n");
	}

	// get the last undo
	undo = g_lastundo;

	if (g_lastundo->prev) 
		g_lastundo->prev->next = NULL;
	else 
		g_undolist = NULL;

	g_lastundo = g_lastundo->prev;

	// allocate a new redo
	redo = (undo_t *)malloc(sizeof(undo_t));
	if (!redo) 
		return;
	
	memset(redo, 0, sizeof(undo_t));
	redo->brushlist.next = &redo->brushlist;
	redo->brushlist.prev = &redo->brushlist;
	redo->entitylist.next = &redo->entitylist;
	redo->entitylist.prev = &redo->entitylist;

	if (g_lastredo)
		g_lastredo->next = redo;
	else
		g_redolist = redo;

	redo->prev = g_lastredo;
	redo->next = NULL;
	g_lastredo = redo;
	redo->time = Sys_DoubleTime();
	redo->id = g_redoId++;
	redo->done = true;
	redo->operation = undo->operation;

	// reset the redo IDs of all brushes using the new ID
	for (pBrush = g_map.brActive.next; pBrush != NULL && pBrush != &g_map.brActive; pBrush = pBrush->next)
		if (pBrush->redoId == redo->id)
			pBrush->redoId = 0;

	for (pBrush = g_brSelectedBrushes.next; pBrush != NULL && pBrush != &g_brSelectedBrushes; pBrush = pBrush->next)
		if (pBrush->redoId == redo->id)
			pBrush->redoId = 0;

	// reset the redo IDs of all entities using thew new ID
	for (pEntity = g_map.entities.next; pEntity != NULL && pEntity != &g_map.entities; pEntity = pEntity->next)
		if (pEntity->redoId == redo->id)
			pEntity->redoId = 0;

	// deselect current sutff
	Select_DeselectAll(true);

	// move "created" brushes to the redo
	for (pBrush = g_map.brActive.next; pBrush != NULL && pBrush != &g_map.brActive; pBrush=pNextBrush)
	{
		pNextBrush = pBrush->next;
		if (pBrush->undoId == undo->id)
		{
//			delete pBrush;
			
			// move the brush to the redo
			pBrush->RemoveFromList();
			pBrush->AddToList(&redo->brushlist);
			
			// make sure the ID of the owner is stored
			pBrush->ownerId = pBrush->owner->entityId;
			
			// unlink the brush from the owner entity
			Entity::UnlinkBrush(pBrush);
		}
	}
	
	// move "created" entities to the redo
	for (pEntity = g_map.entities.next; pEntity != NULL && pEntity != &g_map.entities; pEntity = pNextEntity)
	{
		pNextEntity = pEntity->next;
		if (pEntity->undoId == undo->id)
		{
			// check if this entity is in the undo
			for (pUndoEntity = undo->entitylist.next; pUndoEntity != NULL && pUndoEntity != &undo->entitylist; pUndoEntity = pUndoEntity->next)
			{
				// move brushes to the undo entity
				if (pUndoEntity->entityId == pEntity->entityId)
				{
					pUndoEntity->brushes.next = pEntity->brushes.next;
					pUndoEntity->brushes.prev = pEntity->brushes.prev;
					pEntity->brushes.next = &pEntity->brushes;
					pEntity->brushes.prev = &pEntity->brushes;
				}
			}
			
//			delete pEntity;
			// move the entity to the redo
			pEntity->RemoveFromList();
			pEntity->AddToList(&redo->entitylist);
		}
	}

	// add the undo entities back into the entity list
	for (pEntity = undo->entitylist.next; pEntity != NULL && pEntity != &undo->entitylist; pEntity = undo->entitylist.next)
	{
		g_undoMemorySize -= pEntity->MemorySize();
		
		// if this is the world entity
		if (pEntity->entityId == g_map.world->entityId)
		{
			// free the epairs of the world entity
			g_map.world->FreeEpairs();
			
			// set back the original epairs
			g_map.world->epairs = pEntity->epairs;
			
			// unhook the epairs and free the g_map.world clone that stored the epairs
			pEntity->epairs = NULL;
			delete pEntity;
		}
		else
		{
			pEntity->RemoveFromList();
			pEntity->AddToList(&g_map.entities);
			pEntity->redoId = redo->id;
		}
	}

	// add the undo brushes back into the selected brushes
	for (pBrush = undo->brushlist.next; pBrush != NULL && pBrush != &undo->brushlist; pBrush = undo->brushlist.next)
	{
		g_undoMemorySize -= pBrush->MemorySize();
		pBrush->RemoveFromList();
		pBrush->AddToList(&g_map.brActive);

		for (pEntity = g_map.entities.next; pEntity != NULL && pEntity != &g_map.entities; pEntity = pNextEntity)
		{
			if (pEntity->entityId == pBrush->ownerId)
			{
				pEntity->LinkBrush(pBrush);
				break;
			}
		}

		// if the brush is not linked then it should be linked into the world entity
		// ++timo FIXME: maybe not, maybe we've lost this entity's owner!
		if (pEntity == NULL || pEntity == &g_map.entities)
			g_map.world->LinkBrush(pBrush);

		// build the brush
//		Brush_Build(pBrush);
		Select_HandleBrush(pBrush, false);
		pBrush->redoId = redo->id;
    }
	
	Sys_Printf("CMD: %s undone.\n", undo->operation);

	// free the undo
	g_undoMemorySize -= sizeof(undo_t);
	free(undo);
	g_undoSize--;
	g_undoId--;

	if (g_undoId <= 0) 
		g_undoId = 2 * g_undoMaxSize;
	
    Sys_UpdateWindows(W_ALL);
}

/*
=============
Undo_Redo
=============
*/
void Undo_Redo()
{
	undo_t		*redo;
	Brush		*pBrush, *pNextBrush;
	Entity	*pEntity, *pNextEntity, *pRedoEntity;

	if (!g_lastredo)
	{
		Sys_Printf("MSG: Nothing left to redo.\n");
		return;
	}

	if (g_lastundo)
		if (!g_lastundo->done)
			Sys_Printf("WARNING: Last undo not finished.\n");

	// get the last redo
	redo = g_lastredo;

	if (g_lastredo->prev) 
		g_lastredo->prev->next = NULL;
	else 
		g_redolist = NULL;

	g_lastredo = g_lastredo->prev;

	Undo_GeneralStart(redo->operation);

	// remove current selection
	Select_DeselectAll(true);

	// move "created" brushes back to the last undo
	for (pBrush = g_map.brActive.next; pBrush != NULL && pBrush != &g_map.brActive; pBrush = pNextBrush)
	{
		pNextBrush = pBrush->next;
		if (pBrush->redoId == redo->id)
		{
			//move the brush to the undo
			pBrush->RemoveFromList();
			pBrush->AddToList(&g_lastundo->brushlist);
			g_undoMemorySize += pBrush->MemorySize();
			pBrush->ownerId = pBrush->owner->entityId;
			Entity::UnlinkBrush(pBrush);
		}
	}

	// move "created" entities back to the last undo
	for (pEntity = g_map.entities.next; pEntity != NULL && pEntity != &g_map.entities; pEntity = pNextEntity)
	{
		pNextEntity = pEntity->next;
		if (pEntity->redoId == redo->id)
		{
			// check if this entity is in the redo
			for (pRedoEntity = redo->entitylist.next; pRedoEntity != NULL && pRedoEntity != &redo->entitylist; pRedoEntity = pRedoEntity->next)
			{
				// move brushes to the redo entity
				if (pRedoEntity->entityId == pEntity->entityId)
				{
					pRedoEntity->brushes.next = pEntity->brushes.next;
					pRedoEntity->brushes.prev = pEntity->brushes.prev;
					pEntity->brushes.next = &pEntity->brushes;
					pEntity->brushes.prev = &pEntity->brushes;
				}
			}
			
//			delete pEntity;

			// move the entity to the redo
			pEntity->RemoveFromList();
			pEntity->AddToList(&g_lastundo->entitylist);
			g_undoMemorySize += pEntity->MemorySize();
		}
	}

	// add the undo entities back into the entity list
	for (pEntity = redo->entitylist.next; pEntity != NULL && pEntity != &redo->entitylist; pEntity = redo->entitylist.next)
	{
		// if this is the world entity
		if (pEntity->entityId == g_map.world->entityId)
		{
			// free the epairs of the world entity
			g_map.world->FreeEpairs();

			// set back the original epairs
			g_map.world->epairs = pEntity->epairs;

			// free the g_map.world clone that stored the epairs
			delete pEntity;
		}
		else
		{
			pEntity->RemoveFromList();
			pEntity->AddToList(&g_map.entities);
		}
	}

	// add the redo brushes back into the selected brushes
	for (pBrush = redo->brushlist.next; pBrush != NULL && pBrush != &redo->brushlist; pBrush = redo->brushlist.next)
	{
		pBrush->RemoveFromList();
		pBrush->AddToList(&g_map.brActive);
		for (pEntity = g_map.entities.next; pEntity != NULL && pEntity != &g_map.entities; pEntity = pNextEntity)
		{
			if (pEntity->entityId == pBrush->ownerId)
			{
				pEntity->LinkBrush(pBrush);
				break;
			}
		}

		// if the brush is not linked then it should be linked into the world entity
		if (pEntity == NULL || pEntity == &g_map.entities)
			g_map.world->LinkBrush(pBrush);

		// build the brush
//		Brush_Build(pBrush);
		Select_HandleBrush(pBrush, true);
    }
	
	Undo_End();
	
	Sys_Printf("CMD: %s redone.\n", redo->operation);
	
	g_redoId--;

	// free the undo
	free(redo);
	
    Sys_UpdateWindows(W_ALL);
}

/*
=============
Undo_UndoAvailable
=============
*/
bool Undo_UndoAvailable ()
{
	if (g_lastundo)
		if (g_lastundo->done)
			return true;

	return false;
}

/*
=============
Undo_RedoAvailable
=============
*/
bool Undo_RedoAvailable ()
{
	if (g_lastredo) 
		return true;

	return false;
}

/*
=============
Undo_Clear

Clears the undo buffer.
=============
*/
void Undo_Clear ()
{
	undo_t		*undo, *nextundo;
	Brush		*pBrush, *pNextBrush;
	Entity	*pEntity, *pNextEntity;

	Undo_ClearRedo();

	for (undo = g_undolist; undo; undo = nextundo)
	{
		nextundo = undo->next;

		for (pBrush = undo->brushlist.next; pBrush != NULL && pBrush != &undo->brushlist; pBrush = pNextBrush)
		{
			pNextBrush = pBrush->next;
			g_undoMemorySize -= pBrush->MemorySize();
			delete pBrush;
		}

		for (pEntity = undo->entitylist.next; pEntity != NULL && pEntity != &undo->entitylist; pEntity = pNextEntity)
		{
			pNextEntity = pEntity->next;
			g_undoMemorySize -= pEntity->MemorySize();
			delete pEntity;
		}

		g_undoMemorySize -= sizeof(undo_t);
		free(undo);
	}

	g_undolist = NULL;
	g_lastundo = NULL;
	g_undoSize = 0;
	g_undoMemorySize = 0;
	g_undoId = 1;
}

/*
=============
Undo_ClearRedo
=============
*/
void Undo_ClearRedo ()
{
	undo_t		*redo, *nextredo;
	Brush		*pBrush, *pNextBrush;
	Entity	*pEntity, *pNextEntity;

	for (redo = g_redolist; redo; redo = nextredo)
	{
		nextredo = redo->next;

		for (pBrush = redo->brushlist.next; pBrush != NULL && pBrush != &redo->brushlist; pBrush = pNextBrush)
		{
			pNextBrush = pBrush->next;
			delete pBrush;
		}

		for (pEntity = redo->entitylist.next; pEntity != NULL && pEntity != &redo->entitylist; pEntity = pNextEntity)
		{
			pNextEntity = pEntity->next;
			delete pEntity;
		}

		free(redo);
	}

	g_redolist = NULL;
	g_lastredo = NULL;
	g_redoId = 1;
}

/*
=============
Undo_FreeFirstUndo
=============
*/
void Undo_FreeFirstUndo ()
{
	undo_t		*undo;
	Brush		*pBrush, *pNextBrush;
	Entity	*pEntity, *pNextEntity;

	// remove the oldest undo from the undo buffer
	undo = g_undolist;
	g_undolist = g_undolist->next;
	g_undolist->prev = NULL;

	for (pBrush = undo->brushlist.next; pBrush != NULL && pBrush != &undo->brushlist; pBrush = pNextBrush)
	{
		pNextBrush = pBrush->next;
		g_undoMemorySize -= pBrush->MemorySize();
		delete pBrush;
	}

	for (pEntity = undo->entitylist.next; pEntity != NULL && pEntity != &undo->entitylist; pEntity = pNextEntity)
	{
		pNextEntity = pEntity->next;
		g_undoMemorySize -= pEntity->MemorySize();
		delete pEntity;
	}

	g_undoMemorySize -= sizeof(undo_t);
	free(undo);
	g_undoSize--;
}

/*
=============
Undo_GetMaxSize
=============
*/
int Undo_GetMaxSize ()
{
	return g_undoMaxSize;
}

/*
=============
Undo_SetMaxSize
=============
*/
void Undo_SetMaxSize (int size)
{
	Undo_Clear();

	if (size < 1) 
		g_undoMaxSize = 1;
	else 
		g_undoMaxSize = size;
}

/*
=============
Undo_GetMaxMemorySize
=============
*/
int Undo_GetMaxMemorySize ()
{
	return g_undoMaxMemorySize;
}

/*
=============
Undo_SetMaxMemorySize
=============
*/
void Undo_SetMaxMemorySize (int size)
{
	Undo_Clear();

	if (size < 1024) 
		g_undoMaxMemorySize = 1024;
	else 
		g_undoMaxMemorySize = size;
}

/*
=============
Undo_MemorySize
=============
*/
int Undo_MemorySize ()
{
/*
	int			size;
	undo_t	   *undo;
	Brush    *pBrush;
	Entity   *pEntity;

	size = 0;
	for (undo = g_undolist; undo; undo = undo->next)
	{
		for (pBrush = undo->brushlist.next; pBrush != NULL && pBrush != &undo->brushlist; pBrush = pBrush->next)
			size += pBrush->MemorySize();

		for (pEntity = undo->entitylist.next; pEntity != NULL && pEntity != &undo->entitylist; pEntity = pEntity->next)
			size += Entity_MemorySize(pEntity);

		size += sizeof(undo_t);
	}
	return size;
*/
	return g_undoMemorySize;
}

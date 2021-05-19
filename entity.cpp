//==============================
//	entity.cpp
//==============================

#include "qe3.h"
#include "io.h"


int g_nEntityId = 1;	// sikk - Undo/Redo

//===================================================================

EntClass	*eclass;
epair_t		*epairs;


Entity::Entity() : undoId(0), redoId(0), ownerId(0),
	eclass(nullptr), epairs(nullptr)
{
	entityId = g_nEntityId++;
	brushes.onext = brushes.oprev = &brushes;
	brushes.owner = this;
	origin[0] = origin[1] = origin[2] = 0;
}

Entity::~Entity() 
{

}


//===================================================================

/*
==============
SetSpawnFlag
set an individual spawnflag without affecting the others
==============
*/
void SetSpawnFlag(Entity *ent, int flag, bool on)
{
	char*	val;
	int		fval;
	char	sz[32];

	val = ValueForKey(ent, "spawnflags");
	fval = val ? atoi(val) : 0;
	if (on)
		fval |= flag;
	else
		fval ^= flag;

	sprintf(sz, "%d", fval);
	SetKeyValue(ent, "spawnflags", sz);
}

/*
==============
ValueForKey
==============
*/
char *ValueForKey (Entity *ent, char *key)
{
	epair_t	*ep;
	
	if(!ent)	// sikk - return to prevent crash if no *.qe3 file is found
		return "";

	for (ep = ent->epairs; ep; ep = ep->next)
		if (!strcmp(ep->key, key))
			return ep->value;

	return "";
}

/*
==============
SetKeyValue
==============
*/
void SetKeyValue (Entity *ent, char *key, char *value)
{
	epair_t	*ep;

	if (ent == NULL)
		return;

	if (!key || !key[0])
		return;

	for (ep = ent->epairs; ep; ep = ep->next)
	{
		if (!strcmp(ep->key, key))
		{
			free(ep->value);
			break;
		}
	}
	if (!ep)
	{
		ep = (epair_t*)qmalloc(sizeof(*ep));
		ep->next = ent->epairs;
		ent->epairs = ep;
		ep->key = (char*)qmalloc(strlen(key) + 1);
		strcpy(ep->key, key);
	}

	ep->value = (char*)qmalloc(strlen(value) + 1);
	strcpy(ep->value, value);
	g_bModified = true;
}

/*
==============
SetKeyValueVector
==============
*/
void SetKeyValueIVector(Entity *ent, char *key, vec3_t vec)
{
	char szVec[256];
	sprintf(szVec, "%d %d %d", (int)roundf(vec[0]), (int)roundf(vec[1]), (int)roundf(vec[2]));
	SetKeyValue(ent, key, szVec);
}

/*
==============
SetKeyValueFVector
==============
*/
void SetKeyValueFVector(Entity *ent, char *key, vec3_t vec)
{
	char szVec[256];
	sprintf(szVec, "%f %f %f", vec[0], vec[1], vec[2]);
	SetKeyValue(ent, key, szVec);
}

/*
==============
DeleteKey
==============
*/
void DeleteKey (Entity *ent, char *key)
{
	epair_t	**ep, *next;
	
	if (!strcmp(key, "origin"))
	{
		Sys_Printf("WARNING: Point entities must have an origin\n");
		Sys_Beep();
		return;
	}
	ep = &ent->epairs;
	while (*ep)
	{
		next = *ep;
		if (!strcmp(next->key, key))
		{
			*ep = next->next;
			free(next->key);
			free(next->value);
			free(next);
			return;
		}
		ep = &next->next;
	}
}

/*
==============
FloatForKey
==============
*/
float FloatForKey (Entity *ent, char *key)
{
	char *k;
	
	k = ValueForKey(ent, key);
	return atof(k);
}

/*
==============
IntForKey
==============
*/
int IntForKey (Entity *ent, char *key)
{
	char *k;
	
	k = ValueForKey(ent, key);
	return atoi(k);
}

/*
==============
GetVectorForKey
==============
*/
void GetVectorForKey (Entity *ent, char *key, vec3_t vec)
{
	char *k;
	
	k = ValueForKey(ent, key);
	sscanf(k, "%f %f %f", &vec[0], &vec[1], &vec[2]);
}

/*
===============
Entity_Free

Frees the entity and any brushes it has.
The entity is removed from the global entities list.
===============
*/
void Entity_Free (Entity *e)
{
	epair_t	*ep, *next;

	while (e->brushes.onext != &e->brushes)
		delete e->brushes.onext;

	if (e->next)
	{
		e->next->prev = e->prev;
		e->prev->next = e->next;
	}

	for (ep = e->epairs; ep; ep = next)
	{
		next = ep->next;
		free(ep);
	}
	delete e;
}

// sikk---> Undo/Redo
/*
===============
Entity_FreeEpairs

Frees the entity epairs.
===============
*/
void Entity_FreeEpairs (Entity *e)
{
	epair_t	*ep, *next;

	for (ep = e->epairs; ep; ep = next)
	{
		next = ep->next;
		free (ep->key);
		free (ep->value);
		free (ep);
	}
	e->epairs = NULL;
}

/*
===========
Entity_AddToList
===========
*/
void Entity_AddToList (Entity *e, Entity *list)
{
	if (e->next || e->prev)
		Error("Entity_AddToList: Already linked.");
	e->next = list->next;
	list->next->prev = e;
	list->next = e;
	e->prev = list;
}

/*
===========
Entity_RemoveFromList
===========
*/
void Entity_RemoveFromList (Entity *e)
{
	if (!e->next || !e->prev)
		Error("Entity_RemoveFromList: Not linked.");
	e->next->prev = e->prev;
	e->prev->next = e->next;
	e->next = e->prev = NULL;
}

/*
=================
Entity_MemorySize
=================
*/
int Entity_MemorySize (Entity *e)
{
	epair_t	*ep;
	int size = 0;

	for (ep = e->epairs; ep; ep = ep->next)
	{
		size += _msize(ep->key);
		size += _msize(ep->value);
		size += _msize(ep);
	}
	size += _msize(e);
	return size;
}
// <---sikk

/*
=================
ParseEpair
=================
*/
epair_t *ParseEpair ()
{
	epair_t	*e;
	
	e = (epair_t*)qmalloc(sizeof(*e));
	
	e->key = (char*)qmalloc(strlen(g_szToken) + 1);
	strcpy(e->key, g_szToken);

	GetToken(false);
	e->value = (char*)qmalloc(strlen(g_szToken) + 1);
	strcpy(e->value, g_szToken);

	return e;
}

/*
================
Entity_Parse

If onlypairs is set, the classname info will not be looked up, and the entity
will not be added to the global list.  Used for parsing the project.
================
*/
Entity *Entity_Parse (bool onlypairs)
{
	Entity   *ent;
	EntClass   *e;
	Brush	   *b;
	epair_t	   *ep;
	bool		has_brushes;

	if (!GetToken(true))
		return NULL;

	if (strcmp(g_szToken, "{"))
		Error("Entity_Parse: { not found.");
	
	ent = new Entity();

	do
	{
		if (!GetToken(true))
			Error("Entity_Parse: EOF without closing brace.");
		if (!strcmp(g_szToken, "}"))
			break;
		if (!strcmp(g_szToken, "{"))
		{
			b = Brush::Parse();
			b->owner = ent;

			// add to the end of the entity chain
			b->onext = &ent->brushes;
			b->oprev = ent->brushes.oprev;
			ent->brushes.oprev->onext = b;
			ent->brushes.oprev = b;
		}
		else
		{
			ep = ParseEpair();
			ep->next = ent->epairs;
			ent->epairs = ep;
		}
	} while (1);

	if (onlypairs)
		return ent;

	if (ent->brushes.onext == &ent->brushes)
		has_brushes = false;
	else
		has_brushes = true;

	GetVectorForKey(ent, "origin", ent->origin);

	// lunaran - this now creates fixed/non-fixed entclasses on the fly for point entities
	// with brushes or brush entities without any, so that all the downstream code Just Works
	e = EntClass::ForName(ValueForKey(ent, "classname"), has_brushes, false);
	ent->eclass = e;

	if (e->IsFixedSize())
	{	// fixed size entity
		// create a custom brush
		Entity_MakeBrush(ent);
	}

	// add all the brushes to the main list
	for (b = ent->brushes.onext; b != &ent->brushes; b = b->onext)
	{
		b->next = g_brActiveBrushes.next;
		g_brActiveBrushes.next->prev = b;
		b->prev = &g_brActiveBrushes;
		g_brActiveBrushes.next = b;
	}

	return ent;
}

/*
============
Entity_Write
============
*/
void Entity_Write (Entity *e, FILE *f, bool use_region)
{
	epair_t	   *ep;
	Brush	   *b;
	vec3_t		origin;
	int			count;

	// if none of the entities brushes are in the region,
	// don't write the entity at all
	if (use_region)
	{
		// in region mode, save the camera position as playerstart
		if (!strcmp(ValueForKey(e, "classname"), "info_player_start"))
		{
			fprintf(f, "{\n");
			fprintf(f, "\"classname\" \"info_player_start\"\n");
			fprintf(f, "\"origin\" \"%d %d %d\"\n", (int)g_qeglobals.d_camera.origin[0], 
													(int)g_qeglobals.d_camera.origin[1], 
													(int)g_qeglobals.d_camera.origin[2]);
			fprintf(f, "\"angle\" \"%d\"\n", (int)g_qeglobals.d_camera.angles[YAW]);
			fprintf(f, "}\n");
			return;
		}

		for (b = e->brushes.onext; b != &e->brushes; b = b->onext)
			if (!Map_IsBrushFiltered(b))
				break;	// got one

		if (b == &e->brushes)
			return;	// nothing visible
	}

	// if fixedsize, calculate a new origin based on the current brush position
	if (e->eclass->IsFixedSize())// || *ValueForKey(e, "origin"))	// sikk - Point Entity->Brush Entity Hack (added ValueForKey check)
	{
		// lunaran: origin keyvalue is forcibly kept up to date elsewhere
		vec3_t testorg;
		GetVectorForKey(e, "origin", testorg);
		// but let's be sure for now
		if (!VectorCompare(e->origin, testorg))
		{
			Entity_SetOriginFromBrush(e);
			Sys_Printf("WARNING: Entity origins out of sync on %s at (%f %f %f)\n", e->eclass->name, origin[0], origin[1], origin[2]);
		}
	}

	fprintf(f, "{\n");
	for (ep = e->epairs; ep; ep = ep->next)
		fprintf(f, "\"%s\" \"%s\"\n", ep->key, ep->value);

	if (!e->eclass->IsFixedSize())
	{
		count = 0;
		for (b = e->brushes.onext; b != &e->brushes; b = b->onext)
		{
			if (!use_region || !Map_IsBrushFiltered (b))
			{
				fprintf(f, "// brush %d\n", count);
				count++;
				b->Write(f);
			}
		}
	}
	fprintf(f, "}\n");
}

// sikk---> Export Selection (Map/Prefab)

/*
=================
Entity_WriteSelected

lunaran TODO: why doesn't this just use entity_write
=================
*/
void Entity_WriteSelected (Entity *e, FILE *f)
{
	Brush	   *b;
	epair_t	   *ep;
	vec3_t		origin;
	char		text[128];
	int			count;

	for (b = e->brushes.onext; b != &e->brushes; b = b->onext)
		if (Select_IsBrushSelected(b))
			break;	// got one

	if (b == &e->brushes)
		return;		// nothing selected

	// if fixedsize, calculate a new origin based on the current brush position
	if (e->eclass->IsFixedSize())
	{
		VectorSubtract(e->brushes.onext->mins, e->eclass->mins, origin);
		sprintf(text, "%d %d %d", (int)origin[0], (int)origin[1], (int)origin[2]);
		SetKeyValue(e, "origin", text);
	}

	fprintf (f, "{\n");
	for (ep = e->epairs; ep; ep = ep->next)
		fprintf(f, "\"%s\" \"%s\"\n", ep->key, ep->value);

	if (!e->eclass->IsFixedSize())
	{
		count = 0;
		for (b = e->brushes.onext; b != &e->brushes; b = b->onext)
		{
			if (Select_IsBrushSelected(b))
			{
				fprintf(f, "// brush %d\n", count);
				count++;
				b->Write(f);
			}
		}
	}
	fprintf(f, "}\n");
}
// <---sikk


/*
============
Entity_SetOrigin

entity origins are stored/modified three different ways (by entity.origin, by 
"origin" keyvalue, and derived from brush bounds) - always set a point entity's
origin via these functions or they won't stay synced and you're a JERK
============
*/
void Entity_SetOrigin(Entity *ent, vec3_t org)
{
	if (!ent->eclass->IsFixedSize()) return;

	VectorCopy(org, ent->origin);
	SetKeyValueIVector(ent, "origin", org);
	Entity_MakeBrush(ent);
}

// call one of these three after updating the odd one out, to not loop
void Entity_SetOriginFromMember(Entity *ent)
{
	if (!ent->eclass->IsFixedSize()) return;

	SetKeyValueIVector(ent, "origin", ent->origin);
	Entity_MakeBrush(ent);
}

void Entity_SetOriginFromKeyvalue(Entity *ent)
{
	vec3_t	org;
	if (!ent->eclass->IsFixedSize()) return;

	GetVectorForKey(ent, "origin", org);
	VectorCopy(org, ent->origin);
	Entity_MakeBrush(ent);
}

void Entity_SetOriginFromBrush(Entity *ent)
{
	vec3_t	org;

	if (!ent->eclass->IsFixedSize()) return;
	if (ent->brushes.onext == &ent->brushes) return;

	VectorSubtract(ent->brushes.onext->mins, ent->eclass->mins, org);
	SetKeyValueIVector(ent, "origin", org);
	VectorCopy(org, ent->origin);
}

/*
============
Entity_MakeBrush

update the dummy brush for point entities after the origin is overridden (easier 
than translating it) or the classname is changed
============
*/
void Entity_MakeBrush(Entity *e)
{
	Brush		*b;
	vec3_t		mins, maxs;

	// create a custom brush
	VectorAdd(e->eclass->mins, e->origin, mins);
	VectorAdd(e->eclass->maxs, e->origin, maxs);
	if (e->brushes.onext == &e->brushes)
	{
		b = Brush::Create(mins, maxs, &e->eclass->texdef);
		Entity_LinkBrush(e, b);
	}
	else
	{
		b = e->brushes.onext;
		b->Recreate(mins, maxs, &e->eclass->texdef);
	}

	b->Build();
}

/*
============
Entity_Create

Creates a new entity out of the selected_brushes list.
If the entity class is fixed size, the brushes are only used to find a midpoint. 
Otherwise, the brushes have their ownership transfered to the new entity.
============
*/
Entity *Entity_Create (EntClass *ecIn)
{
	Entity		*e;
	EntClass	*c;
	Brush		*b;

	if (!_stricmp(ecIn->name, "worldspawn"))
	{
		Sys_Printf("WARNING: Cannot create a new worldspawn entity.\n");
		return nullptr;
	}

	if (g_brSelectedBrushes.next == g_brSelectedBrushes.prev &&
		g_brSelectedBrushes.next != &g_brSelectedBrushes &&
		g_brSelectedBrushes.next->owner->eclass->IsFixedSize())
	{
		e = g_brSelectedBrushes.next->owner;

		Entity_ChangeClassname(e, ecIn->name);

		Sys_UpdateWindows(W_CAMERA | W_XY | W_Z);
		return e;
	}

	// check to make sure the brushes are ok
	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
		if (b->owner != g_peWorldEntity)
		{
			Sys_Printf("WARNING: Entity NOT created, brushes not all from world.\n");
			Sys_Beep();
			return nullptr;
		}

	if (Select_HasBrushes() == ecIn->IsFixedSize())	// x0r!
	{
		// check with user that we want to apply the 'wrong' eclass
		if (!ConfirmClassnameHack(ecIn))
			return nullptr;

		// this will spit back a reversed-form eclass
		c = EntClass::ForName(ecIn->name, Select_HasBrushes(), false);
	}
	else
		c = ecIn;

	// create it
	e = new Entity();
	e->eclass = c;
	SetKeyValue(e, "classname", c->name);

	// add the entity to the entity list
	e->next = g_entEntities.next;
	g_entEntities.next = e;
	e->next->prev = e;
	e->prev = &g_entEntities;

	if (c->IsFixedSize())
	{
		// TODO: pass the location of the right-click via an appropriate method
		VectorCopy(g_brSelectedBrushes.mins, e->origin);
		SetKeyValueIVector(e, "origin", e->origin);
		VectorCopy(g_v3VecOrigin, g_brSelectedBrushes.mins);	// reset

		Entity_MakeBrush(e);

		Select_HandleBrush(e->brushes.onext,false);
	}
	else
	{
		// change the selected brushes over to the new entity
		for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
		{
			Entity_UnlinkBrush(b);
			Entity_LinkBrush(e, b);
			b->Build();	// so the key brush gets a name
		}
	}

	Sys_UpdateWindows(W_CAMERA|W_XY|W_Z);
	return e;
}

/*
===========
Entity_ChangeClassname
===========
*/
void Entity_ChangeClassname(Entity *ent, char *value)
{
	bool hasbrushes;

	hasbrushes = (ent->brushes.onext != ent->brushes.oprev);
	EntClass* ec = EntClass::ForName(value, hasbrushes, false);
	ent->eclass = ec;
	SetKeyValue(ent, "classname", value);

	if (ec->IsFixedSize())
	{
		// make a new brush for the entity
		Entity_MakeBrush(ent);
	}
	g_bSelectionChanged = true;
	Sys_UpdateWindows(W_CAMERA | W_XY | W_Z);
}

/*
===========
Entity_LinkBrush
===========
*/
void Entity_LinkBrush (Entity *e, Brush *b)
{
	if (b->oprev || b->onext)
		Error("Entity_LinkBrush: Already linked.");
	b->owner = e;
	b->onext = e->brushes.onext;
	b->oprev = &e->brushes;
	e->brushes.onext->oprev = b;
	e->brushes.onext = b;
}

/*
===========
Entity_UnlinkBrush
===========
*/
void Entity_UnlinkBrush (Brush *b)
{
	if (!b->owner || !b->onext || !b->oprev)
		Error("Entity_UnlinkBrush: Not currently linked.");
	b->onext->oprev = b->oprev;
	b->oprev->onext = b->onext;
	b->onext = b->oprev = NULL;
	b->owner = NULL;
}

/*
===========
Entity_Clone
===========
*/
Entity *Entity_Clone (Entity *e)
{
	Entity	*n;
	epair_t		*ep, *np;

	n = new Entity();
	n->eclass = e->eclass;

	// add the entity to the entity list
	n->next = g_entEntities.next;
	g_entEntities.next = n;
	n->next->prev = n;
	n->prev = &g_entEntities;

	for (ep = e->epairs; ep; ep = ep->next)
	{
		np = (epair_t*)qmalloc(sizeof(*np));
		np->key = CopyString(ep->key);
		np->value = CopyString(ep->value);
		np->next = n->epairs;
		n->epairs = np;
	}
	VectorCopy(e->origin, n->origin);
	return n;
}

/*
==============
GetUniqueTargetId
==============
*/
int GetUniqueTargetId (int iHint)
{
	int			iMin, iMax, i;
	bool		bFound;
	Entity   *pe;
	
	bFound = false;
	pe = g_entEntities.next;
	iMin = 0; 
	iMax = 0;
	
	for ( ; pe != NULL && pe != &g_entEntities; pe = pe->next)
	{
		i = IntForKey(pe, "target");
		if (i)
		{
			iMin = min(i, iMin);
			iMax = max(i, iMax);
			if (i == iHint)
				bFound = true;
		}
	}

	if (bFound)
		return iMax + 1;
	else
		return iHint;
}

/*
==============
FindEntity
==============
*/
Entity *FindEntity (char *pszKey, char *pszValue)
{
	Entity *pe;
	
	pe = g_entEntities.next;
	
	for ( ; pe != NULL && pe != &g_entEntities; pe = pe->next)
		if (!strcmp(ValueForKey(pe, pszKey), pszValue))
			return pe;

	return NULL;
}

/*
==============
FindEntityInt
==============
*/
Entity *FindEntityInt (char *pszKey, int iValue)
{
	Entity *pe;
	
	pe = g_entEntities.next;
	
	for ( ; pe != NULL && pe != &g_entEntities; pe = pe->next)
		if (IntForKey(pe, pszKey) == iValue)
			return pe;

	return NULL;
}

// sikk---> Cut/Copy/Paste
/*
===============
Entity_CleanList
===============
*/
void Entity_CleanList ()
{
	Entity	*pe, *next;
	epair_t		*ep, *enext;

	pe = g_entCopiedEntities.next;

	while (pe != NULL && pe != &g_entCopiedEntities)
	{
		next = pe->next;
		enext = NULL;
		for (ep = pe->epairs; ep; ep = enext)
		{
			enext = ep->next;
			free(ep->key);
			free(ep->value);
			free(ep);
		}
		free (pe);
		pe = next;
	}
	g_entCopiedEntities.next = g_entCopiedEntities.prev = &g_entCopiedEntities;
}

/*
===============
Entity_Copy
===============
*/
Entity *Entity_Copy (Entity *e)
{
	Entity	*n;
	epair_t		*ep, *np;

	n = new Entity();
	n->eclass = e->eclass;

	// add the entity to the entity list
	n->next = g_entCopiedEntities.next;
	g_entCopiedEntities.next = n;
	n->next->prev = n;
	n->prev = &g_entCopiedEntities;

	for (ep = e->epairs; ep; ep = ep->next)
	{
		np = (epair_t*)qmalloc(sizeof(*np));
		np->key = CopyString(ep->key);
		np->value = CopyString(ep->value);
		np->next = n->epairs;
		n->epairs = np;
	}
	return n;
}
// <---sikk

//==============================
//	entity.c
//==============================

#include "qe3.h"
#include "io.h"

// sikk - eclass.c merged here
/*
the classname, color triple, and bounding box are parsed out of comments
A ? size means take the exact brush size.

/*QUAKED <classname> (0 0 0) ?
/*QUAKED <classname> (0 0 0) (-8 -8 -8) (8 8 8)

Flag names can follow the size description:

/*QUAKED func_door (0 .5 .8) ? START_OPEN STONE_SOUND DOOR_DONT_LINK GOLD_KEY SILVER_KEY
*/

eclass_t   *g_pecEclass;
eclass_t   *g_pecEclassBad;
char	   *g_szDebugName;

int g_nEntityId = 1;	// sikk - Undo/Redo


/*
==============
Eclass_InitFromText
==============
*/
eclass_t *Eclass_InitFromText (char *text)
{
	char	   *t;
	int			len;
	int			r, i;
	char	   *p, parms[256];
	eclass_t   *e;
	char		color[128];

	e = qmalloc(sizeof(*e));
	memset(e, 0, sizeof(*e));
	
	text += strlen("/*QUAKED ");
	
	// grab the name
	text = COM_Parse(text);
	e->name = qmalloc(strlen(g_szComToken) + 1);
	strcpy(e->name, g_szComToken);
	g_szDebugName = e->name;
	
	// grab the color, reformat as texture name
	r = sscanf(text, " (%f %f %f)", &e->color[0], &e->color[1], &e->color[2]);
	if (r != 3)
		return e;
	sprintf(color, "(%f %f %f)", e->color[0], e->color[1], e->color[2]);
	strcpy(e->texdef.name, color);

	while (*text != ')')
	{
		if (!*text)
			return e;
		text++;
	}
	text++;
	
	// get the size	
	text = COM_Parse(text);
	if (g_szComToken[0] == '(')
	{	// parse the size as two vectors
		e->fixedsize = true;
		r = sscanf(text,"%f %f %f) (%f %f %f)", &e->mins[0], &e->mins[1], 
												&e->mins[2], &e->maxs[0], 
												&e->maxs[1], &e->maxs[2]);
		if (r != 6)
			return e;

		for (i = 0; i < 2; i++)
		{
			while (*text != ')')
			{
				if (!*text)
					return e;
				text++;
			}
			text++;
		}
	}
	else
	{	// use the brushes
	}
	
	// get the flags
	
	// copy to the first /n
	p = parms;
	while (*text && *text != '\n')
		*p++ = *text++;
	*p = 0;
	text++;
	
	// any remaining words are parm flags
	p = parms;
	for (i = 0; i < 8; i++)
	{
		p = COM_Parse(p);
		if (!p)
			break;
		strcpy(e->flagnames[i], g_szComToken);
	} 

	// find the length until close comment
	for (t = text; t[0] && !(t[0] == '*' && t[1] == '/'); t++)
		;
	
	// copy the comment block out
	len = t - text;
	e->comments = qmalloc(len + 1);
	memcpy(e->comments, text, len);
#if 0
	for (i = 0; i < len; i++)
		if (text[i] == '\n')
			e->comments[i] = '\r';
		else
			e->comments[i] = text[i];
#endif
	e->comments[len] = 0;

	// setup show flags
	e->nShowFlags = 0;
	if (strcmpi(e->name, "light") == 0)
		e->nShowFlags |= ECLASS_LIGHT;

	if ((strnicmp(e->name, "info_player_start",			strlen("info_player_start")) == 0) || 
		(strnicmp(e->name, "info_player_start2",		strlen("info_player_start2")) == 0) || 
		(strnicmp(e->name, "info_player_deathmatch",	strlen("info_player_deathmatch")) == 0) || 
		(strnicmp(e->name, "info_player_coop",			strlen("info_player_coop")) == 0) || 
		(strnicmp(e->name, "info_teleport_destination",	strlen("info_teleport_destination")) == 0) || 
		(strnicmp(e->name, "info_intermission",			strlen("info_intermission")) == 0) || 
// sikk---> added monsters
		(strnicmp(e->name, "monster_",					strlen("monster_")) == 0) || 
// <---sikk
		(strnicmp(e->name, "path_corner",				strlen("path_corner")) == 0) || 
		(strnicmp(e->name, "viewthing",					strlen("viewthing")) == 0))
		e->nShowFlags |= ECLASS_ANGLE;

	if (strcmpi(e->name, "path") == 0)
		e->nShowFlags |= ECLASS_PATH;
	
	return e;
}

/*
=================
Eclass_InsertAlphabetized
=================
*/
void Eclass_InsertAlphabetized (eclass_t *e)
{
	eclass_t	*s;
	
	if (!g_pecEclass)
	{
		g_pecEclass = e;
		return;
	}

	s = g_pecEclass;
	if (stricmp (e->name, s->name) < 0)
	{
		e->next = s;
		g_pecEclass = e;
		return;
	}

	do
	{
		if (!s->next || stricmp (e->name, s->next->name) < 0)
		{
			e->next = s->next;
			s->next = e;
			return;
		}
		s = s->next;
	} while (1);
}

/*
=================
Eclass_ScanFile
=================
*/
void Eclass_ScanFile (char *filename)
{
	int			size;
	char	   *data;
	eclass_t   *e;
	int			i;
	char		temp[1024];

	QE_ConvertDOSToUnixName(temp, filename);

	Sys_Printf("CMD: ScanFile: %s\n", temp);

	size = LoadFile(filename, (void *)&data);
	
	for (i = 0; i < size; i++)
	{
		if (!strncmp(data+i, "/*QUAKED",8))
		{
			e = Eclass_InitFromText(data + i);
			if (e)
				Eclass_InsertAlphabetized(e);
			else
				printf("Error parsing: %s in %s\n", g_szDebugName, filename);
		}
	}
		
	free(data);
}

/*
==============
Eclass_InitForSourceDirectory
==============
*/
void Eclass_InitForSourceDirectory (char *path)
	{
	struct _finddata_t	fileinfo;
	int		handle;
	char	filename[1024];
	char	filebase[1024];
	char    temp[1024];
	char   *s;

	QE_ConvertDOSToUnixName(temp, path);

	Sys_Printf("CMD: ScanEntityPath: %s\n", temp );

	strcpy(filebase, path);
	s = filebase + strlen(filebase) - 1;
	while (*s != '\\' && *s != '/' && s != filebase)
		s--;
	*s = 0;

	g_pecEclass = NULL;

	handle = _findfirst(path, &fileinfo);
	if (handle != -1)
	{
		do
		{
			sprintf(filename, "%s\\%s", filebase, fileinfo.name);
			Eclass_ScanFile(filename);
		} while (_findnext(handle, &fileinfo) != -1);

		_findclose(handle);
	}

	g_pecEclassBad = Eclass_InitFromText("/*QUAKED UNKNOWN_CLASS (0 0.5 0) ?");
}

/*
==============
Eclass_ForName
==============
*/
eclass_t *Eclass_ForName (char *name, bool has_brushes)
{
	eclass_t   *e;
	char		init[1024];

	if (!name)
		return g_pecEclassBad;

	for (e = g_pecEclass; e; e = e->next)
		if (!strcmp(name, e->name))
			return e;

	// create a new class for it
	if (has_brushes)
	{
		sprintf(init, "/*QUAKED %s (0 0.5 0) ?\nNot found in source.\n", name);
		e = Eclass_InitFromText(init);
	}
	else
	{
		sprintf(init, "/*QUAKED %s (0 0.5 0) (-8 -8 -8) (8 8 8)\nNot found in source.\n", name);
		e = Eclass_InitFromText(init);
	}

	Eclass_InsertAlphabetized(e);

	return e;
}

//===================================================================

/*
==============
SetSpawnFlag
set an individual spawnflag without affecting the others
==============
*/
void SetSpawnFlag(entity_t *ent, int flag, bool on)
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
char *ValueForKey (entity_t *ent, char *key)
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
void SetKeyValue (entity_t *ent, char *key, char *value)
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
			ep->value = qmalloc(strlen(value) + 1);
			strcpy(ep->value, value);
			return;
		}
	}
	ep = qmalloc(sizeof(*ep));
	ep->next = ent->epairs;
	ent->epairs = ep;
	ep->key = qmalloc(strlen(key) + 1);
	strcpy(ep->key, key);
	ep->value = qmalloc(strlen(value) + 1);
	strcpy(ep->value, value);
}

/*
==============
DeleteKey
==============
*/
void DeleteKey (entity_t *ent, char *key)
{
	epair_t	**ep, *next;
	
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
float FloatForKey (entity_t *ent, char *key)
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
int IntForKey (entity_t *ent, char *key)
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
void GetVectorForKey (entity_t *ent, char *key, vec3_t vec)
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
void Entity_Free (entity_t *e)
{
	epair_t	*ep, *next;

	while (e->brushes.onext != &e->brushes)
		Brush_Free(e->brushes.onext);

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
	free(e);
}

// sikk---> Undo/Redo
/*
===============
Entity_FreeEpairs

Frees the entity epairs.
===============
*/
void Entity_FreeEpairs (entity_t *e)
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
void Entity_AddToList (entity_t *e, entity_t *list)
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
void Entity_RemoveFromList (entity_t *e)
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
int Entity_MemorySize (entity_t *e)
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
	
	e = qmalloc(sizeof(*e));
	
	e->key = qmalloc(strlen(g_szToken) + 1);
	strcpy(e->key, g_szToken);

	GetToken(false);
	e->value = qmalloc(strlen(g_szToken) + 1);
	strcpy(e->value, g_szToken);

	return e;
}

/*
================
Entity_Parse

If onlypairs is set, the classname info will not
be looked up, and the entity will not be added
to the global list.  Used for parsing the project.
================
*/
entity_t *Entity_Parse (bool onlypairs)
{
	entity_t   *ent;
	eclass_t   *e;
	brush_t	   *b;
	vec3_t		mins, maxs;
	epair_t	   *ep;
	bool		has_brushes;

	if (!GetToken(true))
		return NULL;

	if (strcmp(g_szToken, "{"))
		Error("Entity_Parse: { not found.");
	
	ent = qmalloc(sizeof(*ent));
	ent->entityId = g_nEntityId++;
	ent->brushes.onext = ent->brushes.oprev = &ent->brushes;

	do
	{
		if (!GetToken(true))
			Error("Entity_Parse: EOF without closing brace.");
		if (!strcmp(g_szToken, "}"))
			break;
		if (!strcmp(g_szToken, "{"))
		{
			b = Brush_Parse();
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

	e = Eclass_ForName(ValueForKey(ent, "classname"), has_brushes);
	ent->eclass = e;
	if (e->fixedsize)
	{	// fixed size entity
		if (ent->brushes.onext != &ent->brushes)
		{
			Sys_Printf("WARNING: Fixed size entity with brushes\n");
#if 0
			while (ent->brushes.onext != &ent->brushes)
			{	// FIXME: this will free the entity and crash!
				Brush_Free(b);
			}
#endif
			ent->brushes.next = ent->brushes.prev = &ent->brushes;
		}

		// create a custom brush
		VectorAdd(e->mins, ent->origin, mins);
		VectorAdd(e->maxs, ent->origin, maxs);
		b = Brush_Create(mins, maxs, &e->texdef);
		b->owner = ent;

		b->onext = ent->brushes.onext;
		b->oprev = &ent->brushes;
		ent->brushes.onext->oprev = b;
		ent->brushes.onext = b;
	}
	else
	{	// brush entity
// sikk---> Point Entity->Brush Entity Hack
		// This creates a brush for brush entities defined as point entities
		// This is a valued mapping trick to save on bmodel caches. If I find a reason
		// for a brush entity to have an "origin" key defined I'll need to change this
		if (*ValueForKey(ent, "origin"))
		{
			Sys_Printf("WARNING: Brush entity with no brushes\n");
			e->mins[0] = e->mins[2] = e->mins[1] = -8;
			e->maxs[0] = e->maxs[1] = e->maxs[2] =8;
			// create a custom brush
			VectorAdd(e->mins, ent->origin, mins);
			VectorAdd(e->maxs, ent->origin, maxs);
			b = Brush_Create(mins, maxs, &e->texdef);
			b->owner = ent;

			b->onext = ent->brushes.onext;
			b->oprev = &ent->brushes;
			ent->brushes.onext->oprev = b;
			ent->brushes.onext = b;
		}
// <---sikk
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
void Entity_Write (entity_t *e, FILE *f, bool use_region)
{
	epair_t	   *ep;
	brush_t	   *b;
	vec3_t		origin;
	char		text[128];
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

	// if fixedsize, calculate a new origin based on the current
	// brush position
	if (e->eclass->fixedsize || ValueForKey(e, "origin") != "")	// sikk - Point Entity->Brush Entity Hack (added ValueForKey check)
	{
		VectorSubtract(e->brushes.onext->mins, e->eclass->mins, origin);
		sprintf(text, "%d %d %d", (int)origin[0], (int)origin[1], (int)origin[2]);
		SetKeyValue(e, "origin", text);
	}

	fprintf(f, "{\n");
	for (ep = e->epairs; ep; ep = ep->next)
		fprintf(f, "\"%s\" \"%s\"\n", ep->key, ep->value);

	if (!e->eclass->fixedsize)
	{
		if (!*ValueForKey(e, "origin"))	// sikk - Point Entity->Brush Entity Hack (added ValueForKey check)
		{
			count = 0;
			for (b = e->brushes.onext; b != &e->brushes; b = b->onext)
			{
				if (!use_region || !Map_IsBrushFiltered (b))
				{
					fprintf(f, "// brush %d\n", count);
					count++;
					Brush_Write(b, f);
				}
			}
		}
	}
	fprintf(f, "}\n");
}

// sikk---> Export Selection (Map/Prefab)
/*
=================
IsBrushSelected
=================
*/
bool IsBrushSelected (brush_t* bSel)
{
	brush_t* b;

	for (b = g_brSelectedBrushes.next; b != NULL && b != &g_brSelectedBrushes; b = b->next)
		if (b == bSel)
			return true;

	return false;
}

/*
=================
Entity_WriteSelected
=================
*/
void Entity_WriteSelected (entity_t *e, FILE *f)
{
	brush_t	   *b;
	epair_t	   *ep;
	vec3_t		origin;
	char		text[128];
	int			count;

	for (b = e->brushes.onext; b != &e->brushes; b = b->onext)
		if (IsBrushSelected(b))
			break;	// got one

	if (b == &e->brushes)
		return;		// nothing selected

	// if fixedsize, calculate a new origin based on the current brush position
	if (e->eclass->fixedsize)
	{
		VectorSubtract(e->brushes.onext->mins, e->eclass->mins, origin);
		sprintf(text, "%d %d %d", (int)origin[0], (int)origin[1], (int)origin[2]);
		SetKeyValue(e, "origin", text);
	}

	fprintf (f, "{\n");
	for (ep = e->epairs; ep; ep = ep->next)
		fprintf(f, "\"%s\" \"%s\"\n", ep->key, ep->value);

	if (!e->eclass->fixedsize)
	{
		count = 0;
		for (b = e->brushes.onext; b != &e->brushes; b = b->onext)
		{
			if (IsBrushSelected(b))
			{
				fprintf(f, "// brush %d\n", count);
				count++;
				Brush_Write(b, f);
			}
		}
	}
	fprintf(f, "}\n");
}
// <---sikk

/*
============
Entity_Create

Creates a new entity out of the selected_brushes list.
If the entity class is fixed size, the brushes are only
used to find a midpoint.  Otherwise, the brushes have
their ownership transfered to the new entity.
============
*/
entity_t *Entity_Create (eclass_t *c)
{
	entity_t   *e;
	brush_t	   *b;
	vec3_t		mins, maxs;
	int			i;

	// check to make sure the brushes are ok
	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
		if (b->owner != g_peWorldEntity)
		{
			Sys_Printf("WARNING: Entity NOT created, brushes not all from world.\n");
			Sys_Beep();
			return NULL;
		}

	// create it
	e = qmalloc(sizeof(*e));
	e->entityId = g_nEntityId++;	// sikk - Undo/Redo
	e->brushes.onext = e->brushes.oprev = &e->brushes;
	e->eclass = c;
	SetKeyValue(e, "classname", c->name);

	// add the entity to the entity list
	e->next = g_entEntities.next;
	g_entEntities.next = e;
	e->next->prev = e;
	e->prev = &g_entEntities;

	if (c->fixedsize)
	{
		// just use the selection for positioning
		b = g_brSelectedBrushes.next;
		for (i = 0; i < 3; i++)
			e->origin[i] = b->mins[i] - c->mins[i];

		// create a custom brush
		VectorAdd(c->mins, e->origin, mins);
		VectorAdd(c->maxs, e->origin, maxs);
		b = Brush_Create(mins, maxs, &c->texdef);

		Entity_LinkBrush(e, b);

		// delete the current selection
		Select_Delete();

		// select the new brush
		b->next = b->prev = &g_brSelectedBrushes;
		g_brSelectedBrushes.next = g_brSelectedBrushes.prev = b;

		Brush_Build(b);
	}
	else
	{
		// change the selected brushes over to the new entity
		for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
		{
			Entity_UnlinkBrush(b);
			Entity_LinkBrush(e, b);
			Brush_Build(b);	// so the key brush gets a name
		}
	}

	EntWnd_UpdateEntitySel(e->eclass);	// sikk - Update Enitity Inspector 

	Sys_UpdateWindows(W_ALL);
	return e;
}

/*
===========
Entity_LinkBrush
===========
*/
void Entity_LinkBrush (entity_t *e, brush_t *b)
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
void Entity_UnlinkBrush (brush_t *b)
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
entity_t *Entity_Clone (entity_t *e)
{
	entity_t	*n;
	epair_t		*ep, *np;

	n = qmalloc(sizeof(*n));
	n->entityId = g_nEntityId++;	// sikk - Undo/Redo
	n->brushes.onext = n->brushes.oprev = &n->brushes;
	n->eclass = e->eclass;

	// add the entity to the entity list
	n->next = g_entEntities.next;
	g_entEntities.next = n;
	n->next->prev = n;
	n->prev = &g_entEntities;

	for (ep = e->epairs; ep; ep = ep->next)
	{
		np = qmalloc(sizeof(*np));
		np->key = CopyString(ep->key);
		np->value = CopyString(ep->value);
		np->next = n->epairs;
		n->epairs = np;
	}

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
	entity_t   *pe;
	
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
entity_t *FindEntity (char *pszKey, char *pszValue)
{
	entity_t *pe;
	
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
entity_t *FindEntityInt (char *pszKey, int iValue)
{
	entity_t *pe;
	
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
	entity_t	*pe, *next;
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
entity_t *Entity_Copy (entity_t *e)
{
	entity_t	*n;
	epair_t		*ep, *np;

	n = (entity_t*)qmalloc(sizeof(*n));
	n->entityId = g_nEntityId++;	// sikk - Undo/Redo
	n->brushes.onext = n->brushes.oprev = &n->brushes;
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

//==============================
//	entity.cpp
//==============================

#include "qe3.h"
#include "io.h"

int g_nEntityId = 0;	// sikk - Undo/Redo

//===================================================================


Entity::Entity() :
	next(nullptr), prev(nullptr), 
	eclass(nullptr), epairs(nullptr), 
	undoId(0), redoId(0), ownerId(0)
{
	entityId = ++g_nEntityId;
	brushes.onext = brushes.oprev = &brushes;
	brushes.owner = this;
	origin[0] = origin[1] = origin[2] = 0;
}


/*
===============
Entity::~Entity

Frees the entity and any brushes it has.
The entity is removed from the global entities list.
===============
*/
Entity::~Entity()
{
	while (brushes.onext != &brushes)
		delete brushes.onext;

	if (next)
	{
		next->prev = prev;
		prev->next = next;
	}

	EPair *ep, *epn;
	for (ep = epairs; ep; ep = epn)
	{
		epn = ep->next;
		delete ep;
	}
}


//===================================================================

/*
==============
Entity::SetSpawnFlag
set an individual spawnflag without affecting the others
==============
*/
void Entity::SetSpawnFlag(int flag, bool on)
{
//	char*	val;
	int		ival;

	ival = GetKeyValueInt("spawnflags");
	if (on)
		ival |= flag;
	else
		ival ^= flag;

	SetKeyValue("spawnflags", ival);
}

/*
==============
Entity::SetKeyValue
==============
*/
void Entity::SetKeyValue(const char *key, const char *value)
{
	EPair	*ep;
	int vlen = strlen(value);

	if (!key || !key[0])
		return;

	for (ep = epairs; ep; ep = ep->next)
	{
		if (ep->key == key)
		{
			if ((int)ep->value.size() <= vlen)
				ep->value.resize(vlen + 8);
			//ep->value.zero();
			break;
		}
	}
	if (!ep)
	{
		ep = new EPair();
		ep->next = epairs;
		epairs = ep;
		ep->key.resize(strlen(key) + 8);
		strcpy((char*)*ep->key, key);
		ep->value.resize(vlen + 8);
	}

	strcpy((char*)*ep->value, value);
	g_map.modified = true;
}

/*
==============
Entity::SetKeyValue
==============
*/
void Entity::SetKeyValue(const char *key, const float fvalue)
{
	char sz[64];
	sprintf(sz, "%f", fvalue);
	SetKeyValue(key, sz);
}

/*
==============
Entity::SetKeyValue
==============
*/
void Entity::SetKeyValue(const char *key, const int ivalue)
{
	char sz[64];
	sprintf(sz, "%i", ivalue);
	SetKeyValue(key, sz);
}

/*
==============
Entity::SetKeyValueFVector
==============
*/
void Entity::SetKeyValueFVector(const char *key, const vec3_t vec)
{
	char szVec[128];
	sprintf(szVec, "%f %f %f", vec[0], vec[1], vec[2]);
	SetKeyValue(key, szVec);
}

/*
==============
Entity::SetKeyValueIVector
==============
*/
void Entity::SetKeyValueIVector(const char *key, const vec3_t vec)
{
	char szVec[128];
	sprintf(szVec, "%d %d %d", (int)roundf(vec[0]), (int)roundf(vec[1]), (int)roundf(vec[2]));
	SetKeyValue(key, szVec);
}


/*
==============
Entity::GetKeyValue
==============
*/
char *Entity::GetKeyValue(const char *key) const
{
	EPair	*ep;

	for (ep = epairs; ep; ep = ep->next)
		if (ep->key == key)
			return (char*)*ep->value;
	return "";
}

/*
==============
Entity::GetKeyValueFloat
==============
*/
float Entity::GetKeyValueFloat(const char *key) const
{
	char* cv = GetKeyValue(key);
	if (!*cv) return 0.0;
	return atof(cv);
}

/*
==============
Entity::GetKeyValueInt
==============
*/
int Entity::GetKeyValueInt(const char *key) const
{
	char* cv = GetKeyValue(key);
	if (!*cv) return 0.0;
	return atoi(cv);
}

/*
==============
Entity::GetKeyValueVector
==============
*/
void Entity::GetKeyValueVector(const char *key, vec3_t vec) const
{
	char *cv = GetKeyValue(key);
	if (!*cv) VectorCopy(g_v3VecOrigin, vec);
	sscanf(cv, "%f %f %f", &vec[0], &vec[1], &vec[2]);
}

/*
==============
Entity::DeleteKeyValue
==============
*/
void Entity::DeleteKeyValue(const char *key)
{
	EPair	**ep, *next;

	if (!strcmp(key, "origin"))
	{
		Sys_Printf("WARNING: Point entities must have an origin\n");
		Sys_Beep();
		return;
	}
	ep = &epairs;
	while (*ep)
	{
		next = *ep;
		if (next->key == key)
		{
			*ep = next->next;
			delete next;
			return;
		}
		ep = &next->next;
	}
}


//===================================================================


// sikk---> Undo/Redo
/*
===============
Entity::FreeEpairs

Frees the entity epairs.
===============
*/
void Entity::FreeEpairs ()
{
	EPair	*ep, *next;

	for (ep = epairs; ep; ep = next)
	{
		next = ep->next;
		delete ep;
	}
	epairs = nullptr;
}

/*
===========
Entity::AddToList
===========
*/
void Entity::AddToList (Entity *list)
{
	if (next || prev)
		Error("Entity_AddToList: Already linked.");
	next = list->next;
	list->next->prev = this;
	list->next = this;
	prev = list;
}

/*
=================
Entity::MergeListIntoList
=================
*/
void Entity::MergeListIntoList(Entity *src, Entity *dest)
{
	// properly doubly-linked lists only
	if (!src->next || !src->prev)
	{
		Error("Tried to merge a list with NULL links!\n");
		return;
	}

	if (src->next == src || src->prev == src)
	{
		Sys_Printf("WARNING: Tried to merge an empty list.\n");
		return;
	}
	// merge at head of list
	src->next->prev = dest;
	src->prev->next = dest->next;
	dest->next->prev = src->prev;
	dest->next = src->next;

	/*
	// merge at tail of list
	dest->prev->next = src->next;
	src->next->prev = dest->prev;
	dest->prev = src->prev;
	src->prev->next = dest;
	*/

	src->prev = src->next = src;
}

/*
===========
Entity::RemoveFromList
===========
*/
void Entity::RemoveFromList ()
{
	if (!next || !prev)
		Error("Entity::RemoveFromList: Not linked.");
	next->prev = prev;
	prev->next = next;
	next = prev = NULL;
}

/*
===========
Entity::CloseLinks
===========
*/
void Entity::CloseLinks()
{
	if (next == prev && prev == this)
		return;	// done

	if (next || prev)
		Error("Entity: tried to close non-empty linked list.");

	next = prev = this;
}


/*
=================
Entity::MemorySize
=================
*/
int Entity::MemorySize ()
{
	EPair	*ep;
	int size = 0;

	for (ep = epairs; ep; ep = ep->next)
	{
		size += ep->key.size();
		size += ep->value.size();
		size += sizeof(*ep);
	}
	size += sizeof(Entity);
	return size;
}
// <---sikk

/*
=================
ParseEpair
=================
*/
EPair *ParseEpair ()
{
	EPair	*e;
	
	e = new EPair();
	
	e->key.resize(strlen(g_szToken) + 8);
	strcpy((char*)*e->key, g_szToken);

	GetToken(false);
	e->value.resize(strlen(g_szToken) + 8);
	strcpy((char*)*e->value, g_szToken);

	return e;
}

/*
================
Entity::Parse

If onlypairs is set, the classname info will not be looked up, and the entity
will not be added to the global list.  Used for parsing the project.
================
*/
Entity *Entity::Parse (bool onlypairs)
{
	Entity		*ent;
	EntClass	*e;
	Brush		*b;
	EPair		*ep;
	bool		has_brushes;

	if (!GetToken(true))
		return NULL;

	if (strcmp(g_szToken, "{"))
		Error("Entity_Parse: { not found.");
	
	ent = new Entity();
	ent->brushes.CloseLinks();

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

	ent->GetKeyValueVector("origin", ent->origin);

	// lunaran - this now creates fixed/non-fixed entclasses on the fly for point entities
	// with brushes or brush entities without any, so that all the downstream code Just Works
	e = EntClass::ForName(ent->GetKeyValue("classname"), has_brushes, false);
	ent->eclass = e;

	if (e->IsFixedSize())
	{	// create a custom brush
		ent->MakeBrush();
	}

	// lunaran - entities do not add their brushes to a list by default now, map::load/save/etc does it
	// this is to keep loads/pastes/etc isolated until the parse is complete for exception guarantee
	return ent;
}


void Entity::CheckOrigin()
{
	// if fixedsize, calculate a new origin based on the current brush position
	if (eclass->IsFixedSize())
	{
		// lunaran: origin keyvalue is forcibly kept up to date elsewhere
		vec3_t testorg, org;
		GetKeyValueVector("origin", testorg);
		// but let's be sure for now
		if (!VectorCompare(origin, testorg))
		{
			SetOriginFromBrush();
			Sys_Printf("WARNING: Entity origins out of sync on %s at (%f %f %f)\n", eclass->name, org[0], org[1], org[2]);
		}
	}
}

/*
============
Entity::Write
============
*/
void Entity::Write(std::ostream &out, bool use_region)
{
	EPair	*ep;
	Brush	*b;
	int		count;

	if (use_region)
	{
		// in region mode, save the camera position as playerstart
		if (!strcmp(GetKeyValue("classname"), "info_player_start"))
		{
			out << "{\n";
			out << "\"classname\" \"info_player_start\"\n";
			out << "\"origin\" \"" << (int)g_qeglobals.d_camera.origin[0] << " " << 
									(int)g_qeglobals.d_camera.origin[1] << " " << 
									(int)g_qeglobals.d_camera.origin[2] << "\"\n";
			out << "\"angle\" \"" << (int)g_qeglobals.d_camera.angles[YAW] << "\"\n";
			out << "}\n";
			return;
		}

		for (b = brushes.onext; b != &brushes; b = b->onext)
			if (!g_map.IsBrushFiltered(b))
				break;	// got one

		if (b == &brushes)
			return;	// nothing visible
	}

	CheckOrigin();

	out << "{\n";
	for (ep = epairs; ep; ep = ep->next)
		out << "\"" << (char*)*ep->key << "\" \"" << (char*)*ep->value << "\"\n";

	if (!eclass->IsFixedSize())
	{
		count = 0;
		for (b = brushes.onext; b != &brushes; b = b->onext)
		{
			if (!use_region || !g_map.IsBrushFiltered(b))
			{
				out << "// brush " << count << "\n";
				count++;
				b->Write(out);
			}
		}
	}
	out << "}\n";
}
/*
void Entity::Write (FILE *f, bool use_region)
{
	EPair	*ep;
	Brush	*b;
	int		count;

	// if none of the entities brushes are in the region,
	// don't write the entity at all
	if (use_region)
	{
		// in region mode, save the camera position as playerstart
		if (!strcmp(GetKeyValue("classname"), "info_player_start"))
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

		for (b = brushes.onext; b != &brushes; b = b->onext)
			if (!g_map.IsBrushFiltered(b))
				break;	// got one

		if (b == &brushes)
			return;	// nothing visible
	}

	CheckOrigin();

	fprintf(f, "{\n");
	for (ep = epairs; ep; ep = ep->next)
		fprintf(f, "\"%s\" \"%s\"\n", (char*)*ep->key, (char*)*ep->value);

	if (!eclass->IsFixedSize())
	{
		count = 0;
		for (b = brushes.onext; b != &brushes; b = b->onext)
		{
			if (!use_region || !g_map.IsBrushFiltered (b))
			{
				fprintf(f, "// brush %d\n", count);
				count++;
				b->Write(f);
			}
		}
	}
	fprintf(f, "}\n");
}
*/
// sikk---> Export Selection (Map/Prefab)

/*
=================
Entity::WriteSelected
=================
*/
void Entity::WriteSelected(std::ostream &out)
{
	Brush	*b;
	EPair	*ep;
	int		count;

	// a .map with no worldspawn is broken, so always write worldspawn even if it's empty
	if (eclass == EntClass::worldspawn)
		b = brushes.onext;
	else
		for (b = brushes.onext; b != &brushes; b = b->onext)
			if (Select_IsBrushSelected(b))
				break;

	if (b == &brushes)
		return;		// no part of this entity selected, don't write it at all

	out << "{\n";
	for (ep = epairs; ep; ep = ep->next)
		out << "\"" << (char*)*ep->key << "\" \"" << (char*)*ep->value << "\"\n";

	CheckOrigin();

	if (!eclass->IsFixedSize())
	{
		count = 0;
		for (b; b != &brushes; b = b->onext)
		{
			if (Select_IsBrushSelected(b))
			{
				out << "// brush " << count << "\n";
				count++;
				b->Write(out);
			}
		}
	}
	out << "}\n";
}
/*
void Entity::WriteSelected (FILE *f)
{
	Brush	*b;
	EPair	*ep;
	int		count;

	for (b = brushes.onext; b != &brushes; b = b->onext)
		if (Select_IsBrushSelected(b))
			break;	// got one

	if (b == &brushes)
		return;		// nothing selected

	CheckOrigin();

	fprintf (f, "{\n");
	for (ep = epairs; ep; ep = ep->next)
		fprintf(f, "\"%s\" \"%s\"\n", (char*)*ep->key, (char*)*ep->value);

	if (!eclass->IsFixedSize())
	{
		count = 0;
		for (b = brushes.onext; b != &brushes; b = b->onext)
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
*/
// <---sikk


/*
============
Entity::SetOrigin

entity origins are stored/modified three different ways (by entity.origin, by 
"origin" keyvalue, and derived from brush bounds) - always set a point entity's
origin via these functions or they won't stay synced and you're a JERK
============
*/
void Entity::SetOrigin(vec3_t org)
{
	if (!eclass->IsFixedSize()) return;

	VectorCopy(org, origin);
	SetKeyValueIVector("origin", org);
	MakeBrush();
}

// call one of these three after updating the odd one out, to not loop
void Entity::SetOriginFromMember()
{
	if (!eclass->IsFixedSize()) return;

	SetKeyValueIVector("origin", origin);
	MakeBrush();
}

void Entity::SetOriginFromKeyvalue()
{
	vec3_t	org;
	if (!eclass->IsFixedSize()) return;

	SetKeyValueIVector("origin", org);
	VectorCopy(org, origin);
	MakeBrush();
}

void Entity::SetOriginFromBrush()
{
	vec3_t	org;

	if (!eclass->IsFixedSize()) return;
	if (brushes.onext == &brushes) return;

	VectorSubtract(brushes.onext->mins, eclass->mins, org);
	SetKeyValueIVector("origin", org);
	VectorCopy(org, origin);
}

/*
============
Entity_MakeBrush

update the dummy brush for point entities after the origin is overridden (easier 
than translating it) or when the classname is changed
============
*/
void Entity::MakeBrush()
{
	Brush		*b;
	vec3_t		emins, emaxs;

	// create a custom brush
	VectorAdd(eclass->mins, origin, emins);
	VectorAdd(eclass->maxs, origin, emaxs);
	if (brushes.onext == &brushes)
	{
		b = Brush::Create(emins, emaxs, &eclass->texdef);
		LinkBrush(b);
	}
	else
	{
		b = brushes.onext;
		b->Recreate(emins, emaxs, &eclass->texdef);
	}

	b->Build();
}

/*
============
Entity::Create

Creates a new entity out of the selected_brushes list.
If the entity class is fixed size, the brushes are only used to find a midpoint. 
Otherwise, the brushes have their ownership transfered to the new entity.
============
*/
Entity *Entity::Create (EntClass *ecIn)
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

		e->ChangeClassname(ecIn);

		return e;
	}

	// check to make sure the brushes are ok
	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
		if (b->owner != g_map.world)
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
	e->SetKeyValue("classname", c->name);

	// add the entity to the entity list
	e->next = g_map.entities.next;
	g_map.entities.next = e;
	e->next->prev = e;
	e->prev = &g_map.entities;

	if (c->IsFixedSize())
	{
		// TODO: pass the location of the right-click via an appropriate method
		VectorCopy(g_brSelectedBrushes.mins, e->origin);
		e->SetKeyValueIVector("origin", e->origin);
		VectorCopy(g_v3VecOrigin, g_brSelectedBrushes.mins);	// reset

		e->MakeBrush();

		Select_HandleBrush(e->brushes.onext,false);
	}
	else
	{
		// change the selected brushes over to the new entity
		for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
		{
			Entity::UnlinkBrush(b);
			e->LinkBrush(b);
			b->Build();	// so the key brush gets a name
		}
	}

	return e;
}

/*
===========
Entity::ChangeClassname
===========
*/
void Entity::ChangeClassname(EntClass* ec)
{
	eclass = ec;
	SetKeyValue("classname", ec->name);

	if (ec->IsFixedSize())
	{
		// make a new brush for the entity
		MakeBrush();
	}
}

void Entity::ChangeClassname(const char *classname)
{
	bool hasbrushes = (brushes.onext != brushes.oprev);
	EntClass* ec = EntClass::ForName(classname, hasbrushes, false);
	ChangeClassname(ec);
}

/*
===========
Entity::LinkBrush
===========
*/
void Entity::LinkBrush (Brush *b)
{
	if (b->oprev || b->onext)
		Error("Entity::LinkBrush: Already linked.");
	b->owner = this;
	b->onext = brushes.onext;
	b->oprev = &brushes;
	brushes.onext->oprev = b;
	brushes.onext = b;
}

/*
===========
Entity::UnlinkBrush
===========
*/
void Entity::UnlinkBrush (Brush *b)
{
	if (!b->owner || !b->onext || !b->oprev)
		Error("Entity::UnlinkBrush: Not currently linked.");
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
Entity *Entity::Clone()
{
	Entity	*n;
	EPair	*ep, *np;

	n = new Entity();
	n->eclass = eclass;

	// add the entity to the entity list
	n->next = g_map.entities.next;
	g_map.entities.next = n;
	n->next->prev = n;
	n->prev = &g_map.entities;

	for (ep = epairs; ep; ep = ep->next)
	{
		np = new EPair(*ep);
		np->next = n->epairs;
		n->epairs = np;
	}
	VectorCopy(origin, n->origin);
	return n;
}



/*
==============
FindEntity
==============
*/
Entity *Entity::Find (char *pszKey, char *pszValue)
{
	Entity *pe;
	
	pe = g_map.entities.next;
	
	for ( ; pe != NULL && pe != &g_map.entities; pe = pe->next)
		if (!strcmp(pe->GetKeyValue(pszKey), pszValue))
			return pe;

	return NULL;
}

/*
==============
FindEntityInt
==============
*/
Entity *Entity::Find(char *pszKey, int iValue)
{
	Entity *pe;
	
	pe = g_map.entities.next;
	
	for ( ; pe != NULL && pe != &g_map.entities; pe = pe->next)
		if (pe->GetKeyValueInt(pszKey) == iValue)
			return pe;

	return NULL;
}

// sikk---> Cut/Copy/Paste
/*
===============
Entity::CleanCopiedList
===============
*/
void Entity::CleanCopiedList ()
{
	Entity	*pe, *next;
	EPair		*ep, *enext;

	pe = g_map.copiedEntities.next;

	while (pe != NULL && pe != &g_map.copiedEntities)
	{
		next = pe->next;
		enext = NULL;
		for (ep = pe->epairs; ep; ep = enext)
		{
			enext = ep->next;
			delete ep;
		}
		free (pe);
		pe = next;
	}
	g_map.copiedEntities.CloseLinks();
//	g_map.copiedEntities.next = g_map.copiedEntities.prev = &g_map.copiedEntities;
}

/*
===============
Entity::Copy
===============
*/
Entity *Entity::Copy ()
{
	Entity	*n;
	EPair		*ep, *np;

	n = new Entity();
	n->eclass = eclass;

	// add the entity to the entity list
	n->next = g_map.copiedEntities.next;
	g_map.copiedEntities.next = n;
	n->next->prev = n;
	n->prev = &g_map.copiedEntities;

	for (ep = epairs; ep; ep = ep->next)
	{
		np = new EPair(*ep);
		np->next = n->epairs;
		n->epairs = np;
	}
	return n;
}
// <---sikk

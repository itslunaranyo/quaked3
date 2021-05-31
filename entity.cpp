//==============================
//	entity.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "map.h"
#include "select.h"
#include "CameraView.h"
#include "CmdCreateBrushEntity.h"
#include "CmdCreatePointEntity.h"
#include "parse.h"
#include "win_dlg.h"
#include <iostream>

Entity::Entity() :
	next(nullptr), prev(nullptr), epairs(nullptr), eclass(nullptr),
	showflags(0), origin(0)
{
	brushes.onext = brushes.oprev = &brushes;
	brushes.owner = this;
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
'flag' is the binary integer (post-exponentiation), not the ordinal slot
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

void Entity::SetSpawnflagFilter()
{
	int		ival, efl;
	const int all_efl = (EFL_EASY | EFL_MEDIUM | EFL_HARD | EFL_DEATHMATCH);
	ival = GetKeyValueInt("spawnflags");

	efl = ival & all_efl;
	//showflags = all_efl - efl;
	showflags = efl;
}

/*
==============
Entity::SetKeyValue
==============
*/
void Entity::SetKeyValue(const char *key, const char *value)
{
	EPair	*ep;
	//int vlen = strlen(value);

	if (!key || !key[0])
		return;

	for (ep = epairs; ep; ep = ep->next)
	{
		if (ep->key == key)
		{
			//if ((int)ep->value.size() <= vlen)
			//	ep->value.resize(vlen + 8);
			ep->SetValue(value);
			break;
		}
	}
	if (!ep)
	{
		ep = new EPair();
		ep->next = epairs;
		epairs = ep;
		//ep->key.resize(strlen(key) + 8);
		//strcpy((char*)*ep->key, key);
		//ep->value.resize(vlen + 8);
		ep->SetKey(key);
		ep->SetValue(value);
	}

	//strcpy((char*)*ep->value, value);

	// this has to go here and not in SetSpawnflag because the user could enter
	// the number directly into the keyvalue instead of ticking the boxes
	if (!strcmp(key, "spawnflags"))
		SetSpawnflagFilter();
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
void Entity::SetKeyValueFVector(const char *key, const vec3 vec)
{
	char szVec[128];

	VecToString(vec, szVec);
	SetKeyValue(key, szVec);
}

/*
==============
Entity::SetKeyValueIVector
==============
*/
void Entity::SetKeyValueIVector(const char *key, const vec3 vec)
{
	char szVec[128];
	sprintf(szVec, "%d %d %d", (int)roundf(vec[0]), (int)roundf(vec[1]), (int)roundf(vec[2]));
	SetKeyValue(key, szVec);
}

/*
==============
Entity::GetEPair
==============
*/
EPair *Entity::GetEPair(const char * key) const
{
	for (EPair *ep = epairs; ep; ep = ep->next)
		if (ep->key == key)
			return ep;

	return nullptr;
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
bool Entity::GetKeyValueVector(const char *key, vec3 &out) const
{
	char *cv = GetKeyValue(key);
	if (!*cv)
		return false;
	sscanf(cv, "%f %f %f", &out[0], &out[1], &out[2]);
	return true;
}

/*
==============
Entity::DeleteKeyValue
==============
*/
void Entity::DeleteKeyValue(const char *key)
{
	EPair	**ep, *next;

	if (!strcmp(key, "origin") && IsPoint())
	{
		Warning("Point entities must have an origin");
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


/*
===============
Entity::IsFiltered
===============
*/
bool Entity::IsFiltered() const
{
	if (g_cfgUI.ViewFilter & (showflags | eclass->showFlags))
		return true;

	return false;
}

/*
===============
Entity::GetCenter

Find visual center of this entity (bmodel bounds center)
===============
*/
vec3 Entity::GetCenter() const
{
	// some entities have wonky origins, ie min corner
	if (IsPoint())
		return origin + (eclass->mins + eclass->maxs) / 2.0f;

	vec3 mins, maxs;
	ClearBounds(mins, maxs);
	for (Brush *b = brushes.onext; b != &brushes; b = b->onext)
	{
		AddPointToBounds(b->mins, mins, maxs);
		AddPointToBounds(b->maxs, mins, maxs);
	}
	return (mins + maxs) / 2.0f;
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
	SetSpawnflagFilter();
}

/*
=================
Entity::AddToList
=================
*/
void Entity::AddToList(Entity *list, bool tail)
{
	assert((!next && !prev) || (next && prev));

	if (next || prev)
		Error("Entity_AddToList: Already linked.");

	if (tail)
	{
		next = list;
		prev = list->prev;
	}
	else
	{
		next = list->next;
		prev = list;
	}
	next->prev = this;
	prev->next = this;
}

/*
=================
Entity::RemoveFromList
=================
*/
void Entity::RemoveFromList()
{
	assert((!next && !prev) || (next && prev));

	if (!next || !prev)
		Error("Entity::RemoveFromList: Not currently linked.");

	next->prev = prev;
	prev->next = next;
	next = prev = nullptr;
}

void Entity::CloseLinks()
{
	assert((!next && !prev) || (next && prev));

	if (next == prev && prev == this)
		return;	// done

	if (next || prev)
		Error("Entity: tried to close non-empty linked list.");

	next = prev = this;
}

/*
=================
Entity::MergeListIntoList
=================
*/
void Entity::MergeListIntoList(Entity *dest, bool tail)
{
	// properly doubly-linked lists only
	if (!next || !prev)
	{
		Error("Tried to merge a list with NULL links!");
		return;
	}

	if (next == this || prev == this)
	{
		Warning("Tried to merge an empty list.");
		return;
	}
	if (tail)
	{
		// merge at tail of list
		dest->prev->next = next;
		next->prev = dest->prev;
		dest->prev = prev;
		prev->next = dest;
	}
	else
	{
		// merge at head of list
		next->prev = dest;
		prev->next = dest->next;
		dest->next->prev = prev;
		dest->next = next;
	}

	prev = next = this;
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
EPair::ParseEpair
=================
*/
EPair *EPair::ParseEpair()
{
	EPair	*e;
	
	e = new EPair();
	e->SetKey(g_szToken);
	GetToken(false);
	e->SetValue(g_szToken);
	return e;
}

/*
=================
EPair::IsTarget

check for all the ways an entity can have an outgoing target reference
(target/target2/3/4/killtarget/whatever silly ones arcadim added)
=================
*/
bool EPair::IsTarget()
{
	char *c;
	c = (char*)*key;

	// in non-extended mode, only 'target' is valid
	if (!g_project.extTargets)
		return (strcmp(c, "target") == 0);

	// in extended mode, any kv with 'target' (but not 'targetname') in it is a valid target source
	while (*c)
	{
		if (*c == 't')
		{
			if (!strncmp(c, "target", 6))
			{
				if (!strncmp(c + 6, "name", 4))
					return false;
				return true;
			}
		}
		++c;
	}
	return false;
}

/*
=================
EPair::IsTargetName

check for all the ways an entity can have an incoming target reference,
which so far thankfully all start with 'targetname'
=================
*/
bool EPair::IsTargetName()
{
	char *c;
	c = (char*)*key;
	if (!strncmp(c, "target", 6))
	{
		if (!strncmp(c + 6, "name", 4))
		{
			// in extended mode, any 'targetname' w/suffix is a valid target destination
			if (g_project.extTargets || (*(c + 10) == 0))
				return true;
			return false;
		}
	}
	return false;
}

/*
================
EPair::SetKey

maintain safe buffer padding
================
*/
void EPair::SetKey(const char *k)
{
	assert(*k);
	int klen = strlen(k);
	if ((int)key.size() <= klen)
		key.resize(klen + 8);
	strcpy(key.c_str(), k);
}

/*
================
EPair::SetValue

maintain safe buffer padding
================
*/
void EPair::SetValue(const char *v)
{
	int vlen = strlen(v);
	if ((int)value.size() <= vlen)
		value.resize(vlen + 8);
	strcpy(value.c_str(), v);
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
			ep = EPair::ParseEpair();
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
	ent->SetSpawnflagFilter();

	// lunaran - this now creates fixed/non-fixed entclasses on the fly for point entities
	// with brushes or brush entities without any, so that all the downstream code Just Works
	e = EntClass::ForName(ent->GetKeyValue("classname"), has_brushes, false);
	ent->eclass = e;

	if (e->IsPointClass())
	{	// create a custom brush
		ent->MakeBrush();
	}

	// lunaran - entities do not add their brushes to a list by default now, map::load/save/etc does it
	// this is to keep loads/pastes/etc isolated until the parse is complete for exception guarantee
	return ent;
}

/*
================
Entity::CheckOrigin

if fixedsize, make sure origins are consistent with each other
calculate a new origin based on the current brush position and warn if not
================
*/
void Entity::CheckOrigin()
{
#ifdef _DEBUG
	if (IsBrush()) return;

	vec3 testorg, org;
	GetKeyValueVector("origin", testorg);

	// be sure for now
	if (!VectorCompare(origin, testorg))
	{
		SetOriginFromBrush();
		Warning("Entity origins out of sync on %s at (%f %f %f)", eclass->name, org[0], org[1], org[2]);
	}
#endif
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
			vec3 cOrg = g_vCamera.GetOrigin();
			out << "{\n";
			out << "\"classname\" \"info_player_start\"\n";
			out << "\"origin\" \"" << (int)cOrg.x << " " <<
									(int)cOrg.y << " " <<
									(int)cOrg.z << "\"\n";
			out << "\"angle\" \"" << (int)g_vCamera.GetAngles()[YAW] << "\"\n";
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

	if (IsBrush())
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

// sikk---> Export Selection (Map/Prefab)
/*
=================
Entity::WriteSelected
=================
*/
void Entity::WriteSelected(std::ostream &out, int n)
{
	Brush	*b;
	EPair	*ep;
	int		count;

	// a .map with no worldspawn is broken, so always write worldspawn even if it's empty
	if (eclass == EntClass::worldspawn)
		b = brushes.onext;
	else
		for (b = brushes.onext; b != &brushes; b = b->onext)
			if (Selection::IsBrushSelected(b))
				break;

	if (b == &brushes)
		return;		// no part of this entity selected, don't write it at all

	out << "// entity " << n << "\n";
	out << "{\n";
	for (ep = epairs; ep; ep = ep->next)
		out << "\"" << (char*)*ep->key << "\" \"" << (char*)*ep->value << "\"\n";

	CheckOrigin();

	if (IsBrush())
	{
		count = 0;
		for (b; b != &brushes; b = b->onext)
		{
			if (Selection::IsBrushSelected(b))
			{
				out << "// brush " << count << "\n";
				count++;
				b->Write(out);
			}
		}
	}
	out << "}\n";
}
// <---sikk


/*
============
Entity::SetOrigin

entity origins are stored/modified three different ways (by entity.origin, by 
"origin" keyvalue, and derived from brush bounds) - always set a point entity's
origin via these functions or they won't stay synced and you're a JERK
============
*/
void Entity::SetOrigin(const vec3 org)
{
	if (IsBrush()) return;

	origin = org;
	SetKeyValueIVector("origin", org);
	MakeBrush();
}

// call one of these three after updating the odd one out, to not loop
void Entity::SetOriginFromMember()
{
	if (IsBrush()) return;

	origin[0] = floorf(origin[0] + 0.5f);
	origin[1] = floorf(origin[1] + 0.5f);
	origin[2] = floorf(origin[2] + 0.5f);

	SetKeyValueIVector("origin", origin);
	MakeBrush();
}

void Entity::SetOriginFromKeyvalue()
{
	if (IsBrush()) return;

	GetKeyValueVector("origin", origin);
	MakeBrush();
}

void Entity::SetOriginFromBrush()
{
	vec3	org;

	if (IsBrush()) return;
	if (brushes.onext == &brushes) return;

	org = brushes.onext->mins - eclass->mins;
	SetKeyValueIVector("origin", org);
	origin = org;
}

void Entity::Move(const vec3 trans)
{
	origin = trans + origin;
	SetOriginFromMember();
}

void Entity::Transform(mat4 mat)
{
	origin = mat * glm::vec4(origin, 1);
	origin = glm::round(origin);
	SetOriginFromMember();
}

/*
============
Entity_MakeBrush

update the dummy brush for point entities after the origin is overridden (easier 
than translating it) or when the classname is changed
============
*/
Brush *Entity::MakeBrush()
{
	Brush		*b;
	vec3		emins, emaxs;

	// create a custom brush
	emins = eclass->mins + origin;
	emaxs = eclass->maxs + origin;
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
	return b;
}

/*
============
Entity::Create

Creates a new entity out of the selected_brushes list.
If the entity class is fixed size, the brushes are only used to find a midpoint. 
Otherwise, the brushes have their ownership transfered to the new entity.
============
*/
bool Entity::Create (EntClass *ecIn)
{
	Entity		*e;

	if (g_brSelectedBrushes.next != &g_brSelectedBrushes &&		// brushes are selected
		g_brSelectedBrushes.next == g_brSelectedBrushes.prev &&	// one brush is selected
		g_brSelectedBrushes.next->owner->IsPoint())		// it is a fixedsize brush
	{
		e = g_brSelectedBrushes.next->owner;
		e->ChangeClassname(ecIn);
		WndMain_UpdateWindows(W_SCENE);
		Selection::Changed();
		return true;
	}

	if (Selection::HasBrushes() == ecIn->IsPointClass())	// x0r!
	{
		// check with user that we want to apply the 'wrong' eclass
		if (!ConfirmClassnameHack(ecIn))
			return false;
	}

	if (Selection::HasBrushes())
	{
		if (Selection::OneBrushEntity())
		{
			g_brSelectedBrushes.next->owner->ChangeClassname(ecIn);
			WndMain_UpdateWindows(W_SCENE);
			Selection::Changed();
			return true;
		}

		CmdCreateBrushEntity *cmd;
		try
		{
			cmd = new CmdCreateBrushEntity(ecIn->name);
			cmd->AddBrushes(&g_brSelectedBrushes);
		}
		catch (qe3_cmd_exception &ex)
		{
			ReportError(ex);
			delete cmd;
			return false;
		}
		g_cmdQueue.Complete(cmd);
	}
	else
	{
		// TODO: pass the location of the right-click via an appropriate method
		CmdCreatePointEntity *cmd = new CmdCreatePointEntity(ecIn->name, g_brSelectedBrushes.mins);
		g_cmdQueue.Complete(cmd);
		g_brSelectedBrushes.mins = vec3(0);	// reset
	}
	return true;
}

/*
===========
Entity::ChangeClassname
===========
*/
void Entity::ChangeClassname(EntClass* ec)
{
	assert(ec != EntClass::worldspawn);
	eclass = ec;
	SetKeyValue("classname", ec->name);

	if (ec->IsPointClass())
	{
		// make a new brush for the entity
		MakeBrush();
	}
}

void Entity::ChangeClassname(const char *classname)
{
	// lunaran TODO: jesus christ there has to be a better way
	bool hasbrushes = (brushes.onext->faces->texdef.tex->name[0] != '#');

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
	b->onext = b->oprev = nullptr;
	b->owner = nullptr;
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

	for (ep = epairs; ep; ep = ep->next)
	{
		np = new EPair(*ep);
		np->next = n->epairs;
		n->epairs = np;
	}
	n->showflags = showflags;
	n->origin = origin;
	//	n->CloseLinks();
	return n;
}

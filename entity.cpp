//==============================
//	entity.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "cfgvars.h"
#include "strlib.h"
#include "StringFormatter.h"
//#include "map.h"
#include "select.h"
//#include "CameraView.h"
#include "CmdCreateBrushEntity.h"
#include "CmdCreatePointEntity.h"
//#include "parse.h"
#include "win_dlg.h"
//#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

Entity::Entity() :
	next(this), prev(this), epairs(nullptr), eclass(nullptr),
	showflags(0)
{
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
	while (brushes.ENext() != &brushes)
		delete brushes.ENext();

	if (next != this)
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

bool Entity::IsPoint() const { return (eclass->IsPointClass()); }
bool Entity::IsBrush() const { return !(eclass->IsPointClass()); }
bool Entity::IsWorld() const { return (eclass == EntClass::Worldspawn()); }

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
void Entity::SetKeyValue(const std::string& key, const std::string& value)
{
	EPair	*ep;
	//int vlen = strlen(value);

	if (key.empty())
		return;

	for (ep = epairs; ep; ep = ep->next)
	{
		if (ep->GetKey() == key)
		{
			ep->SetValue(value);
			break;
		}
	}
	if (!ep)
	{
		ep = new EPair();
		ep->next = epairs;
		epairs = ep;
		ep->SetKey(key);
		ep->SetValue(value);
	}

	// this has to go here and not in SetSpawnflag because the user could enter
	// the number directly into the keyvalue instead of ticking the boxes
	if (key == "spawnflags")
		SetSpawnflagFilter();
}

/*
==============
Entity::SetKeyValue
==============
*/
void Entity::SetKeyValue(const std::string& key, const float fvalue)
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
void Entity::SetKeyValue(const std::string& key, const int ivalue)
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
void Entity::SetKeyValueFVector(const std::string& key, const vec3 vec)
{
	SetKeyValue(key, strlib::VecToString(vec));
}

/*
==============
Entity::SetKeyValueIVector
==============
*/
void Entity::SetKeyValueIVector(const std::string& key, const vec3 vec)
{
	vec3 iv = glm::round(vec);
	SetKeyValue(key, strlib::VecToStringNice(iv, 0));
}

/*
==============
Entity::GetEPair
==============
*/
EPair *Entity::GetEPair(const std::string& key) const
{
	for (EPair *ep = epairs; ep; ep = ep->next)
		if (ep->GetKey() == key)
			return ep;

	return nullptr;
}


/*
==============
Entity::GetKeyValue
==============
*/
std::string Entity::GetKeyValue(const std::string& key) const
{
	EPair	*ep;

	for (ep = epairs; ep; ep = ep->next)
		if (ep->GetKey() == key)
			return ep->GetValue();
	return "";
}

/*
==============
Entity::GetKeyValueFloat
==============
*/
float Entity::GetKeyValueFloat(const std::string& key) const
{
	auto cv = GetKeyValue(key);
	if (cv.empty()) return 0.0;
	return std::stof(cv);
}

/*
==============
Entity::GetKeyValueInt
==============
*/
int Entity::GetKeyValueInt(const std::string& key) const
{
	auto cv = GetKeyValue(key);
	if (cv.empty()) return 0;
	return std::stoi(cv);
}

/*
==============
Entity::GetKeyValueVector
==============
*/
bool Entity::GetKeyValueVector(const std::string& key, vec3 &out) const
{
	auto cv = GetKeyValue(key);
	return strlib::StringToVec(cv, out);
}

/*
==============
Entity::DeleteKeyValue
==============
*/
void Entity::DeleteKeyValue(const std::string& key)
{
	EPair	**ep, *next;

	if ((key == "origin") && IsPoint())
	{
		Log::Warning("Point entities must have an origin");
		Sys_Beep();
		return;
	}
	ep = &epairs;
	while (*ep)
	{
		next = *ep;
		if (next->GetKey() == key)
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
		return brushes.ENext()->mins + (eclass->maxs - eclass->mins) / 2.0f;

	vec3 mins, maxs;
	ClearBounds(mins, maxs);
	for (Brush *b = brushes.ENext(); b != &brushes; b = b->ENext())
	{
		AddPointToBounds(b->mins, mins, maxs);
		AddPointToBounds(b->maxs, mins, maxs);
	}
	return (mins + maxs) / 2.0f;
}


//===================================================================


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
Entity::IsLinked
=================
*/
bool Entity::IsLinked() const
{
	assert((next == this) == (prev == this));
	return (next != this);
}

/*
=================
Entity::AddToList
=================
*/
void Entity::AddToList(Entity *list, bool tail)
{
	assert((next == this) == (prev == this));

	if (next != this)
		Error("Entity::AddToList: Already linked.\n");

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
	assert((next == this) == (prev == this));

	if (next == this)
		Error("Entity::RemoveFromList: Not currently linked.\n");

	next->prev = prev;
	prev->next = next;
	next = prev = this;
}

/*
=================
Entity::MergeListIntoList
=================
*/
void Entity::MergeListIntoList(Entity *dest, bool tail)
{
	assert((next == this) == (prev == this));
	if (next == this || prev == this)
	{
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
============
Entity::GetOrigin
============
*/
vec3 Entity::GetOrigin() const
{
	vec3 org(0);
	if (GetKeyValueVector("origin", org))
		return org;
	if (IsPoint() && brushes.ENext() != &brushes)
		return brushes.ENext()->mins - eclass->mins;
	return org;
}

/*
============
Entity::SetOrigin

entity origins are stored/modified by "origin" keyvalue or derived from 
brush bounds - always set origin via these methods or they won't stay synced
============
*/
void Entity::SetOrigin(const vec3 org)
{
	if (IsBrush()) return;

	vec3 oldorg = GetOrigin();
	SetKeyValueIVector("origin", org);
	
	// don't need to remake the brush, which just throws away six faces and then 
	//	reallocates them anyway
	brushes.ENext()->Transform(glm::translate(mat4(1), org - oldorg), false);
}

void Entity::SetOriginFromKeyvalue()
{
	if (IsBrush()) return;

	// makeBrush calls getOrigin which gets the keyvalue itself
	MakeBrush();
}

void Entity::SetOriginFromBrush()
{
	vec3	org;

	if (IsBrush()) return;
	if (brushes.ENext() == &brushes) return;

	SetKeyValueIVector("origin", GetOrigin());
}

void Entity::Transform(mat4 mat)
{
	vec3 origin;
	origin = mat * glm::vec4(GetOrigin(), 1);
	origin = glm::round(origin);
	SetOrigin(origin);
}

/*
============
Entity_MakeBrush

update the dummy brush for point entities when the classname is changed
============
*/
Brush *Entity::MakeBrush()
{
	Brush		*b;
	vec3		emins, emaxs, org;

	// create a custom brush
	org = GetOrigin();
	emins = eclass->mins + org;
	emaxs = eclass->maxs + org;
	if (brushes.ENext() == &brushes)
	{
		b = Brush::Create(emins, emaxs, &eclass->texdef);
		LinkBrush(b);
	}
	else
	{
		b = brushes.ENext();
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

	if (g_brSelectedBrushes.Next() != &g_brSelectedBrushes &&		// brushes are selected
		g_brSelectedBrushes.Next() == g_brSelectedBrushes.Prev() &&	// one brush is selected
		g_brSelectedBrushes.Next()->owner->IsPoint())		// it is a fixedsize brush
	{
		e = g_brSelectedBrushes.Next()->owner;
		e->ChangeClass(ecIn);
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
			g_brSelectedBrushes.Next()->owner->ChangeClass(ecIn);
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
		catch (qe3_cmd_exception)
		{
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
void Entity::ChangeClass(EntClass* ec)
{
	assert(ec != EntClass::Worldspawn());
	eclass = ec;
	SetKeyValue("classname", ec->name);

	if (ec->IsPointClass())
	{
		// make a new brush for the entity
		MakeBrush();
	}
}

void Entity::ChangeClassname(const std::string& classname)
{
	// lunaran TODO: jesus christ there has to be a better way
	bool hasbrushes = (brushes.ENext()->faces->texdef.tex->name[0] != '#');

	EntClass* ec = EntClass::ForName(classname, hasbrushes, false);
	ChangeClass(ec);
}

/*
===========
Entity::LinkBrush
Entity::UnlinkBrush
===========
*/
void _brush_ent_accessor::LinkToEntity(Brush* b, Entity* e)
{
	b->owner = e;
	b->onext = e->brushes.onext;
	b->oprev = &e->brushes;
	e->brushes.onext->oprev = b;
	e->brushes.onext = b;
}
void _brush_ent_accessor::LinkToEntityTail(Brush* b, Entity* e)
{
	b->owner = e;
	b->onext = &e->brushes;
	b->oprev = e->brushes.oprev;
	e->brushes.oprev->onext = b;
	e->brushes.oprev = b;
}

void _brush_ent_accessor::UnlinkFromEntity(Brush* b, bool preserveOwner)
{
	b->onext->oprev = b->oprev;
	b->oprev->onext = b->onext;
	b->onext = b->oprev = b;
	if (preserveOwner) return;
	b->owner = nullptr;
}

void Entity::LinkBrush(Brush *b, bool tail)
{
	if (b->ENext() != b || b->EPrev() != b)
		Error("Entity::LinkBrush: Already linked.");
	if (tail)
		_brush_ent_accessor::LinkToEntityTail(b, this);
	else
		_brush_ent_accessor::LinkToEntity(b, this);
}

void Entity::UnlinkBrush(Brush *b, bool preserveOwner)
{
	if (!b->owner || b->ENext() == b || b->EPrev() == b)
		Error("Entity::UnlinkBrush: Not currently linked.");
	_brush_ent_accessor::UnlinkFromEntity(b, preserveOwner);
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
	//n->origin = origin;
	return n;
}

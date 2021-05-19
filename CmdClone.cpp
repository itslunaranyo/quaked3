//==============================
//	CmdClone.cpp
//==============================

#include "qe3.h"

CmdClone::CmdClone(Brush *brList, const vec3 offset)
{
	Clone(brList, offset);
	state = LIVE;
}

CmdClone::~CmdClone()
{
}

void CmdClone::Clone(Brush *brList, const vec3 offset)
{
	vec3	delta;
	Brush	*n;
	Entity	*e, *laste;

	delta = offset;
	/*
	if (offset)
	{
		delta = offset;
	}
	else
	{
		delta = vec3(0);
	}*/

	laste = nullptr;

	for (Brush *b = brList->next; b != brList; b = b->next)
	{
		// lunaran TODO: "copy within entity" parameter
		if (b->owner != laste)		// selections are sorted by entity
		{
			if (b->owner->IsWorld())	// don't clone the worldspawn
				e = b->owner;
			else
			{
				e = b->owner->Clone();
				cmdAR.AddedEntity(e);
			}
			laste = b->owner;
		}

		// lunaran TODO: "clear target/targetname on clone" parameter
		// DeleteKey(e, "target");
		// DeleteKey(e, "targetname");

		if (e->IsPoint())
		{
			e->Move(delta);
		}
		else
		{
			n = b->Clone();
			n->Build();		// must have geo before the Move() for texture lock
			n->owner = e;
			n->Move(delta);
			cmdAR.AddedBrush(n);
		}
	}
}

void CmdClone::Do_Impl() { cmdAR.Do(); }
void CmdClone::Undo_Impl() { cmdAR.Undo(); }
void CmdClone::Redo_Impl() { cmdAR.Redo(); }

void CmdClone::Select_Impl()
{
	if (state == DONE)
		cmdAR.Select();
}

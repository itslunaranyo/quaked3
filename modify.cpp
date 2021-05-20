//==============================
//	modify.cpp
//==============================

#include "qe3.h"

#include "CmdDelete.h"
#include "CmdReparentBrush.h"
#include "CmdClone.h"
#include "CmdSetKeyvalue.h"
#include "CmdCylinder.h"
#include "CmdCZGCylinder.h"
#include "CmdCone.h"
#include "CmdSphere.h"



/*
===============
Modify::Delete
===============
*/
void Modify::Delete()
{
	if (!Selection::HasBrushes())
		return;

	Selection::DeselectAllFaces();
	
	g_qeglobals.d_selSelectMode = sel_brush;

	CmdDelete *cmd = new CmdDelete(&g_brSelectedBrushes);
	g_cmdQueue.Complete(cmd);
}

/*
================
Modify::Clone

Creates an exact duplicate of the selection in place, then moves
the selected brushes off of their old positions
================
*/
void Modify::Clone()
{
	if (g_cfgEditor.CloneStyle == CLONE_DRAG)
		return;
	vec3	delta(g_qeglobals.d_nGridSize);

	if (g_qeglobals.d_selSelectMode != sel_brush) return;
	if (!Selection::HasBrushes()) return;

	// move cloned brushes based on active XY view
	delta[g_qeglobals.d_wndGrid[0]->xyzv->GetAxis()] = 0;

	CmdClone *cmd;
	if (g_cfgEditor.CloneStyle == CLONE_OFFSET)
		cmd = new CmdClone(&g_brSelectedBrushes, delta);
	else //if (g_cfgEditor.CloneStyle == CLONE_INPLACE)
		cmd = new CmdClone(&g_brSelectedBrushes);
	Selection::DeselectAll();
	g_cmdQueue.Complete(cmd);
}



/*
================================================================================

GROUP SELECTIONS

================================================================================
*/


/*
=================
Modify::Ungroup

Turn the currently selected entity back into normal brushes
=================
*/
void Modify::Ungroup()
{
	try
	{
		CmdReparentBrush *cmd = new CmdReparentBrush();
		cmd->Destination(g_map.world);
		for (Brush *br = g_brSelectedBrushes.next; br != &g_brSelectedBrushes; br = br->next)
		{
			cmd->AddBrush(br);
		}
		g_cmdQueue.Complete(cmd);
	}
	catch (...)
	{
		return;
	}
	Sys_UpdateWindows(W_SCENE);
}

// sikk---> Insert Brush into Entity
/*
===============
Modify::InsertBrush
===============
*/
void Modify::InsertBrush()
{
	Brush *br;
	Entity *dest;

	dest = nullptr;
	for (br = g_brSelectedBrushes.next; br != &g_brSelectedBrushes; br = br->next)
	{
		if (br->owner->IsWorld() || br->owner->IsPoint())
			continue;
		dest = br->owner;
		break;
	}

	if (!dest)
	{
		Sys_Printf("WARNING: No brush entity selected to add brushes to.\n");
		return;
	}

	try
	{
		CmdReparentBrush *cmd = new CmdReparentBrush();
		cmd->Destination(dest);
		for (br = g_brSelectedBrushes.next; br != &g_brSelectedBrushes; br = br->next)
		{
			cmd->AddBrush(br);
		}
		g_cmdQueue.Complete(cmd);
	}
	catch (...)
	{
		return;
	}
	Sys_UpdateWindows(W_SCENE);
}
// <---sikk


//=========================================================================

/*
===============
Modify::HideSelected
===============
*/
void Modify::HideSelected()
{
	Brush *b;

	for (b = g_brSelectedBrushes.next; b && b != &g_brSelectedBrushes; b = b->next)
	{
		b->showFlags |= BFL_HIDDEN;
	}

	Selection::Changed();
}

/*
===============
Modify::HideUnselected
===============
*/
void Modify::HideUnselected()
{
	Brush *b;

	for (b = g_map.brActive.next; b && b != &g_map.brActive; b = b->next)
	{
		b->showFlags |= BFL_HIDDEN;
	}
}


/*
===============
Modify::ShowHidden
===============
*/
void Modify::ShowHidden()
{
	Brush *b;

	for (b = g_brSelectedBrushes.next; b && b != &g_brSelectedBrushes; b = b->next)
		b->showFlags -= BFL_HIDDEN & b->showFlags;

	for (b = g_map.brActive.next; b && b != &g_map.brActive; b = b->next)
		b->showFlags -= BFL_HIDDEN & b->showFlags;
	
	Sys_UpdateWindows(W_SCENE);
}


//=========================================================================


/*
===============
Modify::ConnectEntities

Sets target/targetname on the first two entities selected in order
lunaran TODO: confirmation box if target & targetname already clash before overwriting
===============
*/
void Modify::ConnectEntities()
{
	Entity	*e1, *e2, *e;
	char		*target, *tn;
	Brush		*b;
	char		newtarg[32];

	b = g_brSelectedBrushes.prev;
	e1 = b->owner;
	e2 = e1;
	while (e2 == b->owner)
	{
		if (b->prev == &g_brSelectedBrushes)
		{
			Sys_Printf("WARNING: Must have two entities selected.\n");
			Sys_Beep();
			return;
		}
		b = b->prev;
	}
	e2 = b->owner;

	if (e1->IsWorld() || e2->IsWorld())
	{
		Sys_Printf("WARNING: Cannot connect to the world.\n");
		Sys_Beep();
		return;
	}

	target = e1->GetKeyValue("target");
	if (target && target[0])
		strcpy(newtarg, target);
	else
	{
		target = e2->GetKeyValue("targetname");
		if (target && target[0])
			strcpy(newtarg, target);
		else
		{
			int maxtarg, targetnum;
			// make a unique target value
			maxtarg = 0;
			for (e = g_map.entities.next; e != &g_map.entities; e = e->next)
			{
				tn = e->GetKeyValue("targetname");
				if (tn && tn[0])
				{
					targetnum = atoi(tn + 1);
					if (targetnum > maxtarg)
						maxtarg = targetnum;
				}
			}
			sprintf(newtarg, "t%d", maxtarg + 1);
		}
	}

	e1->SetKeyValue("target", newtarg);
	e2->SetKeyValue("targetname", newtarg);

	Sys_Printf("Entities connected as '%s'.\n", newtarg);
	Sys_UpdateWindows(W_XY | W_CAMERA);

	Selection::DeselectAll();
	Selection::HandleBrush(b, true);
}


/*
===============
Modify::SetKeyValue
===============
*/
void Modify::SetKeyValue(const char *key, const char *value)
{
	Entity *last;

	last = nullptr;
	try
	{
		CmdSetKeyvalue *cmd = new CmdSetKeyvalue(key, value);
		for (Brush *b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
		{
			// skip entity brushes in sequence
			if (b->owner == last)
				continue;
			last = b->owner;
			cmd->AddEntity(last);
		}
		g_cmdQueue.Complete(cmd);
	}
	catch (...)
	{
		return;
	}

	Sys_UpdateWindows(W_SCENE | W_ENTITY);
}

/*
===============
Modify::SetColor
===============
*/
void Modify::SetColor(const vec3 color)
{
	char szColor[128];

	VecToString(color, szColor);
	Modify::SetKeyValue("_color", szColor);
}


//=========================================================================


/*
=============
Modify::MakeCzgCylinder

Makes the current brush into a czg-pattern cylinder because czg is the best
=============
*/
void Modify::MakeCzgCylinder(int degree)
{
	int axis;
	Brush* b;

	if (!QE_SingleBrush())
	{
		Sys_Printf("WARNING: Must have a single brush selected.\n");
		return;
	}

	// lunaran - grid view reunification
	axis = QE_BestViewAxis();

	b = g_brSelectedBrushes.next;
	CmdCzgCylinder *cmdCzgC = new CmdCzgCylinder();

	cmdCzgC->SetAxis(axis);
	cmdCzgC->SetDegree(degree);
	cmdCzgC->UseBrush(b);

	g_cmdQueue.Complete(cmdCzgC);

	Sys_UpdateWindows(W_SCENE);
}


/*
=============
Modify::MakeSided

Makes the current brush have the given number of 2d sides
=============
*/
void Modify::MakeSided(int sides)
{
	int axis;
	Brush* b;

	if (!QE_SingleBrush())
	{
		Sys_Printf("WARNING: Must have a single brush selected.\n");
		return;
	}

	// lunaran - grid view reunification
	axis = QE_BestViewAxis();

	b = g_brSelectedBrushes.next;
	CmdCylinder	*cmdCyl = new CmdCylinder();

	cmdCyl->SetAxis(axis);
	cmdCyl->SetSides(sides);
	cmdCyl->UseBrush(b);

	g_cmdQueue.Complete(cmdCyl);

	Sys_UpdateWindows(W_SCENE);
}


// sikk---> Brush Primitives
/*
=============
Modify::MakeSidedCone

Makes the current brush have the given number of 2D sides and turns it into a cone
=============
*/
void Modify::MakeSidedCone(int sides)
{
	int axis;
	Brush* b;

	if (!QE_SingleBrush())
	{
		Sys_Printf("WARNING: Must have a single brush selected.\n");
		return;
	}

	// lunaran - grid view reunification
	axis = QE_BestViewAxis();

	b = g_brSelectedBrushes.next;
	CmdCone	*cmdCone = new CmdCone();

	cmdCone->SetAxis(axis);
	cmdCone->SetSides(sides);
	cmdCone->UseBrush(b);

	g_cmdQueue.Complete(cmdCone);

	Sys_UpdateWindows(W_SCENE);
}

/*
=============
Modify::MakeSidedSphere

Makes the current brush have the given number of 2d sides and turns it into a sphere
=============
*/
void Modify::MakeSidedSphere(int sides)
{
	Brush* b;

	if (!QE_SingleBrush())
	{
		Sys_Printf("WARNING: Must have a single brush selected.\n");
		return;
	}

	b = g_brSelectedBrushes.next;
	CmdSphere *cmdSph = new CmdSphere();

	cmdSph->SetSides(sides);
	cmdSph->UseBrush(b);

	g_cmdQueue.Complete(cmdSph);

	Sys_UpdateWindows(W_SCENE);

}
// <---sikk


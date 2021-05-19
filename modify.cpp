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
Modify_Delete
===============
*/
void Modify_Delete()
{
	if (!Selection::HasBrushes())
		return;

	Selection::DeselectAllFaces();
	
	g_qeglobals.d_selSelectMode = sel_brush;
	g_qeglobals.d_nNumMovePoints = 0;

	CmdDelete *cmd = new CmdDelete(&g_brSelectedBrushes);
	g_cmdQueue.Complete(cmd);
}

/*
================
Modify_Clone

Creates an exact duplicate of the selection in place, then moves
the selected brushes off of their old positions
================
*/
void Modify_Clone()
{
	vec3	delta(g_qeglobals.d_nGridSize);

	if (g_qeglobals.d_selSelectMode != sel_brush) return;
	if (!Selection::HasBrushes()) return;

	// move cloned brushes based on active XY view
	delta[g_qeglobals.d_wndGrid[0]->xyzv->GetAxis()] = 0;

	CmdClone *cmd = new CmdClone(&g_brSelectedBrushes, delta);
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
Modify_Ungroup

Turn the currently selected entity back into normal brushes
=================
*/
void Modify_Ungroup()
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
Modify_InsertBrush
===============
*/
void Modify_InsertBrush()
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
Modify_HideSelected
===============
*/
void Modify_HideSelected()
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
Modify_HideUnselected
===============
*/
void Modify_HideUnselected()
{
	Brush *b;

	for (b = g_map.brActive.next; b && b != &g_map.brActive; b = b->next)
	{
		b->showFlags |= BFL_HIDDEN;
	}
}


/*
===============
Modify_ShowHidden
===============
*/
void Modify_ShowHidden()
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
Modify_ConnectEntities

Sets target/targetname on the first two entities selected in order
lunaran TODO: confirmation box if target & targetname already clash before overwriting
===============
*/
void Modify_ConnectEntities()
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
Modify_SetKeyValue
===============
*/
void Modify_SetKeyValue(const char *key, const char *value)
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
Modify_SetColor
===============
*/
void Modify_SetColor(const vec3 color)
{
	char szColor[128];

	VecToString(color, szColor);
	Modify_SetKeyValue("_color", szColor);
}


//=========================================================================


/*
=============
Modify_MakeCzgCylinder

Makes the current brush into a czg-pattern cylinder because czg is the best
=============
*/
void Modify_MakeCzgCylinder(int degree)
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
Modify_MakeSided

Makes the current brush have the given number of 2d sides
=============
*/
void Modify_MakeSided(int sides)
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
Modify_MakeSidedCone

Makes the current brush have the given number of 2D sides and turns it into a cone
=============
*/
void Modify_MakeSidedCone(int sides)
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
Modify_MakeSidedSphere

Makes the current brush have the given number of 2d sides and turns it into a sphere
=============
*/
void Modify_MakeSidedSphere(int sides)
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


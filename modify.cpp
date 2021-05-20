//==============================
//	modify.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "map.h"
#include "select.h"
#include "modify.h"
#include "WndGrid.h"
#include "XYZView.h"

#include "CmdDelete.h"
#include "CmdReparentBrush.h"
#include "CmdClone.h"
#include "CmdSetKeyvalue.h"
#include "CmdCylinder.h"
#include "CmdCZGCylinder.h"
#include "CmdCone.h"
#include "CmdSphere.h"
#include "CmdCompound.h"


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
	
	Selection::g_selMode = sel_brush;

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

	if (Selection::g_selMode != sel_brush) return;
	if (!Selection::HasBrushes()) return;

	// move cloned brushes based on active XY view
	delta[g_wndGrid[0]->xyzv->GetAxis()] = 0;

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
	CmdReparentBrush *cmd;
	try
	{
		cmd = new CmdReparentBrush();
		cmd->Destination(g_map.world);
		for (Brush *br = g_brSelectedBrushes.next; br != &g_brSelectedBrushes; br = br->next)
		{
			cmd->AddBrush(br);
		}
	}
	catch (qe3_exception &ex)
	{
		ReportError(ex);
		return;
	}
	g_cmdQueue.Complete(cmd);
	WndMain_UpdateWindows(W_SCENE);
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
		Warning("No brush entity selected to add brushes to.");
		return;
	}
	
	CmdReparentBrush *cmd;
	try
	{
		cmd = new CmdReparentBrush();
		cmd->Destination(dest);
		for (br = g_brSelectedBrushes.next; br != &g_brSelectedBrushes; br = br->next)
		{
			cmd->AddBrush(br);
		}
	}
	catch (qe3_exception &ex)
	{
		ReportError(ex);
		return;
	}

	g_cmdQueue.Complete(cmd);
	WndMain_UpdateWindows(W_SCENE);
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

	WndMain_UpdateWindows(W_SCENE | W_TARGETGRAPH);
	Selection::Changed();
}

/*
===============
Modify::HideUnselected
===============
*/
void Modify::HideUnselected()
{
	if (Selection::IsEmpty())
		return;

	Brush *b;

	for (b = g_map.brActive.next; b && b != &g_map.brActive; b = b->next)
	{
		b->showFlags |= BFL_HIDDEN;
	}

	WndMain_UpdateWindows(W_SCENE | W_TARGETGRAPH);
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
	
	WndMain_UpdateWindows(W_SCENE | W_TARGETGRAPH);
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
			Warning("Must have two entities selected.");
			Sys_Beep();
			return;
		}
		b = b->prev;
	}
	e2 = b->owner;

	if (e1->IsWorld() || e2->IsWorld())
	{
		Warning("Cannot connect to the world.");
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

	// use a compound cmd, because we need to set two different keyvalues but
	// they'll be expected to undo as one operation
	CmdCompound *cmdc = new CmdCompound("Connect Entities");
	CmdSetKeyvalue *cmdKV1, *cmdKV2;

	cmdKV1 = new CmdSetKeyvalue("target", newtarg);
	cmdKV1->AddEntity(e1);
	cmdKV2 = new CmdSetKeyvalue("targetname", newtarg);
	cmdKV2->AddEntity(e2);
	cmdc->Complete(cmdKV1);
	cmdc->Complete(cmdKV2);

	g_cmdQueue.Complete(cmdc);

	Sys_Printf("Entities connected as '%s'.\n", newtarg);
	WndMain_UpdateWindows(W_XY | W_CAMERA);

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
	CmdSetKeyvalue *cmd;
	last = nullptr;

	try
	{
		cmd = new CmdSetKeyvalue(key, value);
		for (Brush *b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
		{
			// skip entity brushes in sequence
			if (b->owner == last)
				continue;
			last = b->owner;
			cmd->AddEntity(last);
		}
	}
	catch (qe3_exception &ex)
	{
		ReportError(ex);
		return;
	}

	g_cmdQueue.Complete(cmd);
	WndMain_UpdateWindows(W_SCENE | W_ENTITY);
}


/*
===============
Modify::SetKeyValueSeries
===============
*/
void Modify::SetKeyValueSeries(const char *key, const char *value, const char *firstSuffix)
{
	if (firstSuffix[0] >= '0' && firstSuffix[0] <= '9')
		Modify::SetKeyValueSeriesNum(key, value, atoi(firstSuffix));
	else if ((firstSuffix[0] >= 'a' && firstSuffix[0] <= 'z') ||
			 (firstSuffix[0] >= 'A' && firstSuffix[0] <= 'Z'))
		Modify::SetKeyValueSeriesAlpha(key, value, firstSuffix);
	else
		Error("Bad suffix specified for keyvalue series");
}

/*
===============
Modify::SetKeyValueSeriesNum
===============
*/
void Modify::SetKeyValueSeriesNum(const char *key, const char *value, const int firstSuffix)
{
	Entity *last;
	char valuesfx[260];
	int sfx = firstSuffix;

	last = nullptr;
	CmdCompound *cmdCmp = new CmdCompound("Set Sequential Keyvalues");

	for (Brush *b = g_brSelectedBrushes.prev; b != &g_brSelectedBrushes; b = b->prev)
	{
		// skip entity brushes in sequence
		if (b->owner == last)
			continue;
		last = b->owner;
		sprintf(valuesfx, "%s%i", value, sfx++);

		CmdSetKeyvalue *cmd = new CmdSetKeyvalue(key, valuesfx);
		cmd->AddEntity(last);
		if (!cmdCmp->Complete(cmd))
		{
			delete cmdCmp;
			return;
		}
	}

	g_cmdQueue.Complete(cmdCmp);

	WndMain_UpdateWindows(W_SCENE | W_ENTITY);
}

/*
===============
Modify::SetKeyValueSeriesNum
===============
*/
void Modify::SetKeyValueSeriesAlpha(const char *key, const char *value, const char *firstSuffix)
{
	Entity *last;
	char valuesfx[260];
	char szSfx[4];
	char cap;
	int sfx;

	if (firstSuffix[0] >= 'a' && firstSuffix[0] <= 'z')
	{
		cap = 'a';
	}
	else if (firstSuffix[0] >= 'A' && firstSuffix[0] <= 'Z')
	{
		cap = 'A';
	}
	else
		assert(0);

	sfx = firstSuffix[0] - cap;
	last = nullptr;
	CmdCompound *cmdCmp = new CmdCompound("Set Sequential Keyvalues");
	
	for (Brush *b = g_brSelectedBrushes.prev; b != &g_brSelectedBrushes; b = b->prev)
	{
		// skip entity brushes in sequence
		if (b->owner == last)
			continue;
		last = b->owner;

		if (sfx >= 26)
		{
			szSfx[0] = cap + sfx / 26 - 1;
			szSfx[1] = cap + (sfx % 26);
			szSfx[2] = 0;
		}
		else
		{
			szSfx[0] = cap + sfx;
			szSfx[1] = 0;
		}
		sprintf(valuesfx, "%s%s", value, szSfx);
		sfx++;

		CmdSetKeyvalue *cmd = new CmdSetKeyvalue(key, valuesfx);
		cmd->AddEntity(last);
		if (!cmdCmp->Complete(cmd))
		{
			delete cmdCmp;
			return;
		}
	}

	g_cmdQueue.Complete(cmdCmp);

	WndMain_UpdateWindows(W_SCENE | W_ENTITY);
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
		Warning("Must have a single brush selected.");
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

	WndMain_UpdateWindows(W_SCENE);
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
		Warning("Must have a single brush selected.");
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

	WndMain_UpdateWindows(W_SCENE);
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
		Warning("Must have a single brush selected.");
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

	WndMain_UpdateWindows(W_SCENE);
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
		Warning("Must have a single brush selected.");
		return;
	}

	b = g_brSelectedBrushes.next;
	CmdSphere *cmdSph = new CmdSphere();

	cmdSph->SetSides(sides);
	cmdSph->UseBrush(b);

	g_cmdQueue.Complete(cmdSph);

	WndMain_UpdateWindows(W_SCENE);

}
// <---sikk


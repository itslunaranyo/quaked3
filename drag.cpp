//==============================
//	drag.c
//==============================

#include "qe3.h"

// drag either multiple brushes, or select plane points from
// a single brush.

bool	g_bDragFirst;
bool	g_bDragOK;
vec3_t	g_v3DragXVec;
vec3_t	g_v3DragYVec;

static int		buttonstate;
static int		pressx, pressy;
static vec3_t	pressdelta;
static int		buttonx, buttony;

entity_t	   *g_peLink;

/*
==============
AxializeVector
lunaran: matches function of TextureAxisFromPlane now, used to fail on diagonal cases
==============
*/
void AxializeVector (vec3_t v)
{
	int		i, bestaxis;
	float	dot, best;

	best = 0;
	bestaxis = 0;

	for (i = 0; i < 6; i++)
	{
		dot = DotProduct(v, g_v3BaseAxis[i*3]);
		if (dot > best)
		{
			best = dot;
			bestaxis = i;
		}
	}

	for (i = 0; i < 3; i++)
	{
		v[i] = fabs(v[i]) * g_v3BaseAxis[bestaxis*3][i];
	}
}

/*
=============
MoveSelection
=============
*/
void MoveSelection (vec3_t move)
{
	int			i;
	bool		success;
	Brush	   *b;
	vec3_t		end;

	if (!move[0] && !move[1] && !move[2])
		return;

	Sys_UpdateWindows(W_ALL);

	// dragging only a part of the selection
	if (g_qeglobals.d_nNumMovePoints)
	{
// sikk---> Vertex Editing Splits Face
		if (g_qeglobals.d_selSelectMode == sel_vertex && g_qeglobals.d_savedinfo.bVertexSplitsFace)
		{
			success = true;
			for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
				success &= g_brSelectedBrushes.next->MoveVertex(g_qeglobals.d_fMovePoints[0], move, end);

			VectorCopy(end, g_qeglobals.d_fMovePoints[0]);
			return;
		}
// <---sikk

		// all other selection types (and old vertex editing mode)
		for (i = 0; i < g_qeglobals.d_nNumMovePoints; i++)
			VectorAdd(g_qeglobals.d_fMovePoints[i], move, g_qeglobals.d_fMovePoints[i]);

		for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
		{
			b->Build();
			for (i = 0; i < 3; i++)
				if (b->mins[i] > b->maxs[i]	|| b->maxs[i] - b->mins[i] > MAX_BRUSH_SIZE)
					break;	// dragged backwards or fucked up
			if (i != 3)
				break;
		}

		// if any of the brushes were crushed out of existance
		// cancel the entire move
		if (b != &g_brSelectedBrushes)
		{
			Sys_Printf("WARNING: Brush dragged backwards, move canceled.\n");
			for (i = 0; i < g_qeglobals.d_nNumMovePoints; i++)
				VectorSubtract(g_qeglobals.d_fMovePoints[i], move, g_qeglobals.d_fMovePoints[i]);

			for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
				b->Build();
		}
	}
	else
	{
// sikk---> Vertex Editing Splits Face
		// reset face originals from vertex edit mode
		// this is dirty, but unfortunately necessary because 
		// Brush_Build can remove windings
		for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
			b->ResetFaceOriginals();
// <---sikk

		// if there are lots of brushes selected, just translate instead
		// of rebuilding the brushes
		if (g_v3DragYVec[2] == 0 && g_brSelectedBrushes.next->next != &g_brSelectedBrushes)
		{
			VectorAdd(g_qeglobals.d_v3SelectTranslate, move, g_qeglobals.d_v3SelectTranslate);
		}
		else
 			Select_Move(move);
	}
}

/*
==============
UpdateTarget
==============
*/
void UpdateTarget (vec3_t origin, vec3_t dir)
{
	trace_t		t;
	entity_t   *pe;
	int			i;
	char		sz[128];

	t = Test_Ray(origin, dir, 0);

	if (!t.brush)
		return;

	pe = t.brush->owner;

	if (pe == NULL)
		return;

	// is this the first?
	if (g_peLink != NULL)
	{
		// Get the target id from out current target
		// if there is no id, make one
		i = IntForKey(pe, "target");
		if (i <= 0)
		{
			i = GetUniqueTargetId(1);
			sprintf(sz, "%d", i);

			SetKeyValue(pe, "target", sz);
		}

		// set the target # into our src
		sprintf(sz, "%d", i);
		SetKeyValue(g_peLink, "targetname", sz);

		Sys_UpdateWindows(W_ENTITY);
	}

	// promote the target to the src
	g_peLink = pe;
}

/*
===========
Drag_Setup
===========
*/
void Drag_Setup(int x, int y, int buttons,
	vec3_t xaxis, vec3_t yaxis,
	vec3_t origin, vec3_t dir)
{
	Face *f;
	trace_t	t;

	if (!Select_HasBrushes())
	{
		Undo_Start("Create Brush");	// sikk - Undo/Redo
//		Sys_Printf("MSG: No selection to drag.\n");	// sikk - Pointless Message
		return;
	}

	g_bDragFirst = true;
	g_qeglobals.d_nNumMovePoints = 0;
	VectorCopy(g_v3VecOrigin, pressdelta);
	pressx = x;
	pressy = y;

	VectorCopy(xaxis, g_v3DragXVec);
	AxializeVector(g_v3DragXVec);
	VectorCopy(yaxis, g_v3DragYVec);
	AxializeVector(g_v3DragYVec);

	if (g_qeglobals.d_selSelectMode == sel_vertex)
	{
		SelectVertexByRay(origin, dir);
		if (g_qeglobals.d_nNumMovePoints)
		{
			g_bDragOK = true;
			Undo_Start("Drag Vertex");	// sikk - Undo/Redo
			Undo_AddBrushList(&g_brSelectedBrushes);	// sikk - Undo/Redo
			return;
		}
	}
	if (g_qeglobals.d_selSelectMode == sel_edge)
	{
		SelectEdgeByRay(origin, dir);
		if (g_qeglobals.d_nNumMovePoints)
		{
			g_bDragOK = true;
			Undo_Start("Drag Edge");	// sikk - Undo/Redo
			Undo_AddBrushList(&g_brSelectedBrushes);	// sikk - Undo/Redo
			return;
		}
	}

	// check for direct hit first
	t = Test_Ray(origin, dir, true);
	if (t.selected)
	{
		g_bDragOK = true;
		Undo_Start("Drag Selection");	// sikk - Undo/Redo
		Undo_AddBrushList(&g_brSelectedBrushes);	// sikk - Undo/Redo

		if (buttons == (MK_LBUTTON | MK_CONTROL))
		{
			Sys_Printf("CMD: Shear dragging face.\n");
			t.brush->SelectFaceForDragging(t.face, true);
		}
		else if (buttons == (MK_LBUTTON | MK_CONTROL | MK_SHIFT))
		{
			// sikk - TODO: This is difficult to do as you have to 
			// hit the buttons in a certain sequence: LMB must
			// be pressed before Shift or the brush is deselected.
			// But it is a useful command to leave hidden as it is.  
			Sys_Printf("CMD: Sticky dragging brush.\n");
			for (f = t.brush->brush_faces; f; f = f->next)
				t.brush->SelectFaceForDragging(f, false);
		}
		else
			Sys_Printf("CMD: Dragging entire selection.\n");

		return;
	}

	if (g_qeglobals.d_selSelectMode == sel_vertex || g_qeglobals.d_selSelectMode == sel_edge)
		return;

	// check for side hit
	if (g_brSelectedBrushes.next->next != &g_brSelectedBrushes)
	{
		for (Brush* pBrush = g_brSelectedBrushes.next; pBrush != &g_brSelectedBrushes; pBrush = pBrush->next)
		{
			if (buttons & MK_CONTROL)
				pBrush->SideSelect(origin, dir, true);
			else
				pBrush->SideSelect(origin, dir, false);
		}
	}
	else
	{
		if (buttons & MK_CONTROL)
			g_brSelectedBrushes.next->SideSelect(origin, dir, true);
		else
			g_brSelectedBrushes.next->SideSelect(origin, dir, false);
	}
	if (g_brSelectedBrushes.next->owner->eclass->fixedsize)
		Sys_Printf("CMD: Dragging entire selection.\n");
	else
		Sys_Printf("CMD: Side stretch.\n");
	g_bDragOK = true;
	Undo_Start("Side Stretch");	// sikk - Undo/Redo
	Undo_AddBrushList(&g_brSelectedBrushes);	// sikk - Undo/Redo
}

/*
===========
Drag_TrySelect
===========
*/
bool Drag_TrySelect(int buttons, vec3_t origin, vec3_t dir)
{
	int		nFlag;	// sikk - Single Selection Cycle (Shift+Alt+LMB)

	// Shift+LBUTTON = select entire brush
	if (buttons == (MK_LBUTTON | MK_SHIFT))
	{
		nFlag = GetAsyncKeyState(VK_MENU) ? SF_CYCLE : 0;	// sikk - Single Selection Cycle (Shift+Alt+LMB)
		if (!dir[0] && !dir[1])
			Select_Ray(origin, dir, nFlag | SF_ENTITIES_FIRST);	// hack for XY	// sikk - Single Selection Cycle (Shift+Alt+LMB)
		else
			Select_Ray(origin, dir, nFlag);	// sikk - Single Selection Cycle (Shift+Alt+LMB)
		return true;
	}

	// Ctrl+Shift+LBUTTON = select single face
	if (buttons == (MK_LBUTTON | MK_CONTROL | MK_SHIFT))
	{
		// if Alt = pressed, don't deselect selected faces
		Select_DeselectAll((GetAsyncKeyState(VK_MENU) == 0));	// sikk - Multiple Face Selection
		Select_Ray(origin, dir, SF_SINGLEFACE);
		return true;
	}
	return false;
}

/*
===========
Drag_Begin
===========
*/
void Drag_Begin (int x, int y, int buttons, 
				 vec3_t xaxis, vec3_t yaxis, 
				 vec3_t origin, vec3_t dir)
{
//	int		nDim1, nDim2;
//	int		nFlag;	// sikk - Single Selection Cycle (Shift+Alt+LMB)
	trace_t	t;

	g_bDragOK = false;
	VectorCopy(g_v3VecOrigin, pressdelta);

	g_bDragFirst = true;
	g_peLink = NULL;

	// LBUTTON = manipulate selection
	if (buttons & MK_LBUTTON)
	{
		if (Drag_TrySelect(buttons, origin, dir))
			return;

		Drag_Setup(x, y, buttons, xaxis, yaxis, origin, dir);
		return;
	}

	// MBUTTON = grab texture
	if (buttons & MK_MBUTTON)
	{
		if (buttons == MK_MBUTTON)
		{
			t = Test_Ray(origin, dir, false);
			if (t.face)
			{
				UpdateWorkzone(t.brush);

				g_qeglobals.d_texturewin.ChooseTexture(&t.face->texdef, true);
				SurfWnd_UpdateUI();
			}
			else
				Sys_Printf("MSG: Did not select a texture.\n");
			return;
		}

		// Ctrl+MBUTTON = set entire brush to texture
		if (buttons == (MK_MBUTTON | MK_CONTROL))
		{
			t = Test_Ray(origin, dir, false);
			if (t.brush)
			{
				if (t.brush->brush_faces->texdef.name[0] == '(')
					Sys_Printf("WARNING: Cannot change an entity texture.\n");
				else
				{
					Undo_Start("Set Brush Texture");
					Undo_AddBrush(t.brush);
					t.brush->SetTexture(&g_qeglobals.d_workTexDef, 0);
					Undo_EndBrush(t.brush);
					Undo_End();
					Sys_UpdateWindows(W_ALL);
				}
			}
			else
				Sys_Printf("MSG: Did not hit a brush.\n");
			return;
		}

	// sikk---> Set Face Texture (face attributes remain)
		// Shift+MBUTTON = set single face to texture (face attributes remain)
		if (buttons == (MK_MBUTTON | MK_SHIFT))
		{
			t = Test_Ray(origin, dir, false);
			if (t.brush)
			{
				if (t.brush->brush_faces->texdef.name[0] == '(')
					Sys_Printf("WARNING: Cannot change an entity texture.\n");
				else
				{
					Undo_Start("Set Face Texture");
					Undo_AddBrush(t.brush);
					strcpy(t.face->texdef.name, g_qeglobals.d_workTexDef.name);
					t.brush->Build();
					Undo_EndBrush(t.brush);
					Undo_End();
					Sys_UpdateWindows(W_ALL);
				}
			}
			else
				Sys_Printf("MSG: Did not hit a brush.\n");
			return;
		}
	// <---sikk

		// Ctrl+Shift+MBUTTON = set single face to texture
		if (buttons == (MK_MBUTTON | MK_CONTROL | MK_SHIFT))
		{
			t = Test_Ray(origin, dir, false);
			if (t.brush)
			{
				if (t.brush->brush_faces->texdef.name[0] == '(')
					Sys_Printf("WARNING: Cannot change an entity texture.\n");
				else
				{
					Undo_Start("Set Face Texture");
					Undo_AddBrush(t.brush);
					t.face->texdef = g_qeglobals.d_workTexDef;
					t.brush->Build();
					Undo_EndBrush(t.brush);
					Undo_End();
					Sys_UpdateWindows(W_ALL);
				}
			}
			else
				Sys_Printf("MSG: Did not hit a brush.\n");
			return;
		}
	}
}

/*
===============
Drag_MouseMoved
===============
*/
void Drag_MouseMoved (int x, int y, int buttons)
{
	int		i;
	char	movestring[128];
	vec3_t	move, delta;

	if (!buttons)
	{
		g_bDragOK = false;
		return;
	}
	if (!g_bDragOK)
		return;

	// clear along one axis
	if (buttons & MK_SHIFT)
	{
		g_bDragFirst = false;
		if (abs(x - pressx) > abs(y - pressy))
			y = pressy;
		else
			x = pressx;
	}

	for (i = 0; i < 3; i++)
	{
		move[i] = g_v3DragXVec[i] * (x - pressx) + g_v3DragYVec[i] * (y - pressy);

		if (!g_qeglobals.d_savedinfo.bNoClamp)
			move[i] = floor(move[i] / g_qeglobals.d_nGridSize + 0.5) * g_qeglobals.d_nGridSize;
	}

	sprintf(movestring, "Drag: (%d %d %d)", (int)move[0], (int)move[1], (int)move[2]);
	Sys_Status(movestring, 0);

	VectorSubtract(move, pressdelta, delta);
	VectorCopy(move, pressdelta);

	MoveSelection(delta);
}

/*
============
Drag_MouseUp
============
*/
void Drag_MouseUp ()
{
//	Sys_Printf("MSG: Drag completed.\n");	// sikk - I Consider it Console Spamming
	if (g_qeglobals.d_v3SelectTranslate[0] || g_qeglobals.d_v3SelectTranslate[1] || g_qeglobals.d_v3SelectTranslate[2])
	{
		Select_Move(g_qeglobals.d_v3SelectTranslate);
		VectorCopy(g_v3VecOrigin, g_qeglobals.d_v3SelectTranslate);
		Sys_UpdateWindows(W_CAMERA);
	}
	Undo_EndBrushList(&g_brSelectedBrushes);	// sikk - Undo/Redo
	Undo_End();	// sikk - Undo/Redo
}

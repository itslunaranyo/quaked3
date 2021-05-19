//==============================
//	select.c
//==============================

#include "qe3.h"


vec3_t	g_v3SelectOrigin;
vec3_t	g_v3SelectMatrix[3];
bool	g_bSelectFlipOrder;

/*
===============
Test_Ray
===============
*/
trace_t Test_Ray (vec3_t origin, vec3_t dir, int flags)
{
	brush_t	   *brush;
	face_t	   *face;
	float		dist;
	trace_t		t;
	
	memset(&t, 0, sizeof(t));
	t.dist = DIST_START;

// sikk---> Single Selection Cycle (Shift+Alt+LMB)
// This code was taken from Q3Radiant. It is a total hack 
// from me to get it working and really should be rewritten.
	if (flags & SF_CYCLE)
	{
		int			nSize, j = 0;
		brush_t	   *pToSelect;
		brush_t	   *brArray[MAX_MAP_BRUSHES];  

		pToSelect = (g_brSelectedBrushes.next != &g_brSelectedBrushes) ? g_brSelectedBrushes.next : NULL;
		Select_Deselect(true);

		// go through active brushes and accumulate all "hit" brushes
		for (brush = g_brActiveBrushes.next; brush != &g_brActiveBrushes; brush = brush->next)
		{
			if (FilterBrush(brush))
				continue;

			face = Brush_Ray(origin, dir, brush, &dist);

			if (face)
			{
				brArray[j] = brush;
				j++;
			}
		}

		nSize = j;
		if (nSize)
		{
			brush_t	   *b;
			bool		bFound = false;
			int			i;

			for (i = 0; i < nSize; i++)
			{
				b = brArray[i];
				// did we hit the last one selected yet ?
				if (b == pToSelect)
				{
					// yes we want to select the next one in the list 
					int n = (i > 0) ? i - 1: nSize - 1;
					pToSelect = brArray[n];
					bFound = true;
					break;
				}
			}
			if (!bFound)
			{
				j = 0;
				pToSelect = brArray[0];
			}
		}
		if (pToSelect)
		{
			face = Brush_Ray(origin, dir, pToSelect, &dist);
			t.dist = dist;
			t.brush = pToSelect;
			t.face = face;
			t.selected = false;
			return t;
		}
	}
// <---sikk

	if (!(flags & SF_SELECTED_ONLY))
	{
		for (brush = g_brActiveBrushes.next; brush != &g_brActiveBrushes; brush = brush->next)
		{
			if ((flags & SF_ENTITIES_FIRST) && brush->owner == g_peWorldEntity)
				continue;
			if (FilterBrush(brush))
				continue;
			face = Brush_Ray(origin, dir, brush, &dist);
			if (dist > 0 && dist < t.dist)
			{
				t.dist = dist;
				t.brush = brush;
				t.face = face;
				t.selected = false;
			}
		}
	}

	for (brush = g_brSelectedBrushes.next; brush != &g_brSelectedBrushes; brush = brush->next)
	{
		if ((flags & SF_ENTITIES_FIRST) && brush->owner == g_peWorldEntity)
			continue;
		if (FilterBrush(brush))
			continue;
		face = Brush_Ray(origin, dir, brush, &dist);
		if (dist > 0 && dist < t.dist)
		{
			t.dist = dist;
			t.brush = brush;
			t.face = face;
			t.selected = true;
		}
	}

	// if entites first, but didn't find any, check regular
	if ((flags & SF_ENTITIES_FIRST) && t.brush == NULL)
		return Test_Ray(origin, dir, flags - SF_ENTITIES_FIRST);

	return t;
}


/*
================
selection testing
================
*/
bool Select_HasBrushes()
{
	return (g_brSelectedBrushes.next != &g_brSelectedBrushes);
}
bool Select_HasFaces()
{
	return (g_nSelFaceCount > 0);
}
bool Select_IsEmpty()
{
	return !( Select_HasBrushes() || Select_HasFaces() );
}

/*
================
Select_Brush
================
*/
void Select_Brush (brush_t *brush, bool bComplete)
{
	brush_t	   *b;
	entity_t   *e;
	char		selectionstring[256];
	char	   *name;
	vec3_t		vMin, vMax, vSize;

//	g_pfaceSelectedFace = NULL;
	// deselect any selected faces on brush
	Select_DeselectFaces();	// sikk - Multiple Face Selection

	if (g_qeglobals.d_nSelectCount < 2)
		g_qeglobals.d_pbrSelectOrder[g_qeglobals.d_nSelectCount] = brush;
	g_qeglobals.d_nSelectCount++;

	e = brush->owner;
	if (e)
	{
		// select complete entity on first click
		if (e != g_peWorldEntity && bComplete == true)
		{
			for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
				if (b->owner == e)
					goto singleselect;
			for (b = e->brushes.onext; b != &e->brushes; b = b->onext)
			{
				Brush_RemoveFromList(b);
				Brush_AddToList(b, &g_brSelectedBrushes);
			}
		}
		else
		{
singleselect:
			Brush_RemoveFromList(brush);
			Brush_AddToList(brush, &g_brSelectedBrushes);
		}

		if (e->eclass)
			EntWnd_UpdateEntitySel(brush->owner->eclass);
	}
	Select_GetBounds(vMin, vMax);
	VectorSubtract(vMax, vMin, vSize);
	name = ValueForKey(brush->owner, "classname");
	sprintf(selectionstring, "Selected object: %s (%d %d %d)", name, (int)vSize[0], (int)vSize[1], (int)vSize[2]);
	Sys_Status(selectionstring, 3);
}

/*
================
Select_DeselectFace
================
*/
void Select_DeselectFaces ()
{
	int i;

	if (g_nSelFaceCount)
	{
		for (i = 0; i < g_nSelFaceCount; i++)
			g_pfaceSelectedFaces[i] = NULL;

		g_nSelFaceCount = 0;
	}
	Sys_UpdateWindows(W_ALL);
}

/*
================
Select_IsFaceSelected
================
*/
bool Select_IsFaceSelected (face_t *face)
{
	int i;
	
	for (i = 0; i < g_nSelFaceCount; i++)
		if (face == g_pfaceSelectedFaces[i])
		{
			g_nSelFacePos = i;
			return true;
		}

	return false;
}

/*
================
Select_Ray

If the origin is inside a brush, that brush will be ignored.
================
*/
void Select_Ray (vec3_t origin, vec3_t dir, int flags)
{
	trace_t	t;

	t = Test_Ray(origin, dir, flags);
	if (!t.brush)
		return;

	if (flags == SF_SINGLEFACE)
	{
/*
		g_pfaceSelectedFace = t.face;
		g_pfaceSelectedFace->owner = t.brush;
		Sys_UpdateWindows(W_ALL);
		g_qeglobals.d_selSelectMode = sel_brush;
		Texture_SetTexture(&t.face->texdef);
		return;
*/			
		// deselect face if already selected
		if (Select_IsFaceSelected(t.face))
		{
			int i;
			g_pfaceSelectedFaces[g_nSelFacePos] = NULL;
			for (i = g_nSelFacePos; i < g_nSelFaceCount; i++)
				if (g_pfaceSelectedFaces[i + 1] != NULL)
					g_pfaceSelectedFaces[i] = g_pfaceSelectedFaces[i + 1];
				else
					g_pfaceSelectedFaces[i] = NULL;

			g_nSelFaceCount--;
//			Select_Deselect();
		} 
		else	
		{	// if face we clicked on is of a selected brush, do nothing
			g_pfaceSelectedFaces[g_nSelFaceCount] = t.face;
			g_pfaceSelectedFaces[g_nSelFaceCount]->owner = t.brush;
			Texture_SetTexture(&t.face->texdef, false);
			g_nSelFaceCount++;
		}

		g_qeglobals.d_selSelectMode = sel_brush;
		Sys_UpdateWindows(W_ALL);
		return;
	}

	// move the brush to the other list
	g_qeglobals.d_selSelectMode = sel_brush;

	if (t.selected)
	{		
		Brush_RemoveFromList(t.brush);
		Brush_AddToList(t.brush, &g_brActiveBrushes);
	} 
	else
		Select_Brush(t.brush, true);

	Sys_UpdateWindows(W_ALL);
}

/*
===============
Select_Delete
===============
*/
void Select_Delete ()
{
	brush_t	*brush;

//	g_pfaceSelectedFace = NULL;
// sikk---> Multiple Face Selection
	if (g_nSelFaceCount)
		Select_DeselectFaces();
// <---sikk
	g_qeglobals.d_selSelectMode = sel_brush;

	g_qeglobals.d_nSelectCount = 0;
	g_qeglobals.d_nNumMovePoints = 0;
	while (g_brSelectedBrushes.next != &g_brSelectedBrushes)
	{
		brush = g_brSelectedBrushes.next;
		Brush_Free(brush);
	}

	// FIXME: remove any entities with no brushes

	Sys_UpdateWindows(W_ALL);
}

/*
===============
UpdateWorkzone

update the workzone to a given brush
===============
*/
void UpdateWorkzone (brush_t* b)
{
	int nDim1, nDim2;

	VectorCopy(b->mins, g_qeglobals.d_v3WorkMin);
	VectorCopy(b->maxs, g_qeglobals.d_v3WorkMax);

	// will update the workzone to the given brush
	nDim1 = (g_qeglobals.d_nViewType == YZ) ? 1 : 0;
	nDim2 = (g_qeglobals.d_nViewType == XY) ? 1 : 2;

	g_qeglobals.d_v3WorkMin[nDim1] = b->mins[nDim1];
	g_qeglobals.d_v3WorkMax[nDim1] = b->maxs[nDim1];
	g_qeglobals.d_v3WorkMin[nDim2] = b->mins[nDim2];
	g_qeglobals.d_v3WorkMax[nDim2] = b->maxs[nDim2];
}

/*
===============
Select_Deselect
===============
*/
void Select_Deselect (bool bDeselectFaces)
{
	brush_t	*b;

	g_qeglobals.d_nWorkCount++;
	g_qeglobals.d_nSelectCount = 0;
	g_qeglobals.d_nNumMovePoints = 0;
	b = g_brSelectedBrushes.next;

	if (b == &g_brSelectedBrushes)
	{
/*		if (g_pfaceSelectedFace)
		{
			g_pfaceSelectedFace = NULL;
			Sys_UpdateWindows(W_ALL);
		}
*/
// sikk---> Multiple Face Selection
		if (bDeselectFaces)
			Select_DeselectFaces();
		return;
	}

//	g_pfaceSelectedFace = NULL;
	if (bDeselectFaces)
		Select_DeselectFaces();
// <---sikk

	g_qeglobals.d_selSelectMode = sel_brush;

	UpdateWorkzone(b);

	g_brSelectedBrushes.next->prev = &g_brActiveBrushes;
	g_brSelectedBrushes.prev->next = g_brActiveBrushes.next;
	g_brActiveBrushes.next->prev = g_brSelectedBrushes.prev;
	g_brActiveBrushes.next = g_brSelectedBrushes.next;
	g_brSelectedBrushes.prev = g_brSelectedBrushes.next = &g_brSelectedBrushes;	

	Sys_Status("", 3);	// sikk - Clear Selection Status

	Sys_UpdateWindows(W_ALL);
}

/*
================
Select_Move
================
*/
void Select_Move (vec3_t delta)
{
	brush_t	*b;

	// actually move the selected brushes
	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
		Brush_Move(b, delta);
//	Sys_UpdateWindows(W_ALL);
}

/*
================
Select_Clone

Creates an exact duplicate of the selection in place, then moves
the selected brushes off of their old positions
================
*/
void Select_Clone ()
{
	brush_t	   *b, *b2, *n, *next, *next2;
	vec3_t		delta;
	entity_t   *e;

	g_qeglobals.d_nWorkCount++;
	g_qeglobals.d_selSelectMode = sel_brush;

	// sikk - if no brushues are selected, return
	if (g_brSelectedBrushes.next == &g_brSelectedBrushes)
		return;

// sikk---> Move cloned brush based on active XY view. 
	if (GetTopWindow(g_qeglobals.d_hwndMain) == g_qeglobals.d_hwndXZ)
	{
		delta[0] = g_qeglobals.d_nGridSize;
		delta[1] = 0;
		delta[2] = g_qeglobals.d_nGridSize;
	}
	else if (GetTopWindow(g_qeglobals.d_hwndMain) == g_qeglobals.d_hwndYZ)
	{
		delta[0] = 0;
		delta[1] = g_qeglobals.d_nGridSize;
		delta[2] = g_qeglobals.d_nGridSize;
	}
	else
	{
		if (g_qeglobals.d_nViewType == XY)
		{
			delta[0] = g_qeglobals.d_nGridSize;
			delta[1] = g_qeglobals.d_nGridSize;
			delta[2] = 0;
		}
		else if (g_qeglobals.d_nViewType == XZ)
		{
			delta[0] = g_qeglobals.d_nGridSize;
			delta[1] = 0;
			delta[2] = g_qeglobals.d_nGridSize;
		}
		else
		{
			delta[0] = 0;
			delta[1] = g_qeglobals.d_nGridSize;
			delta[2] = g_qeglobals.d_nGridSize;
		}
	}
// <---sikk

	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = next)
	{
		next = b->next;

		Undo_EndEntity(b->owner);
		Undo_EndBrush(b);
		// if the brush is a world brush, handle simply
		if (b->owner == g_peWorldEntity)
		{
//		Undo_EndBrush(n);
			n = Brush_Clone(b);
			Brush_AddToList(n, &g_brActiveBrushes);
			Entity_LinkBrush(g_peWorldEntity, n);
			Brush_Build(n);
			Brush_Move(b, delta);
			continue;
		}

		e = Entity_Clone(b->owner);
		// clear the target / targetname
// sikk - Commented out. I'm not sure why id prefered to not keep these keys
// across clones.
//		DeleteKey(e, "target");
//		DeleteKey(e, "targetname");

		// if the brush is a fixed size entity, create a new entity
		if (b->owner->eclass->fixedsize)
		{
			n = Brush_Clone(b);
			Brush_AddToList(n, &g_brActiveBrushes);
			Entity_LinkBrush(e, n);
			Brush_Build(n);
			Brush_Move(b, delta);
			continue;
		}
        
		// brush is a complex entity, grab all the other ones now
		next = &g_brSelectedBrushes;

		for (b2 = b; b2 != &g_brSelectedBrushes; b2 = next2)
		{
			Undo_EndBrush(b2);
			next2 = b2->next;
			if (b2->owner != b->owner)
			{
				if (next == &g_brSelectedBrushes)
					next = b2;
				continue;
			}

			// move b2 to the start of g_brSelectedBrushes,
			// so it won't be hit again
			Brush_RemoveFromList(b2);
			Brush_AddToList(b2, &g_brSelectedBrushes);
			
			n = Brush_Clone(b2);
			Brush_AddToList(n, &g_brActiveBrushes);
			Entity_LinkBrush(e, n);
			Brush_Build(n);
			Brush_Move(b2, delta);
		}
	}
// sikk---> Update Enitity Inspector
	if (strcmp(g_brSelectedBrushes.next->owner->eclass->name, "worldspawn"))
		EntWnd_UpdateEntitySel(g_brSelectedBrushes.next->owner->eclass);
// <---sikk

	Sys_UpdateWindows(W_ALL);
}

/*
===============
OnBrushList

returns true if pFind is in pList
===============
*/
bool OnBrushList (brush_t *pFind, brush_t *pList[MAX_MAP_BRUSHES], int nSize)
{
	while (nSize-- > 0)
		if (pList[nSize] == pFind)
			return true;

	return false;
}

/*
================
Select_SetTexture
================
*/
void Select_SetTexture (texdef_t *texdef)
{
	brush_t	*b;

// sikk---> Multiple Face Selection
	if (g_nSelFaceCount)
	{
		int i, nBrushCount = 0;
		brush_t	*pbrArray[MAX_MAP_BRUSHES];

		for (i = 0; i < g_nSelFaceCount; i++)
		{
			// this check makes sure that brushes are only added to undo once 
			// and not once per selected face on brush
			if (!OnBrushList(g_pfaceSelectedFaces[i]->owner, pbrArray, nBrushCount))
			{
				pbrArray[nBrushCount] = g_pfaceSelectedFaces[i]->owner;
				nBrushCount++;
			}
		}

// sikk - TODO: Set Face Texture Undo bug
		Undo_Start("Set Face Textures");	// sikk - Undo/Redo
		for (i = 0; i < nBrushCount; i++)
			Undo_AddBrush(pbrArray[i]);	// sikk - Undo/Redo

		for (i = 0; i < g_nSelFaceCount; i++)
			Face_SetTexture(g_pfaceSelectedFaces[i], texdef);

		for (i = 0; i < nBrushCount; i++)
			Undo_EndBrush(pbrArray[i]);	// sikk - Undo/Redo
		Undo_End();	// sikk - Undo/Redo
	}
// <---sikk
	else if (g_brSelectedBrushes.next != &g_brSelectedBrushes)
	{
		Undo_Start("Set Brush Textures");	// sikk - Undo/Redo
		for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
		{
			if (!b->owner->eclass->fixedsize)
			{
				Undo_AddBrush(b);	// sikk - Undo/Redo
				Brush_SetTexture(b, texdef);
				Undo_EndBrush(b);	// sikk - Undo/Redo
			}
		}
		Undo_End();	// sikk - Undo/Redo
	}
	Sys_UpdateWindows(W_ALL);
}


/*
================================================================================

  TRANSFORMATIONS

===============================================================================
*/

/*
===============
Select_GetBounds
===============
*/
void Select_GetBounds (vec3_t mins, vec3_t maxs)
{
	int			i;
	brush_t	   *b;

	for (i = 0; i < 3; i++)
	{
		mins[i] = 99999;
		maxs[i] = -99999;
	}

	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		for (i = 0; i < 3; i++)
		{
			if (b->mins[i] < mins[i])
				mins[i] = b->mins[i];
			if (b->maxs[i] > maxs[i])
				maxs[i] = b->maxs[i];
		}
	}
}

/*
===============
Select_GetTrueMid
===============
*/
void Select_GetTrueMid (vec3_t mid)
{
	vec3_t	mins, maxs;
	int		i;

	Select_GetBounds(mins, maxs);

	for (i = 0; i < 3; i++)
		mid[i] = (mins[i] + ((maxs[i] - mins[i]) / 2));
}

/*
===============
Select_GetMid
===============
*/
void Select_GetMid (vec3_t mid)
{
	vec3_t	mins, maxs;
	int		i;

	if (g_qeglobals.d_savedinfo.bNoClamp)
	{
		Select_GetTrueMid(mid);
		return;
	}

	Select_GetBounds(mins, maxs);
	for (i = 0; i < 3; i++)
		mid[i] = g_qeglobals.d_nGridSize * floor(((mins[i] + maxs[i]) * 0.5 ) / g_qeglobals.d_nGridSize);
}

/*
===============
Select_ApplyMatrix
===============
*/
void Select_ApplyMatrix ()
{
	brush_t	   *b;
	face_t	   *f;
	int			i, j;
	vec3_t		temp;

	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		for (f = b->brush_faces; f; f = f->next)
		{
			for (i = 0; i < 3; i++)
			{
				VectorSubtract(f->planepts[i], g_v3SelectOrigin, temp);
				for (j = 0; j < 3; j++)
					f->planepts[i][j] = DotProduct(temp, g_v3SelectMatrix[j]) + g_v3SelectOrigin[j];
			}
			if (g_bSelectFlipOrder)
			{
				VectorCopy(f->planepts[0], temp);
				VectorCopy(f->planepts[2], f->planepts[0]);
				VectorCopy(temp, f->planepts[2]);
			}
		}
		Brush_Build(b);
	}
	Sys_UpdateWindows(W_ALL);
}

/*
===============
Select_FlipAxis
===============
*/
void Select_FlipAxis (int axis)
{
	int		i;

	Select_GetMid(g_v3SelectOrigin);
	for (i = 0; i < 3; i++)
	{
		VectorCopy(g_v3VecOrigin, g_v3SelectMatrix[i]);
		g_v3SelectMatrix[i][i] = 1;
	}
	g_v3SelectMatrix[axis][axis] = -1;

	g_bSelectFlipOrder = true;
	Select_ApplyMatrix();
	Sys_UpdateWindows(W_ALL);
}

/*
====================
Clamp
====================
*/
void Clamp (float *f, int nClamp)
{
	float fFrac = *f - (int)*f;
	*f = (int)*f % nClamp;
	*f += fFrac;
}


/*
===============
ProjectOnPlane
===============
*/
void ProjectOnPlane (vec3_t normal, float dist, vec3_t ez, vec3_t p)
{
	if (fabs(ez[0]) == 1)
		p[0] = (dist - normal[1] * p[1] - normal[2] * p[2]) / normal[0];
	else if (fabs(ez[1]) == 1)
		p[1] = (dist - normal[0] * p[0] - normal[2] * p[2]) / normal[1];
	else
		p[2] = (dist - normal[0] * p[0] - normal[1] * p[1]) / normal[2];
}

/*
===============
Back
===============
*/
void Back (vec3_t dir, vec3_t p)
{
	if (fabs(dir[0]) == 1)
		p[0] = 0;
	else if (fabs(dir[1]) == 1)
		p[1] = 0;
	else p[2] = 0;
}

/*
===============
ComputeScale

using scale[0] and scale[1]
===============
*/
void ComputeScale (vec3_t rex, vec3_t rey, vec3_t p, face_t *f)
{
	float px = DotProduct(rex, p);
	float py = DotProduct(rey, p);
	vec3_t aux;
	px *= f->texdef.scale[0];
	py *= f->texdef.scale[1];
	VectorCopy(rex, aux);
	VectorScale(aux, px, aux);
	VectorCopy(aux, p);
	VectorCopy(rey, aux);
	VectorScale(aux, py, aux);
	VectorAdd(p, aux, p);
}

/*
===============
ComputeAbsolute
===============
*/
void ComputeAbsolute (face_t *f, vec3_t p1, vec3_t p2, vec3_t p3)
{
	vec3_t	ex, ey, ez;	        // local axis base
	vec3_t	aux;
	vec3_t	rex, rey;

	// compute first local axis base
	TextureAxisFromPlane(&f->plane, ex, ey);
	CrossProduct(ex, ey, ez);
	    
	VectorCopy(ex, aux);
	VectorScale(aux, -f->texdef.shift[0], aux);
	VectorCopy(aux, p1);
	VectorCopy(ey, aux);
	VectorScale(aux, -f->texdef.shift[1], aux);
	VectorAdd(p1, aux, p1);
	VectorCopy(p1, p2);
	VectorAdd(p2, ex, p2);
	VectorCopy(p1, p3);
	VectorAdd(p3, ey, p3);
	VectorCopy(ez, aux);
	VectorScale(aux, -f->texdef.rotate, aux);
	VectorRotate(p1, aux, p1);
	VectorRotate(p2, aux, p2);
	VectorRotate(p3, aux, p3);
	// computing rotated local axis base
	VectorCopy(ex, rex);
	VectorRotate(rex, aux, rex);
	VectorCopy(ey, rey);
	VectorRotate(rey, aux, rey);

	ComputeScale(rex, rey, p1, f);
	ComputeScale(rex, rey, p2, f);
	ComputeScale(rex, rey, p3, f);

	// project on normal plane along ez 
	// assumes plane normal is normalized
	ProjectOnPlane(f->plane.normal, f->plane.dist, ez, p1);
	ProjectOnPlane(f->plane.normal, f->plane.dist, ez, p2);
	ProjectOnPlane(f->plane.normal, f->plane.dist, ez, p3);
}

/*
===============
AbsoluteToLocal
===============
*/
void AbsoluteToLocal (plane_t normal2, face_t *f, vec3_t p1, vec3_t p2, vec3_t p3)
{
	vec3_t	ex, ey, ez;
	vec3_t	aux;
	vec3_t	rex, rey;
	vec_t	x;
	vec_t	y;
	
	// computing new local axis base
	TextureAxisFromPlane(&normal2, ex, ey);
	CrossProduct(ex, ey, ez);
	
	// projecting back on (ex, ey)
	Back(ez,p1);
	Back(ez,p2);
	Back(ez,p3);
	
	// rotation
	VectorCopy(p2, aux);
	VectorSubtract(aux, p1, aux);
	
	x = DotProduct(aux, ex);
	y = DotProduct(aux, ey);
	f->texdef.rotate = 180 * atan2(y, x) / Q_PI;
	
	// computing rotated local axis base
	VectorCopy(ez, aux);
	VectorScale(aux, f->texdef.rotate, aux);
	VectorCopy(ex, rex);
	VectorRotate(rex, aux, rex);
	VectorCopy(ey, rey);
	VectorRotate(rey, aux, rey);
	
	// scale
	VectorCopy(p2, aux);
	VectorSubtract(aux, p1, aux);
	f->texdef.scale[0] = DotProduct(aux, rex);
	VectorCopy(p3, aux);
	VectorSubtract(aux, p1, aux);
	f->texdef.scale[1] = DotProduct(aux, rey);
	
	// shift
	// only using p1
	x = DotProduct(rex, p1);
	y = DotProduct(rey, p1);
	x /= f->texdef.scale[0];
	y /= f->texdef.scale[1];
	
	VectorCopy(rex, p1);
	VectorScale(p1, x, p1);
	VectorCopy(rey, aux);
	VectorScale(aux, y, aux);
	VectorAdd(p1, aux, p1);
	VectorCopy(ez, aux);
	VectorScale(aux, -f->texdef.rotate, aux);
	VectorRotate(p1, aux, p1);
	f->texdef.shift[0] = -DotProduct(p1, ex);
	f->texdef.shift[1] = -DotProduct(p1, ey);
	
	// stored rot is good considering local axis base
	// change it if necessary
	f->texdef.rotate = -f->texdef.rotate;
	
	Clamp(&f->texdef.shift[0], f->d_texture->width);
	Clamp(&f->texdef.shift[1], f->d_texture->height);
	Clamp(&f->texdef.rotate, 360);
}

/*
===============
RotateFaceTexture
===============
*/
void RotateFaceTexture (face_t* f, int nAxis, float fDeg)
{
	vec3_t	p1, p2, p3, rota;   
	vec3_t	vNormal;
	plane_t	normal2;

	p1[0] = p1[1] = p1[2] = 0;
	VectorCopy(p1, p2);
	VectorCopy(p1, p3);
	VectorCopy(p1, rota);
	ComputeAbsolute(f, p1, p2, p3);
  
	rota[nAxis] = fDeg;
	VectorRotate2(p1, rota, g_v3SelectOrigin, p1);
	VectorRotate2(p2, rota, g_v3SelectOrigin, p2);
	VectorRotate2(p3, rota, g_v3SelectOrigin, p3);

	vNormal[0] = f->plane.normal[0];
	vNormal[1] = f->plane.normal[1];
	vNormal[2] = f->plane.normal[2];
	VectorRotate(vNormal, rota, vNormal);
	normal2.normal[0] = vNormal[0];
	normal2.normal[1] = vNormal[1];
	normal2.normal[2] = vNormal[2];
	AbsoluteToLocal(normal2, f, p1, p2 ,p3);
}

/*
===============
RotateTextures
===============
*/
void RotateTextures (int nAxis, float fDeg, vec3_t vOrigin)
{
	brush_t *b;
	face_t	*f;

	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		for (f = b->brush_faces; f; f = f->next)
		{
			RotateFaceTexture(f, nAxis, fDeg);
			Brush_Build(b);
		}
		Brush_Build(b);
	}
}

/*
===============
Select_RotateAxis
===============
*/
void Select_RotateAxis (int axis, float deg, bool bMouse)
{
/*
	vec3_t	temp;
	int		i, j;
	vec_t	c, s;

	if (deg == 0)
		return;

	Select_GetMid(g_v3SelectOrigin);
	g_bSelectFlipOrder = false;

	if (deg == 90)
	{
		for (i = 0; i < 3; i++)
		{
			VectorCopy(g_v3VecOrigin, g_v3SelectMatrix[i]);
			g_v3SelectMatrix[i][i] = 1;
		}
		i = (axis + 1) % 3;
		j = (axis + 2) % 3;
		VectorCopy(g_v3SelectMatrix[i], temp);
		VectorCopy(g_v3SelectMatrix[j], g_v3SelectMatrix[i]);
		VectorSubtract(g_v3VecOrigin, temp, g_v3SelectMatrix[j]);
	}
	else
	{
		deg = -deg;
		if (deg == -180)
		{
			c = -1;
			s = 0;
		}
		else if (deg == -270)
		{
			c = 0;
			s = -1;
		}
		else
		{
			c = cos(deg * Q_PI / 180.0);
			s = sin(deg * Q_PI / 180.0);
		}

		for (i = 0; i < 3; i++)
		{
			VectorCopy(g_v3VecOrigin, g_v3SelectMatrix[i]);
			g_v3SelectMatrix[i][i] = 1;
		}

		switch (axis)
		{
		case 0:
			g_v3SelectMatrix[1][1] = c;
			g_v3SelectMatrix[1][2] = -s;
			g_v3SelectMatrix[2][1] = s;
			g_v3SelectMatrix[2][2] = c;
			break;
		case 1:
			g_v3SelectMatrix[0][0] = c;
			g_v3SelectMatrix[0][2] = s;
			g_v3SelectMatrix[2][0] = -s;
			g_v3SelectMatrix[2][2] = c;
			break;
		case 2:
			g_v3SelectMatrix[0][0] = c;
			g_v3SelectMatrix[0][1] = -s;
			g_v3SelectMatrix[1][0] = s;
			g_v3SelectMatrix[1][1] = c;
			break;
		}
	}
*/
	int		i;
	vec_t	c, s;


	while (deg >= 360)
		deg -= 360;
	while (deg < 0)
		deg += 360;

	if (deg == 0)
		return;

	if (bMouse)
	{
		VectorCopy(g_v3RotateOrigin, g_v3SelectOrigin);
	}
	else
		Select_GetMid(g_v3SelectOrigin);
	g_bSelectFlipOrder = false;
	
	deg = -deg;
	c = cos(deg * Q_PI / 180.0);
	s = sin(deg * Q_PI / 180.0);

	for (i = 0; i < 3; i++)
	{
		VectorCopy(g_v3VecOrigin, g_v3SelectMatrix[i]);
		g_v3SelectMatrix[i][i] = 1;
	}
	
	switch (axis)
	{
	case 0:
		g_v3SelectMatrix[1][1] = c;
		g_v3SelectMatrix[1][2] = -s;
		g_v3SelectMatrix[2][1] = s;
		g_v3SelectMatrix[2][2] = c;
		break;
	case 1:
		g_v3SelectMatrix[0][0] = c;
		g_v3SelectMatrix[0][2] = s;
		g_v3SelectMatrix[2][0] = -s;
		g_v3SelectMatrix[2][2] = c;
		break;
	case 2:
		g_v3SelectMatrix[0][0] = c;
		g_v3SelectMatrix[0][1] = -s;
		g_v3SelectMatrix[1][0] = s;
		g_v3SelectMatrix[1][1] = c;
		break;
	}

	if (g_qeglobals.d_bTextureLock)
		RotateTextures(axis, deg, g_v3SelectOrigin);

	Select_ApplyMatrix();
	Sys_UpdateWindows(W_XY | W_Z | W_CAMERA);
}

// sikk---> Brush Scaling
/*
===========
Select_Scale
===========
*/
void Select_Scale (float x, float y, float z)
{
	int			i;
	brush_t	   *b;
	face_t	   *f;

	Select_GetMid(g_v3SelectOrigin);

	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		for (f = b->brush_faces; f; f = f->next)
		{
			for (i = 0; i < 3; i++)
			{
				f->planepts[i][0] -= g_v3SelectOrigin[0];
				f->planepts[i][1] -= g_v3SelectOrigin[1];
				f->planepts[i][2] -= g_v3SelectOrigin[2];

				f->planepts[i][0] *= x;
//				f->planepts[i][0] = floor(f->planepts[i][0] / g_qeglobals.d_nGridSize + 0.5) * g_qeglobals.d_nGridSize;
				f->planepts[i][1] *= y;
//				f->planepts[i][1] = floor(f->planepts[i][1] / g_qeglobals.d_nGridSize + 0.5) * g_qeglobals.d_nGridSize;
				f->planepts[i][2] *= z;
//				f->planepts[i][2] = floor(f->planepts[i][2] / g_qeglobals.d_nGridSize + 0.5) * g_qeglobals.d_nGridSize;
        
				f->planepts[i][0] += g_v3SelectOrigin[0];
				f->planepts[i][1] += g_v3SelectOrigin[1];
				f->planepts[i][2] += g_v3SelectOrigin[2];
			}
		}

		Brush_Build(b);
	}

	Sys_UpdateWindows(W_XY | W_Z | W_CAMERA);
}
// <---sikk

/*
===============
Select_FitTexture
===============
*/
void Select_FitTexture (int nHeight, int nWidth)
{
	brush_t	*b;
	
	if (g_brSelectedBrushes.next == &g_brSelectedBrushes && !g_nSelFaceCount)
		return;
	
	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		Brush_FitTexture(b, nHeight, nWidth);
		Brush_Build(b);
	}
	
// sikk---> Multiple Face Selection
	if (g_nSelFaceCount)
	{
		int i;
		for (i = 0; i < g_nSelFaceCount; i++)
		{
			Face_FitTexture(g_pfaceSelectedFaces[i], nHeight, nWidth);
			Brush_Build(g_pfaceSelectedFaces[i]->owner);
		}
	}
// <---sikk
	
	Sys_UpdateWindows(W_CAMERA);
}

// sikk---> Texture Manipulation Functions (Mouse & Keyboard)
/*
===========
Select_ShiftTexture
===========
*/
void Select_ShiftTexture (int x, int y)
{
	brush_t	*b;
	face_t	*f;

	if(g_brSelectedBrushes.next == &g_brSelectedBrushes && !g_nSelFaceCount)
		return;

	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		for (f = b->brush_faces; f; f = f->next)
		{
			f->texdef.shift[0] += x * g_qeglobals.d_nGridSize;
			f->texdef.shift[1] += y * g_qeglobals.d_nGridSize;
		}

		Brush_Build(b);
	}
// sikk---> Multiple Face Selection
	if (g_nSelFaceCount)
	{
		int i;
		for (i = 0; i < g_nSelFaceCount; i++)
		{
			g_pfaceSelectedFaces[i]->texdef.shift[0] += x * g_qeglobals.d_nGridSize;
			g_pfaceSelectedFaces[i]->texdef.shift[1] += y * g_qeglobals.d_nGridSize;
			Select_SetTexture(&g_pfaceSelectedFaces[i]->texdef);
//			Brush_Build(g_pfaceSelectedFaces[i]->owner);
		}
	}
// <--sikk

	Sys_UpdateWindows(W_CAMERA);
}

/*
===========
Select_ScaleTexture
===========
*/
void Select_ScaleTexture (int x, int y)
{
	brush_t	*b;
	face_t	*f;

	if(g_brSelectedBrushes.next == &g_brSelectedBrushes && !g_nSelFaceCount)
		return;

	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		for (f = b->brush_faces; f; f = f->next)
		{ 
			f->texdef.scale[0] += x * 0.05f;
			f->texdef.scale[1] += y * 0.05f;
		}

		Brush_Build(b);
	}

// sikk---> Multiple Face Selection
	if (g_nSelFaceCount)
	{
		int i;
		for (i = 0; i < g_nSelFaceCount; i++)
		{
			g_pfaceSelectedFaces[i]->texdef.scale[0] += x * 0.05f;
			g_pfaceSelectedFaces[i]->texdef.scale[1] += y * 0.05f;
			Select_SetTexture(&g_pfaceSelectedFaces[i]->texdef);
//			Brush_Build(g_pfaceSelectedFaces[i]->owner);
		}
	}
// <---sikk

	Sys_UpdateWindows(W_CAMERA);
}

/*
===========
Select_RotateTexture
===========
*/
void Select_RotateTexture (int deg)
{
	brush_t	*b;
	face_t	*f;

	if(g_brSelectedBrushes.next == &g_brSelectedBrushes && !g_nSelFaceCount)
		return;

	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		for (f = b->brush_faces; f; f = f->next)
		{
			f->texdef.rotate += deg;
			if (f->texdef.rotate >= 360)
				f->texdef.rotate -= 360;
			if (f->texdef.rotate < 0)
				f->texdef.rotate += 360;

		}

		Brush_Build(b);
	}
	
// sikk---> Multiple Face Selection
	if (g_nSelFaceCount)
	{
		int i;
		for (i = 0; i < g_nSelFaceCount; i++)
		{
			g_pfaceSelectedFaces[i]->texdef.rotate += deg;
			if (g_pfaceSelectedFaces[i]->texdef.rotate >= 360)
				g_pfaceSelectedFaces[i]->texdef.rotate -= 360;
			if (g_pfaceSelectedFaces[i]->texdef.rotate < 0)
				g_pfaceSelectedFaces[i]->texdef.rotate += 360;
			Select_SetTexture(&g_pfaceSelectedFaces[i]->texdef);
//			Brush_Build(g_pfaceSelectedFaces[i]->owner);
		}
	}
// <---sikk

	Sys_UpdateWindows (W_CAMERA);
}
// <---sikk

/*
================================================================================

GROUP SELECTIONS

================================================================================
*/

// sikk---> Select All
/*
===============
Select_All
===============
*/void Select_All ()
{
	brush_t	*b, *next;

	for (b = g_brActiveBrushes.next; b != &g_brActiveBrushes; b = next)
	{
		next = b->next;
		Brush_RemoveFromList(b);
		Brush_AddToList(b, &g_brSelectedBrushes);
	}
	Sys_UpdateWindows(W_ALL);
}
// <---sikk

// sikk---> Select All Type
/*
===============
Select_AllType
===============
*/void Select_AllType ()
{
	brush_t	*b, *next, *selected;

	selected = g_brSelectedBrushes.next;
	// if nothing is selected, do nothing and return
	if (selected == &g_brSelectedBrushes)
		return;

	for (b = g_brActiveBrushes.next; b != &g_brActiveBrushes; b = next)
	{
		next = b->next;
	
		if (!strcmp(b->owner->eclass->name, selected->owner->eclass->name))
		{
			Brush_RemoveFromList(b);
			Brush_AddToList(b, &g_brSelectedBrushes);
		}
	}
	Sys_UpdateWindows(W_ALL);
}
// <---sikk

/*
===============
Select_CompleteTall
===============
*/
void Select_CompleteTall ()
{
	brush_t	   *b, *next;
//	int			i;
	vec3_t		mins, maxs;
	int			nDim1, nDim2;

	if (!QE_SingleBrush())
		return;

	g_qeglobals.d_selSelectMode = sel_brush;

	VectorCopy(g_brSelectedBrushes.next->mins, mins);
	VectorCopy(g_brSelectedBrushes.next->maxs, maxs);
	Select_Delete();

// sikk---> Multiple Orthographic Views
	if (GetTopWindow(g_qeglobals.d_hwndMain) == g_qeglobals.d_hwndXZ)
	{
		nDim1 = 0;
		nDim2 = 2;
	}
	else if (GetTopWindow(g_qeglobals.d_hwndMain) == g_qeglobals.d_hwndYZ)
	{
		nDim1 = 1;
		nDim2 = 2;
	}
	else
	{
		nDim1 = (g_qeglobals.d_nViewType == YZ) ? 1 : 0;
		nDim2 = (g_qeglobals.d_nViewType == XY) ? 1 : 2;
	}
// <---sikk

	for (b = g_brActiveBrushes.next; b != &g_brActiveBrushes; b = next)
	{
		next = b->next;

		if ((b->maxs[nDim1] > maxs[nDim1] || b->mins[nDim1] < mins[nDim1]) || 
			(b->maxs[nDim2] > maxs[nDim2] || b->mins[nDim2] < mins[nDim2]))
			continue;

	 	if (FilterBrush(b))
	 		continue;

		Brush_RemoveFromList(b);
		Brush_AddToList(b, &g_brSelectedBrushes);
/*
		// old stuff
		for (i = 0; i < 2; i++)
			if (b->maxs[i] > maxs[i] || b->mins[i] < mins[i])
				break;
		if (i == 2)
		{
			Brush_RemoveFromList(b);
			Brush_AddToList(b, &g_brSelectedBrushes);
		}
*/
	}
	Sys_UpdateWindows(W_ALL);
}

/*
===============
Select_PartialTall
===============
*/
void Select_PartialTall ()
{
	brush_t	   *b, *next;
//	int			i;
	vec3_t		mins, maxs;
	int			nDim1, nDim2;

	if (!QE_SingleBrush())
		return;

	g_qeglobals.d_selSelectMode = sel_brush;

	VectorCopy(g_brSelectedBrushes.next->mins, mins);
	VectorCopy(g_brSelectedBrushes.next->maxs, maxs);
	Select_Delete();

// sikk---> Multiple Orthographic Views
	if (GetTopWindow(g_qeglobals.d_hwndMain) == g_qeglobals.d_hwndXZ)
	{
		nDim1 = 0;
		nDim2 = 2;
	}
	else if (GetTopWindow(g_qeglobals.d_hwndMain) == g_qeglobals.d_hwndYZ)
	{
		nDim1 = 1;
		nDim2 = 2;
	}
	else
	{
		nDim1 = (g_qeglobals.d_nViewType == YZ) ? 1 : 0;
		nDim2 = (g_qeglobals.d_nViewType == XY) ? 1 : 2;
	}
// <---sikk

	for (b = g_brActiveBrushes.next; b != &g_brActiveBrushes; b = next)
	{
		next = b->next;

		if ((b->mins[nDim1] > maxs[nDim1] || b->maxs[nDim1] < mins[nDim1]) || 
			(b->mins[nDim2] > maxs[nDim2] || b->maxs[nDim2] < mins[nDim2]) )
			continue;

	 	if (FilterBrush(b))
	 		continue;

		Brush_RemoveFromList(b);
		Brush_AddToList(b, &g_brSelectedBrushes);
/*
		// old stuff
		for (i = 0; i < 2; i++)
			if (b->mins[i] > maxs[i] || b->maxs[i] < mins[i])
				break;
		if (i == 2)
		{
			Brush_RemoveFromList(b);
			Brush_AddToList(b, &g_brSelectedBrushes);
		}
*/
	}
	Sys_UpdateWindows(W_ALL);
}

/*
===============
Select_Touching
===============
*/
void Select_Touching ()
{
	brush_t	   *b, *next;
	int			i;
	vec3_t		mins, maxs;

	if (!QE_SingleBrush())
		return;

	g_qeglobals.d_selSelectMode = sel_brush;

	VectorCopy(g_brSelectedBrushes.next->mins, mins);
	VectorCopy(g_brSelectedBrushes.next->maxs, maxs);

	for (b = g_brActiveBrushes.next; b != &g_brActiveBrushes; b = next)
	{
		next = b->next;
		for (i = 0; i < 3; i++)
			if (b->mins[i] > maxs[i] + 1 || b->maxs[i] < mins[i] - 1)
				break;
		if (i == 3)
		{
			Brush_RemoveFromList(b);
			Brush_AddToList(b, &g_brSelectedBrushes);
		}
	}
	Sys_UpdateWindows(W_ALL);
}

/*
===============
Select_Inside
===============
*/
void Select_Inside ()
{
	brush_t	   *b, *next;
	int			i;
	vec3_t		mins, maxs;

	if (!QE_SingleBrush())
		return;

	g_qeglobals.d_selSelectMode = sel_brush;

	VectorCopy(g_brSelectedBrushes.next->mins, mins);
	VectorCopy(g_brSelectedBrushes.next->maxs, maxs);
	Select_Delete();

	for (b = g_brActiveBrushes.next; b != &g_brActiveBrushes; b = next)
	{
		next = b->next;
		for (i = 0; i < 3; i++)
			if (b->maxs[i] > maxs[i] || b->mins[i] < mins[i])
				break;
		if (i == 3)
		{
			Brush_RemoveFromList(b);
			Brush_AddToList(b, &g_brSelectedBrushes);
		}
	}
	Sys_UpdateWindows(W_ALL);
}

/*
=================
Select_Ungroup

Turn the currently selected entity back into normal brushes
=================
*/
void Select_Ungroup ()
{
	entity_t	*e;
	brush_t		*b;

	e = g_brSelectedBrushes.next->owner;

	if (!e || e == g_peWorldEntity || e->eclass->fixedsize)
	{
		Sys_Printf("WARNING: Not a grouped entity.\n");
		return;
	}

	for (b = e->brushes.onext; b != &e->brushes; b = e->brushes.onext)
	{
		Brush_RemoveFromList(b);
		Brush_AddToList(b, &g_brActiveBrushes);
		Entity_UnlinkBrush(b);
		Entity_LinkBrush(g_peWorldEntity, b);
		Brush_Build(b);
		b->owner = g_peWorldEntity;
		Select_Brush(b, true);	// sikk - reselect the ungrouped brush  
	}

	Entity_Free(e);
	Sys_UpdateWindows(W_ALL);
}

/*
===============
Select_NextBrushInGroup
===============
*/
void Select_NextBrushInGroup ()
{
	brush_t		*b;
	brush_t		*b2;
	entity_t	*e;

	// check to see if the selected brush is part of a func group
	// if it is, deselect everything and reselect the next brush 
	// in the group
	b = g_brSelectedBrushes.next;
	if (b != &g_brSelectedBrushes)
	{
		if (strcmpi(b->owner->eclass->name, "worldspawn") != 0)
		{
			e = b->owner;
			Select_Deselect(true);
			for (b2 = e->brushes.onext; b2 != &e->brushes; b2 = b2->onext)
			{
				if (b == b2)
				{
					b2 = b2->onext;
					break;
				}
			}
			if (b2 == &e->brushes)
			b2 = b2->onext;

			Select_Brush(b2, false);
			Sys_UpdateWindows(W_ALL);
		}
	}
}

// sikk---> Insert Brush into Entity
/*
===============
Select_InsertBrush
===============
*/
void Select_InsertBrush ()
{
	eclass_t   *ec;
	epair_t	   *ep;
	entity_t   *e, *e2;
	brush_t	   *b;
	bool		bCheck = false, bInserting = false;

	// check to make sure we have a brush
	if (g_brSelectedBrushes.next == &g_brSelectedBrushes)
		return;

	// if any selected brushes is a point entity, return
	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		if (b->owner->eclass->fixedsize)
		{
			Sys_Printf("WARNING: Selection contains a point entity. No insertion done\n");
			return;
		}
	}
	
	// find first brush that's an entity, pull info and continue
	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		if (b->owner != g_peWorldEntity)
		{
			e2 = b->owner;
			ec = b->owner->eclass;
			ep = b->owner->epairs;
			bCheck = true;
			continue;
		}
	}

	// if all selected brushes are from world, return
	if (!bCheck)
		return;

	// check whether we are inserting a brush or just reordering (for console text only)
	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		if (b->owner == g_peWorldEntity)
		{
			bInserting = true;
			continue;
		}
	}

	// create it
	e = qmalloc(sizeof(*e));
//	e->entityId = g_nEntityId++;	// sikk - Undo/Redo
	e->brushes.onext = e->brushes.oprev = &e->brushes;
	e->eclass = ec;
	e->epairs = ep;

	e->next = g_entEntities.next;
	g_entEntities.next = e;
	e->next->prev = e;
	e->prev = &g_entEntities;
	
	// change the selected brushes over to the new entity
	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		Entity_UnlinkBrush(b);
		Entity_LinkBrush(e, b);
		Brush_Build(b);	// so the key brush gets a name
	}

	// free old entity
	while (e2->brushes.onext != &e2->brushes)
		Brush_Free(e2->brushes.onext);

	if (e2->next)
	{
		e2->next->prev = e2->prev;
		e2->prev->next = e2->next;
	}

	if (bInserting)
		Sys_Printf("CMD: Brush(es) inserted into entity \"%s\"\n", e->eclass->name);
	else
		Sys_Printf("CMD: Brush entity \"%s\" reordered\n", e->eclass->name);

	EntWnd_UpdateEntitySel(e->eclass);	// sikk - Update Enitity Inspector 

	Sys_UpdateWindows(W_ALL);
}
// <---sikk

/*
================
Select_Invert
================
*/
void Select_Invert ()
{
	brush_t *next, *prev;

	Sys_Printf("CMD: Inverting selection...\n");

	next = g_brActiveBrushes.next;
	prev = g_brActiveBrushes.prev;

	if (g_brSelectedBrushes.next != &g_brSelectedBrushes)
	{
		g_brActiveBrushes.next = g_brSelectedBrushes.next;
		g_brActiveBrushes.prev = g_brSelectedBrushes.prev;
		g_brActiveBrushes.next->prev = &g_brActiveBrushes;
		g_brActiveBrushes.prev->next = &g_brActiveBrushes;
	}
	else
	{
		g_brActiveBrushes.next = &g_brActiveBrushes;
		g_brActiveBrushes.prev = &g_brActiveBrushes;
	}

	if (next != &g_brActiveBrushes)
	{
		g_brSelectedBrushes.next = next;
		g_brSelectedBrushes.prev = prev;
		g_brSelectedBrushes.next->prev = &g_brSelectedBrushes;
		g_brSelectedBrushes.prev->next = &g_brSelectedBrushes;
	}
	else
	{
		g_brSelectedBrushes.next = &g_brSelectedBrushes;
		g_brSelectedBrushes.prev = &g_brSelectedBrushes;
	}

	Sys_UpdateWindows(W_ALL);
	Sys_Printf("MSG: Done.\n");
}

/*
===============
Select_Hide
===============
*/
void Select_Hide ()
{
	brush_t *b;

	for (b = g_brSelectedBrushes.next; b && b != &g_brSelectedBrushes; b = b->next)
		b->hiddenBrush = true;

	Sys_UpdateWindows (W_ALL);
}

/*
===============
Select_ShowAllHidden
===============
*/
void Select_ShowAllHidden ()
{
	brush_t *b;

	for (b = g_brSelectedBrushes.next; b && b != &g_brSelectedBrushes; b = b->next)
		b->hiddenBrush = false;

	for (b = g_brActiveBrushes.next; b && b != &g_brActiveBrushes; b = b->next)
		b->hiddenBrush = false;

	Sys_UpdateWindows(W_ALL);
}

/*
===============
Select_ConnectEntities

Sets target/targetname on the two entities selected
from the first selected to the secon
===============
*/
void Select_ConnectEntities ()
{
	entity_t   *e1, *e2, *e;
	char	   *target, *tn;
	int			maxtarg, targetnum;
	char		newtarg[32];

	if (g_qeglobals.d_nSelectCount != 2)
	{
		Sys_Printf("WARNING: Must have two entities selected.\n");
		Sys_Beep();
		return;
	}

	e1 = g_qeglobals.d_pbrSelectOrder[0]->owner;
	e2 = g_qeglobals.d_pbrSelectOrder[1]->owner;

	if (e1 == g_peWorldEntity || e2 == g_peWorldEntity)
	{
		Sys_Printf("WARNING: Cannot connect to the world.\n");
		Sys_Beep();
		return;
	}

	if (e1 == e2)
	{
		Sys_Printf("WARNING: Brushes are from same entity.\n");
		Sys_Beep();
		return;
	}

	target = ValueForKey(e1, "target");
	if (target && target[0])
		strcpy(newtarg, target);
	else
	{
		target = ValueForKey(e2, "targetname");
		if (target && target[0])
			strcpy(newtarg, target);
		else
		{
			// make a unique target value
			maxtarg = 0;
			for (e = g_entEntities.next; e != &g_entEntities; e = e->next)
			{
				tn = ValueForKey(e, "targetname");
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

	SetKeyValue(e1, "target", newtarg);
	SetKeyValue(e2, "targetname", newtarg);
	Sys_UpdateWindows(W_XY | W_CAMERA);

	Select_Deselect(true);
	Select_Brush(g_qeglobals.d_pbrSelectOrder[1], true);
}

// sikk---> Select Matching Key/Value
/*
===============
Select_MatchingKeyValue
===============
*/
void Select_MatchingKeyValue (char *szKey, char *szValue)
{
	brush_t    *b, *bnext;
	epair_t	   *ep;
	bool		bFound;

	Select_Deselect(true);

	if (strlen(szKey) && strlen(szValue))	// if both "key" & "value" are declared
	{
		for (b = g_brActiveBrushes.next; b != &g_brActiveBrushes; b = bnext)
		{
			bnext = b->next;

			if (!strcmp(ValueForKey(b->owner, szKey), szValue))
			{
				Brush_RemoveFromList(b);
				Brush_AddToList(b, &g_brSelectedBrushes);
			}
		}
	}
	else if (strlen(szKey))	// if only "key" is declared
	{
		for (b = g_brActiveBrushes.next; b != &g_brActiveBrushes; b = bnext)
		{
			bnext = b->next;

			if (strlen(ValueForKey(b->owner, szKey)))
			{
				Brush_RemoveFromList(b);
				Brush_AddToList(b, &g_brSelectedBrushes);
			}
		}
	}
	else if (strlen(szValue))	// if only "value" is declared
	{
		for (b = g_brActiveBrushes.next; b != &g_brActiveBrushes; b = bnext)
		{
			bnext = b->next;
			bFound = false;

			for (ep = b->owner->epairs; ep && !bFound; ep = ep->next)
			{
				if (!strcmp(ep->value, szValue))
				{
					Brush_RemoveFromList(b);
					Brush_AddToList(b, &g_brSelectedBrushes);
					bFound = true;	// this is so an entity with two different keys 
				}					// with identical values are not selected twice
			}
		}
	}

	Sys_UpdateWindows(W_ALL);	
}
// <---sikk

// sikk---> Select Matching Textures
/*
===============
Select_MatchingTextures
===============
*/
void Select_MatchingTextures ()
{
	brush_t	   *b, *next;
	face_t	   *f;
	texdef_t   *pt;

	if (g_nSelFaceCount)
		pt = &g_pfaceSelectedFaces[0]->texdef;
	else
		pt = &g_qeglobals.d_texturewin.texdef;

	Select_Deselect(true);
	
	for (b = g_brActiveBrushes.next; b != &g_brActiveBrushes; b = next)
	{
		next = b->next;
		for (f = b->brush_faces; f; f = f->next)
		{
			if (!strcmp(f->texdef.name, pt->name))
			{
				g_pfaceSelectedFaces[g_nSelFaceCount] = f;
				g_pfaceSelectedFaces[g_nSelFaceCount]->owner = b;
				g_nSelFaceCount++;
			}
		}
	}

	Sys_UpdateWindows(W_ALL);
}
// <---sikk

/*
===============
FindReplaceTextures
===============
*/
void FindReplaceTextures (char *pFind, char *pReplace, bool bSelected, bool bForce)
{
	brush_t	*pBrush, *pList;
	face_t	*pFace;

	pList = (bSelected) ? &g_brSelectedBrushes : &g_brActiveBrushes;
	if (!bSelected)
		Select_Deselect(true);

	for (pBrush = pList->next; pBrush != pList; pBrush = pBrush->next)
	{
		for (pFace = pBrush->brush_faces; pFace; pFace = pFace->next)
		{
			if (bForce || strcmpi(pFace->texdef.name, pFind) == 0)
			{
				pFace->d_texture = Texture_ForName(pFace->texdef.name);
				strcpy(pFace->texdef.name, pReplace);
			}
		}
		Brush_Build(pBrush);
	}
	Sys_UpdateWindows(W_CAMERA);
}

// sikk---> Cut/Copy/Paste
/*
===============
OnEntityList

returns true if pFind is in pList
===============
*/
bool OnEntityList (entity_t *pFind, entity_t *pList[MAX_MAP_ENTITIES], int nSize)
{
	while (nSize-- > 0)
	{
		if (pList[nSize] == pFind)
			return true;
	}
	return false;
}

/*
==================
Select_Cut
==================
*/
void Select_Cut ()
{
	int nCount = 0;
	brush_t		*b, *b2, *eb, *eb2;
	entity_t	*e, *e2, *pentArray[MAX_MAP_ENTITIES];


	Brush_CleanList(&g_brCopiedBrushes);
	g_brCopiedBrushes.next = g_brCopiedBrushes.prev = &g_brCopiedBrushes;
	Entity_CleanList();

	for (b = g_brSelectedBrushes.next; b != NULL && b != &g_brSelectedBrushes; b = b->next)
	{
		if (b->owner == g_peWorldEntity)
		{
			b2 = Brush_Clone(b);
			b2->owner = NULL;
			Brush_AddToList(b2, &g_brCopiedBrushes);
		}
		else
		{
			// this check makes sure that brushents are only copied once 
			// and not once per brush in entity
			if (!OnEntityList(b->owner, pentArray, nCount))
			{
				e = b->owner;
				pentArray[nCount] = e;
				e2 = Entity_Copy(e);
				for (eb = e->brushes.onext; eb != &e->brushes; eb = eb->onext)
				{
					eb2 = Brush_Clone(eb);
//					Brush_AddToList(eb2, &g_brCopiedBrushes);
					Entity_LinkBrush(e2, eb2);
					Brush_Build(eb2);
				}
				nCount++;
			}
		}
	}

	Select_Delete();
	
	Sys_UpdateWindows(W_ALL);
}

/*
==================
Select_Copy
==================
*/
void Select_Copy ()
{
	int nCount = 0;
	brush_t		*b, *b2, *eb, *eb2;
	entity_t	*e, *e2, *pentArray[MAX_MAP_ENTITIES];

	Brush_CleanList(&g_brCopiedBrushes);
	g_brCopiedBrushes.next = g_brCopiedBrushes.prev = &g_brCopiedBrushes;
	Entity_CleanList();

	for (b = g_brSelectedBrushes.next; b != NULL && b != &g_brSelectedBrushes; b = b->next)
	{
		if (b->owner == g_peWorldEntity)
		{
			b2 = Brush_Clone(b);
			b2->owner = NULL;
			Brush_AddToList(b2, &g_brCopiedBrushes);
		}
		else
		{
			// this check makes sure that brushents are only copied once 
			// and not once per brush in entity
			if (!OnEntityList(b->owner, pentArray, nCount))
			{
				e = b->owner;
				pentArray[nCount] = e;
				e2 = Entity_Copy(e);
				for (eb = e->brushes.onext; eb != &e->brushes; eb = eb->onext)
				{
					eb2 = Brush_Clone(eb);
//					Brush_AddToList(eb2, &g_brCopiedBrushes);
					Entity_LinkBrush(e2, eb2);
					Brush_Build(eb2);
				}
				nCount++;
			}
		}
	}
}

/*
==================
Select_Paste
==================
*/
void Select_Paste ()
{
	brush_t		*b, *b2, *eb, *eb2;
	entity_t	*e, *e2;

//	if (g_brCopiedBrushes.next != &g_brCopiedBrushes || g_entCopiedEntities.next != &g_entCopiedEntities)
	if (g_brCopiedBrushes.next != NULL)
	{
		Select_Deselect(true);

		for (b = g_brCopiedBrushes.next; b != NULL && b != &g_brCopiedBrushes; b = b->next)
		{
			b2 = Brush_Clone(b);
			Undo_EndBrush(b2);
//			pClone->owner = pBrush->owner;
			if (b2->owner == NULL)
				Entity_LinkBrush(g_peWorldEntity, b2);
			Brush_AddToList(b2, &g_brSelectedBrushes);
			Brush_Build(b2);
		}

		for (e = g_entCopiedEntities.next; e != NULL && e != &g_entCopiedEntities; e = e->next)
		{
			e2 = Entity_Clone(e);
			Undo_EndEntity(e2);
			for (eb = e->brushes.onext; eb != &e->brushes; eb = eb->onext)
			{
				eb2 = Brush_Clone(eb);
				Undo_EndBrush(eb2);
				Brush_AddToList(eb2, &g_brSelectedBrushes);
				Entity_LinkBrush(e2, eb2);
				Brush_Build(eb2);
				if (eb2->owner && eb2->owner != g_peWorldEntity)
					EntWnd_UpdateEntitySel(eb2->owner->eclass);
			}
		}

// sikk---> Update Enitity Inspector
		if (strcmp(g_brSelectedBrushes.next->owner->eclass->name, "worldspawn"))
			EntWnd_UpdateEntitySel(g_brSelectedBrushes.next->owner->eclass);
// <---sikk

		Sys_UpdateWindows(W_ALL);
	}
	else
		Sys_Printf("MSG: Nothing to paste...\n");
}
// <---sikk
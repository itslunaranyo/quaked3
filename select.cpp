//==============================
//	select.c
//==============================

#include "qe3.h"

Brush	g_brSelectedBrushes;	// highlighted
Face	*g_vfSelectedFaces[MAX_MAP_FACES];	// sikk - Multiple Face Selection
int		g_nSelFaceCount;
bool	g_bSelectionChanged;

vec3	g_v3SelectOrigin;
vec3	g_v3SelectMatrix[3];
bool	g_bSelectFlipOrder;

vec3	g_v3RotateOrigin;	// sikk - Free Rotate


//============================================================
//
//	SELECTION STATE FUNCTIONS WHICH BELONG HERE
//
//============================================================


void Selection::Changed() { g_bSelectionChanged = true;  }

/*
================
Selection::HandleChange

lunaran: handle all consequences of the selection changing in one place, so that
ui updates are consistent, centralized, and not repeated unnecessarily
================
*/
void Selection::HandleChange()
{
	if (!g_bSelectionChanged) return;

	DeselectFiltered();

	vec3		vMin, vMax, vSize;
	char		selectionstring[256];
	char		*name;

	if (HasBrushes())
	{
		GetBounds(vMin, vMax);
		vSize = vMax - vMin;
		name = g_brSelectedBrushes.next->owner->GetKeyValue("classname");
		sprintf(selectionstring, "Selected: %s (%d %d %d)", name, (int)vSize[0], (int)vSize[1], (int)vSize[2]);
		Sys_Status(selectionstring, 3);
	}
	else
	{
		Sys_Status("", 3);
	}

	for (auto tIt = g_qeglobals.d_tools.begin(); tIt != g_qeglobals.d_tools.end(); ++tIt)
	{
		(*tIt)->SelectionChanged();
	}

	SurfWnd_UpdateUI();
	Sys_UpdateWindows(W_SCENE|W_ENTITY);
	g_bSelectionChanged = false;
}

/*
================
interface functions
================
*/
bool Selection::HasBrushes()
{
	return (g_brSelectedBrushes.next && (g_brSelectedBrushes.next != &g_brSelectedBrushes));
}
int Selection::FaceCount()
{
	return g_nSelFaceCount;
}
bool Selection::IsEmpty()
{
	return !( HasBrushes() || FaceCount() );
}
bool Selection::OnlyPointEntities()
{
	for (Brush* b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
		if (b->owner->IsBrush())
			return false;

	return true;
}
int Selection::NumBrushes()
{
	int i = 0;
	for (Brush* b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next) i++;
	return i;
}
int Selection::NumFaces()
{
	return g_nSelFaceCount;
}

/*
=================
Selection::IsBrushSelected
=================
*/
bool Selection::IsBrushSelected(Brush* bSel)
{
	Brush* b;

	for (b = g_brSelectedBrushes.next; b != NULL && b != &g_brSelectedBrushes; b = b->next)
		if (b == bSel)
			return true;

	return false;
}

/*
================
Selection::SelectBrush
================
*/
void Selection::SelectBrush(Brush* b)
{
	if (b->next && b->prev)
		b->RemoveFromList();

	b->AddToList(&g_brSelectedBrushes);

	Selection::Changed();
}

/*
================
Selection::SelectBrushSorted
add a brush to the selected list, but adjacent to other brushes in the same entity
================
*/
void Selection::SelectBrushSorted(Brush* b)
{
	if (b->next && b->prev)
		b->RemoveFromList();

	// world brushes, point entities, and brush ents with only one brush go to the end 
	// of the list, so we don't keep looping over them looking for buddies
	if (b->owner->IsWorld() || b->owner->IsPoint() || b->onext == b)
	{
		b->AddToList(g_brSelectedBrushes.prev);
		return;
	}

	Brush* bCur;

	for (bCur = g_brSelectedBrushes.next; bCur != &g_brSelectedBrushes; bCur = bCur->next)
	{
		if (bCur->owner == b->owner)
			break;
	}

	b->AddToList(bCur);

	Selection::Changed();
}

/*
================
Selection::HandleBrush
================
*/
void Selection::HandleBrush (Brush *brush, bool bComplete)
{
	Brush	   *b;
	Entity   *e;

	DeselectAllFaces();	// sikk - Multiple Face Selection

	// we have hit an unselected brush
	e = brush->owner;
	if (e)
	{
		// select complete entity on first click
		if (!e->IsWorld() && bComplete == true)
		{
			for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
			{
				if (b->owner != e) continue;

				// current entity is partially selected, select just this brush
				//SelectBrush(brush);
				brush->RemoveFromList();
				brush->AddToList(b);	// add next to its partners to minimize entity fragmentation in the list
				Selection::Changed();
				return;
			}
			// current entity is not selected at all, select the whole thing
			for (b = e->brushes.onext; b != &e->brushes; b = b->onext)
			{
				SelectBrush(b);
			}
		}
		else
		{
			// select just this brush
			SelectBrush(brush);
		}
	}
}

/*
================
Selection::DeselectAllFaces
================
*/
bool Selection::DeselectAllFaces()
{
	if (!g_nSelFaceCount) return false;

	g_vfSelectedFaces[0] = NULL;
	g_nSelFaceCount = 0;
	g_qeglobals.d_selSelectMode = sel_brush;

	Selection::Changed();
	return true;
}

/*
================
Selection::IsFaceSelected
================
*/
bool Selection::IsFaceSelected (Face *face)
{
	int i;
	
	for (i = 0; i < g_nSelFaceCount; i++)
		if (face == g_vfSelectedFaces[i])
			return true;

	return false;
}


/*
================
Selection::SelectFace
================
*/
void Selection::SelectFace(Face* f)
{
	assert(!IsFaceSelected(f));
	if (!f->face_winding)
		return;
	g_vfSelectedFaces[g_nSelFaceCount++] = f;
	g_vfSelectedFaces[g_nSelFaceCount] = NULL;	// maintain list null-terminated

	Selection::Changed();
}

/*
================
Selection::DeselectFace

returns true or false if face could be deselected or not
================
*/
bool Selection::DeselectFace(Face* f)
{
	for (int i = 0; i < g_nSelFaceCount; i++)
	{
		if (f == g_vfSelectedFaces[i])
		{
			g_vfSelectedFaces[i] = NULL;
			for (i; i < g_nSelFaceCount; i++)
				if (g_vfSelectedFaces[i + 1] != NULL)
					g_vfSelectedFaces[i] = g_vfSelectedFaces[i + 1];
				else
					g_vfSelectedFaces[i] = NULL;

			Selection::Changed();
			g_nSelFaceCount--;
			return true;
		}
	}
	return false;
}

/*
===============
Selection::NumBrushFacesSelected
===============
*/
int Selection::NumBrushFacesSelected(Brush* b)
{
	int sum;
	sum = 0;

	for (int i = 0; i < g_nSelFaceCount; i++)
	{
		if (g_vfSelectedFaces[i]->owner == b)
			sum++;
	}
	return sum;
}

/*
===============
Selection::FacesToBrushes
===============
*/
void Selection::FacesToBrushes(bool partial)
{
	Brush *b;

	if (!FaceCount())
		return;

	b = NULL;
	for (int i = 0; i < FaceCount(); i++)
	{
		if (b == g_vfSelectedFaces[i]->owner)
			continue;

		b = g_vfSelectedFaces[i]->owner;
		if (partial || NumBrushFacesSelected(b) == b->NumFaces())
		{
			SelectBrushSorted(b);
		}
	}
	DeselectAllFaces();
	Selection::Changed();
	g_qeglobals.d_selSelectMode = sel_brush;
}

/*
===============
Selection::BrushesToFaces
===============
*/
void Selection::BrushesToFaces()
{
	Brush *b;
	Face *f;

	if (!HasBrushes())
		return;

	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		for (f = b->basis.faces; f; f = f->fnext)
		{
			SelectFace(f);
		}
	}
	g_qeglobals.d_selSelectMode = sel_face;
	g_brSelectedBrushes.MergeListIntoList(&g_map.brActive);
	Selection::Changed();
}





// ============================================================




/*
===============
Test_BrushFilter
===============
*/
bool Test_BrushFilter(Brush *brush, int flags)
{
	if ((flags & SF_ENTITIES_FIRST) && brush->owner->IsWorld())
		return false;
	if (flags & (SF_NOFIXEDSIZE | SF_FACES) && brush->owner->IsPoint())
		return false;
	if (brush->IsFiltered())
		return false;
	return true;
}

/*
===============
Selection::TestRay
===============
*/
trace_t Selection::TestRay(const vec3 origin, const vec3 dir, int flags)
{
	Brush	*brush;
	Face	*face;
	float	dist;
	trace_t	t;

	// single selection cycle:
	// find the first unselected brush behind the first selected brush
	if (flags & SF_CYCLE && HasBrushes())
	{
		memset(&t, 0, sizeof(t));
		t.dist = DIST_START;

		vec3 org;
		float bestdist = DIST_START;
		for (brush = g_brSelectedBrushes.next; brush != &g_brSelectedBrushes; brush = brush->next)
		{
			if (!Test_BrushFilter(brush, flags))
				continue;
			face = brush->RayTest(origin, dir, &dist);
			if (dist > 0 && dist < bestdist)
				bestdist = dist;
		}
		org = origin + glm::normalize(dir) * bestdist;
		for (brush = g_map.brActive.next; brush != &g_map.brActive; brush = brush->next)
		{
			if (!Test_BrushFilter(brush, flags))
				continue;
			face = brush->RayTest(org, dir, &dist);
			if (dist > 0 && dist < t.dist)
			{
				t.dist = dist;
				t.brush = brush;
				t.face = face;
				t.selected = false;
			}
		}
		if (t.brush) return t;
	}

	memset(&t, 0, sizeof(t));
	t.dist = DIST_START;

	if (!(flags & SF_SELECTED_ONLY))
	{
		for (brush = g_map.brActive.next; brush != &g_map.brActive; brush = brush->next)
		{
			if (!Test_BrushFilter(brush, flags))
				continue;
			face = brush->RayTest(origin, dir, &dist);
			if (dist > 0 && dist < t.dist)
			{
				t.dist = dist;
				t.brush = brush;
				t.face = face;
				t.selected = false;
			}
		}
	}

	if (!(flags & SF_FACES))
	{
		for (brush = g_brSelectedBrushes.next; brush != &g_brSelectedBrushes; brush = brush->next)
		{
			if (!Test_BrushFilter(brush, flags))
				continue;
			face = brush->RayTest(origin, dir, &dist);
			if (dist > 0 && dist < t.dist)
			{
				t.dist = dist;
				t.brush = brush;
				t.face = face;
				t.selected = true;
			}
		}
	}

	// if entities first, but didn't find any, check regular
	if ((flags & SF_ENTITIES_FIRST) && t.brush == NULL)
		return TestRay(origin, dir, flags - SF_ENTITIES_FIRST);

	return t;
}


/*
================
Selection::Ray

If the origin is inside a brush, that brush will be ignored.
================
*/
int Selection::Ray(const vec3 origin, const vec3 dir, int flags)
{
	trace_t	t;
	int out;

	t = TestRay(origin, dir, flags);
	if (!t.brush)
		return 0;

	out = flags;
	if (flags & SF_FACES)
	{
		g_qeglobals.d_selSelectMode = sel_face;
		if (IsFaceSelected(t.face))
		{
			if (flags & SF_UNSELECTED) return 0;
			out |= SF_SELECTED;
			DeselectFace(t.face);
		}
		else
		{
			if (flags & SF_SELECTED) return 0;
			out |= SF_UNSELECTED;
			SelectFace(t.face);
			//t.face->owner = t.brush;	// important safety tip: this is important because apparently face.owner isn't set to anything by default?
			g_qeglobals.d_vTexture.ChooseTexture(&t.face->texdef, false);
		}
	}
	else
	{
		g_qeglobals.d_selSelectMode = sel_brush;
		if (flags & SF_CYCLE)
		{
			DeselectAll();
			HandleBrush(t.brush, false);
			return 0;
		}
		// move the brush to the other list
		if (t.selected)
		{
			if (flags & SF_UNSELECTED) return 0;
			out |= SF_SELECTED;
			t.brush->RemoveFromList();
			t.brush->AddToList(&g_map.brActive);
			Selection::Changed();
		}
		else
		{
			if (flags & SF_SELECTED) return 0;
			out |= SF_UNSELECTED;
			HandleBrush(t.brush, true);
		}
	}
	return out;
}

/*
================
Selection::TestPoint
================
*/
trace_t Selection::TestPoint(const vec3 origin, int flags)
{
	trace_t t;
	Brush* brush;

	memset(&t, 0, sizeof(t));

	if (!(flags & SF_SELECTED_ONLY))
	{
		for (brush = g_map.brActive.next; brush != &g_map.brActive; brush = brush->next)
		{
			if (!Test_BrushFilter(brush, flags))
				continue;
			if (brush->PointTest(origin))
			{
				t.brush = brush;
				t.selected = false;
			}
		}
	}
	if (t.brush) return t;

	if (!(flags & SF_FACES))
	{
		for (brush = g_brSelectedBrushes.next; brush != &g_brSelectedBrushes; brush = brush->next)
		{
			if (!Test_BrushFilter(brush, flags))
				continue;
			if (brush->PointTest(origin))
			{
				t.brush = brush;
				t.selected = true;
			}
		}
	}
	return t;
}

/*
================
Selection::Point
================
*/
void Selection::Point(const vec3 origin, int flags)
{
	if (flags & SF_FACES)
		return;

	trace_t t;
	t = TestPoint(origin, flags);
	if (!t.brush)
		return;

	g_qeglobals.d_selSelectMode = sel_brush;
	// move the brush to the other list
	if (t.selected)
	{
		t.brush->RemoveFromList();
		t.brush->AddToList(&g_map.brActive);
		Selection::Changed();
	}
	else
	{
		HandleBrush(t.brush, true);
	}
}






// ================================================================





/*
===============
Selection::SelectAll
===============
*/
void Selection::SelectAll()
{
	/*
	// sikk---> Select All
	Brush	*b, *next;

	for (b = g_map.brActive.next; b != &g_map.brActive; b = next)
	{
		next = b->next;
		SelectBrush(b);
	}
	// <---sikk
	*/

	DeselectAllFaces();

	g_map.brActive.MergeListIntoList(&g_brSelectedBrushes);

	Selection::Changed();
}

/*
===============
Selection::DeselectFiltered
===============
*/
void Selection::DeselectFiltered()
{
	Brush *b, *next;

	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = next)
	{
		next = b->next;

		if (b->IsFiltered())
		{
			b->RemoveFromList();
			b->AddToList(&g_map.brActive);
			Selection::Changed();
		}
	}
}

/*
===============
Selection::DeselectAll
===============
*/
void Selection::DeselectAll()
{
	//g_qeglobals.d_nWorkCount++;
	g_qeglobals.d_nNumMovePoints = 0;

	DeselectAllFaces();

	g_qeglobals.d_selSelectMode = sel_brush;

	if (!HasBrushes())
		return;

	UpdateWorkzone(g_brSelectedBrushes.next);

	g_brSelectedBrushes.MergeListIntoList(&g_map.brActive);
	Selection::Changed();
}

/*
===============
Selection::GetBounds
===============
*/
void Selection::GetBounds(vec3 &mins, vec3 &maxs)
{
	int			i;
	Brush	   *b;

	ClearBounds(mins, maxs);

	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		for (i = 0; i < 3; i++)
		{
			if (b->basis.mins[i] < mins[i])
				mins[i] = b->basis.mins[i];
			if (b->basis.maxs[i] > maxs[i])
				maxs[i] = b->basis.maxs[i];
		}
	}
}

/*
===============
Selection::GetTrueMid
===============
*/
void Selection::GetTrueMid(vec3 &mid)
{
	vec3	mins, maxs;
	int		i;

	GetBounds(mins, maxs);

	for (i = 0; i < 3; i++)
		mid[i] = (mins[i] + ((maxs[i] - mins[i]) / 2));
}

/*
===============
Selection::GetMid
===============
*/
void Selection::GetMid(vec3 &mid)
{
	vec3	mins, maxs;
	int		i;

	if (g_qeglobals.d_savedinfo.bNoClamp)
	{
		GetTrueMid(mid);
		return;
	}

	GetBounds(mins, maxs);
	// lunaran: don't snap the midpoint to the grid, snap the bounds first so selections don't wander when rotated
	for (i = 0; i < 3; i++)
	{
		mins[i] = g_qeglobals.d_nGridSize * floor(mins[i] / g_qeglobals.d_nGridSize);
		maxs[i] = g_qeglobals.d_nGridSize * ceil(maxs[i] / g_qeglobals.d_nGridSize);
		mid[i] = roundf((mins[i] + maxs[i]) * 0.5f);
	}
	//for (i = 0; i < 3; i++)
	//	mid[i] = g_qeglobals.d_nGridSize * floor(((basis.mins[i] + basis.maxs[i]) * 0.5) / g_qeglobals.d_nGridSize);
}



// sikk---> Select Matching Key/Value
/*
===============
Selection::MatchingKeyValue
===============
*/
void Selection::MatchingKeyValue(char *szKey, char *szValue)
{
	Brush    *b, *bnext;
	EPair	   *ep;
	bool		bFound;

	DeselectAll();

	if (strlen(szKey) && strlen(szValue))	// if both "key" & "value" are declared
	{
		for (b = g_map.brActive.next; b != &g_map.brActive; b = bnext)
		{
			bnext = b->next;

			if (!strcmp(b->owner->GetKeyValue(szKey), szValue))
			{
				SelectBrushSorted(b);
			}
		}
	}
	else if (strlen(szKey))	// if only "key" is declared
	{
		for (b = g_map.brActive.next; b != &g_map.brActive; b = bnext)
		{
			bnext = b->next;

			if (strlen(b->owner->GetKeyValue(szKey)))
			{
				SelectBrushSorted(b);
			}
		}
	}
	else if (strlen(szValue))	// if only "value" is declared
	{
		for (b = g_map.brActive.next; b != &g_map.brActive; b = bnext)
		{
			bnext = b->next;
			bFound = false;

			for (ep = b->owner->epairs; ep && !bFound; ep = ep->next)
			{
				if (ep->value == szValue)
				{
					SelectBrushSorted(b);
					bFound = true;	// this is so an entity with two different keys 
				}					// with identical values are not selected twice
			}
		}
	}
	Selection::Changed();
}
// <---sikk

// sikk---> Select Matching Textures
/*
===============
Selection::MatchingTextures
===============
*/
void Selection::MatchingTextures()
{
	Brush	   *b, *next;
	Face	   *f;
	texdef_t   *texdef;

	if (g_nSelFaceCount)
		texdef = &g_vfSelectedFaces[0]->texdef;
	else
		texdef = &g_qeglobals.d_workTexDef;

	DeselectAll();

	for (b = g_map.brActive.next; b != &g_map.brActive; b = next)
	{
		next = b->next;
		for (f = b->basis.faces; f; f = f->fnext)
		{
			if (!strcmp(f->texdef.name, texdef->name))
			{
				SelectFace(f);
			}
		}
	}
}
// <---sikk


/*
================
Selection::Invert
================
*/
void Selection::Invert()
{
	Brush *next, *prev;

	//Sys_Printf("CMD: Inverting selection...\n");

	next = g_map.brActive.next;
	prev = g_map.brActive.prev;

	if (g_brSelectedBrushes.next != &g_brSelectedBrushes)
	{
		g_map.brActive.next = g_brSelectedBrushes.next;
		g_map.brActive.prev = g_brSelectedBrushes.prev;
		g_map.brActive.next->prev = &g_map.brActive;
		g_map.brActive.prev->next = &g_map.brActive;
	}
	else
	{
		g_map.brActive.next = &g_map.brActive;
		g_map.brActive.prev = &g_map.brActive;
	}

	if (next != &g_map.brActive)
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

	Selection::Changed();
	//Sys_Printf("MSG: Done.\n");
}



// sikk---> Select All Type
/*
===============
Selection::AllType
===============
*/
void Selection::AllType()
{
	Brush	*b, *next, *selected;

	selected = g_brSelectedBrushes.next;
	// if nothing is selected, do nothing and return
	if (selected == &g_brSelectedBrushes)
		return;

	for (b = g_map.brActive.next; b != &g_map.brActive; b = next)
	{
		next = b->next;

		if (!strcmp(b->owner->eclass->name, selected->owner->eclass->name))
		{
			SelectBrushSorted(b);
		}
	}
}
// <---sikk

/*
===============
Selection::CompleteTall
===============
*/
void Selection::CompleteTall()
{
	Brush	   *b, *next;
	//	int			i;
	vec3		mins, maxs;
	int			nDim1, nDim2;

	if (!QE_SingleBrush())
		return;

	g_qeglobals.d_selSelectMode = sel_brush;

	mins = g_brSelectedBrushes.next->basis.mins;
	maxs = g_brSelectedBrushes.next->basis.maxs;
	Modify_Delete();

	// lunaran - grid view reunification
	{
		int nViewType;/*
		XYZView* xyz = XYZWnd_WinFromHandle(GetTopWindow(g_qeglobals.d_hwndMain));
		if (xyz)
			nViewType = xyz->dViewType;
		else
			nViewType = XY;*/
		nViewType = QE_BestViewAxis();
		nDim1 = (nViewType == YZ) ? 1 : 0;
		nDim2 = (nViewType == XY) ? 1 : 2;
	}

	for (b = g_map.brActive.next; b != &g_map.brActive; b = next)
	{
		next = b->next;

		if ((b->basis.maxs[nDim1] > maxs[nDim1] || b->basis.mins[nDim1] < mins[nDim1]) ||
			(b->basis.maxs[nDim2] > maxs[nDim2] || b->basis.mins[nDim2] < mins[nDim2]))
			continue;

		if (b->IsFiltered())
			continue;

		SelectBrushSorted(b);
	}
}

/*
===============
Selection::PartialTall
===============
*/
void Selection::PartialTall()
{
	Brush	   *b, *next;
	//	int			i;
	vec3		mins, maxs;
	int			nDim1, nDim2;

	if (!QE_SingleBrush())
		return;

	g_qeglobals.d_selSelectMode = sel_brush;

	mins = g_brSelectedBrushes.next->basis.mins;
	maxs = g_brSelectedBrushes.next->basis.maxs;
	Modify_Delete();

	// lunaran - grid view reunification
	{
		int nViewType;/*
		XYZView* xyz = XYZWnd_WinFromHandle(GetTopWindow(g_qeglobals.d_hwndMain));
		if (xyz)
			nViewType = xyz->dViewType;
		else
			nViewType = XY; */
			nViewType = QE_BestViewAxis();
		nDim1 = (nViewType == YZ) ? 1 : 0;
		nDim2 = (nViewType == XY) ? 1 : 2;
	}

	for (b = g_map.brActive.next; b != &g_map.brActive; b = next)
	{
		next = b->next;

		if ((b->basis.mins[nDim1] > maxs[nDim1] || b->basis.maxs[nDim1] < mins[nDim1]) ||
			(b->basis.mins[nDim2] > maxs[nDim2] || b->basis.maxs[nDim2] < mins[nDim2]))
			continue;

		if (b->IsFiltered())
			continue;

		SelectBrushSorted(b);
	}
}

/*
===============
Selection::Touching
===============
*/
void Selection::Touching()
{
	Brush	   *b, *next;
	int			i;
	vec3		mins, maxs;

	if (!QE_SingleBrush())
		return;

	g_qeglobals.d_selSelectMode = sel_brush;

	mins = g_brSelectedBrushes.next->basis.mins;
	maxs = g_brSelectedBrushes.next->basis.maxs;

	for (b = g_map.brActive.next; b != &g_map.brActive; b = next)
	{
		next = b->next;
		for (i = 0; i < 3; i++)
			if (b->basis.mins[i] > maxs[i] + 1 || b->basis.maxs[i] < mins[i] - 1)
				break;
		if (i == 3)
		{
			SelectBrushSorted(b);
		}
	}
}

/*
===============
Selection::Inside
===============
*/
void Selection::Inside()
{
	Brush	   *b, *next;
	int			i;
	vec3		mins, maxs;

	if (!QE_SingleBrush())
		return;

	g_qeglobals.d_selSelectMode = sel_brush;

	mins = g_brSelectedBrushes.next->basis.mins;
	maxs = g_brSelectedBrushes.next->basis.maxs;
	Modify_Delete();

	for (b = g_map.brActive.next; b != &g_map.brActive; b = next)
	{
		next = b->next;
		for (i = 0; i < 3; i++)
			if (b->basis.maxs[i] > maxs[i] || b->basis.mins[i] < mins[i])
				break;
		if (i == 3)
		{
			SelectBrushSorted(b);
		}
	}
}

/*
===============
Selection::NextBrushInGroup
===============
*/
void Selection::NextBrushInGroup()
{
	Brush		*b;
	Brush		*b2;
	Entity	*e;

	// check to see if the selected brush is part of a func group
	// if it is, deselect everything and reselect the next brush 
	// in the group
	b = g_brSelectedBrushes.next;
	if (b != &g_brSelectedBrushes)
	{
		if (_strcmpi(b->owner->eclass->name, "worldspawn") != 0)
		{
			e = b->owner;
			DeselectAll();
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

			HandleBrush(b2, false);
		}
	}
}




//============================================================
//
//	FUNCTIONS WHICH OPERATE ON WHAT IS SELECTED AND DO NOT BELONG HERE
//
//============================================================




/*
===============
Modify_Delete
===============
*/
void Modify_Delete ()
{
//	Brush	*brush;
//	Entity	*e;

	if (!Selection::HasBrushes()) return;

//	g_pfaceSelectedFace = NULL;
// sikk---> Multiple Face Selection
	if (Selection::FaceCount())
		Selection::DeselectAllFaces();
// <---sikk
	g_qeglobals.d_selSelectMode = sel_brush;
	g_qeglobals.d_nNumMovePoints = 0;

	CmdDelete *cmd = new CmdDelete(&g_brSelectedBrushes);
	g_cmdQueue.Complete(cmd);

	/*
	while (Selection::HasBrushes())
	{
		brush = g_brSelectedBrushes.next;
	//	e = brush->owner;
	//	delete brush;


		// remove any (not-worldspawn) entities with no brushes
	//	if (e->brushes.onext == &e->brushes && !e->IsWorld())
	//		delete e;
		// actually leave them around, an undo might restore their brushes
	}
	*/
	Sys_UpdateWindows(W_ALL);
}

/*
================
Modify_Clone	

Creates an exact duplicate of the selection in place, then moves
the selected brushes off of their old positions
================
*/
void Modify_Clone ()
{
	vec3	delta(0);

	if (g_qeglobals.d_selSelectMode != sel_brush) return;
	if (!Selection::HasBrushes()) return;

	// move cloned brushes based on active XY view
	switch (QE_BestViewAxis())//XYZWnd_GetTopWindowViewType())
	{
	case XZ:
		delta[0] = g_qeglobals.d_nGridSize;
	//	delta[1] = 0;
		delta[2] = g_qeglobals.d_nGridSize;
		break;
	case YZ:
	//	delta[0] = 0;
		delta[1] = g_qeglobals.d_nGridSize;
		delta[2] = g_qeglobals.d_nGridSize;
		break;
	default:
	case XY:
		delta[0] = g_qeglobals.d_nGridSize;
		delta[1] = g_qeglobals.d_nGridSize;
	//	delta[2] = 0;
	}

	CmdClone *cmd = new CmdClone(&g_brSelectedBrushes, delta);
	Selection::DeselectAll();
	g_cmdQueue.Complete(cmd);

	Sys_UpdateWindows(W_ALL);
}



/*
================================================================================

  TRANSFORMATIONS

===============================================================================
*/


/*
================
Transform_Move
================
*/
void Transform_Move (const vec3 delta)
{
	for (Brush *b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		if (b->owner->IsPoint())
			b->owner->Move(delta);
		else
			b->Move(delta);
	}
//	Sys_UpdateWindows(W_ALL);
}


/*
===============
Transform_ApplyMatrix
===============
*/
void Transform_ApplyMatrix ()
{
	Brush	*b;
	Face	*f;
	int		i, j;
	vec3	temp;

	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		for (f = b->basis.faces; f; f = f->fnext)
		{
			for (i = 0; i < 3; i++)
			{
				temp = f->planepts[i] - g_v3SelectOrigin;
				for (j = 0; j < 3; j++)
					f->planepts[i][j] = DotProduct(temp, g_v3SelectMatrix[j]) + g_v3SelectOrigin[j];
			}
			if (g_bSelectFlipOrder)
			{
				temp = f->planepts[0];
				f->planepts[0] = f->planepts[2];
				f->planepts[2] = temp;
			}
		}
		b->Build();
	}
	Sys_UpdateWindows(W_SCENE);
}

/*
===============
Transform_FlipAxis
===============
*/
void Transform_FlipAxis (int axis)
{
	int		i;

	Selection::GetMid(g_v3SelectOrigin);
	for (i = 0; i < 3; i++)
	{
		g_v3SelectMatrix[i] = vec3(0);
		g_v3SelectMatrix[i][i] = 1;
	}
	g_v3SelectMatrix[axis][axis] = -1;

	g_bSelectFlipOrder = true;
	Transform_ApplyMatrix();
	Sys_UpdateWindows(W_SCENE);
}


/*
===============
Transform_RotateAxis
===============
*/
void Transform_RotateAxis (int axis, float deg, bool bMouse)
{
/*
	vec3_t	temp;
	int		i, j;
	vec_t	c, s;

	if (deg == 0)
		return;

	Selection::GetMid(g_v3SelectOrigin);
	g_bSelectFlipOrder = false;

	if (deg == 90)
	{
		for (i = 0; i < 3; i++)
		{
			g_v3SelectMatrix[i] = vec3(0);
			g_v3SelectMatrix[i][i] = 1;
		}
		i = (axis + 1) % 3;
		j = (axis + 2) % 3;
		temp = g_v3SelectMatrix[i];
		g_v3SelectMatrix[i] = g_v3SelectMatrix[j];
		g_v3SelectMatrix[j] = vec3(0) - temp;
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
			g_v3SelectMatrix[i] = vec3(0);
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
		g_v3SelectOrigin = g_v3RotateOrigin;
	}
	else
		Selection::GetMid(g_v3SelectOrigin);
	g_bSelectFlipOrder = false;
	
	deg = -deg;
	c = cos(deg * Q_PI / 180.0);
	s = sin(deg * Q_PI / 180.0);

	for (i = 0; i < 3; i++)
	{
		g_v3SelectMatrix[i] = vec3(0);
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
		Surf_RotateForTransform(axis, deg, g_v3SelectOrigin);

	Transform_ApplyMatrix();
	Sys_UpdateWindows(W_SCENE);
}

// sikk---> Brush Scaling
/*
===========
Transform_Scale
===========
*/
void Transform_Scale (float x, float y, float z)
{
	int			i;
	Brush	   *b;
	Face	   *f;

	Selection::GetMid(g_v3SelectOrigin);

	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		for (f = b->basis.faces; f; f = f->fnext)
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

		b->Build();
	}

	Sys_UpdateWindows(W_SCENE);
}
// <---sikk


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
void Modify_Ungroup ()
{
//	Entity	*e;
//	Brush	*b;

	/*
	e = g_brSelectedBrushes.next->owner;

	if (!e || e->IsWorld() || e->IsPoint())
	{
		Sys_Printf("WARNING: Not a grouped entity.\n");
		return;
	}

	for (b = e->brushes.onext; b != &e->brushes; b = e->brushes.onext)
	{
		b->RemoveFromList();
		b->AddToList(&g_map.brActive);
		Entity::UnlinkBrush(b);
		g_map.world->LinkBrush(b);
		b->Build();
		b->owner = g_map.world;
		Selection::HandleBrush(b, true);	// sikk - reselect the ungrouped brush  
	}

	delete e;
	*/
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
void Modify_InsertBrush ()
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

	/*

	EntClass	*ec;
	EPair		*ep;
	Entity		*e, *e2;
	Brush		*b;
	bool		bCheck = false, bInserting = false;

	// check to make sure we have a brush
	if (!Selection::HasBrushes())
		return;

	// if any selected brushes is a point entity, return
	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		if (b->owner->IsPoint())
		{
			Sys_Printf("WARNING: Selection contains a point entity. No insertion done\n");
			return;
		}
	}
	
	// find first brush that's an entity, pull info and continue
	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		if (!b->owner->IsWorld())
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
		if (b->owner->IsWorld())
		{
			bInserting = true;
			continue;
		}
	}

	// create it
	e = new Entity();	// note: this shouldn't increment entityid for some reason
	e->eclass = ec;
	e->epairs = ep;

	e->next = g_map.entities.next;
	g_map.entities.next = e;
	e->next->prev = e;
	e->prev = &g_map.entities;
	
	// change the selected brushes over to the new entity
	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		Entity::UnlinkBrush(b);
		e->LinkBrush(b);
		b->Build();	// so the key brush gets a name
	}

	// free old entity
	while (e2->brushes.onext != &e2->brushes)
		delete e2->brushes.onext;

	if (e2->next)
	{
		e2->next->prev = e2->prev;
		e2->prev->next = e2->next;
	}

	if (bInserting)
		Sys_Printf("CMD: Brush(es) inserted into entity \"%s\"\n", e->eclass->name);
	else
		Sys_Printf("CMD: Brush entity \"%s\" reordered\n", e->eclass->name);

	Selection::Changed();
	Sys_UpdateWindows(W_ALL);
	*/
}
// <---sikk



/*
===============
Modify_Hide
===============
*/
void Modify_Hide ()
{
	Brush *b;

	for (b = g_brSelectedBrushes.next; b && b != &g_brSelectedBrushes; b = b->next)
	{
		// lunaran TODO: figure out how these brushes get deselected :itisamystery:
		b->hiddenBrush = true;
	}

	Selection::Changed();
}

/*
===============
Modify_ShowHidden
===============
*/
void Modify_ShowHidden ()
{
	Brush *b;

	for (b = g_brSelectedBrushes.next; b && b != &g_brSelectedBrushes; b = b->next)
		b->hiddenBrush = false;

	for (b = g_map.brActive.next; b && b != &g_map.brActive; b = b->next)
		b->hiddenBrush = false;

	Sys_UpdateWindows(W_ALL);
}

/*
===============
Modify_ConnectEntities

Sets target/targetname on the first two entities selected in order
lunaran TODO: confirmation box if target & targetname already clash before overwriting
===============
*/
void Modify_ConnectEntities ()
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



// ================================================================



/*
===============
OnBrushList

returns true if pFind is in pList
===============
*/
bool OnBrushList(Brush *pFind, Brush *pList[MAX_MAP_BRUSHES], int nSize)
{
	while (nSize-- > 0)
		if (pList[nSize] == pFind)
			return true;

	return false;
}



/*
===============
UpdateWorkzone

update the workzone to a given brush
===============
*/
void UpdateWorkzone(Brush* b)
{
	if (!b) return;
	assert(b != &g_brSelectedBrushes);

	// will update the workzone to the given brush
	g_qeglobals.d_v3WorkMin = b->basis.mins;
	g_qeglobals.d_v3WorkMax = b->basis.maxs;
}


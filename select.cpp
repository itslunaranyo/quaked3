//==============================
//	select.c
//==============================

#include "qe3.h"

Brush	g_brSelectedBrushes;	// highlighted
//Face	*g_vfSelectedFaces[MAX_MAP_FACES];	// sikk - Multiple Face Selection
//int	g_nSelFaceCount;
bool	g_bSelectionChanged;

std::vector<Face*> Selection::faces;

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

	//SurfWnd_UpdateUI();
	Sys_UpdateWindows(W_SCENE|W_ENTITY|W_SURF);
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
int Selection::NumFaces()
{
	return faces.size();
}
bool Selection::IsEmpty()
{
	return !( HasBrushes() || faces.size() );
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
	if (faces.empty()) return false;

	faces.clear();
	//g_vfSelectedFaces[0] = NULL;
	//g_nSelFaceCount = 0;
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
	//for (int i = 0; i < g_nSelFaceCount; i++)
	//	if (face == g_vfSelectedFaces[i])
	//		return true;
	//return false;

	return (std::find(faces.begin(), faces.end(), face) != faces.end());
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
	//g_vfSelectedFaces[g_nSelFaceCount++] = f;
	//g_vfSelectedFaces[g_nSelFaceCount] = NULL;	// maintain list null-terminated
	faces.push_back(f);

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
	std::vector<Face*>::iterator fIdx;
	fIdx = std::find(faces.begin(), faces.end(), f);
	if (fIdx == faces.end())
		return false;

	faces.erase(fIdx);
	Selection::Changed();
	return true;

	/*
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
	*/
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

	for (auto fIt = faces.begin(); fIt != faces.end(); ++fIt)
	{
		if ((*fIt)->owner == b)
			sum++;
	}
	/*
	for (int i = 0; i < g_nSelFaceCount; i++)
	{
		if (g_vfSelectedFaces[i]->owner == b)
			sum++;
	}
	*/
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

	if (faces.empty())
		return;

	b = nullptr;
	for (auto fIt = faces.begin(); fIt != faces.end(); ++fIt)
	{
		if ((*fIt)->owner == b)
			continue;

		b = (*fIt)->owner;
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
		for (f = b->faces; f; f = f->fnext)
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

If the origin is inside a brush, that brush will be ignored

lunaran: now returns a modified version of flags based on what was hit first,
so paint-select can persistently select or deselect across the movement
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
			g_qeglobals.d_vTexture.ChooseTexture(&t.face->texdef);
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
			if (b->mins[i] < mins[i])
				mins[i] = b->mins[i];
			if (b->maxs[i] > maxs[i])
				maxs[i] = b->maxs[i];
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
	//	mid[i] = g_qeglobals.d_nGridSize * floor(((mins[i] + maxs[i]) * 0.5) / g_qeglobals.d_nGridSize);
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
	Brush	*b, *next;
	Face	*f;
	TexDef	*texdef;
	Texture	*txfind;

	if (!faces.empty())
		texdef = &(*faces.begin())->texdef;
	else
		texdef = &g_qeglobals.d_workTexDef;

	txfind = texdef->tex;

	DeselectAll();

	for (b = g_map.brActive.next; b != &g_map.brActive; b = next)
	{
		next = b->next;
		for (f = b->faces; f; f = f->fnext)
		{
			if (f->texdef.tex == txfind)
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

	mins = g_brSelectedBrushes.next->mins;
	maxs = g_brSelectedBrushes.next->maxs;
	Modify_Delete();

	// lunaran - grid view reunification
	{
		int nViewType;
		nViewType = QE_BestViewAxis();
		nDim1 = (nViewType == YZ) ? 1 : 0;
		nDim2 = (nViewType == XY) ? 1 : 2;
	}

	for (b = g_map.brActive.next; b != &g_map.brActive; b = next)
	{
		next = b->next;

		if ((b->maxs[nDim1] > maxs[nDim1] || b->mins[nDim1] < mins[nDim1]) ||
			(b->maxs[nDim2] > maxs[nDim2] || b->mins[nDim2] < mins[nDim2]))
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

	mins = g_brSelectedBrushes.next->mins;
	maxs = g_brSelectedBrushes.next->maxs;
	Modify_Delete();

	// lunaran - grid view reunification
	{
		int nViewType;
		nViewType = QE_BestViewAxis();
		nDim1 = (nViewType == YZ) ? 1 : 0;
		nDim2 = (nViewType == XY) ? 1 : 2;
	}

	for (b = g_map.brActive.next; b != &g_map.brActive; b = next)
	{
		next = b->next;

		if ((b->mins[nDim1] > maxs[nDim1] || b->maxs[nDim1] < mins[nDim1]) ||
			(b->mins[nDim2] > maxs[nDim2] || b->maxs[nDim2] < mins[nDim2]))
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

	mins = g_brSelectedBrushes.next->mins;
	maxs = g_brSelectedBrushes.next->maxs;

	for (b = g_map.brActive.next; b != &g_map.brActive; b = next)
	{
		next = b->next;
		for (i = 0; i < 3; i++)
			if (b->mins[i] > maxs[i] + 1 || b->maxs[i] < mins[i] - 1)
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

	mins = g_brSelectedBrushes.next->mins;
	maxs = g_brSelectedBrushes.next->maxs;
	Modify_Delete();

	for (b = g_map.brActive.next; b != &g_map.brActive; b = next)
	{
		next = b->next;
		for (i = 0; i < 3; i++)
			if (b->maxs[i] > maxs[i] || b->mins[i] < mins[i])
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
	g_qeglobals.d_v3WorkMin = b->mins;
	g_qeglobals.d_v3WorkMax = b->maxs;
}


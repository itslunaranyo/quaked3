//==============================
//	select.c
//==============================

#include "pre.h"
#include "qe3.h"
#include "select.h"
#include "map.h"
#include "TextureView.h"
#include "Tool.h"
#include "winding.h"
#include "modify.h"

Brush	g_brSelectedBrushes;	// highlighted
bool	g_bSelectionChanged;
select_t	Selection::g_selMode;
std::vector<Face*> Selection::faces;
vec3	Selection::g_vMins, Selection::g_vMaxs;

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

	vec3 vSize;

	if (HasBrushes())
	{
		CalcBounds();
		vSize = g_vMaxs - g_vMins;

		WndMain_Status(
			(_S("Selected: %s (%0)")
			<< g_brSelectedBrushes.Next()->owner->GetKeyValue("classname")
			<< vSize), 
			3);
	}
	else
	{
		WndMain_Status("", 3);
	}

	for (auto tIt = Tool::stack.begin(); tIt != Tool::stack.end(); ++tIt)
	{
		(*tIt)->SelectionChanged();
	}

	WndMain_UpdateWindows(W_SCENE|W_ENTITY|W_SURF);
	if (g_cfgUI.PathlineMode == TargetGraph::tgm_selected || 
		g_cfgUI.PathlineMode == TargetGraph::tgm_selected_path )
		WndMain_UpdateWindows(W_TARGETGRAPH);

	g_bSelectionChanged = false;
}

/*
================
interface functions
================
*/
bool Selection::HasBrushes()
{
	return (g_brSelectedBrushes.Next() && (g_brSelectedBrushes.Next() != &g_brSelectedBrushes));
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
	for (Brush* b = g_brSelectedBrushes.Next(); b != &g_brSelectedBrushes; b = b->Next())
		if (b->owner->IsBrush())
			return false;

	return true;
}
bool Selection::OnlyBrushEntities()
{
	for (Brush* b = g_brSelectedBrushes.Next(); b != &g_brSelectedBrushes; b = b->Next())
		if (b->owner->IsPoint())
			return false;

	return true;
}
bool Selection::OnlyWorldBrushes()
{
	for (Brush* b = g_brSelectedBrushes.Next(); b != &g_brSelectedBrushes; b = b->Next())
		if (!b->owner->IsWorld())
			return false;

	return true;
}
int Selection::NumBrushes()
{
	int i = 0;
	for (Brush* b = g_brSelectedBrushes.Next(); b != &g_brSelectedBrushes; b = b->Next()) i++;
	return i;
}
bool Selection::OneBrushEntity()
{
	Entity *first = nullptr;
	for (Brush* b = g_brSelectedBrushes.Next(); b != &g_brSelectedBrushes; b = b->Next())
	{
		if (b->owner->IsPoint() || b->owner->IsWorld())
			return false;
		if (!first)
			first = b->owner;
		else if (first != b->owner)
			return false;
	}

	return true;
}

/*
=================
Selection::IsBrushSelected
=================
*/
bool Selection::IsBrushSelected(const Brush* bSel)
{
	return bSel->IsOnList(g_brSelectedBrushes);
}

/*
=================
Selection::IsEntitySelected
=================
*/
bool Selection::IsEntitySelected(const Entity* eSel)
{
	Brush* b;

	for (b = g_brSelectedBrushes.Next(); (b != nullptr) && (b != &g_brSelectedBrushes); b = b->Next())
		if (b->owner == eSel)
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
	if (b->IsLinked())
		b->RemoveFromList();

	b->AddToList(g_brSelectedBrushes);
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
	if (b->IsLinked())
		b->RemoveFromList();

	// world brushes, point entities, and brush ents with only one brush go to the end 
	// of the list, so we don't keep looping over them looking for buddies
	if (b->owner->IsWorld() || b->owner->IsPoint() || b->ENext() == b)
	{
		//b->AddToList(g_brSelectedBrushes.prev);
		b->AddToList(g_brSelectedBrushes); // no, actually, add them up front because selection order matters
		Selection::Changed();
		return;
	}

	Brush* bCur;

	for (bCur = g_brSelectedBrushes.Next(); bCur != &g_brSelectedBrushes; bCur = bCur->Next())
	{
		if (bCur->owner == b->owner)
			break;
	}

	b->AddToList(*bCur);
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
			/*
			for (b = g_brSelectedBrushes.Next(); b != &g_brSelectedBrushes; b = b->Next())
			{
				if (b->owner != e) continue;

				// current entity is partially selected, select just this brush
				//SelectBrush(brush);
				brush->RemoveFromList();
				brush->AddToList(*b);	// add next to its partners to minimize entity fragmentation in the list
				Selection::Changed();
				return;
			}*/
			// current entity is not selected at all, select the whole thing
			for (b = e->brushes.ENext(); b != &e->brushes; b = b->ENext())
			{
				SelectBrushSorted(b);
			}
		}
		else
		{
			// select just this brush
			SelectBrushSorted(brush);
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
	g_selMode = sel_brush;

	Selection::Changed();
	return true;
}

/*
================
Selection::IsFaceSelected
================
*/
bool Selection::IsFaceSelected (const Face *face)
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
	if (!f->GetWinding())
		return;
	//g_vfSelectedFaces[g_nSelFaceCount++] = f;
	//g_vfSelectedFaces[g_nSelFaceCount] = NULL;	// maintain list null-terminated
	faces.push_back(f);

	Selection::Changed();
}

/*
================
Selection::SelectFaces
================
*/
void Selection::SelectFaces(Brush *b)
{
	for (Face *f = b->faces; f; f = f->fnext)
		if (!IsFaceSelected(f))
			faces.push_back(f);

	Selection::Changed();
}

/*
================
Selection::DeselectFace

returns true or false if face could be deselected or not
================
*/
bool Selection::DeselectFace(const Face* f)
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
================
Selection::DeselectFace

returns true or false if face could be deselected or not
================
*/
void Selection::DeselectFaces(const Brush *b)
{
	std::vector<Face*>::iterator fIdx;
	for (Face *f = b->faces; f; f = f->fnext)
	{
		fIdx = std::find(faces.begin(), faces.end(), f);
		if (fIdx == faces.end())
			continue;

		faces.erase(fIdx);
	}
	Selection::Changed();
}

/*
===============
Selection::NumBrushFacesSelected
===============
*/
int Selection::NumBrushFacesSelected(const Brush* b)
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
	g_selMode = sel_brush;
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

	for (b = g_brSelectedBrushes.Next(); b != &g_brSelectedBrushes; b = b->Next())
	{
		for (f = b->faces; f; f = f->fnext)
		{
			SelectFace(f);
		}
	}
	g_selMode = sel_face;
	g_brSelectedBrushes.MergeListIntoList(g_map.brActive);
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
//	if ((flags & SF_ENTITIES_FIRST) && brush->owner->IsWorld())
//		return false;
	if (flags & (SF_NOFIXEDSIZE | SF_FACES) && brush->owner->IsPoint())
		return false;
	if (brush->IsFiltered())
		return false;
	return true;
}

/*
===============
Selection::TestRay

two selection priorities along a ray: distance, and 'rank' (point/bmodel/world)
- point entities > brush entities > worldspawn
- distance is the higher priority unless SF_ENTITIES_FIRST
- return best trace unless SF_CYCLE, then return next best trace after the lowest already-selected brush on the ray
===============
*/
trace_t Selection::TestRay(const vec3 origin, const vec3 dir, int flags)
{
	Brush	*brush;
	Face	*face;
	trace_t bestP, bestB, bestW;
	float	dist, pMin = 0, bMin = 0, wMin = 0;

	const int DIST_START = g_cfgEditor.MapSize * 2;	// exceed longest diagonal

	bestP.dist = DIST_START;
	bestB.dist = DIST_START;
	bestW.dist = DIST_START;

	if (flags & SF_CYCLE && HasBrushes())
	{
		trace_t	lastS;

		for (brush = g_brSelectedBrushes.Next(); brush != &g_brSelectedBrushes; brush = brush->Next())
		{
			if (!Test_BrushFilter(brush, flags))
				continue;
			face = brush->RayTest(origin, dir, &dist);
			if (!face || !dist)
				continue;
			if (dist > lastS.dist)	// furthest
				lastS = { brush, face, dist, true };
		}

		if (lastS.brush)
		{
			if (flags & SF_ENTITIES_FIRST)
			{
				if (lastS.brush->owner->IsPoint())
				{
					pMin = lastS.dist;
				}
				else if (!lastS.brush->owner->IsWorld())
				{
					pMin = DIST_START;
					bMin = lastS.dist;
				}
				else
				{
					pMin = bMin = DIST_START;
					wMin = lastS.dist;
				}
			}
			else
				pMin = bMin = wMin = lastS.dist;
		}
	}

	// search unselected
	if (!(flags & SF_SELECTED_ONLY))
	{
		for (brush = g_map.brActive.Next(); brush != &g_map.brActive; brush = brush->Next())
		{
			if (!Test_BrushFilter(brush, flags))
				continue;
			face = brush->RayTest(origin, dir, &dist);
			if (!face || !dist)
				continue;

			if (brush->owner->IsPoint())
			{
				if (dist < bestP.dist && dist > pMin)
					bestP = { brush, face, dist, false };
			}
			else if (!brush->owner->IsWorld())
			{
				if (dist < bestB.dist && dist > bMin)
					bestB = { brush, face, dist, false };
			}
			else
			{
				if (dist < bestW.dist && dist > wMin)
					bestW = { brush, face, dist, false };
			}
		}
	}

	// search selection
	if ((flags & (SF_FACES|SF_CYCLE)) == 0)
	{
		for (brush = g_brSelectedBrushes.Next(); brush != &g_brSelectedBrushes; brush = brush->Next())
		{
			if (!Test_BrushFilter(brush, flags))
				continue;
			face = brush->RayTest(origin, dir, &dist);
			if (!face || !dist)
				continue;

			if (brush->owner->IsPoint())
			{
				if (dist < bestP.dist && dist > pMin)
					bestP = { brush, face, dist, true };
			}
			else if (!brush->owner->IsWorld())
			{
				if (dist < bestB.dist && dist > bMin)
					bestB = { brush, face, dist, true };
			}
			else
			{
				if (dist < bestW.dist && dist > wMin)
					bestW = { brush, face, dist, true };
			}
		}
	}

	if (flags & SF_ENTITIES_FIRST)
	{
		if (bestP.brush)
			return bestP;
		if (bestB.brush)
			return bestB;
		if (bestW.brush)
			return bestW;
	}
	else
	{
		if (bestP.dist < bestB.dist && bestP.dist < bestW.dist)
			return bestP;
		if (bestB.dist < bestW.dist)
			return bestB;
		if (bestW.dist < DIST_START)
			return bestW;
	}

	if (flags & SF_CYCLE)	// loop to beginning
		return TestRay(origin, dir, flags - SF_CYCLE);

	return bestW;
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

	// the EXPAND flag behaves in reverse, because it's only passed on a doubleclick, *after*
	// the first click has changed the selection status of what's now being doubleclicked

	out = flags;
	if (flags & SF_FACES)
	{
		g_selMode = sel_face;
		if (IsFaceSelected(t.face))
		{
			if (flags & SF_UNSELECTED) return 0;
			out |= SF_SELECTED;
			DeselectFace(t.face);
			if (flags & SF_EXPAND)
				SelectFaces(t.brush);
		}
		else
		{
			if (flags & SF_SELECTED) return 0;
			out |= SF_UNSELECTED;
			SelectFace(t.face);
			if (flags & SF_EXPAND)
				DeselectFaces(t.brush);
			else
				g_vTexture.ChooseTexture(&t.face->texdef);
		}
	}
	else
	{
		g_selMode = sel_brush;
		if (flags & SF_CYCLE || flags & SF_EXCLUSIVE)
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
			t.brush->AddToList(g_map.brActive);
			Selection::Changed();
			if (flags & SF_EXPAND)
				HandleBrush(t.brush, true);	
		}
		else
		{
			if (flags & SF_SELECTED) return 0;
			out |= SF_UNSELECTED;
			if (flags & SF_EXPAND)
			{
				Brush *brNext;
				for (Brush *b = g_brSelectedBrushes.Next(); b != &g_brSelectedBrushes; b = brNext)
				{
					brNext = b->Next();
					if (b->owner != t.brush->owner) continue;
					b->RemoveFromList();
					b->AddToList(g_map.brActive);
					Selection::Changed();
				}
			}
			else
				HandleBrush(t.brush, false);
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

	if (!(flags & SF_SELECTED_ONLY))
	{
		for (brush = g_map.brActive.Next(); brush != &g_map.brActive; brush = brush->Next())
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
		for (brush = g_brSelectedBrushes.Next(); brush != &g_brSelectedBrushes; brush = brush->Next())
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

	g_selMode = sel_brush;
	// move the brush to the other list
	if (t.selected)
	{
		t.brush->RemoveFromList();
		t.brush->AddToList(g_map.brActive);
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

	g_map.brActive.MergeListIntoList(g_brSelectedBrushes);

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

	for (b = g_brSelectedBrushes.Next(); b != &g_brSelectedBrushes; b = next)
	{
		next = b->Next();

		if (b->IsFiltered())
		{
			b->RemoveFromList();
			b->AddToList(g_map.brActive);
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
	DeselectAllFaces();

	g_selMode = sel_brush;

	if (!HasBrushes())
		return;

	QE_UpdateWorkzone(g_brSelectedBrushes.Next());

	g_brSelectedBrushes.MergeListIntoList(g_map.brActive);
	Selection::Changed();
}

/*
===============
Selection::CalcBounds
===============
*/
bool Selection::CalcBounds()
{
	ClearBounds(g_vMins, g_vMaxs);

	if (IsEmpty())
		return false;

	if (HasBrushes())
	{
		for (Brush *b = g_brSelectedBrushes.Next(); b != &g_brSelectedBrushes; b = b->Next())
		{
			g_vMins = glm::min(g_vMins, b->mins);
			g_vMaxs = glm::max(g_vMaxs, b->maxs);
		}
	}
	else if (NumFaces())
	{
		for (auto fIt = faces.begin(); fIt != faces.end(); ++fIt)
		{
			for (int i = 0; i < (*fIt)->GetWinding()->numpoints; i++)
			{
				g_vMins = glm::min(g_vMins, (*fIt)->GetWinding()->points[i].point);
				g_vMaxs = glm::max(g_vMaxs, (*fIt)->GetWinding()->points[i].point);
			}
		}
	}
	return true;
}

/*
===============
Selection::GetBounds
===============
*/
bool Selection::GetBounds(vec3& mins, vec3& maxs)
{
	if (IsEmpty())
		return false;

	mins = g_vMins;
	maxs = g_vMaxs;
	return true;
}

/*
===============
Selection::GetTrueMid
===============
*/
vec3 Selection::GetTrueMid()
{
	vec3	mins, maxs;

	GetBounds(mins, maxs);

	return (mins + maxs) * 0.5f;
}

/*
===============
Selection::GetMid
===============
*/
vec3 Selection::GetMid()
{
	vec3	mid;
	vec3	mins, maxs;

	if (!g_qeglobals.bGridSnap)
	{
		return GetTrueMid();
	}

	GetBounds(mins, maxs);

	// lunaran: don't snap the midpoint to the grid, snap the bounds first so selections don't wander when rotated
	// ... this didn't help
	mins = glm::floor(mins / (float)g_qeglobals.d_nGridSize) * (float)g_qeglobals.d_nGridSize;
	maxs = glm::ceil(maxs / (float)g_qeglobals.d_nGridSize) * (float)g_qeglobals.d_nGridSize;
	mid = glm::round(((mins + maxs) * 0.5f) / (float)g_qeglobals.d_nGridSize) * (float)g_qeglobals.d_nGridSize;

	return mid;
}



// sikk---> Select Matching Key/Value
/*
===============
Selection::MatchingKeyValue
===============
*/
void Selection::MatchingKeyValue(const char *szKey, const char *szValue)
{
	Brush   *b, *bnext;
	EPair	*ep;
	bool	bFound;
	int		selected;

	DeselectAll();
	selected = 0;

	if (strlen(szKey) && strlen(szValue))	// if both "key" & "value" are declared
	{
		for (b = g_map.brActive.Next(); b != &g_map.brActive; b = bnext)
		{
			bnext = b->Next();
			if (b->IsFiltered()) continue;
			if (b->owner->GetKeyValue(szKey) == szValue)
			{
				SelectBrushSorted(b);
				selected++;
			}
		}
	}
	else if (strlen(szKey))	// if only "key" is declared
	{
		for (b = g_map.brActive.Next(); b != &g_map.brActive; b = bnext)
		{
			bnext = b->Next();
			if (b->IsFiltered()) continue;

			if (!b->owner->GetKeyValue(szKey).empty())
			{
				SelectBrushSorted(b);
				selected++;
			}
		}
	}
	else if (strlen(szValue))	// if only "value" is declared
	{
		for (b = g_map.brActive.Next(); b != &g_map.brActive; b = bnext)
		{
			bnext = b->Next();
			if (b->IsFiltered()) continue;
			bFound = false;

			for (ep = b->owner->epairs; ep && !bFound; ep = ep->next)
			{
				if (ep->GetValue() == szValue)
				{
					SelectBrushSorted(b);
					selected++;
					bFound = true;	// this is so an entity with two different keys 
				}					// with identical values are not selected twice
			}
		}
	}
	Selection::Changed();
	if (selected)
		Log::Print(_S("Selected %i entities\n") << selected);
	else
		Log::Print("No matching unhidden entities found\n");
}
// <---sikk

/*
===============
Selection::MatchingTextures
===============
*/
void Selection::MatchingTextures(Texture *txfind)
{
	Brush	*b, *next;
	Face	*f;
	TexDef	*texdef;
	int selected = 0;

	if (txfind == nullptr)
	{
		if (!faces.empty())
			texdef = &(*faces.begin())->texdef;
		else
			texdef = &g_qeglobals.d_workTexDef;
		txfind = texdef->tex;
	}

	DeselectAll();

	for (b = g_map.brActive.Next(); b != &g_map.brActive; b = next)
	{
		next = b->Next();
		if (b->IsFiltered()) continue;
		for (f = b->faces; f; f = f->fnext)
		{
			if (f->texdef.tex == txfind)
			{
				SelectFace(f);
				selected++;
			}
		}
	}
	if (selected)
		Log::Print(_S("Selected %i faces with %s\n") << selected << txfind->name);
	else
		Log::Print(_S("No visible faces with %s found\n") << txfind->name);
}

/*
================
Selection::Invert
================
*/
void Selection::Invert()
{
	Brush temp;

	g_brSelectedBrushes.MergeListIntoList(temp);
	g_map.brActive.MergeListIntoList(g_brSelectedBrushes);
	temp.MergeListIntoList(g_map.brActive);

	/*
	Brush *next, *prev;

	//Log::Print("Inverting selection...\n");
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
	//Log::Print("Done.\n");
	*/
	Selection::Changed();
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

	selected = g_brSelectedBrushes.Next();
	// if nothing is selected, do nothing and return
	if (selected == &g_brSelectedBrushes)
		return;

	for (b = g_map.brActive.Next(); b != &g_map.brActive; b = next)
	{
		next = b->Next();

		if (b->owner->eclass->name == selected->owner->eclass->name)
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

	g_selMode = sel_brush;

	mins = g_brSelectedBrushes.Next()->mins;
	maxs = g_brSelectedBrushes.Next()->maxs;
	Modify::Delete();

	// lunaran - grid view reunification
	{
		int nViewType;
		nViewType = QE_BestViewAxis();
		nDim1 = (nViewType == GRID_YZ) ? 1 : 0;
		nDim2 = (nViewType == GRID_XY) ? 1 : 2;
	}

	for (b = g_map.brActive.Next(); b != &g_map.brActive; b = next)
	{
		next = b->Next();

		if (b->IsFiltered())
			continue;

		if ((b->maxs[nDim1] > maxs[nDim1] || b->mins[nDim1] < mins[nDim1]) ||
			(b->maxs[nDim2] > maxs[nDim2] || b->mins[nDim2] < mins[nDim2]))
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

	g_selMode = sel_brush;

	mins = g_brSelectedBrushes.Next()->mins;
	maxs = g_brSelectedBrushes.Next()->maxs;
	Modify::Delete();

	// lunaran - grid view reunification
	{
		int nViewType;
		nViewType = QE_BestViewAxis();
		nDim1 = (nViewType == GRID_YZ) ? 1 : 0;
		nDim2 = (nViewType == GRID_XY) ? 1 : 2;
	}

	for (b = g_map.brActive.Next(); b != &g_map.brActive; b = next)
	{
		next = b->Next();

		if (b->IsFiltered())
			continue;

		if ((b->mins[nDim1] > maxs[nDim1] || b->maxs[nDim1] < mins[nDim1]) ||
			(b->mins[nDim2] > maxs[nDim2] || b->maxs[nDim2] < mins[nDim2]))
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

	g_selMode = sel_brush;

	mins = g_brSelectedBrushes.Next()->mins;
	maxs = g_brSelectedBrushes.Next()->maxs;

	for (b = g_map.brActive.Next(); b != &g_map.brActive; b = next)
	{
		next = b->Next();
		if (b->IsFiltered())
			continue;

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

	g_selMode = sel_brush;

	mins = g_brSelectedBrushes.Next()->mins;
	maxs = g_brSelectedBrushes.Next()->maxs;
	Modify::Delete();

	for (b = g_map.brActive.Next(); b != &g_map.brActive; b = next)
	{
		next = b->Next();
		if (b->IsFiltered())
			continue;

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
	b = g_brSelectedBrushes.Next();
	if (b != &g_brSelectedBrushes)
	{
		if (!b->owner->IsWorld())
		{
			e = b->owner;
			DeselectAll();
			for (b2 = e->brushes.ENext(); b2 != &e->brushes; b2 = b2->ENext())
			{
				if (b == b2)
				{
					b2 = b2->ENext();
					break;
				}
			}
			if (b2 == &e->brushes)
				b2 = b2->ENext();

			HandleBrush(b2, false);
		}
	}
}




// ================================================================



//==============================
//	csg.c
//==============================

#include "pre.h"
#include "qe3.h"
#include "csg.h"
#include "map.h"
#include "select.h"
#include "winding.h"

#include "CmdAddRemove.h"
#include "CmdHollow.h"
#include "CmdMerge.h"
#include "CmdBridge.h"

#include <algorithm>

/*
====================
CSG::SplitBrushByFace

The incoming brush is NOT freed.
The incoming face is NOT left referenced.
====================
*/
void CSG::SplitBrushByFace (Brush *in, Face *f, Brush **front, Brush **back)
{
	Brush	*b;
	Face	*nf;
	vec3	temp;

	if (back)
	{
		b = in->Clone();
		nf = f->Clone();

		nf->texdef = b->faces->texdef;
		nf->fnext = b->faces;
		b->faces = nf;

		b->Build();
		b->RemoveEmptyFaces();

		if (!b->faces)
		{	// completely clipped away
			delete b;
			*back = nullptr;
		}
		else
		{
			*back = b;
		}
	}
	b = in->Clone();
	nf = f->Clone();

	// swap the plane winding
	nf->plane.Flip();

	nf->texdef = b->faces->texdef;
	nf->fnext = b->faces;
	b->faces = nf;

	b->Build();
	b->RemoveEmptyFaces();

	if (!b->faces)
	{	// completely clipped away
		delete b;
		*front = nullptr;
	}
	else
	{
		*front = b;
	}
}

/*
====================
CSG_DoSimpleMerge

simple face-pair merge for trying to reunify a brush after a single SplitBrushByFace op.
not used by user-facing merge, only for cleaning up after subtracting by multiple 
brushes at once.
====================
*/
Brush* CSG_DoSimpleMerge(Brush *a, Brush *b)
{
	Face *f1, *f2;
	std::vector<Face*> faces, skipfaces;

	for (f1 = a->faces; f1; f1 = f1->fnext)
	{
		for (f2 = b->faces; f2; f2 = f2->fnext)
		{
			if (!f1->plane.EqualTo(&f2->plane, true))
				continue;	// windings can't be equal if planes aren't

			if (Winding::WindingsEqual(f1->GetWinding(), f2->GetWinding(), true))
			{
				// face to face, ignore both
				skipfaces.push_back(f1);
				skipfaces.push_back(f2);
			}
		}
	}

	if (skipfaces.size() == 0)
		return nullptr;	// no common faces to glue together

	bool match;
	for (f1 = a->faces; f1; f1 = f1->fnext)
	{
		if (std::find(skipfaces.begin(), skipfaces.end(), f1) == skipfaces.end())
			faces.push_back(f1);
	}
	for (f2 = b->faces; f2; f2 = f2->fnext)
	{
		if (std::find(skipfaces.begin(), skipfaces.end(), f2) != skipfaces.end())
			continue;

		match = false;
		for (auto fIt = faces.begin(); fIt != faces.end(); ++fIt)
		{
			if ((*fIt)->plane.EqualTo(&f2->plane, false))
			{
				if ((*fIt)->texdef.tex != f2->texdef.tex)
					return nullptr;	// never merge together two planes with different textures
				match = true;
				break;
			}
		}
		if (!match)
			faces.push_back(f2);
	}

	Brush *out = new Brush();
	out->owner = a->owner;
	for (auto fIt = faces.begin(); fIt != faces.end(); ++fIt)
	{
		f1 = new Face(out, *fIt);
	}
	out->Build();
	return out;
}

Brush* CSG::DoMerge(std::vector<Brush*> &brList, bool notexmerge)
{
	Brush	*b;
	Face	*bf;
	std::vector<Face*> fList;

	for (auto brIt = brList.begin(); brIt != brList.end(); ++brIt)
	{
		b = *brIt;
		for (bf = b->faces; bf; bf = bf->fnext)
		{
			if (!bf->GetWinding())
				continue;
			fList.push_back(bf);
		}
	}

	return DoMerge(fList, notexmerge);
}

Brush* CSG::DoBridge(std::vector<Face*> &fList)
{
	return DoMerge(fList, false);
}

/*
==============
CSG::DoMerge

lunaran: a super-naive brute force convex hull solver
more than fast enough in the normal use case of merging only a handful of brushes
supercedes old CSG merge which was buggy and weird

notexmerge: if true, don't combine coincident planes if they have different textures
		if false, coincident planes' texdefs are overwritten with first one found
==============
*/
Brush* CSG::DoMerge(std::vector<Face*> &fList, bool notexmerge)
{
	Brush	*result;
	Face	*f;
	std::vector<dvec3> pointBuf;
	std::vector<Plane> planeBuf;
	std::vector<dvec3>::iterator curPoint;
	std::vector<Plane>::iterator curPlane;
	int		numPoints, numPlanes;
	int		pointBufSize;

	// put all input points in a buffer
	pointBufSize = 0;
	for (auto fIt = fList.begin(); fIt != fList.end(); ++fIt)
	{
		pointBufSize += (*fIt)->GetWinding()->numpoints;	// buffer size is very greedy
	}
	pointBuf.reserve(pointBufSize);

	for (auto fIt = fList.begin(); fIt != fList.end(); ++fIt)
	{
		for (int i = 0; i < (*fIt)->GetWinding()->numpoints; i++)
		{
			dvec3 dpt = (*fIt)->GetWinding()->points[i].point;
			// avoid duplicates in point buffer
			for (curPoint = pointBuf.begin(); curPoint != pointBuf.end(); ++curPoint)
			{
				dvec3 pdelta = dpt - *curPoint;
				if ((fabs(pdelta[0]) < 0.05) && (fabs(pdelta[1]) < 0.05) && (fabs(pdelta[2]) < 0.05))	// more aggressive point merging
				//if (VectorCompare(dpt, *curPoint))
				{
					// dealing with imprecision: keep whichever components are closer to an integer grid
					// point, and not just the first ones found. in nearly all cases, we want the healthy
					// integer grid points that haven't wandered around, and in cases where we're already 
					// dealing with significantly off-integer points we're already off the rails anyway
					if (fabs(dpt[0] - std::lround(dpt[0])) <= fabs((*curPoint)[0] - std::lround((*curPoint)[0])))
						(*curPoint)[0] = dpt[0];
					if (fabs(dpt[1] - std::lround(dpt[1])) <= fabs((*curPoint)[1] - std::lround((*curPoint)[1])))
						(*curPoint)[1] = dpt[1];
					if (fabs(dpt[2] - std::lround(dpt[2])) <= fabs((*curPoint)[2] - std::lround((*curPoint)[2])))
						(*curPoint)[2] = dpt[2];
					break;
				}
			}
			if (curPoint == pointBuf.end())
			{
				pointBuf.push_back(dpt);
			}
		}
	}
	numPoints = pointBuf.size();

	// find all planes defined by the outermost points
	numPlanes = numPoints * 2 - 4;
	planeBuf.reserve(numPlanes);
	result = new Brush();

	Plane	p;
	int		x, y, z, t;
	int		lCount, rCount, pl;
	double	dot;

	pl = 0;
	// for every unique trio of points
	for (x = 0; x < numPoints-2; x++) for (y = x+1; y < numPoints-1; y++) for (z = y+1; z < numPoints; z++)
	{
		int yf, zf;
		yf = y;
		zf = z;
		if (!p.FromPoints(pointBuf[x], pointBuf[y], pointBuf[z]))
			continue;

		lCount = rCount = 0;
		// compare all other points to the plane
		for (t = 0; t < numPoints; t++)
		{
			if (t == x || t == y || t == z) continue;
				
			dot = DotProduct(dvec3(pointBuf[t]), p.normal) - p.dist;
			// skip coplanar points
			if (dot < -ON_EPSILON) lCount++;
			if (dot > ON_EPSILON) rCount++;
		}
		// if all are on one side or the other, put plane in result buffer
		if (!lCount)
		{
			yf = z;
			zf = y;
			// redo the plane rather than negating it because signs might cancel
			p.FromPoints(pointBuf[x], pointBuf[yf], pointBuf[zf]);
		}
		if (!rCount || !lCount)
		{
			// avoid duplicates in plane buffer now since many collinear plane points could lead to a ton of duplicate planes
			for (curPlane = planeBuf.begin(); curPlane != planeBuf.end(); ++curPlane)
			{
				// FIXME: EqualTo is not precise enough, leads to refusal to merge
				if (p.EqualTo(&(*curPlane), false))
					break;
				assert(!p.EqualTo(&(*curPlane), true));	// there should be no equal opposite planes
			}

			if (curPlane == planeBuf.end())
			{
				planeBuf.push_back(p);

				// add a face to the brush now while we still have the plane
				f = new Face(result);
				f->plane = p;
			}
		}
	}
	numPlanes = planeBuf.size();

	if (numPlanes <= 3)
	{
		// input faces might have all been coplanar
		delete result;
		return nullptr;
	}

	//delete[] pointBuf;
	//delete[] planeBuf;

	for (f = result->faces; f; f = f->fnext)
	{
		// compare result planes to original brush planes, preserve texdefs from matching planes
		bool match = false;
		Texture* ftx = nullptr;
		for (auto fIt = fList.begin(); fIt != fList.end(); ++fIt)
		{
			if ((*fIt)->plane.EqualTo(&f->plane, false))
			{
				if (match && ftx != (*fIt)->texdef.tex)
				{
					// clashing textures on this plane when notexmerge was specified, abort merge
					delete result;
					return nullptr;
				}
				f->texdef = (*fIt)->texdef;
				ftx = f->texdef.tex = (*fIt)->texdef.tex;
				match = true;
				if (!notexmerge)
					break;
			}

			if (match && !notexmerge)
				break;
		}
		if (!match)
		{
			// apply workzone texdef to the rest
			f->texdef = g_qeglobals.d_workTexDef;
		}
	}

	result->owner = fList.front()->owner->owner;
	result->Build();

	// FIXME: only here for imprecision reasons
	// point on plane testing is still vague enough thanks to fp error that
	// sometimes we get empty faces that have been clipped away
	//result->RemoveEmptyFaces();
	for (Face *ft = result->faces; ft; ft = ft->fnext)
	{
		if (!ft->GetWinding())
		{
			delete result;
			return nullptr;
		}
	}

	return result;
}

/*
=============
CSG_CanMerge
=============
*/
bool CSG_CanMerge()
{
	Brush *b;
	Entity *owner;

	if (!Selection::HasBrushes())
	{
		Warning("No brushes selected.");
		return false;
	}

	if (g_brSelectedBrushes.Next()->Next() == &g_brSelectedBrushes)
	{
		Warning("At least two brushes must be selected.");
		return false;
	}

	owner = g_brSelectedBrushes.Next()->owner;

	for (b = g_brSelectedBrushes.Next(); b != &g_brSelectedBrushes; b = b->Next())
	{
		if (b->owner->IsPoint())
		{
			Warning("Cannot merge fixed size entities.");
			return false;
		}

		if (b->owner != owner)
		{
			Warning("Cannot merge brushes from different entities.");
			return false;
		}
	}
	return true;
}

/*
=============
CSG_CanBridge
=============
*/
bool CSG_CanBridge()
{
	if (!Selection::NumFaces())
	{
		Warning("No faces selected.");
		return false;
	}

	for (auto fIt = Selection::faces.begin(); fIt != Selection::faces.end(); ++fIt)
	{
		if ((*fIt)->owner->owner->IsPoint())
		{
			Warning("Cannot bridge fixed size entities.");
			return false;
		}
	}
	return true;
}

/*
====================
CSG::BrushMergeListPairs

Returns a list with merged brushes.
Tries to merge brushes pair wise.
The input list is destroyed.
Input and output should be a single linked list using .next
====================
*/
Brush *CSG::BrushMergeListPairs (Brush *brushlist, bool onlyshape)
{
	int		nummerges, merged;
	Brush	*a, *b, *newbrush;
	Brush	*bNext;

	if (!brushlist)
		return nullptr;

	nummerges = 0;

	Brush open, closed;
	open.AddToList(*brushlist);

	do
	{
		merged = 0;
		for (a = open.Next(); a != &open; a = open.Next())
		{
			for (b = a->Next(); b != &open; b = bNext)
			{
				bNext = b->Next();
				newbrush = CSG_DoSimpleMerge(a, b);
				if (newbrush)
				{
					a->RemoveFromList();
					b->RemoveFromList();
					newbrush->AddToListTail(open);
					merged++;
					nummerges++;
					break;
				}
			}

			// if a can't be merged with any of the other brushes
			if (b == &open)
			{
				a->RemoveFromList();
				a->AddToList(closed);
			}
		}
	} while (merged);
/*
	do
	{
		merged = 0;
		newbrushlist = nullptr;

		for (a = brushlist; a; a = brushlist)
		{
			bLast = a;
			for (b = a->Next(); b != a; b = b->Next())
			{
				newbrush = CSG_DoSimpleMerge(a, b);
				if (newbrush)
				{
					tail->next = newbrush;
					bLast->next = b->next;
					brushlist = brushlist->next;
					a->next = a->prev = nullptr;
					b->next = b->prev = nullptr;
					delete a;
					delete b;

					merged++;
					nummerges++;
					break;
				}
				bLast = b;
			}

			// if b1 can't be merged with any of the other brushes
			if (!b)
			{
				brushlist = brushlist->Next();
				// keep b1
				a->next = newbrushlist;
				newbrushlist = a;
			}
		}
		brushlist = newbrushlist;
	} while (merged);
	*/
	newbrush = closed.Next();
	closed.RemoveFromList();
	return newbrush;
}

/*
=============
CSG_BrushSubtract

Returns a list of brushes that remain after B is subtracted from A.
May by empty if A is contained inside B.
The originals are undisturbed.
=============
*/
Brush *CSG_BrushSubtract (Brush *a, Brush *b)
{
	// a - b = out (list)
	Brush	*front, *back;
	Brush	*in, *out, *next;
	Face	*f;

	for (int i = 0; i < 3; i++)
		if (b->mins[i] >= a->maxs[i] - ON_EPSILON || b->maxs[i] <= a->mins[i] + ON_EPSILON)
			return a;	// definately don't touch

	in = a;
	out = nullptr;

	for (f = b->faces; f && in; f = f->fnext)
	{
		CSG::SplitBrushByFace(in, f, &front, &back);
		if (in != a)
			delete in;
		if (front)
		{	// add to list
			if (!out)
				out = front;
			else
				front->AddToList(*out);
			//front->owner->LinkBrush(front);
		}
		//back->owner->LinkBrush(back);
		in = back;
	}

	// NOTE: in != a just in case brush b has no faces
	if (in && in != a)
		delete in;
	else
	{	// didn't really intersect
		for (b = out; b; b = next)
		{
			next = b->Next();
			//b->next = b->prev = nullptr;
			delete b;
		}
		return a;
	}
	return out;
}

/*
=============
CSG::Subtract
=============
*/
void CSG::Subtract ()
{
	int		numfragments, numbrushes;
	Brush	m_fragList;
	Brush	*b, *s, *fragments, *nextfragment, *frag, *next, *snext;

	std::vector<Brush*> brCarved;
	CmdAddRemove *cmdAR = new CmdAddRemove();

	Sys_Printf("CSG Subtracting...\n");

	if (!Selection::HasBrushes())
	{
		Warning("No brushes selected.");
		return;
	}

	numfragments = 0;
	numbrushes = 0;
	for (b = g_brSelectedBrushes.Next(); b != &g_brSelectedBrushes; b=next)
	{
		next = b->Next();

		if (b->owner->IsPoint())
			continue;	// can't use texture from a fixed entity, so don't subtract

		// chop all fragments further up
		for (s = m_fragList.Next(); s != &m_fragList; s = snext)
		{
			snext = s->Next();

			fragments = CSG_BrushSubtract(s, b);

			// if the brushes did not really intersect
			if (fragments == s)
				continue;

			// try to merge fragments
			fragments = CSG::BrushMergeListPairs(fragments, true);

			// add the fragments to the list
			while (fragments->Next() != fragments)
			{
				frag = fragments->Next();
				frag->owner = s->owner;
			}
			fragments->MergeListIntoList(m_fragList);
			fragments->AddToList(m_fragList);

			// free the original brush
			delete s;
		}

		// chop any active brushes up
		for (s = g_map.brActive.Next(); s != &g_map.brActive; s = snext)
		{
			snext = s->Next();

			if (s->owner->IsPoint() || s->IsFiltered())
				continue;
			if (std::find(brCarved.begin(), brCarved.end(), s) != brCarved.end())
				continue;	// is already in fragments

			fragments = CSG_BrushSubtract(s, b);

			// if the brushes did not really intersect
			if (fragments == s)
				continue;

			// one extra brush chopped up
			numbrushes++;

			// try to merge fragments
			fragments = CSG::BrushMergeListPairs(fragments, true);

			// add the fragments to the list
			frag = fragments;
			do {
				frag->owner = s->owner;
				frag = frag->Next();
			}
			while (frag != fragments);
			fragments->MergeListIntoList(m_fragList);
			fragments->AddToList(m_fragList);

			// free the original brush
			brCarved.push_back(s);
		}
	}
	cmdAR->RemovedBrushes(brCarved);

	// move all fragments to the active brush list
	for (frag = m_fragList.Next(); frag != &m_fragList; frag = nextfragment)
	{
		nextfragment = frag->Next();
		numfragments++;
		frag->RemoveFromList();
		cmdAR->AddedBrush(frag);
	}

	if (numfragments == 0)
	{
		Warning("Selected brush%s did not intersect with any other brushes.",
				   (g_brSelectedBrushes.Next()->Next() == &g_brSelectedBrushes) ? "":"es");
		delete cmdAR;
		return;
	}

	g_cmdQueue.Complete(cmdAR);

	Sys_Printf("Done. (created %d fragment%s out of %d brush%s)\n", 
				numfragments, (numfragments == 1) ? "" : "s",
				numbrushes, (numbrushes == 1) ? "" : "es");
	WndMain_UpdateWindows(W_SCENE);
}

/*
==============
CSG::Hollow
==============
*/
void CSG::Hollow()
{
	Sys_Printf("CSG Hollowing...\n");

	if (!Selection::HasBrushes())
	{
		Warning("No brushes selected.");
		return;
	}

	CmdHollow *cmdH = new CmdHollow();
	cmdH->UseBrushes(&g_brSelectedBrushes);
	g_cmdQueue.Complete(cmdH);
	//cmdH->Select();

	WndMain_UpdateWindows(W_SCENE);
}

/*
=============
CSG::Merge
=============
*/
void CSG::Merge ()
{
	if (Selection::NumFaces() > 0)
	{
		Bridge();
		return;
	}

	Sys_Printf("CSG Merging...\n");

	if (!CSG_CanMerge())
		return;

	CmdMerge *cmdM = new CmdMerge();
	cmdM->UseBrushes(&g_brSelectedBrushes);
	g_cmdQueue.Complete(cmdM);
	//cmdM->Select();

	Sys_Printf("Merge done.\n");
	WndMain_UpdateWindows(W_SCENE);
}

/*
=============
CSG::Bridge
=============
*/
void CSG::Bridge()
{
	Sys_Printf("CSG Bridging...\n");

	if (!CSG_CanBridge())
		return;

	CmdBridge *cmdB = new CmdBridge();
	cmdB->UseFaces(Selection::faces);
	g_cmdQueue.Complete(cmdB);

	Sys_Printf("Bridge done.\n");
	WndMain_UpdateWindows(W_SCENE);
}
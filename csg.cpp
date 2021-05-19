//==============================
//	csg.c
//==============================

#include "qe3.h"
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

		nf->texdef = b->basis.faces->texdef;
		nf->fnext = b->basis.faces;
		b->basis.faces = nf;

		b->Build();
		b->RemoveEmptyFaces();

		if (!b->basis.faces)
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
	temp = nf->planepts[0];
	nf->planepts[0] = nf->planepts[1];
	nf->planepts[1] = temp;

	nf->texdef = b->basis.faces->texdef;
	nf->fnext = b->basis.faces;
	b->basis.faces = nf;

	b->Build();
	b->RemoveEmptyFaces();

	if (!b->basis.faces)
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

	for (f1 = a->basis.faces; f1; f1 = f1->fnext)
	{
		for (f2 = b->basis.faces; f2; f2 = f2->fnext)
		{
			if (!f1->plane.EqualTo(&f2->plane, true))
				continue;	// windings can't be equal if planes aren't

			if (Winding::WindingsEqual(f1->face_winding, f2->face_winding, true))
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
	for (f1 = a->basis.faces; f1; f1 = f1->fnext)
	{
		if (std::find(skipfaces.begin(), skipfaces.end(), f1) == skipfaces.end())
			faces.push_back(f1);
	}
	for (f2 = b->basis.faces; f2; f2 = f2->fnext)
	{
		if (std::find(skipfaces.begin(), skipfaces.end(), f2) != skipfaces.end())
			continue;

		match = false;
		for (auto fIt = faces.begin(); fIt != faces.end(); ++fIt)
		{
			if ((*fIt)->plane.EqualTo(&f2->plane, false))
			{
				if ((*fIt)->d_texture != f2->d_texture)
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

/*
==============
CSG::DoMerge

lunaran: a super-naive brute force convex hull solver
more than fast enough in the normal use case of merging only a handful of brushes
supercedes old CSG merge which was buggy and weird

notexmerge: if true, don't combine coincident planes if they have different textures
		if false, coincident planes' texdefs are overwritten with first one found
convex: if true, always merge any input brushes and create new planes to force convexity
		if false, don't merge the input brushes if their union wouldn't be convex
==============
*/
Brush* CSG::DoMerge(std::vector<Brush*> &brList, bool notexmerge)// , bool convex)
{
	Brush	*result;
	Brush	*b;
	Face	*f, *bf;
	vec3	*pointBuf;
	Plane	*planeBuf;
	int		numPoints, numPlanes;
	vec3	*nextPoint, *curPoint;
	int		pointBufSize;

	// put all input points in a buffer
	pointBufSize = 0;
	for (auto brIt = brList.begin(); brIt != brList.end(); ++brIt)
	{
		b = *brIt;
		for (bf = b->basis.faces; bf; bf = bf->fnext)
			pointBufSize += bf->face_winding->numpoints;	// buffer size is very greedy
	}
	pointBuf = new vec3[pointBufSize]();// (vec3*)qmalloc(sizeof(vec3) * pointBufSize);

	numPoints = 0;
	nextPoint = pointBuf;
	for (auto brIt = brList.begin(); brIt != brList.end(); ++brIt)
	{
		b = *brIt;
		for (bf = b->basis.faces; bf; bf = bf->fnext)
		{
			for (int i = 0; i < bf->face_winding->numpoints; i++)
			{
				curPoint = pointBuf;
				// avoid duplicates in point buffer
				while (curPoint != nextPoint)
				{
					if (VectorCompare(bf->face_winding->points[i].point, *curPoint))	// skip xyzst
						break;
					curPoint++;
				}
				if (curPoint == nextPoint)
				{
					*nextPoint = bf->face_winding->points[i].point;
					nextPoint++;
					numPoints++;
				}
			}
		}
	}

	// find all planes defined by the outermost points
	numPlanes = numPoints * 2 - 4;
	planeBuf = new Plane[numPlanes]();// (Plane*)qmalloc(sizeof(Plane) * numPlanes);
	result = new Brush();

	Plane	p;
	int		x, y, z, t;
	int		lCount, rCount, pl;
	float	dot;
	vec3	*pT;

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
				
			pT = &pointBuf[t];
			dot = DotProduct(*pT, p.normal) - p.dist;
			// skip coplanar points
			if (dot < 0) lCount++;
			if (dot > 0) rCount++;
		}
		// if all are on one side or the other, put plane in result buffer
		if (!rCount)
		{
			yf = z;
			zf = y;
			// redo the plane rather than negating it because signs might cancel
			p.FromPoints(pointBuf[x], pointBuf[yf], pointBuf[zf]);
		}
		if (!rCount || !lCount)
		{
			// avoid duplicates in plane buffer now since many collinear plane points could lead to a ton of duplicate planes
			int pN;
			for (pN = 0; pN < pl; pN++)
				if (p.EqualTo(&planeBuf[pN], false))
					break;

			if (pN == pl)
			{
				planeBuf[pl] = p;
				pl++;

				// add a face to the brush now while we still have the plane points
				f = new Face(result);
				f->plane = p;
				for (int j = 0; j < 3; j++)
				{
					f->planepts[0] = pointBuf[x];
					f->planepts[1] = pointBuf[yf];
					f->planepts[2] = pointBuf[zf];
				}
			}
		}
	}
	numPlanes = pl;

	assert(numPlanes > 3);
	//free(pointBuf);
	//free(planeBuf);
	delete pointBuf;
	delete planeBuf;

	for (f = result->basis.faces; f; f = f->fnext)
	{
		// compare result planes to original brush planes, preserve texdefs from matching planes
		bool match = false;
		Texture* ftx = nullptr;
		for (auto brIt = brList.begin(); brIt != brList.end(); ++brIt)
		{
			b = *brIt;
			for (bf = b->basis.faces; bf; bf = bf->fnext)
			{
				if (bf->plane.EqualTo(&f->plane, true))
				{
					if (match && ftx != bf->d_texture)
					{
						// clashing textures on this plane when notexmerge was specified, abort merge
						delete result;
						return nullptr;
					}
					f->texdef = bf->texdef;
					ftx = f->d_texture = bf->d_texture;
					match = true;
					if (!notexmerge)
						break;
				}
			}
			if (match && !notexmerge)
				break;
		}
		if (!match)
		{
		/*	if (!convex)
			{
				// a concave-only merge wouldn't create any new planes
				delete result;
				return nullptr;
			}*/
			// apply workzone texdef to the rest
			f->texdef = g_qeglobals.d_workTexDef;
			f->d_texture = Textures::ForName(f->texdef.name);
		}
	}

	result->owner = brList.front()->owner;
	result->Build();

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

	if (!Select_HasBrushes())
	{
		Sys_Printf("WARNING: No brushes selected.\n");
		return false;
	}

	if (g_brSelectedBrushes.next->next == &g_brSelectedBrushes)
	{
		Sys_Printf("WARNING: At least two brushes must be selected.\n");
		return false;
	}

	owner = g_brSelectedBrushes.next->owner;

	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		if (b->owner->IsPoint())
		{
			Sys_Printf("WARNING: Cannot add fixed size entities.\n");
			return false;
		}

		if (b->owner != owner)
		{
			Sys_Printf("WARNING: Cannot add brushes from different entities.\n");
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
	Brush	*b1, *b2, *tail, *newbrush, *newbrushlist;
	Brush	*lastb2;

	if (!brushlist)
		return nullptr;

	nummerges = 0;

	do
	{
		for (tail = brushlist; tail; tail = tail->next)
			if (!tail->next) 
				break;

		merged = 0;
		newbrushlist = nullptr;

		for (b1 = brushlist; b1; b1 = brushlist)
		{
			lastb2 = b1;
			for (b2 = b1->next; b2; b2 = b2->next)
			{
				newbrush = CSG_DoSimpleMerge(b1, b2);
				if (newbrush)
				{
					tail->next = newbrush;
					lastb2->next = b2->next;
					brushlist = brushlist->next;
					b1->next = b1->prev = nullptr;
					b2->next = b2->prev = nullptr;
					delete b1;
					delete b2;

					for (tail = brushlist; tail; tail = tail->next)
						if (!tail->next) 
							break;

					merged++;
					nummerges++;
					break;
				}
				lastb2 = b2;
			}

			// if b1 can't be merged with any of the other brushes
			if (!b2)
			{
				brushlist = brushlist->next;
				// keep b1
				b1->next = newbrushlist;
				newbrushlist = b1;
			}
		}
		brushlist = newbrushlist;
	} while (merged);

	return newbrushlist;
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
		if (b->basis.mins[i] >= a->basis.maxs[i] - ON_EPSILON || b->basis.maxs[i] <= a->basis.mins[i] + ON_EPSILON)
			return a;	// definately don't touch

	in = a;
	out = nullptr;

	for (f = b->basis.faces; f && in; f = f->fnext)
	{
		CSG::SplitBrushByFace(in, f, &front, &back);
		if (in != a)
			delete in;
		if (front)
		{	// add to list
			front->next = out;
			out = front;
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
			next = b->next;
			b->next = b->prev = nullptr;
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
	Brush	fragmentlist;
	Brush	*b, *s, *fragments, *nextfragment, *frag, *next, *snext;

	std::vector<Brush*> brCarved;
	CmdAddRemove *cmdAR = new CmdAddRemove();

	Sys_Printf("CMD: CSG Subtracting...\n");

	if (!Select_HasBrushes())
	{
		Sys_Printf("WARNING: No brushes selected.\n");
		return;
	}

	fragmentlist.CloseLinks();

	numfragments = 0;
	numbrushes = 0;
	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b=next)
	{
		next = b->next;

		if (b->owner->IsPoint())
			continue;	// can't use texture from a fixed entity, so don't subtract

		// chop all fragments further up
		for (s = fragmentlist.next; s != &fragmentlist; s = snext)
		{
			snext = s->next;

			fragments = CSG_BrushSubtract(s, b);

			// if the brushes did not really intersect
			if (fragments == s)
				continue;

			// try to merge fragments
			fragments = CSG::BrushMergeListPairs(fragments, true);

			// add the fragments to the list
			for (frag = fragments; frag; frag = nextfragment)
			{
				nextfragment = frag->next;
				frag->next = nullptr;
				frag->owner = s->owner;
				frag->AddToList(&fragmentlist);
			}

			// free the original brush
			delete s;
		}

		// chop any active brushes up
		for (s = g_map.brActive.next; s != &g_map.brActive; s = snext)
		{
			snext = s->next;

			if (s->owner->IsPoint() || s->hiddenBrush)
				continue;
			if (std::find(brCarved.begin(), brCarved.end(), s) != brCarved.end())
				continue;	// is already in fragments

			fragments = CSG_BrushSubtract(s, b);

			// if the brushes did not really intersect
			if (fragments == s)
				continue;

			//Undo::AddBrush(s);	// sikk - Undo/Redo
			//cmdAR->RemovedBrush(s);

			// one extra brush chopped up
			numbrushes++;

			// try to merge fragments
			fragments = CSG::BrushMergeListPairs(fragments, true);

			// add the fragments to the list
			for (frag = fragments; frag; frag = nextfragment)
			{
				nextfragment = frag->next;
				frag->next = nullptr;
				frag->owner = s->owner;
				frag->AddToList(&fragmentlist);
			}

			// free the original brush
			brCarved.push_back(s);
			//delete s;
		}
	}
	cmdAR->RemovedBrushes(brCarved);

	// move all fragments to the active brush list
	for (frag = fragmentlist.next; frag != &fragmentlist; frag = nextfragment)
	{
		nextfragment = frag->next;
		numfragments++;
		frag->RemoveFromList();
		//frag->AddToList( &g_map.brActive);
		//Undo::EndBrush(frag);	// sikk - Undo/Redo
		cmdAR->AddedBrush(frag);
	}

	if (numfragments == 0)
	{
		Sys_Printf("WARNING: Selected brush%s did not intersect with any other brushes.\n",
				   (g_brSelectedBrushes.next->next == &g_brSelectedBrushes) ? "":"es");
		delete cmdAR;
		return;
	}

	g_cmdQueue.Complete(cmdAR);

	Sys_Printf("MSG: Done. (created %d fragment%s out of %d brush%s)\n", 
				numfragments, (numfragments == 1) ? "" : "s",
				numbrushes, (numbrushes == 1) ? "" : "es");
	Sys_UpdateWindows(W_SCENE);
}

/*
==============
CSG::Hollow
==============
*/
void CSG::Hollow()
{
	Sys_Printf("CMD: CSG Hollowing...\n");

	if (!Select_HasBrushes())
	{
		Sys_Printf("WARNING: No brushes selected.\n");
		return;
	}

	CmdHollow *cmdH = new CmdHollow();
	cmdH->UseBrushes(&g_brSelectedBrushes);
	g_cmdQueue.Complete(cmdH);
	cmdH->Select();

	Sys_UpdateWindows(W_SCENE);
}

/*
=============
CSG::Merge
=============
*/
void CSG::Merge ()
{
	Sys_Printf("CMD: CSG Merging...\n");

	if (!CSG_CanMerge())
		return;

	CmdMerge *cmdM = new CmdMerge();
	cmdM->UseBrushes(&g_brSelectedBrushes);
	g_cmdQueue.Complete(cmdM);
	cmdM->Select();

	Sys_Printf("MSG: Merge done.\n");
	Sys_UpdateWindows(W_SCENE);
}
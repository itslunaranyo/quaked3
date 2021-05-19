//==============================
//	csg.c
//==============================

#include "qe3.h"


/*
====================
CSG_SplitBrushByFace

The incoming brush is NOT freed.
The incoming face is NOT left referenced.
====================
*/
void CSG_SplitBrushByFace (Brush *in, Face *f, Brush **front, Brush **back)
{
	Brush	   *b;
	Face	   *nf;
	vec3_t		temp;

	b = in->Clone();
	nf = f->Clone();

	nf->texdef = b->brush_faces->texdef;
	nf->next = b->brush_faces;
	b->brush_faces = nf;

	b->Build();
	b->RemoveEmptyFaces();

	if (!b->brush_faces)
	{	// completely clipped away
		delete b;
		*back = NULL;
	}
	else
	{
		in->owner->LinkBrush(b);
		*back = b;
	}

	b = in->Clone();
	nf = f->Clone();

	// swap the plane winding
	VectorCopy(nf->planepts[0], temp);
	VectorCopy(nf->planepts[1], nf->planepts[0]);
	VectorCopy(temp, nf->planepts[1]);

	nf->texdef = b->brush_faces->texdef;
	nf->next = b->brush_faces;
	b->brush_faces = nf;

	b->Build();
	b->RemoveEmptyFaces();

	if (!b->brush_faces)
	{	// completely clipped away
		delete b;
		*front = NULL;
	}
	else
	{
		in->owner->LinkBrush(b);
		*front = b;
	}
}

/*
==============
CSG_Hollow
==============
*/
void CSG_Hollow ()
{
	int			i;
	Brush	   *b, *front, *back, *next;
	Face	   *f;
	Face		split;
	vec3_t		move;

	Sys_Printf("CMD: CSG Hollowing...\n");

	if (!Select_HasBrushes())
	{
		Sys_Printf("WARNING: No brushes selected.\n");
		return;
	}

	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = next)
	{
		next = b->next;

		if (b->owner->eclass->IsFixedSize() || b->hiddenBrush)
			continue;

		for (f = b->brush_faces; f; f = f->next)
		{
			split = *f;
			VectorScale(f->plane.normal, g_qeglobals.d_nGridSize, move);
			for (i = 0; i < 3; i++)
				VectorSubtract(split.planepts[i], move, split.planepts[i]);

			CSG_SplitBrushByFace(b, &split, &front, &back);

			if (back)
				delete back;
			if (front)
				front->AddToList(&g_brSelectedBrushes);
		}
		delete b;
	}
	Sys_UpdateWindows(W_ALL);
}

/*
==============
Brush_ConvexMerge

lunaran: a super-naive brute force convex hull solver

this is more than fast enough in the normal use case of merging only a handful of
brushes, and only gets n^2 slow with super-large merges.
something like quickHull would absolutely be faster in those cases, but the extra 
gymnastics wouldn't be worth writing.
==============
*/
Brush* Brush_ConvexMerge(Brush *bList)
{
	Brush		*result;
	Brush		*b;
	Face		*f, *bf;
	vec_t		*pointBuf;
	Plane		*planeBuf;
	int			numPoints, numPlanes;

	// put all input points in a buffer
	{
		vec_t	*nextPoint, *curPoint;
		int		pointBufSize;

		pointBufSize = 0;
		for (b = bList; b; b = b->next) for (bf = b->brush_faces; bf; bf = bf->next)
			pointBufSize += bf->face_winding->numpoints;
		pointBuf = (vec_t*)qmalloc(sizeof(vec_t) * 3 * pointBufSize);

		numPoints = 0;
		nextPoint = pointBuf;
		for (b = bList; b; b = b->next) for (bf = b->brush_faces; bf; bf = bf->next)
		{
			for (int i = 0; i < bf->face_winding->numpoints; i++)
			{
				curPoint = pointBuf;
				// avoid duplicates in point buffer
				while (curPoint != nextPoint)
				{
					if (VectorCompare(bf->face_winding->points[i], curPoint))	// skip xyzst
						break;
					curPoint += 3;
				}
				if (curPoint == nextPoint)
				{
					VectorCopy(bf->face_winding->points[i], nextPoint);
					nextPoint += 3;
					numPoints++;
				}
			}
		}
	}

	numPlanes = numPoints * 2 - 4;
	planeBuf = (Plane*)qmalloc(sizeof(Plane) * numPlanes);
	result = new Brush();

	// find all planes defined by the outermost points
	{
		Plane	p;
		int		x, y, z, t;
		int		lCount, rCount, pl;
		float	dot;
	//	vec3_t	dp;
		vec_t	*pT;

		pl = 0;
		// for every unique trio of points
		for (x = 0; x < numPoints-2; x++) for (y = x+1; y < numPoints-1; y++) for (z = y+1; z < numPoints; z++)
		{
			int yf, zf;
			yf = y;
			zf = z;
			if (!p.FromPoints(&pointBuf[x * 3], &pointBuf[y * 3], &pointBuf[z * 3]))
				continue;

			lCount = rCount = 0;
			// compare all other points to the plane
			for (t = 0; t < numPoints; t++)
			{
				if (t == x || t == y || t == z) continue;
				
				pT = &pointBuf[t * 3];
				dot = DotProduct(pT, p.normal) - p.dist;
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
				p.FromPoints(&pointBuf[x * 3], &pointBuf[yf * 3], &pointBuf[zf * 3]);
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
						f->planepts[0][j] = pointBuf[x * 3 + j];
						f->planepts[1][j] = pointBuf[yf * 3 + j];
						f->planepts[2][j] = pointBuf[zf * 3 + j];
					}
				}
			}
		}
		numPlanes = pl;
	}

	assert(numPlanes > 3);
	free(pointBuf);
	free(planeBuf);

	for (f = result->brush_faces; f; f = f->next)
	{
		// compare result planes to original brush planes, preserve texdefs from matching planes
		bool match = false;
		for (b = bList; b; b = b->next)
		{
			for (bf = b->brush_faces; bf; bf = bf->next)
			{
				if (bf->plane.EqualTo(&f->plane, true))
				{
					f->texdef = bf->texdef;
					f->d_texture = bf->d_texture;
					match = true;
					break;
				}
			}
			if (match) break;
		}
		if (!match)
		{
			// apply workzone texdef to the rest
			f->texdef = g_qeglobals.d_workTexDef;
			f->d_texture = Textures::ForName(f->texdef.name);
		}
	}

	bList->owner->LinkBrush(result);
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
		if (b->owner->eclass->IsFixedSize())
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
=============
CSG_ConvexMerge
=============
*/
void CSG_ConvexMerge()
{
	Brush* bListStart, *b, *next;

	Sys_Printf("CMD: CSG Convex Merging...\n");

	if (!CSG_CanMerge())
		return;

	bListStart = NULL;
	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = next)
	{
		next = b->next;

		b->RemoveFromList();
		b->next = bListStart;
		b->prev = NULL;
		bListStart = b;
	}

	b = Brush_ConvexMerge(bListStart);
	Select_SelectBrush(b);
	// free the original brushes
	for (b = bListStart; b; b = next)
	{
		next = b->next;
		b->next = NULL;
		b->prev = NULL;
		delete b;
	}

	Sys_Printf("MSG: Merge done.\n");
}

/*
=============
Brush_Merge

Returns a new brush that is created by merging brush1 and brush2.
May return NULL if brush1 and brush2 do not create a convex brush when merged.
The input brushes brush1 and brush2 stay intact.

If onlyshape is true then the merge is allowed based on the shape only
otherwise the texture references of faces in the same plane have to
be the same as well.
=============
*/
Brush *Brush_Merge (Brush *brush1, Brush *brush2, int onlyshape)
{
	int			i, shared;
	Brush    *newbrush;
	Face     *face1, *face2, *newface, *f;

	// check for bounding box overlapp
	for (i = 0; i < 3; i++)
	{
		if (brush1->mins[i] > brush2->maxs[i] + ON_EPSILON || 
			brush1->maxs[i] < brush2->mins[i] - ON_EPSILON)
		{
			// never merge if the brushes overlap
			return NULL;
		}
	}

	shared = 0;
	// check if the new brush would be convex... flipped planes make a brush non-convex
	for (face1 = brush1->brush_faces; face1; face1 = face1->next)
	{
		// don't check the faces of brush 1 and 2 touching each other
		for (face2 = brush2->brush_faces; face2; face2 = face2->next)
		{
			if (face1->plane.EqualTo(&face2->plane, true))
			{
				shared++;
				// there may only be ONE shared side
				if (shared > 1)
					return NULL;
				break;
			}
		}
		// if this face plane is shared
		if (face2) 
			continue;

		for (face2 = brush2->brush_faces; face2; face2 = face2->next)
		{
			// don't check the faces of brush 1 and 2 touching each other
			for (f = brush1->brush_faces; f; f = f->next)
				if (face2->plane.EqualTo(&f->plane, true))
					break;

			if (f)
				continue;

			if (face1->plane.EqualTo(&face2->plane, false))
			{
				//if the texture references should be the same but are not
				if (!onlyshape && _stricmp(face1->texdef.name, face2->texdef.name) != 0) 
					return NULL;
				continue;
			}

			if (Winding::PlanesConcave(face1->face_winding, face2->face_winding,	
									  face1->plane.normal, face2->plane.normal,
									  face1->plane.dist, face2->plane.dist))
				return NULL;
		}
	}

	newbrush = new Brush();

	for (face1 = brush1->brush_faces; face1; face1 = face1->next)
	{
		// don't add the faces of brush 1 and 2 touching each other
		for (face2 = brush2->brush_faces; face2; face2 = face2->next)
			if (face1->plane.EqualTo(&face2->plane, true))
				break;

		if (face2)
			continue;

		// don't add faces with the same plane twice
		for (f = newbrush->brush_faces; f; f = f->next)
		{
			if (face1->plane.EqualTo(&f->plane, false))
				break;
			if (face1->plane.EqualTo(&f->plane, true))
				break;
		}

		if (f)
			continue;

		newface = new Face(newbrush);
		newface->texdef = face1->texdef;
		VectorCopy(face1->planepts[0], newface->planepts[0]);
		VectorCopy(face1->planepts[1], newface->planepts[1]);
		VectorCopy(face1->planepts[2], newface->planepts[2]);
		newface->plane = face1->plane;
	}

	for (face2 = brush2->brush_faces; face2; face2 = face2->next)
	{
		// don't add the faces of brush 1 and 2 touching each other
		for (face1 = brush1->brush_faces; face1; face1 = face1->next)
			if (face2->plane.EqualTo(&face1->plane, true))
				break;

		if (face1)
			continue;

		// don't add faces with the same plane twice
		for (f = newbrush->brush_faces; f; f = f->next)
		{
			if (face2->plane.EqualTo(&f->plane, false))
				break;
			if (face2->plane.EqualTo(&f->plane, true))
				break;
		}

		if (f)
			continue;

		newface = new Face(newbrush);
		newface->texdef = face2->texdef;
		VectorCopy(face2->planepts[0], newface->planepts[0]);
		VectorCopy(face2->planepts[1], newface->planepts[1]);
		VectorCopy(face2->planepts[2], newface->planepts[2]);
		newface->plane = face2->plane;
	}

	// link the new brush to an entity
	brush1->owner->LinkBrush(newbrush);
	// build windings for the faces
	newbrush->Build();

	return newbrush;
}

/*
====================
Brush_MergeListPairs

Returns a list with merged brushes.
Tries to merge brushes pair wise.
The input list is destroyed.
Input and output should be a single linked list using .next
====================
*/
Brush *Brush_MergeListPairs (Brush *brushlist, int onlyshape)
{
	int			nummerges, merged;
	Brush	   *b1, *b2, *tail, *newbrush, *newbrushlist;
	Brush	   *lastb2;

	if (!brushlist)
		return NULL;

	nummerges = 0;

	do
	{
		for (tail = brushlist; tail; tail = tail->next)
			if (!tail->next) 
				break;

		merged = 0;
		newbrushlist = NULL;

		for (b1 = brushlist; b1; b1 = brushlist)
		{
			lastb2 = b1;
			for (b2 = b1->next; b2; b2 = b2->next)
			{
				newbrush = Brush_Merge(b1, b2, onlyshape);
				if (newbrush)
				{
					tail->next = newbrush;
					lastb2->next = b2->next;
					brushlist = brushlist->next;
					b1->next = b1->prev = NULL;
					b2->next = b2->prev = NULL;
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
===============
Brush_MergeList

Tries to merge all brushes in the list into one new brush.
The input brush list stays intact.
Returns NULL if no merged brush can be created.
To create a new brush the brushes in the list may not overlap and
the outer faces of the brushes together should make a new convex brush.

If onlyshape is true then the merge is allowed based on the shape only
otherwise the texture references of faces in the same plane have to
be the same as well.
===============
*/
Brush *Brush_MergeList (Brush *brushlist, int onlyshape)
{
	Brush	*brush1, *brush2, *brush3, *newbrush;
	Face	*face1, *face2, *face3, *newface, *f;

	if (!brushlist) 
		return NULL;

	for (brush1 = brushlist; brush1; brush1 = brush1->next)
	{
		// check if the new brush would be convex... flipped planes make a brush concave
		for (face1 = brush1->brush_faces; face1; face1 = face1->next)
		{
			// don't check face1 if it touches another brush
			for (brush2 = brushlist; brush2; brush2 = brush2->next)
			{
				if (brush2 == brush1) 
					continue;
				for (face2 = brush2->brush_faces; face2; face2 = face2->next)
					if (face1->plane.EqualTo(&face2->plane, true))
						break;

				if (face2) 
					break;
			}
			// if face1 touches another brush
			if (brush2)
				continue;

			for (brush2 = brush1->next; brush2; brush2 = brush2->next)
			{
				// don't check the faces of brush 2 touching another brush
				for (face2 = brush2->brush_faces; face2; face2 = face2->next)
				{
					for (brush3 = brushlist; brush3; brush3 = brush3->next)
					{
						if (brush3 == brush2) 
							continue;
						for (face3 = brush3->brush_faces; face3; face3 = face3->next)
							if (face2->plane.EqualTo(&face3->plane, true))
								break;

						if (face3) 
							break;
					}

					// if face2 touches another brush
					if (brush3) 
						continue;

					if (face1->plane.EqualTo(&face2->plane, false))
					{
						//if the texture references should be the same but are not
						if (!onlyshape && _stricmp(face1->texdef.name, face2->texdef.name) != 0) 
							return NULL;
						continue;
					}

					if (Winding::PlanesConcave(face1->face_winding, face2->face_winding,
											  face1->plane.normal, face2->plane.normal,
											  face1->plane.dist, face2->plane.dist))
						return NULL;
				}
			}
		}
	}

	newbrush = new Brush();

	for (brush1 = brushlist; brush1; brush1 = brush1->next)
	{
		for (face1 = brush1->brush_faces; face1; face1 = face1->next)
		{
			// don't add face1 to the new brush if it touches another brush
			for (brush2 = brushlist; brush2; brush2 = brush2->next)
			{
				if (brush2 == brush1) 
					continue;
				for (face2 = brush2->brush_faces; face2; face2 = face2->next)
					if (face1->plane.EqualTo(&face2->plane, true))
						break;

				if (face2) 
					break;
			}

			if (brush2) 
				continue;

			// don't add faces with the same plane twice
			for (f = newbrush->brush_faces; f; f = f->next)
			{
				if (face1->plane.EqualTo(&f->plane, false))
					break;
				if (face1->plane.EqualTo(&f->plane, true))
					break;
			}

			if (f)
				continue;

			newface = new Face(newbrush);
			newface->texdef = face1->texdef;
			VectorCopy(face1->planepts[0], newface->planepts[0]);
			VectorCopy(face1->planepts[1], newface->planepts[1]);
			VectorCopy(face1->planepts[2], newface->planepts[2]);
			newface->plane = face1->plane;
		}
	}

	// link the new brush to an entity
	brushlist->owner->LinkBrush(newbrush);
	// build windings for the faces
	newbrush->Build();

	return newbrush;
}

/*
=============
Brush_Subtract

Returns a list of brushes that remain after B is subtracted from A.
May by empty if A is contained inside B.
The originals are undisturbed.
=============
*/
Brush *Brush_Subtract (Brush *a, Brush *b)
{
	// a - b = out (list)
	Brush	*front, *back;
	Brush	*in, *out, *next;
	Face	*f;

	in = a;
	out = NULL;

	for (f = b->brush_faces; f && in; f = f->next)
	{
		CSG_SplitBrushByFace(in, f, &front, &back);
		if (in != a)
			delete in;
		if (front)
		{	// add to list
			front->next = out;
			out = front;
		}
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
			b->next = b->prev = NULL;
			delete b;
		}
		return a;
	}
	return out;
}

/*
=============
CSG_Subtract
=============
*/
void CSG_Subtract ()
{
	int			i, numfragments, numbrushes;
	Brush		fragmentlist;
	Brush	   *b, *s, *fragments, *nextfragment, *frag, *next, *snext;

	Sys_Printf("CMD: CSG Subtracting...\n");

	if (!Select_HasBrushes())
	{
		Sys_Printf("WARNING: No brushes selected.\n");
		return;
	}

	fragmentlist.next = &fragmentlist;
	fragmentlist.prev = &fragmentlist;

	numfragments = 0;
	numbrushes = 0;
	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b=next)
	{
		next = b->next;

		if (b->owner->eclass->IsFixedSize())
			continue;	// can't use texture from a fixed entity, so don't subtract

		// chop all fragments further up
		for (s = fragmentlist.next; s != &fragmentlist; s = snext)
		{
			snext = s->next;

			for (i = 0; i < 3; i++)
				if (b->mins[i] >= s->maxs[i] - ON_EPSILON || b->maxs[i] <= s->mins[i] + ON_EPSILON)
					break;

			if (i != 3)
				continue;	// definately don't touch

			fragments = Brush_Subtract(s, b);

			// if the brushes did not really intersect
			if (fragments == s)
				continue;

			// try to merge fragments
			fragments = Brush_MergeListPairs(fragments, true);

			// add the fragments to the list
			for (frag = fragments; frag; frag = nextfragment)
			{
				nextfragment = frag->next;
				frag->next = NULL;
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

			if (s->owner->eclass->IsFixedSize() || s->hiddenBrush)
				continue;

			for (i = 0; i < 3; i++)
				if (b->mins[i] >= s->maxs[i] - ON_EPSILON || b->maxs[i] <= s->mins[i] + ON_EPSILON)
					break;

			if (i != 3)
				continue;	// definately don't touch

			fragments = Brush_Subtract(s, b);

			// if the brushes did not really intersect
			if (fragments == s)
				continue;

			Undo::AddBrush(s);	// sikk - Undo/Redo

			// one extra brush chopped up
			numbrushes++;

			// try to merge fragments
			fragments = Brush_MergeListPairs(fragments, true);

			// add the fragments to the list
			for (frag = fragments; frag; frag = nextfragment)
			{
				nextfragment = frag->next;
				frag->next = NULL;
				frag->owner = s->owner;
				frag->AddToList(&fragmentlist);
			}

			// free the original brush
			delete s;
		}
	}

	// move all fragments to the active brush list
	for (frag = fragmentlist.next; frag != &fragmentlist; frag = nextfragment)
	{
		nextfragment = frag->next;
		numfragments++;
		frag->RemoveFromList();
		frag->AddToList( &g_map.brActive);
		Undo::EndBrush(frag);	// sikk - Undo/Redo
	}

	if (numfragments == 0)
	{
		Sys_Printf("WARNING: Selected brush%s did not intersect with any other brushes.\n",
				   (g_brSelectedBrushes.next->next == &g_brSelectedBrushes) ? "":"es");
		return;
	}

	Sys_Printf("MSG: Done. (created %d fragment%s out of %d brush%s)\n", 
				numfragments, (numfragments == 1) ? "" : "s",
				numbrushes, (numbrushes == 1) ? "" : "es");
	Sys_UpdateWindows(W_ALL);
}

/*
=============
CSG_Merge
=============
*/
void CSG_Merge ()
{
	Brush		*b, *next, *newlist, *newbrush;
	//Entity	*owner;

	Sys_Printf("CMD: CSG Merging...\n");

	if (!CSG_CanMerge())
		return;

	newlist = NULL;
	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = next)
	{
		next = b->next;

		b->RemoveFromList();
		b->next = newlist;
		b->prev = NULL;
		newlist = b;
	}

	newbrush = Brush_MergeList(newlist, true);
	// if the new brush would not be convex
	if (!newbrush)
	{
		// add the brushes back into the selection
		for (b = newlist; b; b = next)
		{
			next = b->next;
			b->next = NULL;
			b->prev = NULL;
			b->AddToList(&g_brSelectedBrushes);
		}
		Sys_Printf("WARNING: Cannot add a set of brushes with a concave hull.\n");
		return;
	}
	// free the original brushes
	for (b = newlist; b; b = next)
	{
		next = b->next;
		b->next = NULL;
		b->prev = NULL;
		delete b;
	}

	Select_SelectBrush(newbrush);
	Sys_Printf("MSG: Merge done.\n");
}
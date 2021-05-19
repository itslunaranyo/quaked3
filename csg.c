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
void CSG_SplitBrushByFace (brush_t *in, face_t *f, brush_t **front, brush_t **back)
{
	brush_t	   *b;
	face_t	   *nf;
	vec3_t		temp;

	b = Brush_Clone(in);
	nf = Face_Clone(f);

	nf->texdef = b->brush_faces->texdef;
	nf->next = b->brush_faces;
	b->brush_faces = nf;

	Brush_Build(b);
	Brush_RemoveEmptyFaces(b);

	if (!b->brush_faces)
	{	// completely clipped away
		Brush_Free(b);
		*back = NULL;
	}
	else
	{
		Entity_LinkBrush(in->owner, b);
		*back = b;
	}

	b = Brush_Clone(in);
	nf = Face_Clone(f);

	// swap the plane winding
	VectorCopy(nf->planepts[0], temp);
	VectorCopy(nf->planepts[1], nf->planepts[0]);
	VectorCopy(temp, nf->planepts[1]);

	nf->texdef = b->brush_faces->texdef;
	nf->next = b->brush_faces;
	b->brush_faces = nf;

	Brush_Build(b);
	Brush_RemoveEmptyFaces(b);

	if (!b->brush_faces)
	{	// completely clipped away
		Brush_Free(b);
		*front = NULL;
	}
	else
	{
		Entity_LinkBrush(in->owner, b);
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
	brush_t	   *b, *front, *back, *next;
	face_t	   *f;
	face_t		split;
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

		if (b->owner->eclass->fixedsize || b->hiddenBrush)
			continue;

		for (f = b->brush_faces; f; f = f->next)
		{
			split = *f;
			VectorScale(f->plane.normal, g_qeglobals.d_nGridSize, move);
			for (i = 0; i < 3; i++)
				VectorSubtract(split.planepts[i], move, split.planepts[i]);

			CSG_SplitBrushByFace(b, &split, &front, &back);

			if (back)
				Brush_Free(back);
			if (front)
				Brush_AddToList(front, &g_brSelectedBrushes);
		}
		Brush_Free(b);
	}
	Sys_UpdateWindows(W_ALL);
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
brush_t *Brush_Merge (brush_t *brush1, brush_t *brush2, int onlyshape)
{
	int			i, shared;
	brush_t    *newbrush;
	face_t     *face1, *face2, *newface, *f;

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
			if (Plane_Equal(&face1->plane, &face2->plane, true))
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
				if (Plane_Equal(&face2->plane, &f->plane, true)) 
					break;

			if (f)
				continue;

			if (Plane_Equal(&face1->plane, &face2->plane, false))
			{
				//if the texture references should be the same but are not
				if (!onlyshape && stricmp(face1->texdef.name, face2->texdef.name) != 0) 
					return NULL;
				continue;
			}

			if (Winding_PlanesConcave(face1->face_winding, face2->face_winding,	
									  face1->plane.normal, face2->plane.normal,
									  face1->plane.dist, face2->plane.dist))
				return NULL;
		}
	}

	newbrush = Brush_Alloc();

	for (face1 = brush1->brush_faces; face1; face1 = face1->next)
	{
		// don't add the faces of brush 1 and 2 touching each other
		for (face2 = brush2->brush_faces; face2; face2 = face2->next)
			if (Plane_Equal(&face1->plane, &face2->plane, true))
				break;

		if (face2)
			continue;

		// don't add faces with the same plane twice
		for (f = newbrush->brush_faces; f; f = f->next)
		{
			if (Plane_Equal(&face1->plane, &f->plane, false))
				break;
			if (Plane_Equal(&face1->plane, &f->plane, true))
				break;
		}

		if (f)
			continue;

		newface = Face_Alloc();
		newface->texdef = face1->texdef;
		VectorCopy(face1->planepts[0], newface->planepts[0]);
		VectorCopy(face1->planepts[1], newface->planepts[1]);
		VectorCopy(face1->planepts[2], newface->planepts[2]);
		newface->plane = face1->plane;
		newface->next = newbrush->brush_faces;
		newbrush->brush_faces = newface;
	}

	for (face2 = brush2->brush_faces; face2; face2 = face2->next)
	{
		// don't add the faces of brush 1 and 2 touching each other
		for (face1 = brush1->brush_faces; face1; face1 = face1->next)
			if (Plane_Equal(&face2->plane, &face1->plane, true))
				break;

		if (face1)
			continue;

		// don't add faces with the same plane twice
		for (f = newbrush->brush_faces; f; f = f->next)
		{
			if (Plane_Equal(&face2->plane, &f->plane, false))
				break;
			if (Plane_Equal(&face2->plane, &f->plane, true))
				break;
		}

		if (f)
			continue;

		newface = Face_Alloc();
		newface->texdef = face2->texdef;
		VectorCopy(face2->planepts[0], newface->planepts[0]);
		VectorCopy(face2->planepts[1], newface->planepts[1]);
		VectorCopy(face2->planepts[2], newface->planepts[2]);
		newface->plane = face2->plane;
		newface->next = newbrush->brush_faces;
		newbrush->brush_faces = newface;
	}

	// link the new brush to an entity
	Entity_LinkBrush(brush1->owner, newbrush);
	// build windings for the faces
	Brush_BuildWindings(newbrush);

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
brush_t *Brush_MergeListPairs (brush_t *brushlist, int onlyshape)
{
	int			nummerges, merged;
	brush_t	   *b1, *b2, *tail, *newbrush, *newbrushlist;
	brush_t	   *lastb2;

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
					Brush_Free(b1);
					Brush_Free(b2);

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
brush_t *Brush_MergeList (brush_t *brushlist, int onlyshape)
{
	brush_t	*brush1, *brush2, *brush3, *newbrush;
	face_t	*face1, *face2, *face3, *newface, *f;

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
					if (Plane_Equal(&face1->plane, &face2->plane, true))
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
							if (Plane_Equal(&face2->plane, &face3->plane, true)) 
								break;

						if (face3) 
							break;
					}

					// if face2 touches another brush
					if (brush3) 
						continue;

					if (Plane_Equal(&face1->plane, &face2->plane, false))
					{
						//if the texture references should be the same but are not
						if (!onlyshape && stricmp(face1->texdef.name, face2->texdef.name) != 0) 
							return NULL;
						continue;
					}

					if (Winding_PlanesConcave(face1->face_winding, face2->face_winding,
											  face1->plane.normal, face2->plane.normal,
											  face1->plane.dist, face2->plane.dist))
						return NULL;
				}
			}
		}
	}

	newbrush = Brush_Alloc();

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
					if (Plane_Equal(&face1->plane, &face2->plane, true))
						break;

				if (face2) 
					break;
			}

			if (brush2) 
				continue;

			// don't add faces with the same plane twice
			for (f = newbrush->brush_faces; f; f = f->next)
			{
				if (Plane_Equal(&face1->plane, &f->plane, false))
					break;
				if (Plane_Equal(&face1->plane, &f->plane, true))
					break;
			}

			if (f)
				continue;

			newface = Face_Alloc();
			newface->texdef = face1->texdef;
			VectorCopy(face1->planepts[0], newface->planepts[0]);
			VectorCopy(face1->planepts[1], newface->planepts[1]);
			VectorCopy(face1->planepts[2], newface->planepts[2]);
			newface->plane = face1->plane;
			newface->next = newbrush->brush_faces;
			newbrush->brush_faces = newface;
		}
	}

	// link the new brush to an entity
	Entity_LinkBrush(brushlist->owner, newbrush);
	// build windings for the faces
	Brush_BuildWindings(newbrush);

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
brush_t *Brush_Subtract (brush_t *a, brush_t *b)
{
	// a - b = out (list)
	brush_t	*front, *back;
	brush_t	*in, *out, *next;
	face_t	*f;

	in = a;
	out = NULL;

	for (f = b->brush_faces; f && in; f = f->next)
	{
		CSG_SplitBrushByFace(in, f, &front, &back);
		if (in != a) 
			Brush_Free(in);
		if (front)
		{	// add to list
			front->next = out;
			out = front;
		}
		in = back;
	}

	// NOTE: in != a just in case brush b has no faces
	if (in && in != a)
		Brush_Free(in);
	else
	{	// didn't really intersect
		for (b = out; b; b = next)
		{
			next = b->next;
			b->next = b->prev = NULL;
			Brush_Free(b);
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
	brush_t		fragmentlist;
	brush_t	   *b, *s, *fragments, *nextfragment, *frag, *next, *snext;

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

		if (b->owner->eclass->fixedsize)
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
				Brush_AddToList(frag, &fragmentlist);
			}

			// free the original brush
			Brush_Free(s);
		}

		// chop any active brushes up
		for (s = g_brActiveBrushes.next; s != &g_brActiveBrushes; s = snext)
		{
			snext = s->next;

			if (s->owner->eclass->fixedsize || s->hiddenBrush)
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

			Undo_AddBrush(s);	// sikk - Undo/Redo

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
				Brush_AddToList(frag, &fragmentlist);
			}

			// free the original brush
			Brush_Free(s);
		}
	}

	// move all fragments to the active brush list
	for (frag = fragmentlist.next; frag != &fragmentlist; frag = nextfragment)
	{
		nextfragment = frag->next;
		numfragments++;
		Brush_RemoveFromList(frag);
		Brush_AddToList(frag, &g_brActiveBrushes);
		Undo_EndBrush(frag);	// sikk - Undo/Redo
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
	brush_t		*b, *next, *newlist, *newbrush;
	entity_t	*owner;

	Sys_Printf("CMD: CSG Merging...\n");

	if (!Select_HasBrushes())
	{
		Sys_Printf("WARNING: No brushes selected.\n");
		return;
	}

	if (g_brSelectedBrushes.next->next == &g_brSelectedBrushes)
	{
		Sys_Printf("WARNING: At least two brushes must be selected.\n");
		return;
	}

	owner = g_brSelectedBrushes.next->owner;

	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = next)
	{
		next = b->next;

		if (b->owner->eclass->fixedsize)
		{
			// can't use texture from a fixed entity, so don't subtract
			Sys_Printf("WARNING: Cannot add fixed size entities.\n");
			return;
		}

		if (b->owner != owner)
		{
			Sys_Printf("WARNING: Cannot add brushes from different entities.\n");
			return;
		}

	}

	newlist = NULL;
	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = next)
	{
		next = b->next;

		Brush_RemoveFromList(b);
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
			Brush_AddToList(b, &g_brSelectedBrushes);
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
		Brush_Free(b);
	}

	Select_SelectBrush(newbrush);
	Sys_Printf("MSG: Merge done.\n");
}
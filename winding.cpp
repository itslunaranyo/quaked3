//==============================
//	winding.c
//==============================

#include "qe3.h"


/*
=============
Plane_Equal
=============
*/
bool Plane_Equal (plane_t *a, plane_t *b, int flip)
{
	float	dist;
	vec3_t	normal;

	if (flip) 
	{
		normal[0] = - b->normal[0];
		normal[1] = - b->normal[1];
		normal[2] = - b->normal[2];
		dist = - b->dist;
	}
	else 
	{
		normal[0] = b->normal[0];
		normal[1] = b->normal[1];
		normal[2] = b->normal[2];
		dist = b->dist;
	}
	
	if (fabs(a->normal[0] - normal[0]) < NORMAL_EPSILON	&& 
		fabs(a->normal[1] - normal[1]) < NORMAL_EPSILON	&& 
		fabs(a->normal[2] - normal[2]) < NORMAL_EPSILON	&& 
		fabs(a->dist - dist) < DIST_EPSILON)
		return true;

	return false;
}

/*
============
Plane_FromPoints
============
*/
bool Plane_FromPoints (vec3_t p1, vec3_t p2, vec3_t p3, plane_t *plane)
{
	vec3_t	v1, v2;

	VectorSubtract(p2, p1, v1);
	VectorSubtract(p3, p1, v2);
//	CrossProduct(v2, v1, plane->normal);
	CrossProduct(v1, v2, plane->normal);

	if (VectorNormalize(plane->normal) < 0.1) 
		return false;

	plane->dist = DotProduct(p1, plane->normal);

	return true;
}

/*
=================
Point_Equal
=================
*/
bool Point_Equal (vec3_t p1, vec3_t p2, float epsilon)
{
	int i;

	for (i = 0; i < 3; i++)
		if (fabs(p1[i] - p2[i]) > epsilon) 
			return false;

	return true;
}

//===============================================================================

/*
==================
Winding_Alloc
==================
*/
winding_t *Winding_Alloc (int points)
{
	int			size;
	winding_t  *w;
	
	if (points > MAX_POINTS_ON_WINDING)
		Error("Winding_Alloc: %d points.", points);
	
	size = (int)((winding_t *)0)->points[points];
	w = (winding_t *)malloc(size);
	memset(w, 0, size);
	w->maxpoints = points;
	
	return w;
}

/*
=============
Winding_Free
=============
*/
void Winding_Free (winding_t *w)
{
	free(w);
}

/*
==================
Winding_Clone
==================
*/
winding_t *Winding_Clone (winding_t *w)
{
	int size;
	winding_t *c;
	
	size = (int)((winding_t *)0)->points[w->numpoints];
	c = (winding_t*)qmalloc(size);
	memcpy(c, w, size);

	return c;
}

/*
==============
Winding_RemovePoint
==============
*/
void Winding_RemovePoint (winding_t *w, int point)
{
	if (point < 0 || point >= w->numpoints)
		Error("Winding_RemovePoint: Point out of range.");

	if (point < w->numpoints-1)
		memmove(&w->points[point], &w->points[point + 1], (int)((winding_t *)0)->points[w->numpoints - point - 1]);

	w->numpoints--;
}

/*
==================
Winding_Clip

Clips the winding to the plane, returning the new winding on the positive side
Frees the input winding.
If keepon is true, an exactly on-plane winding will be saved, otherwise
it will be clipped away.
==================
*/
winding_t *Winding_Clip (winding_t *in, plane_t *split, bool keepon)
{
	int			i, j;
	int			counts[3];
	int			maxpts;
	int			sides[MAX_POINTS_ON_WINDING];
	vec_t		dists[MAX_POINTS_ON_WINDING];
	vec_t		dot;
	vec_t	   *p1, *p2;
	vec3_t		mid;
	winding_t  *neww;
	
	counts[0] = counts[1] = counts[2] = 0;

	// determine sides for each point
	for (i = 0; i < in->numpoints; i++)
	{
		dot = DotProduct(in->points[i], split->normal);
		dot -= split->dist;
		dists[i] = dot;

		if (dot > ON_EPSILON)
			sides[i] = SIDE_FRONT;
		else if (dot < -ON_EPSILON)
			sides[i] = SIDE_BACK;
		else
			sides[i] = SIDE_ON;

		counts[sides[i]]++;
	}

	sides[i] = sides[0];
	dists[i] = dists[0];
	
	if (keepon && !counts[0] && !counts[1])
		return in;
		
	if (!counts[0])
	{
		Winding_Free(in);
		return NULL;
	}

	if (!counts[1])
		return in;
	
	maxpts = in->numpoints + 4;	// can't use counts[0] + 2 because of fp grouping errors
	neww = Winding_Alloc(maxpts);
		
	for (i = 0; i < in->numpoints; i++)
	{
		p1 = in->points[i];
		
		if (sides[i] == SIDE_ON)
		{
			VectorCopy(p1, neww->points[neww->numpoints]);
			neww->numpoints++;
			continue;
		}
	
		if (sides[i] == SIDE_FRONT)
		{
			VectorCopy(p1, neww->points[neww->numpoints]);
			neww->numpoints++;
		}
		
		if (sides[i + 1] == SIDE_ON || sides[i + 1] == sides[i])
			continue;
			
		// generate a split point
		p2 = in->points[(i + 1) % in->numpoints];
		
		dot = dists[i] / (dists[i] - dists[i + 1]);

		for (j = 0; j < 3; j++)
		{	// avoid round off error when possible
			if (split->normal[j] == 1)
				mid[j] = split->dist;
			else if (split->normal[j] == -1)
				mid[j] = -split->dist;
			else
				mid[j] = p1[j] + dot * (p2[j] - p1[j]);
		}
			
		VectorCopy(mid, neww->points[neww->numpoints]);
		neww->numpoints++;
	}
	
	if (neww->numpoints > maxpts)
		Error("Winding_Clip: Points exceeded estimate.");
		
	// free the original winding
	Winding_Free(in);
	
	return neww;
}

/*
=============
Winding_PlanesConcave
=============
*/
bool Winding_PlanesConcave (winding_t *w1, winding_t *w2,
							vec3_t normal1, vec3_t normal2,
							float dist1, float dist2)
{
	int i;

	if (!w1 || !w2) 
		return false;

	// check if one of the points of winding 1 is at the back of the plane of winding 2
	for (i = 0; i < w1->numpoints; i++)
		if (DotProduct(normal2, w1->points[i]) - dist2 > WCONVEX_EPSILON) 
			return true;

	// check if one of the points of winding 2 is at the back of the plane of winding 1
	for (i = 0; i < w2->numpoints; i++)
		if (DotProduct(normal1, w2->points[i]) - dist1 > WCONVEX_EPSILON) 
			return true;

	return false;
}

/*
=============
Winding_TryMerge

If two windings share a common edge and the edges that meet at the
common points are both inside the other polygons, merge them

Returns NULL if the windings couldn't be merged, or the new winding.
The originals will NOT be freed.

if keep is true no points are ever removed
=============
*/
winding_t *Winding_TryMerge (winding_t *f1, winding_t *f2, vec3_t planenormal, int keep)
{
	int			i, j, k, l;
	bool		keep1, keep2;
	vec_t		dot;
	vec_t	   *p1, *p2, *p3, *p4, *back;
	vec3_t		normal, delta;
	winding_t  *newf;
	
	// find a common edge
	p1 = p2 = NULL;	// stop compiler warning
	j = 0;			// 
	
	for (i = 0; i < f1->numpoints; i++)
	{
		p1 = f1->points[i];
		p2 = f1->points[(i + 1) % f1->numpoints];

		for (j = 0; j < f2->numpoints; j++)
		{
			p3 = f2->points[j];
			p4 = f2->points[(j + 1) % f2->numpoints];

			for (k = 0; k < 3; k++)
			{
				if (fabs(p1[k] - p4[k]) > 0.1)//EQUAL_EPSILON) //ME
					break;
				if (fabs(p2[k] - p3[k]) > 0.1)//EQUAL_EPSILON) //ME
					break;
			}

			if (k==3)
				break;
		}

		if (j < f2->numpoints)
			break;
	}
	
	if (i == f1->numpoints)
		return NULL;			// no matching edges

	// check slope of connected lines
	// if the slopes are colinear, the point can be removed
	back = f1->points[(i + f1->numpoints - 1) % f1->numpoints];
	VectorSubtract(p1, back, delta);
	CrossProduct(planenormal, delta, normal);
	VectorNormalize(normal);
	
	back = f2->points[(j + 2) % f2->numpoints];
	VectorSubtract(back, p1, delta);
	dot = DotProduct(delta, normal);

	if (dot > CONTINUOUS_EPSILON)
		return NULL;			// not a convex polygon

	keep1 = (bool)(dot < -CONTINUOUS_EPSILON);
	back = f1->points[(i + 2) % f1->numpoints];
	VectorSubtract(back, p2, delta);
	CrossProduct(planenormal, delta, normal);
	VectorNormalize(normal);

	back = f2->points[(j + f2->numpoints - 1) % f2->numpoints];
	VectorSubtract(back, p2, delta);
	dot = DotProduct(delta, normal);

	if (dot > CONTINUOUS_EPSILON)
		return NULL;			// not a convex polygon
	
	keep2 = (bool)(dot < -CONTINUOUS_EPSILON);

	// build the new polygon
	newf = Winding_Alloc(f1->numpoints + f2->numpoints);
	
	// copy first polygon
	for (k = (i + 1) % f1->numpoints; k != i; k = (k + 1) % f1->numpoints)
	{
		if (!keep && k == (i + 1) % f1->numpoints && !keep2)
			continue;
		
		VectorCopy(f1->points[k], newf->points[newf->numpoints]);
		newf->numpoints++;
	}
	
	// copy second polygon
	for (l = (j + 1) % f2->numpoints; l != j; l = (l + 1) % f2->numpoints)
	{
		if (!keep && l == (j + 1) % f2->numpoints && !keep1)
			continue;

		VectorCopy(f2->points[l], newf->points[newf->numpoints]);
		newf->numpoints++;
	}

	return newf;
}

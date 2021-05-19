//==============================
//	winding.cpp
//==============================

#include "qe3.h"
#include <list>

#define WINDINGS_CHUNK 512

free_winding_t *first;
std::list<free_winding_t*> windingChunks;

//===============================================================================

/*
==================
Winding::Clear
==================
*/
void Winding::Clear()
{
	first = nullptr;
	for (auto wcIt = windingChunks.begin(); wcIt != windingChunks.end(); wcIt++)
	{
		free(*wcIt);
	}
	windingChunks.clear();
}

void Winding::Test()
{
	winding_t *w, *x, *y;
	w = Alloc(4);
	x = Alloc(8);
	Free(w);
	y = Alloc(8);
	Free(x);
}

/*
==================
Winding::AllocPage
==================
*/
free_winding_t* Winding::AllocPage()
{
	// allocate a new page of windingSize * chunkCount
	free_winding_t* newChunk = (free_winding_t*)malloc(sizeof(winding_t) * WINDINGS_CHUNK);
	memset(newChunk, 0xEF, sizeof(winding_t) * WINDINGS_CHUNK);
	// add it to the chunklist
	windingChunks.push_back(newChunk);

	// format as free_winding_t with chunkCount * 6 points
	newChunk->maxWindings = WINDINGS_CHUNK;
	newChunk->prev = newChunk->next = nullptr;

	return newChunk;
}

/*
==================
Winding::Alloc

lunaran - qe3 already exploited lack of array overrun protection to treat winding_t 
as variably sized. 

i allocate them from a global pool in increments of 6 points, because 6 winding points
equals 120 + 8 bytes, and higher multiples still fit nicely just under multiples of 128.
having six points for a 4-point winding is wasteful, but it allows a cleaner allocation 
strategy. (the original code was always greedy by 4 points so we're technically already 
doing better than that anyway.)
==================
*/
winding_t *Winding::Alloc(int points)
{
	winding_t	*w;
	int			pts;
	free_winding_t* chunk;

	if (points > MAX_POINTS_ON_WINDING 	// MUST be less than WINDINGS_CHUNK * 6
		|| points < 3)
		Error("Winding::Alloc: %d points.", points);

	pts = (points - 1) / 6 + 1;
	w = nullptr;

	// guarantee we're pointing at a chunk big enough to accomodate our new winding

	if (!first)	// no free allocations
	{
		chunk = AllocPage();
		first = chunk;
	}
	else
	{
		chunk = first;
		// if first free has fewer points than needed, cycle them all to find one that's big enough
		while (chunk->maxWindings < pts)
		{
			if (!chunk->next)	// couldn't find any, add a whole new page to make room
			{
				// put it at the end so smaller chunks are still claimed first
				chunk->next = AllocPage();
				chunk->next->prev = chunk;
			}
			chunk = chunk->next;
		}
	}

	w = (winding_t*)chunk;
	// chosen winding chunk has as many points as needed
	if (pts == chunk->maxWindings)
	{
		// change first to point to the free_winding_t's next
		if (chunk == first)
		{
			first = chunk->next;
			if (first)
				first->prev = nullptr;
			//CheckFreeChain();
		}
		else
		{
			if (chunk->prev)
				chunk->prev->next = chunk->next;
			if (chunk->next)
				chunk->next->prev = chunk->prev;
			//CheckFreeChain();
		}
	}
	// has more points than needed, divide it
	else if (pts < chunk->maxWindings)
	{
		winding_t* wtp = w + pts;
		free_winding_t* end = (free_winding_t*)wtp;
		end->maxWindings = chunk->maxWindings - pts;
		end->next = chunk->next;
		end->prev = chunk->prev;

		if (end->prev)
			end->prev->next = end;
		if (end->next)
			end->next->prev = end;

		if (chunk == first)
			first = end;
		//CheckFreeChain();
	}
	else
	{
		assert(0);
	}
	memset(w, 0xBB, sizeof(winding_t)*pts);
	w->numpoints = 0;
	w->maxpoints = pts * 6;

	//CheckFreeChain();
	assert(w->maxpoints != 64);
	return w;
}

/*
winding_t *Winding::Alloc (int points)
{
	int			size;
	winding_t	*w = nullptr;

	if (points+2 > MAX_POINTS_ON_WINDING)	// MUST be less than WINDINGS_CHUNK * 6
		Error("Winding::Alloc: %d points.", points+2);

	size = (int)((winding_t *)0)->points[points+2];
	w = (winding_t *)malloc(size);
	memset(w, 0, size);
	w->maxpoints = points+2;

	return w;
}
*/

/*
==================
Winding::CheckFreeChain
==================
*/
void Winding::CheckFreeChain()
{
	if (!first)
		return;
	assert(first->prev == NULL);
	assert((int)first->next != 0x45400000);
	free_winding_t* chunk;
	chunk = first;
	while (chunk->next)
	{
		assert(chunk->next->prev == chunk);
		chunk = chunk->next;
	}
}

/*
=============
Winding::Free
=============
*/
void Winding::Free(winding_t *w)
{
	int maxW = w->maxpoints / 6;

	memset(w, 0xEF, sizeof(winding_t) * maxW);
	free_winding_t* fw = (free_winding_t*)w;

	fw->maxWindings = maxW;
	fw->next = first;
	if (first)
		first->prev = fw;
	fw->prev = nullptr;
	first = fw;

//	CheckFreeChain();
}
/*
void Winding::Free(winding_t *w)
{
	free(w);
}
*/


/*
==================
Winding::Clone
==================
*/
winding_t *Winding::Clone (winding_t *w)
{
	winding_t *c;
	
	c = Winding::Alloc(w->numpoints);
	Winding::Copy(w, c);
	/*
	int size;
	size = (int)((winding_t *)0)->points[w->numpoints];
	c = (winding_t*)qmalloc(size);
	memcpy(c, w, size);
	*/
	return c;
}

/*
==================
Winding::Copy
==================
*/
void Winding::Copy(winding_t *src, winding_t *dest)
{
	assert(dest->maxpoints >= src->numpoints);

	int size;
	size = sizeof(float) * 5 * src->numpoints;
	memcpy(dest->points, src->points, size);
	dest->numpoints = src->numpoints;
}

/*
==============
Winding::RemovePoint
==============
*/
void Winding::RemovePoint (winding_t *w, int point)
{
	if (point < 0 || point >= w->numpoints)
		Error("Winding::RemovePoint: Point out of range.");

	if (point < w->numpoints-1)
		memmove(&w->points[point], &w->points[point + 1], (int)((winding_t *)0)->points[w->numpoints - point - 1]);

	w->numpoints--;
}

/*
==================
Winding::Clip

Clips the winding to the plane, returning the new winding on the positive side
Frees the input winding.
If keepon is true, an exactly on-plane winding will be saved, otherwise
it will be clipped away.
==================
*/
winding_t *Winding::Clip (winding_t *in, Plane *split, bool keepon)
{
	int			i, j;
	int			counts[3];
//	int			maxpts;
	int			sides[MAX_POINTS_ON_WINDING];
	vec_t		dists[MAX_POINTS_ON_WINDING];
	vec_t		dot;
	vec_t	   *p1, *p2;
	vec3_t		mid;

	// lunaran - scratch max-size winding for working in place, to reduce constant winding alloc/free
	struct {
		int		numpoints;
		int		maxpoints;
		float 	points[MAX_POINTS_ON_WINDING][5];
	} neww;

	neww.numpoints = 0;
	neww.maxpoints = MAX_POINTS_ON_WINDING;
	
	counts[SIDE_FRONT] = counts[SIDE_BACK] = counts[SIDE_ON] = 0;

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
	
	if (keepon && !counts[SIDE_FRONT] && !counts[SIDE_BACK])
		return in;
		
	if (!counts[SIDE_FRONT])
	{
		Winding::Free(in);
		return NULL;
	}

	if (!counts[SIDE_BACK])
		return in;
	
	//maxpts = in->numpoints + 4;	// can't use counts[SIDE_FRONT] + 2 because of fp grouping errors
	//neww = Winding::Alloc(maxpts);
		
	for (i = 0; i < in->numpoints; i++)
	{
		p1 = in->points[i];
		
		if (sides[i] == SIDE_ON)
		{
			VectorCopy(p1, neww.points[neww.numpoints]);
			neww.numpoints++;
			continue;
		}
	
		if (sides[i] == SIDE_FRONT)
		{
			VectorCopy(p1, neww.points[neww.numpoints]);
			neww.numpoints++;
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
			
		VectorCopy(mid, neww.points[neww.numpoints]);
		neww.numpoints++;
	}
	
	winding_t* out;
	if (neww.numpoints > in->maxpoints)
	{
		// we need a bigger boat
		Winding::Free(in);
		out = Winding::Alloc(neww.numpoints);
	}
	else
		out = in;
		
	Winding::Copy((winding_t*)&neww, out);
	return out;
}

/*
=============
Winding::PlanesConcave
=============
*/
bool Winding::PlanesConcave(winding_t *w1, winding_t *w2,
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
Winding::TryMerge

If two windings share a common edge and the edges that meet at the
common points are both inside the other polygons, merge them

Returns NULL if the windings couldn't be merged, or the new winding.
The originals will NOT be freed.

if keep is true no points are ever removed
=============
*/
winding_t *Winding::TryMerge (winding_t *f1, winding_t *f2, vec3_t planenormal, int keep)
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
	newf = Winding::Alloc(f1->numpoints + f2->numpoints);
	
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


/*
==================
Winding::TextureCoordinates
==================
*/
void Winding::TextureCoordinates(winding_t *w, Texture *q, Face *f)
{
	float		s, t, ns, nt;
	float		ang, sinv, cosv;
	vec3_t		vecs[2];
	texdef_t	*texdef;
	float		*xyzst;

	for (int i = 0; i < w->numpoints; i++)
	{
		xyzst = w->points[i];

		// get natural texture axis
		f->plane.GetTextureAxis(vecs[0], vecs[1]);

		texdef = &f->texdef;

		ang = texdef->rotate / 180 * Q_PI;
		sinv = sin(ang);
		cosv = cos(ang);

		if (!texdef->scale[0])
			texdef->scale[0] = 1;
		if (!texdef->scale[1])
			texdef->scale[1] = 1;

		s = DotProduct(xyzst, vecs[0]);
		t = DotProduct(xyzst, vecs[1]);

		ns = cosv * s - sinv * t;
		nt = sinv * s + cosv * t;

		s = ns / texdef->scale[0] + texdef->shift[0];
		t = nt / texdef->scale[1] + texdef->shift[1];

		// gl scales everything from 0 to 1
		s /= q->width;
		t /= q->height;

		xyzst[3] = s;
		xyzst[4] = t;
	}
}

int Winding::MemorySize(winding_t * w)
{
	return sizeof(winding_t);
}

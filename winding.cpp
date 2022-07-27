//==============================
//	winding.cpp
//==============================

#include "pre.h"
#include "mathlib.h"
#include "qe3.h"
#include "winding.h"
#include "map.h"
#include <list>

#define WINDINGS_CHUNK 512

#ifdef OLDWINDING
free_winding_t *first;
std::list<free_winding_t*> windingChunks;
#endif

//===============================================================================

/*
THE NEW WINDING SHIT

- single global pool, no pages
	- start at 768kb (32k verts, enough for ~1300 brushes)
	- reallocate with +768kb whenever free space drops under threshold
	- pool location is static
	- faces keep uint32 index into pool alongside winding size (count+capacity)
- allocation
	- pool is consumed in groups of 4 verts
		- 95% of brushes on map load have only quads for all faces, and stay that way
		- a double consumption of 8 verts covers 95% of the rest
	- pool retains a high water mark with all empty space above and a free list below
	- retain a count of how many windings are pulled from the free list vs highwater
		and use as a rough guide to fragmentation - realloc if too high
- reallocation
	- triggered after a save under 128k free, after cmdqueue op under 32k free
	- rather than copy data, just call map->rebuild to heal all fragmentation as well
	- whole map rebuild is complete in a few hundred ms so this is acceptable
	- if the map is saved while a tool or command is live, defer (or upgrade to URGENT)
- etc
	- all brushes are loaded from a map before being built, so it allows an initial
		pool size guess (32*brushcount verts leaves room to grow)
	- 3 bytes as winding index is enough for 16 million faces - use last byte for...?
		- fork pools for different purposes? other allocators?
		- keeps scratch windings out of the main (render) pool
	- global pool is used directly as the VAO for rendering
*/

constexpr uint32_t vsWINDING_BLOCK_SIZE = 8192;	// measured in 4-vert segs
constexpr uint32_t vsNULL_INDEX = -1;

//Winding::realloc_status Winding::_status = Winding::IMMEDIATE;
Winding::vertex_seg_t* Winding::_pool = nullptr;
uint32_t Winding::_vsMargin = 0;
uint32_t Winding::_vsFreeHead = vsNULL_INDEX;
uint32_t Winding::_vsFreeSegs = 0;
uint32_t Winding::_vsCapacity = 0;
bool Winding::flushed = false;

Winding::Winding()
{
	if (!_pool)
		Allocate(0, false);
}

Winding::~Winding()
{
	Free();
}

/*
==================
Winding::Grow

ensure the winding has room for at least minPoints. does not guarantee
that prior winding points will be copied if a change of location is 
necessary, so windings cannot be safely grown while under construction. 
use scratch memory for this.

resizing to a minPoints of less than the current capacity does nothing.

noFrags: always alloc at the high margin (skip the free list) - used if
	a number of windings are going to be allocated at once and it would
	be nice if they were contiguous.
==================
*/
void Winding::Grow(const int vMinPoints, const bool noFrags)
{
	if (vMaxPts >= vMinPoints) return;	// we're already big enough

	if (GetStatus() == IMMEDIATE)
		Allocate();
										
	// we always alloc in groups of 4 ("segments"): 95% of brushes have only
	//	quads for all faces, and stay that way forever, and a double alloc of
	//	8 verts covers 95% of the rest
	uint32_t vsNewSize = v2vs(vMinPoints);

	// if we're the last winding below the high water mark, just get bigger
	if (IsHighmost())
	{
		_vsMargin = vsIndex + vsNewSize;
		vMaxPts = vs2v(vsNewSize);	// don't use vMinPoints, not guaranteed to be on seg boundary
		return;
	}

	Free();

	// check free list for something of suitable size
	if (!noFrags && _vsFreeHead != vsNULL_INDEX)// && vsNewSize <= 8)
	{
		// for now: leave windings sized as they are - if we're dragging the clip
		// points across a 12-sided cylinder (for example) we don't want to 
		// parasitically gobble the buffer 12 at a time, and if a 12-winding was
		// recently freed it'll be close to the start of the free list anyway

		// for later: solve this problem by not using this (permanent) allocator
		//	for (temporary) tool windings

		uint32_t vsFree;
		uint32_t vsLastFree = vsNULL_INDEX;
		free_vseg_t* freew = nullptr;
		free_vseg_t* lastfreew = nullptr;
		for (vsFree = _vsFreeHead; vsFree != vsNULL_INDEX; vsFree = freew->vsNext)
		{
			freew = (free_vseg_t*)&_pool[vsFree];
			if (freew->vsSize == vsNewSize)
			{
				if (vsLastFree != vsNULL_INDEX)
					lastfreew->vsNext = freew->vsNext;
				else
					_vsFreeHead = freew->vsNext;

				vsIndex = vsFree;
				vMaxPts = vs2v(vsNewSize);
				_vsFreeSegs -= vsNewSize;
				return;
			}
			vsLastFree = vsFree;
			lastfreew = freew;
		}
	}

	// put at the end
	vsIndex = _vsMargin;
	vMaxPts = vs2v(vsNewSize);
	_vsMargin += vsNewSize;
}

void Winding::Free()
{
	if (vMaxPts)
	{
		if (IsHighmost())
		{
			_vsMargin -= v2vs(vMaxPts);
		}
		else
		{
			free_vseg_t* freew = (free_vseg_t*)&_pool[vsIndex];

			freew->vsSize = v2vs(vMaxPts);
			_vsFreeSegs += freew->vsSize;
			freew->vsNext = _vsFreeHead;
			_vsFreeHead = vsIndex;
		}
	}
	vCount = 0;
	vMaxPts = 0;
	vsIndex = vsNULL_INDEX;
}

void Winding::Swap(Winding& other)
{
	byte vCountTemp = other.vCount;
	byte vMaxPtsTemp = other.vMaxPts;
	uint32_t vsIndexTemp = other.vsIndex;

	other.vCount = vCount;
	other.vMaxPts = vMaxPts;
	other.vsIndex = vsIndex;

	vCount = vCountTemp;
	vMaxPts = vMaxPtsTemp;
	vsIndex = vsIndexTemp;
}

Winding::vertex_t* const Winding::Vertex(const int i)
{
	if (i < 0 && i >= vCount)
		throw new std::exception("Index out of range on winding[]\n");
	return _vertex_safe(i);
}


void const Winding::AddBounds(vec3& mins, vec3& maxs)
{
	for (int i = 0; i < vCount; ++i)
	{
		// add to bounding box
		mins = glm::min(mins, _vertex_safe(i)->point);
		maxs = glm::max(maxs, _vertex_safe(i)->point);
	}
}

vec3 const Winding::Centroid()
{
	vec3 a, b, c;
	c = vec3(0);
	for (int i = 0; i < vCount; i++)
	{
		a = _vertex_safe(i)->point;
		b = _vertex_safe((i + 1) % vCount)->point;
		c += (a + b) * 0.5f;
	}
	return c / (float)vCount;
}

// currently only called by ugly old merge used by ugly old csg carve
bool const Winding::Equal(Winding& w2, bool flip)
{
	int offset, i, io;

	if (vCount != w2.vCount)
		return false;

	offset = -1;

	for (i = 0; i < w2.vCount; i++)
	{
		if (_vertex_safe(i)->point == w2._vertex_safe(i)->point)
		{
			offset = i;
			break;
		}
	}
	if (offset == -1)
		return false;	// no points in common

	// w1 point 0 == w2 point 'offset'
	for (i = 0; i < vCount; i++)
	{
		io = ((flip ? offset - i : offset + i) + vCount) % vCount;
		if (_vertex_safe(i)->point != w2._vertex_safe(i)->point)
			return false;
	}
	return true;
}

// FIXME returns Plane::ON if winding is empty
Plane::side const Winding::PlaneSides(Plane& split)
{
	double dot;
	int counts[3];
	counts[Plane::FRONT] = counts[Plane::BACK] = counts[Plane::ON] = 0;

	// determine sides for each point
	for (int i = 0; i < vCount; i++)
	{
		dot = DotProduct(dvec3(_vertex_safe(i)->point), split.normal);
		dot -= split.dist;

		if (dot > ON_EPSILON)
			counts[Plane::FRONT]++;
		else if (dot < -ON_EPSILON)
			counts[Plane::BACK]++;
		else
			counts[Plane::ON]++;
	}

	if (!counts[Plane::BACK] && counts[Plane::FRONT])
		return Plane::FRONT;
	if (counts[Plane::BACK] && !counts[Plane::FRONT])
		return Plane::BACK;
	return Plane::ON;
}


// project a really big	axis aligned box onto a plane
void Winding::Base(Plane& p)
{
	dvec3		org, vright, vup;

	org = p.normal * p.dist;
	vup = VectorPerpendicular(p.normal);
	vright = CrossProduct(vup, p.normal);

	vup = vup * (double)g_cfgEditor.MapSize;
	vright = vright * (double)g_cfgEditor.MapSize;

	if (flushed)
	{
		vsIndex = vsNULL_INDEX;
		vMaxPts = vCount = 0;
	}

	Grow(4);

	_vertex_safe(0)->point = org - vright + vup;
	_vertex_safe(1)->point = org + vright + vup;
	_vertex_safe(2)->point = org + vright - vup;
	_vertex_safe(3)->point = org - vright - vup;

	vCount = 4;
}

/*
==================
Winding::Clip

Clips the winding to the plane, leaving the remainder on the positive side
If keepon is true, an exactly on-plane winding will be saved, otherwise
it will be clipped away.
Returns false if winding was clipped away.
==================
*/
bool Winding::Clip(Plane& split, bool keepon)
{
	int		i, j;
	int		counts[3];
	int		sides[MAX_POINTS_ON_WINDING];
	double	dists[MAX_POINTS_ON_WINDING];
	double	dot;
	dvec3	p1, p2;
	dvec3	mid;

	// scratch max-size winding for working in place
	struct {
		int		numpoints;
		vertex_t	points[MAX_POINTS_ON_WINDING];
	} neww;
	neww.numpoints = 0;
	
	counts[Plane::FRONT] = counts[Plane::BACK] = counts[Plane::ON] = 0;

	// determine sides for each point
	for (i = 0; i < vCount; i++)
	{
		dot = DotProduct(dvec3(_vertex_safe(i)->point), split.normal);
		dot -= split.dist;
		dists[i] = dot;

		if (dot > ON_EPSILON)
			sides[i] = Plane::FRONT;
		else if (dot < -ON_EPSILON)
			sides[i] = Plane::BACK;
		else
			sides[i] = Plane::ON;

		counts[sides[i]]++;
	}

	sides[i] = sides[0];
	dists[i] = dists[0];
	
	if (keepon && !counts[Plane::FRONT] && !counts[Plane::BACK])
		return true;	// coplanar to splitting plane

	if (!counts[Plane::FRONT])
	{
		Free();
		return false;	// no positive points, winding clipped away
	}

	if (!counts[Plane::BACK])
		return true;	// splitting plane didn't touch us

	for (i = 0; i < vCount; i++)
	{
		p1 = _vertex_safe(i)->point;
		
		if (sides[i] == Plane::ON)
		{
			neww.points[neww.numpoints].point = p1;
			neww.numpoints++;
			continue;
		}
	
		if (sides[i] == Plane::FRONT)
		{
			neww.points[neww.numpoints].point = p1;
			neww.numpoints++;
		}
		
		if (sides[i + 1] == Plane::ON || sides[i + 1] == sides[i])
			continue;
			
		// generate a split point
		p2 = _vertex_safe((i + 1) % vCount)->point;
		
		dot = dists[i] / (dists[i] - dists[i + 1]);

		for (j = 0; j < 3; j++)
			mid[j] = p1[j] + dot * (p2[j] - p1[j]);
			
		neww.points[neww.numpoints].point = mid;
		neww.numpoints++;
	}
	
	if (neww.numpoints > vMaxPts)
		Grow(neww.numpoints);	// we need a bigger boat
		
	for (i = 0; i < neww.numpoints; ++i)
		*_vertex_safe(i) = neww.points[i];
	vCount = neww.numpoints;

	return true;
}

void Winding::TextureCoordinates(Texture& q, Face& f)
{
	float		s, t, ns, nt;
	float		ang, sinv, cosv;
	vec3		vecs[2];
	TexDef*		texdef;
	vertex_t*	xyzst;

	for (int i = 0; i < vCount; i++)
	{
		xyzst = _vertex_safe(i);

		// get natural texture axis
		f.plane.GetTextureAxis(vecs[0], vecs[1]);

		texdef = &f.texdef;

		ang = texdef->rotate / 180 * Q_PI;
		sinv = sin(ang);
		cosv = cos(ang);

		if (!texdef->scale[0])
			texdef->scale[0] = 1;
		if (!texdef->scale[1])
			texdef->scale[1] = 1;

		s = DotProduct(xyzst->point, vecs[0]);
		t = DotProduct(xyzst->point, vecs[1]);

		ns = cosv * s - sinv * t;
		nt = sinv * s + cosv * t;

		s = ns / texdef->scale[0] + texdef->shift[0];
		t = nt / texdef->scale[1] + texdef->shift[1];

		// gl scales everything from 0 to 1
		s /= q.width;
		t /= q.height;

		xyzst->s = s;
		xyzst->t = t;
	}
}

void Winding::RemovePoint(int point)
{
	if (point < 0 || point >= vCount)
		Error("Winding::RemovePoint: Point out of range.");

	if (point < vCount - 1)
		for (int i = point; i < vCount - 1; ++i)
		{
			*_vertex_safe(i) = *_vertex_safe(i + 1);
		}

	vCount--;
}

void Winding::OnMapFree() {
	Clear();
}

void Winding::OnMapSave()
{
	if (GetStatus() != OK)
	{
		//Log::Print("OnSave reallocating windings\n");
		Allocate(0, true);
		//Log::Print("  Windings rebuilding map\n");
		g_map.BuildBrushData();
		flushed = false;
		LogStatus();
	}
}

void Winding::OnBeforeMapRebuild()
{
	realloc_status stat = GetStatus();
	if (stat == URGENT || stat == IMMEDIATE)
	{
		//Log::Print("BeforeMapRebuild reallocating windings\n");
		Allocate(0, false);
		LogStatus();
	}
}

void Winding::OnCommandComplete()
{
	realloc_status stat = GetStatus();
	if (stat == URGENT || stat == IMMEDIATE)
	{
		//Log::Print("OnCmdComplete reallocating windings\n");
		Allocate(0, (_vsFreeSegs / (float)_vsCapacity) > 0.2f);	// trigger rebuild if fragmentation is at 20%
		//Log::Print("  Windings rebuilding map\n");
		g_map.BuildBrushData();
		flushed = false;
		LogStatus();
	}
}

//===============================================================================

void Winding::Allocate(const int vMinPoints, bool flush)
{
	uint32_t min = max(vMinPoints, g_map.numBrushes * 32);
	uint32_t newCapacity = max( ((v2vs(min) / vsWINDING_BLOCK_SIZE)) * vsWINDING_BLOCK_SIZE, _vsCapacity) + vsWINDING_BLOCK_SIZE;

	if (flush)
	{
		flushed = true;	// so windings know their sizes are invalid now

		// don't copy data, require map to rebuild all instead as a defragmentation strategy
		//	this must only be done in the context of a full rebuild or lots of windings will 
		//	be very stale
		Clear();

		_pool = (vertex_seg_t*)malloc(newCapacity * sizeof(vertex_seg_t));
		if (!_pool)
			throw new std::exception("Winding::Allocate failed! that's p. bad\n");
	}
	else
	{
		// copy old pool
		// free list is stored as indices and not pointers so this is safe anytime
		vertex_seg_t* oldpool = _pool;
		_pool = (vertex_seg_t*)malloc(newCapacity * sizeof(vertex_seg_t));
		if (!_pool)
			throw new std::exception("Winding::Allocate failed! that's p. bad\n");
		if (oldpool)
		{
			memcpy(_pool, oldpool, _vsMargin * sizeof(vertex_seg_t));
			free(oldpool);
		}
	}
	_vsCapacity = newCapacity;
	Log::Print(_S("Winding pool reallocated to %d %s\n") << (int)_vsCapacity << (flush ? "and flushed" : ""));
}

void Winding::Clear()
{
	if (_pool) free(_pool);
	_pool = nullptr;
	_vsFreeHead = vsNULL_INDEX;
	_vsFreeSegs = 0;
	_vsMargin = 0;
	_vsCapacity = 0;
}

Winding::realloc_status const Winding::GetStatus()
{
	int remaining = _vsCapacity - _vsMargin;
	if (remaining < 80)
		return IMMEDIATE;
	else if (remaining < 320)
		return URGENT;
	else if (remaining < 1280)
		return PRUDENT;
	return OK;
}

void Winding::LogStatus()
{
	Log::Print(_S("Windings: %d/%d, %d frags\n") << (int)_vsMargin << (int)_vsCapacity << (int)_vsFreeSegs );
}


//===============================================================================
#ifdef OLDWINDING
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
		Error(_S("Winding::Alloc: %d points.") << points);

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
	//assert((int)first->next != 0x45400000);
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
	assert(w->maxpoints <= MAX_POINTS_ON_WINDING);
	assert(w->maxpoints >= 6);

	int maxW = w->maxpoints / 6;

	//memset(w, 0xEF, sizeof(winding_t) * maxW);
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

	if (point < w->numpoints - 1)
		//memmove(&w->points[point], &w->points[point + 1], (int)((winding_t *)0)->points[w->numpoints - point - 1]);
		for (int i = point; i < w->numpoints - 1; ++i)
		{
			w->points[i] = w->points[i + 1];
		}

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
	int		i, j;
	int		counts[3];
//	int		maxpts;
	int		sides[MAX_POINTS_ON_WINDING];
	double	dists[MAX_POINTS_ON_WINDING];
	double	dot;
	dvec3	p1, p2;
	dvec3	mid;

	// lunaran - scratch max-size winding for working in place, to reduce constant winding alloc/free
	struct {
		int		numpoints;
		int		maxpoints;
		windingpoint_t	points[MAX_POINTS_ON_WINDING];
	} neww;

	neww.numpoints = 0;
	neww.maxpoints = MAX_POINTS_ON_WINDING;
	
	counts[SIDE_FRONT] = counts[SIDE_BACK] = counts[SIDE_ON] = 0;

	// determine sides for each point
	for (i = 0; i < in->numpoints; i++)
	{
		dot = DotProduct(dvec3(in->points[i].point), split->normal);
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
		p1 = in->points[i].point;
		
		if (sides[i] == SIDE_ON)
		{
			neww.points[neww.numpoints].point = p1;
			neww.numpoints++;
			continue;
		}
	
		if (sides[i] == SIDE_FRONT)
		{
			neww.points[neww.numpoints].point = p1;
			neww.numpoints++;
		}
		
		if (sides[i + 1] == SIDE_ON || sides[i + 1] == sides[i])
			continue;
			
		// generate a split point
		p2 = in->points[(i + 1) % in->numpoints].point;
		
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
			
		neww.points[neww.numpoints].point = mid;
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


bool Winding::WindingsEqual(winding_t *w1, winding_t *w2, bool flip)
{
	int offset, i, io;

	if (w1->numpoints != w2->numpoints)
		return false;

	offset = -1;

	for (i = 0; i < w2->numpoints; i++)
	{
		if (w1->points[0].point == w2->points[i].point)
		{
			offset = i;
			break;
		}
	}
	if (offset == -1)
		return false;	// no points in common

	// w1 point 0 == w2 point 'offset'
	for (i = 0; i < w1->numpoints; i++)
	{
		io = ((flip ? offset - i: offset + i) + w1->numpoints) % w1->numpoints;
		if (w1->points[i].point != w2->points[io].point)
			return false;
	}
	return true;
}


/*
=============
Winding::PlanesConcave
=============
*/
bool Winding::PlanesConcave(winding_t *w1, winding_t *w2,
							const vec3 normal1, const vec3 normal2,
							float dist1, float dist2)
{
	int i;

	if (!w1 || !w2) 
		return false;

	// check if one of the points of winding 1 is at the back of the plane of winding 2
	for (i = 0; i < w1->numpoints; i++)
		if (DotProduct(normal2, w1->points[i].point) - dist2 > WCONVEX_EPSILON) 
			return true;

	// check if one of the points of winding 2 is at the back of the plane of winding 1
	for (i = 0; i < w2->numpoints; i++)
		if (DotProduct(normal1, w2->points[i].point) - dist1 > WCONVEX_EPSILON)
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
winding_t *Winding::TryMerge (winding_t &f1, winding_t &f2, const vec3 planenormal, int keep)
{
	int			i, j, k, l;
	bool		keep1, keep2;
	float		dot;
	vec3		*p1, *p2, *p3, *p4, *back;
	vec3		normal, delta;
	winding_t	*newf;
	
	// find a common edge
	p1 = p2 = NULL;	// stop compiler warning
	j = 0;			// 
	
	for (i = 0; i < f1.numpoints; i++)
	{
		p1 = &f1.points[i].point;
		p2 = &f1.points[(i + 1) % f1.numpoints].point;

		for (j = 0; j < f2.numpoints; j++)
		{
			p3 = &f2.points[j].point;
			p4 = &f2.points[(j + 1) % f2.numpoints].point;

			for (k = 0; k < 3; k++)
			{
				if (fabs((*p1)[k] - (*p4)[k]) > 0.1)//EQUAL_EPSILON) //ME
					break;
				if (fabs((*p2)[k] - (*p3)[k]) > 0.1)//EQUAL_EPSILON) //ME
					break;
			}

			if (k==3)
				break;
		}

		if (j < f2.numpoints)
			break;
	}
	
	if (i == f1.numpoints)
		return NULL;			// no matching edges

	// check slope of connected lines
	// if the slopes are colinear, the point can be removed
	back = &f1.points[(i + f1.numpoints - 1) % f1.numpoints].point;
	delta = *p1 - *back;
	normal = CrossProduct(planenormal, delta);
	VectorNormalize(normal);
	
	back = &f2.points[(j + 2) % f2.numpoints].point;
	delta = *back - *p1;
	dot = DotProduct(delta, normal);

	if (dot > CONTINUOUS_EPSILON)
		return NULL;			// not a convex polygon

	keep1 = (bool)(dot < -CONTINUOUS_EPSILON);
	back = &f1.points[(i + 2) % f1.numpoints].point;
	delta = *back - *p2;
	normal = CrossProduct(planenormal, delta);
	VectorNormalize(normal);

	back = &f2.points[(j + f2.numpoints - 1) % f2.numpoints].point;
	delta = *back - *p2;
	dot = DotProduct(delta, normal);

	if (dot > CONTINUOUS_EPSILON)
		return NULL;			// not a convex polygon
	
	keep2 = (bool)(dot < -CONTINUOUS_EPSILON);

	// build the new polygon
	newf = Winding::Alloc(f1.numpoints + f2.numpoints);
	
	// copy first polygon
	for (k = (i + 1) % f1.numpoints; k != i; k = (k + 1) % f1.numpoints)
	{
		if (!keep && k == (i + 1) % f1.numpoints && !keep2)
			continue;
		
		newf->points[newf->numpoints].point = f1.points[k].point;
		newf->numpoints++;
	}
	
	// copy second polygon
	for (l = (j + 1) % f2.numpoints; l != j; l = (l + 1) % f2.numpoints)
	{
		if (!keep && l == (j + 1) % f2.numpoints && !keep1)
			continue;

		newf->points[newf->numpoints].point = f2.points[l].point;
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
	vec3		vecs[2];
	TexDef	*texdef;
	windingpoint_t		*xyzst;

	for (int i = 0; i < w->numpoints; i++)
	{
		xyzst = &w->points[i];

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

		s = DotProduct(xyzst->point, vecs[0]);
		t = DotProduct(xyzst->point, vecs[1]);

		ns = cosv * s - sinv * t;
		nt = sinv * s + cosv * t;

		s = ns / texdef->scale[0] + texdef->shift[0];
		t = nt / texdef->scale[1] + texdef->shift[1];

		// gl scales everything from 0 to 1
		s /= q->width;
		t /= q->height;

		xyzst->s = s;
		xyzst->t = t;
	}
}

int Winding::MemorySize(winding_t * w)
{
	return sizeof(winding_t);
}

vec3 Winding::Centroid(winding_t * w)
{
	vec3 a, b, c;
	c = vec3(0);
	for (int i = 0; i < w->numpoints; i++)
	{
		a = w->points[i].point;
		b = w->points[(i+1)%w->numpoints].point;
		c += (a + b) * 0.5f;
	}
	return c / (float)w->numpoints;
}

#endif
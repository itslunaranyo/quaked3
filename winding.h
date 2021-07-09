//==============================
//	winding.h
//==============================
#ifndef __WINDING_H__
#define __WINDING_H__

#define WCONVEX_EPSILON		0.2
#define	DIST_EPSILON		0.02
#define	CONTINUOUS_EPSILON	0.005
#define	ZERO_EPSILON		0.001
#define	NORMAL_EPSILON		0.0001
#define	BOGUS_RANGE			18000


//====================================================================

typedef unsigned char byte;

struct windingpoint_t
{
	vec3 point;
	float s, t;
};

class NuWinding
{
public:
	NuWinding() {};
	~NuWinding();
	void Grow(const int vMinPoints = 0, const bool noFrags = false);
	void Free();

	struct vertex_t
	{
		vec3 point;
		float s, t;
		float shade;
	};

	//inline vertex_t* GetPoints() const { return (vertex_t*)(_pool + vsIndex); }
	vertex_t* operator[](const int i);

	enum realloc_status {
		OK,			// no need to reallocate at present
		PRUDENT,	// reallocate after next save
		URGENT,		// reallocate after next save or cmdQueue op
		EMERGENCY	// reallocate immediately
	};

	inline realloc_status GetStatus() const { return _status; }

private:
	byte vCount = 0, vMaxPts = 0;	// measured in singular vertices
	uint32_t vsIndex = -1;	// SEG offset
	// 8 MSBs guaranteed to never be used (24 bits is enough for ~2mil brushes)

	static constexpr int segSize = 4;
	struct vertex_seg_t { vertex_t v[segSize]; };
	struct free_vseg_t {
		uint32_t vsLoc = 0;
		byte vsSize = 0;
		free_vseg_t* next = nullptr;
	};
	static vertex_seg_t* _pool;
	static uint32_t _vsCapacity;
	// pool retains a high water mark with all empty space above and a fragmented free list below
	static uint32_t _vsMargin;
	static free_vseg_t* _freeList;
	static realloc_status _status;

	static inline uint32_t v2vs(int x) { return (x + segSize - 1) / segSize; }
	static inline uint32_t vs2v(int x) { return x * segSize; }
	static void Allocate(const int vMinPoints = 0);
	static void SetStatus();
	inline bool IsHighmost() const { return (vsIndex + v2vs(vMaxPts) == _vsMargin); }
};

struct winding_t
{
	int		numpoints;
	int		maxpoints;
	windingpoint_t 	points[6];	// variable sized
};

struct free_winding_t {
	int maxWindings;	// winding points / 6
	free_winding_t* next;
	free_winding_t* prev;	// is this needed?
};

namespace Winding
{
	void Clear();	// mass reset of the winding pool
	void Test();	// test

	void		CheckFreeChain();	// run over the chain of free windings and make sure nothing's fucky

	free_winding_t* AllocPage();	// actually grab memory for new windings
	winding_t	*Alloc(int points);	// allocate a winding from the pool
	void		Free(winding_t *w);	// free the winding
	winding_t	*Clone(winding_t *w);	// make a winding clone
	void		Copy(winding_t *src, winding_t *dest);	// copy one winding into another

	winding_t	*Clip(winding_t *in, Plane *split, bool keepon);	// clip the winding with the plane
	winding_t	*TryMerge(winding_t &f1, winding_t &f2, vec3 planenormal, int keep);	// try to merge the windings, returns the new merged winding or NULL
	void		RemovePoint(winding_t *w, int point);	// remove a point from the winding
	void		TextureCoordinates(winding_t *w, Texture *q, Face *f);	// compute s/t coords for textured face winding

	bool		WindingsEqual(winding_t *w1, winding_t *w2, bool flip);
	bool		PlanesConcave(winding_t *w1, winding_t *w2, const vec3 normal1, const vec3 normal2, float dist1, float dist2);	// returns true if the planes are concave
	int			MemorySize(winding_t *w);	//dum

	vec3		Centroid(winding_t *w);
}
#endif

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
class Face;

class Winding
{
public:
	Winding();
	~Winding();
	void Grow(const int vMinPoints = 0, const bool noFrags = false);
	void Free();
	void Swap(Winding& other);

	struct vertex_t
	{
		vec3 point;
		float s, t;
		float shade;
	};

	vertex_t* const Vertex(const int i);
	inline vertex_t* operator[](const int i) { return Vertex(i); }

	inline int	const Count() { return vCount; }
	void		const AddBounds(vec3& mins, vec3& maxs);
	vec3		const Centroid();
	bool		const Equal(Winding& w2, bool flip);
	Plane::side	const PlaneSides(Plane& split);	// returns FRONT if entirely positive to, BACK if negative, ON if straddling

	void Base(Plane& p);
	bool Clip(Plane& split, bool keepon);	// returns false if winding was clipped away
	void TextureCoordinates(Texture& q, Face& f);	// compute s/t coords for textured face winding
	void RemovePoint(int point);	// remove a point from the winding

	//inline vertex_t* GetPoints() const { return (vertex_t*)(_pool + vsIndex); }
	enum realloc_status {
		OK,			// no need to reallocate at present
		PRUDENT,	// reallocate after next save
		URGENT,		// reallocate after next save or cmdQueue op
		IMMEDIATE	// reallocate immediately
	};

	static realloc_status const GetStatus();
	static void LogStatus();

	static void OnMapFree();
	static void OnMapSave();
	static void OnBeforeMapRebuild();
	static void OnCommandComplete();

protected:

private:
	byte vCount = 0, vMaxPts = 0;	// measured in singular vertices
	uint32_t vsIndex = -1;	// SEG offset
	// 8 MSBs guaranteed to never be used (24 bits is enough for ~2mil brushes)

	inline vertex_t* _vertex_safe(const int i) { return (vertex_t*)(_pool + vsIndex) + i; }

	static constexpr int segSize = 4;
	struct vertex_seg_t { vertex_t v[segSize]; };
	struct free_vseg_t {
		byte vsSize = 0;
		uint32_t vsNext = -1;
	};
	static vertex_seg_t* _pool;
	static uint32_t _vsCapacity;

	static uint32_t _vsMargin;	// high water mark, empty space above and fragmented free list below
	static uint32_t _vsFreeHead;	// index of first freed seg
	static uint32_t _vsFreeSegs;	// total freed seg count for fragmentation guess
	static bool flushed;	// signal that previous winding buffer is trash

	static inline uint32_t v2vs(int x) { return (x + segSize - 1) / segSize; }
	static inline uint32_t vs2v(int x) { return x * segSize; }
	static void Allocate(const int vMinPoints = 0, bool flush = false);
	static void Clear();
	inline bool IsHighmost() const { return (vsIndex + v2vs(vMaxPts) == _vsMargin); }
};

#ifdef OLDWINDING
struct windingpoint_t
{
	vec3 point;
	float s, t;
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
#endif

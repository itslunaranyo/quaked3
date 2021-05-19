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

typedef struct winding_s
{
	int		numpoints;
	int		maxpoints;
	float 	points[6][5];	// variable sized
} winding_t;

typedef struct free_winding_s {
	int maxWindings;	// winding points / 6
	free_winding_s* next;
	free_winding_s* prev;
} free_winding_t;

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
	winding_t	*TryMerge(winding_t *f1, winding_t *f2, vec3_t planenormal, int keep);	// try to merge the windings, returns the new merged winding or NULL
	void		RemovePoint(winding_t *w, int point);	// remove a point from the winding
	void		TextureCoordinates(winding_t *w, Texture *q, Face *f);	// compute s/t coords for textured face winding

	bool		PlanesConcave(winding_t *w1, winding_t *w2, vec3_t normal1, vec3_t normal2, float dist1, float dist2);	// returns true if the planes are concave
	int			MemorySize(winding_t *w);	//dum
}
#endif

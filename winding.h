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

// returns true if the planes are equal
bool Plane_Equal (plane_t *a, plane_t *b, int flip);
// returns false if the points are colinear
bool Plane_FromPoints (vec3_t p1, vec3_t p2, vec3_t p3, plane_t *plane);
// returns true if the points are equal
bool Point_Equal (vec3_t p1, vec3_t p2, float epsilon);

// allocate a winding
winding_t *Winding_Alloc (int points);
// make a winding clone
winding_t *Winding_Clone (winding_t *w);
// clip the winding with the plane
winding_t *Winding_Clip (winding_t *in, plane_t *split, bool keepon);
// try to merge the windings, returns the new merged winding or NULL
winding_t *Winding_TryMerge (winding_t *f1, winding_t *f2, vec3_t planenormal, int keep);
// free the winding
void Winding_Free (winding_t *w);
// remove a point from the winding
void Winding_RemovePoint (winding_t *w, int point);
// returns true if the planes are concave
bool Winding_PlanesConcave (winding_t *w1, winding_t *w2, vec3_t normal1, vec3_t normal2, float dist1, float dist2);

#endif

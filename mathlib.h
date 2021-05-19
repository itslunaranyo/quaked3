//==============================
//	mathlib.h
//==============================

#ifndef __MATHLIB_H__
#define __MATHLIB_H__

#include <math.h>
#include <glm/glm.hpp>


#define	SIDE_FRONT		0
#define	SIDE_ON			2
#define	SIDE_BACK		1
#define	SIDE_CROSS	   -2

#define	Q_PI		3.14159265358979323846
#define Q_DEG2RAD	0.01745329251994329577
#define Q_RAD2DEG	57.2957795130823208768

#define	EQUAL_EPSILON	0.001

//========================================================================

typedef glm::vec3	vec3;

typedef float	vec_t;
//typedef vec_t	vec3_t[3];

/*
#define DotProduct(x, y)		(x[0] * y[0] + x[1] * y[1] + x[2] * y[2])
#define VectorSubtract(a, b, c) {c[0] = a[0] - b[0]; c[1] = a[1] - b[1]; c[2] = a[2] - b[2];}
#define VectorAdd(a, b, c)		{c[0] = a[0] + b[0]; c[1] = a[1] + b[1]; c[2] = a[2] + b[2];}
#define VectorCopy(a, b)		{b[0] = a[0]; b[1] = a[1]; b[2] = a[2];}
#define VectorSet(v, a, b, c)	{v[0] = a; v[1] = b; v[2] = c;}
*/
inline float DotProduct(const vec3 x, const vec3 y) { return glm::dot(x, y); }
/*
inline void VectorSubtract(const vec3 a, const vec3 b, vec3 &c) { c = a - b; }
inline void VectorAdd(const vec3 a, const vec3 b, vec3 &c) { c = a + b; }
inline void VectorCopy(const vec3 a, vec3 &b) { b = a; }
inline void VectorSet(vec3 &v, const float a, const float b, const float c) { v = vec3(a, b, c); }
inline void VectorSet(vec3 &v, const int a, const int b, const int c) { v = vec3(a, b, c); }
*/

//========================================================================

vec_t	Q_rint (vec_t in);

// lunaran: glm blows these defines away somehow
inline int min(int a, int b) { if (a < b) return a; return b; }
inline int max(int a, int b) { if (a > b) return a; return b; }
inline float min(float a, float b) { if (a < b) return a; return b; }
inline float max(float a, float b) { if (a > b) return a; return b; }

/*
inline vec_t _DotProduct(const vec3 v1, const vec3 v2) { return glm::dot(v1, v2); }
inline void _VectorSubtract(const vec3 va, const vec3 vb, vec3 &out) { out = va - vb; }
inline void _VectorAdd(const vec3 va, const vec3 vb, vec3 &out) { out = va + vb; }
inline void _VectorCopy(const vec3 in, vec3 &out) { out = in; }
*/
bool	VectorCompare(const vec3 v1, const vec3 v2);
inline float VectorLength(const vec3 v) { return glm::length(v); }

//inline void VectorMA(const vec3 va, const float scale, const vec3 vb, vec3 &vc) { vc = va + scale * vb; }

vec_t	VectorNormalize(vec3 &v);
inline void VectorInverse(vec3 &v) { v = -v; }
//inline void VectorScale(const vec3 v, const vec_t scale, vec3 &out) { out = v * scale; }
void	VectorRotate(const vec3 vIn, const vec3 vRotation, vec3 &out);
void	VectorRotate2(const vec3 vIn, const vec3 vRotation, const vec3 vOrigin, vec3 &out);
void	VectorPolar(vec3 &v, const float radius, const float theta, const float phi);	// sikk - Brush Primitives

inline vec3 CrossProduct(const vec3 v1, const vec3 v2) { return glm::cross(v1, v2); }

inline void ClearBounds(vec3 &mins, vec3 &maxs) { mins = vec3(99999); maxs = vec3(-99999); }
void	AddPointToBounds(const vec3 v, vec3 &mins, vec3 &maxs);

void	AngleVectors(const vec3 angles, vec3 &forward, vec3 &right, vec3 &up);
void	VectorToAngles(const vec3 vec, vec3 &angles);

bool	Point_Equal(const vec3 p1, const vec3 p2, const float epsilon); // returns true if the points are equal

/*
vec_t	_DotProduct (vec3_t v1, vec3_t v2);
void	_VectorSubtract (vec3_t va, vec3_t vb, vec3_t out);
void	_VectorAdd (vec3_t va, vec3_t vb, vec3_t out);
void	_VectorCopy (vec3_t in, vec3_t out);

bool	VectorCompare (vec3_t v1, vec3_t v2);
float	VectorLength(vec3_t v);

void	VectorMA (vec3_t va, float scale, vec3_t vb, vec3_t vc);

vec_t	VectorNormalize (vec3_t v);
void	VectorInverse (vec3_t v);
void	VectorScale (vec3_t v, vec_t scale, vec3_t out);
void	VectorRotate (vec3_t vIn, vec3_t vRotation, vec3_t out);
void	VectorRotate2 (vec3_t vIn, vec3_t vRotation, vec3_t vOrigin, vec3_t out);
void	VectorPolar (vec3_t v, float radius, float theta, float phi);	// sikk - Brush Primitives

void	CrossProduct (vec3_t v1, vec3_t v2, vec3_t cross);

void	ClearBounds (vec3_t mins, vec3_t maxs);
void	AddPointToBounds (vec3_t v, vec3_t mins, vec3_t maxs);

void	AngleVectors (vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
void	VectorToAngles (vec3_t vec, vec3_t angles);

bool	Point_Equal(vec3_t p1, vec3_t p2, float epsilon); // returns true if the points are equal
*/

#endif

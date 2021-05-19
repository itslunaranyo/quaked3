//==============================
//	mathlib.h
//==============================

#ifndef __MATHLIB_H__
#define __MATHLIB_H__

#include <math.h>


#define	SIDE_FRONT		0
#define	SIDE_ON			2
#define	SIDE_BACK		1
#define	SIDE_CROSS	   -2

#define	Q_PI		3.14159265358979323846
#define Q_DEG2RAD	0.01745329251994329577
#define Q_RAD2DEG	57.2957795130823208768

#define	EQUAL_EPSILON	0.001

#define DotProduct(x, y)		(x[0] * y[0] + x[1] * y[1] + x[2] * y[2])
#define VectorSubtract(a, b, c) {c[0] = a[0] - b[0]; c[1] = a[1] - b[1]; c[2] = a[2] - b[2];}
#define VectorAdd(a, b, c)		{c[0] = a[0] + b[0]; c[1] = a[1] + b[1]; c[2] = a[2] + b[2];}
#define VectorCopy(a, b)		{b[0] = a[0]; b[1] = a[1]; b[2] = a[2];}
#define VectorSet(v, a, b, c)	{v[0] = a; v[1] = b; v[2] = c;}

//========================================================================

typedef float	vec_t;
typedef vec_t	vec3_t[3];

extern vec3_t	g_v3VecOrigin;

//========================================================================

vec_t	Q_rint (vec_t in);
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

#endif

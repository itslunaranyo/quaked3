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
typedef glm::dvec3	dvec3;
typedef glm::mat4	mat4;

//========================================================================

inline int qround(const float i, const int r) { return std::lroundf(i / r) * r; }
inline int qround(const float i) { return std::lroundf(i); }

// lunaran: glm blows these defines away somehow
inline int min(int a, int b) { if (a < b) return a; return b; }
inline int max(int a, int b) { if (a > b) return a; return b; }
inline float min(float a, float b) { if (a < b) return a; return b; }
inline float max(float a, float b) { if (a > b) return a; return b; }
inline uint32_t min(uint32_t a, uint32_t b) { if (a < b) return a; return b; }
inline uint32_t max(uint32_t a, uint32_t b) { if (a > b) return a; return b; }

bool	VectorCompareLT(const vec3 &v1, const vec3 &v2);
bool	VectorCompare(const vec3 &v1, const vec3 &v2);
inline float VectorLength(const vec3 v) { return glm::length(v); }
inline float DotProduct(const vec3 x, const vec3 y) { return glm::dot(x, y); }
inline double DotProduct(const dvec3 x, const dvec3 y) { return glm::dot(x, y); }
vec3	VectorPerpendicular(const vec3 u);
vec3	ZeroTinyComponents(const vec3 v);

bool	LineSegmentIntersect2D(const vec3 l1start, const vec3 l1end, const vec3 l2start, const vec3 l2end, vec3 &intersect);

float	VectorNormalize(vec3 &v);
double	VectorNormalize(dvec3 &v);
inline void VectorInverse(vec3 &v) { v = -v; }
void	VectorRotate(const vec3 vIn, const vec3 vRotation, vec3 &out);
void	VectorRotate(const dvec3 vIn, const dvec3 vRotation, dvec3 &out);
void	VectorRotate2(const dvec3 vIn, const dvec3 vRotation, const dvec3 vOrigin, dvec3 &out);
void	VectorPolar(vec3 &v, const float radius, const float theta, const float phi);	// sikk - Brush Primitives

inline vec3 CrossProduct(const vec3 v1, const vec3 v2) { return glm::cross(v1, v2); }
inline dvec3 CrossProduct(const dvec3 v1, const dvec3 v2) { return glm::cross(v1, v2); }

inline void ClearBounds(vec3 &mins, vec3 &maxs) { mins = vec3(99999); maxs = vec3(-99999); }
void	AddPointToBounds(const vec3 v, vec3 &mins, vec3 &maxs);

void	AngleVectors(const vec3 angles, vec3 &forward, vec3 &right, vec3 &up);
void	VectorToAngles(const vec3 vec, vec3 &angles);

glm::mat4 RotateMatrix(glm::mat4 mat, float pitch, float yaw);

bool	Point_Equal(const vec3 p1, const vec3 p2, const float epsilon); // returns true if the points are equal

void	rgbToHex(const vec3 vrgb, char *hex);
void	hexToRGB(const char *hex, vec3 &vrgb);


#endif

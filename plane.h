#ifndef __PLANE_H__
#define __PLANE_H__

//==============================
//	plane.h
//==============================

struct winding_s;
typedef winding_s winding_t;

extern const vec3 g_v3BaseAxis[18];

class Plane
{
public:
	Plane();
	vec3	pts[3];
	dvec3	normal;
	double	dist;

	bool		PointOn(const vec3 &pt) const;
	bool		EqualTo(const Plane *b, const int flip) const;	// returns true if the planes are equal-ish
	bool		ConvexTo(const Plane *other) const;
	bool		FromPoints(const vec3 p0, const vec3 p1, const vec3 p2);	// returns false if the points are collinear
	bool		FromNormDist(const vec3 n, const double d);
	bool		FromNormPoint(const vec3 n, const vec3 pt);
	bool		TestRay(const vec3 org, const vec3 dir, vec3 &out) const;
	bool		ClipLine(vec3 &p1, vec3 &p2);
	bool		Make();
	void		Flip();
	void		Translate(const vec3 move);
	void		Snap(int increment = 1);
	winding_t	*BasePoly();
	vec3		GetTextureAxis(vec3 &xv, vec3 &yv) const;
	dvec3		GetTextureAxis(dvec3 &xv, dvec3 &yv) const;
	vec3		ProjectPointAxial(const vec3 &in, const vec3 &axis) const;
	dvec3		ProjectPointAxial(const dvec3 &in, const dvec3 &axis) const;
	vec3		ProjectVectorAxial(const vec3 &in, const vec3 &axis) const;
};


#endif
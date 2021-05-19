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
	vec3	normal;
	double	dist;

	bool		EqualTo(Plane *b, int flip);	// returns true if the planes are equal-ish
	bool		FromPoints(const vec3 p0, const vec3 p1, const vec3 p2);	// returns false if the points are collinear
	bool		ClipLine(vec3 &p1, vec3 &p2);
	bool		Make();
	void		Flip();
	void		Translate(vec3 move);
	void		Snap(int increment = 1);
	winding_t	*BasePoly();
	void		GetTextureAxis(vec3 &xv, vec3 &yv);
};


#endif
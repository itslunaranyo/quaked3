//==============================
//	plane.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "winding.h"

const vec3 g_v3BaseAxis[18] =
{
	vec3(0, 0, 1), vec3(1, 0, 0), vec3(0,-1, 0),	// floor
	vec3(0, 0,-1), vec3(1, 0, 0), vec3(0,-1, 0),	// ceiling
	vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0,-1),	// west wall
	vec3(-1, 0, 0), vec3(0, 1, 0), vec3(0, 0,-1),	// east wall
	vec3(0, 1, 0), vec3(1, 0, 0), vec3(0, 0,-1),	// south wall
	vec3(0,-1, 0), vec3(1, 0, 0), vec3(0, 0,-1)		// north wall
};


//==========================================================================


/*
================
Plane::Plane
================
*/
Plane::Plane() : dist(0), normal(0)
{
}

bool Plane::PointOn(const vec3 &pt) const
{
	return (fabs(DotProduct(dvec3(pt), normal) - dist) < ON_EPSILON);
}

/*
=============
Plane::EqualTo
=============
*/
bool Plane::EqualTo(const Plane *b, const int flip) const
{
	double	tdist;
	dvec3	tnorm;

	if (flip == SIDE_ON)	// planes can be equal or inverse
	{
		return (EqualTo(b, false) || EqualTo(b, true));
	}
	else if (flip == SIDE_BACK)	// planes must be inverse
	{
		tnorm = normal + b->normal;
		tdist = dist + b->dist;
	}
	else	// SIDE_FRONT	// planes must be equal
	{
		tnorm = normal - b->normal;
		tdist = dist - b->dist;
	}

	if (fabs(tnorm[0]) < NORMAL_EPSILON &&
		fabs(tnorm[1]) < NORMAL_EPSILON &&
		fabs(tnorm[2]) < NORMAL_EPSILON &&
		fabs(tdist) < DIST_EPSILON)
		return true;

	return false;
}

/*
=============
Plane::ConvexTo

any two non-coincident planes in space aren't really concave or convex to
each other, since they extend to infinity and thus are concave in half
of infinity and convex in the other half. thus any convexity test for two
planes must be made relative to points somewhere on that plane, to determine
which half of infinity those points are in, which is just a sidedness test.

this function assumes the plane points are in the neighborhood the calling
code is interested in (ideally corresponding to face winding points), 
otherwise the result is fairly meaningless.
=============
*/
bool Plane::ConvexTo(const Plane *other) const
{
	dvec3 p;

	p = (pts[0] + pts[1] + pts[2]) / 3.0f;
	if (DotProduct(p, other->normal) >= other->dist)
		return false;

	p = (other->pts[0] + other->pts[1] + other->pts[2]) / 3.0f;
	if (DotProduct(p, normal) >= dist)
		return false;

	return true;
}

/*
============
Plane::FromPoints
returns false and does not modify the plane if the points are collinear
============
*/
bool Plane::FromPoints(const vec3 p0, const vec3 p1, const vec3 p2)
{
	dvec3	pNorm;

	pNorm = CrossProduct(dvec3(p0) - dvec3(p1), dvec3(p2) - dvec3(p1));

	if (VectorNormalize(pNorm) < 0.1)
		return false;

	normal = pNorm;
	dist = DotProduct(dvec3(p0), pNorm);
	pts[0] = p0;
	pts[1] = p1;
	pts[2] = p2;

	return true;
}

bool Plane::FromNormDist(const vec3 n, const double d)
{
	normal = n;
	dist = d;
	return true;
}

bool Plane::FromNormPoint(const vec3 n, const vec3 pt)
{
	normal = n;
	dist = DotProduct(dvec3(pt), normal);
	return true;
}

bool Plane::TestRay(const vec3 org, const vec3 dir, vec3 &out) const
{
	float	d1, d2, fr;
	dvec3	dorg = dvec3(org);

	d1 = DotProduct(dorg, normal) - dist;
	d2 = DotProduct(dorg + dvec3(dir) * (double)g_cfgEditor.MapSize, normal) - dist;
	if (d1 == d2)	// FIXME precision
		return false;	// parallel to plane

	fr = d1 / (d1 - d2);

	out = org + dir * (float)g_cfgEditor.MapSize * fr;
	return true;
}

bool Plane::ClipLine(vec3 &p1, vec3 &p2)
{
	int		i;
	float	d1, d2, fr;
	vec3	*v;

	d1 = DotProduct(dvec3(p1), normal) - dist;
	d2 = DotProduct(dvec3(p2), normal) - dist;

	if (d1 >= 0 && d2 >= 0)
		return false;	// totally outside
	if (d1 <= 0 && d2 <= 0)
		return true;	// totally inside

	fr = d1 / (d1 - d2);

	if (d1 > 0)
		v = &p1;
	else
		v = &p2;

	for (i = 0; i < 3; i++)
		(*v)[i] = p1[i] + fr * (p2[i] - p1[i]);
	//(*v) = p1 + fr * (p2 - p1);

	return true;
}

/*
=================
Plane::Make
=================
*/
bool Plane::Make()
{
	vec3 norm;
	norm = CrossProduct(pts[0] - pts[1], pts[2] - pts[1]);

	if (VectorCompare(norm, vec3(0)) || VectorNormalize(norm) < 0.05f)
	{
		Warning("Brush plane with no normal");
		return false;
	}

	normal = norm;
	dist = DotProduct(dvec3(pts[1]), normal);
	return true;
}

/*
=================
Plane::Flip
=================
*/
void Plane::Flip()
{
	/*
	vec3 temp;
	temp = pts[0];
	pts[0] = pts[1];
	pts[1] = temp;
	*/
	std::swap(pts[0], pts[1]);
	Make();
}

/*
=================
Plane::Translate
=================
*/
void Plane::Translate(const vec3 move)
{
	pts[0] += move;
	pts[1] += move;
	pts[2] += move;
	dist = DotProduct(dvec3(pts[0]), normal);
}

/*
=================
Plane::Snap
=================
*/
void Plane::Snap(int increment)
{
	for (int i = 0; i < 3; i++)
	{
		pts[i] = glm::floor(pts[i] / (float)increment + vec3(0.5)) * (float)increment;
	}
	Make();
}

/*
=================
Plane::BasePoly
=================
*/
winding_t *Plane::BasePoly()
{
	dvec3		org, vright, vup;
	winding_t  *w;

	/*
	int			i, x;
	float		max, v;

	// find the major axis
	max = -BOGUS_RANGE;
	x = -1;

	for (i = 0; i < 3; i++)
	{
		v = fabs(normal[i]);
		if (v > max)
		{
			x = i;
			max = v;
		}
	}

	if (x == -1)
		Error("BasePolyForPlane: No axis found.");

	vup = vec3(0);

	switch (x)
	{
	case 0:
	case 1:
		vup[2] = 1;
		break;
	case 2:
		vup[0] = 1;
		break;
	}

	v = DotProduct(vup, normal);
	vup = vup + -v * normal;
	VectorNormalize(vup);
	org = normal * (float)dist;
	vright = CrossProduct(vup, normal);
	*/

	// lunaran - this made it shorter and removed all the branches, and made it zero percent faster :(
	org = normal * dist;
	vup = VectorPerpendicular(normal);
	vright = CrossProduct(vup, normal);

	vup = vup * (double)g_cfgEditor.MapSize;
	vright = vright * (double)g_cfgEditor.MapSize;

	// project a really big	axis aligned box onto the plane
	w = Winding::Alloc(4);

	w->points[0].point = org - vright + vup;
	w->points[1].point = org + vright + vup;
	w->points[2].point = org + vright - vup;
	w->points[3].point = org - vright - vup;

	w->numpoints = 4;

	return w;
}

/*
==================
Plane::GetTextureAxis
==================
*/
vec3 Plane::GetTextureAxis(vec3 &xv, vec3 &yv) const
{
	int		i, bestaxis;
	float	dot, best;

	best = 0;
	bestaxis = 0;

	for (i = 0; i < 6; i++)
	{
		dot = DotProduct(normal, dvec3(g_v3BaseAxis[i * 3]));
		if (dot > best)
		{
			best = dot;
			bestaxis = i;
		}
	}

	xv = g_v3BaseAxis[bestaxis * 3 + 1];
	yv = g_v3BaseAxis[bestaxis * 3 + 2];
	return g_v3BaseAxis[bestaxis * 3];
}

dvec3 Plane::GetTextureAxis(dvec3 &xv, dvec3 &yv) const
{
	vec3 x, y, n;
	n = GetTextureAxis(x, y);
	xv = x;
	yv = y;
	return n;

}

/*
==================
Plane::ProjectPointAxial
==================
*/
vec3 Plane::ProjectPointAxial(const vec3 &in, const vec3 &axis) const
{
	vec3 out = in;
	if (fabs(axis[0]) == 1)
		out[0] = (dist - normal[1] * in[1] - normal[2] * in[2]) / normal[0];
	else if (fabs(axis[1]) == 1)
		out[1] = (dist - normal[0] * in[0] - normal[2] * in[2]) / normal[1];
	else
		out[2] = (dist - normal[0] * in[0] - normal[1] * in[1]) / normal[2];
	return out;
}
dvec3 Plane::ProjectPointAxial(const dvec3 &in, const dvec3 &axis) const
{
	dvec3 out = in;
	if (fabs(axis[0]) == 1)
		out[0] = (dist - normal[1] * in[1] - normal[2] * in[2]) / normal[0];
	else if (fabs(axis[1]) == 1)
		out[1] = (dist - normal[0] * in[0] - normal[2] * in[2]) / normal[1];
	else
		out[2] = (dist - normal[0] * in[0] - normal[1] * in[1]) / normal[2];
	return out;
}

/*
==================
Plane::ProjectVectorAxial
==================
*/
vec3 Plane::ProjectVectorAxial(const vec3 &in, const vec3 &axis) const
{
	vec3 out = in;
	if (fabs(axis[0]) == 1)
		out[0] = (-normal[1] * in[1] - normal[2] * in[2]) / normal[0];
	else if (fabs(axis[1]) == 1)
		out[1] = (-normal[0] * in[0] - normal[2] * in[2]) / normal[1];
	else
		out[2] = (-normal[0] * in[0] - normal[1] * in[1]) / normal[2];
	return out;
}
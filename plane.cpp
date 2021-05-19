//==============================
//	plane.cpp
//==============================

#include "qe3.h"

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

/*
=============
Plane::EqualTo
=============
*/
bool Plane::EqualTo(Plane *b, int flip)
{
	float	tdist;
	vec3	tnormal;

	if (flip)
	{
		tnormal = -b->normal;
		tdist = -b->dist;
	}
	else
	{
		tnormal = b->normal;
		tdist = b->dist;
	}

	if (fabs(normal[0] - tnormal[0]) < NORMAL_EPSILON	&&
		fabs(normal[1] - tnormal[1]) < NORMAL_EPSILON	&&
		fabs(normal[2] - tnormal[2]) < NORMAL_EPSILON	&&
		fabs(dist - tdist) < DIST_EPSILON)
		return true;

	return false;
}

/*
============
Plane::FromPoints
returns false and does not modify the plane if the points are collinear
============
*/
bool Plane::FromPoints(const vec3 p0, const vec3 p1, const vec3 p2)
{
	vec3	pNorm;

	pNorm = CrossProduct(p0 - p1, p2 - p1);

	if (VectorNormalize(pNorm) < 0.1)
		return false;

	normal = pNorm;
	dist = DotProduct(p0, pNorm);
	pts[0] = p0;
	pts[1] = p1;
	pts[2] = p2;

	return true;
}

bool Plane::ClipLine(vec3 &p1, vec3 &p2)
{
	int		i;
	float	d1, d2, fr;
	vec3	*v;

	d1 = DotProduct(p1, normal) - dist;
	d2 = DotProduct(p2, normal) - dist;

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
		printf("WARNING: Brush plane with no normal\n");
		return false;
	}

	normal = norm;
	dist = DotProduct(pts[1], normal);
	return true;
}

/*
=================
Plane::Flip
=================
*/
void Plane::Flip()
{
	vec3 temp;
	temp = pts[0];
	pts[0] = pts[1];
	pts[1] = temp;
	Make();
}

/*
=================
Plane::Translate
=================
*/
void Plane::Translate(vec3 move)
{
	for (int i = 0; i < 3; i++)
	{
		pts[i] += move;
	}
	dist = DotProduct(pts[0], normal);
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
	int			i, x;
	float		max, v;
	vec3		org, vright, vup;
	winding_t  *w;

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

	// These are to keep the brush restrained within the Map Size limit
	vup = vup * (float)g_qeglobals.d_savedinfo.nMapSize;	// sikk - Map Size was 8192
	vright = vright * (float)g_qeglobals.d_savedinfo.nMapSize;

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
void Plane::GetTextureAxis(vec3 &xv, vec3 &yv)
{
	int		i, bestaxis;
	float	dot, best;

	best = 0;
	bestaxis = 0;

	for (i = 0; i < 6; i++)
	{
		dot = DotProduct(normal, g_v3BaseAxis[i * 3]);
		if (dot > best)
		{
			best = dot;
			bestaxis = i;
		}
	}

	xv = g_v3BaseAxis[bestaxis * 3 + 1];
	yv = g_v3BaseAxis[bestaxis * 3 + 2];
}


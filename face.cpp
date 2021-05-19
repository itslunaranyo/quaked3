//==============================
//	face.cpp
//==============================

#include "qe3.h"

const vec3_t g_v3BaseAxis[18] =
{
	{ 0, 0, 1}, {1, 0, 0}, {0,-1, 0},	// floor
	{ 0, 0,-1}, {1, 0, 0}, {0,-1, 0},	// ceiling
	{ 1, 0, 0}, {0, 1, 0}, {0, 0,-1},	// west wall
	{-1, 0, 0}, {0, 1, 0}, {0, 0,-1},	// east wall
	{ 0, 1, 0}, {1, 0, 0}, {0, 0,-1},	// south wall
	{ 0,-1, 0}, {1, 0, 0}, {0, 0,-1}	// north wall
};

const float g_fLightAxis[3] = {0.8f, 0.9f, 1.0f};	// lunaran: lightened a bit
//vec_t g_vLightAxis[3] = {(vec_t)0.6, (vec_t)0.8, (vec_t)1.0};




/*
================
Plane::Plane
================
*/
Plane::Plane()
{
	dist = 0;
	memset(normal, 0, sizeof(normal));
}

/*
=============
Plane::EqualTo
=============
*/
bool Plane::EqualTo(Plane *b, int flip)
{
	float	tdist;
	vec3_t	tnormal;

	if (flip)
	{
		tnormal[0] = -b->normal[0];
		tnormal[1] = -b->normal[1];
		tnormal[2] = -b->normal[2];
		tdist = -b->dist;
	}
	else
	{
		tnormal[0] = b->normal[0];
		tnormal[1] = b->normal[1];
		tnormal[2] = b->normal[2];
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
bool Plane::FromPoints(vec3_t p1, vec3_t p2, vec3_t p3)
{
	vec3_t	v1, v2;
	vec3_t	pNorm;

	VectorSubtract(p2, p1, v1);
	VectorSubtract(p3, p1, v2);
	CrossProduct(v1, v2, pNorm);

	if (VectorNormalize(pNorm) < 0.1)
		return false;

	VectorCopy(pNorm, normal);
	dist = DotProduct(p1, pNorm);

	return true;
}

/*
=================
Plane::BasePoly

TODO: is this the cause of brushes appearing broken/invisible when near the map extents?
=================
*/
winding_t *Plane::BasePoly()
{
	int			i, x;
	vec_t		max, v;
	vec3_t		org, vright, vup;
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

	VectorCopy(g_v3VecOrigin, vup);

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
	VectorMA(vup, -v, normal, vup);
	VectorNormalize(vup);
	VectorScale(normal, dist, org);
	CrossProduct(vup, normal, vright);

	// These are to keep the brush restrained within the Map Size limit
	VectorScale(vup, g_qeglobals.d_savedinfo.nMapSize, vup);	// sikk - Map Size was 8192
	VectorScale(vright, g_qeglobals.d_savedinfo.nMapSize, vright);

	// project a really big	axis aligned box onto the plane
	w = Winding::Alloc(4);

	VectorSubtract(org, vright, w->points[0]);
	VectorAdd(w->points[0], vup, w->points[0]);

	VectorAdd(org, vright, w->points[1]);
	VectorAdd(w->points[1], vup, w->points[1]);

	VectorAdd(org, vright, w->points[2]);
	VectorSubtract(w->points[2], vup, w->points[2]);

	VectorSubtract(org, vright, w->points[3]);
	VectorSubtract(w->points[3], vup, w->points[3]);

	w->numpoints = 4;

	return w;
}

/*
==================
Plane::GetTextureAxis
==================
*/
void Plane::GetTextureAxis(vec3_t xv, vec3_t yv)
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

	VectorCopy(g_v3BaseAxis[bestaxis * 3 + 1], xv);
	VectorCopy(g_v3BaseAxis[bestaxis * 3 + 2], yv);
}

//==========================================================================

/*
================
Face::Face
================
*/
Face::Face()
{
	Init();
}

/*
================
Face::Face
================
*/
Face::Face(Brush* b)
{
	assert(b);

	Init();

	owner = b;
	next = b->basis.faces;
	b->basis.faces = this;
}

/*
================
Face::~Face
================
*/
Face::~Face()
{
	if (face_winding)
	{
		Winding::Free(face_winding);
	//	face_winding = nullptr;
	}
}

/*
================
Face::Init
================
*/
void Face::Init()
{
	owner = nullptr;
	next = nullptr;
	original = nullptr;
	face_winding = nullptr;
	d_texture = nullptr;

	texdef = { 0,0,0,0 };

	memset(d_color, 0, sizeof(d_color));
	memset(planepts, 0, sizeof(planepts));
}


/*
================
Face::Clone
================
*/
Face *Face::Clone()
{
	Face	*n;

	n = new Face();
	n->texdef = texdef;
	memcpy(n->planepts, planepts, sizeof(n->planepts));

	// all other fields are derived, and will be set by Brush_Build
	return n;
}

/*
================
Face::FullClone

sikk - Undo/Redo
makes an exact copy of the face
================
*/
Face *Face::FullClone(Brush *own)
{
	Face	*n;

	n = new Face(own);
	n->texdef = texdef;
	memcpy(n->planepts, planepts, sizeof(n->planepts));
	memcpy(&n->plane, &plane, sizeof(Plane));

	if (face_winding)
		n->face_winding = Winding::Clone(face_winding);
	else
		n->face_winding = NULL;

	n->d_texture = d_texture;

	return n;
}


/*
=============
Face::MemorySize

sikk - Undo/Redo
=============
*/
int Face::MemorySize()
{
	int size = 0;

	if (face_winding)
		size += Winding::MemorySize(face_winding);

	size += sizeof(Face);

	return size;
}


/*
=================
Face::BoundsOnAxis

lunaran: new texture fit
=================
*/
void Face::BoundsOnAxis(vec3_t a, float* min, float* max)
{
	int i;
	float p;
	vec3_t an;

	if (!face_winding)
		return;

	*max = -99999;
	*min = 99999;

	VectorCopy(a, an);
	VectorNormalize(an);

	for (i = 0; i < face_winding->numpoints; i++)
	{
		p = DotProduct(an, face_winding->points[i]);
		if (p > *max) *max = p;
		if (p < *min) *min = p;
	}
}

/*
=================
Face::ClipLine
=================
*/
bool Face::ClipLine(vec3_t p1, vec3_t p2)
{
	int		i;
	float	d1, d2, fr;
	float  *v;

	d1 = DotProduct(p1, plane.normal) - plane.dist;
	d2 = DotProduct(p2, plane.normal) - plane.dist;

	if (d1 >= 0 && d2 >= 0)
		return false;	// totally outside
	if (d1 <= 0 && d2 <= 0)
		return true;	// totally inside

	fr = d1 / (d1 - d2);

	if (d1 > 0)
		v = p1;
	else
		v = p2;

	for (i = 0; i < 3; i++)
		v[i] = p1[i] + fr * (p2[i] - p1[i]);

	return true;
}

/*
=================
Face::FitTexture

lunaran: rewrote all this so it works with decimal fit increments
=================
*/
void Face::FitTexture(float fHeight, float fWidth)
{
	vec_t		scale;
	vec3_t		u, v, n;
	float		min, max, len;
	winding_t	*w;

	w = face_winding;
	if (!w)
		return;

	// convert current rotation to U and V vectors
	plane.GetTextureAxis(u, v);
	CrossProduct(u, v, n);
	if (texdef.rotate != 0.0f)
	{
		VectorScale(n, -texdef.rotate, n);
		VectorRotate(u, n, u);
		VectorRotate(v, n, v);
	}

	// find size of winding along those vectors and resize
	if (fWidth != 0.0f)
	{
		BoundsOnAxis(u, &min, &max);
		len = max - min;
		scale = (len / d_texture->width) / fWidth;
		texdef.shift[0] = -((int)roundf(min / scale + 0.001f) % d_texture->width);
		texdef.scale[0] = scale;
	}

	if (fHeight != 0.0f)
	{
		BoundsOnAxis(v, &min, &max);
		len = max - min;
		scale = (len / d_texture->height) / fHeight;
		texdef.shift[1] = -((int)roundf(min / scale + 0.001f) % d_texture->width);
		texdef.scale[1] = scale;
	}
}

/*
================
Face::MoveTexture
================
*/
void Face::MoveTexture(vec3_t move)
{
	vec3_t		pvecs[2];
	vec_t		s, t, ns, nt;
	vec_t		ang, sinv, cosv;

	plane.GetTextureAxis(pvecs[0], pvecs[1]);
	ang = texdef.rotate / 180 * Q_PI;
	sinv = sin(ang);
	cosv = cos(ang);

	if (!texdef.scale[0])
		texdef.scale[0] = 1;
	if (!texdef.scale[1])
		texdef.scale[1] = 1;

	s = DotProduct(move, pvecs[0]);
	t = DotProduct(move, pvecs[1]);
	ns = cosv * s - sinv * t;
	nt = sinv * s + cosv * t;
	s = ns / texdef.scale[0];
	t = nt / texdef.scale[1];

	texdef.shift[0] -= s;
	texdef.shift[1] -= t;

	while (texdef.shift[0] > d_texture->width)
		texdef.shift[0] -= d_texture->width;
	while (texdef.shift[1] > d_texture->height)
		texdef.shift[1] -= d_texture->height;
	while (texdef.shift[0] < 0)
		texdef.shift[0] += d_texture->width;
	while (texdef.shift[1] < 0)
		texdef.shift[1] += d_texture->height;
}

/*
=================
Face::ColorAndTexture
=================
*/
void Face::ColorAndTexture()
{
	if (!d_texture) return;

	SetColor();
	Winding::TextureCoordinates(face_winding, d_texture, this);
}

/*
=================
Face::SetTexture
=================
*/
void Face::SetTexture(texdef_t *texdefIn, int nSkipFlags)
{
	Surf_ApplyTexdef(&texdef, texdefIn, nSkipFlags);
	d_texture = Textures::ForName(texdefIn->name);
	owner->Build();
}

/*
================
Face::SetColor
================
*/
void Face::SetColor()
{
	float shade;
	Texture *q;

	q = d_texture;
	shade = ShadeForPlane();

	if (g_qeglobals.d_camera.draw_mode == cd_texture && owner->owner->IsBrush())
	{
		d_color[0] = d_color[1] = d_color[2] = shade;
	}
	else
	{
		d_color[0] = shade * q->color[0];
		d_color[1] = shade * q->color[1];
		d_color[2] = shade * q->color[2];
	}
}

/*
================
Face::ShadeForPlane

Light different planes differently to improve recognition
================
*/
float Face::ShadeForPlane()
{
	float f = 0;
	for (int i = 0; i < 3; i++)
		f += plane.normal[i] * plane.normal[i] * g_fLightAxis[i];

	return f;
}



/*
=================
Face::Draw
=================
*/
void Face::Draw()
{
	int i;

	if (face_winding == 0)
		return;

	glBegin(GL_POLYGON);
	for (i = 0; i < face_winding->numpoints; i++)
		glVertex3fv(face_winding->points[i]);
	glEnd();
}


/*
================
Face::MakePlane

sikk - Vertex Editing Splits Face
================
*/
void Face::MakePlane()
{
	int		j;
	vec3_t	t1, t2, t3;

	// convert to a vector / dist plane
	for (j = 0; j < 3; j++)
	{
		t1[j] = planepts[0][j] - planepts[1][j];
		t2[j] = planepts[2][j] - planepts[1][j];
		t3[j] = planepts[1][j];
	}

	CrossProduct(t1, t2, plane.normal);

	if (VectorCompare(plane.normal, g_v3VecOrigin))
		Sys_Printf("WARNING: Brush plane with no normal.\n");

	VectorNormalize(plane.normal);
	plane.dist = DotProduct(t3, plane.normal);
}





/*
=================
AddPlanePoint
part of vertex/edge dragging / face skewing functionality
=================
*/
int AddPlanePoint(float *f)
{
	int i;

	for (i = 0; i < g_qeglobals.d_nNumMovePoints; i++)
		if (g_qeglobals.d_fMovePoints[i] == f)
			return 0;

	g_qeglobals.d_fMovePoints[g_qeglobals.d_nNumMovePoints++] = f;

	return 1;
}


// ================================================================

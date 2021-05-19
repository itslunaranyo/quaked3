//==============================
//	face.cpp
//==============================

#include "qe3.h"

const vec3 g_v3BaseAxis[18] =
{
	vec3( 0, 0, 1), vec3(1, 0, 0), vec3(0,-1, 0),	// floor
	vec3( 0, 0,-1), vec3(1, 0, 0), vec3(0,-1, 0),	// ceiling
	vec3( 1, 0, 0), vec3(0, 1, 0), vec3(0, 0,-1),	// west wall
	vec3(-1, 0, 0), vec3(0, 1, 0), vec3(0, 0,-1),	// east wall
	vec3( 0, 1, 0), vec3(1, 0, 0), vec3(0, 0,-1),	// south wall
	vec3( 0,-1, 0), vec3(1, 0, 0), vec3(0, 0,-1)	// north wall
};

const float g_fLightAxis[3] = {0.8f, 0.9f, 1.0f};	// lunaran: lightened a bit
//float g_vLightAxis[3] = {(float)0.6, (float)0.8, (float)1.0};




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

//==========================================================================

/*
================
Face::Face
================
*/
Face::Face() : owner(nullptr), fnext(nullptr), original(nullptr), face_winding(nullptr)//, DEPtexture(nullptr)
{
	//texdef = { 0,0,0,0 };
}

/*
================
Face::Face
================
*/
Face::Face(Brush *b) : 
	owner(b), fnext(b->basis.faces), 
	original(nullptr), face_winding(nullptr)//, DEPtexture(nullptr)
{
	assert(b);

	//texdef = { 0,0,0,0 };

	b->basis.faces = this;
}

Face::Face(Face *f) : 
	owner(nullptr), 
	plane(f->plane), texdef(f->texdef), //DEPtexture(f->DEPtexture), 
	original(nullptr), face_winding(nullptr)
{
	assert(f);
}

Face::Face(Brush *b, Face *f) : 
	owner(b), original(nullptr), face_winding(nullptr), 
	plane(f->plane), texdef(f->texdef)//, DEPtexture(f->DEPtexture)
{
	assert(b);
	assert(f);

	fnext = b->basis.faces;
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
Face::Clone
================
*/
Face *Face::Clone()
{
	Face	*n;

	n = new Face();
	n->texdef = texdef;
	n->plane = plane;

	//n->owner = owner;

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

	n->plane = plane;

	if (face_winding)
		n->face_winding = Winding::Clone(face_winding);
	else
		n->face_winding = NULL;

	//n->DEPtexture = DEPtexture;

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
Face::MakeWinding

returns the visible polygon on the face

TODO: why isn't this in Face::?
=================
*/
winding_t *Face::MakeWinding()
{
	bool		past;
	Face		*clip;
	Plane		p;
	winding_t	*w;

	// get a poly that covers an effectively infinite area
	w = plane.BasePoly();

	// chop the poly by all of the other faces
	past = false;
	for (clip = owner->basis.faces; clip && w; clip = clip->fnext)
	{
		if (clip == this)
		{
			past = true;
			continue;
		}

		if (DotProduct(plane.normal, clip->plane.normal) > 0.999 &&
			fabs(plane.dist - clip->plane.dist) < 0.01)
		{	// identical plane, use the later one
			if (past)
			{
				Winding::Free(w);
				return NULL;
			}
			continue;
		}

		// flip the plane, because we want to keep the back side
		p.normal = vec3(0) - clip->plane.normal;
		p.dist = -clip->plane.dist;

		w = Winding::Clip(w, &p, false);
		if (!w)
			return w;
	}

	if (w->numpoints < 3)
	{
		Winding::Free(w);
		w = nullptr;
	}

	if (!w)
		printf("unused plane\n");

	return w;
}

/*
=================
Face::BoundsOnAxis

lunaran: new texture fit
=================
*/
void Face::BoundsOnAxis(const vec3 a, float* min, float* max)
{
	int i;
	float p;
	vec3 an;

	if (!face_winding)
		return;

	*max = -99999;
	*min = 99999;

	an = a;
	VectorNormalize(an);

	for (i = 0; i < face_winding->numpoints; i++)
	{
		p = DotProduct(an, face_winding->points[i].point);
		if (p > *max) *max = p;
		if (p < *min) *min = p;
	}
}

/*
=================
Face::AddBounds
=================
*/
void Face::AddBounds(vec3 & mins, vec3 & maxs)
{
	for (int i = 0; i < face_winding->numpoints; i++)
	{
		mins = glm::min(mins, face_winding->points[i].point);
		maxs = glm::max(maxs, face_winding->points[i].point);
	}
}

/*
=================
Face::ClipLine
=================
*/
bool Face::ClipLine(vec3 &p1, vec3 &p2)
{
	return plane.ClipLine(p1, p2);
}

/*
=================
Face::TestSideSelect
=================
*/
bool Face::TestSideSelect(const vec3 origin, const vec3 dir)
{
	Face	*f2;
	vec3	p1, p2;

	p1 = origin;
	p2 = origin + dir * (float)g_qeglobals.d_savedinfo.nMapSize * 2.0f;

	for (f2 = owner->basis.faces; f2; f2 = f2->fnext)
	{
		if (f2 == this)
			continue;
		f2->ClipLine(p1, p2);
	}

	if (f2)
		return false;
	//if (VectorCompare(p1, origin))
	//	return false;
	if (ClipLine(p1, p2))
		return false;

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
	float		scale;
	vec3		u, v, n;
	float		min, max, len;
	winding_t	*w;

	w = face_winding;
	if (!w)
		return;

	// convert current rotation to U and V vectors
	plane.GetTextureAxis(u, v);
	n = CrossProduct(u, v);
	if (texdef.rotate != 0.0f)
	{
		n = n * -texdef.rotate;
		VectorRotate(u, n, u);
		VectorRotate(v, n, v);
	}

	// find size of winding along those vectors and resize
	if (fWidth != 0.0f)
	{
		BoundsOnAxis(u, &min, &max);
		len = max - min;
		scale = (len / texdef.tex->width) / fWidth;
		texdef.shift[0] = -((int)roundf(min / scale + 0.001f) % texdef.tex->width);
		texdef.scale[0] = scale;
	}

	if (fHeight != 0.0f)
	{
		BoundsOnAxis(v, &min, &max);
		len = max - min;
		scale = (len / texdef.tex->height) / fHeight;
		texdef.shift[1] = -((int)roundf(min / scale + 0.001f) % texdef.tex->width);
		texdef.scale[1] = scale;
	}
}

/*
================
Face::MoveTexture
================
*/
void Face::MoveTexture(const vec3 move)
{
	vec3		pvecs[2];
	float		s, t, ns, nt;
	float		ang, sinv, cosv;

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
	
	texdef.Clamp();
}

/*
=================
Face::ColorAndTexture
=================
*/
void Face::ColorAndTexture()
{
	if (!texdef.tex) return;

	SetColor();
	Winding::TextureCoordinates(face_winding, texdef.tex, this);
}

/*
=================
Face::SetTexture
=================
*/
void Face::SetTexture(TexDef *texdefIn, unsigned flags)
{
	Surf_ApplyTexdef(texdef, *texdefIn, flags);
	//DEPtexture = Textures::ForName(texdefIn->name);
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

	q = texdef.tex;
	shade = ShadeForPlane();

	if (g_qeglobals.d_vCamera.draw_mode == cd_texture && owner->owner->IsBrush())
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
	if (!face_winding)
		return;

	glBegin(GL_POLYGON);
	for (int i = 0; i < face_winding->numpoints; i++)
		glVertex3fv(&face_winding->points[i].point[0]);
	glEnd();
}

void Face::DrawWire()
{
	if (!face_winding)
		return;

	glBegin(GL_LINE_LOOP);
	for (int i = 0; i < face_winding->numpoints; i++)
		glVertex3fv(&face_winding->points[i].point[0]);
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
	plane.Make();
}





/*
=================
AddPlanePoint
part of vertex/edge dragging / face skewing functionality
=================
*/
int AddPlanePoint(vec3 *f)
{
	int i;

	for (i = 0; i < g_qeglobals.d_nNumMovePoints; i++)
		if (g_qeglobals.d_fMovePoints[i] == f)
			return 0;

	g_qeglobals.d_fMovePoints[g_qeglobals.d_nNumMovePoints++] = f;

	return 1;
}


// ================================================================

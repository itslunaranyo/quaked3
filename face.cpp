//==============================
//	face.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "winding.h"
#include "surface.h"


/*
================
Face::Face
================
*/
Face::Face() : owner(nullptr), fnext(nullptr) {}

Face::Face(Brush *b) : 
	owner(b), fnext(b->faces)
{
	assert(b);

	b->faces = this;
}

Face::Face(Face *f) : 
	owner(nullptr), fnext(nullptr),
	plane(f->plane), texdef(f->texdef)
{
	assert(f);
}

Face::Face(Brush *b, Face *f) : 
	owner(b), 
	plane(f->plane), texdef(f->texdef)
{
	assert(b);
	assert(f);

	fnext = b->faces;
	b->faces = this;
}

Face::Face(Plane &p, TexDef &td) : 
	owner(nullptr), fnext(nullptr), 
	plane(p), texdef(td)
{}

/*
================
Face::~Face
================
*/
Face::~Face()
{
	winding.Free();
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

void Face::ClearChain(Face **f)
{
	if (*f)
	{
		Face *fn, *fp;
		fn = *f;
		while (fn)
		{
			fp = fn;
			fn = fn->fnext;
			delete fp;
		}
		*f = nullptr;
	}
}


/*
=================
Face::MakeWinding

returns the visible polygon on the face
=================
*/
bool Face::MakeWinding()
{
	bool		past;
	Face		*clip;
	Plane		p;

	// get a poly that covers an effectively infinite area
	winding.Base(plane);

	// chop the poly by all of the other faces
	past = false;
	for (clip = owner->faces; clip && winding.Count(); clip = clip->fnext)
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
				winding.Free();
				return false;
			}
			continue;
		}

		// flip the plane, because we want to keep the back side
		p.normal = dvec3(0) - clip->plane.normal;
		p.dist = -clip->plane.dist;

		if (!winding.Clip(p, false))
			return false;
	}

	if (winding.Count() < 3)
	{
		winding.Free();
		return false;
	}

	return true;
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

	if (!winding.Count())
		return;

	*max = -99999;
	*min = 99999;

	an = a;
	VectorNormalize(an);

	for (i = 0; i < winding.Count(); i++)
	{
		p = DotProduct(an, winding[i]->point);
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
	winding.AddBounds(mins, maxs);
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
	p2 = origin + dir * (float)g_cfgEditor.MapSize;

	for (f2 = owner->faces; f2; f2 = f2->fnext)
	{
		if (f2 == this)
			continue;
		f2->ClipLine(p1, p2);
	}

	if (f2)
		return false;

	if (VectorCompare(p1, origin))
		return false;

	if (ClipLine(p1, p2))
		return false;

	return true;
}

/*
=================
Face::TestSideSelectAxis
=================
*/
bool Face::TestSideSelectAxis(const vec3 origin, const int axis)
{
	if (owner->Center()[axis] < origin[axis])
	{
		if (plane.normal[axis] >= 0.7071)
			return true;
	}
	else if (plane.normal[axis] <= -0.7071)
		return true;

	return false;
}

/*
=================
Face::ConcaveTo
=================
*/
bool Face::ConcaveTo(Face& f2)
{
	// check if one of the points of face 1 is at the back of the plane of face 2
	if (winding.PlaneSides(f2.plane) != Plane::FRONT)
		return true;

	// check if one of the points of face 2 is at the back of the plane of face 1
	if (f2.winding.PlaneSides(plane) != Plane::FRONT)
		return true;

	return false;
}


/*
=================
Face::FitTexture

lunaran: rewrote all this so it works with decimal fit increments
=================
*/
void Face::FitTexture(const float fHeight, const float fWidth)
{
	float		scale, baseline;
	vec3		u, v, n;
	int			uSign, vSign, offset;
	float		min, max, len;

	if (!winding.Count())
		return;

	uSign = vSign = 1;

	// convert current rotation to U and V vectors
	plane.GetTextureAxis(u, v);
	n = CrossProduct(u, v);
	if (texdef.rotate != 0.0f)
	{
		n = n * -texdef.rotate;
		VectorRotate(u, n, u);
		VectorRotate(v, n, v);
	}

	// maintain negations
	if (texdef.scale[0] < 0)
	{
		uSign = -1;
		u = -u;
	}
	if (texdef.scale[1] < 0)
	{
		vSign = -1;
		v = -v;
	}

	// find size of winding along those vectors and resize
	if (fWidth != 0.0f)
	{
		BoundsOnAxis(u, &min, &max);
		// for fits smaller than the texture, snap to any existing shift in the
		// alignment that's close to the period of the fit (so we don't always
		// reset to the bottom left corner and lose existing alignment on trims/etc)
		baseline = ((min + texdef.shift[0]) / texdef.Tex()->width) / texdef.scale[0];
		baseline = roundf(baseline / fWidth) * fWidth;
		offset = roundf(baseline * texdef.Tex()->width);

		len = max - min;
		scale = (len / texdef.Tex()->width) / fWidth * uSign;
		texdef.shift[0] = -fmod((min / scale - offset), texdef.Tex()->width) * vSign;
		texdef.scale[0] = scale;
	}

	if (fHeight != 0.0f)
	{
		BoundsOnAxis(v, &min, &max);

		baseline = ((min + texdef.shift[1]) / texdef.Tex()->height) / texdef.scale[1];
		baseline = roundf(baseline / fHeight) * fHeight;
		offset = roundf(baseline * texdef.Tex()->height);

		len = max - min;
		scale = (len / texdef.Tex()->height) / fHeight * vSign;
		texdef.shift[1] = -fmod((min / scale - offset), texdef.Tex()->height) * vSign;
		texdef.scale[1] = scale;
	}
}

/*
================
Face::Transform
================
*/
void Face::Transform(mat4 mat, bool bTexLock)
{
	dvec3	p1, p2, p3;

	if (bTexLock)
		Surface::ComputeAbsolute(this, p1, p2, p3);

	for (int i = 0; i < 3; i++)
		plane.pts[i] = mat * glm::vec4(plane.pts[i], 1);
	plane.Make();

	if (bTexLock)
	{
		p1 = mat * glm::dvec4(p1, 1);
		p2 = mat * glm::dvec4(p2, 1);
		p3 = mat * glm::dvec4(p3, 1);
		Surface::AbsoluteToLocal(plane, texdef, p1, p2, p3);
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
	SetColor();
	winding.TextureCoordinates(*texdef.Tex(), *this);
}

/*
=================
Face::SetTexture
=================
*/
void Face::SetTexture(TexDef *texdefIn, unsigned flags)
{
	Surface::ApplyTexdef(texdef, *texdefIn, flags);
	//owner->Build();
	ColorAndTexture();
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

	q = texdef.Tex();
	shade = ShadeForPlane();

	// lunaran TODO: get rid of this branch
	if (g_cfgUI.DrawMode == CD_TEXTURED && owner->owner->IsBrush())
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


//==========================================================================

/*
================
Face::ShadeForPlane

Light different planes differently to improve recognition
================
*/
float Face::ShadeForPlane()
{
	const float g_fLightAxis[3] = { 0.8f, 0.9f, 1.0f };	// lunaran: lightened a bit

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
	if (!winding.Count())
		return;

	glBegin(GL_POLYGON);
	for (int i = 0; i < winding.Count(); i++)
		glVertex3fv(&(winding[i]->point[0]));
	glEnd();
}

void Face::DrawWire()
{
	if (!winding.Count())
		return;

	glBegin(GL_LINE_LOOP);
	for (int i = 0; i < winding.Count(); i++)
		glVertex3fv(&(winding[i]->point[0]));
	glEnd();
}
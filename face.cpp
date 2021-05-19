//==============================
//	face.cpp
//==============================

#include "qe3.h"


/*
================
Face::Face
================
*/
Face::Face() : owner(nullptr), fnext(nullptr), face_winding(nullptr) {}

Face::Face(Brush *b) : 
	owner(b), fnext(b->faces), face_winding(nullptr)
{
	assert(b);

	b->faces = this;
}

Face::Face(Face *f) : 
	owner(nullptr), fnext(nullptr),
	plane(f->plane), texdef(f->texdef), face_winding(nullptr)
{
	assert(f);
}

Face::Face(Brush *b, Face *f) : 
	owner(b), face_winding(nullptr), 
	plane(f->plane), texdef(f->texdef)
{
	assert(b);
	assert(f);

	fnext = b->faces;
	b->faces = this;
}

Face::Face(Plane &p, TexDef &td) : 
	owner(nullptr), fnext(nullptr), face_winding(nullptr),
	plane(p), texdef(td)
{}

/*
================
Face::~Face
================
*/
Face::~Face()
{
	if (face_winding)
		Winding::Free(face_winding);
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
	for (clip = owner->faces; clip && w; clip = clip->fnext)
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
Face::TestSideSelect
=================
*/
bool Face::TestSideSelect(const vec3 origin, const vec3 dir)
{
	Face	*f2;
	vec3	p1, p2;

	p1 = origin;
	p2 = origin + dir * 2.0f * (float)g_cfgEditor.MapSize;

	for (f2 = owner->faces; f2; f2 = f2->fnext)
	{
		if (f2 == this)
			continue;
		f2->ClipLine(p1, p2);
	}

	if (f2)
		return false;

	// lunaran: commented this out as it prevents side selects that don't clip
	// off the origin of the ray (ie the ray starts 'alongside' the face) - 
	// this could have been unintentionally nefarious, so keep an eye on it
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
Face::Transform
================
*/
void Face::Transform(mat4 mat, bool bTexLock)
{
	vec3	p1, p2, p3;

	if (bTexLock)
		ComputeAbsolute(this, p1, p2, p3);

	for (int i = 0; i < 3; i++)
		plane.pts[i] = mat * glm::vec4(plane.pts[i], 1);
	plane.Make();

	if (bTexLock)
	{
		p1 = mat * glm::vec4(p1, 1);
		p2 = mat * glm::vec4(p2, 1);
		p3 = mat * glm::vec4(p3, 1);
		AbsoluteToLocal(plane, this, p1, p2, p3);
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

	q = texdef.tex;
	shade = ShadeForPlane();

	// lunaran TODO: get rid of this branch
	if (g_cfgUI.DrawMode == cd_texture && owner->owner->IsBrush())
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


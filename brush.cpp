//==============================
//	brush.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "parse.h"
#include "winding.h"
#include <iostream>

/*
=============
Brush::Brush
=============
*/
Brush::Brush() :
	prev(this), next(this), oprev(this), onext(this),
	owner(nullptr), showFlags(0),
	faces(nullptr), mins(99999), maxs(-99999)
{}

/*
=============
Brush::~Brush

Frees the brush with all of its faces and display list.
Unlinks the brush from whichever chain it is in.
=============
*/
Brush::~Brush()
{
	// free faces
	ClearFaces();

	// unlink from active/selected list
	if (next != this)
		RemoveFromList();

	// unlink from entity list
	if (onext != this)
		Entity::UnlinkBrush(this);
}

/*
==================
Brush::NumFaces
==================
*/
int Brush::NumFaces() const
{
	int sum = 0;
	Face* f;
	for (f = faces; f; f = f->fnext)
		if (f->HasWinding())
			sum++;
	return sum;
}

/*
=================
Brush::IsConvex
=================
*/
bool Brush::IsConvex() const
{
	Face	*face1, *face2;

	for (face1 = faces; face1; face1 = face1->fnext)
	{
		if (!face1->HasWinding())
			continue;
		for (face2 = faces; face2; face2 = face2->fnext)
		{
			if (face1 == face2)
				continue;
			if (!face2->HasWinding())
				continue;

			if (face1->ConcaveTo(*face2))
				return false;
		}
	}
	return true;
}

/*
=================
Brush::IsFiltered
=================
*/
bool Brush::IsFiltered() const
{
	if (!owner)
		return true;		// during construction

	if (g_cfgUI.ViewFilter & showFlags)
		return true;

	return owner->IsFiltered();
}


//=========================================================================


/*
=============
Brush::Create

Create new rectilinear brush
The brush is NOT linked to any list
=============
*/
Brush *Brush::Create(const vec3 inMins, const vec3 inMaxs, TexDef *texdef)
{
	Brush	   *b;

	for (int i = 0; i < 3; i++)
	{
		if (inMaxs[i] <= inMins[i])
			Error("Brush_InitSolid: Backwards.");
	}

	b = new Brush();

	b->Recreate(inMins, inMaxs, texdef);
	return b;
}

void Brush::Recreate(const vec3 inMins, const vec3 inMaxs, TexDef *inTexDef)
{
	vec3	pts[4][2];
	int		i, j;
	Face	*f;

	// with no wads loaded, a brush created has to summon a broken texture
	// from somewhere - putting it here prevents a pink garbage texture added 
	// to the list until actually needed
	/*
	if (inTexDef->tex == nullptr)
	{
		if (inTexDef->name[0])
			inTexDef->Set(inTexDef->name);
		else
			inTexDef->Set("none");
	}*/

	// reuse old faces - chances are they're already contiguous in memory, and
	// deleting/reallocating them might get us fragments all over instead.
	i = 0;
	for (f = faces; f; f = f->fnext)	// ensure we have six first though
		++i;
	while (i < 6)
	{
		f = new Face(this);
		++i;
	}
	while (i > 6)
	{
		f = faces->fnext;
		delete faces;
		faces = f;
		--i;
	}

	mins = inMins;
	maxs = inMaxs;

	pts[0][0][0] = inMins[0];
	pts[0][0][1] = inMins[1];

	pts[1][0][0] = inMins[0];
	pts[1][0][1] = inMaxs[1];

	pts[2][0][0] = inMaxs[0];
	pts[2][0][1] = inMaxs[1];

	pts[3][0][0] = inMaxs[0];
	pts[3][0][1] = inMins[1];

	for (i = 0; i < 4; i++)
	{
		pts[i][0][2] = inMins[2];
		pts[i][1][0] = pts[i][0][0];
		pts[i][1][1] = pts[i][0][1];
		pts[i][1][2] = inMaxs[2];
	}

	f = faces;
	for (i = 0; i < 4; i++)
	{
		f->texdef = *inTexDef;
		j = (i + 1) % 4;
		f->plane.FromPoints(pts[j][1], pts[i][1], pts[i][0]);
		f = f->fnext;
	}

	f->texdef = *inTexDef;
	f->plane.FromPoints(pts[0][1], pts[1][1], pts[2][1]);
	f = f->fnext;

	f->texdef = *inTexDef;
	f->plane.FromPoints(pts[2][0], pts[1][0], pts[0][0]);
}



/*
============
Brush::Clone

Does NOT add the new brush to any lists
============
*/
Brush *Brush::Clone() const
{
	Brush	*n;
	Face	*f, *nf;

	n = new Brush();
	n->owner = owner;

	n->mins = mins;
	n->maxs = maxs;
	for (f = faces; f; f = f->fnext)
	{
		nf = f->Clone();
		nf->fnext = n->faces;
		n->faces = nf;
	}

	return n;
}

/*
============
Brush::ClearFaces
============
*/
void Brush::ClearFaces()
{
	Face::ClearChain(&faces);
}

/*
==================
Brush::Transform
==================
*/
void Brush::Transform(const mat4 mat, const bool textureLock)
{
	//if (!owner->IsBrush())
	//	return;
	bool tlock = textureLock & owner->IsBrush();

	for (Face *f = faces; f; f = f->fnext)
		f->Transform(mat, textureLock);

	// detect reflection and flip all the planes
	if (glm::determinant(glm::mat3(mat)) < 0)
	{
		for (Face *f = faces; f; f = f->fnext)
			f->plane.Flip();
	}

	Build();
}

/*
==================
Brush::RefreshFlags

union of the showflags of all textures on the brush
==================
*/
void Brush::RefreshFlags()
{
	showFlags = showFlags & BFL_HIDDEN;	// preserve this one, as it's a brush-level flag
	for (Face *f = faces; f; f = f->fnext)
		showFlags |= f->texdef.Tex()->showflags;
}

/*
==================
Brush::FreeWindings
==================
*/
void Brush::FreeWindings()
{
	for (Face* f = faces; f; f = f->fnext)
		f->FreeWinding();
}

/*
==================
Brush::FullBuild

Certifies the texture pointer for the texture name on every face before Build()ing
for map load/import operations
==================
*/
bool Brush::FullBuild()
{
	RefreshTexdefs();
	return Build();
}

/*
==================
Brush::Build

Builds a brush rendering data and also sets the min/max bounds
==================
*/
bool Brush::Build()
{
	Face		*face;
	int			i;

	ClearBounds(mins, maxs);
	SnapPlanePoints();
	MakeFacePlanes();	// planes should be made by now but snapping makes it necessary

	for (face = faces; face; face = face->fnext)
	{
		face->owner = this;
		
		if (!face->MakeWinding())
			continue;

		// add to bounding box
		face->AddBounds(mins, maxs);

		// setup s and t vectors, and set color
		face->ColorAndTexture();
	}

	// check that brush planes didn't completely clip each other away
	for (i = 0; i < 3; i++)
	{
		if (mins[i] >= maxs[i])
			return false;
	}

	RefreshFlags();
	//g_map.modified = true;	// mark the map as changed

	return true;
}


/*
==================
Brush::MakeFacePlanes
==================
*/
void Brush::MakeFacePlanes()
{
	for (Face *f = faces; f; f = f->fnext)
		f->plane.Make();
}

/*
==================
Brush::SnapPlanePoints
==================
*/
void Brush::SnapPlanePoints()
{
	if (!g_qeglobals.bGridSnap)
		return;

	for (Face *f = faces; f; f = f->fnext)
		f->plane.Snap();
}

/*
==================
Brush::RemoveEmptyFaces

lunaran FIXME: this changes the face pointer makeup of a brush, so it must only
be used in the context of a CmdBrushMod
==================
*/
void Brush::RemoveEmptyFaces()
{
	Face	*f, *fnext;

	f = faces;
	faces = NULL;

	for (; f; f = fnext)
	{
		fnext = f->fnext;

		if (!f->HasWinding())
			delete f;
		else
		{
			f->fnext = faces;
			faces = f;
		}
	}
}


/*
=================
Brush::FitTexture
=================
*/
void Brush::FitTexture(int nHeight, int nWidth)
{
	for (Face *face = faces; face; face = face->fnext)
		face->FitTexture(nHeight, nWidth);
	Build();
}

/*
=================
Brush::SetTexture
=================
*/
void Brush::SetTexture(TexDef *texdef, unsigned flags)
{
	for (Face *f = faces; f; f = f->fnext)
		f->SetTexture(texdef, flags);

	Build();
}

/*
=================
Brush::RefreshTexdefs
=================
*/
void Brush::RefreshTexdefs()
{
	for (Face* face = faces; face; face = face->fnext)
		face->texdef.Set(face->texdef.Name());
	RefreshFlags();
}

/*
==============
Brush::RayTest

Intersects a ray with a brush
Returns the face hit and the distance along the ray the intersection occured at
Returns NULL and 0 if not hit at all
==============
*/
Face *Brush::RayTest(const vec3 origin, const vec3 dir, float *dist)
{
	int		i;
	float	frac, d1, d2;
	vec3	p1, p2;
	Face *f, *firstface;

	p1 = origin;

	for (i = 0; i < 3; i++)
		p2[i] = p1[i] + dir[i] * (g_cfgEditor.MapSize * 2);// 16384;

	for (f = faces; f; f = f->fnext)
	{
		d1 = DotProduct(p1, vec3(f->plane.normal)) - f->plane.dist;
		d2 = DotProduct(p2, vec3(f->plane.normal)) - f->plane.dist;

		if (d1 >= 0 && d2 >= 0)
		{
			*dist = 0;
			return NULL;	// ray is on front side of face
		}

		if (d1 <= 0 && d2 <= 0)
			continue;

		// clip the ray to the plane
		frac = d1 / (d1 - d2);

		if (d1 > 0)
		{
			firstface = f;
			for (i = 0; i < 3; i++)
				p1[i] = p1[i] + frac * (p2[i] - p1[i]);
		}
		else
			for (i = 0; i < 3; i++)
				p2[i] = p1[i] + frac * (p2[i] - p1[i]);
	}

	// find distance p1 is along dir
	p1 = p1 - origin;
	d1 = DotProduct(p1, dir);

	*dist = d1;

	return firstface;
}



/*
=================
Brush::PointTest

simply test if a point is inside the volume of the brush
=================
*/
bool Brush::PointTest(const vec3 origin)
{
	float d;
	for (Face *f = faces; f; f = f->fnext)
	{
		d = DotProduct(origin, vec3(f->plane.normal)) - f->plane.dist;

		if (d >= 0)
			return false;	// point is on front side of face
	}
	return true;
}


//=========================================================================

/*
=================
Brush::IsLinked
=================
*/
bool Brush::IsLinked() const
{
	assert((next == this) == (prev == this));
	return (next != this);
}

/*
=================
Brush::IsOnList
expensive
=================
*/
bool Brush::IsOnList(Brush& list) const
{
	Brush* b;

	for (b = list.Next(); (b != nullptr) && (b != &list); b = b->Next())
		if (b == this)
			return true;

	return false;
}

/*
=================
Brush::AddToList
=================
*/
void Brush::AddToList (Brush &list)
{
	assert((next == this) == (prev == this));
	if (next != this)
		Error("Brush::AddToList: Already linked!\n");

	next = list.next;
	prev = &list;
	next->prev = this;
	prev->next = this;
}

/*
=================
Brush::AddToList
=================
*/
void Brush::AddToListTail(Brush &list)
{
	assert((next == this) == (prev == this));
	if (next != this)
		Error("Brush::AddToList: Already linked!\n");

	next = &list;
	prev = list.prev;
	next->prev = this;
	prev->next = this;
}

/*
=================
Brush::RemoveFromList
=================
*/
void Brush::RemoveFromList()
{
	if (!IsLinked())
	{
		Error("Brush::RemoveFromList: Already unlinked.");
		return;
	}

	next->prev = prev;
	prev->next = next;
	next = prev = this;
}

/*
=================
Brush::MergeListIntoList
=================
*/
void Brush::MergeListIntoList(Brush &dest, bool tail)
{
	if (!IsLinked())
	{
//		Error("Tried to merge an empty list.");
		return;
	}
	if (tail)
	{
		// merge at tail of list
		dest.prev->next = next;
		next->prev = dest.prev;
		dest.prev = prev;
		prev->next = &dest;
	}
	else
	{
		// merge at head of list
		next->prev = &dest;
		prev->next = dest.next;
		dest.next->prev = prev;
		dest.next = next;
	}

	prev = next = this;
}

/*
==================
Brush::FreeList
==================
*/
void Brush::FreeList(Brush *pList)
{
	Brush *pBrush;
	Brush *pNext;

	pBrush = pList->next;

	while (pBrush != nullptr && pBrush != pList)
	{
		pNext = pBrush->next;
		delete pBrush;
		pBrush = pNext;
	}
}




//=========================================================================



/*
==================
Brush::Draw
==================
*/
void Brush::Draw ()
{
	int			i, order;
	Face	   *face;
    Texture *tprev = 0;
	//winding_t  *w;

//	if (owner->IsPoint() && g_cfgUI.DrawMode == CD_TEXTURED)
//		glDisable (GL_TEXTURE_2D);

	if (owner->IsPoint())
	{
		if (owner->eclass->form & EntClass::ECF_ANGLE)
			DrawFacingAngle();

		if (g_cfgUI.RadiantLights && (owner->eclass->showFlags & EFL_LIGHT))
		{
			DrawLight();
			return;
		}

		if (g_cfgUI.DrawMode == CD_TEXTURED)
			glDisable(GL_TEXTURE_2D);
	}

	// guarantee the texture will be set first
	tprev = NULL;
	for (face = faces, order = 0; face; face = face->fnext, order++)
	{
		Winding& w = face->GetWinding();
		if (!w.Count())
			continue;	// freed face

		//assert(face->texdef.tex);
		if (face->texdef.Tex() != tprev && g_cfgUI.DrawMode == CD_TEXTURED)
		{
			// set the texture for this face
			tprev = face->texdef.Tex();
			tprev->glTex.Bind();
		}

//		glColor3fv(face->d_color);
		glColor4f(face->d_color[0], face->d_color[1], face->d_color[2], 0.6f);	// lunaran TODO: pref for alpha

		// draw the polygon
		glBegin(GL_POLYGON);

	    for (i = 0; i < w.Count(); i++)
		{
			if (g_cfgUI.DrawMode == CD_TEXTURED)
				glTexCoord2fv(&w[i]->s);
			glVertex3fv(&w[i]->point[0]);
		}
		glEnd();
	}

	if (owner->IsPoint() && g_cfgUI.DrawMode == CD_TEXTURED)
		glEnable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, 0);
}

/*
=================
Brush::DrawXY
=================
*/
void Brush::DrawXY (int nViewType)
{
	int			i;
	int			order;
	Face		*face;

	if (owner->IsPoint())
	{
		if (g_cfgUI.RadiantLights && (owner->eclass->showFlags & EFL_LIGHT))
		{
 			vec3	vCorners[4];
			vec3	vTop, vBottom;
			float	fMid;

   			fMid = mins[2] + (maxs[2] - mins[2]) / 2;

			vCorners[0][0] = mins[0];
			vCorners[0][1] = mins[1];
			vCorners[0][2] = fMid;

			vCorners[1][0] = mins[0];
			vCorners[1][1] = maxs[1];
			vCorners[1][2] = fMid;

			vCorners[2][0] = maxs[0];
			vCorners[2][1] = maxs[1];
			vCorners[2][2] = fMid;

			vCorners[3][0] = maxs[0];
			vCorners[3][1] = mins[1];
			vCorners[3][2] = fMid;

			vTop[0] = mins[0] + ((maxs[0] - mins[0]) / 2);
			vTop[1] = mins[1] + ((maxs[1] - mins[1]) / 2);
			vTop[2] = maxs[2];

			vBottom = vTop;
			vBottom[2] = mins[2];
	    
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

			glBegin(GL_TRIANGLE_FAN);
			glVertex3fv(&vTop.x);
			glVertex3fv(&vCorners[0].x);
			glVertex3fv(&vCorners[1].x);
			glVertex3fv(&vCorners[2].x);
			glVertex3fv(&vCorners[3].x);
			glVertex3fv(&vCorners[0].x);
			glEnd();

			glBegin(GL_TRIANGLE_FAN);
			glVertex3fv(&vBottom.x);
			glVertex3fv(&vCorners[0].x);
			glVertex3fv(&vCorners[3].x);
			glVertex3fv(&vCorners[2].x);
			glVertex3fv(&vCorners[1].x);
			glVertex3fv(&vCorners[0].x);
			glEnd();

			DrawEntityName();
			return;
		}
	}

	for (face = faces, order = 0; face; face = face->fnext, order++)
	{
		// only draw polygons facing in a direction we care about
		if (face->plane.normal[nViewType] <= 0)
			continue;

		if (!face->HasWinding())
			continue;

		// draw the polygon
		Winding& w = face->GetWinding();
		glBegin(GL_LINE_LOOP);
	    for (i = 0; i < w.Count(); i++)
			glVertex3fv(&w[i]->point.x);
		glEnd();
	}

	// optionally add a text label
	if (g_cfgUI.ShowNames)
		DrawEntityName();
}

/*
==================
Brush::DrawFacingAngle
==================
*/
void Brush::DrawFacingAngle ()
{
	float	dist;
	vec3	forward, right, up;
	vec3	endpoint, tip1, tip2;
	vec3	start;
	int		angleVal;
	vec3	angles;

	angleVal = this->owner->GetKeyValueInt("angle");

	if (angleVal == -1)
		angles = vec3(270, 0, 0);
	else if (angleVal == -2)
		angles = vec3(90, 0, 0);
	else
		angles = vec3(0, angleVal, 0);

	AngleVectors(angles, forward, right, up);

	start = owner->brushes.ENext()->mins + owner->brushes.ENext()->maxs;
	start = start * 0.5f;
	dist = (maxs[0] - start[0]) * 2.5f;

	endpoint = start + dist * forward;
	dist = (maxs[0] - start[0]) * 0.5f;
	tip1 = endpoint + -dist * forward;
	tip1 = tip1 + -dist * up;
	tip2 = tip1 + 2 * dist * up;

	glColor4f(1, 1, 1, 1);
	glLineWidth(4);
	glBegin(GL_LINES);
	glVertex3fv(&start.x);
	glVertex3fv(&endpoint.x);
	glVertex3fv(&endpoint.x);
	glVertex3fv(&tip1.x);
	glVertex3fv(&endpoint.x);
	glVertex3fv(&tip2.x);
	glEnd();
	glLineWidth(1);
}

/*
==================
Brush::DrawEntityName
==================
*/
void Brush::DrawEntityName ()
{
	int		i;
	float	a, s, c;
	vec3	mid;

	if (!owner)
		return;	// during contruction

	if (owner->IsWorld())
		return;

	if (this != owner->brushes.ENext())
		return;	// not key brush

	// draw the angle pointer
	a = owner->GetKeyValueFloat("angle");
	if (a)
	{
		s = sin(a / 180 * Q_PI);
		c = cos(a / 180 * Q_PI);
		for (i = 0; i < 3; i++)
			mid[i] = (mins[i] + maxs[i]) * 0.5; 

		glBegin(GL_LINE_STRIP);
		glVertex3fv(&mid.x);
		mid[0] += c * 8;
		mid[1] += s * 8;
		glVertex3fv(&mid.x);
		mid[0] -= c * 4;
		mid[1] -= s * 4;
		mid[0] -= s * 4;
		mid[1] += c * 4;
		glVertex3fv(&mid.x);
		mid[0] += c * 4;
		mid[1] += s * 4;
		mid[0] += s * 4;
		mid[1] -= c * 4;
		glVertex3fv(&mid.x);
		mid[0] -= c * 4;
		mid[1] -= s * 4;
		mid[0] += s * 4;
		mid[1] -= c * 4;
		glVertex3fv(&mid.x);
		glEnd();
	}

	if (g_cfgUI.ShowNames)
	{
		std::string& name = owner->GetKeyValue("classname");
// sikk---> Draw Light Styles
		if (name._Starts_with("light"))
		{
			char sz[24];
			
			if (!owner->GetKeyValue("style").empty())
			{
				sprintf(sz, "%s (style: %s)", owner->GetKeyValue("classname").c_str(), owner->GetKeyValue("style").c_str());
				glRasterPos3f(mins[0] + 4, mins[1] + 4, mins[2] + 4);
				glCallLists(strlen(sz), GL_UNSIGNED_BYTE, sz);
			}
			else
			{
				glRasterPos3f(mins[0] + 4, mins[1] + 4, mins[2] + 4);
				glCallLists(name.length(), GL_UNSIGNED_BYTE, name.data());
			}
		}
// <---sikk
		else
		{
			glRasterPos3f(mins[0] + 4, mins[1] + 4, mins[2] + 4);
			glCallLists(name.length(), GL_UNSIGNED_BYTE, name.data());
		}
	}
}

/*
==================
Brush::DrawLight
==================
*/
void Brush::DrawLight()
{
	int		i;
	float	mag;
	float	fMid;
	vec3	color;
	vec3	vCorners[4];
	vec3	vTop, vBottom;
	vec3	vSave;

	color = vec3(1.0f);

	if(owner->GetKeyValueVector("_color", color))
	{
		mag = max(color.x, max(color.y, color.z));
		color /= mag;	// scale to a normalized light color
	}
	glColor3f(color[0], color[1], color[2]);
  
	fMid = mins[2] + (maxs[2] - mins[2]) / 2;

	vCorners[0][0] = mins[0];
	vCorners[0][1] = mins[1];
	vCorners[0][2] = fMid;

	vCorners[1][0] = mins[0];
	vCorners[1][1] = maxs[1];
	vCorners[1][2] = fMid;

	vCorners[2][0] = maxs[0];
	vCorners[2][1] = maxs[1];
	vCorners[2][2] = fMid;

	vCorners[3][0] = maxs[0];
	vCorners[3][1] = mins[1];
	vCorners[3][2] = fMid;

	vTop[0] = mins[0] + ((maxs[0] - mins[0]) / 2);
	vTop[1] = mins[1] + ((maxs[1] - mins[1]) / 2);
	vTop[2] = maxs[2];

	vBottom = vTop;
	vBottom[2] = mins[2];

	vSave = color;

	glBegin(GL_TRIANGLE_FAN);
	glVertex3fv(&vTop.x);
	for (i = 0; i <= 3; i++)
	{
		color *= 0.95f;
		glColor3f(color[0], color[1], color[2]);
		glVertex3fv(&vCorners[i].x);
	}
	glVertex3fv(&vCorners[0].x);
	glEnd();
  
	color = vSave * 0.95f;

	glBegin(GL_TRIANGLE_FAN);
	glVertex3fv(&vBottom.x);
	glVertex3fv(&vCorners[0].x);
	for (i = 3; i >= 0; i--)
	{
		color *= 0.95f;
		glColor3f(color[0], color[1], color[2]);
		glVertex3fv(&vCorners[i].x);
	}
	glEnd();
}

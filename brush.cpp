//==============================
//	brush.cpp
//==============================

#include <assert.h>
#include "qe3.h"
#include "io.h"
#include "parse.h"

/*
bool g_bMBCheck;	// sikk - This is to control the MessageBox displayed when
					// a bad inTexDef is found during saving map so it doesn't
					// continuously popup with each bad face. It's reset to 
					// "false" after saving is complete.

int	g_nBrushNumCheck;	// sikk - This is to keep multiple listings of the same
						// brush from spamming the console from bad texture name 
						// warnings when saving.
*/


//=========================================================================

/*
=============
Brush::Brush
=============
*/
Brush::Brush() :
	prev(nullptr), next(nullptr), oprev(nullptr), onext(nullptr),
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
	if (next)
		RemoveFromList();

	// unlink from entity list
	if (onext)
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
		if (f->face_winding)
			sum++;
	return sum;
}

/*
=============
Brush::MemorySize
=============
*/
int Brush::MemorySize() const
{
	int		size = 0;
	Face *f;

	for (f = faces; f; f = f->fnext)
		size += f->MemorySize();

	size += sizeof(*this);

	return size;
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
		if (!face1->face_winding)
			continue;
		for (face2 = faces; face2; face2 = face2->fnext)
		{
			if (face1 == face2)
				continue;
			if (!face2->face_winding)
				continue;
			if (Winding::PlanesConcave(face1->face_winding, face2->face_winding,
				face1->plane.normal, face2->plane.normal,
				face1->plane.dist, face2->plane.dist))
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
	vec3		pts[4][2];
	int			i, j;
	Face	   *f, *fnext;

	// with no wads loaded, a brush created has to summon a broken texture
	// from somewhere - putting it here prevents a pink garbage texture added 
	// to the list until actually needed
	if (inTexDef->tex == nullptr)
	{
		if (inTexDef->name[0])
			inTexDef->Set(inTexDef->name);
		else
			inTexDef->Set("none");
	}

	// free old faces
	for (f = faces; f; f = fnext)
	{
		fnext = f->fnext;
		delete f;
		faces = nullptr;
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

	for (i = 0; i < 4; i++)
	{
		f = new Face(this);
		f->texdef = *inTexDef;
		j = (i + 1) % 4;
		f->plane.FromPoints(pts[j][1], pts[i][1], pts[i][0]);
	}

	f = new Face(this);
	f->texdef = *inTexDef;
	f->plane.FromPoints(pts[0][1], pts[1][1], pts[2][1]);

	f = new Face(this);
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
	if (!owner->IsBrush())
		return;

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
		showFlags |= f->texdef.tex->showflags;
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
	//float		v;
	Face		*face;
	winding_t	*w;
	int			i;

	ClearBounds(mins, maxs);
	SnapPlanePoints();
	MakeFacePlanes();	// planes should be made by now but snapping makes it necessary

	for (face = faces; face; face = face->fnext)
	{
		face->owner = this;
		
		//w = face->face_winding = MakeFaceWinding(face);
		w = face->face_winding = face->MakeWinding();
		if (!w)
			continue;

		for (i = 0; i < w->numpoints; i++)
		{
			// add to bounding box
			mins = glm::min(mins, w->points[i].point);
			maxs = glm::max(maxs, w->points[i].point);
		}
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

		if (!f->face_winding)
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
Brush::CheckTexdef

sikk---> temporary fix for bug when undoing manipulation of multiple entity types. 
detect potential problems when saving the texture name and apply default tex

lunaran: what bug did this fix? is it necessary? was it ever?
=================
*/
void Brush::CheckTexdef (Face *f, char *pszName)
{
/*	if (!strlen(f->inTexDef.name))
	{
#ifdef _DEBUG
		Warning("Unexpected inTexDef.name is empty in Brush.cpp Brush_CheckTexdef()");
#endif
//		fa->inTexDef.SetName("unnamed");
		strcpy(pszName, "unnamed");
		return;
	}
*/
	/*
	// Check for texture names containing "(" (parentheses) or " " (spaces) in their maps
	if (f->texdef.name[0] == '#' || strchr(f->texdef.name, ' '))
	{
		int			nBrushNum = 0;
		char		sz[128], szMB[128];
		bool		bFound = false;
		Brush	   *b2;

		 // sikk - This seems to work but it needs more testing
		b2 = owner->brushes.onext;

		// this is to catch if the bad brush is brush #0
		if (b2 == this || b2 == &owner->brushes)
			bFound = true;

		// find brush
		while (!bFound)
		{
			b2 = b2->onext;
			if (b2 == this || b2 == &owner->brushes)
				bFound = true;
			nBrushNum++;
		}
		
		if (!g_bMBCheck)
		{
			sprintf(szMB, "Bad texture name was found.\nSaving will continue and the brush(es)\ncontaining bad faces will be listed in the console.");
			MessageBox(g_qeglobals.d_hwndMain, szMB, "QuakeEd 3: Warning", MB_OK | MB_ICONEXCLAMATION);
			g_bMBCheck = true;
		}

		if (g_nBrushNumCheck != nBrushNum)
		{
			g_nBrushNumCheck = nBrushNum;
			sprintf(sz, "Bad texture name found on brush(#%d)\n...Texture replaced with editor default.\n", nBrushNum);
			Sys_Printf("%s", sz);
		}

		strcpy(pszName, "unnamed");
		return;
	}

//	strcpy(pszName, f->inTexDef.name);
*/
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
	for (Face *face = faces; face; face = face->fnext)
		face->texdef.tex = Textures::ForName(face->texdef.name);
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
Brush::AddToList
=================
*/
void Brush::AddToList (Brush *list, bool tail)
{
	assert((!next && !prev) || (next && prev));

	if (next || prev)
		Error("Brush_AddToList: Already linked.");

	if (tail)
	{
		next = list;
		prev = list->prev;
	}
	else
	{
		next = list->next;
		prev = list;
	}
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
	assert((!next && !prev) || (next && prev));

	if (!next || !prev)
		Error("Brush_RemoveFromList: Not currently linked.");

	next->prev = prev;
	prev->next = next;
	next = prev = nullptr;
}

void Brush::CloseLinks()
{
	assert((!next && !prev) || (next && prev));

	if (next == prev && prev == this)
		return;	// done

	if (next || prev)
		Error("Brush: tried to close non-empty linked list.");

	next = prev = this;
}

/*
=================
Brush::MergeListIntoList
=================
*/
void Brush::MergeListIntoList(Brush *dest, bool tail)
{
	// properly doubly-linked lists only
	if (!next || !prev)
	{
		Error("Tried to merge a list with NULL links!\n");
		return;
	}

	if (next == this || prev == this)
	{
		Warning("Tried to merge an empty list.");
		return;
	}
	if (tail)
	{
		// merge at tail of list
		dest->prev->next = next;
		next->prev = dest->prev;
		dest->prev = prev;
		prev->next = dest;
	}
	else
	{
		// merge at head of list
		next->prev = dest;
		prev->next = dest->next;
		dest->next->prev = prev;
		dest->next = next;
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

/*
==================
Brush::CopyList
==================
*/
void Brush::CopyList(Brush *pFrom, Brush *pTo)
{
	Brush *pBrush, *pNext;

	pBrush = pFrom->next;

	while (pBrush != NULL && pBrush != pFrom)
	{
		pNext = pBrush->next;
		pBrush->RemoveFromList();
		pBrush->AddToList(pTo);
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
	winding_t  *w;

//	if (owner->IsPoint() && g_cfgUI.DrawMode == cd_texture)
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

		if (g_cfgUI.DrawMode == cd_texture)
			glDisable(GL_TEXTURE_2D);
	}

	// guarantee the texture will be set first
	tprev = NULL;
	for (face = faces, order = 0; face; face = face->fnext, order++)
	{
		w = face->face_winding;
		if (!w)
			continue;	// freed face

		assert(face->texdef.tex);
		if (face->texdef.tex != tprev && g_cfgUI.DrawMode == cd_texture)
		{
			// set the texture for this face
			tprev = face->texdef.tex;
			glBindTexture(GL_TEXTURE_2D, face->texdef.tex->texture_number);
		}

//		glColor3fv(face->d_color);
		glColor4f(face->d_color[0], face->d_color[1], face->d_color[2], 0.6f);	// lunaran TODO: pref for alpha

		// draw the polygon
		glBegin(GL_POLYGON);

	    for (i = 0; i < w->numpoints; i++)
		{
			if (g_cfgUI.DrawMode == cd_texture)
				glTexCoord2fv(&w->points[i].s);
			glVertex3fv(&w->points[i].point[0]);
		}
		glEnd();
	}

	if (owner->IsPoint() && g_cfgUI.DrawMode == cd_texture)
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
	winding_t	*w;

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

		w = face->face_winding;
		if (!w)
			continue;

		// draw the polygon
		glBegin(GL_LINE_LOOP);
	    for (i = 0; i < w->numpoints; i++)
			glVertex3fv(&w->points[i].point.x);
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

	start = owner->brushes.onext->mins + owner->brushes.onext->maxs;
	start = start * 0.5f;
	dist = (maxs[0] - start[0]) * 2.5f;

	FacingVectors(this, forward, right, up);
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
	char   *name;
	vec3	mid;

	if (!owner)
		return;	// during contruction

	if (owner->IsWorld())
		return;

	if (this != owner->brushes.onext)
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
		name = owner->GetKeyValue("classname");
// sikk---> Draw Light Styles
		if (!strcmp(name, "light"))
		{
			char sz[24];
			
			if (*owner->GetKeyValue("style"))
			{
				sprintf(sz, "%s (style: %s)", owner->GetKeyValue("classname"), owner->GetKeyValue("style"));
				glRasterPos3f(mins[0] + 4, mins[1] + 4, mins[2] + 4);
				glCallLists(strlen(sz), GL_UNSIGNED_BYTE, sz);
			}
			else
			{
				glRasterPos3f(mins[0] + 4, mins[1] + 4, mins[2] + 4);
				glCallLists(strlen(name), GL_UNSIGNED_BYTE, name);
			}
		}
// <---sikk
		else
		{
			glRasterPos3f(mins[0] + 4, mins[1] + 4, mins[2] + 4);
			glCallLists(strlen(name), GL_UNSIGNED_BYTE, name);
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
	int		n, i;
	float	fR, fG, fB;
	float	fMid;
	bool	bTriPaint;
	char   *strColor;
	vec3	vTriColor;
	vec3	vCorners[4];
	vec3	vTop, vBottom;
	vec3	vSave;

	bTriPaint = false;

	vTriColor[0] = vTriColor[1] = vTriColor[2] = 1.0;

	bTriPaint = true;

	strColor = owner->GetKeyValue("_color");

	if (strColor)
	{
		n = sscanf(strColor,"%f %f %f", &fR, &fG, &fB);
		if (n == 3)
		{
			vTriColor[0] = fR;
			vTriColor[1] = fG;
			vTriColor[2] = fB;
		}
	}
	glColor3f(vTriColor[0], vTriColor[1], vTriColor[2]);
  
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

	vSave = vTriColor;

	glBegin(GL_TRIANGLE_FAN);
	glVertex3fv(&vTop.x);
	for (i = 0; i <= 3; i++)
	{
		vTriColor[0] *= 0.95f;
		vTriColor[1] *= 0.95f;
		vTriColor[2] *= 0.95f;
		glColor3f(vTriColor[0], vTriColor[1], vTriColor[2]);
		glVertex3fv(&vCorners[i].x);
	}
	glVertex3fv(&vCorners[0].x);
	glEnd();
  
	vTriColor = vSave;
	vTriColor[0] *= 0.95f;
	vTriColor[1] *= 0.95f;
	vTriColor[2] *= 0.95f;

	glBegin(GL_TRIANGLE_FAN);
	glVertex3fv(&vBottom.x);
	glVertex3fv(&vCorners[0].x);
	for (i = 3; i >= 0; i--)
	{
		vTriColor[0] *= 0.95f;
		vTriColor[1] *= 0.95f;
		vTriColor[2] *= 0.95f;
		glColor3f(vTriColor[0], vTriColor[1], vTriColor[2]);
		glVertex3fv(&vCorners[i].x);
	}
	glEnd();
}

//==========================================================================

/*
=================
Brush::Parse

The brush is NOT linked to any list
=================
*/
Brush *Brush::Parse ()
{
	int		i, j;
	Brush	*b;
	Face	*f;

//	g_qeglobals.d_nParsedBrushes++;
	b = new Brush();
		
	do
	{
		if (!GetToken(true))
			break;
		if (!strcmp(g_szToken, "}"))
			break;
		
		f = new Face();
		// add the brush to the end of the chain, so loading and saving a map doesn't reverse the order
		if (!b->faces)
			b->faces = f;
		else
		{
			Face *scan;

			for (scan = b->faces; scan->fnext; scan = scan->fnext)
				;
			scan->fnext = f;
		}

		// read the three point plane definition
		for (i = 0; i < 3; i++)
		{
			if (i != 0)
				GetToken(true);

			if (strcmp(g_szToken, "("))
				Error("Brush_Parse: Incorrect token.");
			
			for (j = 0; j < 3; j++)
			{
				GetToken(false);
				f->plane.pts[i][j] = atof(g_szToken);	// sikk - changed to atof for BrushPrecision option
			}
			
			GetToken(false);
			if (strcmp(g_szToken, ")"))
				Error("Brush_Parse: Incorrect token.");
		}
		f->plane.Make();

		// read the texturedef
		GetToken(false);
		StringTolower(g_szToken);
		strncpy(f->texdef.name, g_szToken, MAX_TEXNAME - 1);
		f->texdef.name[MAX_TEXNAME - 1] = 0;
		GetToken(false);
		f->texdef.shift[0] = atof(g_szToken);
		GetToken(false);
		f->texdef.shift[1] = atof(g_szToken);
		GetToken(false);
		f->texdef.rotate = atof(g_szToken);	
		GetToken(false);
		f->texdef.scale[0] = atof(g_szToken);
		GetToken(false);
		f->texdef.scale[1] = atof(g_szToken);
	} while (1);

	return b;
}


/*
=================
Brush::Write
=================
*/
void Brush::Write(std::ostream& out)
{
	int		i;
	char	*pname;
	char	ftxt[16];
	Face	*fa;

	out << "{\n";
	for (fa = faces; fa; fa = fa->fnext)
	{
		for (i = 0; i < 3; i++)
			if (g_cfgEditor.BrushPrecision)	// sikk - Brush Precision
				out << "( " << fa->plane.pts[i][0] << " " << fa->plane.pts[i][1] << " " << fa->plane.pts[i][2] << " ) ";
			else
				out << "( " << (int)fa->plane.pts[i][0] << " " << (int)fa->plane.pts[i][1] << " " << (int)fa->plane.pts[i][2] << " ) ";

		pname = fa->texdef.name;
		if (pname[0] == 0)
			pname = "unnamed";

		//CheckTexdef(fa, pname);	// sikk - Check Texdef - temp fix for Multiple Entity Undo Bug

		// lunaran: moving to float shifts and rotations
		FloatToString(fa->texdef.shift[0], ftxt);
		out << pname << " " << ftxt;
		FloatToString(fa->texdef.shift[1], ftxt);
		out << " " << ftxt << " ";
		FloatToString(fa->texdef.rotate, ftxt);
		out << ftxt << " ";

		if (fa->texdef.scale[0] == (int)fa->texdef.scale[0])
			out << (int)fa->texdef.scale[0] << " ";
		else
			out << fa->texdef.scale[0] << " ";

		if (fa->texdef.scale[1] == (int)fa->texdef.scale[1])
			out << (int)fa->texdef.scale[1] << " ";
		else
			out << fa->texdef.scale[1] << " ";

		out << "\n";
	}
	out << "}\n";
}


//==========================================================================


/*
==================
FacingVectors
==================
*/
void FacingVectors(const Brush *b, vec3 &forward, vec3 &right, vec3 &up)
{
	int		angleVal;
	vec3	angles;

	angleVal = b->owner->GetKeyValueInt("angle");

	if (angleVal == -1)
	{
		angles = vec3(270, 0, 0);
	}
	else if (angleVal == -2)
	{
		angles = vec3(90, 0, 0);
	}
	else
	{
		angles = vec3(0, angleVal, 0);
	}

	AngleVectors(angles, forward, right, up);
}
//==============================
//	brush.cpp
//==============================

#include <assert.h>
#include "qe3.h"
#include "io.h"

vec3	g_v3Vecs[2];
float	g_fShift[2];

bool g_bMBCheck;	// sikk - This is to control the MessageBox displayed when
					// a bad inTexDef is found during saving map so it doesn't
					// continuously popup with each bad face. It's reset to 
					// "false" after saving is complete.

int	g_nBrushNumCheck;	// sikk - This is to keep multiple listings of the same
						// brush from spamming the console from bad texture name 
						// warnings when saving.



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

	if (g_qeglobals.d_savedinfo.nExclude & (showFlags | owner->eclass->showFlags) )
		return true;

	//if (g_qeglobals.d_savedinfo.nExclude & owner->eclass->showFlags)
	//	return true;
	/*
	if (hiddenBrush)
		return true;
	
	if (g_qeglobals.d_savedinfo.nExclude & BFL_CLIP)
		if (!strncmp(faces->texdef.name, "clip", 4))
			return true;

	if (g_qeglobals.d_savedinfo.nExclude & BFL_HINT)
		if (!strncmp(faces->texdef.name, "hint", 4))	// catches hint and hintskip
			return true;

	if (g_qeglobals.d_savedinfo.nExclude & BFL_LIQUID)
		if (faces->texdef.name[0] == '*')
			return true;

	if (g_qeglobals.d_savedinfo.nExclude & BFL_SKY)
		if (!strncmp(faces->texdef.name, "sky", 3))
			return true;

	if (g_qeglobals.d_savedinfo.nExclude & EFL_FUNCWALL)
		if (!strncmp(owner->eclass->name, "func_wall", 9))
			return true;

	if (g_qeglobals.d_savedinfo.nExclude & EFL_DETAIL)
		if (!strncmp(owner->eclass->name, "func_detail", 11))
			return true;

	if (owner->IsWorld())
	{
		if (g_qeglobals.d_savedinfo.nExclude & EFL_WORLDSPAWN)
			return true;
		return false;
	}
	else if (g_qeglobals.d_savedinfo.nExclude & EFL_POINTENTITY)
		return true;

	if (g_qeglobals.d_savedinfo.nExclude & EFL_LIGHT)
		return (owner->eclass->showFlags & ECLASS_LIGHT);

	if (g_qeglobals.d_savedinfo.nExclude & EXCLUDE_PATHS)
		return ((owner->eclass->showFlags & ECLASS_PATH) != 0);
		*/
	return false;
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
Brush::FullClone

Does NOT add the new brush to any lists
============
*/
/*
Brush *Brush::FullClone() const
{
	Brush	   *n = NULL;
	Face	   *f, *nf, *f2, *nf2;

	n = new Brush();
	n->owner = owner;
	n->mins = mins;
	n->maxs = maxs;

	for (f = faces; f; f = f->fnext)
	{
		if (f->original)
			continue;

		nf = f->FullClone(n);

		// copy all faces that have the original set to this face
		for (f2 = faces; f2; f2 = f2->fnext)
		{
			if (f2->original == f)
			{
				nf2 = f2->FullClone(n);

				// set original
				nf2->original = nf;
			}
		}
	}

	for (nf = n->faces; nf; nf = nf->fnext)
	{
		nf->ColorAndTexture();
	}

	return n;
}
*/

/*
============
Brush::ClearFaces
============
*/
void Brush::ClearFaces()
{
	Face *f, *fnext;
	for (f = faces; f; f = fnext)
	{
		fnext = f->fnext;
		delete f;
	}
	faces = nullptr;
}

/*
============
Brush::Move
============
*/
void Brush::Move(const vec3 move, const bool texturelock)
{
	Face	*f;

	if (VectorCompare(move, vec3(0))) return;
	assert(owner->IsBrush());
	for (f = faces; f; f = f->fnext)
	{
		if (texturelock)
			f->MoveTexture(move);
		f->plane.Translate(move);
	}

	Build();
}

/*
==================
Brush::Transform
==================
*/
void Brush::Transform(const glm::mat4 mat, const bool textureLock)
{
	int i;
	Face *f;
	for (f = faces; f; f = f->fnext)
	{
		//if (owner->IsBrush() && textureLock)
		//	f->MoveTexture(move);
		assert(0);	// write me
		for (i = 0; i < 3; i++)
			f->plane.pts[i] = mat * glm::vec4(f->plane.pts[i], 1);
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
	showFlags = showFlags & BFL_HIDDEN;
	for (Face *f = faces; f; f = f->fnext)
	{
		showFlags |= f->texdef.tex->showflags;
	}
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

	// move the points and edges if in select mode
	if (g_qeglobals.d_selSelectMode == sel_vertex || g_qeglobals.d_selSelectMode == sel_edge)
		SetupVertexSelection();

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
	if (g_qeglobals.d_savedinfo.bNoClamp)
		return;

	for (Face *f = faces; f; f = f->fnext)
		f->plane.Snap();
}

/*
==================
Brush::RemoveEmptyFaces

HEY: this changes the face pointer makeup of a brush, so it must only
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
		Sys_Printf("WARNING: Unexpected inTexDef.name is empty in Brush.cpp Brush_CheckTexdef()\n");
#endif
//		fa->inTexDef.SetName("unnamed");
		strcpy(pszName, "unnamed");
		return;
	}
*/
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

	//Build();
	RefreshFlags();
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
		p2[i] = p1[i] + dir[i] * 16384;

	for (f = faces; f; f = f->fnext)
	{
		d1 = DotProduct(p1, f->plane.normal) - f->plane.dist;
		d2 = DotProduct(p2, f->plane.normal) - f->plane.dist;

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
==============
Brush::SelectFaceForDragging

Adds the faces plane.pts to move_points, and
rotates and adds the plane.pts of adjacent face if shear is set
==============
*/
void Brush::SelectFaceForDragging(Face *f, bool shear)
{
	int			i;
	int			c;
	float		d;
	Brush	   *b2;
	Face	   *f2;
	winding_t  *w;

	if (owner->IsPoint())
		return;

	c = 0;
	for (i = 0; i < 3; i++)
		c += AddPlanePoint(&f->plane.pts[i]);
	if (c == 0)
		return;		// already completely added

	// select all points on this plane in all brushes the selection
	for (b2 = g_brSelectedBrushes.next; b2 != &g_brSelectedBrushes; b2 = b2->next)
	{
		if (b2 == this)
			continue;
		for (f2 = b2->faces; f2; f2 = f2->fnext)
		{
			for (i = 0; i < 3; i++)
				if (fabs(DotProduct(f2->plane.pts[i], f->plane.normal) - f->plane.dist) > ON_EPSILON)
					break;
			if (i == 3)
			{	// move this face as well
				b2->SelectFaceForDragging(f2, shear);
				break;
			}
		}
	}

	// if shearing, take all the planes adjacent to selected faces and rotate their
	// points so the edge clipped by a selcted face has two of the points
	if (!shear)
		return;

	for (f2 = faces; f2; f2 = f2->fnext)
	{
		if (f2 == f)
			continue;
		//w = MakeFaceWinding(f2);
		w = f2->MakeWinding();
		if (!w)
			continue;

		// any points on f will become new control points
		for (i = 0; i < w->numpoints; i++)
		{
			d = DotProduct(w->points[i].point, f->plane.normal) - f->plane.dist;
			if (d > -ON_EPSILON && d < ON_EPSILON)
				break;
		}

		// if none of the points were on the plane, leave it alone
		if (i != w->numpoints)
		{
			if (i == 0)
			{	// see if the first clockwise point was the last point on the winding
				d = DotProduct(w->points[w->numpoints - 1].point, f->plane.normal) - f->plane.dist;
				if (d > -ON_EPSILON && d < ON_EPSILON)
					i = w->numpoints - 1;
			}

			AddPlanePoint(&f2->plane.pts[0]);

			f2->plane.pts[0] = w->points[i].point;
			if (++i == w->numpoints)
				i = 0;

			// see if the next point is also on the plane
			d = DotProduct(w->points[i].point, f->plane.normal) - f->plane.dist;
			if (d > -ON_EPSILON && d < ON_EPSILON)
				AddPlanePoint(&f2->plane.pts[1]);

			f2->plane.pts[1] = w->points[i].point;
			if (++i == w->numpoints)
				i = 0;

			// the third point is never on the plane
			f2->plane.pts[2] = w->points[i].point;
		}

		Winding::Free(w);
	}
}


/*
==============
Brush::SideSelect

The mouse click did not hit the brush, so grab one or more side
planes for dragging
==============
*/
void Brush::SideSelect(const vec3 origin, const vec3 dir, bool shear)
{
	for (Face *f = faces; f; f = f->fnext)
	{
		if (f->TestSideSelect(origin, dir))
			SelectFaceForDragging(f, shear);
	}
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
		d = DotProduct(origin, f->plane.normal) - f->plane.dist;

		if (d >= 0)
			return false;	// point is on front side of face
	}
	return true;
}


/*
=================
Brush::MoveVertex

- The input brush must be convex
- The input brush must have face windings.
- The output brush will be convex.
- Returns true if the WHOLE vertex movement is performed.
=================
*/
bool Brush::MoveVertex(const vec3 vertex, const vec3 delta, vec3 &end)
{
	int			i, j, k, nummovefaces, result, done;
	int			movefacepoints[MAX_MOVE_FACES];
	float		dot, front, back, frac, smallestfrac;
	Face		*f, *face, *newface, *lastface, *nextface;
	Face		*movefaces[MAX_MOVE_FACES];
	Plane		plane;
	vec3		start, mid;
	winding_t	*w, tmpw;

	result = true;

#ifdef _DEBUG
	memset(movefacepoints, 0, MAX_MOVE_FACES * sizeof(int));
	memset(movefaces, 0, MAX_MOVE_FACES * sizeof(Face*));
#endif

	tmpw.numpoints = 3;
	tmpw.maxpoints = 3;
	start = vertex;
	end = vertex + delta;

	// snap or not?
	if (!g_qeglobals.d_savedinfo.bNoClamp)
		for (i = 0; i < 3; i++)
			end[i] = floor(end[i] / g_qeglobals.d_nGridSize + 0.5) * g_qeglobals.d_nGridSize;

	mid = end;

	// if the start and end are the same
	if (Point_Equal(start, end, 0.3f))
		return false;

	// the end point may not be the same as another vertex
	for (face = faces; face; face = face->fnext)
	{
		w = face->face_winding;
		if (!w)
			continue;
		for (i = 0; i < w->numpoints; i++)
		{
			if (Point_Equal(w->points[i].point, end, 0.3f))
			{
				end = vertex;
				return false;
			}
		}
	}

	done = false;

	while (!done)
	{
		// chop off triangles from all brush faces that use the to be moved vertex
		// store pointers to these chopped off triangles in movefaces[]
		nummovefaces = 0;
		for (face = faces; face; face = face->fnext)
		{
			w = face->face_winding;
			if (!w)
				continue;
			for (i = 0; i < w->numpoints; i++)
			{
				if (Point_Equal(w->points[i].point, start, 0.2f))
				{
					if (face->face_winding->numpoints <= 3)
					{
						movefacepoints[nummovefaces] = i;
						movefaces[nummovefaces++] = face;
						break;
					}

					dot = DotProduct(end, face->plane.normal) - face->plane.dist;

					// if the end point is in front of the face plane
					if (dot > 0.1)
					{
						// fanout triangle subdivision
						for (k = i; k < i + w->numpoints - 3; k++)
						{
							tmpw.points[0].point = w->points[i].point;
							tmpw.points[1].point = w->points[(k + 1) % w->numpoints].point;
							tmpw.points[2].point = w->points[(k + 2) % w->numpoints].point;

							newface = face->Clone();

							// get the original
							for (f = face; f->original; f = f->original)
								;
							newface->original = f;

							// store the new winding
							if (newface->face_winding)
								Winding::Free(newface->face_winding);
							newface->face_winding = Winding::Clone(&tmpw);

							// get the texture information

							// add the face to the brush
							newface->fnext = faces;
							faces = newface;
							newface->owner = this;

							// add this new triangle to the move faces
							movefacepoints[nummovefaces] = 0;
							movefaces[nummovefaces++] = newface;
						}

						// give the original face a new winding
						tmpw.points[0].point = w->points[(i - 2 + w->numpoints) % w->numpoints].point;
						tmpw.points[1].point = w->points[(i - 1 + w->numpoints) % w->numpoints].point;
						tmpw.points[2].point = w->points[i].point;
						Winding::Free(face->face_winding);
						face->face_winding = Winding::Clone(&tmpw);

						// add the original face to the move faces
						movefacepoints[nummovefaces] = 2;
						movefaces[nummovefaces++] = face;
					}
					else
					{
						// chop a triangle off the face
						tmpw.points[0].point = w->points[(i - 1 + w->numpoints) % w->numpoints].point;
						tmpw.points[1].point = w->points[i].point;
						tmpw.points[2].point = w->points[(i + 1) % w->numpoints].point;

						Winding::RemovePoint(w, i);		// remove the point from the face winding
						face->ColorAndTexture();		// get texture crap right
						newface = face->Clone();		// make a triangle face

						// get the original
						for (f = face; f->original; f = f->original)
							;
						newface->original = f;

						// store the new winding
						if (newface->face_winding)
							Winding::Free(newface->face_winding);
						newface->face_winding = Winding::Clone(&tmpw);

						// add the face to the brush
						newface->fnext = faces;
						faces = newface;
						newface->owner = this;

						movefacepoints[nummovefaces] = 1;
						movefaces[nummovefaces++] = newface;
					}
					break;
				}
			}
		}

		// now movefaces contains pointers to triangle faces that
		// contain the to be moved vertex
		done = true;

		mid = end;
		smallestfrac = 1;

		for (face = faces; face; face = face->fnext)
		{
			// check if there is a move face that has this face as the original
			for (i = 0; i < nummovefaces; i++)
				if (movefaces[i]->original == face)
					break;

			if (i >= nummovefaces)
				continue;

			// check if the original is not a move face itself
			for (j = 0; j < nummovefaces; j++)
				if (face == movefaces[j])
					break;

			// if the original is not a move face itself
			if (j >= nummovefaces)
				memcpy(&plane, &movefaces[i]->original->plane, sizeof(Plane));
			else
			{
				k = movefacepoints[j];
				w = movefaces[j]->face_winding;
				tmpw.points[0].point = w->points[(k + 1) % w->numpoints].point;
				tmpw.points[1].point = w->points[(k + 2) % w->numpoints].point;
				//
				k = movefacepoints[i];
				w = movefaces[i]->face_winding;
				tmpw.points[2].point = w->points[(k + 1) % w->numpoints].point;
				if (!plane.FromPoints(tmpw.points[0].point, tmpw.points[1].point, tmpw.points[2].point))
				{
					tmpw.points[2].point = w->points[(k + 2) % w->numpoints].point;
					if (!plane.FromPoints(tmpw.points[0].point, tmpw.points[1].point, tmpw.points[2].point))
						// this should never happen otherwise the face merge did a crappy job a previous pass
						continue;
				}
			}

			// now we've got the plane to check agains
			front = DotProduct(start, plane.normal) - plane.dist;
			back = DotProduct(end, plane.normal) - plane.dist;

			// if the whole move is at one side of the plane
			if (front < 0.01 && back < 0.01)
				continue;
			if (front > -0.01 && back > -0.01)
				continue;

			// if there's no movement orthogonal to this plane at all
			if (fabs(front - back) < 0.001)
				continue;

			// ok first only move till the plane is hit
			frac = front / (front - back);
			if (frac < smallestfrac)
			{
				mid = start + (end - start) * frac;
				smallestfrac = frac;
			}

			done = false;
		}

		// move the vertex
		for (i = 0; i < nummovefaces; i++)
		{
			// move vertex to end position
			movefaces[i]->face_winding->points[movefacepoints[i]].point = mid;

			// create new face plane
			if (!movefaces[i]->plane.FromPoints(movefaces[i]->face_winding->points[0].point,
				movefaces[i]->face_winding->points[1].point,
				movefaces[i]->face_winding->points[2].point))
				result = false;
		}

		// if the brush is no longer convex
		if (!result || !IsConvex())
		{
			for (i = 0; i < nummovefaces; i++)
			{
				// move the vertex back to the initial position
				movefaces[i]->face_winding->points[movefacepoints[i]].point = start;
				// create new face plane
				movefaces[i]->plane.FromPoints(movefaces[i]->face_winding->points[0].point,
					movefaces[i]->face_winding->points[1].point,
					movefaces[i]->face_winding->points[2].point);
			}
			result = false;
			end = start;
			done = true;
		}
		else
			start = mid;

		// get texture crap right
		for (i = 0; i < nummovefaces; i++)
			movefaces[i]->ColorAndTexture();

		// now try to merge faces with their original faces
		lastface = NULL;
		for (face = faces; face; face = nextface)
		{
			nextface = face->fnext;
			if (!face->original)
			{
				lastface = face;
				continue;
			}
			if (!face->plane.EqualTo(&face->original->plane, false))
			{
				lastface = face;
				continue;
			}
			w = Winding::TryMerge(*face->face_winding, *face->original->face_winding, face->plane.normal, true);
			if (!w)
			{
				lastface = face;
				continue;
			}
			Winding::Free(face->original->face_winding);
			face->original->face_winding = w;

			// get texture crap right
			face->original->ColorAndTexture();

			// remove the face that was merged with the original
			if (lastface)
				lastface->fnext = face->fnext;
			else
				faces = face->fnext;
			delete face;
		}
	}
	return (result != 0);
}

/*
=================
Brush::ResetFaceOriginals
=================
*/
void Brush::ResetFaceOriginals()
{
	Face *f;

	for (f = faces; f; f = f->fnext)
		f->original = nullptr;
}
// <---sikk



//=========================================================================


void Brush::MakeCzgCylinder(int degree)
{
	int axis;
	Brush* b;

	if (!QE_SingleBrush())
	{
		Sys_Printf("WARNING: Must have a single brush selected.\n");
		return;
	}

	// lunaran - grid view reunification
	axis = QE_BestViewAxis();

	b = g_brSelectedBrushes.next;
	CmdCzgCylinder *cmdCzgC = new CmdCzgCylinder();

	cmdCzgC->SetAxis(axis);
	cmdCzgC->SetDegree(degree);
	cmdCzgC->UseBrush(b);

	g_cmdQueue.Complete(cmdCzgC);

	Sys_UpdateWindows(W_SCENE);
}


/*
=============
Brush::MakeSided

Makes the current brush have the given number of 2d sides
=============
*/
void Brush::MakeSided(int sides)
{
	int axis;
	Brush* b;

	if (!QE_SingleBrush())
	{
		Sys_Printf("WARNING: Must have a single brush selected.\n");
		return;
	}

	// lunaran - grid view reunification
	axis = QE_BestViewAxis();

	b = g_brSelectedBrushes.next;
	CmdCylinder	*cmdCyl = new CmdCylinder();

	cmdCyl->SetAxis(axis);
	cmdCyl->SetSides(sides);
	cmdCyl->UseBrush(b);

	g_cmdQueue.Complete(cmdCyl);
	
	Sys_UpdateWindows(W_SCENE);
}


// sikk---> Brush Primitives
/*
=============
Brush::MakeSidedCone

Makes the current brush have the given number of 2D sides and turns it into a cone
=============
*/
void Brush::MakeSidedCone(int sides)
{
	int axis;
	Brush* b;

	if (!QE_SingleBrush())
	{
		Sys_Printf("WARNING: Must have a single brush selected.\n");
		return;
	}

	// lunaran - grid view reunification
	axis = QE_BestViewAxis();

	b = g_brSelectedBrushes.next;
	CmdCone	*cmdCone = new CmdCone();

	cmdCone->SetAxis(axis);
	cmdCone->SetSides(sides);
	cmdCone->UseBrush(b);

	g_cmdQueue.Complete(cmdCone);

	Sys_UpdateWindows(W_SCENE);
}

/*
=============
Brush::MakeSidedSphere

Makes the current brush have the given number of 2d sides and turns it into a sphere
=============
*/
void Brush::MakeSidedSphere(int sides)
{
	Brush* b;

	if (!QE_SingleBrush())
	{
		Sys_Printf("WARNING: Must have a single brush selected.\n");
		return;
	}

	b = g_brSelectedBrushes.next;
	CmdSphere *cmdSph = new CmdSphere();

	cmdSph->SetSides(sides);
	cmdSph->UseBrush(b);

	g_cmdQueue.Complete(cmdSph);

	Sys_UpdateWindows(W_SCENE);

}
// <---sikk



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
		Sys_Printf("WARNING: Tried to merge an empty list.\n");
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

//	if (owner->IsPoint() && g_qeglobals.d_vCamera.draw_mode == cd_texture)
//		glDisable (GL_TEXTURE_2D);

	if (owner->IsPoint())
	{
		//if (!(g_qeglobals.d_savedinfo.nExclude & EXCLUDE_ANGLES) && (owner->eclass->showFlags & ECLASS_ANGLE))
		if (owner->eclass->form & EntClass::ECF_ANGLE)
			DrawFacingAngle();

		if (g_qeglobals.d_savedinfo.bRadiantLights && (owner->eclass->showFlags & EFL_LIGHT))
		{
			DrawLight();
			return;
		}

		if (g_qeglobals.d_vCamera.draw_mode == cd_texture)
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
		if (face->texdef.tex != tprev && g_qeglobals.d_vCamera.draw_mode == cd_texture)
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
			if (g_qeglobals.d_vCamera.draw_mode == cd_texture)
				glTexCoord2fv(&w->points[i].s);
			glVertex3fv(&w->points[i].point[0]);
		}
		glEnd();
	}

	if (owner->IsPoint() && g_qeglobals.d_vCamera.draw_mode == cd_texture)
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
		if (g_qeglobals.d_savedinfo.bRadiantLights && (owner->eclass->showFlags & EFL_LIGHT))
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
	if (g_qeglobals.d_savedinfo.bShow_Names)
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

	if (g_qeglobals.d_savedinfo.bShow_Names)
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

	g_qeglobals.d_nParsedBrushes++;
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
		f->texdef.Set(g_szToken);
		GetToken(false);
		f->texdef.shift[0] = atoi(g_szToken);
		GetToken(false);
		f->texdef.shift[1] = atoi(g_szToken);
		GetToken(false);
		f->texdef.rotate = atoi(g_szToken);	
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
	char   *pname;
	Face *fa;

	out << "{\n";
	for (fa = faces; fa; fa = fa->fnext)
	{
		for (i = 0; i < 3; i++)
			if (g_qeglobals.d_savedinfo.bBrushPrecision)	// sikk - Brush Precision
				out << "( " << fa->plane.pts[i][0] << " " << fa->plane.pts[i][1] << " " << fa->plane.pts[i][2] << " ) ";
			else
				out << "( " << (int)fa->plane.pts[i][0] << " " << (int)fa->plane.pts[i][1] << " " << (int)fa->plane.pts[i][2] << " ) ";

		pname = fa->texdef.name;
		if (pname[0] == 0)
			pname = "unnamed";

		CheckTexdef(fa, pname);	// sikk - Check Texdef - temp fix for Multiple Entity Undo Bug

		out << pname << " " << (int)fa->texdef.shift[0] << " " << (int)fa->texdef.shift[1] << " " << (int)fa->texdef.rotate << " ";

		if (fa->texdef.scale[0] == (int)fa->texdef.scale[0])
			out << (int)fa->texdef.scale[0] << " ";
		else
			out << (float)fa->texdef.scale[0] << " ";

		if (fa->texdef.scale[1] == (int)fa->texdef.scale[1])
			out << (int)fa->texdef.scale[1] << " ";
		else
			out << (float)fa->texdef.scale[1] << " ";

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
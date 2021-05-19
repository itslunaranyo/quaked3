//==============================
//	brush.cpp
//==============================

#include <assert.h>
#include "qe3.h"
#include "io.h"

vec3_t  g_v3Vecs[2];
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
	prev(nullptr), next(nullptr), 
	oprev(nullptr), onext(nullptr),
	owner(nullptr), brush_faces(nullptr),
	undoId(0), redoId(0), ownerId(0)
{
	ClearBounds(mins, maxs);
	hiddenBrush = false;
}

/*
=============
Brush::~Brush

Frees the brush with all of its faces and display list.
Unlinks the brush from whichever chain it is in.
Decrements the owner entity's brushcount.
Removes owner entity if this was the last brush unless owner is the world.
=============
*/
Brush::~Brush()
{
	Face *f, *fnext;

	// free faces
	for (f = brush_faces; f; f = fnext)
	{
		fnext = f->next;
		delete f;
	}

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
	for (f = brush_faces; f; f = f->next)
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

	for (f = brush_faces; f; f = f->next)
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

	for (face1 = brush_faces; face1; face1 = face1->next)
	{
		if (!face1->face_winding)
			continue;
		for (face2 = brush_faces; face2; face2 = face2->next)
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

	if (hiddenBrush)
		return true;
	
	if (g_qeglobals.d_savedinfo.nExclude & EXCLUDE_CLIP)
		if (!strncmp(brush_faces->texdef.name, "clip", 4))
			return true;

	if (g_qeglobals.d_savedinfo.nExclude & EXCLUDE_HINT)
		if (!strncmp(brush_faces->texdef.name, "hint", 4))	// catches hint and hintskip
			return true;

	if (g_qeglobals.d_savedinfo.nExclude & EXCLUDE_WATER)
		if (brush_faces->texdef.name[0] == '*')
			return true;

	if (g_qeglobals.d_savedinfo.nExclude & EXCLUDE_SKY)
		if (!strncmp(brush_faces->texdef.name, "sky", 3))
			return true;

	if (g_qeglobals.d_savedinfo.nExclude & EXCLUDE_FUNC_WALL)
		if (!strncmp(owner->eclass->name, "func_wall", 9))
			return true;

	if (g_qeglobals.d_savedinfo.nExclude & EXCLUDE_DETAIL)
		if (!strncmp(owner->eclass->name, "func_detail", 11))
			return true;

	if (owner == g_map.world)
	{
		if (g_qeglobals.d_savedinfo.nExclude & EXCLUDE_WORLD)
			return true;
		return false;
	}
	else if (g_qeglobals.d_savedinfo.nExclude & EXCLUDE_ENT)
		return true;

	if (g_qeglobals.d_savedinfo.nExclude & EXCLUDE_LIGHTS)
		return (owner->eclass->nShowFlags & ECLASS_LIGHT);

	if (g_qeglobals.d_savedinfo.nExclude & EXCLUDE_PATHS)
		return ((owner->eclass->nShowFlags & ECLASS_PATH) != 0);

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
Brush *Brush::Create(vec3_t mins, vec3_t maxs, texdef_t *texdef)
{
	Brush	   *b;

	for (int i = 0; i < 3; i++)
	{
		if (maxs[i] <= mins[i])
			Error("Brush_InitSolid: Backwards.");
	}

	b = new Brush();

	b->Recreate(mins, maxs, texdef);
	return b;
}

void Brush::Recreate(vec3_t inMins, vec3_t inMaxs, texdef_t *inTexDef)
{
	vec3_t		pts[4][2];
	int			i, j;
	Face	   *f, *fnext;

	// free old faces
	for (f = brush_faces; f; f = fnext)
	{
		fnext = f->next;
		delete f;
		brush_faces = nullptr;
	}

	VectorCopy(inMins, mins);
	VectorCopy(inMaxs, maxs);

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

		VectorCopy(pts[j][1], f->planepts[0]);
		VectorCopy(pts[i][1], f->planepts[1]);
		VectorCopy(pts[i][0], f->planepts[2]);
	}

	f = new Face(this);
	f->texdef = *inTexDef;

	VectorCopy(pts[0][1], f->planepts[0]);
	VectorCopy(pts[1][1], f->planepts[1]);
	VectorCopy(pts[2][1], f->planepts[2]);

	f = new Face(this);
	f->texdef = *inTexDef;

	VectorCopy(pts[2][0], f->planepts[0]);
	VectorCopy(pts[1][0], f->planepts[1]);
	VectorCopy(pts[0][0], f->planepts[2]);
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

	for (f = brush_faces; f; f = f->next)
	{
		nf = f->Clone();
		nf->next = n->brush_faces;
		n->brush_faces = nf;
	}

	return n;
}

/*
============
Brush::FullClone

Does NOT add the new brush to any lists
============
*/
Brush *Brush::FullClone() const
{
	Brush	   *n = NULL;
	Face	   *f, *nf, *f2, *nf2;

	n = new Brush();
	n->owner = owner;
	VectorCopy(mins, n->mins);
	VectorCopy(maxs, n->maxs);

	for (f = brush_faces; f; f = f->next)
	{
		if (f->original)
			continue;

		nf = f->FullClone(n);

		// copy all faces that have the original set to this face
		for (f2 = brush_faces; f2; f2 = f2->next)
		{
			if (f2->original == f)
			{
				nf2 = f2->FullClone(n);

				// set original
				nf2->original = nf;
			}
		}
	}

	for (nf = n->brush_faces; nf; nf = nf->next)
	{
		nf->ColorAndTexture();
	}

	return n;
}

/*
============
Brush::Move
============
*/
void Brush::Move(vec3_t move)
{
	int		i;
	Face *f;

	for (f = brush_faces; f; f = f->next)
	{
		if (!owner->eclass->IsFixedSize())
			if (g_qeglobals.d_bTextureLock)
				f->MoveTexture(move);

		for (i = 0; i < 3; i++)
			VectorAdd(f->planepts[i], move, f->planepts[i]);
	}

	Build();

	// PGM - keep the origin vector up to date on fixed size entities.
	if (owner->eclass->IsFixedSize() && onext != this)
	{
		// lunaran: update everything
		owner->SetOriginFromBrush();
	//	VectorAdd(owner->origin, move, owner->origin);

		// lunaran TODO: update only once at the end of a drag or the window flickers too much
		//EntWnd_UpdateUI();
	}
}

/*
==================
Brush::Build

Builds a brush rendering data and also sets the min/max bounds
==================
*/
void Brush::Build()
{
	vec_t		v;
	Face		*face;
	winding_t	*w;

	ClearBounds(mins, maxs);
	SnapPlanePoints();
	MakeFacePlanes();

	for (face = brush_faces; face; face = face->next)
	{
		int	i, j;
		face->owner = this;
		
		w = face->face_winding = MakeFaceWinding(face);
		if (!w)
			continue;

		// lunaran TODO: doesn't need to happen this often, move to face_settexdef
		// also duplicate this somewhere else so it still happens for all faces during map_buildbrushdata
		face->d_texture = Textures::ForName(face->texdef.name);

		for (i = 0; i < w->numpoints; i++)
		{
			// add to bounding box
			for (j = 0; j < 3; j++)
			{
				v = w->points[i][j];
				if (v > maxs[j])
					maxs[j] = v;
				if (v < mins[j])
					mins[j] = v;
			}
		}
		// setup s and t vectors, and set color
		face->ColorAndTexture();
	}

	g_map.modified = true;	// mark the map as changed

	// move the points and edges if in select mode
	if (g_qeglobals.d_selSelectMode == sel_vertex || g_qeglobals.d_selSelectMode == sel_edge)
		SetupVertexSelection();
}

/*
=================
Brush::MakeFaceWinding

returns the visible polygon on a face
=================
*/
winding_t *Brush::MakeFaceWinding(Face *face)
{
	bool		past;
	Face	   *clip;
	Plane		plane;
	winding_t  *w;

	// get a poly that covers an effectively infinite area
	w = face->plane.BasePoly();

	// chop the poly by all of the other faces
	past = false;
	for (clip = brush_faces; clip && w; clip = clip->next)
	{
		if (clip == face)
		{
			past = true;
			continue;
		}

		if (DotProduct(face->plane.normal, clip->plane.normal) > 0.999 &&
			fabs(face->plane.dist - clip->plane.dist) < 0.01)
		{	// identical plane, use the later one
			if (past)
			{
				Winding::Free(w);
				return NULL;
			}
			continue;
		}

		// flip the plane, because we want to keep the back side
		VectorSubtract(g_v3VecOrigin, clip->plane.normal, plane.normal);
		plane.dist = -clip->plane.dist;

		w = Winding::Clip(w, &plane, false);
		if (!w)
			return w;
	}

	if (w->numpoints < 3)
	{
		Winding::Free(w);
		w = NULL;
	}

	if (!w)
		printf("unused plane\n");

	return w;
}

/*
==================
Brush::MakeFacePlanes
==================
*/
void Brush::MakeFacePlanes()
{
	int		i;
	Face *f;
	vec3_t	t1, t2, t3;

	for (f = brush_faces; f; f = f->next)
	{
		// convert to a vector / dist plane
		for (i = 0; i < 3; i++)
		{
			t1[i] = f->planepts[0][i] - f->planepts[1][i];
			t2[i] = f->planepts[2][i] - f->planepts[1][i];
			t3[i] = f->planepts[1][i];
		}

		CrossProduct(t1, t2, f->plane.normal);

		if (VectorCompare(f->plane.normal, g_v3VecOrigin))
			printf("WARNING: Brush plane with no normal\n");

		VectorNormalize(f->plane.normal);
		f->plane.dist = DotProduct(t3, f->plane.normal);
	}
}

/*
==================
Brush::SnapPlanePoints
==================
*/
void Brush::SnapPlanePoints()
{
	int		i, j;
	Face *f;

	if (g_qeglobals.d_savedinfo.bNoClamp)
		return;

	for (f = brush_faces; f; f = f->next)
		for (i = 0; i < 3; i++)
			for (j = 0; j < 3; j++)
				f->planepts[i][j] = floor(f->planepts[i][j] + 0.5);
}

/*
==================
Brush::RemoveEmptyFaces

Frees any overconstraining faces
==================
*/
void Brush::RemoveEmptyFaces()
{
	Face	*f, *next;

	f = brush_faces;
	brush_faces = NULL;

	for (; f; f = next)
	{
		next = f->next;

		if (!f->face_winding)
			delete f;
		else
		{
			f->next = brush_faces;
			brush_faces = f;
		}
	}
}



/*
=================
Brush::CheckTexdef

sikk---> temporary fix for bug when undoing manipulation of multiple entity types. 
detect potential problems when saving the texture name and apply default tex
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
	if (f->texdef.name[0] == '(' || strchr(f->texdef.name, ' '))
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
	Face *face;

	for (face = brush_faces; face; face = face->next)
		face->FitTexture(nHeight, nWidth);
}

/*
=================
Brush::SetTexture
=================
*/
void Brush::SetTexture(texdef_t *texdef, int nSkipFlags)
{
	Face *f;

	for (f = brush_faces; f; f = f->next)
		f->SetTexture(texdef, nSkipFlags);

	Build();
}

/*
==============
Brush::RayTest

Intersects a ray with a brush
Returns the face hit and the distance along the ray the intersection occured at
Returns NULL and 0 if not hit at all
==============
*/
Face *Brush::RayTest(vec3_t origin, vec3_t dir, float *dist)
{
	int		i;
	float	frac, d1, d2;
	vec3_t	p1, p2;
	Face *f, *firstface;

	VectorCopy(origin, p1);

	for (i = 0; i < 3; i++)
		p2[i] = p1[i] + dir[i] * 16384;

	for (f = brush_faces; f; f = f->next)
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
	VectorSubtract(p1, origin, p1);
	d1 = DotProduct(p1, dir);

	*dist = d1;

	return firstface;
}




/*
==============
Brush::SelectFaceForDragging

Adds the faces planepts to move_points, and
rotates and adds the planepts of adjacent face if shear is set
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

	if (owner->eclass->IsFixedSize())
		return;

	c = 0;
	for (i = 0; i < 3; i++)
		c += AddPlanePoint(f->planepts[i]);
	if (c == 0)
		return;		// already completely added

	// select all points on this plane in all brushes the selection
	for (b2 = g_brSelectedBrushes.next; b2 != &g_brSelectedBrushes; b2 = b2->next)
	{
		if (b2 == this)
			continue;
		for (f2 = b2->brush_faces; f2; f2 = f2->next)
		{
			for (i = 0; i < 3; i++)
				if (fabs(DotProduct(f2->planepts[i], f->plane.normal) - f->plane.dist) > ON_EPSILON)
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

	for (f2 = brush_faces; f2; f2 = f2->next)
	{
		if (f2 == f)
			continue;
		w = MakeFaceWinding(f2);
		if (!w)
			continue;

		// any points on f will become new control points
		for (i = 0; i < w->numpoints; i++)
		{
			d = DotProduct(w->points[i], f->plane.normal) - f->plane.dist;
			if (d > -ON_EPSILON && d < ON_EPSILON)
				break;
		}

		// if none of the points were on the plane, leave it alone
		if (i != w->numpoints)
		{
			if (i == 0)
			{	// see if the first clockwise point was the last point on the winding
				d = DotProduct(w->points[w->numpoints - 1], f->plane.normal) - f->plane.dist;
				if (d > -ON_EPSILON && d < ON_EPSILON)
					i = w->numpoints - 1;
			}

			AddPlanePoint(f2->planepts[0]);

			VectorCopy(w->points[i], f2->planepts[0]);
			if (++i == w->numpoints)
				i = 0;

			// see if the next point is also on the plane
			d = DotProduct(w->points[i], f->plane.normal) - f->plane.dist;
			if (d > -ON_EPSILON && d < ON_EPSILON)
				AddPlanePoint(f2->planepts[1]);

			VectorCopy(w->points[i], f2->planepts[1]);
			if (++i == w->numpoints)
				i = 0;

			// the third point is never on the plane
			VectorCopy(w->points[i], f2->planepts[2]);
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
void Brush::SideSelect(vec3_t origin, vec3_t dir, bool shear)
{
	Face	*f, *f2;
	vec3_t	 p1, p2;

	for (f = brush_faces; f; f = f->next)
	{
		VectorCopy(origin, p1);
		VectorMA(origin, 16384, dir, p2);

		for (f2 = brush_faces; f2; f2 = f2->next)
		{
			if (f2 == f)
				continue;
			f2->ClipLine(p1, p2);
		}

		if (f2)
			continue;
		if (VectorCompare(p1, origin))
			continue;
		if (f->ClipLine(p1, p2))
			continue;

		SelectFaceForDragging(f, shear);
	}
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
bool Brush::MoveVertex(vec3_t vertex, vec3_t delta, vec3_t end)
{
	int			i, j, k, nummovefaces, result, done;
	int			movefacepoints[MAX_MOVE_FACES];
	float		dot, front, back, frac, smallestfrac;
	Face	   *f, *face, *newface, *lastface, *nextface;
	Face	   *movefaces[MAX_MOVE_FACES];
	Plane		plane;
	vec3_t		start, mid;
	winding_t  *w, tmpw;

	result = true;

	tmpw.numpoints = 3;
	tmpw.maxpoints = 3;
	VectorCopy(vertex, start);
	VectorAdd(vertex, delta, end);

	// snap or not?
	if (!g_qeglobals.d_savedinfo.bNoClamp)
		for (i = 0; i < 3; i++)
			end[i] = floor(end[i] / g_qeglobals.d_nGridSize + 0.5) * g_qeglobals.d_nGridSize;

	VectorCopy(end, mid);

	// if the start and end are the same
	if (Point_Equal(start, end, 0.3f))
		return false;

	// the end point may not be the same as another vertex
	for (face = brush_faces; face; face = face->next)
	{
		w = face->face_winding;
		if (!w)
			continue;
		for (i = 0; i < w->numpoints; i++)
		{
			if (Point_Equal(w->points[i], end, 0.3f))
			{
				VectorCopy(vertex, end);
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
		for (face = brush_faces; face; face = face->next)
		{
			w = face->face_winding;
			if (!w)
				continue;
			for (i = 0; i < w->numpoints; i++)
			{
				if (Point_Equal(w->points[i], start, 0.2f))
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
							VectorCopy(w->points[i], tmpw.points[0]);
							VectorCopy(w->points[(k + 1) % w->numpoints], tmpw.points[1]);
							VectorCopy(w->points[(k + 2) % w->numpoints], tmpw.points[2]);

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
							newface->d_texture = face->d_texture;

							// add the face to the brush
							newface->next = brush_faces;
							brush_faces = newface;

							// add this new triangle to the move faces
							movefacepoints[nummovefaces] = 0;
							movefaces[nummovefaces++] = newface;
						}

						// give the original face a new winding
						VectorCopy(w->points[(i - 2 + w->numpoints) % w->numpoints], tmpw.points[0]);
						VectorCopy(w->points[(i - 1 + w->numpoints) % w->numpoints], tmpw.points[1]);
						VectorCopy(w->points[i], tmpw.points[2]);
						Winding::Free(face->face_winding);
						face->face_winding = Winding::Clone(&tmpw);

						// add the original face to the move faces
						movefacepoints[nummovefaces] = 2;
						movefaces[nummovefaces++] = face;
					}
					else
					{
						// chop a triangle off the face
						VectorCopy(w->points[(i - 1 + w->numpoints) % w->numpoints], tmpw.points[0]);
						VectorCopy(w->points[i], tmpw.points[1]);
						VectorCopy(w->points[(i + 1) % w->numpoints], tmpw.points[2]);

						// remove the point from the face winding
						Winding::RemovePoint(w, i);

						// get texture crap right
						face->ColorAndTexture();

						// make a triangle face
						newface = face->Clone();

						// get the original
						for (f = face; f->original; f = f->original)
							;
						newface->original = f;

						// store the new winding
						if (newface->face_winding)
							Winding::Free(newface->face_winding);
						newface->face_winding = Winding::Clone(&tmpw);

						// get the texture
						newface->d_texture = face->d_texture;
						//newface->d_texture = Texture_ForName(newface->inTexDef.name);

						// add the face to the brush
						newface->next = brush_faces;
						brush_faces = newface;

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

		VectorCopy(end, mid);
		smallestfrac = 1;

		for (face = brush_faces; face; face = face->next)
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
				VectorCopy(w->points[(k + 1) % w->numpoints], tmpw.points[0]);
				VectorCopy(w->points[(k + 2) % w->numpoints], tmpw.points[1]);
				//
				k = movefacepoints[i];
				w = movefaces[i]->face_winding;
				VectorCopy(w->points[(k + 1) % w->numpoints], tmpw.points[2]);
				if (!plane.FromPoints(tmpw.points[0], tmpw.points[1], tmpw.points[2]))
				{
					VectorCopy(w->points[(k + 2) % w->numpoints], tmpw.points[2]);
					if (!plane.FromPoints(tmpw.points[0], tmpw.points[1], tmpw.points[2]))
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
				mid[0] = start[0] + (end[0] - start[0]) * frac;
				mid[1] = start[1] + (end[1] - start[1]) * frac;
				mid[2] = start[2] + (end[2] - start[2]) * frac;
				smallestfrac = frac;
			}

			done = false;
		}

		// move the vertex
		for (i = 0; i < nummovefaces; i++)
		{
			// move vertex to end position
			VectorCopy(mid, movefaces[i]->face_winding->points[movefacepoints[i]]);

			// create new face plane
			for (j = 0; j < 3; j++)
				VectorCopy(movefaces[i]->face_winding->points[j], movefaces[i]->planepts[j]);

			movefaces[i]->MakePlane();
			if (VectorLength(movefaces[i]->plane.normal) < 0.1)
				result = false;
		}

		// if the brush is no longer convex
		if (!result || !IsConvex())
		{
			for (i = 0; i < nummovefaces; i++)
			{
				// move the vertex back to the initial position
				VectorCopy(start, movefaces[i]->face_winding->points[movefacepoints[i]]);
				// create new face plane
				for (j = 0; j < 3; j++)
					VectorCopy(movefaces[i]->face_winding->points[j], movefaces[i]->planepts[j]);

				movefaces[i]->MakePlane();
			}
			result = false;
			VectorCopy(start, end);
			done = true;
		}
		else
			VectorCopy(mid, start);

		// get texture crap right
		for (i = 0; i < nummovefaces; i++)
		{
			movefaces[i]->ColorAndTexture();
		}

		// now try to merge faces with their original faces
		lastface = NULL;
		for (face = brush_faces; face; face = nextface)
		{
			nextface = face->next;
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
			w = Winding::TryMerge(face->face_winding, face->original->face_winding, face->plane.normal, true);
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
				lastface->next = face->next;
			else
				brush_faces = face->next;
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
	Face *face;

	for (face = brush_faces; face; face = face->next)
		face->original = NULL;
}
// <---sikk



//=========================================================================


/*
=============
Brush::MakeSided

Makes the current brush have the given number of 2d sides
=============
*/
void Brush::MakeSided(int sides)
{
	int			i, axis;
	float		width;
	float		sv, cv;
	vec3_t		mins, maxs;
	vec3_t		mid;
	Brush	   *b;
	Face	   *f;
	texdef_t   *texdef;
	int			nViewType;	// sikk - Multiple Orthographic Views

	if (sides < 3)
	{
		Sys_Printf("WARNING: Brush must have at least 3 sides.\n");
		return;
	}

	if (sides >= MAX_POINTS_ON_WINDING - 4)
	{
		Sys_Printf("WARNING: Couldn't create brush. Too many sides.\n");
		return;
	}

	if (!QE_SingleBrush())
	{
		Sys_Printf("WARNING: Must have a single brush selected.\n");
		return;
	}

	b = g_brSelectedBrushes.next;
	VectorCopy(b->mins, mins);
	VectorCopy(b->maxs, maxs);
	texdef = &g_qeglobals.d_workTexDef;

	delete b;

	// lunaran - grid view reunification
	XYZView* xyz = XYZWnd_WinFromHandle(GetTopWindow(g_qeglobals.d_hwndMain));
	if (xyz)
		nViewType = xyz->dViewType;
	else
		nViewType = XY;

	switch (nViewType)
	{
	case XY:
		axis = 2;
		break;
	case XZ:
		axis = 1;
		break;
	case YZ:
		axis = 0;
		break;
	}

	// find center of brush
	width = 8;

	for (i = 0; i < 3; i++)
	{
		mid[i] = (maxs[i] + mins[i]) * 0.5;
		if (i == axis)
			continue;

		if ((maxs[i] - mins[i]) * 0.5 > width)
			width = (maxs[i] - mins[i]) * 0.5;
	}

	b = new Brush();

	// create top face
	f = new Face(b);
	f->texdef = *texdef;

	f->planepts[2][(axis + 1) % 3] = mins[(axis + 1) % 3];
	f->planepts[2][(axis + 2) % 3] = mins[(axis + 2) % 3];
	f->planepts[2][axis] = maxs[axis];
	f->planepts[1][(axis + 1) % 3] = maxs[(axis + 1) % 3];
	f->planepts[1][(axis + 2) % 3] = mins[(axis + 2) % 3];
	f->planepts[1][axis] = maxs[axis];
	f->planepts[0][(axis + 1) % 3] = maxs[(axis + 1) % 3];
	f->planepts[0][(axis + 2) % 3] = maxs[(axis + 2) % 3];
	f->planepts[0][axis] = maxs[axis];

	// create bottom face
	f = new Face(b);
	f->texdef = *texdef;

	f->planepts[0][(axis + 1) % 3] = mins[(axis + 1) % 3];
	f->planepts[0][(axis + 2) % 3] = mins[(axis + 2) % 3];
	f->planepts[0][axis] = mins[axis];
	f->planepts[1][(axis + 1) % 3] = maxs[(axis + 1) % 3];
	f->planepts[1][(axis + 2) % 3] = mins[(axis + 2) % 3];
	f->planepts[1][axis] = mins[axis];
	f->planepts[2][(axis + 1) % 3] = maxs[(axis + 1) % 3];
	f->planepts[2][(axis + 2) % 3] = maxs[(axis + 2) % 3];
	f->planepts[2][axis] = mins[axis];

	for (i = 0; i < sides; i++)
	{
		f = new Face(b);
		f->texdef = *texdef;

		sv = sin(i * Q_PI * 2 / sides);
		cv = cos(i * Q_PI * 2 / sides);

		f->planepts[0][(axis + 1) % 3] = floor(mid[(axis + 1) % 3] + width * cv + 0.5);
		f->planepts[0][(axis + 2) % 3] = floor(mid[(axis + 2) % 3] + width * sv + 0.5);
		f->planepts[0][axis] = mins[axis];
		f->planepts[1][(axis + 1) % 3] = f->planepts[0][(axis + 1) % 3];
		f->planepts[1][(axis + 2) % 3] = f->planepts[0][(axis + 2) % 3];
		f->planepts[1][axis] = maxs[axis];
		f->planepts[2][(axis + 1) % 3] = floor(f->planepts[0][(axis + 1) % 3] - width * sv + 0.5);
		f->planepts[2][(axis + 2) % 3] = floor(f->planepts[0][(axis + 2) % 3] + width * cv + 0.5);
		f->planepts[2][axis] = maxs[axis];
	}

	Select_SelectBrush(b);
	g_map.world->LinkBrush(b);
	b->Build();
}

// sikk---> Brush Primitives
/*
=============
Brush::MakeSidedCone

Makes the current brush have the given
number of 2D sides and turns it into a cone
=============
*/
void Brush::MakeSidedCone(int sides)
{
	int			i;
	float		width;
	float		sv, cv;
	vec3_t		mins, maxs;
	vec3_t		mid;
	Brush	   *b;
	Face	   *f;
	texdef_t   *texdef;

	if (sides < 3)
	{
		Sys_Printf("WARNING: Cone must have at least 3 sides.\n");
		return;
	}

	if (sides >= MAX_POINTS_ON_WINDING - 2)
	{
		Sys_Printf("WARNING: Couldn't create brush. Too many sides.\n");
		return;
	}

	if (!QE_SingleBrush())
	{
		Sys_Printf("WARNING: Must have a single brush selected.\n");
		return;
	}

	b = g_brSelectedBrushes.next;
	VectorCopy(b->mins, mins);
	VectorCopy(b->maxs, maxs);
	texdef = &g_qeglobals.d_workTexDef;

	delete b;

	// find center of brush
	width = 8;

	for (i = 0; i < 2; i++)
	{
		mid[i] = (maxs[i] + mins[i]) * 0.5;
		if (maxs[i] - mins[i] > width)
			width = maxs[i] - mins[i];
	}

	width /= 2;

	b = new Brush();

	// create bottom face
	f = new Face(b);
	f->texdef = *texdef;

	f->planepts[0][0] = mins[0];
	f->planepts[0][1] = mins[1];
	f->planepts[0][2] = mins[2];

	f->planepts[1][0] = maxs[0];
	f->planepts[1][1] = mins[1];
	f->planepts[1][2] = mins[2];

	f->planepts[2][0] = maxs[0];
	f->planepts[2][1] = maxs[1];
	f->planepts[2][2] = mins[2];

	for (i = 0; i < sides; i++)
	{
		f = new Face(b);
		f->texdef = *texdef;

		sv = sin(i * Q_PI * 2 / sides);
		cv = cos(i * Q_PI * 2 / sides);

		f->planepts[0][0] = floor(mid[0] + width * cv + 0.5);
		f->planepts[0][1] = floor(mid[1] + width * sv + 0.5);
		f->planepts[0][2] = mins[2];

		f->planepts[1][0] = mid[0];
		f->planepts[1][1] = mid[1];
		f->planepts[1][2] = maxs[2];

		f->planepts[2][0] = floor(f->planepts[0][0] - width * sv + 0.5);
		f->planepts[2][1] = floor(f->planepts[0][1] + width * cv + 0.5);
		f->planepts[2][2] = maxs[2];
	}

	Select_SelectBrush(b);
	g_map.world->LinkBrush(b);
	b->Build();
}

/*
=============
Brush::MakeSidedSphere

Makes the current brush have the given
number of 2d sides and turns it into a sphere
=============
*/
void Brush::MakeSidedSphere(int sides)
{
	int			i, j, k;
	double		radius;
	double		dt, dp;
	double		t, p;
	vec3_t		mins, maxs;
	vec3_t		mid;
	Brush	   *b;
	Face	   *f;
	texdef_t   *texdef;

	if (sides < 4)
	{
		Sys_Printf("WARNING: Sphere must have at least 4 sides.\n");
		return;
	}

	if (!QE_SingleBrush())
	{
		Sys_Printf("WARNING: Must have a single brush selected.\n");
		return;
	}

	b = g_brSelectedBrushes.next;
	VectorCopy(b->mins, mins);
	VectorCopy(b->maxs, maxs);
	texdef = &g_qeglobals.d_workTexDef;

	delete b;

	// find center of brush
	radius = 8;

	for (i = 0; i < 2; i++)
	{
		mid[i] = (maxs[i] + mins[i]) * 0.5;
		if (maxs[i] - mins[i] > radius)
			radius = maxs[i] - mins[i];
	}

	radius /= 2;

	b = new Brush();

	dt = (2 * Q_PI / sides);
	dp = (Q_PI / sides);

	for (i = 0; i <= sides - 1; i++)
	{
		for (j = 0; j <= sides - 2; j++)
		{
			t = i * dt;
			p = (j * dp - Q_PI / 2);

			f = new Face(b);
			f->texdef = *texdef;

			VectorPolar(f->planepts[0], radius, t, p);
			VectorPolar(f->planepts[1], radius, t, p + dp);
			VectorPolar(f->planepts[2], radius, t + dt, p + dp);

			for (k = 0; k < 3; k++)
				VectorAdd(f->planepts[k], mid, f->planepts[k]);
		}
	}

	p = ((sides - 1) * dp - Q_PI / 2);

	for (i = 0; i <= sides - 1; i++)
	{
		t = i * dt;

		f = new Face(b);
		f->texdef = *texdef;

		VectorPolar(f->planepts[0], radius, t, p);
		VectorPolar(f->planepts[1], radius, t + dt, p + dp);
		VectorPolar(f->planepts[2], radius, t + dt, p);

		for (k = 0; k < 3; k++)
			VectorAdd(f->planepts[k], mid, f->planepts[k]);
	}

	Select_SelectBrush(b);
	g_map.world->LinkBrush(b);
	b->Build();
}
// <---sikk



//=========================================================================

/*
=================
Brush::AddToList
=================
*/
void Brush::AddToList (Brush *list)
{
	if (next || prev)
		Error("Brush_AddToList: Already linked.");

	next = list->next;
	list->next->prev = this;
	list->next = this;
	prev = list;
}

/*
=================
Brush::RemoveFromList
=================
*/
void Brush::RemoveFromList()
{
	if (!next || !prev)
		Error("Brush_RemoveFromList: Not currently linked.");

	next->prev = prev;
	prev->next = next;
	next = prev = NULL;
}

void Brush::CloseLinks()
{
	if (next == prev && prev == this)
		return;	// done

	if (next || prev)
		Error("Entity: tried to close non-empty linked list.");

	next = prev = this;
}

/*
=================
Brush::MergeListIntoList
=================
*/
void Brush::MergeListIntoList(Brush *src, Brush *dest)
{
	// properly doubly-linked lists only
	if (!src->next || !src->prev)
	{
		Error("Tried to merge a list with NULL links!\n");
		return;
	}

	if (src->next == src || src->prev == src)
	{
		Sys_Printf("WARNING: Tried to merge an empty list.\n");
		return;
	}
	// merge at head of list
	src->next->prev = dest;
	src->prev->next = dest->next;
	dest->next->prev = src->prev;
	dest->next = src->next;

	/*
	// merge at tail of list
	dest->prev->next = src->next;
	src->next->prev = dest->prev;
	dest->prev = src->prev;
	src->prev->next = dest;
	*/

	src->prev = src->next = src;
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

	while (pBrush != NULL && pBrush != pList)
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








/*
==================
Brush::Draw
==================
*/
void Brush::Draw ()
{
	int			i, order;
	Face	   *face;
    Texture *prev = 0;
	winding_t  *w;

	if (hiddenBrush)
		return;

//	if (owner->eclass->IsFixedSize() && g_qeglobals.d_camera.draw_mode == cd_texture)
//		glDisable (GL_TEXTURE_2D);

	if (owner->eclass->IsFixedSize())
	{
		if (!(g_qeglobals.d_savedinfo.nExclude & EXCLUDE_ANGLES) && (owner->eclass->nShowFlags & ECLASS_ANGLE))
			DrawFacingAngle();

		if (g_qeglobals.d_savedinfo.bRadiantLights && (owner->eclass->nShowFlags & ECLASS_LIGHT))
		{
			DrawLight();
			return;
		}

		if (g_qeglobals.d_camera.draw_mode == cd_texture)
			glDisable(GL_TEXTURE_2D);
	}

	// guarantee the texture will be set first
	prev = NULL;
	for (face = brush_faces, order = 0; face; face = face->next, order++)
	{
		w = face->face_winding;
		if (!w)
			continue;	// freed face

		assert(face->d_texture);
		if (face->d_texture != prev && g_qeglobals.d_camera.draw_mode == cd_texture)
		{
			// set the texture for this face
			prev = face->d_texture;
			glBindTexture(GL_TEXTURE_2D, face->d_texture->texture_number);
		}

//		glColor3fv(face->d_color);
		glColor4f(face->d_color[0], face->d_color[1], face->d_color[2], 0.6f);	// lunaran TODO: pref for alpha

		// draw the polygon
		glBegin(GL_POLYGON);

	    for (i = 0; i < w->numpoints; i++)
		{
			if (g_qeglobals.d_camera.draw_mode == cd_texture)
				glTexCoord2fv(&w->points[i][3]);
			glVertex3fv(w->points[i]);
		}
		glEnd();
	}

	if (owner->eclass->IsFixedSize() && g_qeglobals.d_camera.draw_mode == cd_texture)
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
	float		fMid;
	vec3_t		vCorners[4];
	vec3_t		vTop, vBottom;
	Face	   *face;
	winding_t  *w;

	if (hiddenBrush)
		return;

	if (owner->eclass->IsFixedSize())
	{
		if (g_qeglobals.d_savedinfo.bRadiantLights && (owner->eclass->nShowFlags & ECLASS_LIGHT))
		{
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

			VectorCopy(vTop, vBottom);
			vBottom[2] = mins[2];
	    
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

			glBegin(GL_TRIANGLE_FAN);
			glVertex3fv(vTop);
			glVertex3fv(vCorners[0]);
			glVertex3fv(vCorners[1]);
			glVertex3fv(vCorners[2]);
			glVertex3fv(vCorners[3]);
			glVertex3fv(vCorners[0]);
			glEnd();

			glBegin(GL_TRIANGLE_FAN);
			glVertex3fv(vBottom);
			glVertex3fv(vCorners[0]);
			glVertex3fv(vCorners[3]);
			glVertex3fv(vCorners[2]);
			glVertex3fv(vCorners[1]);
			glVertex3fv(vCorners[0]);
			glEnd();

			DrawEntityName();
			return;
		}
	}

	for (face = brush_faces, order = 0; face; face = face->next, order++)
	{
		// only draw polygons facing in a direction we care about
		switch (nViewType)
		{
		case XY:
			if (face->plane.normal[2] <= 0)
				continue;
			break;
		case XZ:
			if (face->plane.normal[1] <= 0)
				continue;
			break;
		case YZ:
			if (face->plane.normal[0] <= 0)
				continue;
			break;
		}

		w = face->face_winding;
		if (!w)
			continue;

		// draw the polygon
		glBegin(GL_LINE_LOOP);
	    for (i = 0; i < w->numpoints; i++)
			glVertex3fv(w->points[i]);
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
	vec3_t	forward, right, up;
	vec3_t	endpoint, tip1, tip2;
	vec3_t	start;

	VectorAdd(owner->brushes.onext->mins, owner->brushes.onext->maxs, start);
	VectorScale(start, 0.5, start);
	dist = (maxs[0] - start[0]) * 2.5;

	FacingVectors(this, forward, right, up);
	VectorMA(start, dist, forward, endpoint);

	dist = (maxs[0] - start[0]) * 0.5;
	VectorMA(endpoint, -dist, forward, tip1);
	VectorMA(tip1, -dist, up, tip1);
	VectorMA(tip1, 2 * dist, up, tip2);

	glColor4f(1, 1, 1, 1);
	glLineWidth(4);
	glBegin(GL_LINES);
	glVertex3fv(start);
	glVertex3fv(endpoint);
	glVertex3fv(endpoint);
	glVertex3fv(tip1);
	glVertex3fv(endpoint);
	glVertex3fv(tip2);
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
	vec3_t	mid;

	if (!owner)
		return;	// during contruction

	if (owner == g_map.world)
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
		glVertex3fv(mid);
		mid[0] += c * 8;
		mid[1] += s * 8;
		glVertex3fv(mid);
		mid[0] -= c * 4;
		mid[1] -= s * 4;
		mid[0] -= s * 4;
		mid[1] += c * 4;
		glVertex3fv(mid);
		mid[0] += c * 4;
		mid[1] += s * 4;
		mid[0] += s * 4;
		mid[1] -= c * 4;
		glVertex3fv(mid);
		mid[0] -= c * 4;
		mid[1] -= s * 4;
		mid[0] += s * 4;
		mid[1] -= c * 4;
		glVertex3fv(mid);
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
	vec3_t	vTriColor;
	vec3_t	vCorners[4];
	vec3_t	vTop, vBottom;
	vec3_t	vSave;

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

	VectorCopy(vTop, vBottom);
	vBottom[2] = mins[2];

	VectorCopy(vTriColor, vSave);

	glBegin(GL_TRIANGLE_FAN);
	glVertex3fv(vTop);
	for (i = 0; i <= 3; i++)
	{
		vTriColor[0] *= 0.95f;
		vTriColor[1] *= 0.95f;
		vTriColor[2] *= 0.95f;
		glColor3f(vTriColor[0], vTriColor[1], vTriColor[2]);
		glVertex3fv(vCorners[i]);
	}
	glVertex3fv(vCorners[0]);
	glEnd();
  
	VectorCopy(vSave, vTriColor);
	vTriColor[0] *= 0.95f;
	vTriColor[1] *= 0.95f;
	vTriColor[2] *= 0.95f;

	glBegin(GL_TRIANGLE_FAN);
	glVertex3fv(vBottom);
	glVertex3fv(vCorners[0]);
	for (i = 3; i >= 0; i--)
	{
		vTriColor[0] *= 0.95f;
		vTriColor[1] *= 0.95f;
		vTriColor[2] *= 0.95f;
		glColor3f(vTriColor[0], vTriColor[1], vTriColor[2]);
		glVertex3fv(vCorners[i]);
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
	int			i, j;
	Brush    *b;
	Face	   *f;

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
		if (!b->brush_faces)
			b->brush_faces = f;
		else
		{
			Face *scan;

			for (scan = b->brush_faces; scan->next; scan = scan->next)
				;
			scan->next = f;
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
				f->planepts[i][j] = atof(g_szToken);	// sikk - changed to atof for BrushPrecision option
			}
			
			GetToken(false);
			if (strcmp(g_szToken, ")"))
				Error("Brush_Parse: Incorrect token.");
		}

		// read the texturedef
		GetToken(false);
		strcpy(f->texdef.name, g_szToken);
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
		
		StringTolower(f->texdef.name);

		// the wads probably aren't loaded yet, but this replaces the default null
		// reference with 'nulltexture' on all faces
		f->d_texture = Textures::ForName(f->texdef.name);
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
	for (fa = brush_faces; fa; fa = fa->next)
	{
		for (i = 0; i < 3; i++)
			if (g_qeglobals.d_savedinfo.bBrushPrecision)	// sikk - Brush Precision
				out << "( " << fa->planepts[i][0] << " " << fa->planepts[i][1] << " " << fa->planepts[i][2] << " ) ";
			else
				out << "( " << (int)fa->planepts[i][0] << " " << (int)fa->planepts[i][1] << " " << (int)fa->planepts[i][2] << " ) ";

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
/*
void Brush::Write (FILE *f)
{
	int		i;
	char   *pname;
	Face *fa;

	fprintf(f, "{\n");
	for (fa = brush_faces; fa; fa = fa->next)
	{
		for (i = 0; i < 3; i++)
			if (g_qeglobals.d_savedinfo.bBrushPrecision)	// sikk - Brush Precision
				fprintf(f, "( %f %f %f ) ", fa->planepts[i][0], fa->planepts[i][1], fa->planepts[i][2]);
			else
				fprintf(f, "( %d %d %d ) ", (int)fa->planepts[i][0], (int)fa->planepts[i][1], (int)fa->planepts[i][2]);

		pname = fa->texdef.name;
		if (pname[0] == 0)
			pname = "unnamed";

		CheckTexdef(fa, pname);	// sikk - Check Texdef - temp fix for Multiple Entity Undo Bug

		fprintf(f, "%s %d %d %d ", pname, (int)fa->texdef.shift[0], (int)fa->texdef.shift[1], (int)fa->texdef.rotate);

		if (fa->texdef.scale[0] == (int)fa->texdef.scale[0])
			fprintf(f, "%d ", (int)fa->texdef.scale[0]);
		else
			fprintf(f, "%f ", (float)fa->texdef.scale[0]);

		if (fa->texdef.scale[1] == (int)fa->texdef.scale[1])
			fprintf(f, "%d", (int)fa->texdef.scale[1]);
		else
			fprintf(f, "%f", (float)fa->texdef.scale[1]);

		fprintf(f, "\n");
	}
	fprintf(f, "}\n");
}
*/




//==========================================================================


/*
==================
FacingVectors
==================
*/
void FacingVectors(Brush *b, vec3_t forward, vec3_t right, vec3_t up)
{
	int		angleVal;
	vec3_t	angles;

	angleVal = b->owner->GetKeyValueInt("angle");

	if (angleVal == -1)
	{
		VectorSet(angles, 270, 0, 0);
	}
	else if (angleVal == -2)
	{
		VectorSet(angles, 90, 0, 0);
	}
	else
	{
		VectorSet(angles, 0, angleVal, 0);
	}

	AngleVectors(angles, forward, right, up);
}
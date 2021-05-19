//==============================
//	brush.c
//==============================

#include <assert.h>
#include "qe3.h"
#include "io.h"


/*
=============================================================================

			TEXTURE COORDINATES

=============================================================================
*/

vec3_t g_v3BaseAxis[18] =
{
	{ 0, 0, 1}, {1, 0, 0}, {0,-1, 0},	// floor
	{ 0, 0,-1}, {1, 0, 0}, {0,-1, 0},	// ceiling
	{ 1, 0, 0}, {0, 1, 0}, {0, 0,-1},	// west wall
	{-1, 0, 0}, {0, 1, 0}, {0, 0,-1},	// east wall
	{ 0, 1, 0}, {1, 0, 0}, {0, 0,-1},	// south wall
	{ 0,-1, 0}, {1, 0, 0}, {0, 0,-1}	// north wall
};

const float g_fLightAxis[3] = {0.6f, 0.8f, 1.0f};
//vec_t g_vLightAxis[3] = {(vec_t)0.6, (vec_t)0.8, (vec_t)1.0};
vec3_t  g_v3Vecs[2];
float	g_fShift[2];


/*
==================
TextureAxisFromPlane
==================
*/
void TextureAxisFromPlane (plane_t *pln, vec3_t xv, vec3_t yv)
{
	int		i, bestaxis;
	float	dot, best;
	
	best = 0;
	bestaxis = 0;
	
	for (i = 0; i < 6; i++)
	{
		dot = DotProduct(pln->normal, g_v3BaseAxis[i * 3]);
		if (dot > best)
		{
			best = dot;
			bestaxis = i;
		}
	}
	
	VectorCopy(g_v3BaseAxis[bestaxis * 3 + 1], xv);
	VectorCopy(g_v3BaseAxis[bestaxis * 3 + 2], yv);
}

/*
================
SetShadeForPlane

Light different planes differently to
improve recognition
================
*/
float SetShadeForPlane (plane_t *p)
{
	int		i;
	float	f;

	// axial plane
	for (i = 0; i < 3; i++)
	{
		if (fabs(p->normal[i]) > 0.9)
		{
			f = g_fLightAxis[i];
			return f;
		}
	}

	// between two axial planes
	for (i = 0; i < 3; i++)
	{
		if (fabs(p->normal[i]) < 0.1)
		{
			f = (g_fLightAxis[(i + 1) % 3] + g_fLightAxis[(i + 2) % 3]) / 2;
			return f;
		}
	}

	// other
	f = (g_fLightAxis[0] + g_fLightAxis[1] + g_fLightAxis[2]) / 3;
	return f;
}

/*
================
BeginTexturingFace
================
*/
void BeginTexturingFace (brush_t *b, face_t *f, qtexture_t *q)
{
	int		i, j;
	int		sv, tv;
	float	ang, sinv, cosv;
	float	ns, nt;
	float	shade;
	vec3_t	pvecs[2];

	// get natural texture axis
	TextureAxisFromPlane(&f->plane, pvecs[0], pvecs[1]);

	// set shading for face
	shade = SetShadeForPlane(&f->plane);
	if (g_qeglobals.d_camera.draw_mode == cd_texture && !b->owner->eclass->fixedsize)
		f->d_color[0] = f->d_color[1] = f->d_color[2] = shade;
	else
	{
		f->d_color[0] = shade * q->color[0];
		f->d_color[1] = shade * q->color[1];
		f->d_color[2] = shade * q->color[2];
	}

	if (g_qeglobals.d_camera.draw_mode != cd_texture)
		return;

	if (!f->texdef.scale[0])
		f->texdef.scale[0] = 1;
	if (!f->texdef.scale[1])
		f->texdef.scale[1] = 1;

	// rotate axis
	if (f->texdef.rotate == 0)
	{ 
		sinv = 0; 
		cosv = 1; 
	}
	else if (f->texdef.rotate == 90)
	{
		sinv = 1;
		cosv = 0;
	}
	else if (f->texdef.rotate == 180)
	{
		sinv = 0;
		cosv = -1;
	}
	else if (f->texdef.rotate == 270)
	{
		sinv = -1;
		cosv = 0;
	}
	else
	{	
		ang = f->texdef.rotate / 180 * Q_PI;
		sinv = sin(ang);
		cosv = cos(ang);
	}

	if (pvecs[0][0])
		sv = 0;
	else if (pvecs[0][1])
		sv = 1;
	else
		sv = 2;
				
	if (pvecs[1][0])
		tv = 0;
	else if (pvecs[1][1])
		tv = 1;
	else
		tv = 2;
					
	for (i = 0; i < 2; i++)
	{
		ns = cosv * pvecs[i][sv] - sinv * pvecs[i][tv];
		nt = sinv * pvecs[i][sv] + cosv * pvecs[i][tv];
		g_v3Vecs[i][sv] = ns;
		g_v3Vecs[i][tv] = nt;
	}

	for (i = 0; i < 2; i++)
		for (j = 0; j < 3; j++)
			g_v3Vecs[i][j] = g_v3Vecs[i][j] / f->texdef.scale[i];
}

/*
==================
_EmitTextureCoordinates
==================
*/
void _EmitTextureCoordinates (vec3_t v, qtexture_t *q)
{
	float	s, t;

	s = DotProduct(v, g_v3Vecs[0]);
	t = DotProduct(v, g_v3Vecs[1]);
	
	s += g_fShift[0];
	t += g_fShift[1];
	
	s /= q->width;
	t /= q->height;

	glTexCoord2f(s, t);
}

/*
==================
EmitTextureCoordinates
==================
*/
void EmitTextureCoordinates (float *xyzst, qtexture_t *q, face_t *f)
{
	float		s, t, ns, nt;
	float		ang, sinv, cosv;
	vec3_t		vecs[2];
	texdef_t   *td;

	// get natural texture axis
	TextureAxisFromPlane(&f->plane, vecs[0], vecs[1]);

	td = &f->texdef;

	ang = td->rotate / 180 * Q_PI;
	sinv = sin(ang);
	cosv = cos(ang);
	
	if (!td->scale[0])
		td->scale[0] = 1;
	if (!td->scale[1])
		td->scale[1] = 1;

	s = DotProduct(xyzst, vecs[0]);
	t = DotProduct(xyzst, vecs[1]);

	ns = cosv * s - sinv * t;
	nt = sinv * s + cosv * t;

	s = ns / td->scale[0] + td->shift[0];
	t = nt / td->scale[1] + td->shift[1];

	// gl scales everything from 0 to 1
	s /= q->width;
	t /= q->height;

	xyzst[3] = s;
	xyzst[4] = t;
}

//==========================================================================

/*
=================
Face_Alloc
=================
*/
face_t *Face_Alloc ()
{
	face_t *f = (face_t*)qmalloc(sizeof(*f));

	return f;
}

/*
================
Face_Clone
================
*/
face_t *Face_Clone (face_t *f)
{
	face_t	*n;

	n = Face_Alloc();
	n->texdef = f->texdef;
	memcpy(n->planepts, f->planepts, sizeof(n->planepts));

	// all other fields are derived, and will be set by Brush_Build
	return n;
}

// sikk---> Undo/Redo
/*
================
Face_FullClone

makes an exact copy of the face
================
*/
face_t *Face_FullClone (face_t *f)
{
	face_t	*n;

	n = Face_Alloc();
	n->texdef = f->texdef;
	memcpy(n->planepts, f->planepts, sizeof(n->planepts));
	memcpy(&n->plane, &f->plane, sizeof(plane_t));

	if (f->face_winding)
		n->face_winding = Winding_Clone(f->face_winding);
	else
		n->face_winding = NULL;

	n->d_texture = f->d_texture;

	return n;
}
// <---sikk


/*
=================
Face_Draw
=================
*/
void Face_Draw (face_t *f)
{
	int i;

	if (f->face_winding == 0)
		return;

	glBegin(GL_POLYGON);
	for (i = 0; i < f->face_winding->numpoints; i++)
		glVertex3fv(f->face_winding->points[i]);
	glEnd();
}

/*
=================
Face_FitTexture
=================
*/
void Face_FitTexture (face_t *face, int nHeight, int nWidth)
{
	int			i;
	vec_t		width, height, temp;
	vec_t		rot_width, rot_height;
	vec_t		cosv, sinv, ang;
	vec_t		min_t, min_s, max_t, max_s;
	vec_t		s, t;
	vec3_t		mins, maxs;
	vec3_t		vecs[2];
	vec3_t		coords[4];
	texdef_t   *td;
	winding_t  *w;
	
	if (nHeight < 1)
		nHeight = 1;
	if (nWidth < 1)
		nWidth = 1;
	
	ClearBounds (mins, maxs);
	
	w = face->face_winding;
	if (!w)
		return;

	for (i = 0; i < w->numpoints; i++)
		AddPointToBounds(w->points[i], mins, maxs);
	
	td = &face->texdef;
 
	// get the current angle
	ang = td->rotate / 180 * Q_PI;
	sinv = sin(ang);
	cosv = cos(ang);
	
	// get natural texture axis
	TextureAxisFromPlane(&face->plane, vecs[0], vecs[1]);
	
	min_s = DotProduct(mins, vecs[0]);
	min_t = DotProduct(mins, vecs[1]);
	max_s = DotProduct(maxs, vecs[0]);
	max_t = DotProduct(maxs, vecs[1]);
	width = max_s - min_s;
	height = max_t - min_t;
	coords[0][0] = min_s;
	coords[0][1] = min_t;
	coords[1][0] = max_s;
	coords[1][1] = min_t;
	coords[2][0] = min_s;
	coords[2][1] = max_t;
	coords[3][0] = max_s;
	coords[3][1] = max_t;
	min_s = min_t = 99999;
	max_s = max_t = -99999;

	for (i = 0; i < 4; i++)
	{
		s = cosv * coords[i][0] - sinv * coords[i][1];
		t = sinv * coords[i][0] + cosv * coords[i][1];

		if (i & 1)
		{
			if (s > max_s) 
				max_s = s;
		}
		else
		{
			if (s < min_s) 
				min_s = s;
			if (i < 2)
			{
				if (t < min_t) 
					min_t = t;
			}
			else
			{
				if (t > max_t) 
					max_t = t;
			}
		}
	}

	rot_width = (max_s - min_s);
	rot_height = (max_t - min_t);

	td->scale[0] = -(rot_width / ((float)(face->d_texture->width * nWidth)));
	td->scale[1] = -(rot_height / ((float)(face->d_texture->height * nHeight)));
	td->shift[0] = min_s / td->scale[0];

	temp = (int)(td->shift[0] / (face->d_texture->width * nWidth));
	temp = (temp + 1) * face->d_texture->width * nWidth;

	td->shift[0] = (int)(temp - td->shift[0]) % (face->d_texture->width * nWidth);
	td->shift[1] = min_t / td->scale[1];

	temp = (int)(td->shift[1] / (face->d_texture->height * nHeight));
	temp = (temp + 1) * (face->d_texture->height * nHeight);

	td->shift[1] = (int)(temp - td->shift[1]) % (face->d_texture->height * nHeight);
}

/*
=================
Face_Free
=================
*/
void Face_Free (face_t *f)
{
	assert(f != 0);

	if (f->face_winding)
		free(f->face_winding), f->face_winding = 0;

	free(f);
}

// sikk---> Vertex Editing Splits Face
/*
================
Face_MakePlane
================
*/
void Face_MakePlane (face_t *f)
{
	int		j;
	vec3_t	t1, t2, t3;

	// convert to a vector / dist plane
	for (j = 0; j < 3; j++)
	{
		t1[j] = f->planepts[0][j] - f->planepts[1][j];
		t2[j] = f->planepts[2][j] - f->planepts[1][j];
		t3[j] = f->planepts[1][j];
	}
	
	CrossProduct(t1, t2, f->plane.normal);

	if (VectorCompare(f->plane.normal, g_v3VecOrigin))
		Sys_Printf("WARNING: Brush plane with no normal.\n");

	VectorNormalize(f->plane.normal);
	f->plane.dist = DotProduct(t3, f->plane.normal);
}
// <---sikk

/*
================
Face_MoveTexture
================
*/
void Face_MoveTexture (face_t *f, vec3_t move)
{
	vec3_t		pvecs[2];
	vec_t		s, t, ns, nt;
	vec_t		ang, sinv, cosv;
	texdef_t   *td;

	TextureAxisFromPlane(&f->plane, pvecs[0], pvecs[1]);
	td = &f->texdef;
	ang = td->rotate / 180 * Q_PI;
	sinv = sin(ang);
	cosv = cos(ang);

	if (!td->scale[0])
		td->scale[0] = 1;
	if (!td->scale[1])
		td->scale[1] = 1;

	s = DotProduct(move, pvecs[0]);
	t = DotProduct(move, pvecs[1]);
	ns = cosv * s - sinv * t;
	nt = sinv * s + cosv * t;
	s = ns / td->scale[0];
	t = nt / td->scale[1];

	f->texdef.shift[0] -= s;
	f->texdef.shift[1] -= t;

	while(f->texdef.shift[0] > f->d_texture->width)
		f->texdef.shift[0] -= f->d_texture->width;
	while(f->texdef.shift[1] > f->d_texture->height)
		f->texdef.shift[1] -= f->d_texture->height;
	while(f->texdef.shift[0] < 0)
		f->texdef.shift[0] += f->d_texture->width;
	while(f->texdef.shift[1] < 0)
		f->texdef.shift[1] += f->d_texture->height;
}

/*
================
Face_SetColor
================
*/
void Face_SetColor (brush_t *b, face_t *f) 
{
	float shade;
	qtexture_t *q;

	q = f->d_texture;

	// set shading for face
	shade = SetShadeForPlane(&f->plane);

	if (g_qeglobals.d_camera.draw_mode == cd_texture && !b->owner->eclass->fixedsize)
	{
		f->d_color[0] = f->d_color[1] = f->d_color[2] = shade;
	}
	else
	{
		f->d_color[0] = shade * q->color[0];
		f->d_color[1] = shade * q->color[1];
		f->d_color[2] = shade * q->color[2];
	}
}

/*
=================
Face_SetTexture
=================
*/
void Face_SetTexture (face_t *f, texdef_t *texdef)
{
	f->texdef = *texdef;
	Brush_Build(f->owner);
}

/*
==================
FacingVectors
==================
*/
void FacingVectors (brush_t *b, vec3_t forward, vec3_t right, vec3_t up)
{
	int		angleVal;
	vec3_t	angles;

	angleVal = IntForKey(b->owner, "angle");

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

//=========================================================================

/*
==================
Brush_Alloc
==================
*/
brush_t *Brush_Alloc ()
{
	brush_t *b = (brush_t*)qmalloc(sizeof(brush_t));

	return b;
}

/*
=================
Brush_AddToList
=================
*/
void Brush_AddToList (brush_t *b, brush_t *list)
{
	if (b->next || b->prev)
		Error("Brush_AddToList: Already linked.");

	b->next = list->next;
	list->next->prev = b;
	list->next = b;
	b->prev = list;
}

/*
=================
Brush_MergeListIntoList
=================
*/
void Brush_MergeListIntoList(brush_t *src, brush_t *dest)
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
	// merge at head of list - preferred for shorter search time to recently manipulated brushes
	src->next->prev = dest;
	src->prev->next = dest->next;
	dest->next->prev = src->prev;
	dest->next = src->next;

	/*
	// for merge at tail of list
	dest->prev->next = src->next;
	src->next->prev = dest->prev;
	dest->prev = src->prev;
	src->prev->next = dest;
	*/

	src->prev = src->next = src;
}



/*
==================
Brush_Build

Builds a brush rendering data and also sets the min/max bounds
==================
*/
void Brush_Build (brush_t *b)
{
	char title[1024];

	if (g_bModified != 1)
	{
		g_bModified = true;	// mark the map as changed
		sprintf(title, "%s *", g_szCurrentMap);
		QE_ConvertDOSToUnixName(title, title);
		Sys_SetTitle(title);
	}

	// build the windings and generate the bounding box
	Brush_BuildWindings(b);

	// move the points and edges if in select mode
	if (g_qeglobals.d_selSelectMode == sel_vertex || g_qeglobals.d_selSelectMode == sel_edge)
		SetupVertexSelection();
}

/*
=================
Brush_BuildWindings
=================
*/
void Brush_BuildWindings (brush_t *b)
{
	vec_t      v;
	face_t    *face;
	winding_t *w;

	Brush_SnapPlanepts(b);

	// clear the mins/maxs bounds
	b->mins[0] = b->mins[1] = b->mins[2] = 99999;
	b->maxs[0] = b->maxs[1] = b->maxs[2] = -99999;

	Brush_MakeFacePlanes(b);

	face = b->brush_faces;

	for ( ; face; face = face->next)
	{
		int	i, j;

		w = face->face_winding = Brush_MakeFaceWinding(b, face);
		face->d_texture = Texture_ForName(face->texdef.name);

		if (!w)
			continue;
	
	    for (i = 0; i < w->numpoints; i++)
	    {
			// add to bounding box
			for (j = 0; j < 3; j++)
			{
				v = w->points[i][j];
				if (v > b->maxs[j])
					b->maxs[j] = v;
				if (v < b->mins[j])
					b->mins[j] = v;
			}
	    }
		// setup s and t vectors, and set color
		BeginTexturingFace(b, face, face->d_texture);

	    for (i = 0; i < w->numpoints; i++)
			EmitTextureCoordinates(w->points[i], face->d_texture, face);
	}
}

// sikk---> Check Texdef - temp fix for Multiple Entity Undo Bug
/*
=================
Brush_CheckTexdef

temporary fix for bug when undoing 
manipulation of multiple entity types. 
detect potential problems when saving 
the texture name and apply default tex
=================
*/
void Brush_CheckTexdef (brush_t *b, face_t *f, char *pszName)
{
/*	if (!strlen(f->texdef.name))
	{
#ifdef _DEBUG
		Sys_Printf("WARNING: Unexpected texdef.name is empty in Brush.cpp Brush_CheckTexdef()\n");
#endif
//		fa->texdef.SetName("unnamed");
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
		brush_t	   *b2;

		 // sikk - This seems to work but it needs more testing
		b2 = b->owner->brushes.onext;

		// this is to catch if the bad brush is brush #0
		if (b2 == b || b2 == &b->owner->brushes)
			bFound = true;

		// find brush
		while (!bFound)
		{
			b2 = b2->onext;
			if (b2 == b || b2 == &b->owner->brushes)
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

//	strcpy(pszName, f->texdef.name);
}
// <---sikk

/*
============
Brush_Clone

Does NOT add the new brush to any lists
============
*/
brush_t *Brush_Clone (brush_t *b)
{
	brush_t	*n;
	face_t	*f, *nf;

	n = Brush_Alloc();
	n->owner = b->owner;

	for (f = b->brush_faces; f; f = f->next)
	{
		nf = Face_Clone(f);
		nf->next = n->brush_faces;
		n->brush_faces = nf;
	}

	return n;
}

// sikk---> Undo/Redo
/*
============
Brush_FullClone

Does NOT add the new brush to any lists
============
*/
brush_t *Brush_FullClone (brush_t *b)
{
	brush_t	   *n = NULL;
	face_t	   *f, *nf, *f2, *nf2;
	int			j;

  	n = Brush_Alloc();
	n->owner = b->owner;
	VectorCopy(b->mins, n->mins);
	VectorCopy(b->maxs, n->maxs);

	for (f = b->brush_faces; f; f = f->next)
	{
		if (f->original) 
			continue;

		nf = Face_FullClone(f);
		nf->next = n->brush_faces;
		n->brush_faces = nf;
	
		// copy all faces that have the original set to this face
		for (f2 = b->brush_faces; f2; f2 = f2->next)
		{
			if (f2->original == f)
			{
				nf2 = Face_FullClone(f2);
				nf2->next = n->brush_faces;
				n->brush_faces = nf2;
				// set original
				nf2->original = nf;
			}
		}
	}

	for (nf = n->brush_faces; nf; nf = nf->next)
	{
		Face_SetColor(n, nf);
		if (nf->face_winding)
			for (j = 0; j < nf->face_winding->numpoints; j++)
				EmitTextureCoordinates(nf->face_winding->points[j], nf->d_texture, nf);
	}

	return n;
}
// <---sikk

/*
==================
Brush_CleanList
==================
*/
void Brush_CleanList (brush_t *pList)
{
	brush_t *pBrush;
	brush_t *pNext;

	pBrush = pList->next; 

	while (pBrush != NULL && pBrush != pList)
	{
		pNext = pBrush->next;
		Brush_Free(pBrush);
		pBrush = pNext;
	}

}

/*
==================
Brush_CopyList
==================
*/
void Brush_CopyList (brush_t *pFrom, brush_t *pTo)
{
	brush_t	*pBrush;
	brush_t *pNext;

	pBrush = pFrom->next;

	while (pBrush != NULL && pBrush != pFrom)
	{
		pNext = pBrush->next;
		Brush_RemoveFromList(pBrush);
		Brush_AddToList(pBrush, pTo);
		pBrush = pNext;
	}
}

/*
=============
Brush_Create

Create non-textured blocks for entities
The brush is NOT linked to any list
=============
*/
brush_t	*Brush_Create (vec3_t mins, vec3_t maxs, texdef_t *texdef)
{
	int			i, j;
	vec3_t		pts[4][2];
	brush_t	   *b;
	face_t	   *f;

	for (i = 0; i < 3; i++)
		if (maxs[i] < mins[i])
			Error("Brush_InitSolid: Backwards.");

	b = Brush_Alloc();
	
	pts[0][0][0] = mins[0];
	pts[0][0][1] = mins[1];
	
	pts[1][0][0] = mins[0];
	pts[1][0][1] = maxs[1];
	
	pts[2][0][0] = maxs[0];
	pts[2][0][1] = maxs[1];
	
	pts[3][0][0] = maxs[0];
	pts[3][0][1] = mins[1];
	
	for (i = 0; i < 4; i++)
	{
		pts[i][0][2] = mins[2];
		pts[i][1][0] = pts[i][0][0];
		pts[i][1][1] = pts[i][0][1];
		pts[i][1][2] = maxs[2];
	}
	
	for (i = 0; i < 4; i++)
	{
		f = Face_Alloc();
		f->texdef = *texdef;
		f->next = b->brush_faces;
		b->brush_faces = f;
		j = (i + 1) % 4;

		VectorCopy(pts[j][1], f->planepts[0]);
		VectorCopy(pts[i][1], f->planepts[1]);
		VectorCopy(pts[i][0], f->planepts[2]);
	}
	
	f = Face_Alloc();
	f->texdef = *texdef;
	f->next = b->brush_faces;
	b->brush_faces = f;

	VectorCopy(pts[0][1], f->planepts[0]);
	VectorCopy(pts[1][1], f->planepts[1]);
	VectorCopy(pts[2][1], f->planepts[2]);

	f = Face_Alloc();
	f->texdef = *texdef;
	f->next = b->brush_faces;
	b->brush_faces = f;

	VectorCopy(pts[2][0], f->planepts[0]);
	VectorCopy(pts[1][0], f->planepts[1]);
	VectorCopy(pts[0][0], f->planepts[2]);

	return b;
}

/*
==================
Brush_DrawFacingAngle
==================
*/
void Brush_DrawFacingAngle (brush_t *b)
{
	float	dist;
	vec3_t	forward, right, up;
	vec3_t	endpoint, tip1, tip2;
	vec3_t	start;

	VectorAdd(b->owner->brushes.onext->mins, b->owner->brushes.onext->maxs, start);
	VectorScale(start, 0.5, start);
	dist = (b->maxs[0] - start[0]) * 2.5;

	FacingVectors(b, forward, right, up);
	VectorMA(start, dist, forward, endpoint);

	dist = (b->maxs[0] - start[0]) * 0.5;
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
Brush_Draw
==================
*/
void Brush_Draw (brush_t *b)
{
	int			i, order;
	face_t	   *face;
    qtexture_t *prev = 0;
	winding_t  *w;

	if (b->hiddenBrush)
		return;

//	if (b->owner->eclass->fixedsize && g_qeglobals.d_camera.draw_mode == cd_texture)
//		glDisable (GL_TEXTURE_2D);

	if (b->owner->eclass->fixedsize)
	{
		if (!(g_qeglobals.d_savedinfo.nExclude & EXCLUDE_ANGLES) && (b->owner->eclass->nShowFlags & ECLASS_ANGLE))
			Brush_DrawFacingAngle(b);

		if (g_qeglobals.d_savedinfo.bRadiantLights && (b->owner->eclass->nShowFlags & ECLASS_LIGHT))
		{
			DrawLight(b);
			return;
		}

		if (g_qeglobals.d_camera.draw_mode == cd_texture)
			glDisable(GL_TEXTURE_2D);
	}

	// guarantee the texture will be set first
	prev = NULL;
	for (face = b->brush_faces, order = 0; face; face = face->next, order++)
	{
		w = face->face_winding;
		if (!w)
			continue;	// freed face

		if (face->d_texture != prev && g_qeglobals.d_camera.draw_mode == cd_texture)
		{
			// set the texture for this face
			prev = face->d_texture;
			glBindTexture(GL_TEXTURE_2D, face->d_texture->texture_number);
		}

//		glColor3fv(face->d_color);
		glColor4f(face->d_color[0], face->d_color[1], face->d_color[2], 0.13f);

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

	if (b->owner->eclass->fixedsize && g_qeglobals.d_camera.draw_mode == cd_texture)
		glEnable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, 0);
}

/*
=================
Brush_DrawXY
=================
*/
void Brush_DrawXY (brush_t *b, int nViewType)
{
	int			i;
	int			order;
	float		fMid;
	vec3_t		vCorners[4];
	vec3_t		vTop, vBottom;
	face_t	   *face;
	winding_t  *w;

	if (b->hiddenBrush)
		return;

	if (b->owner->eclass->fixedsize)
	{
		if (g_qeglobals.d_savedinfo.bRadiantLights && (b->owner->eclass->nShowFlags & ECLASS_LIGHT))
		{
    		fMid = b->mins[2] + (b->maxs[2] - b->mins[2]) / 2;

			vCorners[0][0] = b->mins[0];
			vCorners[0][1] = b->mins[1];
			vCorners[0][2] = fMid;

			vCorners[1][0] = b->mins[0];
			vCorners[1][1] = b->maxs[1];
			vCorners[1][2] = fMid;

			vCorners[2][0] = b->maxs[0];
			vCorners[2][1] = b->maxs[1];
			vCorners[2][2] = fMid;

			vCorners[3][0] = b->maxs[0];
			vCorners[3][1] = b->mins[1];
			vCorners[3][2] = fMid;

			vTop[0] = b->mins[0] + ((b->maxs[0] - b->mins[0]) / 2);
			vTop[1] = b->mins[1] + ((b->maxs[1] - b->mins[1]) / 2);
			vTop[2] = b->maxs[2];

			VectorCopy(vTop, vBottom);
			vBottom[2] = b->mins[2];
	    
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

			DrawBrushEntityName(b);
			return;
		}
	}

	for (face = b->brush_faces, order = 0; face; face = face->next, order++)
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
		DrawBrushEntityName(b);
}

/*
=================
Brush_FitTexture
=================
*/
void Brush_FitTexture (brush_t *b, int nHeight, int nWidth)
{
	face_t *face;
	
	for (face = b->brush_faces; face; face = face->next)
		Face_FitTexture(face, nHeight, nWidth);
}

/*
=============
Brush_Free

Frees the brush with all of its faces and display list.
Unlinks the brush from whichever chain it is in.
Decrements the owner entity's brushcount.
Removes owner entity if this was the last brush
unless owner is the world.
=============
*/
void Brush_Free (brush_t *b)
{
	face_t *f, *next;

	// free faces
	for (f = b->brush_faces; f; f = next)
	{
		next = f->next;
		Face_Free(f);
	}

	// unlink from active/selected list
	if (b->next)
		Brush_RemoveFromList(b);

	// unlink from entity list
	if (b->onext)
		Entity_UnlinkBrush(b);

	free(b);
}

// sikk---> Undo/Redo
/*
=============
Face_MemorySize
=============
*/
int Face_MemorySize (face_t *f)
{
	int size = 0;

	if (f->face_winding)
		size += _msize(f->face_winding);

//	f->texdef.~texdef_t();;
	size += _msize(f);

	return size;
}

/*
=============
Brush_MemorySize
=============
*/
int Brush_MemorySize (brush_t *b)
{
	int		size = 0;
	face_t *f;

	for (f = b->brush_faces; f; f = f->next)
		size += Face_MemorySize(f);

	size += _msize(b);
	
	return size;
}
// <---sikk

/*
==================
Brush_MakeFacePlanes
==================
*/
void Brush_MakeFacePlanes (brush_t *b)
{
	int		i;
	face_t *f;
	vec3_t	t1, t2, t3;

	for (f = b->brush_faces; f; f = f->next)
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
=================
Brush_MakeFaceWinding

returns the visible polygon on a face
=================
*/
winding_t *Brush_MakeFaceWinding (brush_t *b, face_t *face)
{
	bool		past;
	face_t	   *clip;
	plane_t		plane;
	winding_t  *w;

	// get a poly that covers an effectively infinite area
	w = BasePolyForPlane(&face->plane);

	// chop the poly by all of the other faces
	past = false;
	for (clip = b->brush_faces; clip && w; clip = clip->next)
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
				free(w);
				return NULL;
			}
			continue;
		}

		// flip the plane, because we want to keep the back side
		VectorSubtract(g_v3VecOrigin, clip->plane.normal, plane.normal);
		plane.dist = -clip->plane.dist;
		
		w = Winding_Clip(w, &plane, false);
		if (!w)
			return w;
	}
	
	if (w->numpoints < 3)
	{
		free(w);
		w = NULL;
	}

	if (!w)
		printf("unused plane\n");

	return w;
}

/*
=============
Brush_MakeSided

Makes the current brush have the given number of 2d sides
=============
*/
void Brush_MakeSided (int sides)
{
	int			i, axis;
	float		width;
	float		sv, cv;
	vec3_t		mins, maxs;
	vec3_t		mid;
	brush_t	   *b;
	face_t	   *f;
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
	texdef = &g_qeglobals.d_texturewin.texdef;

	Brush_Free(b);

// sikk---> Multiple Orthographic Views
	if (GetTopWindow(g_qeglobals.d_hwndMain) == g_qeglobals.d_hwndXZ)
		nViewType = XZ;
	else if (GetTopWindow(g_qeglobals.d_hwndMain) == g_qeglobals.d_hwndYZ)
		nViewType = YZ;
	else
		nViewType = g_qeglobals.d_nViewType;
// <---sikk

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

	b = Brush_Alloc();
		
	// create top face
	f = Face_Alloc();
	f->texdef = *texdef;
	f->next = b->brush_faces;
	b->brush_faces = f;

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
	f = Face_Alloc();
	f->texdef = *texdef;
	f->next = b->brush_faces;
	b->brush_faces = f;

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
		f = Face_Alloc();
		f->texdef = *texdef;
		f->next = b->brush_faces;
		b->brush_faces = f;

		sv = sin(i * Q_PI * 2 / sides);
		cv = cos(i * Q_PI * 2 / sides);

		f->planepts[0][(axis + 1) % 3] = floor(mid[(axis + 1) % 3] + width * cv +0.5);
		f->planepts[0][(axis + 2) % 3] = floor(mid[(axis + 2) % 3] + width * sv +0.5);
		f->planepts[0][axis] = mins[axis];
		f->planepts[1][(axis + 1) % 3] = f->planepts[0][(axis + 1) % 3];
		f->planepts[1][(axis + 2) % 3] = f->planepts[0][(axis + 2) % 3];
		f->planepts[1][axis] = maxs[axis];
		f->planepts[2][(axis + 1) % 3] = floor(f->planepts[0][(axis + 1) % 3] - width * sv + 0.5);
		f->planepts[2][(axis + 2) % 3] = floor(f->planepts[0][(axis + 2) % 3] + width * cv + 0.5);
		f->planepts[2][axis] = maxs[axis];
	}

	Select_SelectBrush(b);
	Entity_LinkBrush(g_peWorldEntity, b);
	Brush_Build(b);
}

// sikk---> Brush Primitives
/*
=============
Brush_MakeSidedCone

Makes the current brush have the given 
number of 2D sides and turns it into a cone
=============
*/
void Brush_MakeSidedCone (int sides)
{
	int			i;
	float		width;
	float		sv, cv;
	vec3_t		mins, maxs;
	vec3_t		mid;
	brush_t	   *b;
	face_t	   *f;
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
	texdef = &g_qeglobals.d_texturewin.texdef;

	Brush_Free(b);

	// find center of brush
	width = 8;

	for (i = 0; i < 2; i++)
	{
		mid[i] = (maxs[i] + mins[i]) * 0.5;
		if (maxs[i] - mins[i] > width)
			width = maxs[i] - mins[i];
	}

	width /= 2;

	b = Brush_Alloc();

	// create bottom face
	f = Face_Alloc();
	f->texdef = *texdef;
	f->next = b->brush_faces;
	b->brush_faces = f;

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
		f = Face_Alloc();
		f->texdef = *texdef;
		f->next = b->brush_faces;
		b->brush_faces = f;

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
	Entity_LinkBrush(g_peWorldEntity, b);
	Brush_Build(b);
}

/*
=============
Brush_MakeSidedSphere

Makes the current brush have the given 
number of 2d sides and turns it into a sphere
=============
*/
void Brush_MakeSidedSphere (int sides)
{
	int			i, j, k;
	double		radius;
	double		dt, dp;
	double		t, p;
	vec3_t		mins, maxs;
	vec3_t		mid;
	brush_t	   *b;
	face_t	   *f;
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
	texdef = &g_qeglobals.d_texturewin.texdef;

	Brush_Free(b);

	// find center of brush
	radius = 8;

	for (i = 0; i < 2; i++)
	{
		mid[i] = (maxs[i] + mins[i]) * 0.5;
		if (maxs[i] - mins[i] > radius)
			radius = maxs[i] - mins[i];
	}

	radius /= 2;

	b = Brush_Alloc();

	dt = (2 * Q_PI / sides);
	dp = (Q_PI / sides);

	for (i = 0; i <= sides - 1; i++)
	{
		for (j = 0; j <= sides - 2; j++)
		{
			t = i * dt;
			p = (j * dp - Q_PI / 2);

			f = Face_Alloc();
			f->texdef = *texdef;
			f->next = b->brush_faces;
			b->brush_faces = f;

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

		f = Face_Alloc();
		f->texdef = *texdef;
		f->next = b->brush_faces;
		b->brush_faces = f;

		VectorPolar(f->planepts[0], radius, t, p);
		VectorPolar(f->planepts[1], radius, t + dt, p + dp);
		VectorPolar(f->planepts[2], radius, t + dt, p);

		for (k = 0; k < 3; k++)
			VectorAdd(f->planepts[k], mid, f->planepts[k]);
	}

	Select_SelectBrush(b);
	Entity_LinkBrush(g_peWorldEntity, b);
	Brush_Build(b);
}
// <---sikk

/*
============
Brush_Move
============
*/
void Brush_Move (brush_t *b, vec3_t move)
{
	int		i;
	face_t *f;

	for (f = b->brush_faces; f; f = f->next)
	{
		if (g_qeglobals.d_bTextureLock)
			Face_MoveTexture(f, move);

		for (i = 0; i < 3; i++)
			VectorAdd(f->planepts[i], move, f->planepts[i]);
	}

	Brush_Build(b);

	// PGM - keep the origin vector up to date on fixed size entities.
	if (b->owner->eclass->fixedsize)
		VectorAdd(b->owner->origin, move, b->owner->origin);
	// lunaran TODO: update the keyvalue dammit
}

/*
=================
Brush_Parse

The brush is NOT linked to any list
=================
*/
brush_t *Brush_Parse ()
{
	int			i, j;
	brush_t    *b;
	face_t	   *f;

	g_qeglobals.d_nParsedBrushes++;
	b = Brush_Alloc ();//qmalloc(sizeof(brush_t));
		
	do
	{
		if (!GetToken(true))
			break;
		if (!strcmp(g_szToken, "}"))
			break;
		
		f = Face_Alloc();

		// add the brush to the end of the chain, so
		// loading and saving a map doesn't reverse the order
		f->next = NULL;
		if (!b->brush_faces)
			b->brush_faces = f;
		else
		{
			face_t *scan;

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

		// the flags and value field aren't necessarily present
		f->d_texture = Texture_ForName(f->texdef.name);

	} while (1);

	return b;
}

/*
==============
Brush_Ray

Itersects a ray with a brush
Returns the face hit and the distance along the ray the intersection occured at
Returns NULL and 0 if not hit at all
==============
*/
face_t *Brush_Ray (vec3_t origin, vec3_t dir, brush_t *b, float *dist)
{
	int		i;
	float	frac, d1, d2;
	vec3_t	p1, p2;
	face_t *f, *firstface;

	VectorCopy(origin, p1);

	for (i = 0; i < 3; i++)
		p2[i] = p1[i] + dir[i] * 16384;

	for (f = b->brush_faces; f; f = f->next)
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
==================
Brush_RemoveEmptyFaces

Frees any overconstraining faces
==================
*/
void Brush_RemoveEmptyFaces (brush_t *b)
{
	face_t	*f, *next;

	f = b->brush_faces;
	b->brush_faces = NULL;

	for ( ; f; f = next)
	{
		next = f->next;

		if (!f->face_winding)
			Face_Free(f);
		else
		{
			f->next = b->brush_faces;
			b->brush_faces = f;
		}
	}
}

/*
=================
Brush_RemoveFromList
=================
*/
void Brush_RemoveFromList (brush_t *b)
{
	if (!b->next || !b->prev)
		Error("Brush_RemoveFromList: Not currently linked.");

	b->next->prev = b->prev;
	b->prev->next = b->next;
	b->next = b->prev = NULL;
}

/*
==============
Brush_SelectFaceForDragging

Adds the faces planepts to move_points, and
rotates and adds the planepts of adjacent face if shear is set
==============
*/
void Brush_SelectFaceForDragging (brush_t *b, face_t *f, bool shear)
{
	int			i;
	int			c;
	float		d;
	brush_t	   *b2;
	face_t	   *f2;
	winding_t  *w;

	if (b->owner->eclass->fixedsize)
		return;

	c = 0;
	for (i = 0; i < 3; i++)
		c += AddPlanept(f->planepts[i]);
	if (c == 0)
		return;		// already completely added

	// select all points on this plane in all brushes the selection
	for (b2 = g_brSelectedBrushes.next; b2 != &g_brSelectedBrushes; b2 = b2->next)
	{
		if (b2 == b)
			continue;
		for (f2 = b2->brush_faces; f2; f2 = f2->next)
		{
			for (i = 0; i < 3; i++)
				if (fabs(DotProduct(f2->planepts[i], f->plane.normal) - f->plane.dist) > ON_EPSILON)
					break;
			if (i == 3)
			{	// move this face as well
				Brush_SelectFaceForDragging(b2, f2, shear);
				break;
			}
		}
	}

	// if shearing, take all the planes adjacent to 
	// selected faces and rotate their points so the
	// edge clipped by a selcted face has two of the points
	if (!shear)
		return;

	for (f2 = b->brush_faces; f2; f2 = f2->next)
	{
		if (f2 == f)
			continue;
		w = Brush_MakeFaceWinding(b, f2);
		if (!w)
			continue;

		// any points on f will become new control points
		for (i = 0; i < w->numpoints; i++)
		{
			d = DotProduct(w->points[i], f->plane.normal) - f->plane.dist;
			if (d > -ON_EPSILON && d < ON_EPSILON)
				break;
		}

		// if none of the points were on the plane,
		// leave it alone
		if (i != w->numpoints)
		{
			if (i == 0)
			{	// see if the first clockwise point was the
				// last point on the winding
				d = DotProduct(w->points[w->numpoints - 1], f->plane.normal) - f->plane.dist;
				if (d > -ON_EPSILON && d < ON_EPSILON)
					i = w->numpoints - 1;
			}

			AddPlanept(f2->planepts[0]);

			VectorCopy(w->points[i], f2->planepts[0]);
			if (++i == w->numpoints)
				i = 0;
			
			// see if the next point is also on the plane
			d = DotProduct(w->points[i], f->plane.normal) - f->plane.dist;
			if (d > -ON_EPSILON && d < ON_EPSILON)
				AddPlanept(f2->planepts[1]);

			VectorCopy(w->points[i], f2->planepts[1]);
			if (++i == w->numpoints)
				i = 0;

			// the third point is never on the plane
			VectorCopy(w->points[i], f2->planepts[2]);
		}

		free(w);
	}
}

/*
=================
Brush_SetTexture
=================
*/
void Brush_SetTexture (brush_t *b, texdef_t *texdef)
{
	face_t *f;

	for (f = b->brush_faces; f; f = f->next)
		f->texdef = *texdef;

	Brush_Build(b);
}

/*
==============
Brush_SideSelect

The mouse click did not hit the brush, so grab one or more side
planes for dragging
==============
*/
void Brush_SideSelect (brush_t *b, vec3_t origin, vec3_t dir, bool shear)
{
	face_t	*f, *f2;
	vec3_t	 p1, p2;

	for (f = b->brush_faces; f; f = f->next)
	{
		VectorCopy(origin, p1);
		VectorMA(origin, 16384, dir, p2);

		for (f2 = b->brush_faces; f2; f2 = f2->next)
		{
			if (f2 == f)
				continue;
			ClipLineToFace(p1, p2, f2);
		}

		if (f2)
			continue;
		if (VectorCompare(p1, origin))
			continue;
		if (ClipLineToFace(p1, p2, f))
			continue;

		Brush_SelectFaceForDragging(b, f, shear);
	}
}

/*
==================
Brush_SnapPlanepts
==================
*/
void Brush_SnapPlanepts (brush_t *b)
{
	int		i, j;
	face_t *f;

	if (g_qeglobals.d_savedinfo.bNoClamp)
		return;

	for (f = b->brush_faces; f; f = f->next)
		for (i = 0; i < 3; i++)
			for (j = 0; j < 3; j++)
				f->planepts[i][j] = floor(f->planepts[i][j] + 0.5);
}

/*
=================
Brush_Write
=================
*/
void Brush_Write (brush_t *b, FILE *f)
{
	int		i;
	char   *pname;
	face_t *fa;

	fprintf(f, "{\n");
	for (fa = b->brush_faces; fa; fa = fa->next)
	{
		for (i = 0; i < 3; i++)
			if (g_qeglobals.d_savedinfo.bBrushPrecision)	// sikk - Brush Precision
				fprintf(f, "( %f %f %f ) ", fa->planepts[i][0], fa->planepts[i][1], fa->planepts[i][2]);
			else
				fprintf(f, "( %d %d %d ) ", (int)fa->planepts[i][0], (int)fa->planepts[i][1], (int)fa->planepts[i][2]);

		pname = fa->texdef.name;
		if (pname[0] == 0)
			pname = "unnamed";

		Brush_CheckTexdef(b, fa, pname);	// sikk - Check Texdef - temp fix for Multiple Entity Undo Bug

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

// sikk---> Vertex Editing Splits Face
/*
=================
Brush_Convex
=================
*/
bool Brush_Convex (brush_t *b)
{
	face_t	*face1, *face2;

	for (face1 = b->brush_faces; face1; face1 = face1->next)
	{
		if (!face1->face_winding)
			continue;
		for (face2 = b->brush_faces; face2; face2 = face2->next)
		{
			if (face1 == face2)
				continue;
			if (!face2->face_winding)
				continue;
			if (Winding_PlanesConcave(face1->face_winding, face2->face_winding,
									  face1->plane.normal, face2->plane.normal,
									  face1->plane.dist, face2->plane.dist))
				return false;
		}
	}
	return true;
}

/*
=================
Brush_MoveVertexes

- The input brush must be convex
- The input brush must have face windings.
- The output brush will be convex.
- Returns true if the WHOLE vertex movement is performed.
=================
*/
bool Brush_MoveVertex (brush_t *b, vec3_t vertex, vec3_t delta, vec3_t end)
{
	int			i, j, k, nummovefaces, result, done;
	int			movefacepoints[MAX_MOVE_FACES];
	float		dot, front, back, frac, smallestfrac;
	face_t	   *f, *face, *newface, *lastface, *nextface;
	face_t	   *movefaces[MAX_MOVE_FACES];
	plane_t		plane;
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
	for (face = b->brush_faces; face; face = face->next)
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

	while(!done)
	{
		// chop off triangles from all brush faces that use the to be moved vertex
		// store pointers to these chopped off triangles in movefaces[]
		nummovefaces = 0;
		for (face = b->brush_faces; face; face = face->next)
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
						for (k = i; k < i + w->numpoints-3; k++)
						{
							VectorCopy(w->points[i], tmpw.points[0]);
							VectorCopy(w->points[(k + 1) % w->numpoints], tmpw.points[1]);
							VectorCopy(w->points[(k + 2) % w->numpoints], tmpw.points[2]);

							newface = Face_Clone(face);
							
							// get the original
							for (f = face; f->original; f = f->original)
								;
							newface->original = f;
							
							// store the new winding
							if (newface->face_winding) 
								Winding_Free(newface->face_winding);
							newface->face_winding = Winding_Clone(&tmpw);
							
							// get the texture information
							newface->d_texture = face->d_texture;

							// add the face to the brush
							newface->next = b->brush_faces;
							b->brush_faces = newface;

							// add this new triangle to the move faces
							movefacepoints[nummovefaces] = 0;
							movefaces[nummovefaces++] = newface;
						}

						// give the original face a new winding
						VectorCopy(w->points[(i - 2 + w->numpoints) % w->numpoints], tmpw.points[0]);
						VectorCopy(w->points[(i - 1 + w->numpoints) % w->numpoints], tmpw.points[1]);
						VectorCopy(w->points[i], tmpw.points[2]);
						Winding_Free(face->face_winding);
						face->face_winding = Winding_Clone(&tmpw);

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
						Winding_RemovePoint(w, i);
						
						// get texture crap right
						Face_SetColor(b, face);
						for (j = 0; j < w->numpoints; j++)
							EmitTextureCoordinates(w->points[j], face->d_texture, face);
						
						// make a triangle face
						newface = Face_Clone(face);
						
						// get the original
						for (f = face; f->original; f = f->original) 
							;
						newface->original = f;
						
						// store the new winding
						if (newface->face_winding) 
							Winding_Free(newface->face_winding);
						newface->face_winding = Winding_Clone(&tmpw);
						
						// get the texture
						newface->d_texture = face->d_texture;
//						newface->d_texture = Texture_ForName(newface->texdef.name);
						
						// add the face to the brush
						newface->next = b->brush_faces;
						b->brush_faces = newface;

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

		for (face = b->brush_faces; face; face = face->next)
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
				memcpy(&plane, &movefaces[i]->original->plane, sizeof(plane_t));
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
				if (!Plane_FromPoints(tmpw.points[0], tmpw.points[1], tmpw.points[2], &plane))
				{
					VectorCopy(w->points[(k + 2) % w->numpoints], tmpw.points[2]);
					if (!Plane_FromPoints(tmpw.points[0], tmpw.points[1], tmpw.points[2], &plane))
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
			if (fabs(front-back) < 0.001) 
				continue;

			// ok first only move till the plane is hit
			frac = front/(front-back);
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

			Face_MakePlane(movefaces[i]);
			if (VectorLength(movefaces[i]->plane.normal) < 0.1)
				result = false;
		}

		// if the brush is no longer convex
		if (!result || !Brush_Convex(b))
		{
			for (i = 0; i < nummovefaces; i++)
			{
				// move the vertex back to the initial position
				VectorCopy(start, movefaces[i]->face_winding->points[movefacepoints[i]]);
				// create new face plane
				for (j = 0; j < 3; j++)
					VectorCopy(movefaces[i]->face_winding->points[j], movefaces[i]->planepts[j]);

				Face_MakePlane(movefaces[i]);
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
			Face_SetColor(b, movefaces[i]);
			for (j = 0; j < movefaces[i]->face_winding->numpoints; j++)
				EmitTextureCoordinates(movefaces[i]->face_winding->points[j], movefaces[i]->d_texture, movefaces[i]);
		}

		// now try to merge faces with their original faces
		lastface = NULL;
		for (face = b->brush_faces; face; face = nextface)
		{
			nextface = face->next;
			if (!face->original)
			{
				lastface = face;
				continue;
			}
			if (!Plane_Equal(&face->plane, &face->original->plane, false))
			{
				lastface = face;
				continue;
			}
			w = Winding_TryMerge(face->face_winding, face->original->face_winding, face->plane.normal, true);
			if (!w)
			{
				lastface = face;
				continue;
			}
			Winding_Free(face->original->face_winding);
			face->original->face_winding = w;

			// get texture crap right
			Face_SetColor(b, face->original);
			for (j = 0; j < face->original->face_winding->numpoints; j++)
				EmitTextureCoordinates(face->original->face_winding->points[j], face->original->d_texture, face->original);

			// remove the face that was merged with the original
			if (lastface)
				lastface->next = face->next;
			else 
				b->brush_faces = face->next;
			Face_Free(face);
		}
	}
	return result;
}

/*
=================
Brush_ResetFaceOriginals
=================
*/
void Brush_ResetFaceOriginals (brush_t *b)
{
	face_t *face;

	for (face = b->brush_faces; face; face = face->next)
		face->original = NULL;
}
// <---sikk

//==========================================================================

/*
=================
AddPlanept
=================
*/
int AddPlanept (float *f)
{
	int i;

	for (i = 0; i < g_qeglobals.d_nNumMovePoints; i++)
		if (g_qeglobals.d_fMovePoints[i] == f)
			return 0;

	g_qeglobals.d_fMovePoints[g_qeglobals.d_nNumMovePoints++] = f;

	return 1;
}

/*
=================
BasePolyForPlane
=================
*/
winding_t *BasePolyForPlane (plane_t *p)
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
		v = fabs(p->normal[i]);
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

	v = DotProduct(vup, p->normal);
	VectorMA(vup, -v, p->normal, vup);
	VectorNormalize(vup);
	VectorScale(p->normal, p->dist, org);
	CrossProduct(vup, p->normal, vright);
	
	// These are to keep the brush restrained within the Map Size limit
	VectorScale(vup, g_qeglobals.d_savedinfo.nMapSize * 0.5, vup);	// sikk - Map Size was 8192
	VectorScale(vright, g_qeglobals.d_savedinfo.nMapSize * 0.5, vright);	//

	// project a really big	axis aligned box onto the plane
	w = Winding_Alloc(4);
	
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
=================
ClipLineToFace
=================
*/
bool ClipLineToFace (vec3_t p1, vec3_t p2, face_t *f)
{
	int		i;
	float	d1, d2, fr;
	float  *v;

	d1 = DotProduct(p1, f->plane.normal) - f->plane.dist;
	d2 = DotProduct(p2, f->plane.normal) - f->plane.dist;

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
==================
DrawBrushEntityName
==================
*/
void DrawBrushEntityName (brush_t *b)
{
	int		i;
	float	a, s, c;
	char   *name;
	vec3_t	mid;

	if (!b->owner)
		return;	// during contruction

	if (b->owner == g_peWorldEntity)
		return;

	if (b != b->owner->brushes.onext)
		return;	// not key brush

	// draw the angle pointer
	a = FloatForKey(b->owner, "angle");
	if (a)
	{
		s = sin(a / 180 * Q_PI);
		c = cos(a / 180 * Q_PI);
		for (i = 0; i < 3; i++)
			mid[i] = (b->mins[i] + b->maxs[i]) * 0.5; 

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
		name = ValueForKey(b->owner, "classname");
// sikk---> Draw Light Styles
		if (!strcmp(name, "light"))
		{
			char sz[24];
			
			if (*ValueForKey(b->owner, "style"))
			{
				sprintf(sz, "%s (style: %s)", ValueForKey(b->owner, "classname"), ValueForKey(b->owner, "style"));
				glRasterPos3f(b->mins[0] + 4, b->mins[1] + 4, b->mins[2] + 4);
				glCallLists(strlen(sz), GL_UNSIGNED_BYTE, sz);
			}
			else
			{
				glRasterPos3f(b->mins[0] + 4, b->mins[1] + 4, b->mins[2] + 4);
				glCallLists(strlen(name), GL_UNSIGNED_BYTE, name);
			}
		}
// <---sikk
		else
		{
			glRasterPos3f(b->mins[0] + 4, b->mins[1] + 4, b->mins[2] + 4);
			glCallLists(strlen(name), GL_UNSIGNED_BYTE, name);
		}
	}
}

/*
==================
DrawLight
==================
*/
void DrawLight (brush_t *b)
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

	strColor = ValueForKey(b->owner, "_color");

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
  
	fMid = b->mins[2] + (b->maxs[2] - b->mins[2]) / 2;

	vCorners[0][0] = b->mins[0];
	vCorners[0][1] = b->mins[1];
	vCorners[0][2] = fMid;

	vCorners[1][0] = b->mins[0];
	vCorners[1][1] = b->maxs[1];
	vCorners[1][2] = fMid;

	vCorners[2][0] = b->maxs[0];
	vCorners[2][1] = b->maxs[1];
	vCorners[2][2] = fMid;

	vCorners[3][0] = b->maxs[0];
	vCorners[3][1] = b->mins[1];
	vCorners[3][2] = fMid;

	vTop[0] = b->mins[0] + ((b->maxs[0] - b->mins[0]) / 2);
	vTop[1] = b->mins[1] + ((b->maxs[1] - b->mins[1]) / 2);
	vTop[2] = b->maxs[2];

	VectorCopy(vTop, vBottom);
	vBottom[2] = b->mins[2];

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


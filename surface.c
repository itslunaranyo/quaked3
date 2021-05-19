//==============================
//	surface.c
//==============================

#include "qe3.h"

//============================================================
//
//	TEXTURE ALIGNMENT
//
//============================================================



/*
===============
ProjectOnPlane
===============
*/
void ProjectOnPlane(vec3_t normal, float dist, vec3_t ez, vec3_t p)
{
	if (fabs(ez[0]) == 1)
		p[0] = (dist - normal[1] * p[1] - normal[2] * p[2]) / normal[0];
	else if (fabs(ez[1]) == 1)
		p[1] = (dist - normal[0] * p[0] - normal[2] * p[2]) / normal[1];
	else
		p[2] = (dist - normal[0] * p[0] - normal[1] * p[1]) / normal[2];
}

/*
===============
Back
===============
*/
void Back(vec3_t dir, vec3_t p)
{
	if (fabs(dir[0]) == 1)
		p[0] = 0;
	else if (fabs(dir[1]) == 1)
		p[1] = 0;
	else p[2] = 0;
}

/*
===============
ComputeScale

using scale[0] and scale[1]
===============
*/
void ComputeScale(vec3_t rex, vec3_t rey, vec3_t p, face_t *f)
{
	float px = DotProduct(rex, p);
	float py = DotProduct(rey, p);
	vec3_t aux;
	px *= f->texdef.scale[0];
	py *= f->texdef.scale[1];
	VectorCopy(rex, aux);
	VectorScale(aux, px, aux);
	VectorCopy(aux, p);
	VectorCopy(rey, aux);
	VectorScale(aux, py, aux);
	VectorAdd(p, aux, p);
}

/*
===============
ComputeAbsolute
===============
*/
void ComputeAbsolute(face_t *f, vec3_t p1, vec3_t p2, vec3_t p3)
{
	vec3_t	ex, ey, ez;	        // local axis base
	vec3_t	aux;
	vec3_t	rex, rey;

	// compute first local axis base
	TextureAxisFromPlane(&f->plane, ex, ey);
	CrossProduct(ex, ey, ez);

	VectorCopy(ex, aux);
	VectorScale(aux, -f->texdef.shift[0], aux);
	VectorCopy(aux, p1);
	VectorCopy(ey, aux);
	VectorScale(aux, -f->texdef.shift[1], aux);
	VectorAdd(p1, aux, p1);
	VectorCopy(p1, p2);
	VectorAdd(p2, ex, p2);
	VectorCopy(p1, p3);
	VectorAdd(p3, ey, p3);
	VectorCopy(ez, aux);
	VectorScale(aux, -f->texdef.rotate, aux);
	VectorRotate(p1, aux, p1);
	VectorRotate(p2, aux, p2);
	VectorRotate(p3, aux, p3);
	// computing rotated local axis base
	VectorCopy(ex, rex);
	VectorRotate(rex, aux, rex);
	VectorCopy(ey, rey);
	VectorRotate(rey, aux, rey);

	ComputeScale(rex, rey, p1, f);
	ComputeScale(rex, rey, p2, f);
	ComputeScale(rex, rey, p3, f);

	// project on normal plane along ez 
	// assumes plane normal is normalized
	ProjectOnPlane(f->plane.normal, f->plane.dist, ez, p1);
	ProjectOnPlane(f->plane.normal, f->plane.dist, ez, p2);
	ProjectOnPlane(f->plane.normal, f->plane.dist, ez, p3);
}

/*
====================
Clamp
====================
*/
void Clamp(float *f, int nClamp)
{
	float fFrac = *f - (int)*f;
	*f = (int)*f % nClamp;
	*f += fFrac;
}

/*
===============
AbsoluteToLocal
===============
*/
void AbsoluteToLocal(plane_t normal2, face_t *f, vec3_t p1, vec3_t p2, vec3_t p3)
{
	vec3_t	ex, ey, ez;
	vec3_t	aux;
	vec3_t	rex, rey;
	vec_t	x;
	vec_t	y;

	// computing new local axis base
	TextureAxisFromPlane(&normal2, ex, ey);
	CrossProduct(ex, ey, ez);

	// projecting back on (ex, ey)
	Back(ez, p1);
	Back(ez, p2);
	Back(ez, p3);

	// rotation
	VectorCopy(p2, aux);
	VectorSubtract(aux, p1, aux);

	x = DotProduct(aux, ex);
	y = DotProduct(aux, ey);
	f->texdef.rotate = 180 * atan2(y, x) / Q_PI;

	// computing rotated local axis base
	VectorCopy(ez, aux);
	VectorScale(aux, f->texdef.rotate, aux);
	VectorCopy(ex, rex);
	VectorRotate(rex, aux, rex);
	VectorCopy(ey, rey);
	VectorRotate(rey, aux, rey);

	// scale
	VectorCopy(p2, aux);
	VectorSubtract(aux, p1, aux);
	f->texdef.scale[0] = DotProduct(aux, rex);
	VectorCopy(p3, aux);
	VectorSubtract(aux, p1, aux);
	f->texdef.scale[1] = DotProduct(aux, rey);

	// shift
	// only using p1
	x = DotProduct(rex, p1);
	y = DotProduct(rey, p1);
	x /= f->texdef.scale[0];
	y /= f->texdef.scale[1];

	VectorCopy(rex, p1);
	VectorScale(p1, x, p1);
	VectorCopy(rey, aux);
	VectorScale(aux, y, aux);
	VectorAdd(p1, aux, p1);
	VectorCopy(ez, aux);
	VectorScale(aux, -f->texdef.rotate, aux);
	VectorRotate(p1, aux, p1);
	f->texdef.shift[0] = -DotProduct(p1, ex);
	f->texdef.shift[1] = -DotProduct(p1, ey);

	// stored rot is good considering local axis base
	// change it if necessary
	f->texdef.rotate = -f->texdef.rotate;

	Clamp(&f->texdef.shift[0], f->d_texture->width);
	Clamp(&f->texdef.shift[1], f->d_texture->height);
	Clamp(&f->texdef.rotate, 360);

	// lunaran fix: prefer small rotation and fewer negative scales
	if (abs(f->texdef.rotate) >= 90 && (f->texdef.scale[0] < 0 || f->texdef.scale[1] < 0))
	{
		if (f->texdef.rotate < 0)
			f->texdef.rotate += 180;
		else
			f->texdef.rotate -= 180;

		f->texdef.scale[0] *= -1.0f;
		f->texdef.scale[1] *= -1.0f;
	}
	// lunaran fix: snap texture shifts back to whole numbers so they don't creep with successive translations
	f->texdef.shift[0] = roundf(f->texdef.shift[0]);
	f->texdef.shift[1] = roundf(f->texdef.shift[1]);
	f->texdef.rotate = roundf(f->texdef.rotate);
}


/*
===============
Surf_FindReplace
===============
*/
void Surf_FindReplace(char *pFind, char *pReplace, bool bSelected, bool bForce)
{
	brush_t	*pBrush, *pList;
	face_t	*pFace;

	pList = (bSelected) ? &g_brSelectedBrushes : &g_brActiveBrushes;
	if (!bSelected)
		Select_DeselectAll(true);

	for (pBrush = pList->next; pBrush != pList; pBrush = pBrush->next)
	{
		for (pFace = pBrush->brush_faces; pFace; pFace = pFace->next)
		{
			if (bForce || strcmpi(pFace->texdef.name, pFind) == 0)
			{
				pFace->d_texture = Texture_ForName(pFace->texdef.name);
				strcpy(pFace->texdef.name, pReplace);
			}
		}
		Brush_Build(pBrush);
	}
	Sys_UpdateWindows(W_CAMERA);
}

/*
=================
Surf_ApplyTexdef
=================
*/
void Surf_ApplyTexdef(texdef_t *dst, texdef_t *src, int nSkipFlags)
{
	if (nSkipFlags)
	{
		if (!(nSkipFlags & SF_MIXEDNAME))
			strcpy(dst->name, src->name);
		if (!(nSkipFlags & SF_MIXEDSHIFTX))
			dst->shift[0] = src->shift[0];
		if (!(nSkipFlags & SF_MIXEDSHIFTY))
			dst->shift[1] = src->shift[1];
		if (!(nSkipFlags & SF_MIXEDSCALEX))
			dst->scale[0] = src->scale[0];
		if (!(nSkipFlags & SF_MIXEDSCALEY))
			dst->scale[1] = src->scale[1];
		if (!(nSkipFlags & SF_MIXEDROTATE))
			dst->rotate = src->rotate;
	}
	else
	{
		*dst = *src;
	}
}

/*
================
Surf_SetTexdef
================
*/
void Surf_SetTexdef(texdef_t *texdef, int nSkipFlags)
{
	brush_t	*b;

	if (Select_IsEmpty())
	{
		Surf_ApplyTexdef(&g_qeglobals.d_workTexDef, texdef, nSkipFlags);
		return;
	}

	// sikk---> Multiple Face Selection
	if (Select_FaceCount())
	{
		int i, nBrushCount = 0;
		brush_t	*pbrArray[MAX_MAP_BRUSHES];

		for (i = 0; i < Select_FaceCount(); i++)
		{
			// this check makes sure that brushes are only added to undo once 
			// and not once per selected face on brush
			if (!OnBrushList(g_pfaceSelectedFaces[i]->owner, pbrArray, nBrushCount))
			{
				pbrArray[nBrushCount] = g_pfaceSelectedFaces[i]->owner;
				nBrushCount++;
			}
		}

		// sikk - TODO: Set Face Texture Undo bug
		Undo_Start("Set Face Textures");	// sikk - Undo/Redo
		for (i = 0; i < nBrushCount; i++)
			Undo_AddBrush(pbrArray[i]);	// sikk - Undo/Redo

		for (i = 0; i < Select_FaceCount(); i++)
			Face_SetTexture(g_pfaceSelectedFaces[i], texdef, nSkipFlags);

		for (i = 0; i < nBrushCount; i++)
			Undo_EndBrush(pbrArray[i]);	// sikk - Undo/Redo
	}
	// <---sikk
	else if (Select_HasBrushes())
	{
		Undo_Start("Set Brush Textures");	// sikk - Undo/Redo
		for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
		{
			if (!b->owner->eclass->fixedsize)
			{
				Undo_AddBrush(b);	// sikk - Undo/Redo
				Brush_SetTexture(b, texdef, nSkipFlags);
				Undo_EndBrush(b);	// sikk - Undo/Redo
			}
		}
	}

	Undo_End();	// sikk - Undo/Redo
	SurfWnd_UpdateUI();
	Sys_UpdateWindows(W_ALL);
}




/*
===============
RotateFaceTexture
===============
*/
void RotateFaceTexture(face_t* f, int nAxis, float fDeg, vec3_t vOrigin)
{
	vec3_t	p1, p2, p3, rota;
	vec3_t	vNormal;
	plane_t	normal2;

	p1[0] = p1[1] = p1[2] = 0;
	VectorCopy(p1, p2);
	VectorCopy(p1, p3);
	VectorCopy(p1, rota);
	ComputeAbsolute(f, p1, p2, p3);

	rota[nAxis] = fDeg;
	VectorRotate2(p1, rota, vOrigin, p1);
	VectorRotate2(p2, rota, vOrigin, p2);
	VectorRotate2(p3, rota, vOrigin, p3);

	vNormal[0] = f->plane.normal[0];
	vNormal[1] = f->plane.normal[1];
	vNormal[2] = f->plane.normal[2];
	VectorRotate(vNormal, rota, vNormal);
	normal2.normal[0] = vNormal[0];
	normal2.normal[1] = vNormal[1];
	normal2.normal[2] = vNormal[2];
	AbsoluteToLocal(normal2, f, p1, p2, p3);
}

/*
===============
Surf_RotateForTransform
===============
*/
void Surf_RotateForTransform(int nAxis, float fDeg, vec3_t vOrigin)
{
	brush_t *b;
	face_t	*f;

	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		for (f = b->brush_faces; f; f = f->next)
		{
			RotateFaceTexture(f, nAxis, fDeg, vOrigin);
			Brush_Build(b);
		}
		Brush_Build(b);
	}
}


/*
===============
Surf_FitTexture
===============
*/
void Surf_FitTexture(float nHeight, float nWidth)
{
	brush_t	*b;

	if (Select_IsEmpty())
		return;

	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		Brush_FitTexture(b, nHeight, nWidth);
		Brush_Build(b);
	}

	// sikk---> Multiple Face Selection
	if (Select_FaceCount())
	{
		int i;
		for (i = 0; i < Select_FaceCount(); i++)
		{
			Face_FitTexture(g_pfaceSelectedFaces[i], nHeight, nWidth);
			Brush_Build(g_pfaceSelectedFaces[i]->owner);
		}
	}
	// <---sikk

	Sys_UpdateWindows(W_CAMERA);
}

// sikk---> Texture Manipulation Functions (Mouse & Keyboard)
/*
===========
Surf_ShiftTexture
===========
*/
void Surf_ShiftTexture(int x, int y)
{
	brush_t	*b;
	face_t	*f;

	if (Select_IsEmpty())
		return;

	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		for (f = b->brush_faces; f; f = f->next)
		{
			f->texdef.shift[0] += x;
			f->texdef.shift[1] += y;
		}

		Brush_Build(b);
	}
	// sikk---> Multiple Face Selection
	if (Select_FaceCount())
	{
		int i;
		for (i = 0; i < Select_FaceCount(); i++)
		{
			g_pfaceSelectedFaces[i]->texdef.shift[0] += x;
			g_pfaceSelectedFaces[i]->texdef.shift[1] += y;
		//	Surf_SetTexdef(&g_pfaceSelectedFaces[i]->texdef);
			Brush_Build(g_pfaceSelectedFaces[i]->owner);
		}
	}
	// <--sikk

	Sys_UpdateWindows(W_CAMERA);
}

/*
===========
Surf_ScaleTexture
===========
*/
void Surf_ScaleTexture(int x, int y)
{
	brush_t	*b;
	face_t	*f;

	if (Select_IsEmpty())
		return;

	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		for (f = b->brush_faces; f; f = f->next)
		{
			f->texdef.scale[0] += x / 100.0f;
			f->texdef.scale[1] += y / 100.0f;
		}

		Brush_Build(b);
	}

	// sikk---> Multiple Face Selection
	if (Select_FaceCount())
	{
		int i;
		for (i = 0; i < Select_FaceCount(); i++)
		{
			g_pfaceSelectedFaces[i]->texdef.scale[0] += x / 100.0f;
			g_pfaceSelectedFaces[i]->texdef.scale[1] += y / 100.0f;
		//	Surf_SetTexdef(&g_pfaceSelectedFaces[i]->texdef);
			Brush_Build(g_pfaceSelectedFaces[i]->owner);
		}
	}
	// <---sikk

	Sys_UpdateWindows(W_CAMERA);
}

/*
===========
Surf_RotateTexture
===========
*/
void Surf_RotateTexture(int deg)
{
	brush_t	*b;
	face_t	*f;

	if (Select_IsEmpty())
		return;

	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		for (f = b->brush_faces; f; f = f->next)
		{
			f->texdef.rotate += deg;
			if (f->texdef.rotate >180)
				f->texdef.rotate -= 360;
			if (f->texdef.rotate <= -180)
				f->texdef.rotate += 360;

		}
		Brush_Build(b);
	}

	// sikk---> Multiple Face Selection
	if (Select_FaceCount())
	{
		int i;
		for (i = 0; i < Select_FaceCount(); i++)
		{
			g_pfaceSelectedFaces[i]->texdef.rotate += deg;
			if (g_pfaceSelectedFaces[i]->texdef.rotate > 180)
				g_pfaceSelectedFaces[i]->texdef.rotate -= 360;
			if (g_pfaceSelectedFaces[i]->texdef.rotate <= -180)
				g_pfaceSelectedFaces[i]->texdef.rotate += 360;
		//	Surf_SetTexdef(&g_pfaceSelectedFaces[i]->texdef);
			Brush_Build(g_pfaceSelectedFaces[i]->owner);
		}
	}
	// <---sikk

	Sys_UpdateWindows(W_CAMERA);
}
// <---sikk
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
void ProjectOnPlane(const vec3 normal, const float dist, const vec3 ez, vec3 &p)
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
void Back(const vec3 dir, vec3 &p)
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
void ComputeScale(const vec3 rex, const vec3 rey, vec3 &p, Face *f)
{
	float px = DotProduct(rex, p);
	float py = DotProduct(rey, p);
	vec3 aux;

	px *= f->texdef.scale[0];
	py *= f->texdef.scale[1];
	p = rex * px + rey * py;
}

/*
===============
ComputeAbsolute
===============
*/
void ComputeAbsolute(Face *f, vec3 &p1, vec3 &p2, vec3 &p3)
{
	vec3	ex, ey, ez;	        // local axis base
	vec3	aux;
	vec3	rex, rey;

	// compute first local axis base
	f->plane.GetTextureAxis(ex, ey);
	ez = CrossProduct(ex, ey);

	aux = ex * -f->texdef.shift[0];
	p1 = aux;
	aux = ey * -f->texdef.shift[1];
	p1 = p1 + aux;
	p2 = p1 + ex;
	p3 = p1 + ey;
	aux = ez * -f->texdef.rotate;
	VectorRotate(p1, aux, p1);
	VectorRotate(p2, aux, p2);
	VectorRotate(p3, aux, p3);

	// computing rotated local axis base
	VectorRotate(ex, aux, rex);
	VectorRotate(ey, aux, rey);

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
void AbsoluteToLocal(Plane normal2, Face *f, vec3 &p1, vec3 &p2, vec3 &p3)
{
	vec3	ex, ey, ez;
	vec3	aux;
	vec3	rex, rey;
	float	x;
	float	y;

	// computing new local axis base
	normal2.GetTextureAxis(ex, ey);
	ez = CrossProduct(ex, ey);

	// projecting back on (ex, ey)
	Back(ez, p1);
	Back(ez, p2);
	Back(ez, p3);

	// rotation
	aux = p2 - p1;

	x = DotProduct(aux, ex);
	y = DotProduct(aux, ey);
	f->texdef.rotate = 180 * atan2(y, x) / Q_PI;

	// computing rotated local axis base
	aux = ez * f->texdef.rotate;
	VectorRotate(ex, aux, rex);
	VectorRotate(ey, aux, rey);

	// scale
	f->texdef.scale[0] = DotProduct(p2 - p1, rex);
	f->texdef.scale[1] = DotProduct(p3 - p1, rey);

	// shift
	// only using p1
	x = DotProduct(rex, p1);
	y = DotProduct(rey, p1);
	x /= f->texdef.scale[0];
	y /= f->texdef.scale[1];

	p1 = rex * x + rey * y;
	aux = ez * -f->texdef.rotate;
	VectorRotate(p1, aux, p1);
	f->texdef.shift[0] = -DotProduct(p1, ex);
	f->texdef.shift[1] = -DotProduct(p1, ey);

	// stored rot is good considering local axis base
	// change it if necessary
	f->texdef.rotate = -f->texdef.rotate;

	Clamp(&f->texdef.shift[0], f->texdef.tex->width);
	Clamp(&f->texdef.shift[1], f->texdef.tex->height);
	Clamp(&f->texdef.rotate, 360);

	// lunaran fix: prefer small rotation and fewer negative scales
	if (fabs(f->texdef.rotate) >= 90 && (f->texdef.scale[0] < 0 || f->texdef.scale[1] < 0))
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
	Brush	*pBrush, *pList;
	Face	*pFace;
	Texture *txFind, *txRepl;

	txFind = Textures::ForName(pFind);
	txRepl = Textures::ForName(pReplace);

	pList = (bSelected) ? &g_brSelectedBrushes : &g_map.brActive;
	if (!bSelected)
		Selection::DeselectAll();

	for (pBrush = pList->next; pBrush != pList; pBrush = pBrush->next)
	{
		for (pFace = pBrush->basis.faces; pFace; pFace = pFace->fnext)
		{
			//if (bForce || _strcmpi(pFace->texdef.name, pFind) == 0)
			if (bForce || pFace->texdef.tex == txFind)
			{
				//pFace->DEPtexture = Textures::ForName(pFace->texdef.name);
				pFace->texdef.tex = txRepl;
				strcpy(pFace->texdef.name, pReplace);
			}
		}
		pBrush->Build();
	}
	Sys_UpdateWindows(W_CAMERA);
}

/*
=================
Surf_ApplyTexdef
=================
*/
void Surf_ApplyTexdef(TexDef *dst, TexDef *src, int nSkipFlags)
{
	if (nSkipFlags)
	{
		if (!(nSkipFlags & SURF_MIXEDNAME))
		{
			strncpy(dst->name, src->name, MAX_TEXNAME);
			dst->tex = src->tex;
		}
		if (!(nSkipFlags & SURF_MIXEDSHIFTX))
			dst->shift[0] = src->shift[0];
		if (!(nSkipFlags & SURF_MIXEDSHIFTY))
			dst->shift[1] = src->shift[1];
		if (!(nSkipFlags & SURF_MIXEDSCALEX))
			dst->scale[0] = src->scale[0];
		if (!(nSkipFlags & SURF_MIXEDSCALEY))
			dst->scale[1] = src->scale[1];
		if (!(nSkipFlags & SURF_MIXEDROTATE))
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
void Surf_SetTexdef(TexDef *texdef, int nSkipFlags)
{
	Brush	*b;

	if (Selection::IsEmpty())
	{
		Surf_ApplyTexdef(&g_qeglobals.d_workTexDef, texdef, nSkipFlags);
		return;
	}

	// sikk---> Multiple Face Selection
	if (Selection::FaceCount())
	{
		int i, nBrushCount = 0;
		Brush	*pbrArray[MAX_MAP_BRUSHES];

		for (i = 0; i < Selection::FaceCount(); i++)
		{
			// this check makes sure that brushes are only added to undo once 
			// and not once per selected face on brush
			if (!OnBrushList(g_vfSelectedFaces[i]->owner, pbrArray, nBrushCount))
			{
				pbrArray[nBrushCount] = g_vfSelectedFaces[i]->owner;
				nBrushCount++;
			}
		}

		// sikk - TODO: Set Face Texture Undo bug
		Undo::Start("Set Face Textures");	// sikk - Undo/Redo
		for (i = 0; i < nBrushCount; i++)
			Undo::AddBrush(pbrArray[i]);	// sikk - Undo/Redo

		for (i = 0; i < Selection::FaceCount(); i++)
			g_vfSelectedFaces[i]->SetTexture(texdef, nSkipFlags);

		for (i = 0; i < nBrushCount; i++)
			Undo::EndBrush(pbrArray[i]);	// sikk - Undo/Redo
	}
	// <---sikk
	else if (Selection::HasBrushes())
	{
		Undo::Start("Set Brush Textures");	// sikk - Undo/Redo
		for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
		{
			if (b->owner->IsBrush())
			{
				Undo::AddBrush(b);	// sikk - Undo/Redo
				b->SetTexture(texdef, nSkipFlags);
				Undo::EndBrush(b);	// sikk - Undo/Redo
			}
		}
	}

	Undo::End();	// sikk - Undo/Redo
	SurfWnd_UpdateUI();
	Sys_UpdateWindows(W_SCENE|W_TEXTURE);
}




/*
===============
RotateFaceTexture
===============
*/
void RotateFaceTexture(Face* f, int nAxis, float fDeg, const vec3 vOrigin)
{
	vec3	p1, p2, p3, rota;
	vec3	vNormal;
	Plane	normal2;

	p1[0] = p1[1] = p1[2] = 0;
	p2 = p1;
	p3 = p1;
	rota = p1;
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
void Surf_RotateForTransform(int nAxis, float fDeg, const vec3 vOrigin)
{
	Brush *b;
	Face	*f;

	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		for (f = b->basis.faces; f; f = f->fnext)
		{
			RotateFaceTexture(f, nAxis, fDeg, vOrigin);
			b->Build();
		}
		b->Build();
	}
}


/*
===============
Surf_FitTexture
===============
*/
void Surf_FitTexture(float nHeight, float nWidth)
{
	Brush	*b;

	if (Selection::IsEmpty())
		return;

	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		b->FitTexture(nHeight, nWidth);
		b->Build();
	}

	// sikk---> Multiple Face Selection
	if (Selection::FaceCount())
	{
		int i;
		for (i = 0; i < Selection::FaceCount(); i++)
		{
			g_vfSelectedFaces[i]->FitTexture(nHeight, nWidth);
			g_vfSelectedFaces[i]->owner->Build();
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
	Brush	*b;
	Face	*f;

	if (Selection::IsEmpty())
		return;

	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		for (f = b->basis.faces; f; f = f->fnext)
		{
			f->texdef.shift[0] += x;
			f->texdef.shift[1] += y;
		}

		b->Build();
	}
	// sikk---> Multiple Face Selection
	if (Selection::FaceCount())
	{
		int i;
		for (i = 0; i < Selection::FaceCount(); i++)
		{
			g_vfSelectedFaces[i]->texdef.shift[0] += x;
			g_vfSelectedFaces[i]->texdef.shift[1] += y;
		//	Surf_SetTexdef(&g_vfSelectedFaces[i]->texdef);
			g_vfSelectedFaces[i]->owner->Build();
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
	Brush	*b;
	Face	*f;

	if (Selection::IsEmpty())
		return;

	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		for (f = b->basis.faces; f; f = f->fnext)
		{
			f->texdef.scale[0] += x / 100.0f;
			f->texdef.scale[1] += y / 100.0f;
		}

		b->Build();
	}

	// sikk---> Multiple Face Selection
	if (Selection::FaceCount())
	{
		int i;
		for (i = 0; i < Selection::FaceCount(); i++)
		{
			g_vfSelectedFaces[i]->texdef.scale[0] += x / 100.0f;
			g_vfSelectedFaces[i]->texdef.scale[1] += y / 100.0f;
		//	Surf_SetTexdef(&g_vfSelectedFaces[i]->texdef);
			g_vfSelectedFaces[i]->owner->Build();
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
	Brush	*b;
	Face	*f;

	if (Selection::IsEmpty())
		return;

	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		for (f = b->basis.faces; f; f = f->fnext)
		{
			f->texdef.rotate += deg;
			if (f->texdef.rotate >180)
				f->texdef.rotate -= 360;
			if (f->texdef.rotate <= -180)
				f->texdef.rotate += 360;

		}
		b->Build();
	}

	// sikk---> Multiple Face Selection
	if (Selection::FaceCount())
	{
		int i;
		for (i = 0; i < Selection::FaceCount(); i++)
		{
			g_vfSelectedFaces[i]->texdef.rotate += deg;
			if (g_vfSelectedFaces[i]->texdef.rotate > 180)
				g_vfSelectedFaces[i]->texdef.rotate -= 360;
			if (g_vfSelectedFaces[i]->texdef.rotate <= -180)
				g_vfSelectedFaces[i]->texdef.rotate += 360;
		//	Surf_SetTexdef(&g_vfSelectedFaces[i]->texdef);
			g_vfSelectedFaces[i]->owner->Build();
		}
	}
	// <---sikk

	Sys_UpdateWindows(W_CAMERA);
}
// <---sikk
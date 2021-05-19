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
	Brush	*br;
	Face	*f;
	Texture *txFind;
	TexDef	tdRepl;

	txFind = Textures::ForName(pFind);
	tdRepl.Set(pReplace);

	CmdTextureApply *cmdTA = new CmdTextureApply();

	if (bForce)
	{
		if (bSelected)
		{
			if (Selection::HasBrushes())
				cmdTA->UseBrushes(&g_brSelectedBrushes);
			else if (Selection::NumFaces())
				cmdTA->UseFaces(Selection::faces);
			else
			{
				Sys_Printf("Couldn't replace; nothing selected\n");
				delete cmdTA;
				return;
			}
		}
		else
		{
			if (MessageBox(g_qeglobals.d_hwndMain,
				"This will indiscriminately apply the chosen texture to every visible brush face in the entire map.\n\nAre you sure you want to do this?\n",
				"QuakeEd 3: Really?", MB_YESNO | MB_ICONEXCLAMATION) == IDNO)
			{
				delete cmdTA;
				return;
			}
			cmdTA->UseBrushes(&g_brSelectedBrushes);
			for (br = g_map.brActive.next; br != &g_map.brActive; br = br->next)
			{
				if (!br->IsFiltered())
					cmdTA->UseBrush(br);
			}
		}
	}
	else
	{
		if (Selection::HasBrushes())
		{
			for (br = g_brSelectedBrushes.next; br != &g_brSelectedBrushes; br = br->next)
			{
				for (f = br->basis.faces; f; f = f->fnext)
				{
					if (f->texdef.tex == txFind)
						cmdTA->UseFace(f);
				}
			}
		}

		if (bSelected)
		{
			if (Selection::NumFaces())
			{
				for (auto fIt = Selection::faces.begin(); fIt != Selection::faces.end(); ++fIt)
				{
					if ((*fIt)->texdef.tex == txFind)
						cmdTA->UseFace((*fIt));
				}
			}
			else if (!Selection::HasBrushes())
			{
				Sys_Printf("Couldn't replace; nothing selected\n");
				delete cmdTA;
				return;
			}
		}
		else
		{
			for (br = g_map.brActive.next; br != &g_map.brActive; br = br->next)
			{
				if (!br->IsFiltered())
				{
					for (f = br->basis.faces; f; f = f->fnext)
					{
						if (f->texdef.tex == txFind)
							cmdTA->UseFace(f);
					}
				}
			}
		}
	}
	cmdTA->Apply(tdRepl, SFI_ALL - SFI_NAME);
	g_cmdQueue.Complete(cmdTA);

	// while f&r dialog is modal, forcibly redraw the camera view so we can see the result immediately
	Sys_ForceUpdateWindows(W_CAMERA);
}

/*
=================
Surf_ApplyTexdef
=================
*/
void Surf_ApplyTexdef(TexDef &dst, TexDef &src, unsigned flags)
{
	if (flags)
	{
		if (!(flags & SFI_NAME))
		{
			strncpy(dst.name, src.name, MAX_TEXNAME);
			dst.tex = src.tex;
		}
		if (!(flags & SFI_SHIFTX))
			dst.shift[0] = src.shift[0];
		if (!(flags & SFI_SHIFTY))
			dst.shift[1] = src.shift[1];
		if (!(flags & SFI_SCALEX))
			dst.scale[0] = src.scale[0];
		if (!(flags & SFI_SCALEY))
			dst.scale[1] = src.scale[1];
		if (!(flags & SFI_ROTATE))
			dst.rotate = src.rotate;
	}
	else
	{
		dst = src;
	}
}

/*
================
Surf_SetTexdef
================
*/
void Surf_SetTexdef(TexDef &texdef, unsigned flags)
{
	if (Selection::IsEmpty())
	{
		Surf_ApplyTexdef(g_qeglobals.d_workTexDef, texdef, flags);
		return;
	}

	CmdTextureApply *cmdTA = new CmdTextureApply();

	if (Selection::NumFaces())
		cmdTA->UseFaces(Selection::faces);
	else if (Selection::HasBrushes())
		cmdTA->UseBrushes(&g_brSelectedBrushes);

	cmdTA->Apply(texdef, flags);
	g_cmdQueue.Complete(cmdTA);

	/*
	if (Selection::NumFaces())
	{
		for (auto fIt = Selection::faces.begin(); fIt != Selection::faces.end(); ++fIt)
			(*fIt)->SetTexture(texdef, flags);
	}
	else if (Selection::HasBrushes())
	{
		Brush *b;
		for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
		{
			if (b->owner->IsBrush())
			{
				b->SetTexture(texdef, flags);
			}
		}
	}
	//SurfWnd_UpdateUI();
	Sys_UpdateWindows(W_SCENE|W_TEXTURE|W_SURF);
	*/
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


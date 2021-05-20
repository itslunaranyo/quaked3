//==============================
//	surface.c
//==============================

#include "qe3.h"
#include "CmdTextureApply.h"
#include <glm/gtx/rotate_vector.hpp>

//============================================================
//
//	TEXTURE ALIGNMENT
//
//============================================================



/*
===============
ProjectOnPlane
===============
void ProjectOnPlane(const vec3 normal, const float dist, const vec3 ez, vec3 &out)
{
	if (fabs(ez[0]) == 1)
		out[0] = (dist - normal[1] * out[1] - normal[2] * out[2]) / normal[0];
	else if (fabs(ez[1]) == 1)
		out[1] = (dist - normal[0] * out[0] - normal[2] * out[2]) / normal[1];
	else
		out[2] = (dist - normal[0] * out[0] - normal[1] * out[1]) / normal[2];
}
*/

/*
===============
Surface::Back
===============
*/
void Surface::Back(const vec3 dir, vec3 &p)
{
	if (fabs(dir[0]) == 1)
		p[0] = 0;
	else if (fabs(dir[1]) == 1)
		p[1] = 0;
	else p[2] = 0;
}

/*
===============
Surface::ComputeScale

using scale[0] and scale[1]
===============
*/
void Surface::ComputeScale(const vec3 rex, const vec3 rey, vec3 &p, TexDef &td)
{
	float px = DotProduct(rex, p);
	float py = DotProduct(rey, p);
	vec3 aux;

	px *= td.scale[0];
	py *= td.scale[1];
	p = rex * px + rey * py;
}

/*
===============
Surface::ComputeAbsolute
===============
*/
void Surface::ComputeAbsolute(Face *f, vec3 &p1, vec3 &p2, vec3 &p3)
{
	ComputeAbsolute(f->plane, f->texdef, p1, p2, p3);
}

/*
===============
Surface::ComputeAbsolute
===============
*/
void Surface::ComputeAbsolute(Plane &p, TexDef &td, vec3 &p1, vec3 &p2, vec3 &p3)
{
	vec3	ex, ey, ez;	        // local axis base
	vec3	aux;
	vec3	rex, rey;

	// compute first local axis base
	p.GetTextureAxis(ex, ey);
	ez = CrossProduct(ex, ey);

	p1 = ex * -td.shift[0] + ey * -td.shift[1];
	p2 = p1 + ex;
	p3 = p1 + ey;
	aux = ez * -td.rotate;
	VectorRotate(p1, aux, p1);
	VectorRotate(p2, aux, p2);
	VectorRotate(p3, aux, p3);

	// computing rotated local axis base
	VectorRotate(ex, aux, rex);
	VectorRotate(ey, aux, rey);

	ComputeScale(rex, rey, p1, td);
	ComputeScale(rex, rey, p2, td);
	ComputeScale(rex, rey, p3, td);

	// project on normal plane along ez 
	// assumes plane normal is normalized
	p1 = p.ProjectPointAxial(p1, ez);
	p2 = p.ProjectPointAxial(p2, ez);
	p3 = p.ProjectPointAxial(p3, ez);
}

/*
====================
Surface::Clamp
====================
*/
void Surface::Clamp(float *f, int nClamp)
{
	float fFrac = *f - (int)*f;
	*f = (int)*f % nClamp;
	*f += fFrac;
}

/*
===============
Surface::AbsoluteToLocal
===============
*/
void Surface::AbsoluteToLocal(Plane normal2, TexDef &td, vec3 &p1, vec3 &p2, vec3 &p3)
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
	td.rotate = 180 * atan2(y, x) / Q_PI;

	// computing rotated local axis base
	aux = ez * td.rotate;
	VectorRotate(ex, aux, rex);
	VectorRotate(ey, aux, rey);

	// scale
	td.scale[0] = DotProduct(p2 - p1, rex);
	td.scale[1] = DotProduct(p3 - p1, rey);

	// shift
	// only using p1
	x = DotProduct(rex, p1);
	y = DotProduct(rey, p1);
	x /= td.scale[0];
	y /= td.scale[1];

	p1 = rex * x + rey * y;
	aux = ez * -td.rotate;
	VectorRotate(p1, aux, p1);
	td.shift[0] = -DotProduct(p1, ex);
	td.shift[1] = -DotProduct(p1, ey);

	// stored rot is good considering local axis base
	// change it if necessary
	td.rotate = -td.rotate;

	Clamp(&td.shift[0], td.tex->width);
	Clamp(&td.shift[1], td.tex->height);
	Clamp(&td.rotate, 360);

	// lunaran fix: prefer small rotation and fewer negative scales
	if (fabs(td.rotate) >= 90 && (td.scale[0] < 0 || td.scale[1] < 0))
	{
		if (td.rotate < 0)
			td.rotate += 180;
		else
			td.rotate -= 180;

		td.scale[0] *= -1.0f;
		td.scale[1] *= -1.0f;
	}
	// lunaran fix: snap texture shifts back to whole numbers so they don't creep with successive translations
	td.shift[0] = roundf(td.shift[0]);
	td.shift[1] = roundf(td.shift[1]);

	td.scale[0] = glm::round(td.scale[0] * 1000) / 1000;
	td.scale[1] = glm::round(td.scale[1] * 1000) / 1000;

	// lunaran: moving to float rotations
	//td.rotate = roundf(td.rotate);
}


/*
===============
Surface::FindReplace
===============
*/
void Surface::FindReplace(char *pFind, char *pReplace, bool bSelected, bool bForce)
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
				for (f = br->faces; f; f = f->fnext)
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
					for (f = br->faces; f; f = f->fnext)
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
Surface::ApplyTexdef
=================
*/
void Surface::ApplyTexdef(TexDef &dst, TexDef &src, unsigned flags)
{
	if (flags)
	{
		if (!(flags & SFI_NAME))
		{
			strncpy(dst.name, src.name, MAX_TEXNAME);
			dst.tex = src.tex;
			if (&dst != &g_qeglobals.d_workTexDef)
			{
				dst.tex->Use();
			}
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
Surface::SetTexdef
================
*/
void Surface::SetTexdef(TexDef &texdef, unsigned flags)
{
	if (Selection::IsEmpty())
	{
		Surface::ApplyTexdef(g_qeglobals.d_workTexDef, texdef, flags);
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
	//WndSurf_UpdateUI();
	Sys_UpdateWindows(W_SCENE|W_TEXTURE|W_SURF);
	*/
}




/*
===============
Surface::RotateFaceTexture
===============
*/
void Surface::RotateFaceTexture(Face* f, int nAxis, float fDeg, const vec3 vOrigin)
{
	vec3	p1, p2, p3, rota;
	vec3	vNormal;
	Plane	normal2;

	rota = p1;
	ComputeAbsolute(f, p1, p2, p3);

	rota[nAxis] = fDeg;
	VectorRotate2(p1, rota, vOrigin, p1);
	VectorRotate2(p2, rota, vOrigin, p2);
	VectorRotate2(p3, rota, vOrigin, p3);

	vNormal = f->plane.normal;
	VectorRotate(vNormal, rota, vNormal);
	normal2.normal = vNormal;
	AbsoluteToLocal(normal2, f->texdef, p1, p2, p3);
}

/*
===============
Surface::RotateForTransform
===============
*/
void Surface::RotateForTransform(int nAxis, float fDeg, const vec3 vOrigin)
{
	Brush *b;
	Face	*f;

	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		for (f = b->faces; f; f = f->fnext)
		{
			RotateFaceTexture(f, nAxis, fDeg, vOrigin);
			b->Build();
		}
		b->Build();
	}
}

void Surface::WrapProjection(Plane &from, Plane &to, TexDef &td)
{
	mat4 trans;
	vec3 fp1, fp2, fp3;
	vec3 rot, org, dir;
	vec3 norm1, norm2;
	float ang;

	if (from.normal == to.normal)
		return;

	// from-plane local texdef to absolute
	ComputeAbsolute(from, td, fp1, fp2, fp3);

	// find point on intersection line
	dir = glm::normalize(fp2 - fp1);
	if (!to.TestRay(fp1, dir, org))
	{
		dir = glm::normalize(fp3 - fp1);
		if (!to.TestRay(fp1, dir, org))
			return;
	}

	// offset absolutes by point
	fp1 -= org;
	fp2 -= org;
	fp3 -= org;

	rot = CrossProduct(from.normal, to.normal);
	if (g_cfgEditor.TexProjectionMode == TEX_PROJECT_AXIAL)
	{
		// use dir as junk vector
		norm1 = from.GetTextureAxis(dir, dir);
		norm2 = to.GetTextureAxis(dir, dir);
	}
	else
	{
		norm1 = from.normal;
		norm2 = to.normal;
	}

	// rotate absolutes around intersection vector
	ang = acosf(DotProduct(norm1, norm2));
	if (!ang)
		return;
	fp1 = glm::rotate(fp1, ang, rot);
	fp2 = glm::rotate(fp2, ang, rot);
	fp3 = glm::rotate(fp3, ang, rot);

	// un-offset rotated absolutes
	fp1 += org;
	fp2 += org;
	fp3 += org;

	// convert back to local for to-plane
	AbsoluteToLocal(to, td, fp1, fp2, fp3);
}


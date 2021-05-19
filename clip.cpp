//==============================
//	clip.cpp
//==============================

#include "qe3.h"

#define CLIP_CAMDOT 0.9996f

clippoint_t		g_cpClip1;
clippoint_t		g_cpClip2;
clippoint_t		g_cpClip3;
clippoint_t		*g_pcpMovingClip;

// lunaran - let clip tool store the 2-point axis projection rather than derive it in various places
int				g_nClipAxis;

// lunaran - moved from globals in anticipation of wrapping clip tool in an object
bool	    d_bClipSwitch;

CmdBrushClip *g_pcmdBC;

void Clip_Crosshair(bool bCrossHair)
{
	SetCursor((bCrossHair) ? LoadCursor(NULL, IDC_CROSS) : LoadCursor(NULL, IDC_ARROW));
}

/*
============================================================================

	CLIP MODE

============================================================================
*/

/*
==================
Clip_StartCommand
==================
*/
void Clip_StartCommand()
{
	if (g_pcmdBC) return;

	g_pcmdBC = new CmdBrushClip();
	g_pcmdBC->SetSide(d_bClipSwitch ? CmdBrushClip::clipside::CLIP_BACK : CmdBrushClip::clipside::CLIP_FRONT);
	g_pcmdBC->StartBrushes(&g_brSelectedBrushes);
}

/*
==================
Clip_SetMode
==================
*/
void Clip_SetMode ()
{
	g_qeglobals.d_bClipMode ^= 1;
	Clip_ResetMode();
	Sys_UpdateGridStatusBar();
}

/*
==================
Clip_UnsetMode
==================
*/
void Clip_UnsetMode ()
{
	if (!g_qeglobals.d_bClipMode) return;
	g_qeglobals.d_bClipMode = false;
	Clip_ResetMode();
	Sys_UpdateGridStatusBar();
}

/*
==================
Clip_ResetMode
==================
*/
void Clip_ResetMode ()
{
	if (g_qeglobals.d_bClipMode)
	{
		g_cpClip1.ptClip = vec3(0);
		g_cpClip1.bSet = false;
		g_cpClip2.ptClip = vec3(0);
		g_cpClip2.bSet = false;
		g_cpClip3.ptClip = vec3(0);
		g_cpClip3.bSet = false;
	}
	else
	{
		if (g_pcpMovingClip)
		{
			ReleaseCapture();
			g_pcpMovingClip = NULL;
		}
	}
	if (g_pcmdBC)
	{
		delete g_pcmdBC;
		g_pcmdBC = nullptr;
	}

	Sys_UpdateWindows(W_SCENE);
}

/*
==================
Clip_PointsUpdated
==================
*/
void Clip_PointsUpdated()
{
	assert(g_pcmdBC);
	if (!g_cpClip1.bSet)
	{
		g_pcmdBC->UnsetPoints();
		return;
	}

	if (g_cpClip3.bSet)
	{
		// use all three points
		g_pcmdBC->SetPoints(g_cpClip1.ptClip, g_cpClip2.ptClip, g_cpClip3.ptClip);
	}
	else if (g_cpClip2.bSet)
	{
		// use implied third point
		vec3 p3 = g_cpClip2.ptClip;
		p3[g_nClipAxis] += 128;
		g_pcmdBC->SetPoints(g_cpClip1.ptClip, g_cpClip2.ptClip, p3);
	}
}


/*
==================
Clip_Clip
==================
*/
void Clip_Clip ()
{
	//Brush	   *pList;
	//Brush		hold_brushes;

	if (!g_qeglobals.d_bClipMode || !g_pcmdBC)
		return;

	g_cmdQueue.Complete(g_pcmdBC);
	g_pcmdBC->Select();
	g_pcmdBC = nullptr;
	Clip_ResetMode();

	Sys_UpdateWindows(W_SCENE);
}

/*
==================
Clip_Split
==================
*/
void Clip_Split ()
{
	if (!g_qeglobals.d_bClipMode || !g_pcmdBC)
		return;

	g_pcmdBC->SetSide(CmdBrushClip::clipside::CLIP_BOTH);
	g_cmdQueue.Complete(g_pcmdBC);
	g_pcmdBC->Select();
	g_pcmdBC = nullptr;
	Clip_ResetMode();
	Sys_UpdateWindows(W_SCENE);
}

/*
==================
Clip_Flip
==================
*/
void Clip_Flip ()
{
	if (!g_qeglobals.d_bClipMode || !g_pcmdBC)
		return;

	d_bClipSwitch = !d_bClipSwitch;
	g_pcmdBC->SetSide(d_bClipSwitch ? CmdBrushClip::clipside::CLIP_BACK : CmdBrushClip::clipside::CLIP_FRONT);

	Sys_UpdateWindows(W_SCENE);
}

/*
==================
Clip_StartNextPoint
==================
*/
clippoint_t* Clip_StartNextPoint()
{
	clippoint_t *pt;
	pt = nullptr;

	if (g_cpClip1.bSet == false)
	{
		Clip_StartCommand();
		pt = &g_cpClip1;
		g_cpClip1.bSet = true;
	}
	else if (g_cpClip2.bSet == false)
	{
		pt = &g_cpClip2;
		g_cpClip2.bSet = true;
	}
	else if (g_cpClip3.bSet == false)
	{
		pt = &g_cpClip3;
		g_cpClip3.bSet = true;
	}
	else
	{
		Clip_ResetMode();
		pt = &g_cpClip1;
		g_cpClip1.bSet = true;
	}
	return pt;
}


/*
============================================================================

	CLIP POINT 3D

============================================================================
*/

/*
==================
Clip_CamStartQuickClip
==================
*/
void Clip_CamStartQuickClip(int x, int y)
{
	Clip_ResetMode();

	Clip_CamDropPoint(x, y);
	Clip_CamDropPoint(x, y);
	g_pcpMovingClip = &g_cpClip2;
}

/*
==================
Clip_CamEndQuickClip
==================
*/
void Clip_CamEndQuickClip()
{
	Clip_CamEndPoint();
	Clip_Clip();
}

/*
==================
SnapToPoint
==================
*/
void SnapToPoint(vec3 &point)
{
	for (int i = 0; i < 3; i++)
		point[i] = floor(point[i] / g_qeglobals.d_nGridSize + 0.5f) * g_qeglobals.d_nGridSize;
}

/*
==================
Clip_CamPointOnSelection
==================
*/
bool Clip_CamPointOnSelection(int x, int y, vec3 &out, int* outAxis)
{
	vec3		dir, pn, pt;
	trace_t		t;
	g_qeglobals.d_camera.PointToRay(x, y, dir);
	t = Test_Ray(g_qeglobals.d_camera.origin, dir, SF_NOFIXEDSIZE | SF_SELECTED_ONLY);
	if (t.brush && t.face)
	{
		pn = t.face->plane.normal;
		AxializeVector(pn);

		if (pn[0])
			*outAxis = YZ;
		else if (pn[1])
			*outAxis = XZ;
		else
			*outAxis = XY;

		pt = g_qeglobals.d_camera.origin + t.dist * dir;
		SnapToPoint(pt);
		ProjectOnPlane(t.face->plane.normal, t.face->plane.dist, pn, pt);
		out = pt;
		return true;
	}
	return false;
}

/*
==================
Clip_CamDropPoint
==================
*/
void Clip_CamDropPoint(int x, int y)
{
	int		nAxis;
	vec3	pt;

	if (!Clip_CamPointOnSelection(x, y, pt, &nAxis))
		return;

	if (g_pcpMovingClip && GetCapture() == g_qeglobals.d_hwndCamera)
	{
		g_pcpMovingClip->ptClip = pt;
	}
	else
	{
		if (g_cpClip1.bSet == false)
			g_nClipAxis = nAxis;

		clippoint_t		*pPt;
		pPt = Clip_StartNextPoint();
		pPt->ptClip = pt;
	}
	Clip_PointsUpdated();
	Sys_UpdateWindows(W_XY | W_CAMERA);
}

/*
==================
Clip_CamGetNearestClipPoint
==================
*/
clippoint_t *Clip_CamGetNearestClipPoint(int x, int y)
{
	vec3 dir, cPt;
	g_qeglobals.d_camera.PointToRay(x, y, dir);

	if (g_cpClip1.bSet)
	{
		cPt = g_cpClip1.ptClip - g_qeglobals.d_camera.origin;
		VectorNormalize(cPt);

		if (DotProduct(cPt, dir) > CLIP_CAMDOT)
			return &g_cpClip1;
	}
	if (g_cpClip2.bSet)
	{
		cPt = g_cpClip2.ptClip - g_qeglobals.d_camera.origin;
		VectorNormalize(cPt);

		if (DotProduct(cPt, dir) > CLIP_CAMDOT)
			return &g_cpClip2;
	}
	if (g_cpClip3.bSet)
	{
		cPt = g_cpClip3.ptClip - g_qeglobals.d_camera.origin;
		VectorNormalize(cPt);

		if (DotProduct(cPt, dir) > CLIP_CAMDOT)
			return &g_cpClip3;
	}
	return NULL;
}

/*
==================
Clip_CamMovePoint
==================
*/
void Clip_CamMovePoint(int x, int y)
{
	int junk;
	bool	bCrossHair = false;

	// lunaran TODO: don't use windows mouse capture status for control flow maybe
	if (g_pcpMovingClip && GetCapture() == g_qeglobals.d_hwndCamera)
	{
		// lunaran - grid view reunification
		if (Clip_CamPointOnSelection(x, y, g_pcpMovingClip->ptClip, &junk))
		{
			Clip_PointsUpdated();
			Sys_UpdateWindows(W_XY | W_CAMERA);
			bCrossHair = true;
		}
	}
	else
	{
		g_pcpMovingClip = Clip_CamGetNearestClipPoint(x, y);
		if (g_pcpMovingClip)
			bCrossHair = true;
	}

	Clip_Crosshair(bCrossHair);
}

/*
==================
Clip_CamEndPoint
==================
*/
void Clip_CamEndPoint()
{
	if (g_pcpMovingClip && GetCapture() == g_qeglobals.d_hwndCamera)
	{
		g_pcpMovingClip = NULL;
		ReleaseCapture();
		Clip_PointsUpdated();
	}
	Sys_UpdateWindows(W_XY | W_CAMERA);
}

/*
============================================================================

	CLIP POINT 2D

============================================================================
*/


/*
==============
Clip_StartQuickClip

lunaran: puts point 1 and 2 in the same place and immediately begins dragging point 2
==============
*/
void Clip_StartQuickClip(XYZView* xyz, int x, int y)
{
	Clip_ResetMode();

	Clip_DropPoint(xyz, x, y);
	Clip_DropPoint(xyz, x, y);
	g_pcpMovingClip = &g_cpClip2;

}

/*
==============
Clip_EndQuickClip

lunaran: stop dragging the current point and immediately clip with it
==============
*/
void Clip_EndQuickClip()
{
	Clip_EndPoint();
	Clip_Clip();
}

/*
==============
Clip_DropPoint
==============
*/
void Clip_DropPoint (XYZView* xyz, int x, int y)
{
	int nDim;

	g_nClipAxis = xyz->dViewType;

	if (g_pcpMovingClip)  // <-- when a new click is issued on an existing point
	{
		// lunaran - grid view reunification
		if (xyz == XYZWnd_WinFromHandle(GetCapture()))
			xyz->SnapToPoint(x, y, g_pcpMovingClip->ptClip);
	}
	else // <-- if a new point is dropped in space
	{
		clippoint_t		*pPt;
		pPt = Clip_StartNextPoint();

		// lunaran - grid view reunification
		if (xyz == XYZWnd_WinFromHandle(GetCapture()))
			xyz->SnapToPoint(x, y, pPt->ptClip);

		// third coordinates for clip point: use d_v3WorkMax
		nDim = (xyz->dViewType == YZ ) ? 0 : ((xyz->dViewType == XZ) ? 1 : 2);

		// lunaran: put third clip at work min instead of work max so they aren't all coplanar to the view
		if (g_cpClip3.bSet)
			(pPt->ptClip)[nDim] = g_qeglobals.d_v3WorkMin[nDim];
		else
			(pPt->ptClip)[nDim] = g_qeglobals.d_v3WorkMax[nDim];
	}
	Clip_PointsUpdated();
	Sys_UpdateWindows(W_XY| W_CAMERA);
}

/*
==============
Clip_GetCoordXY
==============
*/
void Clip_GetCoordXY(int x, int y, vec3 &pt)
{
	int nDim;
	// lunaran - grid view reunification
	XYZView* xyz = XYZWnd_WinFromHandle(GetCapture());
	if (xyz)
		xyz->SnapToPoint(x, y, pt);
	nDim = (xyz->dViewType == YZ) ? 0 : ((xyz->dViewType == XZ) ? 1 : 2);

	pt[nDim] = g_qeglobals.d_v3WorkMax[nDim];
}

/*
==============
Clip_XYGetNearestClipPoint
==============
*/
clippoint_t* Clip_XYGetNearestClipPoint(XYZView* xyz, int x, int y)
{
	int nDim1, nDim2;
	vec3	tdp;

	tdp[0] = tdp[1] = tdp[2] = 256;

	xyz->SnapToPoint(x, y, tdp);
	nDim1 = (xyz->dViewType == YZ) ? 1 : 0;
	nDim2 = (xyz->dViewType == XY) ? 1 : 2;

	// lunaran: based on screen distance and not world distance
	float margin = 10.0f / xyz->scale;
	if (g_cpClip1.bSet)
	{
		if (fabs(g_cpClip1.ptClip[nDim1] - tdp[nDim1]) < margin &&
			fabs(g_cpClip1.ptClip[nDim2] - tdp[nDim2]) < margin)
		{
			return &g_cpClip1;
		}
	}
	if (g_cpClip2.bSet)
	{
		if (fabs(g_cpClip2.ptClip[nDim1] - tdp[nDim1]) < margin &&
			fabs(g_cpClip2.ptClip[nDim2] - tdp[nDim2]) < margin)
		{
			return &g_cpClip2;
		}
	}
	if (g_cpClip3.bSet)
	{
		if (fabs(g_cpClip3.ptClip[nDim1] - tdp[nDim1]) < margin &&
			fabs(g_cpClip3.ptClip[nDim2] - tdp[nDim2]) < margin)
		{
			return &g_cpClip3;
		}
	}
	return NULL;
}

/*
==============
Clip_MovePoint
should be named Clip_MoveMouse
==============
*/
void Clip_MovePoint (XYZView* xyz, int x, int y)
{
	bool	bCrossHair = false;

	// lunaran TODO: don't use windows mouse capture status for control flow maybe
	if (g_pcpMovingClip && GetCapture())
	{
		// lunaran - grid view reunification
		if (xyz == XYZWnd_WinFromHandle(GetCapture()))
			xyz->SnapToPoint(x, y, g_pcpMovingClip->ptClip);
		bCrossHair = true;
		Clip_PointsUpdated();
		Sys_UpdateWindows(W_XY | W_CAMERA);
	}
	else
	{
		g_pcpMovingClip = Clip_XYGetNearestClipPoint(xyz, x, y);
		if (g_pcpMovingClip)
			bCrossHair = true;
	}

	Clip_Crosshair(bCrossHair);

}

/*
==============
Clip_EndPoint
==============
*/
void Clip_EndPoint ()
{
	if (g_pcpMovingClip && GetCapture())
	{
		XYZView* xyz = XYZWnd_WinFromHandle(GetCapture());
		if (xyz)
			g_nClipAxis = xyz->dViewType;
		else
			g_nClipAxis = XY;

		g_pcpMovingClip = NULL;
		ReleaseCapture();
	}
	Sys_UpdateWindows(W_XY | W_CAMERA);
}

/*
==============
Clip_Draw
==============
*/
void Clip_Draw ()
{
	if (!g_qeglobals.d_bClipMode || !g_pcmdBC)
		return;
	
	char		strMsg[4];
	Brush		*pBrush;
	Face		*face;
	winding_t	*w;
	int			order;
	int			i;

	glPointSize(4);
	glColor3fv(&g_qeglobals.d_savedinfo.v3Colors[COLOR_CLIPPER].r);
	glBegin(GL_POINTS);

	if (g_cpClip1.bSet)
		glVertex3fv(&g_cpClip1.ptClip.x);
	if (g_cpClip2.bSet)
		glVertex3fv(&g_cpClip2.ptClip.x);
	if (g_cpClip3.bSet)
		glVertex3fv(&g_cpClip3.ptClip.x);

	glEnd();
	glPointSize(1);
     
	if (g_cpClip1.bSet)
	{
		glRasterPos3f(g_cpClip1.ptClip[0] + 2, g_cpClip1.ptClip[1] + 2, g_cpClip1.ptClip[2] + 2);
		strcpy(strMsg, "1");
		glCallLists(strlen(strMsg), GL_UNSIGNED_BYTE, strMsg);
	}
	if (g_cpClip2.bSet)
	{
		glRasterPos3f(g_cpClip2.ptClip[0] + 2, g_cpClip2.ptClip[1] + 2, g_cpClip2.ptClip[2] + 2);
		strcpy(strMsg, "2");
		glCallLists(strlen(strMsg), GL_UNSIGNED_BYTE, strMsg);
	}
	if (g_cpClip3.bSet)
	{
		glRasterPos3f(g_cpClip3.ptClip[0] + 2, g_cpClip3.ptClip[1] + 2, g_cpClip3.ptClip[2] + 2);
		strcpy(strMsg, "3");
		glCallLists(strlen(strMsg), GL_UNSIGNED_BYTE, strMsg);
	}
	if (g_cpClip1.bSet && g_cpClip2.bSet)
	{
		std::vector<Brush*> *brList = (d_bClipSwitch) ? &g_pcmdBC->brBack : &g_pcmdBC->brFront;

		for (auto brIt = brList->begin(); brIt != brList->end(); ++brIt)
		{
			pBrush = *brIt;
			glColor3f(1, 1, 0);
			for (face = pBrush->basis.faces, order = 0; face; face = face->fnext, order++)
			{
				w = face->face_winding;
				if (!w)
					continue;
				// draw the polygon
				glBegin(GL_LINE_LOOP);
				for (i = 0; i < w->numpoints; i++)
					glVertex3fv(&w->points[i].point.x);
				glEnd();
			}
		}
	}
}


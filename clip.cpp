//==============================
//	clip.c
//==============================

#include "qe3.h"

#define CLIP_CAMDOT 0.9996f

clippoint_t		g_cpClip1;
clippoint_t		g_cpClip2;
clippoint_t		g_cpClip3;
clippoint_t	   *g_pcpMovingClip;

// lunaran - let clip tool store the 2-point axis projection rather than derive it in various places
int				g_nClipAxis;



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
		g_cpClip1.ptClip[0] = g_cpClip1.ptClip[1] = g_cpClip1.ptClip[2] = 0.0;
		g_cpClip1.bSet = false;
		g_cpClip2.ptClip[0] = g_cpClip2.ptClip[1] = g_cpClip2.ptClip[2] = 0.0;
		g_cpClip2.bSet = false;
		g_cpClip3.ptClip[0] = g_cpClip3.ptClip[1] = g_cpClip3.ptClip[2] = 0.0;
		g_cpClip3.bSet = false;

		Brush::FreeList(&g_qeglobals.d_brFrontSplits);
		Brush::FreeList(&g_qeglobals.d_brBackSplits);
	
		g_qeglobals.d_brFrontSplits.next = &g_qeglobals.d_brFrontSplits;
		g_qeglobals.d_brBackSplits.next = &g_qeglobals.d_brBackSplits;
	}
	else
	{
		if (g_pcpMovingClip)
		{
			ReleaseCapture();
			g_pcpMovingClip = NULL;
		}
		Brush::FreeList(&g_qeglobals.d_brFrontSplits);
		Brush::FreeList(&g_qeglobals.d_brBackSplits);
	
	    g_qeglobals.d_brFrontSplits.next = &g_qeglobals.d_brFrontSplits;
		g_qeglobals.d_brBackSplits.next = &g_qeglobals.d_brBackSplits;

		Sys_UpdateWindows(W_ALL);
	}
}

/*
==================
Clip_ProduceSplitLists
==================
*/
void Clip_ProduceSplitLists ()
{
	Brush	   *pBrush;
	Brush	   *pFront;
	Brush	   *pBack;
	Face		face;

	Brush::FreeList(&g_qeglobals.d_brFrontSplits);
	Brush::FreeList(&g_qeglobals.d_brBackSplits);

	g_qeglobals.d_brFrontSplits.next = &g_qeglobals.d_brFrontSplits;
	g_qeglobals.d_brBackSplits.next = &g_qeglobals.d_brBackSplits;

	for (pBrush = g_brSelectedBrushes.next; pBrush != NULL && pBrush != &g_brSelectedBrushes; pBrush = pBrush->next)
	{
		pFront = NULL;
		pBack = NULL;
    
		if (g_cpClip1.bSet && g_cpClip2.bSet)
		{
			VectorCopy(g_cpClip1.ptClip, face.planepts[0]);
			VectorCopy(g_cpClip2.ptClip, face.planepts[1]);
			VectorCopy(g_cpClip3.ptClip, face.planepts[2]);

			if (!g_cpClip3.bSet)
			{
				if (g_nClipAxis == YZ)
				{
					face.planepts[0][0] = pBrush->basis.mins[0];
					face.planepts[1][0] = pBrush->basis.mins[0];
					face.planepts[2][1] = (g_cpClip1.ptClip[1] + g_cpClip2.ptClip[1]) * 0.5f;
					face.planepts[2][2] = (g_cpClip1.ptClip[2] + g_cpClip2.ptClip[2]) * 0.5f;
					face.planepts[2][0] = pBrush->basis.maxs[0];
				}
				else if (g_nClipAxis == XZ)
				{
					face.planepts[0][1] = pBrush->basis.mins[1];
					face.planepts[1][1] = pBrush->basis.mins[1];
					face.planepts[2][0] = (g_cpClip1.ptClip[0] + g_cpClip2.ptClip[0]) * 0.5f;
					face.planepts[2][2] = (g_cpClip1.ptClip[2] + g_cpClip2.ptClip[2]) * 0.5f;
					face.planepts[2][1] = pBrush->basis.maxs[1];
				}
				else
				{
					face.planepts[0][2] = pBrush->basis.mins[2];
					face.planepts[1][2] = pBrush->basis.mins[2];
					face.planepts[2][0] = (g_cpClip1.ptClip[0] + g_cpClip2.ptClip[0]) * 0.5f;
					face.planepts[2][1] = (g_cpClip1.ptClip[1] + g_cpClip2.ptClip[1]) * 0.5f;
					face.planepts[2][2] = pBrush->basis.maxs[2];
				}
			}

			CSG_SplitBrushByFace(pBrush, &face, &pFront, &pBack);
			if (pBack)
				pBack->AddToList(&g_qeglobals.d_brBackSplits);
			if (pFront)
				pFront->AddToList(&g_qeglobals.d_brFrontSplits);
		}
	}
}



/*
==================
Clip_Clip
==================
*/
void Clip_Clip ()
{
	Brush	   *pList;
	Brush		hold_brushes;

	if (!g_qeglobals.d_bClipMode)
		return;

	Undo::Start("Clip Selected");
	Undo::AddBrushList(&g_brSelectedBrushes);

	hold_brushes.next = hold_brushes.prev = &hold_brushes;
	Clip_ProduceSplitLists();

	pList = (g_qeglobals.d_bClipSwitch) ? &g_qeglobals.d_brFrontSplits : &g_qeglobals.d_brBackSplits;
    
	if (pList->next != pList)
	{
		Brush::CopyList(pList, &hold_brushes);
		Brush::FreeList(&g_qeglobals.d_brFrontSplits);
		Brush::FreeList(&g_qeglobals.d_brBackSplits);
		Select_Delete();
		Brush::CopyList(&hold_brushes, &g_brSelectedBrushes);
	}
	Clip_ResetMode();

	Undo::EndBrushList(&g_brSelectedBrushes);
	Undo::End();

	Sys_UpdateWindows(W_ALL);
}

/*
==================
Clip_Split
==================
*/
void Clip_Split ()
{
	if (!g_qeglobals.d_bClipMode)
		return;

	Undo::Start("Split Selected");
	Undo::AddBrushList(&g_brSelectedBrushes);

	Clip_ProduceSplitLists();
	if ((g_qeglobals.d_brFrontSplits.next != &g_qeglobals.d_brFrontSplits) &&
		(g_qeglobals.d_brBackSplits.next != &g_qeglobals.d_brBackSplits))
	{
		Select_Delete();
		Brush::CopyList(&g_qeglobals.d_brFrontSplits, &g_brSelectedBrushes);
		Brush::CopyList(&g_qeglobals.d_brBackSplits, &g_brSelectedBrushes);
		Brush::FreeList(&g_qeglobals.d_brFrontSplits);
		Brush::FreeList(&g_qeglobals.d_brBackSplits);
	}
	Clip_ResetMode();

	Undo::EndBrushList(&g_brSelectedBrushes);
	Undo::End();
	Sys_UpdateWindows(W_ALL);
}

/*
==================
Clip_Flip
==================
*/
void Clip_Flip ()
{
	if (g_qeglobals.d_bClipMode)
	{
		g_qeglobals.d_bClipSwitch = !g_qeglobals.d_bClipSwitch;
		Sys_UpdateWindows(W_ALL);
	}
}

/*
==================
Clip_StartNextPoint
==================
*/
clippoint_t* Clip_StartNextPoint()
{
	clippoint_t *pt;
	pt = NULL;

	if (g_cpClip1.bSet == false)
	{
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
void SnapToPoint(vec3_t point)
{
	for (int i = 0; i < 3; i++)
		point[i] = floor(point[i] / g_qeglobals.d_nGridSize + 0.5) * g_qeglobals.d_nGridSize;
}

/*
==================
Clip_CamPointOnSelection
==================
*/
void Clip_CamPointOnSelection(int x, int y, vec3_t out, int* outAxis)
{
	vec3_t		dir, pn, pt;
	trace_t		t;
	g_qeglobals.d_camera.PointToRay(x, y, dir);
	t = Test_Ray(g_qeglobals.d_camera.origin, dir, SF_NOFIXEDSIZE | SF_SELECTED_ONLY);
	if (t.brush && t.face)
	{
		VectorCopy(t.face->plane.normal, pn);
		AxializeVector(pn);

		if (pn[0])
			*outAxis = YZ;
		else if (pn[1])
			*outAxis = XZ;
		else
			*outAxis = XY;

		VectorMA(g_qeglobals.d_camera.origin, t.dist, dir, pt);
		SnapToPoint(pt);
		ProjectOnPlane(t.face->plane.normal, t.face->plane.dist, pn, pt);
		VectorCopy(pt, out);
	}
}

/*
==================
Clip_CamDropPoint
==================
*/
void Clip_CamDropPoint(int x, int y)
{
	int		nAxis;
	vec3_t	pt;
	Clip_CamPointOnSelection(x, y, pt, &nAxis);

	if (g_pcpMovingClip && GetCapture() == g_qeglobals.d_hwndCamera)
	{
		VectorCopy(pt, g_pcpMovingClip->ptClip);
	}
	else
	{
		if (g_cpClip1.bSet == false)
			g_nClipAxis = nAxis;

		clippoint_t		*pPt;
		pPt = Clip_StartNextPoint();
		VectorCopy(pt, pPt->ptClip);
	}
	Sys_UpdateWindows(W_XY | W_CAMERA);
}

/*
==================
Clip_CamGetNearestClipPoint
==================
*/
clippoint_t *Clip_CamGetNearestClipPoint(int x, int y)
{
	vec3_t dir, cPt;
	g_qeglobals.d_camera.PointToRay(x, y, dir);

	if (g_cpClip1.bSet)
	{
		VectorSubtract(g_cpClip1.ptClip, g_qeglobals.d_camera.origin, cPt);
		VectorNormalize(cPt);

		if (DotProduct(cPt, dir) > CLIP_CAMDOT)
			return &g_cpClip1;
	}
	if (g_cpClip2.bSet)
	{
		VectorSubtract(g_cpClip2.ptClip, g_qeglobals.d_camera.origin, cPt);
		VectorNormalize(cPt);

		if (DotProduct(cPt, dir) > CLIP_CAMDOT)
			return &g_cpClip2;
	}
	if (g_cpClip3.bSet)
	{
		VectorSubtract(g_cpClip3.ptClip, g_qeglobals.d_camera.origin, cPt);
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
		Clip_CamPointOnSelection(x, y, g_pcpMovingClip->ptClip, &junk);
		bCrossHair = true;
	}
	else
	{
		g_pcpMovingClip = Clip_CamGetNearestClipPoint(x, y);
		if (g_pcpMovingClip)
			bCrossHair = true;
	}

	Clip_Crosshair(bCrossHair);
	Sys_UpdateWindows(W_XY | W_CAMERA);
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
	Sys_UpdateWindows(W_XY| W_CAMERA);
}

/*
==============
Clip_GetCoordXY
==============
*/
void Clip_GetCoordXY(int x, int y, vec3_t pt)
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
	vec3_t	tdp;

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
	}
	else
	{
		g_pcpMovingClip = Clip_XYGetNearestClipPoint(xyz, x, y);
		if (g_pcpMovingClip)
			bCrossHair = true;
	}

	Clip_Crosshair(bCrossHair);

	Sys_UpdateWindows(W_XY | W_CAMERA);
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
Clip_DrawPoints
==============
*/
void Clip_DrawPoints ()
{
	if (!g_qeglobals.d_bClipMode)
		return;
	
	char		strMsg[8];
	Brush    *pBrush;
	Brush    *pList;
	Face	   *face;
	winding_t  *w;
	int			order;
	int			i;

	glPointSize(4);
	glColor3fv(g_qeglobals.d_savedinfo.v3Colors[COLOR_CLIPPER]);
	glBegin(GL_POINTS);

	if (g_cpClip1.bSet)
		glVertex3fv(g_cpClip1.ptClip);
	if (g_cpClip2.bSet)
		glVertex3fv(g_cpClip2.ptClip);
	if (g_cpClip3.bSet)
		glVertex3fv(g_cpClip3.ptClip);

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
		Clip_ProduceSplitLists();
		pList = (g_qeglobals.d_bClipSwitch) ? &g_qeglobals.d_brFrontSplits : &g_qeglobals.d_brBackSplits;

		for (pBrush = pList->next; pBrush != NULL && pBrush != pList; pBrush = pBrush->next)
		{
			glColor3f(1, 1, 0);
			for (face = pBrush->basis.faces, order = 0; face; face = face->next, order++)
			{
				w = face->face_winding;
				if (!w)
					continue;
				// draw the polygon
				glBegin(GL_LINE_LOOP);
				for (i = 0; i < w->numpoints; i++)
					glVertex3fv(w->points[i]);
				glEnd();
			}
		}
	}
}


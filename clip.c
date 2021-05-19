//==============================
//	clip.c
//==============================

#include "qe3.h"


clippoint_t		g_cpClip1;
clippoint_t		g_cpClip2;
clippoint_t		g_cpClip3;
clippoint_t	   *g_pcpMovingClip;

// lunaran - let clip tool store the 2-point axis projection rather than derive it in various places
int				g_nClipAxis;


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

		Brush_CleanList(&g_qeglobals.d_brFrontSplits);
		Brush_CleanList(&g_qeglobals.d_brBackSplits);
	
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
		Brush_CleanList(&g_qeglobals.d_brFrontSplits);
		Brush_CleanList(&g_qeglobals.d_brBackSplits);
	
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
	brush_t	   *pBrush;
	brush_t	   *pFront;
	brush_t	   *pBack;
	face_t		face;

	if (GetCapture() == g_qeglobals.d_hwndXY)
		g_qeglobals.d_nLastActiveXY = XY;
	if (GetCapture() == g_qeglobals.d_hwndXZ)
		g_qeglobals.d_nLastActiveXY = XZ;
	if (GetCapture() == g_qeglobals.d_hwndYZ)
		g_qeglobals.d_nLastActiveXY = YZ;

	Brush_CleanList(&g_qeglobals.d_brFrontSplits);
	Brush_CleanList(&g_qeglobals.d_brBackSplits);

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

			if (g_cpClip3.bSet == false)
			{
				if (g_qeglobals.d_nLastActiveXY == XY)
				{
					if (g_qeglobals.d_nViewType == XY)
					{
						face.planepts[0][2] = pBrush->mins[2];
						face.planepts[1][2] = pBrush->mins[2];
						face.planepts[2][0] = Betwixt(g_cpClip1.ptClip[0], g_cpClip2.ptClip[0]);
						face.planepts[2][1] = Betwixt(g_cpClip1.ptClip[1], g_cpClip2.ptClip[1]);
						face.planepts[2][2] = pBrush->maxs[2];
					}
					else if (g_qeglobals.d_nViewType == YZ)
					{
						face.planepts[0][0] = pBrush->mins[0];
						face.planepts[1][0] = pBrush->mins[0];
						face.planepts[2][1] = Betwixt(g_cpClip1.ptClip[1], g_cpClip2.ptClip[1]);
						face.planepts[2][2] = Betwixt(g_cpClip1.ptClip[2], g_cpClip2.ptClip[2]);
						face.planepts[2][0] = pBrush->maxs[0];
					}
					else
					{
						face.planepts[0][1] = pBrush->mins[1];
						face.planepts[1][1] = pBrush->mins[1];
						face.planepts[2][0] = Betwixt(g_cpClip1.ptClip[0], g_cpClip2.ptClip[0]);
						face.planepts[2][2] = Betwixt(g_cpClip1.ptClip[2], g_cpClip2.ptClip[2]);
						face.planepts[2][1] = pBrush->maxs[1];
					}
				}
// sikk---> Multiple Orthographic Views
				if (g_qeglobals.d_nLastActiveXY == XZ)
				{
					face.planepts[0][1] = pBrush->mins[1];
					face.planepts[1][1] = pBrush->mins[1];
					face.planepts[2][0] = Betwixt(g_cpClip1.ptClip[0], g_cpClip2.ptClip[0]);
					face.planepts[2][2] = Betwixt(g_cpClip1.ptClip[2], g_cpClip2.ptClip[2]);
					face.planepts[2][1] = pBrush->maxs[1];
				}
				if (g_qeglobals.d_nLastActiveXY == YZ)
				{
					face.planepts[0][0] = pBrush->mins[0];
					face.planepts[1][0] = pBrush->mins[0];
					face.planepts[2][1] = Betwixt(g_cpClip1.ptClip[1], g_cpClip2.ptClip[1]);
					face.planepts[2][2] = Betwixt(g_cpClip1.ptClip[2], g_cpClip2.ptClip[2]);
					face.planepts[2][0] = pBrush->maxs[0];
				}
// <---sikk
			}
			CSG_SplitBrushByFace(pBrush, &face, &pFront, &pBack);
			if (pBack)
				Brush_AddToList(pBack, &g_qeglobals.d_brBackSplits);
			if (pFront)
				Brush_AddToList(pFront, &g_qeglobals.d_brFrontSplits);
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
	brush_t	   *pList;
	brush_t		hold_brushes;

	if (!g_qeglobals.d_bClipMode)
		return;

	Undo_Start("Clip Selected");
	Undo_AddBrushList(&g_brSelectedBrushes);

	hold_brushes.next = &hold_brushes;
	Clip_ProduceSplitLists();

	pList = ((g_qeglobals.d_nViewType == XZ) ? !g_qeglobals.d_bClipSwitch : g_qeglobals.d_bClipSwitch) ? &g_qeglobals.d_brFrontSplits : &g_qeglobals.d_brBackSplits;
    
	if (pList->next != pList)
	{
		Brush_CopyList(pList, &hold_brushes);
		Brush_CleanList(&g_qeglobals.d_brFrontSplits);
		Brush_CleanList(&g_qeglobals.d_brBackSplits);
		Select_Delete();
		Brush_CopyList(&hold_brushes, &g_brSelectedBrushes);
	}
	Clip_ResetMode();

	Undo_EndBrushList(&g_brSelectedBrushes);
	Undo_End();

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

	Undo_Start("Split Selected");
	Undo_AddBrushList(&g_brSelectedBrushes);

	Clip_ProduceSplitLists();
	if ((g_qeglobals.d_brFrontSplits.next != &g_qeglobals.d_brFrontSplits) &&
		(g_qeglobals.d_brBackSplits.next != &g_qeglobals.d_brBackSplits))
	{
		Select_Delete();
		Brush_CopyList(&g_qeglobals.d_brFrontSplits, &g_brSelectedBrushes);
		Brush_CopyList(&g_qeglobals.d_brBackSplits, &g_brSelectedBrushes);
		Brush_CleanList(&g_qeglobals.d_brFrontSplits);
		Brush_CleanList(&g_qeglobals.d_brBackSplits);
	}
	Clip_ResetMode();

	Undo_EndBrushList(&g_brSelectedBrushes);
	Undo_End();
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
============================================================================

	CLIP POINT

============================================================================
*/

/*
==============
Clip_StartQuickClip

lunaran: puts point 1 and 2 in the same place and immediately begins dragging point 2
==============
*/
void Clip_StartQuickClip(int x, int y)
{
	Clip_ResetMode();

	Clip_DropPoint(x, y);
	Clip_DropPoint(x, y);
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
void Clip_DropPoint (int x, int y)
{
	clippoint_t    *pPt;
	int				nViewType;
	int				nDim;

	if (g_pcpMovingClip)  // <-- when a new click is issued on an existing point
	{
// sikk---> Multiple Orthographic Views
		if (GetCapture() == g_qeglobals.d_hwndXY)
			XY_SnapToPoint(x, y, g_pcpMovingClip->ptClip);
		if (GetCapture() == g_qeglobals.d_hwndXZ)
			XZ_SnapToPoint(x, y, g_pcpMovingClip->ptClip);
		if (GetCapture() == g_qeglobals.d_hwndYZ)
			YZ_SnapToPoint(x, y, g_pcpMovingClip->ptClip);
// <---sikk
	}
	else // <-- if a new point is dropped in space
	{
		pPt = NULL;

		if (g_cpClip1.bSet == false)
		{
			pPt = &g_cpClip1;
			g_cpClip1.bSet = true;
		}
		else if (g_cpClip2.bSet == false)
		{
			pPt = &g_cpClip2;
			g_cpClip2.bSet = true;
		}
		else if (g_cpClip3.bSet == false)
		{
			pPt = &g_cpClip3;
			g_cpClip3.bSet = true;
		}
		else 
		{
			Clip_ResetMode();
			pPt = &g_cpClip1;
			g_cpClip1.bSet = true;
		}

// sikk---> Multiple Orthographic Views
		if (GetCapture() == g_qeglobals.d_hwndXY)
		{
			XY_SnapToPoint(x, y, pPt->ptClip);
			// third coordinates for clip point: use d_v3WorkMax
			// cf VIEWTYPE defintion: enum VIEWTYPE {YZ, XZ, XY};
			nViewType = g_qeglobals.d_nViewType;
			nDim = (nViewType == YZ ) ? 0 : ((nViewType == XZ) ? 1 : 2);
		}
		if (GetCapture() == g_qeglobals.d_hwndXZ)
		{
			XZ_SnapToPoint(x, y, pPt->ptClip);
			nDim = 1;
		}
		if (GetCapture() == g_qeglobals.d_hwndYZ)
		{
			YZ_SnapToPoint(x, y, pPt->ptClip);
			nDim = 0;
		}
// <---sikk
		if (g_cpClip3.bSet)
			(pPt->ptClip)[nDim] = g_qeglobals.d_v3WorkMin[nDim];
		else
			(pPt->ptClip)[nDim] = g_qeglobals.d_v3WorkMax[nDim];
	}
	Sys_UpdateWindows(W_XY| W_Z);
}

void Clip_GetCoordXY(int x, int y, vec3_t pt)
{
	int nDim;
	// sikk---> Multiple Orthographic Views
	if (GetCapture() == g_qeglobals.d_hwndXY)
	{
		XY_SnapToPoint(x, y, pt);
		// third coordinates for clip point: use d_v3WorkMax
		// cf VIEWTYPE defintion: enum VIEWTYPE {YZ, XZ, XY};
		nDim = (g_qeglobals.d_nViewType == YZ) ? 0 : ((g_qeglobals.d_nViewType == XZ) ? 1 : 2);
	}
	if (GetCapture() == g_qeglobals.d_hwndXZ)
	{
		XZ_SnapToPoint(x, y, pt);
		nDim = 1;
	}
	if (GetCapture() == g_qeglobals.d_hwndYZ)
	{
		YZ_SnapToPoint(x, y, pt);
		nDim = 0;
	}
	// <---sikk				
	pt[nDim] = g_qeglobals.d_v3WorkMax[nDim];
}

clippoint_t* Clip_GetNearestClipPointXY(int x, int y, int nView)
{
	int nDim1, nDim2;
	vec3_t	tdp;

	tdp[0] = tdp[1] = tdp[2] = 256;

	if (nView == XY)
	{
		XY_SnapToPoint(x, y, tdp);
		nDim1 = (g_qeglobals.d_nViewType == YZ) ? 1 : 0;
		nDim2 = (g_qeglobals.d_nViewType == XY) ? 1 : 2;
	}
	if (nView == XZ)
	{
		XZ_SnapToPoint(x, y, tdp);
		nDim1 = 0;
		nDim2 = 2;
	}
	if (nView == YZ)
	{
		YZ_SnapToPoint(x, y, tdp);
		nDim1 = 1;
		nDim2 = 2;
	}

	// lunaran: based on screen distance and not world distance
	float margin = 10.0f / g_qeglobals.d_xyz.scale;
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
void Clip_MovePoint (int x, int y, int nView)
{
	int		nDim1, nDim2;
	bool	bCrossHair = false;

	// lunaran TODO: don't use windows mouse capture status for control flow
	if (g_pcpMovingClip && GetCapture())
	{
		if (GetCapture() == g_qeglobals.d_hwndXY)
		{
			XY_SnapToPoint(x, y, g_pcpMovingClip->ptClip);
		}
		else if (GetCapture() == g_qeglobals.d_hwndXZ)
		{
			XZ_SnapToPoint(x, y, g_pcpMovingClip->ptClip);
		}
		else if (GetCapture() == g_qeglobals.d_hwndYZ)
		{
			YZ_SnapToPoint(x, y, g_pcpMovingClip->ptClip);
		}
		bCrossHair = true;
	}
	else
	{
		g_pcpMovingClip = Clip_GetNearestClipPointXY(x, y, nView);
		if (g_pcpMovingClip)
			bCrossHair = true;
	}

	if (bCrossHair)
		SetCursor(LoadCursor(NULL, IDC_CROSS));
	else
		SetCursor(LoadCursor(NULL, IDC_ARROW));

	Sys_UpdateWindows(W_XY);
}

/*
==============
Clip_DrawPoint
==============
*/
void Clip_DrawPoint (int nView)
{
	char		strMsg[128];
	brush_t    *pBrush;
	brush_t    *pList;
	face_t	   *face;
	winding_t  *w;
	int			order;
	int			i;
	
	if (nView == XY)	// sikk - Multiple Orthographic Views
	{
		if (g_qeglobals.d_nViewType != XY)
		{
			glPushMatrix();
			if (g_qeglobals.d_nViewType == YZ)
				glRotatef(-90,  0, 1, 0);	// put Z going up
			glRotatef(-90,  1, 0, 0);		// put Z going up
		}
	}
// sikk---> Multiple Orthographic Views
	if (nView == XZ)
	{
		glPushMatrix();
		glRotatef(-90,  1, 0, 0);	// put Z going up
	}
	if (nView == YZ)
	{
		glPushMatrix();
		glRotatef(-90,  0, 1, 0);	// put Z going up
		glRotatef(-90,  1, 0, 0);	// put Z going up
	}
// <---sikk

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
		pList = ((g_qeglobals.d_nViewType == XZ) ? !g_qeglobals.d_bClipSwitch : g_qeglobals.d_bClipSwitch) ? &g_qeglobals.d_brFrontSplits : &g_qeglobals.d_brBackSplits;

		for (pBrush = pList->next; pBrush != NULL && pBrush != pList; pBrush = pBrush->next)
		{
			glColor3f(1, 1, 0);
			for (face = pBrush->brush_faces, order = 0; face; face = face->next, order++)
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
	if (g_qeglobals.d_nViewType != XY || nView == XZ || nView == YZ)	// sikk - Multiple Orthographic Views
		glPopMatrix();
}

/*
==============
Clip_EndPoint
==============
*/
void Clip_EndPoint ()
{
	if (g_pcpMovingClip)
	{
		if (GetCapture() == g_qeglobals.d_hwndXY)
			g_nClipAxis = XY;
		else if (GetCapture() == g_qeglobals.d_hwndXZ)
			g_nClipAxis = XZ;
		else if (GetCapture() == g_qeglobals.d_hwndYZ)
			g_nClipAxis = YZ;

		g_pcpMovingClip = NULL;
		ReleaseCapture();
	}
}

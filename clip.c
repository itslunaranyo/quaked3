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

	/*
	if (GetCapture() == g_qeglobals.d_hwndXYZ[0])
		g_qeglobals.d_nLastActiveXY = XY;
	if (GetCapture() == g_qeglobals.d_hwndXYZ[2])
		g_qeglobals.d_nLastActiveXY = XZ;
	if (GetCapture() == g_qeglobals.d_hwndXYZ[1])
		g_qeglobals.d_nLastActiveXY = YZ;
	*/

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

			if (!g_cpClip3.bSet)
			{
				if (g_nClipAxis == YZ)
				{
					face.planepts[0][0] = pBrush->mins[0];
					face.planepts[1][0] = pBrush->mins[0];
					face.planepts[2][1] = (g_cpClip1.ptClip[1] + g_cpClip2.ptClip[1]) * 0.5f;
					face.planepts[2][2] = (g_cpClip1.ptClip[2] + g_cpClip2.ptClip[2]) * 0.5f;
					face.planepts[2][0] = pBrush->maxs[0];
				}
				else if (g_nClipAxis == XZ)
				{
					face.planepts[0][1] = pBrush->mins[1];
					face.planepts[1][1] = pBrush->mins[1];
					face.planepts[2][0] = (g_cpClip1.ptClip[0] + g_cpClip2.ptClip[0]) * 0.5f;
					face.planepts[2][2] = (g_cpClip1.ptClip[2] + g_cpClip2.ptClip[2]) * 0.5f;
					face.planepts[2][1] = pBrush->maxs[1];
				}
				else
				{
					face.planepts[0][2] = pBrush->mins[2];
					face.planepts[1][2] = pBrush->mins[2];
					face.planepts[2][0] = (g_cpClip1.ptClip[0] + g_cpClip2.ptClip[0]) * 0.5f;
					face.planepts[2][1] = (g_cpClip1.ptClip[1] + g_cpClip2.ptClip[1]) * 0.5f;
					face.planepts[2][2] = pBrush->maxs[2];
				}
			}
			/*
			if (g_cpClip3.bSet == false)
			{
				if (g_qeglobals.d_nLastActiveXY == XY)
				{
					if (g_qeglobals.d_xyz[0].dViewType == XY)
					{
						face.planepts[0][2] = pBrush->mins[2];
						face.planepts[1][2] = pBrush->mins[2];
						face.planepts[2][0] = (g_cpClip1.ptClip[0] + g_cpClip2.ptClip[0]) * 0.5f;
						face.planepts[2][1] = (g_cpClip1.ptClip[1] + g_cpClip2.ptClip[1]) * 0.5f;
						face.planepts[2][2] = pBrush->maxs[2];
					}
					else if (g_qeglobals.d_xyz[0].dViewType == YZ)
					{
						face.planepts[0][0] = pBrush->mins[0];
						face.planepts[1][0] = pBrush->mins[0];
						face.planepts[2][1] = (g_cpClip1.ptClip[1] + g_cpClip2.ptClip[1]) * 0.5f;
						face.planepts[2][2] = (g_cpClip1.ptClip[2] + g_cpClip2.ptClip[2]) * 0.5f;
						face.planepts[2][0] = pBrush->maxs[0];
					}
					else
					{
						face.planepts[0][1] = pBrush->mins[1];
						face.planepts[1][1] = pBrush->mins[1];
						face.planepts[2][0] = (g_cpClip1.ptClip[0] + g_cpClip2.ptClip[0]) * 0.5f;
						face.planepts[2][2] = (g_cpClip1.ptClip[2] + g_cpClip2.ptClip[2]) * 0.5f;
						face.planepts[2][1] = pBrush->maxs[1];
					}
				}
// sikk---> Multiple Orthographic Views
				if (g_qeglobals.d_nLastActiveXY == XZ)
				{
					face.planepts[0][1] = pBrush->mins[1];
					face.planepts[1][1] = pBrush->mins[1];
					face.planepts[2][0] = (g_cpClip1.ptClip[0]+ g_cpClip2.ptClip[0]) * 0.5f;
					face.planepts[2][2] = (g_cpClip1.ptClip[2]+ g_cpClip2.ptClip[2]) * 0.5f;
					face.planepts[2][1] = pBrush->maxs[1];
				}
				if (g_qeglobals.d_nLastActiveXY == YZ)
				{
					face.planepts[0][0] = pBrush->mins[0];
					face.planepts[1][0] = pBrush->mins[0];
					face.planepts[2][1] = (g_cpClip1.ptClip[1]+ g_cpClip2.ptClip[1]) * 0.5f;
					face.planepts[2][2] = (g_cpClip1.ptClip[2]+ g_cpClip2.ptClip[2]) * 0.5f;
					face.planepts[2][0] = pBrush->maxs[0];
				}
// <---sikk
			}
			*/
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

	pList = (g_qeglobals.d_bClipSwitch) ? &g_qeglobals.d_brFrontSplits : &g_qeglobals.d_brBackSplits;
    
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
void Clip_StartQuickClip(xyz_t* xyz, int x, int y)
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
void Clip_DropPoint (xyz_t* xyz, int x, int y)
{
	clippoint_t    *pPt;
	int				nDim;

	g_nClipAxis = xyz->dViewType;

	if (g_pcpMovingClip)  // <-- when a new click is issued on an existing point
	{
		// lunaran - grid view reunification
		if (xyz == XYZWnd_WinFromHandle(GetCapture()))
			XYZ_SnapToPoint(xyz, x, y, g_pcpMovingClip->ptClip);
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

		// lunaran - grid view reunification
		if (xyz == XYZWnd_WinFromHandle(GetCapture()))
			XYZ_SnapToPoint(xyz, x, y, pPt->ptClip);
		// third coordinates for clip point: use d_v3WorkMax
		nDim = (xyz->dViewType == YZ ) ? 0 : ((xyz->dViewType == XZ) ? 1 : 2);

		// lunaran: put third clip at work min instead of work max so they aren't all coplanar to the view
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
	// lunaran - grid view reunification
	xyz_t* xyz = XYZWnd_WinFromHandle(GetCapture());
	if (xyz)
		XYZ_SnapToPoint(xyz, x, y, pt);
	nDim = (xyz->dViewType == YZ) ? 0 : ((xyz->dViewType == XZ) ? 1 : 2);

	pt[nDim] = g_qeglobals.d_v3WorkMax[nDim];
}

clippoint_t* Clip_GetNearestClipPointXY(xyz_t* xyz, int x, int y)
{
	int nDim1, nDim2;
	vec3_t	tdp;

	tdp[0] = tdp[1] = tdp[2] = 256;

	XYZ_SnapToPoint(xyz, x, y, tdp);
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
void Clip_MovePoint (xyz_t* xyz, int x, int y)
{
	bool	bCrossHair = false;

	// lunaran TODO: don't use windows mouse capture status for control flow maybe
	if (g_pcpMovingClip && GetCapture())
	{
		// lunaran - grid view reunification
		if (xyz == XYZWnd_WinFromHandle(GetCapture()))
			XYZ_SnapToPoint(xyz, x, y, g_pcpMovingClip->ptClip);
		bCrossHair = true;
	}
	else
	{
		g_pcpMovingClip = Clip_GetNearestClipPointXY(xyz, x, y);
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
void Clip_DrawPoint (xyz_t* xyz)
{
	char		strMsg[128];
	brush_t    *pBrush;
	brush_t    *pList;
	face_t	   *face;
	winding_t  *w;
	int			order;
	int			i;
	
	if (xyz->dViewType != XY)
	{
		glPushMatrix();
		if (xyz->dViewType == YZ)
			glRotatef(-90,  0, 1, 0);	// put Z going up
		glRotatef(-90,  1, 0, 0);		// put Z going up
	}

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
		pList = ((xyz->dViewType == XZ) ? !g_qeglobals.d_bClipSwitch : g_qeglobals.d_bClipSwitch) ? &g_qeglobals.d_brFrontSplits : &g_qeglobals.d_brBackSplits;

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
	if (xyz->dViewType != XY)
		glPopMatrix();
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
		xyz_t* xyz = XYZWnd_WinFromHandle(GetCapture());
		if (xyz)
			g_nClipAxis = xyz->dViewType;
		else
			g_nClipAxis = XY;

		g_pcpMovingClip = NULL;
		ReleaseCapture();
	}
}

//==============================
//	xy.c
//==============================

#include "qe3.h"


#define	PAGEFLIPS	2

const char *g_pszDimStrings[] = {"x:%.f", "y:%.f", "z:%.f"};
const char *g_pszOrgStrings[] = {"(x:%.f  y:%.f)", "(x:%.f  z:%.f)", "(y:%.f  z:%.f)"};

// sikk - Free Rotate & Free Scale
bool	g_bSnapCheck, g_bRotateCheck, g_bScaleCheck;
int		g_nRotate = 0;
	
/*
============
XY_Init
============
*/
void XY_Init ()
{
	g_qeglobals.d_xyz.origin[0] = 0;
	g_qeglobals.d_xyz.origin[1] = 0;	// sikk - changed from "20"
	g_qeglobals.d_xyz.origin[2] = 0;	// sikk - changed from "46"
	g_qeglobals.d_xyz.scale = 1;
	Map_RegionOff();	// sikk - For initiating Map Size change
}

/*
==================
XY_PositionView
==================
*/
void XY_PositionView ()
{
	int			nDim1, nDim2;
	brush_t	   *b;

	nDim1 = (g_qeglobals.d_nViewType == YZ) ? 1 : 0;
	nDim2 = (g_qeglobals.d_nViewType == XY) ? 1 : 2;

	b = g_brSelectedBrushes.next;
	if (b && b->next != b)
	{
		g_qeglobals.d_xyz.origin[nDim1] = b->mins[nDim1];
		g_qeglobals.d_xyz.origin[nDim2] = b->mins[nDim2];
	}
	else
	{
		g_qeglobals.d_xyz.origin[nDim1] = g_qeglobals.d_camera.origin[nDim1];
		g_qeglobals.d_xyz.origin[nDim2] = g_qeglobals.d_camera.origin[nDim2];
	}
}

/*
==================
Betwixt
==================
*/
float Betwixt (float f1, float f2)
{
	return (f1 + f2) * 0.5;	// wtf
	/*
	if (f1 > f2)
		return f2 + ((f1 - f2) / 2);
	else
		return f1 + ((f2 - f1) / 2);*/
}


/*
==================
fDiff
==================
*/
float fDiff (float f1, float f2)
{
	return fabs(f1 - f2); // wtfff
	/*
	if (f1 > f2)
		return f1 - f2;
	else
		return f2 - f1;*/
}

/*
==================
XY_VectorCopy
==================
*/
void XY_VectorCopy (vec3_t in, vec3_t out)
{
	if (g_qeglobals.d_nViewType == XY)
	{
		out[0] = in[0];
		out[1] = in[1];
	}
	else if (g_qeglobals.d_nViewType == XZ)
	{
		out[0] = in[0];
		out[2] = in[2];
	}
	else
	{
		out[1] = in[1];
		out[2] = in[2];
	}
}




/*
============================================================================

  MOUSE ACTIONS

============================================================================
*/

static int		cursorx, cursory;
static int		buttonstate;
static int		pressx, pressy;
static bool		press_selection;
static vec3_t	pressdelta;

/*
==================
XY_ToPoint
==================
*/
void XY_ToPoint (int x, int y, vec3_t point)
{
	if (g_qeglobals.d_nViewType == XY)
	{
		point[0] = g_qeglobals.d_xyz.origin[0] + (x - g_qeglobals.d_xyz.width / 2) / g_qeglobals.d_xyz.scale;
		point[1] = g_qeglobals.d_xyz.origin[1] + (y - g_qeglobals.d_xyz.height / 2) / g_qeglobals.d_xyz.scale;
	}
	else if (g_qeglobals.d_nViewType == YZ)
	{
		point[1] = g_qeglobals.d_xyz.origin[1] + (x - g_qeglobals.d_xyz.width / 2) / g_qeglobals.d_xyz.scale;
		point[2] = g_qeglobals.d_xyz.origin[2] + (y - g_qeglobals.d_xyz.height / 2) / g_qeglobals.d_xyz.scale;
	}
	else
	{
		point[0] = g_qeglobals.d_xyz.origin[0] + (x - g_qeglobals.d_xyz.width / 2) / g_qeglobals.d_xyz.scale;
		point[2] = g_qeglobals.d_xyz.origin[2] + (y - g_qeglobals.d_xyz.height / 2) / g_qeglobals.d_xyz.scale;
	}
}

/*
==================
XY_ToGridPoint
==================
*/
void XY_ToGridPoint (int x, int y, vec3_t point)
{
	if (g_qeglobals.d_nViewType == XY)
	{
		point[0] = g_qeglobals.d_xyz.origin[0] + (x - g_qeglobals.d_xyz.width / 2) / g_qeglobals.d_xyz.scale;
		point[1] = g_qeglobals.d_xyz.origin[1] + (y - g_qeglobals.d_xyz.height / 2) / g_qeglobals.d_xyz.scale;

		point[0] = floor(point[0] / g_qeglobals.d_nGridSize + 0.5) * g_qeglobals.d_nGridSize;
		point[1] = floor(point[1] / g_qeglobals.d_nGridSize + 0.5) * g_qeglobals.d_nGridSize;
	}
	else if (g_qeglobals.d_nViewType == YZ)
	{
		point[1] = g_qeglobals.d_xyz.origin[1] + (x - g_qeglobals.d_xyz.width / 2) / g_qeglobals.d_xyz.scale;
		point[2] = g_qeglobals.d_xyz.origin[2] + (y - g_qeglobals.d_xyz.height / 2) / g_qeglobals.d_xyz.scale;
		
		point[1] = floor(point[1] / g_qeglobals.d_nGridSize + 0.5) * g_qeglobals.d_nGridSize;
		point[2] = floor(point[2] / g_qeglobals.d_nGridSize + 0.5) * g_qeglobals.d_nGridSize;

	}
	else
	{
		point[0] = g_qeglobals.d_xyz.origin[0] + (x - g_qeglobals.d_xyz.width / 2) / g_qeglobals.d_xyz.scale;
		point[2] = g_qeglobals.d_xyz.origin[2] + (y - g_qeglobals.d_xyz.height / 2) / g_qeglobals.d_xyz.scale;
		
		point[0] = floor(point[0] / g_qeglobals.d_nGridSize + 0.5) * g_qeglobals.d_nGridSize;
		point[2] = floor(point[2] / g_qeglobals.d_nGridSize + 0.5) * g_qeglobals.d_nGridSize;
	}
}

/*
==================
XY_SnapToPoint
==================
*/
void XY_SnapToPoint (int x, int y, vec3_t point)
{
	if (g_qeglobals.d_savedinfo.bNoClamp)
		XY_ToPoint(x, y, point);
	else
		XY_ToGridPoint(x, y, point);
}

/*
==================
Drag_Delta
==================
*/
bool Drag_Delta (int x, int y, vec3_t move)
{
	vec3_t	xvec, yvec, delta;
	int		i;

	xvec[0] = 1 / g_qeglobals.d_xyz.scale;
	xvec[1] = xvec[2] = 0;
	yvec[1] = 1 / g_qeglobals.d_xyz.scale;
	yvec[0] = yvec[2] = 0;

	for (i = 0; i < 3; i++)
	{
		delta[i] = xvec[i] * (x - pressx) + yvec[i] * (y - pressy);

		if (!g_qeglobals.d_savedinfo.bNoClamp)
			delta[i] = floor(delta[i] / g_qeglobals.d_nGridSize + 0.5) * g_qeglobals.d_nGridSize;		
	}
	VectorSubtract(delta, pressdelta, move);
	VectorCopy(delta, pressdelta);

	if (move[0] || move[1] || move[2])
		return true;

	return false;
}

/*
==============
Drag_NewBrush
==============
*/
void Drag_NewBrush (int x, int y)
{
	vec3_t		mins, maxs, junk;
	int			i;
	float		temp;
	brush_t	   *n;
	int			nDim;

	if (!Drag_Delta(x, y, junk))
		return;

	// delete the current selection
	if (Select_HasBrushes())
		Brush_Free(g_brSelectedBrushes.next);

	XY_SnapToPoint(pressx, pressy, mins);

	nDim = (g_qeglobals.d_nViewType == XY) ? 2 : (g_qeglobals.d_nViewType == YZ) ? 0 : 1;

	mins[nDim] = g_qeglobals.d_nGridSize * ((int)(g_qeglobals.d_v3WorkMin[nDim] / g_qeglobals.d_nGridSize));

	XY_SnapToPoint(x, y, maxs);

	maxs[nDim] = g_qeglobals.d_nGridSize * ((int)(g_qeglobals.d_v3WorkMax[nDim] / g_qeglobals.d_nGridSize));


	if (maxs[nDim] <= mins[nDim])
		maxs[nDim] = mins[nDim] + g_qeglobals.d_nGridSize;

	for (i = 0; i < 3; i++)
	{
		if (mins[i] == maxs[i])
			return;	// don't create a degenerate brush
		if (mins[i] > maxs[i])
		{
			temp = mins[i];
			mins[i] = maxs[i];
			maxs[i] = temp;
		}
	}

	n = Brush_Create(mins, maxs, &g_qeglobals.d_workTexDef);
	if (!n)
		return;

	Brush_AddToList(n, &g_brSelectedBrushes);
	Entity_LinkBrush(g_peWorldEntity, n);
	Brush_Build(n);

	Sys_UpdateWindows(W_XY | W_Z | W_CAMERA);
}

/*
==============
XY_MouseDown
==============
*/
void XY_MouseDown (int x, int y, int buttons)
{
	vec3_t	point;
	vec3_t	origin, dir, right, up;

	int n1, n2;
	int nAngle;

	buttonstate = buttons;

	// clipper
	if ((buttonstate & MK_LBUTTON) && g_qeglobals.d_bClipMode)
	{
		// lunaran - alt quick clip
		if (GetKeyState(VK_MENU) < 0)
			Clip_StartQuickClip(x, y);
		else
			Clip_DropPoint(x, y);
		Sys_UpdateWindows(W_ALL);
	}
	else
	{
		pressx = x;
		pressy = y;

		VectorCopy(g_v3VecOrigin, pressdelta);

		XY_ToPoint(x, y, point);

		VectorCopy(point, origin);

		dir[0] = 0; 
		dir[1] = 0; 
		dir[2] = 0;
		if (g_qeglobals.d_nViewType == XY)
		{
			origin[2] = 8192;
			dir[2] = -1;

			right[0] = 1 / g_qeglobals.d_xyz.scale;
			right[1] = 0;
			right[2] = 0;

			up[0] = 0;
			up[1] = 1 / g_qeglobals.d_xyz.scale;
			up[2] = 0;
		}
		else if (g_qeglobals.d_nViewType == YZ)
		{
			origin[0] = 8192;
			dir[0] = -1;

			right[0] = 0;
			right[1] = 1 / g_qeglobals.d_xyz.scale;
			right[2] = 0; 
			
	   		up[0] = 0; 
			up[1] = 0;
			up[2] = 1 / g_qeglobals.d_xyz.scale;
		}
		else
		{
			origin[1] = 8192;
			dir[1] = -1;

			right[0] = 1 / g_qeglobals.d_xyz.scale;
			right[1] = 0;
			right[2] = 0; 

			up[0] = 0; 
			up[1] = 0;
			up[2] = 1 / g_qeglobals.d_xyz.scale;
		}

		press_selection = (Select_HasBrushes());

		Sys_GetCursorPos(&cursorx, &cursory);

		// LMB = manipulate selection
		// Shift+LMB = select
		if (buttonstate & MK_LBUTTON)
		{
// sikk---> Quick Move Selection (Ctrl+Alt+LMB)
			if (GetKeyState(VK_MENU) < 0 && GetKeyState(VK_CONTROL) < 0)
			{
				brush_t    *b;
				vec3_t		v1, v2;
	
				XY_SnapToPoint(x, y, v1);

				b = g_brSelectedBrushes.next;
				VectorSubtract(v1, b->mins, v2);

				if (g_qeglobals.d_nViewType == XY)
					v2[2] = 0;
				if (g_qeglobals.d_nViewType == XZ)
					v2[1] = 0;
				if (g_qeglobals.d_nViewType == YZ)
					v2[0] = 0;

				// this is so we don't drag faces when faces were previously dragged
				g_qeglobals.d_nNumMovePoints = 0;
				
				MoveSelection(v2);

				// update g_v3RotateOrigin to selection
				Select_GetTrueMid(g_v3RotateOrigin);	// sikk - Free Rotate
			}
// <---sikk
			else
			{
				Drag_Begin(x, y, buttons, right, up, origin, dir);

				// update g_v3RotateOrigin to selection
				Select_GetTrueMid(g_v3RotateOrigin);	// sikk - Free Rotate
			}
			return;
		}

		// Ctrl+MMB = move camera
		if (buttonstate == (MK_CONTROL | MK_MBUTTON))
		{	
//			g_qeglobals.d_camera.origin[0] = point[0];
//			g_qeglobals.d_camera.origin[1] = point[1];
			XY_VectorCopy(point, g_qeglobals.d_camera.origin);

			Sys_UpdateWindows(W_CAMERA | W_XY | W_Z);
		}

		// MMB = angle camera
		if (buttonstate == MK_MBUTTON)
		{	
// sikk---> Free Rotate: Pivot Icon
			if (GetKeyState(VK_MENU) < 0)
			{
				XY_SnapToPoint(x, y, point);

				if (g_qeglobals.d_nViewType == XY)
				{
					g_v3RotateOrigin[0] = point[0];
					g_v3RotateOrigin[1] = point[1];
				}
				else if (g_qeglobals.d_nViewType == XZ)
				{
					g_v3RotateOrigin[0] = point[0];
					g_v3RotateOrigin[2] = point[2];
				}
				else
				{
					g_v3RotateOrigin[1] = point[1];
					g_v3RotateOrigin[2] = point[2];
				}
				Sys_UpdateWindows(W_XY);
				return;
			}
// <---sikk
			else
			{
				VectorSubtract(point, g_qeglobals.d_camera.origin, point);

				n1 = (g_qeglobals.d_nViewType == XY) ? 1 : 2;
				n2 = (g_qeglobals.d_nViewType == YZ) ? 1 : 0;
				nAngle = (g_qeglobals.d_nViewType == XY) ? YAW : PITCH;

				if (point[n1] || point[n2])
				{
					g_qeglobals.d_camera.angles[nAngle] = 180 / Q_PI * atan2(point[n1], point[n2]);
					Sys_UpdateWindows(W_CAMERA | W_XY | W_Z);
				}
			}
		}

		// Shift+MMB = move z checker
		if (buttonstate == (MK_SHIFT | MK_MBUTTON))
		{
			XY_SnapToPoint(x, y, point);

			if (g_qeglobals.d_nViewType == XY)
			{
				g_qeglobals.d_z.origin[0] = point[0];
				g_qeglobals.d_z.origin[1] = point[1];
			}
			else if (g_qeglobals.d_nViewType == YZ)
			{
				g_qeglobals.d_z.origin[0] = point[1];
				g_qeglobals.d_z.origin[1] = point[2];
			}
			else
			{
				g_qeglobals.d_z.origin[0] = point[0];
				g_qeglobals.d_z.origin[1] = point[2];
			}
			Sys_UpdateWindows(W_XY | W_Z);
			return;
		}

// sikk - Undo/Redo for Free Rotate & Free Scale
		if (buttonstate & MK_RBUTTON)
		{
			if (GetKeyState(VK_SHIFT) < 0)
			{
				Undo_Start("Free Scale");
				Undo_AddBrushList(&g_brSelectedBrushes);
			}	
			if (GetKeyState(VK_MENU) < 0)
			{
				Undo_Start("Free Rotate");
				Undo_AddBrushList(&g_brSelectedBrushes);
			}
		}
// <---sikk
	}
}

/*
==============
XY_MouseUp
==============
*/
void XY_MouseUp (int x, int y, int buttons)
{
	// clipper
	if (g_qeglobals.d_bClipMode)
	{
		// lunaran - alt quick clip
		if (GetKeyState(VK_MENU) < 0)
			Clip_EndQuickClip();
		else
			Clip_EndPoint();
		Sys_UpdateWindows(W_ALL);
	}
	else
	{
		Drag_MouseUp();

		if (!press_selection)
			Sys_UpdateWindows(W_ALL);
	}

// sikk--->	Free Rotate & Free Scaling
	if (g_bRotateCheck || g_bScaleCheck)
	{
		g_nRotate = 0;

		if (g_bSnapCheck)
		{
			g_qeglobals.d_savedinfo.bNoClamp = false;
			g_bSnapCheck = false;
		}

		g_bRotateCheck = false;
		g_bScaleCheck = false;

// sikk - Undo/Redo for Free Rotate & Free Scale
		Undo_EndBrushList(&g_brSelectedBrushes);
		Undo_End();
// <---sikk

		Sys_UpdateWindows(W_XY | W_Z);
	}
// <---sikk


	buttonstate = 0;
}

/*
==============
XY_MouseMoved
==============
*/
void XY_MouseMoved (int x, int y, int buttons)
{
	vec3_t	point;
    int		n1, n2;
    int		nAngle;			
	int		nDim1, nDim2;
	vec3_t	tdp;
	char	xystring[256];

	char	szRotate[16];	// sikk - Free Rotate

	int		i;		// sikk - Mouse Zoom
	float	scale;	// sikk - Mouse Zoom


	if (!buttonstate || buttonstate ^ MK_RBUTTON)
	{
		XY_SnapToPoint(x, y, tdp);
		sprintf(xystring, "xyz Coordinates: (%d %d %d)", (int)tdp[0], (int)tdp[1], (int)tdp[2]);
		Sys_Status(xystring, 0);
	}

	// clipper
	if ((!buttonstate || buttonstate & MK_LBUTTON) && g_qeglobals.d_bClipMode)
	{
		Clip_MovePoint(x, y, XY);
		Sys_UpdateWindows(W_ALL);
	}
	else
	{
		if (!buttonstate)
			return;

		// LMB without selection = drag new brush
		if (buttonstate == MK_LBUTTON && !press_selection)
		{
			Drag_NewBrush(x, y);

			// update g_v3RotateOrigin to new brush (for when 'quick move' is used)
			Select_GetTrueMid(g_v3RotateOrigin);	// sikk - Free Rotate
			return;
		}

		// LMB (possibly with control and or shift)
		// with selection = drag selection
		if (buttonstate & MK_LBUTTON)
		{
			Drag_MouseMoved(x, y, buttons);
		
			// update g_v3RotateOrigin to new brush
			Select_GetTrueMid(g_v3RotateOrigin);	// sikk - Free Rotate

			Sys_UpdateWindows(W_XY | W_CAMERA | W_Z);
			return;
		}

		// Ctrl+MMB = move camera
		if (buttonstate == (MK_CONTROL | MK_MBUTTON))
		{
			XY_SnapToPoint(x, y, point);
			XY_VectorCopy(point, g_qeglobals.d_camera.origin);

			Sys_UpdateWindows(W_XY | W_CAMERA | W_Z);
			return;
		}

		// Shift+MMB = move z checker
		if (buttonstate == (MK_SHIFT | MK_MBUTTON))
		{
			XY_SnapToPoint(x, y, point);

			if (g_qeglobals.d_nViewType == XY)
			{
				g_qeglobals.d_z.origin[0] = point[0];
				g_qeglobals.d_z.origin[1] = point[1];
			}
			else if (g_qeglobals.d_nViewType == YZ)
			{
				g_qeglobals.d_z.origin[0] = point[1];
				g_qeglobals.d_z.origin[1] = point[2];
			}
			else
			{
				g_qeglobals.d_z.origin[0] = point[0];
				g_qeglobals.d_z.origin[1] = point[2];
			}
			Sys_UpdateWindows(W_XY | W_Z);
			return;
		}

		// MMB = angle camera
		if (buttonstate == MK_MBUTTON)
		{
// sikk---> Free Rotate: Pivot Icon
			// Alt+MMB = move free rotate pivot icon
			if (GetKeyState(VK_MENU) < 0)
			{
				XY_SnapToPoint(x, y, point);

				if (g_qeglobals.d_nViewType == XY)
				{
					g_v3RotateOrigin[0] = point[0];
					g_v3RotateOrigin[1] = point[1];
				}
				else if (g_qeglobals.d_nViewType == XZ)
				{
					g_v3RotateOrigin[0] = point[0];
					g_v3RotateOrigin[2] = point[2];
				}
				else
				{
					g_v3RotateOrigin[1] = point[1];
					g_v3RotateOrigin[2] = point[2];
				}
				Sys_UpdateWindows(W_XY);
				return;
			}
// <---sikk
			else
			{
				XY_SnapToPoint(x, y, point);
				VectorSubtract(point, g_qeglobals.d_camera.origin, point);

				n1 = (g_qeglobals.d_nViewType == XY) ? 1 : 2;
				n2 = (g_qeglobals.d_nViewType == YZ) ? 1 : 0;
				nAngle = (g_qeglobals.d_nViewType == XY) ? YAW : PITCH;

				if (point[n1] || point[n2])
				{
					g_qeglobals.d_camera.angles[nAngle] = 180 / Q_PI * atan2(point[n1], point[n2]);
					Sys_UpdateWindows(W_XY | W_CAMERA);
				}
			}
			return;
		}

		// RMB = drag xy origin
		if (buttonstate == MK_RBUTTON)
		{
			SetCursor(NULL); // sikk - Remove Cursor
			Sys_GetCursorPos(&x, &y);

// sikk---> Free Rotate
			// Alt+RMB = free rotate selected brush
			if (GetKeyState(VK_MENU) < 0)
			{
				g_bRotateCheck = true;

				if (!g_qeglobals.d_savedinfo.bNoClamp)
				{
					g_qeglobals.d_savedinfo.bNoClamp = true;
					g_bSnapCheck = true;
				}
				
				switch (g_qeglobals.d_nViewType)
				{
				case XY:
					x -= cursorx;
					g_nRotate += x;
					Select_RotateAxis(2, x, true);
					break;
				case XZ:
					x = cursorx - x;
					g_nRotate += -x;
					Select_RotateAxis(1, x, true);
					break;
				case YZ:
					x -= cursorx;
					g_nRotate += x;
					Select_RotateAxis(0, x, true);
					break;
				}

				if (g_nRotate >= 360)
					g_nRotate -= 360;
				if (g_nRotate < -360)
					g_nRotate += 360;

				Sys_SetCursorPos(cursorx, cursory);

				// sikk - this ensures that the windows are updated in realtime
				InvalidateRect(g_qeglobals.d_hwndCamera, NULL, FALSE);
				InvalidateRect(g_qeglobals.d_hwndXY, NULL, FALSE);
				InvalidateRect(g_qeglobals.d_hwndXZ, NULL, FALSE);
				InvalidateRect(g_qeglobals.d_hwndYZ, NULL, FALSE);
				InvalidateRect(g_qeglobals.d_hwndZ, NULL, FALSE);
				UpdateWindow(g_qeglobals.d_hwndCamera);					
				UpdateWindow(g_qeglobals.d_hwndXY);					
				UpdateWindow(g_qeglobals.d_hwndXZ);					
				UpdateWindow(g_qeglobals.d_hwndYZ);					
				UpdateWindow(g_qeglobals.d_hwndZ);					

				sprintf(szRotate, "Rotate: %d°", g_nRotate);
				Sys_Status(szRotate, 0);
			}
// <---sikk
			else
			{
				if (x != cursorx || y != cursory)
				{
					nDim1 = (g_qeglobals.d_nViewType == YZ) ? 1 : 0;
					nDim2 = (g_qeglobals.d_nViewType == XY) ? 1 : 2;

					g_qeglobals.d_xyz.origin[nDim1] -= (x - cursorx) / g_qeglobals.d_xyz.scale;
					g_qeglobals.d_xyz.origin[nDim2] += (y - cursory) / g_qeglobals.d_xyz.scale;

					Sys_SetCursorPos(cursorx, cursory);
					Sys_UpdateWindows(W_XY| W_Z);

					sprintf(xystring, "xyz Origin: (%d %d %d)", (int)g_qeglobals.d_xyz.origin[0], (int)g_qeglobals.d_xyz.origin[1], (int)g_qeglobals.d_xyz.origin[2]);
					Sys_Status(xystring, 0);
				}
			}
			return;
		}
		
// sikk---> Free Scaling
		// Shift+RMB = free scale selected brush
		if (buttonstate == (MK_SHIFT | MK_RBUTTON))
		{
			int i;

			g_bScaleCheck = true;

			if (!g_qeglobals.d_savedinfo.bNoClamp)
			{
				g_qeglobals.d_savedinfo.bNoClamp = true;
				g_bSnapCheck = true;
			}
			
			SetCursor(NULL); // sikk - Remove Cursor
			Sys_GetCursorPos(&x, &y);

			i = cursory - y;

			if (i < 0)
			{
				if (g_qeglobals.d_savedinfo.bScaleLockX)
					Select_Scale(0.9f, 1.0f, 1.0f);
				if (g_qeglobals.d_savedinfo.bScaleLockY)
					Select_Scale(1.0f, 0.9f, 1.0f);
				if (g_qeglobals.d_savedinfo.bScaleLockZ)
					Select_Scale(1.0f, 1.0f, 0.9f);
			}
			if (i > 0)
			{
				if (g_qeglobals.d_savedinfo.bScaleLockX)
					Select_Scale(1.1f, 1.0f, 1.0f);
				if (g_qeglobals.d_savedinfo.bScaleLockY)
					Select_Scale(1.0f, 1.1f, 1.0f);
				if (g_qeglobals.d_savedinfo.bScaleLockZ)
					Select_Scale(1.0f, 1.0f, 1.1f);
			}
			
			Sys_SetCursorPos(cursorx, cursory);
//			cursory = y;

			// sikk - this ensures that the windows are updated in realtime
			InvalidateRect(g_qeglobals.d_hwndCamera, NULL, FALSE);
			InvalidateRect(g_qeglobals.d_hwndXY, NULL, FALSE);
			InvalidateRect(g_qeglobals.d_hwndXZ, NULL, FALSE);
			InvalidateRect(g_qeglobals.d_hwndYZ, NULL, FALSE);
			InvalidateRect(g_qeglobals.d_hwndZ, NULL, FALSE);
			UpdateWindow(g_qeglobals.d_hwndCamera);					
			UpdateWindow(g_qeglobals.d_hwndXY);					
			UpdateWindow(g_qeglobals.d_hwndXZ);					
			UpdateWindow(g_qeglobals.d_hwndYZ);					
			UpdateWindow(g_qeglobals.d_hwndZ);					
			return;
		}
// <---sikk

// sikk---> Mouse Zoom
		// Ctrl+RMB = zoom xy view
		if (buttonstate == (MK_CONTROL | MK_RBUTTON))
		{
			SetCursor(NULL); // sikk - Remove Cursor
			Sys_GetCursorPos(&x, &y);
			
			if (y != cursory)
			{
				scale = 1;

				for (i = abs(y-cursory); i > 0; i--)
					scale *=  y > cursory ? 1.01f : 0.99f;

				g_qeglobals.d_xyz.scale *= scale;

				if (g_qeglobals.d_xyz.scale > 32)
					g_qeglobals.d_xyz.scale = 32;
				else if (g_qeglobals.d_xyz.scale < 0.05f)
					g_qeglobals.d_xyz.scale = 0.05f;

				Sys_SetCursorPos(cursorx, cursory);
				Sys_UpdateWindows(W_XY);
			}
			return;
		}
// <---sikk
	}
}

/*
=============================================================

  PATH LINES

=============================================================
*/

/*
==================
DrawPathLines

Draws connections between entities.
Needs to consider all entities, not just ones on screen,
because the lines can be visible when neither end is.
Called for both camera view and xy view.
==================
*/
void DrawPathLines ()
{
	int			i, j, k;
	vec3_t		mid, mid1;
	entity_t   *se, *te;
	brush_t	   *sb, *tb;
	char	   *psz;
	vec3_t		dir, s1, s2;
	vec_t		len, f;
	int			arrows;
	int			num_entities;
	char	   *ent_target[MAX_MAP_ENTITIES];
	entity_t   *ent_entity[MAX_MAP_ENTITIES];

	if (g_qeglobals.d_savedinfo.nExclude & EXCLUDE_PATHS)
		return;

	num_entities = 0;
	for (te = g_entEntities.next; te != &g_entEntities && num_entities != MAX_MAP_ENTITIES; te = te->next)
	{
		ent_target[num_entities] = ValueForKey(te, "target");
		if (ent_target[num_entities][0])
		{
			ent_entity[num_entities] = te;
			num_entities++;
		}
	}

	for (se = g_entEntities.next; se != &g_entEntities; se = se->next)
	{

		psz = ValueForKey(se, "targetname");
	
		if (psz == NULL || psz[0] == '\0')
			continue;
		
		sb = se->brushes.onext;
		if (sb == &se->brushes)
			continue;

		for (k = 0; k < num_entities; k++)
		{
			if (strcmp(ent_target[k], psz))
				continue;

			te = ent_entity[k];
			tb = te->brushes.onext;
			if (tb == &te->brushes)
				continue;

// sikk---> Don't Draw Path Line if entity on either side is hidden/filtered/regioned off
			if (FilterBrush(tb) || FilterBrush(se->brushes.onext))
				continue;
			if (Map_IsBrushFiltered(tb) || Map_IsBrushFiltered(se->brushes.onext))
				continue;
// <---sikk

			for (i = 0; i < 3; i++)
				mid[i] = (sb->mins[i] + sb->maxs[i]) * 0.5; 

			for (i = 0; i < 3; i++)
				mid1[i] = (tb->mins[i] + tb->maxs[i]) * 0.5; 

			VectorSubtract(mid1, mid, dir);
			len = VectorNormalize(dir);
			s1[0] = -dir[1] * 8 + dir[0] * 8;
			s2[0] =  dir[1] * 8 + dir[0] * 8;
			s1[1] =  dir[0] * 8 + dir[1] * 8;
			s2[1] = -dir[0] * 8 + dir[1] * 8;

			glColor3f(se->eclass->color[0], se->eclass->color[1], se->eclass->color[2]);

			glBegin(GL_LINES);
			glVertex3fv(mid);
			glVertex3fv(mid1);

			arrows = (int)(len / 256) + 1;

			for (i = 0; i < arrows; i++)
			{
				f = len * (i + 0.5) / arrows;

				for (j = 0; j < 3; j++)
					mid1[j] = mid[j] + f * dir[j];
				glVertex3fv(mid1);
				glVertex3f(mid1[0] + s1[0], mid1[1] + s1[1], mid1[2]);
				glVertex3fv(mid1);
				glVertex3f(mid1[0] + s2[0], mid1[1] + s2[1], mid1[2]);
			}

			glEnd();
		}
	}

	return;
}


/*
============================================================================

	DRAWING

============================================================================
*/

/*
==================
FilterBrush
==================
*/
bool FilterBrush (brush_t *b)
{
	if (!b->owner)
		return true;		// during construction

	if (b->hiddenBrush)
		return true;

	if (g_qeglobals.d_savedinfo.nExclude & EXCLUDE_CLIP)
		if (!strncmp(b->brush_faces->texdef.name, "clip", 4))
			return true;

	if (g_qeglobals.d_savedinfo.nExclude & EXCLUDE_HINT)
		if (!strncmp(b->brush_faces->texdef.name, "hint", 4))	// catches hint and hintskip
			return true;

	if (g_qeglobals.d_savedinfo.nExclude & EXCLUDE_WATER)
		if (b->brush_faces->texdef.name[0] == '*')
			return true;

	if (g_qeglobals.d_savedinfo.nExclude & EXCLUDE_SKY)
		if (!strncmp(b->brush_faces->texdef.name, "sky", 3))
			return true;

	if (g_qeglobals.d_savedinfo.nExclude & EXCLUDE_FUNC_WALL)
		if (!strncmp(b->owner->eclass->name, "func_wall", 9))
			return true;

	if (g_qeglobals.d_savedinfo.nExclude & EXCLUDE_DETAIL)
		if (!strncmp(b->owner->eclass->name, "func_detail", 11))
			return true;

	if (b->owner == g_peWorldEntity)
	{
		if (g_qeglobals.d_savedinfo.nExclude & EXCLUDE_WORLD)
			return true;
		return false;
	}
	else if (g_qeglobals.d_savedinfo.nExclude & EXCLUDE_ENT)
		return true;

	if (g_qeglobals.d_savedinfo.nExclude & EXCLUDE_LIGHTS)
		return (b->owner->eclass->nShowFlags & ECLASS_LIGHT);

	if (g_qeglobals.d_savedinfo.nExclude & EXCLUDE_PATHS)
		return (b->owner->eclass->nShowFlags & ECLASS_PATH);

	return false;
}

/*
==============
XY_DrawGrid
==============
*/
void XY_DrawGrid ()
{
	float	x, y, xb, xe, yb, ye;
	int		w, h;
	int		nDim1, nDim2;
	int		nSize;

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	if (g_qeglobals.d_nGridSize == 128)
		nSize = 128;
	else if (g_qeglobals.d_nGridSize == 256)
		nSize = 256;
	else
		nSize = 64;

	w = g_qeglobals.d_xyz.width / 2 / g_qeglobals.d_xyz.scale;
	h = g_qeglobals.d_xyz.height / 2 / g_qeglobals.d_xyz.scale;

	nDim1 = (g_qeglobals.d_nViewType == YZ) ? 1 : 0;
	nDim2 = (g_qeglobals.d_nViewType == XY) ? 1 : 2;

	xb = g_qeglobals.d_xyz.origin[nDim1] - w;
	if (xb < g_v3RegionMins[nDim1])
		xb = g_v3RegionMins[nDim1];
	xb = nSize * floor(xb / nSize);

	xe = g_qeglobals.d_xyz.origin[nDim1] + w;
	if (xe > g_v3RegionMaxs[nDim1])
		xe = g_v3RegionMaxs[nDim1];
	xe = nSize * ceil(xe / nSize);

	yb = g_qeglobals.d_xyz.origin[nDim2] - h;
	if (yb < g_v3RegionMins[nDim2])
		yb = g_v3RegionMins[nDim2];
	yb = nSize * floor(yb / nSize);

	ye = g_qeglobals.d_xyz.origin[nDim2] + h;
	if (ye > g_v3RegionMaxs[nDim2])
		ye = g_v3RegionMaxs[nDim2];
	ye = nSize * ceil(ye / nSize);

	// draw major blocks
	glColor3fv(g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDMAJOR]);

	if (g_qeglobals.d_bShowGrid)
	{
		glBegin(GL_LINES);
		
		for (x = xb; x <= xe; x += nSize)
		{
			glVertex2f(x, yb);
			glVertex2f(x, ye);
		}
		for (y = yb; y <= ye; y += nSize)
		{
			glVertex2f(xb, y);
			glVertex2f(xe, y);
		}
		glEnd();
	}

	// draw minor blocks
	if (g_qeglobals.d_bShowGrid && g_qeglobals.d_nGridSize * g_qeglobals.d_xyz.scale >= 4)
	{
		glColor3fv(g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDMINOR]);

		glBegin(GL_LINES);
		for (x = xb; x < xe; x += g_qeglobals.d_nGridSize)
		{
			if (!((int)x & 63))
				continue;
			glVertex2f(x, yb);
			glVertex2f(x, ye);
		}
		for (y = yb; y < ye; y += g_qeglobals.d_nGridSize)
		{
			if (!((int)y & 63))
				continue;
			glVertex2f(xb, y);
			glVertex2f(xe, y);
		}
		glEnd();
	}

	// draw grid axis
	if (g_qeglobals.d_bShowGrid)
	{
		glColor3f(g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDMAJOR][0] * 0.65,
				  g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDMAJOR][1] * 0.65,
				  g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDMAJOR][2] * 0.65);
		glBegin(GL_LINES);
		glVertex2f(xb, 0);
		glVertex2f(xe, 0);
		glVertex2f(0, yb);
		glVertex2f(0, ye);
		glEnd();
	}

	// show current work zone?
	// the work zone is used to place dropped points and brushes
	if (g_qeglobals.d_savedinfo.bShow_Workzone)
	{
		glColor3f(1.0f, 0.0f, 0.0f);
		glBegin(GL_LINES);
		glVertex2f(xb, g_qeglobals.d_v3WorkMin[nDim2]);
		glVertex2f(xe, g_qeglobals.d_v3WorkMin[nDim2]);
		glVertex2f(xb, g_qeglobals.d_v3WorkMax[nDim2]);
		glVertex2f(xe, g_qeglobals.d_v3WorkMax[nDim2]);
		glVertex2f(g_qeglobals.d_v3WorkMin[nDim1], yb);
		glVertex2f(g_qeglobals.d_v3WorkMin[nDim1], ye);
		glVertex2f(g_qeglobals.d_v3WorkMax[nDim1], yb);
		glVertex2f(g_qeglobals.d_v3WorkMax[nDim1], ye);
		glEnd();
	}
}

/*
==============
XY_DrawBlockGrid
==============
*/
void XY_DrawBlockGrid ()
{
	float	x, y, xb, xe, yb, ye;
	int		w, h;
	char	text[32];
	int		nDim1, nDim2;

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	w = g_qeglobals.d_xyz.width / 2 / g_qeglobals.d_xyz.scale;
	h = g_qeglobals.d_xyz.height / 2 / g_qeglobals.d_xyz.scale;

	nDim1 = (g_qeglobals.d_nViewType == YZ) ? 1 : 0;
	nDim2 = (g_qeglobals.d_nViewType == XY) ? 1 : 2;

	xb = g_qeglobals.d_xyz.origin[nDim1] - w;
	if (xb < g_v3RegionMins[nDim1])
		xb = g_v3RegionMins[nDim1];
	xb = 1024 * floor(xb / 1024);

	xe = g_qeglobals.d_xyz.origin[nDim1] + w;
	if (xe > g_v3RegionMaxs[nDim1])
		xe = g_v3RegionMaxs[nDim1];
	xe = 1024 * ceil(xe / 1024);

	yb = g_qeglobals.d_xyz.origin[nDim2] - h;
	if (yb < g_v3RegionMins[nDim2])
		yb = g_v3RegionMins[nDim2];
	yb = 1024 * floor(yb / 1024);

	ye = g_qeglobals.d_xyz.origin[nDim2] + h;
	if (ye > g_v3RegionMaxs[nDim2])
		ye = g_v3RegionMaxs[nDim2];
	ye = 1024 * ceil(ye / 1024);

	// draw major blocks
	glColor3fv(g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDBLOCK]);
	glLineWidth(2);
	glBegin(GL_LINES);
	
	for (x = xb; x <= xe; x += 1024)
	{
		glVertex2f(x, yb);
		glVertex2f(x, ye);
	}
	for (y = yb; y <= ye; y += 1024)
	{
		glVertex2f(xb, y);
		glVertex2f(xe, y);
	}
	
	glEnd();
	glLineWidth(1);

	// draw coordinate text if needed
	for (x = xb; x < xe; x += 1024)
		for (y = yb; y < ye; y += 1024)
		{
			glRasterPos2f(x + 512, y + 512);
			sprintf(text, "%d,%d",(int)floor(x / 1024), (int)floor(y / 1024));
			glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);
		}

//	glColor4f(0, 0, 0, 0);
}

/*
==============
XY_DrawCoords
==============
*/
void XY_DrawCoords ()
{
	float	x, y, xb, xe, yb, ye;
	int		w, h;
	int		nDim1, nDim2;
	char	text[8];
	char	*szView;
	float	fColor[][3] = {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
	int		nSize;

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	if (g_qeglobals.d_nGridSize == 128)
		nSize = 128;
	else if (g_qeglobals.d_nGridSize == 256)
		nSize = 256;
	else
		nSize = 64;

	w = g_qeglobals.d_xyz.width / 2 / g_qeglobals.d_xyz.scale;
	h = g_qeglobals.d_xyz.height / 2 / g_qeglobals.d_xyz.scale;

	nDim1 = (g_qeglobals.d_nViewType == YZ) ? 1 : 0;
	nDim2 = (g_qeglobals.d_nViewType == XY) ? 1 : 2;

	xb = g_qeglobals.d_xyz.origin[nDim1] - w;
	if (xb < g_v3RegionMins[nDim1])
		xb = g_v3RegionMins[nDim1];
	xb = nSize * floor(xb / nSize);

	xe = g_qeglobals.d_xyz.origin[nDim1] + w;
	if (xe > g_v3RegionMaxs[nDim1])
		xe = g_v3RegionMaxs[nDim1];
	xe = nSize * ceil(xe / nSize);

	yb = g_qeglobals.d_xyz.origin[nDim2] - h;
	if (yb < g_v3RegionMins[nDim2])
		yb = g_v3RegionMins[nDim2];
	yb = nSize * floor(yb / nSize);

	ye = g_qeglobals.d_xyz.origin[nDim2] + h;
	if (ye > g_v3RegionMaxs[nDim2])
		ye = g_v3RegionMaxs[nDim2];
	ye = nSize * ceil(ye / nSize);

// sikk---> Filter Coords so they don't become bunched and unreadable 
	// TODO: Fix the bug that makes next coord off screen draw in various locations. 
	// draw coordinate text if needed
	if (g_qeglobals.d_savedinfo.bShow_Coordinates)
	{
		glColor3fv(g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDTEXT]);

		nSize = 64;
		if (g_qeglobals.d_xyz.scale <= 0.6)
			nSize = 128;
		if (g_qeglobals.d_xyz.scale <= 0.3)
			nSize = 256;
		if (g_qeglobals.d_xyz.scale <= 0.15)
			nSize = 512;
		if (g_qeglobals.d_xyz.scale <= 0.075)
			nSize = 1024;

		xb = g_qeglobals.d_xyz.origin[nDim1] - w;
		if (xb < g_v3RegionMins[nDim1])
			xb = g_v3RegionMins[nDim1];
		xb = nSize * floor(xb / nSize);

		xe = g_qeglobals.d_xyz.origin[nDim1] + w;
		if (xe > g_v3RegionMaxs[nDim1])
			xe = g_v3RegionMaxs[nDim1];
		xe = nSize * ceil(xe / nSize);

		yb = g_qeglobals.d_xyz.origin[nDim2] - h;
		if (yb < g_v3RegionMins[nDim2])
			yb = g_v3RegionMins[nDim2];
		yb = nSize * floor(yb / nSize);

		ye = g_qeglobals.d_xyz.origin[nDim2] + h;
		if (ye > g_v3RegionMaxs[nDim2])
			ye = g_v3RegionMaxs[nDim2];
		ye = nSize * ceil(ye / nSize);

		for (x = xb; x <= xe; x += nSize)	// sikk - 'x <= xe' instead of 'x < xe' so last coord is drawn 
		{
			glRasterPos2f(x, g_qeglobals.d_xyz.origin[nDim2] + h - 6 / g_qeglobals.d_xyz.scale);
			sprintf(text, "%d",(int)x);
			glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);
		}
		for (y = yb; y <= ye; y += nSize)	// sikk - 'y <= ye' instead of 'y < ye' so last coord is drawn
		{
			glRasterPos2f(g_qeglobals.d_xyz.origin[nDim1] - w + 1, y);
			sprintf(text, "%d",(int)y);
			glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);
		}
	}
// <---sikk

// sikk---> View Axis - cleaner, more intuitive look
	if (g_qeglobals.d_savedinfo.bShow_Viewname)
	{
		float *p1, *p2;

		if (g_qeglobals.d_nViewType == XY)
		{
			p1 = fColor[0];
			p2 = fColor[1];
			szView = "XY";
		}
		else if (g_qeglobals.d_nViewType == XZ)
		{
			p1 = fColor[0];
			p2 = fColor[2];
			szView = "XZ";
		}
		else
		{
			p1 = fColor[1];
			p2 = fColor[2];
			szView = "YZ";
		}

		glColor3fv(g_qeglobals.d_savedinfo.v3Colors[COLOR_VIEWNAME]);
		glRasterPos2f(g_qeglobals.d_xyz.origin[nDim1] - w + 68 / g_qeglobals.d_xyz.scale, 
					  g_qeglobals.d_xyz.origin[nDim2] + h - 51 / g_qeglobals.d_xyz.scale);
		glCallLists(1, GL_UNSIGNED_BYTE, &szView[0]);

		glColor3fv(g_qeglobals.d_savedinfo.v3Colors[COLOR_VIEWNAME]);
		glRasterPos2f(g_qeglobals.d_xyz.origin[nDim1] - w + 44 / g_qeglobals.d_xyz.scale, 
				      g_qeglobals.d_xyz.origin[nDim2] + h - 28 / g_qeglobals.d_xyz.scale);
		glCallLists(1, GL_UNSIGNED_BYTE, &szView[1]);

		glLineWidth(2);
		glColor3fv(p1);
		glBegin(GL_LINES);
		glVertex2f(g_qeglobals.d_xyz.origin[nDim1] - w + 48 / g_qeglobals.d_xyz.scale, 
				   g_qeglobals.d_xyz.origin[nDim2] + h - 48 / g_qeglobals.d_xyz.scale);
		glVertex2f(g_qeglobals.d_xyz.origin[nDim1] - w + 64 / g_qeglobals.d_xyz.scale, 
				   g_qeglobals.d_xyz.origin[nDim2] + h - 48 / g_qeglobals.d_xyz.scale);
		glEnd();

		glColor3fv(p2);
		glBegin(GL_LINES);
		glVertex2f(g_qeglobals.d_xyz.origin[nDim1] - w + 48 / g_qeglobals.d_xyz.scale, 
				   g_qeglobals.d_xyz.origin[nDim2] + h - 48 / g_qeglobals.d_xyz.scale);
		glVertex2f(g_qeglobals.d_xyz.origin[nDim1] - w + 48 / g_qeglobals.d_xyz.scale, 
				   g_qeglobals.d_xyz.origin[nDim2] + h - 32 / g_qeglobals.d_xyz.scale);
		glEnd();
		glLineWidth(1);
	}
// <---sikk

/*
// sikk---> Show Axis in center of view
	if (g_qeglobals.d_savedinfo.bShow_Axis)
	{
		glLineWidth(2);
		if (g_qeglobals.d_nViewType == XY)
			glColor3f(1.0, 0.0, 0.0);	
		else if (g_qeglobals.d_nViewType == XZ)
			glColor3f(1.0, 0.0, 0.0);
		else
			glColor3f(0.0, 1.0, 0.0);
		glBegin(GL_LINES);
		glVertex2f(0, 0);
		glVertex2f(64,0);
		glEnd();
			
		if (g_qeglobals.d_nViewType == XY)
			glColor3f(0.0, 1.0, 0.0);
		else if (g_qeglobals.d_nViewType == XZ)
			glColor3f(0.0, 0.0, 1.0);
		else
			glColor3f(0.0, 0.0, 1.0);
		glBegin(GL_LINES);
		glVertex2f(0, 0);
		glVertex2f(0, 64);
		glEnd();
		glLineWidth(1);
	}
// <---sikk
*/
}

/*
==================
XY_DrawCameraIcon
==================
*/
void XY_DrawCameraIcon ()
{
	float	x, y, a;
//	char	text[128];

	if (g_qeglobals.d_nViewType == XY)
	{
		x = g_qeglobals.d_camera.origin[0];
		y = g_qeglobals.d_camera.origin[1];
		a = g_qeglobals.d_camera.angles[YAW] / 180 * Q_PI;
	}
	else if (g_qeglobals.d_nViewType == YZ)
	{
		x = g_qeglobals.d_camera.origin[1];
		y = g_qeglobals.d_camera.origin[2];
		a = g_qeglobals.d_camera.angles[PITCH] / 180 * Q_PI;
	}
	else
	{
		x = g_qeglobals.d_camera.origin[0];
		y = g_qeglobals.d_camera.origin[2];
		a = g_qeglobals.d_camera.angles[PITCH] / 180 * Q_PI;
	}

	glColor3f(0.0, 0.0, 1.0);
	glBegin(GL_LINE_STRIP);
	glVertex3f(x - 16, y, 0);
	glVertex3f(x, y + 8, 0);
	glVertex3f(x + 16, y, 0);
	glVertex3f(x, y - 8, 0);
	glVertex3f(x - 16, y, 0);
	glVertex3f(x + 16, y, 0);
	glEnd();
	
	glBegin(GL_LINE_STRIP);
	glVertex3f(x + 48 * cos(a + Q_PI / 4), y + 48 * sin(a + Q_PI / 4), 0);
	glVertex3f(x, y, 0);
	glVertex3f(x + 48 * cos(a - Q_PI / 4), y + 48 * sin(a - Q_PI / 4), 0);
	glEnd();

//	glRasterPos2f(x + 64, y + 64);
//	sprintf(text, "angle: %f", g_qeglobals.d_camera.angles[YAW]);
//	glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);
}

/*
==================
XY_DrawZIcon
==================
*/
void XY_DrawZIcon ()
{
	float x, y;

	if (g_qeglobals.d_nViewType == XY)
	{
		x = g_qeglobals.d_z.origin[0];
		y = g_qeglobals.d_z.origin[1];

		glEnable(GL_BLEND);
		glDisable(GL_TEXTURE_2D);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDisable(GL_CULL_FACE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(0.0, 0.0, 1.0, 0.25);

		glBegin(GL_QUADS);
		glVertex3f(x - 8, y - 8, 0);
		glVertex3f(x + 8, y - 8, 0);
		glVertex3f(x + 8, y + 8, 0);
		glVertex3f(x - 8, y + 8, 0);
		glEnd();

		glDisable(GL_BLEND);
		glColor4f(0.0, 0.0, 1.0, 1);

		glBegin(GL_LINE_LOOP);
		glVertex3f(x - 8, y - 8, 0);
		glVertex3f(x + 8, y - 8, 0);
		glVertex3f(x + 8, y + 8, 0);
		glVertex3f(x - 8, y + 8, 0);
		glEnd();

		glBegin(GL_LINE_STRIP);
		glVertex3f(x - 4, y + 4, 0);
		glVertex3f(x + 4, y + 4, 0);
		glVertex3f(x - 4, y - 4, 0);
		glVertex3f(x + 4, y - 4, 0);
		glEnd();
	}
}

// sikk---> Free Rotate: Pivot Icon
/*
===============
XY_DrawRotateIcon
===============
*/
void XY_DrawRotateIcon ()
{
	float x, y;

	if (g_qeglobals.d_nViewType == XY)
	{
		x = g_v3RotateOrigin[0];
		y = g_v3RotateOrigin[1];
	}
	else if (g_qeglobals.d_nViewType == YZ)
	{
		x = g_v3RotateOrigin[1];
		y = g_v3RotateOrigin[2];
	}
	else
	{
		x = g_v3RotateOrigin[0];
		y = g_v3RotateOrigin[2];
	}

	glEnable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0.8f, 0.1f, 0.9f, 0.25f);

	glBegin(GL_QUADS);
	glVertex3f(x - 4, y - 4, 0);
	glVertex3f(x + 4, y - 4, 0);
	glVertex3f(x + 4, y + 4, 0);
	glVertex3f(x - 4, y + 4, 0);
	glEnd();
	glDisable(GL_BLEND);

	glColor4f(1.0f, 0.2f, 1.0f, 1.0f);
	glBegin(GL_POINTS);
	glVertex3f(x, y, 0);
	glEnd();
}
// <---sikk

/*
==================
XY_DrawSizeInfo

Can be greatly simplified but per usual i am in a hurry 
which is not an excuse, just a fact
==================
*/
void XY_DrawSizeInfo (int nDim1, int nDim2, vec3_t vMinBounds, vec3_t vMaxBounds)
{
	vec3_t	vSize;
	char	dimstr[128];

	VectorSubtract(vMaxBounds, vMinBounds, vSize);

	glColor3f(g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES][0] * .65, 
			  g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES][1] * .65,
			  g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES][2] * .65);

	if (g_qeglobals.d_nViewType == XY)
	{
		glBegin(GL_LINES);

		glVertex3f(vMinBounds[nDim1], vMinBounds[nDim2] - 6.0f  / g_qeglobals.d_xyz.scale, 0.0f);
		glVertex3f(vMinBounds[nDim1], vMinBounds[nDim2] - 10.0f / g_qeglobals.d_xyz.scale, 0.0f);

		glVertex3f(vMinBounds[nDim1], vMinBounds[nDim2] - 10.0f / g_qeglobals.d_xyz.scale, 0.0f);
		glVertex3f(vMaxBounds[nDim1], vMinBounds[nDim2] - 10.0f / g_qeglobals.d_xyz.scale, 0.0f);

		glVertex3f(vMaxBounds[nDim1], vMinBounds[nDim2] - 6.0f  / g_qeglobals.d_xyz.scale, 0.0f);
		glVertex3f(vMaxBounds[nDim1], vMinBounds[nDim2] - 10.0f / g_qeglobals.d_xyz.scale, 0.0f);
  
		glVertex3f(vMaxBounds[nDim1] + 6.0f  / g_qeglobals.d_xyz.scale, vMinBounds[nDim2], 0.0f);
		glVertex3f(vMaxBounds[nDim1] + 10.0f / g_qeglobals.d_xyz.scale, vMinBounds[nDim2], 0.0f);

		glVertex3f(vMaxBounds[nDim1] + 10.0f / g_qeglobals.d_xyz.scale, vMinBounds[nDim2], 0.0f);
		glVertex3f(vMaxBounds[nDim1] + 10.0f / g_qeglobals.d_xyz.scale, vMaxBounds[nDim2], 0.0f);
  
		glVertex3f(vMaxBounds[nDim1] + 6.0f  / g_qeglobals.d_xyz.scale, vMaxBounds[nDim2], 0.0f);
		glVertex3f(vMaxBounds[nDim1] + 10.0f / g_qeglobals.d_xyz.scale, vMaxBounds[nDim2], 0.0f);

		glEnd();

		glRasterPos3f(Betwixt(vMinBounds[nDim1], vMaxBounds[nDim1]), vMinBounds[nDim2] - 20.0 / g_qeglobals.d_xyz.scale, 0.0f);
		sprintf(dimstr, g_pszDimStrings[nDim1], vSize[nDim1]);
		glCallLists(strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);
    
		glRasterPos3f(vMaxBounds[nDim1] + 16.0 / g_qeglobals.d_xyz.scale, Betwixt(vMinBounds[nDim2], vMaxBounds[nDim2]), 0.0f);
		sprintf(dimstr, g_pszDimStrings[nDim2], vSize[nDim2]);
		glCallLists(strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);

		glRasterPos3f(vMinBounds[nDim1] + 4, vMaxBounds[nDim2] + 8 / g_qeglobals.d_xyz.scale, 0.0f);
		sprintf(dimstr, g_pszOrgStrings[0], vMinBounds[nDim1], vMaxBounds[nDim2]);
		glCallLists(strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);
	}
	else if (g_qeglobals.d_nViewType == XZ)
	{
		glBegin(GL_LINES);

		glVertex3f(vMinBounds[nDim1], 0.0f, vMinBounds[nDim2] - 6.0f  / g_qeglobals.d_xyz.scale);
		glVertex3f(vMinBounds[nDim1], 0.0f, vMinBounds[nDim2] - 10.0f / g_qeglobals.d_xyz.scale);

		glVertex3f(vMinBounds[nDim1], 0.0f, vMinBounds[nDim2] - 10.0f / g_qeglobals.d_xyz.scale);
		glVertex3f(vMaxBounds[nDim1], 0.0f, vMinBounds[nDim2] - 10.0f / g_qeglobals.d_xyz.scale);

		glVertex3f(vMaxBounds[nDim1], 0.0f, vMinBounds[nDim2] - 6.0f  / g_qeglobals.d_xyz.scale);
		glVertex3f(vMaxBounds[nDim1], 0.0f, vMinBounds[nDim2] - 10.0f / g_qeglobals.d_xyz.scale);
  
		glVertex3f(vMaxBounds[nDim1] + 6.0f  / g_qeglobals.d_xyz.scale, 0.0f, vMinBounds[nDim2]);
		glVertex3f(vMaxBounds[nDim1] + 10.0f / g_qeglobals.d_xyz.scale, 0.0f, vMinBounds[nDim2]);

		glVertex3f(vMaxBounds[nDim1] + 10.0f / g_qeglobals.d_xyz.scale, 0.0f, vMinBounds[nDim2]);
		glVertex3f(vMaxBounds[nDim1] + 10.0f / g_qeglobals.d_xyz.scale, 0.0f, vMaxBounds[nDim2]);
  
		glVertex3f(vMaxBounds[nDim1] + 6.0f  / g_qeglobals.d_xyz.scale, 0.0f, vMaxBounds[nDim2]);
		glVertex3f(vMaxBounds[nDim1] + 10.0f / g_qeglobals.d_xyz.scale, 0.0f, vMaxBounds[nDim2]);

		glEnd();

		glRasterPos3f(Betwixt(vMinBounds[nDim1], vMaxBounds[nDim1]), 0.0f, vMinBounds[nDim2] - 20.0  / g_qeglobals.d_xyz.scale);
		sprintf(dimstr, g_pszDimStrings[nDim1], vSize[nDim1]);
		glCallLists(strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);
    
		glRasterPos3f(vMaxBounds[nDim1] + 16.0  / g_qeglobals.d_xyz.scale, 0.0f, Betwixt(vMinBounds[nDim2], vMaxBounds[nDim2]));
		sprintf(dimstr, g_pszDimStrings[nDim2], vSize[nDim2]);
		glCallLists(strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);

		glRasterPos3f(vMinBounds[nDim1] + 4, 0.0f, vMaxBounds[nDim2] + 8 / g_qeglobals.d_xyz.scale);
		sprintf(dimstr, g_pszOrgStrings[1], vMinBounds[nDim1], vMaxBounds[nDim2]);
		glCallLists(strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);
	}
	else
	{
		glBegin (GL_LINES);

		glVertex3f(0.0f, vMinBounds[nDim1], vMinBounds[nDim2] - 6.0f  / g_qeglobals.d_xyz.scale);
		glVertex3f(0.0f, vMinBounds[nDim1], vMinBounds[nDim2] - 10.0f / g_qeglobals.d_xyz.scale);

		glVertex3f(0.0f, vMinBounds[nDim1], vMinBounds[nDim2] - 10.0f / g_qeglobals.d_xyz.scale);
		glVertex3f(0.0f, vMaxBounds[nDim1], vMinBounds[nDim2] - 10.0f / g_qeglobals.d_xyz.scale);

		glVertex3f(0.0f, vMaxBounds[nDim1], vMinBounds[nDim2] - 6.0f  / g_qeglobals.d_xyz.scale);
		glVertex3f(0.0f, vMaxBounds[nDim1], vMinBounds[nDim2] - 10.0f / g_qeglobals.d_xyz.scale);
  
		glVertex3f(0.0f, vMaxBounds[nDim1] + 6.0f  / g_qeglobals.d_xyz.scale, vMinBounds[nDim2]);
		glVertex3f(0.0f, vMaxBounds[nDim1] + 10.0f / g_qeglobals.d_xyz.scale, vMinBounds[nDim2]);

		glVertex3f(0.0f, vMaxBounds[nDim1] + 10.0f / g_qeglobals.d_xyz.scale, vMinBounds[nDim2]);
		glVertex3f(0.0f, vMaxBounds[nDim1] + 10.0f / g_qeglobals.d_xyz.scale, vMaxBounds[nDim2]);
  
		glVertex3f(0.0f, vMaxBounds[nDim1] + 6.0f  / g_qeglobals.d_xyz.scale, vMaxBounds[nDim2]);
		glVertex3f(0.0f, vMaxBounds[nDim1] + 10.0f / g_qeglobals.d_xyz.scale, vMaxBounds[nDim2]);

		glEnd();

		glRasterPos3f(0.0f, Betwixt(vMinBounds[nDim1], vMaxBounds[nDim1]),  vMinBounds[nDim2] - 20.0 / g_qeglobals.d_xyz.scale);
		sprintf(dimstr, g_pszDimStrings[nDim1], vSize[nDim1]);
		glCallLists(strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);
    
		glRasterPos3f(0.0f, vMaxBounds[nDim1] + 16.0 / g_qeglobals.d_xyz.scale, Betwixt(vMinBounds[nDim2], vMaxBounds[nDim2]));
		sprintf(dimstr, g_pszDimStrings[nDim2], vSize[nDim2]);
		glCallLists(strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);

		glRasterPos3f(0.0f, vMinBounds[nDim1] + 4.0, vMaxBounds[nDim2] + 8 / g_qeglobals.d_xyz.scale);
		sprintf(dimstr, g_pszOrgStrings[2], vMinBounds[nDim1], vMaxBounds[nDim2]);
		glCallLists(strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);
	}
}

// sikk---> Light Radius
/*
==============
XY_DrawLightRadius
==============
*/
void XY_DrawLightRadius (brush_t *pBrush, int nViewType)
{
	float		f, fRadius;
	float		fOrigX, fOrigY, fOrigZ;
	double		fStep = (2.0f * Q_PI) / 200.0f;

	if (strncmp(pBrush->owner->eclass->name, "light", 5))
		return;

	// get the value of the "light" key
	fRadius = atof(ValueForKey(pBrush->owner, "light"));
	// if entity's "light" key is not found, default to 300
	if (!fRadius)
		fRadius = 300;

	// find the center
	fOrigX = (pBrush->mins[0] + ((pBrush->maxs[0] - pBrush->mins[0]) / 2));
	fOrigY = (pBrush->mins[1] + ((pBrush->maxs[1] - pBrush->mins[1]) / 2));
	fOrigZ = (pBrush->mins[2] + ((pBrush->maxs[2] - pBrush->mins[2]) / 2));

	glDisable(GL_LINE_STIPPLE);
	glLineWidth(1);

	glColor3f(g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES][0] * 0.5,
			  g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES][1] * 0.5,
			  g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES][2] * 0.5);
	glBegin(GL_LINE_STRIP);
	for (f = 0; f <= 8; f += fStep)
	{
		if (nViewType == XY)
			glVertex3f(fOrigX + fRadius * cos(f), fOrigY + fRadius * sin(f), fOrigZ);
		else if (nViewType == XZ)
			glVertex3f(fOrigX + fRadius * cos(f), fOrigY, fOrigZ + fRadius * sin(f));
		else
			glVertex3f(fOrigX, fOrigY + fRadius * cos(f), fOrigZ + fRadius * sin(f));
	}
	glEnd();

	glColor3f(g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES][0] * 0.75,
			  g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES][1] * 0.75,
			  g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES][2] * 0.75);
	glBegin(GL_LINE_STRIP);
	for (f = 0; f <= 8; f += fStep)
	{
		if (nViewType == XY)
			glVertex3f(fOrigX + (fRadius * 0.667) * cos(f), fOrigY + (fRadius * 0.667) * sin(f), fOrigZ);
		else if (nViewType == XZ)
			glVertex3f(fOrigX + (fRadius * 0.667) * cos(f), fOrigY, fOrigZ + (fRadius * 0.667) * sin(f));
		else
			glVertex3f(fOrigX, fOrigY + (fRadius * 0.667) * cos(f), fOrigZ + (fRadius * 0.667) * sin(f));
	}
	glEnd();

	glColor3fv(g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES]);
	glBegin(GL_LINE_STRIP);
	for (f = 0; f <= 8; f += fStep)
	{
		if (nViewType == XY)
			glVertex3f(fOrigX + (fRadius * 0.334) * cos(f), fOrigY + (fRadius * 0.334) * sin(f), fOrigZ);
		else if (nViewType == XZ)
			glVertex3f(fOrigX + (fRadius * 0.334) * cos(f), fOrigY, fOrigZ + (fRadius * 0.334) * sin(f));
		else
			glVertex3f(fOrigX, fOrigY + (fRadius * 0.334) * cos(f), fOrigZ + (fRadius * 0.334) * sin(f));
	}
	glEnd();

	glLineWidth(2);
	if (!g_qeglobals.d_savedinfo.bNoStipple)
		glEnable(GL_LINE_STIPPLE);
}
// <---sikk

/*
==============
XY_Draw
==============
*/
void XY_Draw ()
{
    brush_t	   *brush;
	float		w, h;
	entity_t   *e;
	double		start, end;
	vec3_t		mins, maxs;
	int			drawn, culled;
	int			i;
	int			nDim1, nDim2;
	int			nSaveDrawn;
	bool		bFixedSize;
	vec3_t		vMinBounds, vMaxBounds;

	if (!g_brActiveBrushes.next)
		return;	// not valid yet

	if (g_qeglobals.d_xyz.timing)
		start = Sys_DoubleTime();

	// clear
	g_qeglobals.d_xyz.d_dirty = false;

	glViewport(0, 0, g_qeglobals.d_xyz.width, g_qeglobals.d_xyz.height);
	glClearColor(g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDBACK][0], 
				 g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDBACK][1],
				 g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDBACK][2],
				 0);

    glClear(GL_COLOR_BUFFER_BIT);

	// set up viewpoint
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

	w = g_qeglobals.d_xyz.width / 2 / g_qeglobals.d_xyz.scale;
	h = g_qeglobals.d_xyz.height / 2 / g_qeglobals.d_xyz.scale;

	nDim1 = (g_qeglobals.d_nViewType == YZ) ? 1 : 0;
	nDim2 = (g_qeglobals.d_nViewType == XY) ? 1 : 2;

	mins[0] = g_qeglobals.d_xyz.origin[nDim1] - w;
	maxs[0] = g_qeglobals.d_xyz.origin[nDim1] + w;
	mins[1] = g_qeglobals.d_xyz.origin[nDim2] - h;
	maxs[1] = g_qeglobals.d_xyz.origin[nDim2] + h;

	glOrtho(mins[0], maxs[0], mins[1], maxs[1], 
			-g_qeglobals.d_savedinfo.nMapSize,  
			g_qeglobals.d_savedinfo.nMapSize);//;//-8192, 8192);	// sikk - Map Size

	// now draw the grid
	XY_DrawGrid();

	// draw block grid
	if (g_qeglobals.d_savedinfo.bShow_Blocks)
		XY_DrawBlockGrid();

	// draw stuff
	glShadeModel(GL_FLAT);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glColor3f(0, 0, 0);
//	glEnable (GL_LINE_SMOOTH);

	drawn = culled = 0;

	if (g_qeglobals.d_nViewType != XY)
	{
		glPushMatrix();
		if (g_qeglobals.d_nViewType == YZ)
			glRotatef(-90, 0, 1, 0);	// put Z going up
		glRotatef(-90, 1, 0, 0);	    // put Z going up
	}

//	e = NULL;
	e = g_peWorldEntity;
	for (brush = g_brActiveBrushes.next; brush != &g_brActiveBrushes; brush = brush->next)
	{
		if (brush->mins[nDim1] > maxs[0] || 
			brush->mins[nDim2] > maxs[1] || 
			brush->maxs[nDim1] < mins[0] || 
			brush->maxs[nDim2] < mins[1])
		{
			culled++;
			continue;		// off screen
		}

		if (FilterBrush(brush))
			continue;

		drawn++;

		if (brush->owner != e && brush->owner)
		{
//			e = brush->owner;
//			glColor3fv(e->eclass->color);
			glColor3fv(brush->owner->eclass->color);
		}
		else
			glColor3fv(g_qeglobals.d_savedinfo.v3Colors[COLOR_BRUSHES]);

		Brush_DrawXY(brush, g_qeglobals.d_nViewType);
	}

	DrawPathLines();

	// draw pointfile
	if (g_qeglobals.d_nPointfileDisplayList)
		glCallList(g_qeglobals.d_nPointfileDisplayList);

	// lunaran - why do this?
	/*
	if (!(g_qeglobals.d_nViewType == XY))
		glPopMatrix();

	// now draw selected brushes
	if (g_qeglobals.d_nViewType != XY)
	{
		glPushMatrix();
		if (g_qeglobals.d_nViewType == YZ)
			glRotatef(-90, 0, 1, 0);	    // put Z going up
		glRotatef(-90, 1, 0, 0);	    // put Z going up
	}
	*/

	glTranslatef(g_qeglobals.d_v3SelectTranslate[0], 
				 g_qeglobals.d_v3SelectTranslate[1], 
				 g_qeglobals.d_v3SelectTranslate[2]);

	if (g_bRotateCheck)	// sikk - Free Rotate
		glColor3f(0.8f, 0.1f, 0.9f);
	else if (g_bScaleCheck)	// sikk - Free Scaling
		glColor3f(0.1f, 0.8f, 0.1f);
	else
		glColor3fv(g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES]);
	if (!g_qeglobals.d_savedinfo.bNoStipple)
		glEnable(GL_LINE_STIPPLE);
	glLineStipple(3, 0xaaaa);
	glLineWidth(2);

	// paint size
	vMinBounds[0] = vMinBounds[1] = vMinBounds[2] = 8192.0f;
	vMaxBounds[0] = vMaxBounds[1] = vMaxBounds[2] = -8192.0f;

	nSaveDrawn = drawn;
	bFixedSize = false;

	for (brush = g_brSelectedBrushes.next; brush != &g_brSelectedBrushes; brush = brush->next)
	{
		drawn++;

		Brush_DrawXY(brush, g_qeglobals.d_nViewType);

// sikk---> Light Radius
		if (g_qeglobals.d_savedinfo.bShow_LightRadius)
			XY_DrawLightRadius(brush, g_qeglobals.d_nViewType);
// <---sikk

		// paint size
	    if (!bFixedSize)
	    {
			if (brush->owner->eclass->fixedsize)
				bFixedSize = true;
			if (g_qeglobals.d_savedinfo.bShow_SizeInfo)
			{
				for (i = 0; i < 3; i++)
				{
					if (brush->mins[i] < vMinBounds[i])
						vMinBounds[i] = brush->mins[i];
					if (brush->maxs[i] > vMaxBounds[i])
						vMaxBounds[i] = brush->maxs[i];
				}
			}
		}
	}

	glDisable(GL_LINE_STIPPLE);
	glLineWidth(1);

	// paint size
	if (!bFixedSize && drawn - nSaveDrawn > 0 && g_qeglobals.d_savedinfo.bShow_SizeInfo)
		XY_DrawSizeInfo(nDim1, nDim2, vMinBounds, vMaxBounds);

	// edge / vertex flags
	if (g_qeglobals.d_selSelectMode == sel_vertex)
	{
		glPointSize(4);
		glColor3f(0, 1, 0);
		glBegin(GL_POINTS);
		for (i = 0; i < g_qeglobals.d_nNumPoints; i++)
			glVertex3fv(g_qeglobals.d_v3Points[i]);
		glEnd();
		glPointSize(1);
	}
	else if (g_qeglobals.d_selSelectMode == sel_edge)
	{
		float	*v1, *v2;

		glPointSize(4);
		glColor3f(0, 0, 1);
		glBegin(GL_POINTS);
		for (i = 0; i < g_qeglobals.d_nNumEdges; i++)
		{
			v1 = g_qeglobals.d_v3Points[g_qeglobals.d_pEdges[i].p1];
			v2 = g_qeglobals.d_v3Points[g_qeglobals.d_pEdges[i].p2];
			glVertex3f((v1[0] + v2[0]) * 0.5, 
				       (v1[1] + v2[1]) * 0.5, 
					   (v1[2] + v2[2]) * 0.5);
		}
		glEnd();
		glPointSize(1);
	}

	glTranslatef(-g_qeglobals.d_v3SelectTranslate[0], 
		         -g_qeglobals.d_v3SelectTranslate[1], 
				 -g_qeglobals.d_v3SelectTranslate[2]);

	if (!(g_qeglobals.d_nViewType == XY))
		glPopMatrix();

	// clipper
	if (g_qeglobals.d_bClipMode)
		Clip_DrawPoint(XY);
	else if (GetKeyState(VK_MENU) < 0)
		XY_DrawRotateIcon();

	// now draw camera point
	XY_DrawCameraIcon();
	if (g_qeglobals.d_savedinfo.bShow_Z)	// sikk - Don't draw Z Icon if Z window is hidden
		XY_DrawZIcon();
	XY_DrawCoords();	// sikk - Draw Coords last so they are on top
    glFinish();
	QE_CheckOpenGLForErrors();

	if (g_qeglobals.d_xyz.timing)
	{
		end = Sys_DoubleTime();
		Sys_Printf("MSG: XYZ: %d ms\n", (int)(1000 * (end - start)));
	}
}

/*
==============
XY_Overlay
==============
*/
void XY_Overlay ()
{
	int				w, h;
	int				r[4];
	static vec3_t	lastz;
	static vec3_t	lastcamera;

	glViewport(0, 0, g_qeglobals.d_xyz.width, g_qeglobals.d_xyz.height);

	// set up viewpoint
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

	w = g_qeglobals.d_xyz.width / 2 / g_qeglobals.d_xyz.scale;
	h = g_qeglobals.d_xyz.height / 2 / g_qeglobals.d_xyz.scale;

	glOrtho(g_qeglobals.d_xyz.origin[0] - w, 
		    g_qeglobals.d_xyz.origin[0] + w, 
			g_qeglobals.d_xyz.origin[1] - h, 
			g_qeglobals.d_xyz.origin[1] + h, 
			-g_qeglobals.d_savedinfo.nMapSize, 
			g_qeglobals.d_savedinfo.nMapSize);// -8192, 8192); 	// sikk - Map Size

	// erase the old camera and z checker positions
	// if the entire xy hasn't been redrawn
	if (g_qeglobals.d_xyz.d_dirty)
	{
		glReadBuffer(GL_BACK);
		glDrawBuffer(GL_FRONT);

		glRasterPos2f(lastz[0] - 9, lastz[1] - 9);
		glGetIntegerv(GL_CURRENT_RASTER_POSITION, r);
		glCopyPixels(r[0], r[1], 18, 18, GL_COLOR);

		glRasterPos2f(lastcamera[0] - 50, lastcamera[1] - 50);
		glGetIntegerv(GL_CURRENT_RASTER_POSITION, r);
		glCopyPixels(r[0], r[1], 100, 100, GL_COLOR);
	}
	g_qeglobals.d_xyz.d_dirty = true;

	// save off underneath where we are about to draw
	VectorCopy(g_qeglobals.d_z.origin, lastz);
	VectorCopy(g_qeglobals.d_camera.origin, lastcamera);

	glReadBuffer(GL_FRONT);
	glDrawBuffer(GL_BACK);

	glRasterPos2f(lastz[0] - 9, lastz[1] - 9);
	glGetIntegerv(GL_CURRENT_RASTER_POSITION, r);
	glCopyPixels(r[0], r[1], 18, 18, GL_COLOR);

	glRasterPos2f(lastcamera[0] - 50, lastcamera[1] - 50);
	glGetIntegerv(GL_CURRENT_RASTER_POSITION,r);
	glCopyPixels(r[0], r[1], 100, 100, GL_COLOR);

	// draw the new icons
	glDrawBuffer(GL_FRONT);

    glShadeModel(GL_FLAT);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glColor3f(0, 0, 0);

	XY_DrawCameraIcon();
	XY_DrawZIcon();

	glDrawBuffer(GL_BACK);
    glFinish();
}

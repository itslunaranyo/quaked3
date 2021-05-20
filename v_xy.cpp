//==============================
//	xy.c
//==============================

#include "qe3.h"

const char *g_pszDimStrings[] = {"x:%.f", "y:%.f", "z:%.f"};
const char *g_pszOrgStrings[] = {"(x:%.f  y:%.f)", "(x:%.f  z:%.f)", "(y:%.f  z:%.f)"};

bool	g_bSnapCheck;
	

XYZView::XYZView() : nRotate(0)
{
	Init();
}

XYZView::~XYZView()
{
}


/*
============
XYZView::Init
============
*/
void XYZView::Init()
{
	SetBounds();
}

/*
==================
XYZView::SetAxis
==================
*/
void XYZView::SetAxis(int newViewType)
{
	dViewType = newViewType;

	nDim1 = (dViewType == YZ) ? 1 : 0;
	nDim2 = (dViewType == XY) ? 1 : 2;

	PositionView();
}

/*
==================
XYZView::PositionView
==================
*/
void XYZView::PositionView()
{
	Brush	   *b;

	b = g_brSelectedBrushes.next;
	if (b && b->next != b)
	{
		origin[nDim1] = b->mins[nDim1];
		origin[nDim2] = b->mins[nDim2];
	}
	else
	{
		origin[nDim1] = g_qeglobals.d_vCamera.origin[nDim1];
		origin[nDim2] = g_qeglobals.d_vCamera.origin[nDim2];
	}
	SetBounds();
}

/*
==================
XYZView::PositionAllViews
==================
*/
void XYZView::PositionAllViews()
{
	for (int i = 0; i < 4; i++)
		g_qeglobals.d_vXYZ[i].PositionView();
}

/*
==================
XYZView::CopyVector
==================
*/
void XYZView::CopyVector (const vec3 in, vec3 &out)
{
	if (dViewType == XY)
	{
		out[0] = in[0];
		out[1] = in[1];
	}
	else if (dViewType == XZ)
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


void XYZView::ScaleUp()
{
	scale = min(32.0f, scale * 1.25f);
	Sys_UpdateWindows(W_XY);
}
void XYZView::ScaleDown()
{
	scale = max(0.05f, scale * 0.8f);
	Sys_UpdateWindows(W_XY);
}

/*
============================================================================

  MOUSE ACTIONS

============================================================================
*/

//static int		buttonstate;
//static int		pressx, pressy;
//static bool		press_selection;
//static vec3		pressdelta;

/*
==================
XYZView::ToPoint
==================
*/
void const XYZView::ToPoint (const int x, const int y, vec3 &point)
{
	point[nDim1] = origin[nDim1] + (x - (float)width / 2) / scale;
	point[nDim2] = origin[nDim2] + (y - (float)height / 2) / scale;
	return;
}

void const XYZView::ToPoint(const int xIn, const int yIn, int &xOut, int &yOut)
{
	xOut = origin[nDim1] + (xIn - (float)width / 2) / scale;
	yOut = origin[nDim2] + (yIn - (float)height / 2) / scale;
	return;
}

/*
==================
XYZView::ToGridPoint
==================
*/
void const XYZView::ToGridPoint (const int x, const int y, vec3 &point)
{
	ToPoint(x, y, point);
	point[nDim1] = qround(point[nDim1], g_qeglobals.d_nGridSize);
	point[nDim2] = qround(point[nDim2], g_qeglobals.d_nGridSize);
	return;
}

void const XYZView::ToGridPoint(const int xIn, const int yIn, int &xOut, int &yOut)
{
	ToPoint(xIn, yIn, xOut, yOut);
	xOut = qround(xOut, g_qeglobals.d_nGridSize);
	yOut = qround(yOut, g_qeglobals.d_nGridSize);
	return;
}

/*
==================
XYZView::SnapToPoint
==================
*/
void const XYZView::SnapToPoint (const int x, const int y, vec3 &point)
{
	if (!g_qeglobals.bGridSnap)
		ToPoint(x, y, point);
	else
		ToGridPoint(x, y, point);
}

/*
==================
XYZView::SnapPoint
==================
*/
vec3 const XYZView::SnapPoint(const vec3 ptIn)
{
	if (!g_qeglobals.bGridSnap)
		return ptIn;

	vec3 ptOut;
	for (int i = 0; i < 3; i++)
	{
		ptOut[i] = floor((float)ptIn[i] / g_qeglobals.d_nGridSize + 0.5) * g_qeglobals.d_nGridSize;
	}
	return ptOut;
}

/*
==================
XYZView::GetBasis
==================
*/
bool XYZView::GetBasis(vec3 &right, vec3 &up, vec3 &forward)
{
	right = vec3(0);
	up = vec3(0);
	forward = vec3(0);

	forward[dViewType] = -1;
	right[nDim1] = 1;
	up[nDim2] = 1;

	return true;
}

/*
==================
XYZView::GetMouseContext
==================
*/
mouseContext_t const XYZView::GetMouseContext(const int x, const int y)
{
	mouseContext_t mc;

	ToPoint(x, y, mc.org);

	mc.pt = vec3(x, y, 0);
	mc.org[dViewType] = g_cfgEditor.MapSize / 2;
	mc.ray[dViewType] = -1;
	mc.right[nDim1] = 1;
	mc.up[nDim2] = 1;
	mc.dims = 2;	// 2D view

	return mc;
}





/*
==================
XYZView::DragDelta
==================
*/
/*
bool XYZView::DragDelta (int x, int y, vec3 move)
{
	vec3	xvec, yvec, delta;
	int		i;

	xvec[0] = 1 / scale;
	xvec[1] = xvec[2] = 0;
	yvec[1] = 1 / scale;
	yvec[0] = yvec[2] = 0;

	for (i = 0; i < 3; i++)
	{
		delta[i] = xvec[i] * (x - pressx) + yvec[i] * (y - pressy);

		if (g_qeglobals.bGridSnap)
			delta[i] = floor(delta[i] / g_qeglobals.d_nGridSize + 0.5) * g_qeglobals.d_nGridSize;		
	}
	move = delta - pressdelta;
	pressdelta = delta;

	if (move[0] || move[1] || move[2])
		return true;

	return false;
}
*/
/*
==============
XYZView::DragNewBrush
==============
*/
/*
void XYZView::DragNewBrush (int x, int y)
{
	vec3		mins, maxs, junk;
	int			i;
	float		temp;
	Brush	   *n;

	if (!DragDelta(x, y, junk))
		return;

	// delete the current selection
	if (Selection::HasBrushes())
		delete g_brSelectedBrushes.next;

	SnapToPoint(pressx, pressy, mins);
	mins[dViewType] = g_qeglobals.d_nGridSize * ((int)(g_qeglobals.d_v3WorkMin[dViewType] / g_qeglobals.d_nGridSize));

	SnapToPoint(x, y, maxs);
	maxs[dViewType] = g_qeglobals.d_nGridSize * ((int)(g_qeglobals.d_v3WorkMax[dViewType] / g_qeglobals.d_nGridSize));

	if (maxs[dViewType] <= mins[dViewType])
		maxs[dViewType] = mins[dViewType] + g_qeglobals.d_nGridSize;

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

	n = Brush::Create(mins, maxs, &g_qeglobals.d_workTexDef);
	if (!n)
		return;

	n->AddToList(&g_brSelectedBrushes);
	g_map.world->LinkBrush(n);
	n->Build();

	Sys_UpdateWindows(W_XY | W_Z | W_CAMERA);
}
*/
/*
==============
XYZView::MouseDown
==============
*/
void XYZView::MouseDown (int x, int y, int buttons)
{
	vec3	point;
	vec3	orgLocal, dir, right, up;

	//int nAngle;

	//buttonstate = buttons;
	//pressx = x;
	//pressy = y;

	//pressdelta = vec3(0);
	ToPoint(x, y, point);
	orgLocal = point;

	orgLocal[dViewType] = 8192;
	dir[dViewType] = -1;
	right[nDim1] = 1 / scale;
	up[nDim2] = 1 / scale;

	//press_selection = (Selection::HasBrushes());

	Sys_GetCursorPos(&cursorX, &cursorY);

	if (buttons & MK_MBUTTON)
	{
		// Ctrl+MMB = place camera
		// Alt+MMB = place camera and drag to aim
		if (buttons & MK_CONTROL || GetKeyState(VK_MENU) < 0)
		{
			CopyVector(point, g_qeglobals.d_vCamera.origin);

			Sys_UpdateWindows(W_SCENE);
			return;
		}
		// Shift+MMB = place z checker
		else if (buttons & MK_SHIFT)
		{
			SnapToPoint(x, y, point);
			CopyVector(point, g_qeglobals.d_vZ.origin);
			Sys_UpdateWindows(W_XY | W_Z);
			return;
		}
		// MMB = angle camera
		else if (buttons == MK_MBUTTON)
		{
			AngleCamera(point - g_qeglobals.d_vCamera.origin);
			return;
		}
	}
	/*
// sikk - Undo/Redo for Free Rotate & Free Scale
	if (buttonstate & MK_RBUTTON)
	{
		if (GetKeyState(VK_SHIFT) < 0)
		{
			Undo::Start("Free Scale");
			Undo::AddBrushList(&g_brSelectedBrushes);
		}	
		if (GetKeyState(VK_MENU) < 0)
		{
			Undo::Start("Free Rotate");
			Undo::AddBrushList(&g_brSelectedBrushes);
		}
	}
// <---sikk
*/
}


/*
==============
XYZView::MouseUp
==============
*/
void XYZView::MouseUp (int x, int y, int buttons)
{
//	Drag_MouseUp();

	//if (!press_selection)
	//	Sys_UpdateWindows(W_SCENE);
	/*
// sikk--->	Free Rotate & Free Scaling
	if (g_bRotateCheck || g_bScaleCheck)
	{
		nRotate = 0;

		if (g_bSnapCheck)
		{
			g_qeglobals.bGridSnap = true;
			g_bSnapCheck = false;
		}

		g_bRotateCheck = false;
		g_bScaleCheck = false;

// sikk - Undo/Redo for Free Rotate & Free Scale
		//Undo::EndBrushList(&g_brSelectedBrushes);
		//Undo::End();
// <---sikk

		Sys_UpdateWindows(W_XY | W_Z);
	}
// <---sikk
*/

	//buttonstate = 0;
}

/*
==============
XYZView::MouseMoved
==============
*/
void XYZView::MouseMoved (int x, int y, int buttons)
{
	vec3	point;
    int		nAngle;			
	vec3	tdp;
	char	xystring[256];
	int		cx, cy;

	//char	szRotate[16];	// sikk - Free Rotate

	Sys_GetCursorPos(&cx, &cy);
	if (cx == cursorX && cy == cursorY)
		return;

	if (!buttons || buttons ^ MK_RBUTTON)
	{
		SnapToPoint( x, y, tdp);
		sprintf(xystring, "xyz Coordinates: (%d %d %d)", (int)tdp[0], (int)tdp[1], (int)tdp[2]);
		Sys_Status(xystring, 0);
	}

	if (!buttons)
		return;
	/*
	// LMB without selection = drag new brush
	if (buttons == MK_LBUTTON && !press_selection)
	{
	//	DragNewBrush(x, y);

		// update g_v3RotateOrigin to new brush (for when 'quick move' is used)
		//g_v3RotateOrigin = Selection::GetTrueMid();	// sikk - Free Rotate
		return;
	}

	// LMB (possibly with control and or shift)
	// with selection = drag selection
	if (buttons & MK_LBUTTON)
	{
	//	Drag_MouseMoved(x, y, buttons);
		
		// update g_v3RotateOrigin to new brush
		//g_v3RotateOrigin = Selection::GetTrueMid();	// sikk - Free Rotate

		Sys_UpdateWindows(W_XY | W_CAMERA | W_Z);
		return;
	}
	*/
	// Ctrl+MMB = move camera
	if (buttons == (MK_CONTROL | MK_MBUTTON))
	{
		SnapToPoint( x, y, point);
		CopyVector(point, g_qeglobals.d_vCamera.origin);

		Sys_UpdateWindows(W_XY | W_CAMERA | W_Z);
		return;
	}

	// Shift+MMB = move z checker
	if (buttons == (MK_SHIFT | MK_MBUTTON))
	{
		SnapToPoint( x, y, point);

		CopyVector(point, g_qeglobals.d_vZ.origin);

		Sys_UpdateWindows(W_XY | W_Z);
		return;
	}

	// MMB = angle camera
	if (buttons == MK_MBUTTON)
	{
		/*
// sikk---> Free Rotate: Pivot Icon
		// Alt+MMB = move free rotate pivot icon
		if (GetKeyState(VK_MENU) < 0)
		{
			SnapToPoint( x, y, point);
			CopyVector(point, g_v3RotateOrigin);

			Sys_UpdateWindows(W_XY);
			return;
		}
// <---sikk
		else
		{*/
			SnapToPoint( x, y, point);
			point = point - g_qeglobals.d_vCamera.origin;

			nAngle = (dViewType == XY) ? YAW : PITCH;

			if (point[nDim1] || point[nDim2])
			{
				g_qeglobals.d_vCamera.angles[nAngle] = 180 / Q_PI * atan2(point[nDim2], point[nDim1]);
				g_qeglobals.d_vCamera.BoundAngles();
				Sys_UpdateWindows(W_XY | W_CAMERA);
			}
		//}
		return;
	}

	// RMB = drag xy origin
	if (buttons == MK_RBUTTON)
	{
		SetCursor(NULL); // sikk - Remove Cursor

		/*
// sikk---> Free Rotate
		// Alt+RMB = free rotate selected brush
		if (GetKeyState(VK_MENU) < 0)
		{
			g_bRotateCheck = true;

			if (g_qeglobals.bGridSnap)
			{
				g_qeglobals.bGridSnap = false;
				g_bSnapCheck = true;
			}
				
			switch (dViewType)
			{
			case XY:
				cx -= cursorX;
				nRotate += cx;
				Transform_RotateAxis(2, cx, true);
				break;
			case XZ:
				cx = cursorX - cx;
				nRotate += -cx;
				Transform_RotateAxis(1, cx, true);
				break;
			case YZ:
				cx -= cursorX;
				nRotate += cx;
				Transform_RotateAxis(0, cx, true);
				break;
			}

			if (nRotate >= 360)
				nRotate -= 360;
			if (nRotate < -360)
				nRotate += 360;

			Sys_SetCursorPos(cursorX, cursorY);

			// sikk - this ensures that the windows are updated in realtime
			Sys_ForceUpdateWindows(W_SCENE);

			sprintf(szRotate, "Rotate: %d°", nRotate);
			Sys_Status(szRotate, 0);
		}
// <---sikk
		else
		{
		*/
			if (cx != cursorX || cy != cursorY)
			{
				origin[nDim1] -= (cx - cursorX) / scale;
				origin[nDim2] += (cy - cursorY) / scale;
				SetBounds();

				Sys_SetCursorPos(cursorX, cursorY);
				Sys_UpdateWindows(W_XY| W_Z);

				sprintf(xystring, "this Origin: (%d %d %d)", (int)origin[0], (int)origin[1], (int)origin[2]);
				Sys_Status(xystring, 0);
			}
		//}
		return;
	}
		
	/*
// sikk---> Free Scaling
	// Shift+RMB = free scale selected brush
	if (buttons == (MK_SHIFT | MK_RBUTTON))
	{
		int i;

		g_bScaleCheck = true;

		if (g_qeglobals.bGridSnap)
		{
			g_qeglobals.bGridSnap = false;
			g_bSnapCheck = true;
		}
			
		SetCursor(NULL); // sikk - Remove Cursor

		i = cursorY - cy;

		if (i < 0)
		{
			if (g_qeglobals.d_savedinfo.bScaleLockX)
				Transform_Scale(0.9f, 1.0f, 1.0f);
			if (g_qeglobals.d_savedinfo.bScaleLockY)
				Transform_Scale(1.0f, 0.9f, 1.0f);
			if (g_qeglobals.d_savedinfo.bScaleLockZ)
				Transform_Scale(1.0f, 1.0f, 0.9f);
		}
		if (i > 0)
		{
			if (g_qeglobals.d_savedinfo.bScaleLockX)
				Transform_Scale(1.1f, 1.0f, 1.0f);
			if (g_qeglobals.d_savedinfo.bScaleLockY)
				Transform_Scale(1.0f, 1.1f, 1.0f);
			if (g_qeglobals.d_savedinfo.bScaleLockZ)
				Transform_Scale(1.0f, 1.0f, 1.1f);
		}
			
		Sys_SetCursorPos(cursorX, cursorY);
//			cursorY = y;

		// sikk - this ensures that the windows are updated in realtime
		Sys_ForceUpdateWindows(W_SCENE);

		return;
	}
// <---sikk
*/
// sikk---> Mouse Zoom
	// Ctrl+RMB = zoom xy view
	if (buttons == (MK_CONTROL | MK_RBUTTON))
	{
		SetCursor(NULL); // sikk - Remove Cursor
			
		if (cy != cursorY)
		{
			if (cy > cursorY)
				scale *= powf(1.01f, fabs(cy - cursorY));
			else
				scale *= powf(0.99f, fabs(cy - cursorY));

			scale = max(0.05f, min(scale, 32.0f));


			Sys_SetCursorPos(cursorX, cursorY);
			Sys_UpdateWindows(W_XY);
		}
		return;
	}
// <---sikk
}





/*
==============
XYZView::AngleCamera
==============
*/
void XYZView::AngleCamera(vec3 point)
{
	int nAngle;
	nAngle = (dViewType == XY) ? YAW : PITCH;

	if (point[nDim2] || point[nDim1])
	{
		g_qeglobals.d_vCamera.angles[nAngle] = 180 / Q_PI * atan2(point[nDim2], point[nDim1]);
		g_qeglobals.d_vCamera.BoundAngles();
		Sys_UpdateWindows(W_SCENE);
	}
	return;
}






/*
============================================================================

	DRAWING

============================================================================
*/

/*
==============
XYZView::Draw2D
==============
*/
void XYZView::DrawGrid ()
{
	float	x, y, xb, xe, yb, ye;
	int		majorSize;

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	if (g_qeglobals.d_nGridSize == 128)
		majorSize = 128;
	else if (g_qeglobals.d_nGridSize == 256)
		majorSize = 256;
	else
		majorSize = 64;

	xb = max(vMins[nDim1], g_map.regionMins[nDim1]);
	xb = majorSize * floor(xb / majorSize);

	xe = min(vMaxs[nDim1], g_map.regionMaxs[nDim1]);
	xe = majorSize * ceil(xe / majorSize);

	yb = max(vMins[nDim2], g_map.regionMins[nDim2]);
	yb = majorSize * floor(yb / majorSize);

	ye = min(vMaxs[nDim2], g_map.regionMaxs[nDim2]);
	ye = majorSize * ceil(ye / majorSize);

	// draw major blocks
	glColor3fv(&g_colors.gridMajor.r);

	if (g_qeglobals.d_bShowGrid)
	{
		glBegin(GL_LINES);
		
		for (x = xb; x <= xe; x += majorSize)
		{
			glVertex2f(x, yb);
			glVertex2f(x, ye);
		}
		for (y = yb; y <= ye; y += majorSize)
		{
			glVertex2f(xb, y);
			glVertex2f(xe, y);
		}
		glEnd();

		// draw minor blocks
		if (g_qeglobals.d_nGridSize * scale >= 4)
		{
			glColor3fv(&g_colors.gridMinor.r);

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
		// lunaran - grid axis now block color, not grid major * 65%
		glColor3fv(&g_colors.gridBlock.r);
		glBegin(GL_LINES);
		glVertex2f(xb, 0);
		glVertex2f(xe, 0);
		glVertex2f(0, yb);
		glVertex2f(0, ye);
		glEnd();
	}

	// show current work zone?
	// the work zone is used to place dropped points and brushes
	if (g_cfgUI.ShowWorkzone)
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

	DrawBlockGrid();
}

/*
==============
XYZView::DrawBlockGrid
==============
*/
void XYZView::DrawBlockGrid ()
{
	if (!g_cfgUI.ShowBlocks)
		return;
	
	float	x, y, xb, xe, yb, ye;
	int		w, h;
	char	text[32];

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	w = width / 2 / scale;
	h = height / 2 / scale;

	xb = origin[nDim1] - w;
	if (xb < g_map.regionMins[nDim1])
		xb = g_map.regionMins[nDim1];
	xb = 1024 * floor(xb / 1024);

	xe = origin[nDim1] + w;
	if (xe > g_map.regionMaxs[nDim1])
		xe = g_map.regionMaxs[nDim1];
	xe = 1024 * ceil(xe / 1024);

	yb = origin[nDim2] - h;
	if (yb < g_map.regionMins[nDim2])
		yb = g_map.regionMins[nDim2];
	yb = 1024 * floor(yb / 1024);

	ye = origin[nDim2] + h;
	if (ye > g_map.regionMaxs[nDim2])
		ye = g_map.regionMaxs[nDim2];
	ye = 1024 * ceil(ye / 1024);

	// draw major blocks
	glColor3fv(&g_colors.gridBlock.r);
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
XYZView::DrawViewName
sikk: View Axis - cleaner, more intuitive look
==============
*/
void XYZView::DrawViewName()
{
	// lunaran: always draw view name if we have one grid view, and never in 3-view mode
	if ((!g_qeglobals.d_wndGrid[1] || g_qeglobals.d_wndGrid[1]->Open()) &&
		(!g_qeglobals.d_wndGrid[2] || g_qeglobals.d_wndGrid[2]->Open()) &&
		(!g_qeglobals.d_wndGrid[3] || g_qeglobals.d_wndGrid[3]->Open()))
		return;

	float *p1, *p2;
	float fColor[][3] = { { 1.0f, 0.0f, 0.0f },{ 0.0f, 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } };
	char *szView;
	int w, h;

	w = width / 2 / scale;
	h = height / 2 / scale;

	if (dViewType == XY)
	{
		p1 = fColor[0];
		p2 = fColor[1];
		szView = "XY";
	}
	else if (dViewType == XZ)
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

	glColor3fv(&g_colors.gridText.r);
	glRasterPos2f(origin[nDim1] - w + 68 / scale,
		origin[nDim2] + h - 51 / scale);
	glCallLists(1, GL_UNSIGNED_BYTE, &szView[0]);

	glColor3fv(&g_colors.gridText.r);
	glRasterPos2f(origin[nDim1] - w + 44 / scale,
		origin[nDim2] + h - 28 / scale);
	glCallLists(1, GL_UNSIGNED_BYTE, &szView[1]);

	glLineWidth(2);
	glColor3fv(p1);
	glBegin(GL_LINES);
	glVertex2f(origin[nDim1] - w + 48 / scale,
		origin[nDim2] + h - 48 / scale);
	glVertex2f(origin[nDim1] - w + 64 / scale,
		origin[nDim2] + h - 48 / scale);
	glEnd();

	glColor3fv(p2);
	glBegin(GL_LINES);
	glVertex2f(origin[nDim1] - w + 48 / scale,
		origin[nDim2] + h - 48 / scale);
	glVertex2f(origin[nDim1] - w + 48 / scale,
		origin[nDim2] + h - 32 / scale);
	glEnd();
	glLineWidth(1);
}

/*
==============
XYZView::DrawCoords
==============
*/
void XYZView::DrawCoords()
{
	float	x, y, xb, xe, yb, ye;
	int		w, h;
	char	text[8];
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

	w = width / 2 / scale;
	h = height / 2 / scale;

	xb = origin[nDim1] - w;
	if (xb < g_map.regionMins[nDim1])
		xb = g_map.regionMins[nDim1];
	xb = nSize * floor(xb / nSize);

	xe = origin[nDim1] + w;
	if (xe > g_map.regionMaxs[nDim1])
		xe = g_map.regionMaxs[nDim1];
	xe = nSize * ceil(xe / nSize);

	yb = origin[nDim2] - h;
	if (yb < g_map.regionMins[nDim2])
		yb = g_map.regionMins[nDim2];
	yb = nSize * floor(yb / nSize);

	ye = origin[nDim2] + h;
	if (ye > g_map.regionMaxs[nDim2])
		ye = g_map.regionMaxs[nDim2];
	ye = nSize * ceil(ye / nSize);

// sikk---> Filter Coords so they don't become bunched and unreadable 
	// TODO: Fix the bug that makes next coord off screen draw in various locations. 
	// draw coordinate text if needed
	if (g_cfgUI.ShowCoordinates)
	{
		glColor3fv(&g_colors.gridText.r);

		nSize = 64;
		if (scale <= 0.6)
			nSize = 128;
		if (scale <= 0.3)
			nSize = 256;
		if (scale <= 0.15)
			nSize = 512;
		if (scale <= 0.075)
			nSize = 1024;

		xb = origin[nDim1] - w;
		if (xb < g_map.regionMins[nDim1])
			xb = g_map.regionMins[nDim1];
		xb = nSize * floor(xb / nSize);

		xe = origin[nDim1] + w;
		if (xe > g_map.regionMaxs[nDim1])
			xe = g_map.regionMaxs[nDim1];
		xe = nSize * ceil(xe / nSize);

		yb = origin[nDim2] - h;
		if (yb < g_map.regionMins[nDim2])
			yb = g_map.regionMins[nDim2];
		yb = nSize * floor(yb / nSize);

		ye = origin[nDim2] + h;
		if (ye > g_map.regionMaxs[nDim2])
			ye = g_map.regionMaxs[nDim2];
		ye = nSize * ceil(ye / nSize);

		for (x = xb; x <= xe; x += nSize)	// sikk - 'x <= xe' instead of 'x < xe' so last coord is drawn 
		{
			glRasterPos2f(x, origin[nDim2] + h - 6 / scale);
			sprintf(text, "%d",(int)x);
			glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);
		}
		for (y = yb; y <= ye; y += nSize)	// sikk - 'y <= ye' instead of 'y < ye' so last coord is drawn
		{
			glRasterPos2f(origin[nDim1] - w + 1, y);
			sprintf(text, "%d",(int)y);
			glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);
		}
	}
// <---sikk

	DrawViewName();	

/*
// sikk---> Show Axis in center of view
	if (g_qeglobals.d_savedinfo.bShow_Axis)
	{
		glLineWidth(2);
		if (dViewType == XY)
			glColor3f(1.0, 0.0, 0.0);	
		else if (dViewType == XZ)
			glColor3f(1.0, 0.0, 0.0);
		else
			glColor3f(0.0, 1.0, 0.0);
		glBegin(GL_LINES);
		glVertex2f(0, 0);
		glVertex2f(64,0);
		glEnd();
			
		if (dViewType == XY)
			glColor3f(0.0, 1.0, 0.0);
		else if (dViewType == XZ)
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
XYZView::DrawCameraIcon
==================
*/
void XYZView::DrawCameraIcon ()
{
	float	x, y, a;
//	char	text[128];

	if (dViewType == XY)
	{
		x = g_qeglobals.d_vCamera.origin[0];
		y = g_qeglobals.d_vCamera.origin[1];
		a = g_qeglobals.d_vCamera.angles[YAW] / 180 * Q_PI;
	}
	else if (dViewType == YZ)
	{
		x = g_qeglobals.d_vCamera.origin[1];
		y = g_qeglobals.d_vCamera.origin[2];
		a = g_qeglobals.d_vCamera.angles[PITCH] / 180 * Q_PI;
	}
	else
	{
		x = g_qeglobals.d_vCamera.origin[0];
		y = g_qeglobals.d_vCamera.origin[2];
		a = g_qeglobals.d_vCamera.angles[PITCH] / 180 * Q_PI;
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
//	sprintf(text, "angle: %f", g_qeglobals.d_vCamera.angles[YAW]);
//	glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);
}

/*
==================
XYZView::DrawZIcon
==================
*/
void XYZView::DrawZIcon ()
{
	float x, y;

	if (dViewType != XY)
		return;

	x = g_qeglobals.d_vZ.origin[0];
	y = g_qeglobals.d_vZ.origin[1];

	glDisable(GL_DEPTH_TEST);
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

/*
==================
XYZView::DrawTools
==================
*/
bool XYZView::DrawTools()
{
	for (auto tIt = g_qeglobals.d_tools.rbegin(); tIt != g_qeglobals.d_tools.rend(); ++tIt)
	{
		if ((*tIt)->Draw2D(*this))
			return true;
	}
	return false;
}

// sikk---> Free Rotate: Pivot Icon
/*
===============
XYZView::DrawRotateIcon
===============
*/
void XYZView::DrawRotateIcon ()
{
	/*
	float x, y;

	if (dViewType == XY)
	{
		x = g_v3RotateOrigin[0];
		y = g_v3RotateOrigin[1];
	}
	else if (dViewType == YZ)
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
	*/
}
// <---sikk

/*
==================
XYZView::DrawSizeInfo

lunaran TODO: simplify
==================
*/
void XYZView::DrawSizeInfo (const vec3 vMinBounds, const vec3 vMaxBounds)
{
	if (!g_cfgUI.ShowSizeInfo)
		return;

	vec3	vSize;
	char	dimstr[128];

	vSize = vMaxBounds - vMinBounds;

	glColor3f(g_colors.selection[0] * .65, 
			  g_colors.selection[1] * .65,
			  g_colors.selection[2] * .65);

	if (dViewType == XY)
	{
		glBegin(GL_LINES);

		glVertex3f(vMinBounds[nDim1], vMinBounds[nDim2] - 6.0f  / scale, 0.0f);
		glVertex3f(vMinBounds[nDim1], vMinBounds[nDim2] - 10.0f / scale, 0.0f);

		glVertex3f(vMinBounds[nDim1], vMinBounds[nDim2] - 10.0f / scale, 0.0f);
		glVertex3f(vMaxBounds[nDim1], vMinBounds[nDim2] - 10.0f / scale, 0.0f);

		glVertex3f(vMaxBounds[nDim1], vMinBounds[nDim2] - 6.0f  / scale, 0.0f);
		glVertex3f(vMaxBounds[nDim1], vMinBounds[nDim2] - 10.0f / scale, 0.0f);
  
		glVertex3f(vMaxBounds[nDim1] + 6.0f  / scale, vMinBounds[nDim2], 0.0f);
		glVertex3f(vMaxBounds[nDim1] + 10.0f / scale, vMinBounds[nDim2], 0.0f);

		glVertex3f(vMaxBounds[nDim1] + 10.0f / scale, vMinBounds[nDim2], 0.0f);
		glVertex3f(vMaxBounds[nDim1] + 10.0f / scale, vMaxBounds[nDim2], 0.0f);
  
		glVertex3f(vMaxBounds[nDim1] + 6.0f  / scale, vMaxBounds[nDim2], 0.0f);
		glVertex3f(vMaxBounds[nDim1] + 10.0f / scale, vMaxBounds[nDim2], 0.0f);

		glEnd();

		glColor4f(g_colors.selection[0],
			g_colors.selection[1],
			g_colors.selection[2],
			1.0f);

		glRasterPos3f((vMinBounds[nDim1] + vMaxBounds[nDim1]) * 0.5f, vMinBounds[nDim2] - 20.0 / scale, 0.0f);
		sprintf(dimstr, g_pszDimStrings[nDim1], vSize[nDim1]);
		glCallLists(strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);
    
		glRasterPos3f(vMaxBounds[nDim1] + 16.0 / scale, (vMinBounds[nDim2] + vMaxBounds[nDim2]) * 0.5f, 0.0f);
		sprintf(dimstr, g_pszDimStrings[nDim2], vSize[nDim2]);
		glCallLists(strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);

		glRasterPos3f(vMinBounds[nDim1] + 4, vMaxBounds[nDim2] + 8 / scale, 0.0f);
		sprintf(dimstr, g_pszOrgStrings[0], vMinBounds[nDim1], vMaxBounds[nDim2]);
		glCallLists(strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);
	}
	else if (dViewType == XZ)
	{
		glBegin(GL_LINES);

		glVertex3f(vMinBounds[nDim1], 0.0f, vMinBounds[nDim2] - 6.0f  / scale);
		glVertex3f(vMinBounds[nDim1], 0.0f, vMinBounds[nDim2] - 10.0f / scale);

		glVertex3f(vMinBounds[nDim1], 0.0f, vMinBounds[nDim2] - 10.0f / scale);
		glVertex3f(vMaxBounds[nDim1], 0.0f, vMinBounds[nDim2] - 10.0f / scale);

		glVertex3f(vMaxBounds[nDim1], 0.0f, vMinBounds[nDim2] - 6.0f  / scale);
		glVertex3f(vMaxBounds[nDim1], 0.0f, vMinBounds[nDim2] - 10.0f / scale);
  
		glVertex3f(vMaxBounds[nDim1] + 6.0f  / scale, 0.0f, vMinBounds[nDim2]);
		glVertex3f(vMaxBounds[nDim1] + 10.0f / scale, 0.0f, vMinBounds[nDim2]);

		glVertex3f(vMaxBounds[nDim1] + 10.0f / scale, 0.0f, vMinBounds[nDim2]);
		glVertex3f(vMaxBounds[nDim1] + 10.0f / scale, 0.0f, vMaxBounds[nDim2]);
  
		glVertex3f(vMaxBounds[nDim1] + 6.0f  / scale, 0.0f, vMaxBounds[nDim2]);
		glVertex3f(vMaxBounds[nDim1] + 10.0f / scale, 0.0f, vMaxBounds[nDim2]);

		glEnd();

		glColor3fv((GLfloat*)&g_colors.selection);
		glRasterPos3f((vMinBounds[nDim1] + vMaxBounds[nDim1]) * 0.5f, 0.0f, vMinBounds[nDim2] - 20.0  / scale);
		sprintf(dimstr, g_pszDimStrings[nDim1], vSize[nDim1]);
		glCallLists(strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);
    
		glRasterPos3f(vMaxBounds[nDim1] + 16.0  / scale, 0.0f, (vMinBounds[nDim2]+ vMaxBounds[nDim2]) * 0.5f);
		sprintf(dimstr, g_pszDimStrings[nDim2], vSize[nDim2]);
		glCallLists(strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);

		glRasterPos3f(vMinBounds[nDim1] + 4, 0.0f, vMaxBounds[nDim2] + 8 / scale);
		sprintf(dimstr, g_pszOrgStrings[1], vMinBounds[nDim1], vMaxBounds[nDim2]);
		glCallLists(strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);
	}
	else
	{
		glBegin (GL_LINES);

		glVertex3f(0.0f, vMinBounds[nDim1], vMinBounds[nDim2] - 6.0f  / scale);
		glVertex3f(0.0f, vMinBounds[nDim1], vMinBounds[nDim2] - 10.0f / scale);

		glVertex3f(0.0f, vMinBounds[nDim1], vMinBounds[nDim2] - 10.0f / scale);
		glVertex3f(0.0f, vMaxBounds[nDim1], vMinBounds[nDim2] - 10.0f / scale);

		glVertex3f(0.0f, vMaxBounds[nDim1], vMinBounds[nDim2] - 6.0f  / scale);
		glVertex3f(0.0f, vMaxBounds[nDim1], vMinBounds[nDim2] - 10.0f / scale);
  
		glVertex3f(0.0f, vMaxBounds[nDim1] + 6.0f  / scale, vMinBounds[nDim2]);
		glVertex3f(0.0f, vMaxBounds[nDim1] + 10.0f / scale, vMinBounds[nDim2]);

		glVertex3f(0.0f, vMaxBounds[nDim1] + 10.0f / scale, vMinBounds[nDim2]);
		glVertex3f(0.0f, vMaxBounds[nDim1] + 10.0f / scale, vMaxBounds[nDim2]);
  
		glVertex3f(0.0f, vMaxBounds[nDim1] + 6.0f  / scale, vMaxBounds[nDim2]);
		glVertex3f(0.0f, vMaxBounds[nDim1] + 10.0f / scale, vMaxBounds[nDim2]);

		glEnd();

		glColor3fv((GLfloat*)&g_colors.selection);
		glRasterPos3f(0.0f, (vMinBounds[nDim1]+ vMaxBounds[nDim1]) * 0.5f,  vMinBounds[nDim2] - 20.0 / scale);
		sprintf(dimstr, g_pszDimStrings[nDim1], vSize[nDim1]);
		glCallLists(strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);
    
		glRasterPos3f(0.0f, vMaxBounds[nDim1] + 16.0 / scale, (vMinBounds[nDim2] + vMaxBounds[nDim2]) * 0.5f);
		sprintf(dimstr, g_pszDimStrings[nDim2], vSize[nDim2]);
		glCallLists(strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);

		glRasterPos3f(0.0f, vMinBounds[nDim1] + 4.0, vMaxBounds[nDim2] + 8 / scale);
		sprintf(dimstr, g_pszOrgStrings[2], vMinBounds[nDim1], vMaxBounds[nDim2]);
		glCallLists(strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);
	}
}

// sikk---> Light Radius
/*
==============
XYZView::DrawLightRadius
==============
*/
void XYZView::DrawLightRadius (Brush *pBrush, int nViewType)
{
	float		f, fRadius, fWait;
	float		fOrigX, fOrigY, fOrigZ;
	double		fStep = (2.0f * Q_PI) / 200.0f;

	if (!(pBrush->owner->eclass->showFlags & EFL_LIGHT))
		return;

	// get the value of the "light" key
	fRadius = pBrush->owner->GetKeyValueFloat("light");
	fWait = pBrush->owner->GetKeyValueFloat("wait");
	// if entity's "light" key is not found, default to 300
	if (!fRadius)
		fRadius = 300;
	if (fWait)
		fRadius /= fWait;

	// find the center
	fOrigX = (pBrush->mins[0] + ((pBrush->maxs[0] - pBrush->mins[0]) / 2));
	fOrigY = (pBrush->mins[1] + ((pBrush->maxs[1] - pBrush->mins[1]) / 2));
	fOrigZ = (pBrush->mins[2] + ((pBrush->maxs[2] - pBrush->mins[2]) / 2));

	glDisable(GL_LINE_STIPPLE);
	glLineWidth(1);

	glColor3f(g_colors.selection[0] * 0.5,
			  g_colors.selection[1] * 0.5,
			  g_colors.selection[2] * 0.5);
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

	glColor3f(g_colors.selection[0] * 0.75,
			  g_colors.selection[1] * 0.75,
			  g_colors.selection[2] * 0.75);
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

	glColor4f(g_colors.selection[0],
		g_colors.selection[1],
		g_colors.selection[2],
		1.0f);
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
	if (g_cfgUI.Stipple)
		glEnable(GL_LINE_STIPPLE);
}
// <---sikk


/*
==============
XYZView::SetBounds
==============
*/
void XYZView::SetBounds()
{
	float		w, h;
	w = width / 2 / scale;
	h = height / 2 / scale;

	vMins[nDim1] = origin[nDim1] - w;
	vMaxs[nDim1] = origin[nDim1] + w;
	vMins[nDim2] = origin[nDim2] - h;
	vMaxs[nDim2] = origin[nDim2] + h;
	vMins[dViewType] = -g_cfgEditor.MapSize;
	vMaxs[dViewType] = g_cfgEditor.MapSize;
}

/*
==============
XYZView::CullBrush
==============
*/
bool XYZView::CullBrush(Brush *b)
{
	if (b->IsFiltered())
		return true;
	if (b->mins[nDim1] > vMaxs[nDim1] ||
		b->mins[nDim2] > vMaxs[nDim2] ||
		b->maxs[nDim1] < vMins[nDim1] ||
		b->maxs[nDim2] < vMins[nDim2])
		return true;
	return false;
}

void XYZView::BeginDrawSelection(vec3 selColor)
{
	if (g_cfgUI.Stipple)
		glEnable(GL_LINE_STIPPLE);
	if (g_qeglobals.d_selSelectMode != sel_face)
	{
		glLineStipple(3, 0xaaaa);
		glLineWidth(2);
	}
	else
	{
		glLineStipple(2, 0xaaaa);
	}
	glColor4f(selColor[0], selColor[1], selColor[2], 1.0f);
}

void XYZView::EndDrawSelection() 
{
	glDisable(GL_LINE_STIPPLE);
	glLineWidth(1);
}

void XYZView::DrawSelection(vec3 selColor)
{
	bool	bFixedSize;
	vec3	vMinBounds, vMaxBounds;
	Brush	*brush;

	BeginDrawSelection(selColor);

	if (g_qeglobals.d_selSelectMode == sel_face)
	{
		for (auto fIt = Selection::faces.begin(); fIt != Selection::faces.end(); ++fIt)
		{
			(*fIt)->DrawWire();
		}
		EndDrawSelection();
		return;
	}

	// paint size
	ClearBounds(vMinBounds, vMaxBounds);
	bFixedSize = false;

	for (brush = g_brSelectedBrushes.next; brush != &g_brSelectedBrushes; brush = brush->next)
	{
		brush->DrawXY(dViewType);

		// sikk---> Light Radius
		if (g_cfgUI.ShowLightRadius)
			DrawLightRadius(brush, dViewType);
		// <---sikk

		// paint size
		if (g_cfgUI.ShowSizeInfo)
		{
			if (!bFixedSize)
			{
				if (brush->owner->IsPoint())
					bFixedSize = true;
				for (int i = 0; i < 3; i++)
				{
					vMinBounds[i] = min(vMinBounds[i], brush->mins[i]);
					vMaxBounds[i] = max(vMaxBounds[i], brush->maxs[i]);
				}
			}
		}
	}

	EndDrawSelection();

	// paint size
	if (!bFixedSize)
		DrawSizeInfo(vMinBounds, vMaxBounds);
}

/*
==============
XYZView::Draw
==============
*/
void XYZView::Draw ()
{
    Brush	*brush;
	Entity	*e;
	double	start, end;
	//int		i;
	vec3	mins, maxs;

	if (!g_map.brActive.next)
		return;	// not valid yet

	if (timing)
		start = Sys_DoubleTime();

	glViewport(0, 0, width, height);
	glClearColor(g_colors.gridBackground[0], 
				 g_colors.gridBackground[1],
				 g_colors.gridBackground[2],
				 0);
    glClear(GL_COLOR_BUFFER_BIT);

	// set up viewpoint
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

	SetBounds();

	glOrtho(vMins[nDim1], vMaxs[nDim1],
			vMins[nDim2], vMaxs[nDim2],
			vMins[dViewType], vMaxs[dViewType]);

	// now draw the grid
	DrawGrid();

	// draw stuff
	glShadeModel(GL_FLAT);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glColor3f(0, 0, 0);
//	glEnable (GL_LINE_SMOOTH);

	if (dViewType != XY)
	{
		glPushMatrix();
		if (dViewType == YZ)
			glRotatef(-90, 0, 1, 0);	// put Z going up
		glRotatef(-90, 1, 0, 0);	    // put Z going up
	}


	e = g_map.world;
	for (brush = g_map.brActive.next; brush != &g_map.brActive; brush = brush->next)
	{
		if (brush->IsFiltered())
			continue;
		if (CullBrush(brush))
			continue;

		if (brush->owner != e && brush->owner)
			glColor3fv(&brush->owner->eclass->color.r);
		else
			glColor3fv(&g_colors.brush.r);

		brush->DrawXY(dViewType);
	}

	DrawPathLines();

	// draw pointfile
	if (g_qeglobals.d_nPointfileDisplayList)
		glCallList(g_qeglobals.d_nPointfileDisplayList);
	
	if (!DrawTools())
		DrawSelection(g_colors.selection);
	
	if (!(dViewType == XY))
		glPopMatrix();

	// now draw camera point
	DrawCameraIcon();
	if (g_qeglobals.d_wndZ->Open())
		DrawZIcon();
	DrawCoords();	// sikk - Draw Coords last so they are on top
    glFinish();

	if (timing)
	{
		end = Sys_DoubleTime();
		Sys_Printf("XYZ: %d ms\n", (int)(1000 * (end - start)));
	}
}

//==============================
//	xy.c
//==============================

#include "qe3.h"

const char *g_pszDimStrings[] = {"x:%.f", "y:%.f", "z:%.f"};
const char *g_pszOrgStrings[] = {"(x:%.f  y:%.f)", "(x:%.f  z:%.f)", "(y:%.f  z:%.f)"};

// sikk - Free Rotate & Free Scale
bool	g_bSnapCheck, g_bRotateCheck, g_bScaleCheck;
// TODO: this is a tool, not an attribute of a view
	

XYZView::XYZView()
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
	origin[0] = 0;
	origin[1] = 0;	// sikk - changed from "20"
	origin[2] = 0;	// sikk - changed from "46"

	nRotate = 0;
	scale = 1;

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
		origin[nDim1] = b->basis.mins[nDim1];
		origin[nDim2] = b->basis.mins[nDim2];
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



/*
============================================================================

  MOUSE ACTIONS

============================================================================
*/

static int		buttonstate;
static int		pressx, pressy;
static bool		press_selection;
static vec3		pressdelta;

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
	point[nDim1] = floor((float)point[nDim1] / g_qeglobals.d_nGridSize + 0.5) * g_qeglobals.d_nGridSize;
	point[nDim2] = floor((float)point[nDim2] / g_qeglobals.d_nGridSize + 0.5) * g_qeglobals.d_nGridSize;
	return;
}

void const XYZView::ToGridPoint(const int xIn, const int yIn, int &xOut, int &yOut)
{
	ToPoint(xIn, yIn, xOut, yOut);
	xOut = floor((float)xOut / g_qeglobals.d_nGridSize + 0.5) * g_qeglobals.d_nGridSize;
	yOut = floor((float)yOut / g_qeglobals.d_nGridSize + 0.5) * g_qeglobals.d_nGridSize;
	return;
}

/*
==================
XYZView::SnapToPoint
==================
*/
void const XYZView::SnapToPoint (const int x, const int y, vec3 &point)
{
	if (g_qeglobals.d_savedinfo.bNoClamp)
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
	if (g_qeglobals.d_savedinfo.bNoClamp)
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
XYZView::DragDelta
==================
*/
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

		if (!g_qeglobals.d_savedinfo.bNoClamp)
			delta[i] = floor(delta[i] / g_qeglobals.d_nGridSize + 0.5) * g_qeglobals.d_nGridSize;		
	}
	move = delta - pressdelta;
	pressdelta = delta;

	if (move[0] || move[1] || move[2])
		return true;

	return false;
}

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

	int nAngle;

	buttonstate = buttons;

	pressx = x;
	pressy = y;

	pressdelta = vec3(0);
	ToPoint(x, y, point);
	orgLocal = point;

	orgLocal[dViewType] = 8192;
	dir[dViewType] = -1;
	right[nDim1] = 1 / scale;
	up[nDim2] = 1 / scale;

	press_selection = (Selection::HasBrushes());

	Sys_GetCursorPos(&cursorX, &cursorY);

	// LMB = manipulate selection
	// Shift+LMB = select
	if (buttonstate & MK_LBUTTON)
	{
// sikk---> Quick Move Selection (Ctrl+Alt+LMB)
		if (GetKeyState(VK_MENU) < 0 && GetKeyState(VK_CONTROL) < 0)
		{
			Brush	*b;
			vec3	v1, v2;
	
			SnapToPoint( x, y, v1);

			b = g_brSelectedBrushes.next;
			v2 = v1 - b->basis.mins;

			v2[dViewType] = 0;

			// this is so we don't drag faces when faces were previously dragged
			g_qeglobals.d_nNumMovePoints = 0;
				
			MoveSelection(v2);

			// update g_v3RotateOrigin to selection
			Selection::GetTrueMid(g_v3RotateOrigin);	// sikk - Free Rotate
		}
// <---sikk
		else
		{
			Drag_Begin(x, y, buttons, right, up, orgLocal, dir);

			// update g_v3RotateOrigin to selection
			Selection::GetTrueMid(g_v3RotateOrigin);	// sikk - Free Rotate
		}
		return;
	}

	// Ctrl+MMB = move camera
	if (buttonstate == (MK_CONTROL | MK_MBUTTON))
	{	
//			g_qeglobals.d_vCamera.origin[0] = point[0];
//			g_qeglobals.d_vCamera.origin[1] = point[1];
		CopyVector(point, g_qeglobals.d_vCamera.origin);

		Sys_UpdateWindows(W_CAMERA | W_XY | W_Z);
	}

	// MMB = angle camera
	if (buttonstate == MK_MBUTTON)
	{	
// sikk---> Free Rotate: Pivot Icon
		if (GetKeyState(VK_MENU) < 0)
		{
			SnapToPoint( x, y, point);
			CopyVector(point, g_v3RotateOrigin);
			Sys_UpdateWindows(W_XY);
			return;
		}
// <---sikk
		else
		{
			point = point - g_qeglobals.d_vCamera.origin;

			nAngle = (dViewType == XY) ? YAW : PITCH;

			if (point[nDim2] || point[nDim1])
			{
				g_qeglobals.d_vCamera.angles[nAngle] = 180 / Q_PI * atan2(point[nDim2], point[nDim1]);
				g_qeglobals.d_vCamera.BoundAngles();
				Sys_UpdateWindows(W_CAMERA | W_XY | W_Z);
			}
		}
	}

	// Shift+MMB = move z checker
	if (buttonstate == (MK_SHIFT | MK_MBUTTON))
	{
		SnapToPoint( x, y, point);
		CopyVector(point, g_qeglobals.d_vZ.origin);
		Sys_UpdateWindows(W_XY | W_Z);
		return;
	}

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
}

/*
==============
XYZView::MouseUp
==============
*/
void XYZView::MouseUp (int x, int y, int buttons)
{
	Drag_MouseUp();

	if (!press_selection)
		Sys_UpdateWindows(W_SCENE);

// sikk--->	Free Rotate & Free Scaling
	if (g_bRotateCheck || g_bScaleCheck)
	{
		nRotate = 0;

		if (g_bSnapCheck)
		{
			g_qeglobals.d_savedinfo.bNoClamp = false;
			g_bSnapCheck = false;
		}

		g_bRotateCheck = false;
		g_bScaleCheck = false;

// sikk - Undo/Redo for Free Rotate & Free Scale
		Undo::EndBrushList(&g_brSelectedBrushes);
		Undo::End();
// <---sikk

		Sys_UpdateWindows(W_XY | W_Z);
	}
// <---sikk


	buttonstate = 0;
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

	char	szRotate[16];	// sikk - Free Rotate

	if (!buttonstate || buttonstate ^ MK_RBUTTON)
	{
		SnapToPoint( x, y, tdp);
		sprintf(xystring, "xyz Coordinates: (%d %d %d)", (int)tdp[0], (int)tdp[1], (int)tdp[2]);
		Sys_Status(xystring, 0);
	}

	if (!buttonstate)
		return;

	// LMB without selection = drag new brush
	if (buttonstate == MK_LBUTTON && !press_selection)
	{
	//	DragNewBrush(x, y);

		// update g_v3RotateOrigin to new brush (for when 'quick move' is used)
		Selection::GetTrueMid(g_v3RotateOrigin);	// sikk - Free Rotate
		return;
	}

	// LMB (possibly with control and or shift)
	// with selection = drag selection
	if (buttonstate & MK_LBUTTON)
	{
		Drag_MouseMoved(x, y, buttons);
		
		// update g_v3RotateOrigin to new brush
		Selection::GetTrueMid(g_v3RotateOrigin);	// sikk - Free Rotate

		Sys_UpdateWindows(W_XY | W_CAMERA | W_Z);
		return;
	}

	// Ctrl+MMB = move camera
	if (buttonstate == (MK_CONTROL | MK_MBUTTON))
	{
		SnapToPoint( x, y, point);
		CopyVector(point, g_qeglobals.d_vCamera.origin);

		Sys_UpdateWindows(W_XY | W_CAMERA | W_Z);
		return;
	}

	// Shift+MMB = move z checker
	if (buttonstate == (MK_SHIFT | MK_MBUTTON))
	{
		SnapToPoint( x, y, point);

		CopyVector(point, g_qeglobals.d_vZ.origin);

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
			SnapToPoint( x, y, point);
			CopyVector(point, g_v3RotateOrigin);

			Sys_UpdateWindows(W_XY);
			return;
		}
// <---sikk
		else
		{
			SnapToPoint( x, y, point);
			point = point - g_qeglobals.d_vCamera.origin;

			nAngle = (dViewType == XY) ? YAW : PITCH;

			if (point[nDim1] || point[nDim2])
			{
				g_qeglobals.d_vCamera.angles[nAngle] = 180 / Q_PI * atan2(point[nDim2], point[nDim1]);
				g_qeglobals.d_vCamera.BoundAngles();
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
				
			switch (dViewType)
			{
			case XY:
				x -= cursorX;
				nRotate += x;
				Transform_RotateAxis(2, x, true);
				break;
			case XZ:
				x = cursorX - x;
				nRotate += -x;
				Transform_RotateAxis(1, x, true);
				break;
			case YZ:
				x -= cursorX;
				nRotate += x;
				Transform_RotateAxis(0, x, true);
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
			if (x != cursorX || y != cursorY)
			{
				origin[nDim1] -= (x - cursorX) / scale;
				origin[nDim2] += (y - cursorY) / scale;
				SetBounds();

				Sys_SetCursorPos(cursorX, cursorY);
				Sys_UpdateWindows(W_XY| W_Z);

				sprintf(xystring, "this Origin: (%d %d %d)", (int)origin[0], (int)origin[1], (int)origin[2]);
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

		i = cursorY - y;

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

// sikk---> Mouse Zoom
	// Ctrl+RMB = zoom xy view
	if (buttonstate == (MK_CONTROL | MK_RBUTTON))
	{
		SetCursor(NULL); // sikk - Remove Cursor
		Sys_GetCursorPos(&x, &y);
			
		if (y != cursorY)
		{
			if (y > cursorY)
				scale *= powf(1.01f, fabs(y - cursorY));
			else
				scale *= powf(0.99f, fabs(y - cursorY));

			scale = max(0.05f, min(scale, 32.0f));


			Sys_SetCursorPos(cursorX, cursorY);
			Sys_UpdateWindows(W_XY);
		}
		return;
	}
// <---sikk
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
	glColor3fv(&g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDMAJOR].r);

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
	}

	// draw minor blocks
	if (g_qeglobals.d_bShowGrid && g_qeglobals.d_nGridSize * scale >= 4)
	{
		glColor3fv(&g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDMINOR].r);

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
		// lunaran - grid axis now block color, not grid major * 65%
		glColor3fv(&g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDBLOCK].r);
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

	DrawBlockGrid();
}

/*
==============
XYZView::DrawBlockGrid
==============
*/
void XYZView::DrawBlockGrid ()
{
	if (!g_qeglobals.d_savedinfo.bShow_Blocks)
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
	glColor3fv(&g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDBLOCK].r);
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
XYZView::DrawCoords
==============
*/
void XYZView::DrawCoords()
{
	float	x, y, xb, xe, yb, ye;
	int		w, h;
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
	if (g_qeglobals.d_savedinfo.bShow_Coordinates)
	{
		glColor3fv(&g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDTEXT].r);

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

// sikk---> View Axis - cleaner, more intuitive look
	if (g_qeglobals.d_savedinfo.bShow_Viewname)
	{
		float *p1, *p2;

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

		glColor3fv(&g_qeglobals.d_savedinfo.v3Colors[COLOR_VIEWNAME].r);
		glRasterPos2f(origin[nDim1] - w + 68 / scale, 
					  origin[nDim2] + h - 51 / scale);
		glCallLists(1, GL_UNSIGNED_BYTE, &szView[0]);

		glColor3fv(&g_qeglobals.d_savedinfo.v3Colors[COLOR_VIEWNAME].r);
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
// <---sikk

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
	if (!g_qeglobals.d_savedinfo.bShow_SizeInfo)
		return;

	vec3	vSize;
	char	dimstr[128];

	vSize = vMaxBounds - vMinBounds;

	glColor3f(g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES][0] * .65, 
			  g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES][1] * .65,
			  g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES][2] * .65);

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
	float		f, fRadius;
	float		fOrigX, fOrigY, fOrigZ;
	double		fStep = (2.0f * Q_PI) / 200.0f;

	if (strncmp(pBrush->owner->eclass->name, "light", 5))
		return;

	// get the value of the "light" key
	fRadius = pBrush->owner->GetKeyValueFloat("light");
	// if entity's "light" key is not found, default to 300
	if (!fRadius)
		fRadius = 300;

	// find the center
	fOrigX = (pBrush->basis.mins[0] + ((pBrush->basis.maxs[0] - pBrush->basis.mins[0]) / 2));
	fOrigY = (pBrush->basis.mins[1] + ((pBrush->basis.maxs[1] - pBrush->basis.mins[1]) / 2));
	fOrigZ = (pBrush->basis.mins[2] + ((pBrush->basis.maxs[2] - pBrush->basis.mins[2]) / 2));

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

	glColor3fv(&g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES].r);
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
	vMins[dViewType] = -g_qeglobals.d_savedinfo.nMapSize;
	vMaxs[dViewType] = g_qeglobals.d_savedinfo.nMapSize;
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
	if (b->basis.mins[nDim1] > vMaxs[nDim1] ||
		b->basis.mins[nDim2] > vMaxs[nDim2] ||
		b->basis.maxs[nDim1] < vMins[nDim1] ||
		b->basis.maxs[nDim2] < vMins[nDim2])
		return true;
	return false;
}

void XYZView::BeginDrawSelection()
{
	glColor3fv(&g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES].r);
	if (!g_qeglobals.d_savedinfo.bNoStipple)
		glEnable(GL_LINE_STIPPLE);
	glLineStipple(3, 0xaaaa);
	glLineWidth(2);
}

void XYZView::EndDrawSelection() 
{
	glDisable(GL_LINE_STIPPLE);
	glLineWidth(1);
}

void XYZView::DrawSelection()
{
	bool	bFixedSize;
	vec3	vMinBounds, vMaxBounds;
	Brush	*brush;

	BeginDrawSelection();

	// paint size
	ClearBounds(vMinBounds, vMaxBounds);
	bFixedSize = false;

	for (brush = g_brSelectedBrushes.next; brush != &g_brSelectedBrushes; brush = brush->next)
	{
		brush->DrawXY(dViewType);

		// sikk---> Light Radius
		if (g_qeglobals.d_savedinfo.bShow_LightRadius)
			DrawLightRadius(brush, dViewType);
		// <---sikk

		// paint size
		if (g_qeglobals.d_savedinfo.bShow_SizeInfo)
		{
			if (!bFixedSize)
			{
				if (brush->owner->IsPoint())
					bFixedSize = true;
				for (int i = 0; i < 3; i++)
				{
					vMinBounds[i] = min(vMinBounds[i], brush->basis.mins[i]);
					vMaxBounds[i] = max(vMaxBounds[i], brush->basis.maxs[i]);
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
	int		i;
	vec3	mins, maxs;

	if (!g_map.brActive.next)
		return;	// not valid yet

	if (timing)
		start = Sys_DoubleTime();

	glViewport(0, 0, width, height);
	glClearColor(g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDBACK][0], 
				 g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDBACK][1],
				 g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDBACK][2],
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
		if (CullBrush(brush))
			continue;

		if (brush->owner != e && brush->owner)
			glColor3fv(&brush->owner->eclass->color.r);
		else
			glColor3fv(&g_qeglobals.d_savedinfo.v3Colors[COLOR_BRUSHES].r);

		brush->DrawXY(dViewType);
	}

	DrawPathLines();

	// draw pointfile
	if (g_qeglobals.d_nPointfileDisplayList)
		glCallList(g_qeglobals.d_nPointfileDisplayList);

	glTranslatef(g_qeglobals.d_v3SelectTranslate[0], 
				 g_qeglobals.d_v3SelectTranslate[1], 
				 g_qeglobals.d_v3SelectTranslate[2]);

	/*
	if (g_bRotateCheck)	// sikk - Free Rotate
		glColor3f(0.8f, 0.1f, 0.9f);
	else if (g_bScaleCheck)	// sikk - Free Scaling
		glColor3f(0.1f, 0.8f, 0.1f);
	else
		glColor3fv(&g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES].r);
	if (!g_qeglobals.d_savedinfo.bNoStipple)
		glEnable(GL_LINE_STIPPLE);
	glLineStipple(3, 0xaaaa);
	glLineWidth(2);
	*/

	if (!DrawTools())
		DrawSelection();

	// edge / vertex flags
	if (g_qeglobals.d_selSelectMode == sel_vertex)
	{
		glPointSize(4);
		glColor3f(0, 1, 0);
		glBegin(GL_POINTS);
		for (i = 0; i < g_qeglobals.d_nNumPoints; i++)
			glVertex3fv(&g_qeglobals.d_v3Points[i].x);
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
			v1 = &g_qeglobals.d_v3Points[g_qeglobals.d_pEdges[i].p1].x;
			v2 = &g_qeglobals.d_v3Points[g_qeglobals.d_pEdges[i].p2].x;
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


	if (!(dViewType == XY))
		glPopMatrix();

	if (GetKeyState(VK_MENU) < 0)
		DrawRotateIcon();

	// now draw camera point
	DrawCameraIcon();
	if (g_qeglobals.d_savedinfo.bShow_Z)	// sikk - Don't draw Z Icon if Z window is hidden
		DrawZIcon();
	DrawCoords();	// sikk - Draw Coords last so they are on top
    glFinish();

	if (timing)
	{
		end = Sys_DoubleTime();
		Sys_Printf("MSG: XYZ: %d ms\n", (int)(1000 * (end - start)));
	}
}

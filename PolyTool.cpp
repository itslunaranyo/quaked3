//==============================
//	PolyTool.cpp
//==============================

#include "qe3.h"
#include "CmdPolyBrushConcave.h"


PolyTool::PolyTool() : 
	axis(-1), valid(false), pcmdPBC(nullptr), 
	Tool("PolyBrush", true)	// poly tool is modal
{
	ptMoving = pointList.end();
}

PolyTool::~PolyTool()
{
	Reset();
}

/*
==================
PolyTool::Reset
==================
*/
void PolyTool::Reset()
{
	if (PointMoving())
	{
		ReleaseCapture();
		hot = false;
	}

	pointList.clear();
	ptMoving = pointList.end();
	axis = -1;
	valid = false;

	if (pcmdPBC)
	{
		delete pcmdPBC;
		pcmdPBC = nullptr;
	}

	Sys_UpdateWindows(W_SCENE);
}

/*
==================
PolyTool::Input3D
==================
*/
bool PolyTool::Input3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView & v, WndView & vWnd)
{
	return false;
}

/*
==================
PolyTool::Input2D
==================
*/
bool PolyTool::Input2D(UINT uMsg, WPARAM wParam, LPARAM lParam, XYZView & v, WndView & vWnd)
{
	int keys = wParam;
	int x, y;

	switch (uMsg)
	{
	case WM_COMMAND:
		return InputCommand(wParam);

	case WM_LBUTTONDOWN:
		if (wParam & MK_SHIFT || wParam & MK_CONTROL)
			return false;
		vWnd.GetMsgXY(lParam, x, y);
		SetCapture(vWnd.w_hwnd);
		hot = true;

		AddPoint(&v, x, y);

		Sys_UpdateWindows(W_XY | W_CAMERA);
		return true;

	case WM_LBUTTONDBLCLK:
		Commit();

		Sys_UpdateWindows(W_XY | W_CAMERA);
		return true;

	case WM_LBUTTONUP:
		if (!hot) return false;
		hot = false;
		vWnd.GetMsgXY(lParam, x, y);
		
		EndPoint();

		Sys_UpdateWindows(W_XY | W_CAMERA);
		if (!(keys & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
			ReleaseCapture();
		return true;

	case WM_MOUSEMOVE:
		if ((!keys || keys & MK_LBUTTON))
		{
			vWnd.GetMsgXY(lParam, x, y);
			MovePoint(&v, x, y);
			return !!keys;
		}
		return false;
	}
	return hot;
}

/*
==================
PolyTool::Input
==================
*/
bool PolyTool::Input(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg != WM_COMMAND)
		return false;

	return InputCommand(wParam);
}

/*
==================
PolyTool::Commit
==================
*/
bool PolyTool::Commit()
{
	if (!pcmdPBC || !valid)
		return false;

	Selection::DeselectAll();
	g_cmdQueue.Complete(pcmdPBC);
	pcmdPBC = nullptr;
	Reset();
	Sys_UpdateWindows(W_SCENE);
	return true;
}

/*
==================
PolyTool::InputCommand
==================
*/
bool PolyTool::InputCommand(WPARAM w)
{
	switch (LOWORD(w))
	{
	case ID_SELECTION_DELETE:
		DeleteLastPoint();
		Sys_UpdateWindows(W_SCENE);
		return true;
	case ID_SELECTION_CLIPSELECTED:	// lunaran FIXME: 'enter' ...
		return Commit();
	}
	return hot;
}

/*
============================================================================

	POINT PLACEMENT 2D

============================================================================
*/

/*
==============
PolyTool::XYGetNearestPoint
==============
*/
PolyTool::pointIt PolyTool::XYGetNearestPoint(XYZView* xyz, int x, int y)
{
	int		nDim1, nDim2;
	vec3	tdp;

	tdp = g_qeglobals.d_v3WorkMin;

	xyz->ToPoint(x, y, tdp);
	nDim1 = (xyz->GetAxis() == YZ) ? 1 : 0;
	nDim2 = (xyz->GetAxis() == XY) ? 1 : 2;

	// based on screen distance and not world distance
	float margin = 10.0f / xyz->scale;

	for (pointIt pIt = pointList.begin(); pIt != pointList.end(); ++pIt)
	{
		if (fabs((*pIt)[nDim1] - tdp[nDim1]) < margin &&
			fabs((*pIt)[nDim2] - tdp[nDim2]) < margin)
		{
			return pIt;
		}
	}
	return pointList.end();
}

/*
==============
PolyTool::AddPoint
==============
*/
void PolyTool::AddPoint(XYZView* xyz, int x, int y)
{
	if (PointMoving())  // new click issued on an existing point
	{
		if (xyz == XYZWnd_WinFromHandle(GetCapture()))
			xyz->SnapToPoint(x, y, *ptMoving);
	}
	else // new point added
	{
		if (pointList.empty())
			axis = XYZWnd_WinFromHandle(GetCapture())->GetAxis();

		vec3 newpt = g_qeglobals.d_v3WorkMin;
		if (xyz == XYZWnd_WinFromHandle(GetCapture()))
			xyz->SnapToPoint(x, y, newpt);

		// be helpful and don't place coincident points if they're illegal anyway
		int vaxis = xyz->GetAxis();
		for (auto ptIt = pointList.begin(); ptIt != pointList.end(); ++ptIt)
		{
			if ((*ptIt)[(vaxis + 1) % 3] == newpt[(vaxis + 1) % 3] &&
				(*ptIt)[(vaxis + 2) % 3] == newpt[(vaxis + 2) % 3])
				return;
		}

		pointList.push_back(newpt);
		ptMoving = pointList.end() - 1;
	}

	PointsUpdated();
	Sys_UpdateWindows(W_XY | W_CAMERA);
}

/*
==============
PolyTool::MovePoint
==============
*/
void PolyTool::MovePoint(XYZView* xyz, int x, int y)
{
	bool	bCrossHair = false;

	// lunaran TODO: don't use windows mouse capture status for control flow maybe
	if (PointMoving() && GetCapture())
	{
		if (xyz == XYZWnd_WinFromHandle(GetCapture()))
			xyz->SnapToPoint(x, y, *ptMoving);
		bCrossHair = true;

		PointsUpdated();
		Sys_UpdateWindows(W_XY | W_CAMERA);
	}
	else
	{
		ptMoving = XYGetNearestPoint(xyz, x, y);
		if (PointMoving())
			bCrossHair = true;
	}

	Crosshair(bCrossHair);
}

/*
==============
PolyTool::EndPoint
==============
*/
void PolyTool::EndPoint()
{
	if (PointMoving() && GetCapture())
	{
		ptMoving = pointList.end();
		ReleaseCapture();
	}
	PointsUpdated();
	Sys_UpdateWindows(W_XY | W_CAMERA);
}

/*
==============
PolyTool::DeleteLastPoint
==============
*/
void PolyTool::DeleteLastPoint()
{
	if (!pointList.empty())
		pointList.pop_back();
	if (pointList.empty())
		axis = -1;
	ptMoving = pointList.end();
	PointsUpdated();
}

/*
==============
PolyTool::PointsUpdated
==============
*/
bool PolyTool::PointsUpdated()
{
	if (!pcmdPBC)
		pcmdPBC = new CmdPolyBrushConcave();

	if (pointList.size())
	{
		try {
			pcmdPBC->SetAxis(axis);
			pcmdPBC->SetBounds(g_qeglobals.d_v3WorkMin[axis], g_qeglobals.d_v3WorkMax[axis]);
			pcmdPBC->SetPoints(pointList);
			valid = true;
		}
		catch (qe3_cmd_exception)// &ex)
		{
			// don't report the error - the UI widget changes color when current cmdPBC
			// input is bad, so we don't need to spam it to the console too
			valid = false;
		}
		return valid;
	}
	return false;
}



/*
============================================================================

	DRAWING

============================================================================
*/

void PolyTool::DrawSetColor()
{
	if (valid)
		glColor3fv(&g_colors.tool.r);
	else
		glColor3f(1,0,0);
}

void PolyTool::DrawLoop(int plane)
{
	int nDim1 = (axis == YZ) ? 1 : 0;
	int nDim2 = (axis == XY) ? 1 : 2;
	vec3 pt(plane);
	pointIt pIt;

	glDisable(GL_DEPTH_TEST);
	DrawSetColor();

	glBegin(GL_LINE_STRIP);
	for (pIt = pointList.begin(); pIt != pointList.end(); ++pIt)
	{
		pt[nDim1] = (*pIt)[nDim1];
		pt[nDim2] = (*pIt)[nDim2];
		glVertex3fv(&pt.x);
	}
	glEnd();

	// show the implied final edge that closes the loop with a dotted line
	glEnable(GL_LINE_STIPPLE);
	glLineStipple(3, 0xaaaa);

	glBegin(GL_LINE_STRIP);
	glVertex3fv(&pt.x);
	pIt = pointList.begin();
	pt[nDim1] = (*pIt)[nDim1];
	pt[nDim2] = (*pIt)[nDim2];
	glVertex3fv(&pt.x);
	glEnd();

	glDisable(GL_LINE_STIPPLE);

	//glEnable(GL_DEPTH_TEST);
}

void PolyTool::DrawPegs()
{
	int nDim1 = (axis == YZ) ? 1 : 0;
	int nDim2 = (axis == XY) ? 1 : 2;
	vec3 pt1(g_qeglobals.d_v3WorkMin);
	vec3 pt2(g_qeglobals.d_v3WorkMax);

	glDisable(GL_DEPTH_TEST);
	DrawSetColor();

	glEnable(GL_LINE_STIPPLE);
	glLineStipple(3, 0xaaaa);

	glBegin(GL_LINES);
	for (pointIt pIt = pointList.begin(); pIt != pointList.end(); ++pIt)
	{
		pt1[nDim1] = (*pIt)[nDim1];
		pt2[nDim1] = (*pIt)[nDim1];
		pt1[nDim2] = (*pIt)[nDim2];
		pt2[nDim2] = (*pIt)[nDim2];
		glVertex3fv(&pt1.x);
		glVertex3fv(&pt2.x);
	}
	glEnd();

	glDisable(GL_LINE_STIPPLE);
	//glEnable(GL_DEPTH_TEST);
}

void PolyTool::DrawPoints()
{
	glDisable(GL_DEPTH_TEST);

	glPointSize(7);
	glColor3f(0, 0, 0);
	glBegin(GL_POINTS);
	for (pointIt pIt = pointList.begin(); pIt != pointList.end(); ++pIt)
		glVertex3fv(&pIt->x);
	glEnd();

	glPointSize(5);
	DrawSetColor();
	glBegin(GL_POINTS);
	for (pointIt pIt = pointList.begin(); pIt != pointList.end(); ++pIt)
		glVertex3fv(&pIt->x);
	glEnd();

	//glEnable(GL_DEPTH_TEST);
}

/*
==================
PolyTool::Draw3D
==================
*/
bool PolyTool::Draw3D(CameraView & v)
{
	if (pointList.empty())
		return false;

	DrawLoop(g_qeglobals.d_v3WorkMin[axis]);
	DrawLoop(g_qeglobals.d_v3WorkMax[axis]);
	DrawPegs();
	DrawPoints();
	return false;
}

/*
==================
PolyTool::Draw2D
==================
*/
bool PolyTool::Draw2D(XYZView & v)
{
	if (pointList.empty())
		return false;

	DrawLoop(0);
	DrawPoints();
	return false;
}



//==============================
//	ClipTool.cpp
//==============================

#include "qe3.h"

#define CLIP_CAMDOT 0.9996f

ClipTool::ClipTool() : 
	ptMoving(nullptr),
	g_pcmdBC(nullptr),
	Tool("Clipper", true)	// clip tool is modal
{
	g_qeglobals.d_clipTool = this;
	Selection::DeselectAllFaces();
	Reset();
}

ClipTool::~ClipTool()
{
	g_qeglobals.d_clipTool = nullptr;
	Reset();
	Sys_UpdateGridStatusBar();
}

/*
==================
ClipTool::Input3D
==================
*/
bool ClipTool::Input3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndView &vWnd)
{
	int keys = wParam;
	int x, y;

	switch (uMsg)
	{
	case WM_COMMAND:
		return InputCommand(wParam);

//	case WM_KEYDOWN:
//		return false;

	case WM_LBUTTONDOWN:
		if (wParam & MK_SHIFT)
			return !!(wParam & MK_CONTROL);	// prevent face selection
			//return false;
		vWnd.GetMsgXY(lParam, x, y);
		SetCapture(vWnd.w_hwnd);
		hot = true;

		// lunaran - alt quick clip
		if (AltDown())
			CamStartQuickClip(x, y);
		else
			CamDropPoint(x, y);
		Sys_UpdateWindows(W_XY | W_CAMERA);
		return true;

	case WM_LBUTTONUP:
		if (!hot) return false;
		hot = false;
		vWnd.GetMsgXY(lParam, x, y);
		// lunaran - alt quick clip
		if (AltDown())
			CamEndQuickClip();
		else
			CamEndPoint();
		Sys_UpdateWindows(W_XY | W_CAMERA);
		if (!(keys & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
			ReleaseCapture();
		return true;

	case WM_MOUSEMOVE:
		if ((!keys || keys & MK_LBUTTON))
		{
			vWnd.GetMsgXY(lParam, x, y);
			CamMovePoint(x, y);
			return !!keys;
		}
		return false;
	}
	return hot;
}

/*
==================
ClipTool::Input2D
==================
*/
bool ClipTool::Input2D(UINT uMsg, WPARAM wParam, LPARAM lParam, XYZView &v, WndView &vWnd)
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

		// lunaran - alt quick clip
		if (wParam & MK_ALT)
			StartQuickClip(&v, x, y);
		else
			DropPoint(&v, x, y);
		Sys_UpdateWindows(W_XY | W_CAMERA);
		return true;

	case WM_LBUTTONUP:
		if (!hot) return false;
		hot = false;
		vWnd.GetMsgXY(lParam, x, y);
		// lunaran - alt quick clip
		if (wParam & MK_ALT)
			EndQuickClip();
		else
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
ClipTool::Input
==================
*/
bool ClipTool::Input(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg != WM_COMMAND)
		return false;

	return InputCommand(wParam);
}

/*
==================
ClipTool::InputCommand
==================
*/
bool ClipTool::InputCommand(WPARAM w)
{
	switch (LOWORD(w))
	{
	case ID_SELECTION_CLIPSELECTED:
		Clip();
		return true;
	case ID_SELECTION_SPLITSELECTED:
		Split();
		return true;
	case ID_SELECTION_FLIPCLIP:
		Flip();
		return true;
	}
	return hot;
}

/*
==================
ClipTool::SelectionChanged

trash the command and remake it with the same points/sidedness
==================
*/
void ClipTool::SelectionChanged()
{
	if (g_pcmdBC)
	{
		delete g_pcmdBC;
		g_pcmdBC = nullptr;
	}
	StartCommand();
	PointsUpdated();
}

/*
==================
ClipTool::Crosshair
==================
*/
void ClipTool::Crosshair(bool bCrossHair)
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
ClipTool::StartCommand
==================
*/
void ClipTool::StartCommand()
{
	if (g_pcmdBC) return;

	g_pcmdBC = new CmdBrushClip();
	g_pcmdBC->SetSide(backside ? CmdBrushClip::clipside::CLIP_BACK : CmdBrushClip::clipside::CLIP_FRONT);
	g_pcmdBC->StartBrushes(&g_brSelectedBrushes);
}

/*
==================
ClipTool::Reset
==================
*/
void ClipTool::Reset ()
{
	for (int i = 0; i < 3; i++)
		points[i].Reset();

	if (ptMoving)
	{
		ReleaseCapture();
		hot = false;
		ptMoving = nullptr;
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
ClipTool::PointsUpdated
==================
*/
void ClipTool::PointsUpdated()
{
	assert(g_pcmdBC);
	if (!points[0].set)
	{
		g_pcmdBC->UnsetPoints();
		return;
	}

	if (points[2].set)
	{
		// use all three points
		g_pcmdBC->SetPoints(points[0].point, points[1].point, points[2].point);
	}
	else if (points[1].set)
	{
		// use implied third point
		vec3 p3 = points[1].point;
		p3[axis] += 128;
		g_pcmdBC->SetPoints(points[0].point, points[1].point, p3);
	}
}


/*
==================
ClipTool::Clip
==================
*/
void ClipTool::Clip ()
{
	if (!g_pcmdBC)
		return;

	g_cmdQueue.Complete(g_pcmdBC);
	g_pcmdBC = nullptr;
	Reset();

	Sys_UpdateWindows(W_SCENE);
}

/*
==================
ClipTool::Split
==================
*/
void ClipTool::Split ()
{
	if (!g_pcmdBC)
		return;

	g_pcmdBC->SetSide(CmdBrushClip::clipside::CLIP_BOTH);
	g_cmdQueue.Complete(g_pcmdBC);
	g_pcmdBC = nullptr;
	Reset();
	Sys_UpdateWindows(W_SCENE);
}

/*
==================
ClipTool::Flip
==================
*/
void ClipTool::Flip ()
{
	if (!g_pcmdBC)
		return;

	backside = !backside;
	g_pcmdBC->SetSide(backside ? CmdBrushClip::clipside::CLIP_BACK : CmdBrushClip::clipside::CLIP_FRONT);

	Sys_UpdateWindows(W_SCENE);
}

/*
==================
ClipTool::StartNextPoint
==================
*/
ClipTool::clippoint_t* ClipTool::StartNextPoint()
{
	clippoint_t *pt;
	pt = nullptr;

	if (!points[0].set)
	{
		assert(!points[1].set);
		assert(!points[2].set);
		StartCommand();
		pt = &points[0];
		points[0].set = true;
	}
	else if (!points[1].set)
	{
		assert(points[0].set);
		assert(!points[2].set);
		pt = &points[1];
		points[1].set = true;
	}
	else if (!points[2].set)
	{
		assert(points[0].set);
		assert(points[1].set);
		pt = &points[2];
		points[2].set = true;
	}
	else
	{
		Reset();
		StartCommand();
		pt = &points[0];
		points[0].set = true;
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
ClipTool::CamStartQuickClip
==================
*/
void ClipTool::CamStartQuickClip(int x, int y)
{
	Reset();

	CamDropPoint(x, y);
	CamDropPoint(x, y);
	ptMoving = &points[1];
}

/*
==================
ClipTool::CamEndQuickClip
==================
*/
void ClipTool::CamEndQuickClip()
{
	CamEndPoint();
	Clip();
}

/*
==================
ClipTool::CamPointOnSelection
==================
*/
bool ClipTool::CamPointOnSelection(int x, int y, vec3 &out, int* outAxis)
{
	vec3		dir, pn, pt;
	trace_t		t;
	g_qeglobals.d_vCamera.PointToRay(x, y, dir);
	t = Selection::TestRay(g_qeglobals.d_vCamera.origin, dir, SF_NOFIXEDSIZE | SF_SELECTED);
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

		pt = g_qeglobals.d_vCamera.origin + t.dist * dir;
		SnapToPoint(pt);
		ProjectOnPlane(t.face->plane.normal, t.face->plane.dist, pn, pt);
		out = pt;
		return true;
	}
	return false;
}

/*
==================
ClipTool::CamDropPoint
==================
*/
void ClipTool::CamDropPoint(int x, int y)
{
	int		nAxis = -1;
	vec3	pt;

	if (!CamPointOnSelection(x, y, pt, &nAxis))
		return;

	if (ptMoving && GetCapture() == g_qeglobals.d_hwndCamera)
	{
		ptMoving->point = pt;
	}
	else
	{
		if (points[0].set == false)
			axis = nAxis;

		clippoint_t		*pPt;
		pPt = StartNextPoint();
		pPt->point = pt;
	}
	PointsUpdated();
	Sys_UpdateWindows(W_XY | W_CAMERA);
}

/*
==================
ClipTool::CamGetNearestClipPoint
==================
*/
ClipTool::clippoint_t *ClipTool::CamGetNearestClipPoint(int x, int y)
{
	vec3 dir, cPt;
	g_qeglobals.d_vCamera.PointToRay(x, y, dir);

	for (int i = 0; i < 3; i++)
	{
		if (points[i].set)
		{
			cPt = points[i].point - g_qeglobals.d_vCamera.origin;
			VectorNormalize(cPt);

			if (DotProduct(cPt, dir) > CLIP_CAMDOT)
				return &points[i];
		}
	}

	return nullptr;
}

/*
==================
ClipTool::CamMovePoint
==================
*/
void ClipTool::CamMovePoint(int x, int y)
{
	int junk;
	bool	bCrossHair = false;

	// lunaran TODO: don't use windows mouse capture status for control flow maybe
	if (ptMoving && GetCapture() == g_qeglobals.d_hwndCamera)
	{
		// lunaran - grid view reunification
		if (CamPointOnSelection(x, y, ptMoving->point, &junk))
		{
			PointsUpdated();
			Sys_UpdateWindows(W_XY | W_CAMERA);
			bCrossHair = true;
		}
	}
	else
	{
		ptMoving = CamGetNearestClipPoint(x, y);
		if (ptMoving)
			bCrossHair = true;
	}

	Crosshair(bCrossHair);
}

/*
==================
ClipTool::CamEndPoint
==================
*/
void ClipTool::CamEndPoint()
{
	if (ptMoving && GetCapture() == g_qeglobals.d_hwndCamera)
	{
		ptMoving = NULL;
		ReleaseCapture();
		PointsUpdated();
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
ClipTool::StartQuickClip

lunaran: puts point 1 and 2 in the same place and immediately begins dragging point 2
==============
*/
void ClipTool::StartQuickClip(XYZView* xyz, int x, int y)
{
	Reset();

	DropPoint(xyz, x, y);
	DropPoint(xyz, x, y);
	ptMoving = &points[1];

}

/*
==============
ClipTool::EndQuickClip

lunaran: stop dragging the current point and immediately clip with it
==============
*/
void ClipTool::EndQuickClip()
{
	EndPoint();
	Clip();
}

/*
==============
ClipTool::DropPoint
==============
*/
void ClipTool::DropPoint (XYZView* xyz, int x, int y)
{
	int nDim;

	axis = xyz->GetAxis();

	if (ptMoving)  // <-- when a new click is issued on an existing point
	{
		// lunaran - grid view reunification
		if (xyz == XYZWnd_WinFromHandle(GetCapture()))
			xyz->SnapToPoint(x, y, ptMoving->point);
	}
	else // <-- if a new point is dropped in space
	{
		clippoint_t		*pPt;
		pPt = StartNextPoint();

		// lunaran - grid view reunification
		if (xyz == XYZWnd_WinFromHandle(GetCapture()))
			xyz->SnapToPoint(x, y, pPt->point);

		// third coordinates for clip point: use d_v3WorkMax
		nDim = (axis == YZ ) ? 0 : ((axis == XZ) ? 1 : 2);

		// lunaran: put third clip at work min instead of work max so they aren't all coplanar to the view
		if (points[2].set)
			(pPt->point)[nDim] = g_qeglobals.d_v3WorkMin[nDim];
		else
			(pPt->point)[nDim] = g_qeglobals.d_v3WorkMax[nDim];
	}
	PointsUpdated();
	Sys_UpdateWindows(W_XY| W_CAMERA);
}

/*
==============
ClipTool::GetCoordXY
==============
*/
void ClipTool::GetCoordXY(int x, int y, vec3 &pt)
{
	int nDim;
	// lunaran - grid view reunification
	XYZView* xyz = XYZWnd_WinFromHandle(GetCapture());
	if (xyz)
		xyz->SnapToPoint(x, y, pt);
	nDim = (xyz->GetAxis() == YZ) ? 0 : ((xyz->GetAxis() == XZ) ? 1 : 2);

	pt[nDim] = g_qeglobals.d_v3WorkMax[nDim];
}

/*
==============
ClipTool::XYGetNearestClipPoint
==============
*/
ClipTool::clippoint_t* ClipTool::XYGetNearestClipPoint(XYZView* xyz, int x, int y)
{
	int nDim1, nDim2;
	vec3	tdp;

	tdp[0] = tdp[1] = tdp[2] = 256;

	xyz->SnapToPoint(x, y, tdp);
	nDim1 = (xyz->GetAxis() == YZ) ? 1 : 0;
	nDim2 = (xyz->GetAxis() == XY) ? 1 : 2;

	// lunaran: based on screen distance and not world distance
	float margin = 10.0f / xyz->scale;
	if (points[0].set)
	{
		if (fabs(points[0].point[nDim1] - tdp[nDim1]) < margin &&
			fabs(points[0].point[nDim2] - tdp[nDim2]) < margin)
		{
			return &points[0];
		}
	}
	if (points[1].set)
	{
		if (fabs(points[1].point[nDim1] - tdp[nDim1]) < margin &&
			fabs(points[1].point[nDim2] - tdp[nDim2]) < margin)
		{
			return &points[1];
		}
	}
	if (points[2].set)
	{
		if (fabs(points[2].point[nDim1] - tdp[nDim1]) < margin &&
			fabs(points[2].point[nDim2] - tdp[nDim2]) < margin)
		{
			return &points[2];
		}
	}
	return NULL;
}

/*
==============
ClipTool::MovePoint
should be named ClipTool::MoveMouse
==============
*/
void ClipTool::MovePoint (XYZView* xyz, int x, int y)
{
	bool	bCrossHair = false;

	// lunaran TODO: don't use windows mouse capture status for control flow maybe
	if (ptMoving && GetCapture())
	{
		// lunaran - grid view reunification
		if (xyz == XYZWnd_WinFromHandle(GetCapture()))
			xyz->SnapToPoint(x, y, ptMoving->point);
		bCrossHair = true;
		PointsUpdated();
		Sys_UpdateWindows(W_XY | W_CAMERA);
	}
	else
	{
		ptMoving = XYGetNearestClipPoint(xyz, x, y);
		if (ptMoving)
			bCrossHair = true;
	}

	Crosshair(bCrossHair);

}

/*
==============
ClipTool::EndPoint
==============
*/
void ClipTool::EndPoint ()
{
	if (ptMoving && GetCapture())
	{
		axis = QE_BestViewAxis();

		ptMoving = nullptr;
		ReleaseCapture();
	}
	Sys_UpdateWindows(W_XY | W_CAMERA);
}







bool ClipTool::Draw3D(CameraView &v)
{
	if (!g_pcmdBC)
		return false;
	Draw();
	DrawPoints();
	glEnable(GL_DEPTH_TEST);
	return true;
}

bool ClipTool::Draw2D(XYZView &v)
{
	if (!g_pcmdBC)
		return false;
	Draw();
	DrawPoints();
	return true;
}


void ClipTool::DrawPoints()
{
	char		strMsg[4];

	glDisable(GL_DEPTH_TEST);
	glColor3fv(&g_qeglobals.d_savedinfo.v3Colors[COLOR_CLIPPER].r);

	glPointSize(4);
	glBegin(GL_POINTS);
	for (int i = 0; i < 3; i++)
	{
		if (!points[i].set)
			break;
		glVertex3fv(&points[i].point.x);
	}
	glEnd();
	glPointSize(1);

	for (int i = 0; i < 3; i++)
	{
		if (!points[i].set)
			break;
		glRasterPos3f(points[i].point[0] + 1, points[i].point[1] + 1, points[i].point[2] + 1);
		sprintf(strMsg, "%i", i);
		glCallLists(strlen(strMsg), GL_UNSIGNED_BYTE, strMsg);
	}
}


/*
==============
ClipTool::Draw
==============
*/
void ClipTool::Draw ()
{
	Brush		*brush;
	Face		*face;

	if (points[1].set)
	{
		std::vector<Brush*> *brList = (backside) ? &g_pcmdBC->brBack : &g_pcmdBC->brFront;
		for (auto brIt = brList->begin(); brIt != brList->end(); ++brIt)
			(*brIt)->Draw();

		glDisable(GL_TEXTURE_2D);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glColor4f(g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES][0],
					g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES][1],
					g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES][2],
					0.3f);
		for (auto brIt = brList->begin(); brIt != brList->end(); ++brIt)
			for (face = (*brIt)->basis.faces; face; face = face->fnext)
				face->Draw();
		glDisable(GL_BLEND);

		glDisable(GL_DEPTH_TEST);
		glColor3f(1, 1, 0);
		for (auto brIt = brList->begin(); brIt != brList->end(); ++brIt)
			for (face = (*brIt)->basis.faces; face; face = face->fnext)
				face->DrawWire();

	}
	else
	{
		for (brush = g_brSelectedBrushes.next; brush != &g_brSelectedBrushes; brush = brush->next)
			brush->Draw();

		glDisable(GL_TEXTURE_2D);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glColor4f(g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES][0],
				g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES][1],
				g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES][2],
				0.3f);
		for (brush = g_brSelectedBrushes.next; brush != &g_brSelectedBrushes; brush = brush->next)
			for (face = brush->basis.faces; face; face = face->fnext)
				face->Draw();
		glDisable(GL_BLEND);

		glDisable(GL_DEPTH_TEST);
		glColor3f(1, 1, 0);
		for (brush = g_brSelectedBrushes.next; brush != &g_brSelectedBrushes; brush = brush->next)
			for (face = brush->basis.faces; face; face = face->fnext)
				face->DrawWire();
	}

	glEnable(GL_TEXTURE_2D);
}






/*
==================
SnapToPoint

TODO: find me a real home
==================
*/
void SnapToPoint(vec3 &point)
{
	for (int i = 0; i < 3; i++)
		point[i] = floor(point[i] / g_qeglobals.d_nGridSize + 0.5f) * g_qeglobals.d_nGridSize;
}



//==============================
//	ClipTool.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "ClipTool.h"

#include "CameraView.h"
#include "CameraRenderer.h"
#include "WndCamera.h"
#include "GridView.h"
#include "GridViewRenderer.h"
#include "WndGrid.h"

#include "CmdBrushClip.h"
#include "select.h"

#define CLIP_CAMDOT 0.9997f

ClipTool::ClipTool() : 
	ptMoving(nullptr),
	g_pcmdBC(nullptr),
	Tool("Clipper", true)	// clip tool is modal
{
	Selection::DeselectAllFaces();
	Reset();
}

ClipTool::~ClipTool()
{
	Reset();
	WndMain_UpdateGridStatusBar();
}

/*
==================
ClipTool::Input3D
==================
*/
bool ClipTool::Input3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndCamera &vWnd)
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
		SetCapture(vWnd.wHwnd);
		hot = true;

		// lunaran - alt quick clip
		if (AltDown())
			CamStartQuickClip(x, y);
		else
			CamDropPoint(x, y);
		WndMain_UpdateWindows(W_XY | W_CAMERA);
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
		WndMain_UpdateWindows(W_XY | W_CAMERA);
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
	case WM_MOUSELEAVE:
		ptHover.set = false;
		WndMain_UpdateWindows(W_XY | W_CAMERA);
		return false;
	}
	return hot;
}

/*
==================
ClipTool::Input2D
==================
*/
bool ClipTool::Input2D(UINT uMsg, WPARAM wParam, LPARAM lParam, GridView &v, WndGrid &vWnd)
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
		SetCapture(vWnd.wHwnd);
		hot = true;

		// lunaran - alt quick clip
		if (AltDown())
			StartQuickClip(&v, x, y);
		else
			DropPoint(&v, x, y);
		WndMain_UpdateWindows(W_XY | W_CAMERA);
		return true;

	case WM_LBUTTONUP:
		if (!hot) return false;
		hot = false;
		vWnd.GetMsgXY(lParam, x, y);
		// lunaran - alt quick clip
		if (AltDown())
			EndQuickClip();
		else
			EndPoint();
		WndMain_UpdateWindows(W_XY | W_CAMERA);
		if (!(keys & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
			ReleaseCapture();
		return true;

	case WM_MOUSEMOVE:
		ptHover.set = false;
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
	ptHover.Reset();

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

	WndMain_UpdateWindows(W_SCENE);
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
	if (!g_pcmdBC || !points[1].set)
		return;

	g_cmdQueue.Complete(g_pcmdBC);
	g_pcmdBC = nullptr;
	Reset();

	WndMain_UpdateWindows(W_SCENE);
}

/*
==================
ClipTool::Split
==================
*/
void ClipTool::Split ()
{
	if (!g_pcmdBC || !points[1].set)
		return;

	g_pcmdBC->SetSide(CmdBrushClip::clipside::CLIP_BOTH);
	g_cmdQueue.Complete(g_pcmdBC);
	g_pcmdBC = nullptr;
	Reset();
	WndMain_UpdateWindows(W_SCENE);
}

/*
==================
ClipTool::Flip
==================
*/
void ClipTool::Flip ()
{
	if (!g_pcmdBC || !points[1].set)
		return;

	backside = !backside;
	g_pcmdBC->SetSide(backside ? CmdBrushClip::clipside::CLIP_BACK : CmdBrushClip::clipside::CLIP_FRONT);

	WndMain_UpdateWindows(W_SCENE);
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
bool ClipTool::CamPointOnSelection(int x, int y, vec3 &out, int* outAxis)	// lunaran TODO: int*?
{
	vec3		dir, pn, pt;
	trace_t		t;
	g_vCamera.PointToRay(x, y, dir);
	t = Selection::TestRay(g_vCamera.GetOrigin(), dir, SF_NOFIXEDSIZE | SF_SELECTED_ONLY);
	if (t.brush && t.face)
	{
		pn = AxisForVector(t.face->plane.normal);

		if (pn[0])
			*outAxis = GRID_YZ;
		else if (pn[1])
			*outAxis = GRID_XZ;
		else
			*outAxis = GRID_XY;

		pt = g_vCamera.GetOrigin() + t.dist * dir;
		//ProjectOnPlane(t.face->plane.normal, t.face->plane.dist, pn, pointOnGrid(pt));
		pt = t.face->plane.ProjectPointAxial(pointOnGrid(pt), pn);

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

	if (ptMoving && GetCapture() == g_hwndCamera)
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
	WndMain_UpdateWindows(W_XY | W_CAMERA);
}

/*
==================
ClipTool::CamGetNearestClipPoint
==================
*/
ClipTool::clippoint_t *ClipTool::CamGetNearestClipPoint(int x, int y)
{
	vec3 dir, cPt;
	g_vCamera.PointToRay(x, y, dir);

	for (int i = 0; i < 3; i++)
	{
		if (points[i].set)
		{
			cPt = points[i].point - g_vCamera.GetOrigin();
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
	if (ptMoving && GetCapture() == g_hwndCamera)
	{
		// lunaran - grid view reunification
		if (CamPointOnSelection(x, y, ptMoving->point, &junk))
		{
			PointsUpdated();
			WndMain_UpdateWindows(W_XY | W_CAMERA);
			bCrossHair = true;
		}
	}
	else
	{
		ptMoving = CamGetNearestClipPoint(x, y);
		if (ptMoving)
		{
			bCrossHair = true;
			ptHover.set = false;
		}
		else if (CamPointOnSelection(x, y, ptHover.point, &junk))
		{
			ptHover.set = true;
			WndMain_UpdateWindows(W_XY | W_CAMERA);
			bCrossHair = true;
		}
		else
		{
			if (ptHover.set)
			{
				ptHover.set = false;
				WndMain_UpdateWindows(W_XY | W_CAMERA);
			}
		}
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
	if (ptMoving && GetCapture() == g_hwndCamera)
	{
		ptMoving = NULL;
		ReleaseCapture();
		PointsUpdated();
	}
	WndMain_UpdateWindows(W_XY | W_CAMERA);
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
void ClipTool::StartQuickClip(GridView* xyz, int x, int y)
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
void ClipTool::DropPoint (GridView* xyz, int x, int y)
{
	axis = xyz->GetAxis();

	if (ptMoving)  // <-- when a new click is issued on an existing point
	{
		// lunaran - grid view reunification
		if (xyz == GridView::FromHwnd(GetCapture()))
			xyz->ScreenToWorldGrid(x, y, ptMoving->point);
	}
	else // <-- if a new point is dropped in space
	{
		clippoint_t		*pPt;
		pPt = StartNextPoint();

		// lunaran - grid view reunification
		if (xyz == GridView::FromHwnd(GetCapture()))
			xyz->ScreenToWorldGrid(x, y, pPt->point);

		// third coordinates for clip point: use d_v3WorkMax
		// lunaran: put third clip at work min instead of work max so they aren't all coplanar to the view
		if (points[2].set)
			(pPt->point)[axis] = g_qeglobals.d_v3WorkMin[axis];
		else
			(pPt->point)[axis] = g_qeglobals.d_v3WorkMax[axis];
	}
	PointsUpdated();
	WndMain_UpdateWindows(W_XY| W_CAMERA);
}

/*
==============
ClipTool::GetCoordXY
==============
*/
void ClipTool::GetCoordXY(int x, int y, vec3 &pt)
{
	// lunaran - grid view reunification
	GridView* xyz = GridView::FromHwnd(GetCapture());
	if (xyz)
		xyz->ScreenToWorldGrid(x, y, pt);

	pt[xyz->GetAxis()] = g_qeglobals.d_v3WorkMax[xyz->GetAxis()];
}

/*
==============
ClipTool::XYGetNearestClipPoint
==============
*/
ClipTool::clippoint_t* ClipTool::XYGetNearestClipPoint(GridView* gv, int x, int y)
{
	int nDim1, nDim2;
	vec3	tdp;

	tdp[0] = tdp[1] = tdp[2] = 256;

	gv->ScreenToWorld(x, y, tdp);
	nDim1 = gv->DimU();
	nDim2 = gv->DimV();

	// lunaran: based on screen distance and not world distance
	float margin = 10.0f / gv->GetScale();
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
void ClipTool::MovePoint (GridView* xyz, int x, int y)
{
	bool	bCrossHair = false;

	// lunaran TODO: don't use windows mouse capture status for control flow maybe
	if (ptMoving && GetCapture())
	{
		// lunaran - grid view reunification
		if (xyz == GridView::FromHwnd(GetCapture()))
			xyz->ScreenToWorldGrid(x, y, ptMoving->point);
		bCrossHair = true;
		PointsUpdated();
		WndMain_UpdateWindows(W_XY | W_CAMERA);
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
	WndMain_UpdateWindows(W_XY | W_CAMERA);
}







bool ClipTool::Draw3D(CameraRenderer &rc)
{
	//if (!g_pcmdBC)
	//	return false;
	if (!points[0].set && !ptHover.set)
		return false;

	if (points[1].set)
		Draw();
	else
		rc.DrawSelected(&g_brSelectedBrushes, g_colors.selection);

	DrawPoints();
	glEnable(GL_DEPTH_TEST);
	return true;
}

bool ClipTool::Draw2D(GridViewRenderer &gr)
{
	if (!points[0].set && !ptHover.set)
		return false;
	gr.DrawSelection(g_colors.selection);
	
	if (g_pcmdBC)
		DrawClipWire((backside) ? &g_pcmdBC->brBack : &g_pcmdBC->brFront);

	DrawPoints();
	return true;
}


void ClipTool::DrawPoints()
{
	char		strMsg[4];

	glDisable(GL_DEPTH_TEST);
	glColor3fv(&g_colors.tool.r);

	glPointSize(4);
	glBegin(GL_POINTS);
	for (int i = 0; i < 3; i++)
	{
		if (!points[i].set)
			break;
		glVertex3fv(&points[i].point.x);
	}
	if (ptHover.set)
		glVertex3fv(&ptHover.point.x);

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
	//glEnable(GL_DEPTH_TEST);
}

void ClipTool::DrawClipWire(std::vector<Brush*> *brList)
{
	Face* face;

	// draw yellow wireframe of carved brushes
	glDisable(GL_DEPTH_TEST);
	glColor3f(1, 1, 0);
	for (auto brIt = brList->begin(); brIt != brList->end(); ++brIt)
		for (face = (*brIt)->faces; face; face = face->fnext)
			face->DrawWire();
	//glEnable(GL_DEPTH_TEST);
}

/*
==============
ClipTool::Draw
==============
*/
void ClipTool::Draw()
{
	Face *face;

	// draw carved brushes textured
	std::vector<Brush*> *brList = (backside) ? &g_pcmdBC->brBack : &g_pcmdBC->brFront;
	for (auto brIt = brList->begin(); brIt != brList->end(); ++brIt)
		(*brIt)->Draw();

	glDisable(GL_TEXTURE_2D);

	// draw red highlight over carved brushes
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glColor4f(g_colors.selection[0],
				g_colors.selection[1],
				g_colors.selection[2],
				0.3f);
	for (auto brIt = brList->begin(); brIt != brList->end(); ++brIt)
		for (face = (*brIt)->faces; face; face = face->fnext)
			face->Draw();
	glDisable(GL_BLEND);

	DrawClipWire(brList);
	glEnable(GL_TEXTURE_2D);
}





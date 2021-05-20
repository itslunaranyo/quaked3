//==============================
//	SelectTool.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "SelectTool.h"
#include "select.h"
#include "CameraView.h"
#include "GridView.h"
#include "ZView.h"
#include "WndZChecker.h"
#include "WndCamera.h"
#include "WndGrid.h"
#include "win_dlg.h"

SelectTool::SelectTool() : 
	selecting(true),
	Tool("Selection", false)	// always on (not modal)
{
}


SelectTool::~SelectTool()
{
}

/*
Shift - brush select/deselect
Shift + Ctrl - face select/deselect
Shift + Alt - brush drill select (exclusive by nature)
Shift + Ctrl + Alt - exclusive select/deselect
*/

bool SelectTool::Input3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndCamera &vWnd)
{
	vec3 ray;
	int x, y;
	vWnd.GetMsgXY(lParam, x, y);

	switch (uMsg)
	{
	//case WM_COMMAND:
	//	return InputCommand(wParam);
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
		if (!(wParam & MK_SHIFT)) return false;
		hot = true;
		SetCapture(vWnd.wHwnd);
		xDown = x;
		yDown = y;
		v.PointToRay(x, y, ray);

		if (wParam & MK_CONTROL)
		{
			selFlags = SF_FACES;
			if (AltDown())
				selFlags |= SF_EXCLUSIVE;
		}
		else
		{
			selFlags = 0;
			if (AltDown())
				selFlags |= SF_CYCLE;
		}
		if (uMsg == WM_LBUTTONDBLCLK)
		{
			hot = false;
			ReleaseCapture();
			selFlags |= SF_EXPAND;
		}

		TrySelect(wParam, v.GetOrigin(), ray, selFlags);
		return true;

	case WM_MOUSEMOVE:
		if (!hot) return false;
		if (x == xDown && y == yDown) return false;	// ignore the one immediate move that gets sent after a mousedown
		if (!(wParam & MK_LBUTTON)) return false;
		if (selFlags & SF_CYCLE) return false;
		if (selFlags & SF_FACES && selFlags & SF_EXCLUSIVE) return false;
		v.PointToRay(x, y, ray);
		TrySelect(wParam, v.GetOrigin(), ray, selFlags);
		return true;

	case WM_LBUTTONUP:
		if (!hot) return false;
		hot = false;
		if (!(wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
			ReleaseCapture();
		return true;
	}
	return false;
}

bool SelectTool::Input2D(UINT uMsg, WPARAM wParam, LPARAM lParam, GridView &v, WndGrid &vWnd)
{
	int x, y;
	vec3 rayOrg, rayEnd;
	vWnd.GetMsgXY(lParam, x, y);

	switch (uMsg)
	{
	//case WM_COMMAND:
	//	return InputCommand(wParam);
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
		if (!(wParam & MK_SHIFT)) return false;
		SetCapture(vWnd.wHwnd);
		hot = true;
		v.ScreenToWorld(x, y, rayOrg);

		// can't select faces in 2d view, but we do select entities first for ease of use reasons
		selFlags = SF_ENTITIES_FIRST;
		if (AltDown())
			selFlags |= SF_CYCLE;
		if (uMsg == WM_LBUTTONDBLCLK)
			selFlags |= SF_EXPAND;

		rayOrg[v.GetAxis()] = g_cfgEditor.MapSize / 2;
		rayEnd[v.GetAxis()] = -1;

		TrySelect(wParam, rayOrg, rayEnd, selFlags);
		return true;
	case WM_LBUTTONUP:
		if (!hot) return false;
		hot = false;
		if (!(wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
			ReleaseCapture();
		return true;
	}
	return false;
}

bool SelectTool::Input1D(UINT uMsg, WPARAM wParam, LPARAM lParam, ZView &v, WndZChecker &vWnd)
{
	int x, y;
	vec3 point;
	vWnd.GetMsgXY(lParam, x, y);

	switch (uMsg)
	{
	//case WM_COMMAND:
	//	return InputCommand(wParam);
	case WM_LBUTTONDOWN:
		if (!(wParam & MK_SHIFT)) return false;
		hot = true;
		SetCapture(vWnd.wHwnd);
		v.ScreenToWorld(x, y, point);

		Selection::Point(point, 0);
		return true;

	case WM_LBUTTONUP:
		if (!hot) return false;
		hot = false;
		if (!(wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
			ReleaseCapture();
		return true;
	}
	return false;
}

bool SelectTool::Input(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg != WM_COMMAND)
		return false;

	return InputCommand(wParam);
}

/*
===========
SelectTool::TrySelect

buttons == wParam
===========
*/
bool SelectTool::TrySelect(int buttons, const vec3 origin, const vec3 dir, int &flags)
{
	if (!(flags & SF_FACES) || 
		(flags & SF_FACES && flags & SF_EXCLUSIVE))
		Selection::DeselectAllFaces();

	if ((flags & SF_FACES) && (Selection::g_selMode != sel_face))
	{
		Selection::DeselectAll();
		Selection::g_selMode = sel_face;
	}

	int newflags;
	newflags = Selection::Ray(origin, dir, flags);
	if (newflags && !(flags & (SF_SELECTED|SF_UNSELECTED)))
	{
		flags = newflags;
		return true;
	}
	return false;
}

bool SelectTool::InputCommand(WPARAM w)
{
	switch (LOWORD(w))
	{
	case ID_SELECTION_INVERT:
		Selection::Invert();
		return true;
	
	// lunaran - back and forth face<->brush conversion
	case ID_SELECTION_FACESTOBRUSHESPARTIAL:
		Selection::FacesToBrushes(true);
		return true;
	case ID_SELECTION_FACESTOBRUSHESCOMPLETE:
		Selection::FacesToBrushes(false);
		return true;
	case ID_SELECTION_BRUSHESTOFACES:
		Selection::BrushesToFaces();
		return true;

	case ID_SELECTION_SELECTCOMPLETETALL:
		Selection::CompleteTall();
		return true;
	case ID_SELECTION_SELECTTOUCHING:
		Selection::Touching();
		return true;
	case ID_SELECTION_SELECTPARTIALTALL:
		Selection::PartialTall();
		return true;
	case ID_SELECTION_SELECTINSIDE:
		Selection::Inside();
		return true;

	case ID_SELECTION_SELECTALL:	// sikk - Select All
		// lunaran FIXME: ensure this works in the entity edit fields
		Selection::SelectAll();
		return true;
	case ID_SELECTION_SELECTALLTYPE:	// sikk - Select All Type
		Selection::AllType();
		return true;
	case ID_SELECTION_SELECTMATCHINGTEXTURES:	// sikk - Select Matching Textures
		Selection::MatchingTextures();
		return true;
	case ID_SELECTION_SELECTMATCHINGKEYVALUE:	// sikk - Select Matching Key/Value
		DoFindKeyValue();
		return true;
	}
	return false;
}


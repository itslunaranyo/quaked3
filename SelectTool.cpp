//==============================
//	SelectTool.cpp
//==============================

#include "qe3.h"
#include "SelectTool.h"

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
Shift + Ctrl - single face select/deselect
Shift + Alt - brush drill select
Shift + Ctrl + Alt - multiface select/deselect
*/

bool SelectTool::Input3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndView &vWnd)
{
	vec3 ray;
	int x, y;
	vWnd.GetMsgXY(lParam, x, y);

	switch (uMsg)
	{
	//case WM_COMMAND:
	//	return InputCommand(wParam);
	case WM_LBUTTONDOWN:
		if (!(wParam & MK_SHIFT)) return false;
		hot = true;
		SetCapture(vWnd.w_hwnd);
		v.PointToRay(x, y, ray);

		if (wParam & MK_CONTROL)
		{
			selFlags = SF_FACES;
		if (AltDown())
			selFlags |= SF_MULTIFACE;
		}
		else
		{
			selFlags = 0;
			if (AltDown())
				selFlags |= SF_CYCLE;
		}

		TrySelect(wParam, v.origin, ray, selFlags);
		return true;

	case WM_MOUSEMOVE:
		if (!hot) return false;
		if (!(wParam & MK_LBUTTON)) return false;
		if (selFlags & SF_CYCLE) return false;
		if (selFlags & SF_FACES && !(selFlags & SF_MULTIFACE)) return false;
		v.PointToRay(x, y, ray);
		TrySelect(wParam, v.origin, ray, selFlags);
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

bool SelectTool::Input2D(UINT uMsg, WPARAM wParam, LPARAM lParam, XYZView &v, WndView &vWnd)
{
	int x, y;
	vec3 rayOrg, rayEnd;
	vWnd.GetMsgXY(lParam, x, y);

	switch (uMsg)
	{
	//case WM_COMMAND:
	//	return InputCommand(wParam);
	case WM_LBUTTONDOWN:
		if (!(wParam & MK_SHIFT)) return false;
		SetCapture(vWnd.w_hwnd);
		hot = true;
		v.ToPoint(x, y, rayOrg);

		// can't select faces in 2d view, but we do select entities first for ease of use reasons
		selFlags = SF_ENTITIES_FIRST;
		if (AltDown())
			selFlags |= SF_CYCLE;

		switch (v.GetAxis())
		{
		case YZ:
			rayOrg.x = g_qeglobals.d_savedinfo.nMapSize / 2;
			rayEnd.x = -1;
			break;
		case XZ:
			rayOrg.y = g_qeglobals.d_savedinfo.nMapSize / 2;
			rayEnd.y = -1;
			break;
		case XY:
		default:
			rayOrg.z = g_qeglobals.d_savedinfo.nMapSize / 2;
			rayEnd.z = -1;
			break;
		}

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

bool SelectTool::Input1D(UINT uMsg, WPARAM wParam, LPARAM lParam, ZView &v, WndView &vWnd)
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
		SetCapture(vWnd.w_hwnd);
		v.ToPoint(x, y, point);

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
		(flags & SF_FACES) && !(flags & SF_MULTIFACE))
		Selection::DeselectAllFaces();

	if ((flags & SF_FACES) && (g_qeglobals.d_selSelectMode != sel_face))
	{
		Selection::DeselectAll();
		g_qeglobals.d_selSelectMode = sel_face;
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


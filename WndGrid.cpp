//==============================
//	WndGrid.c
//==============================

#include "pre.h"
#include "qe3.h"
#include "win_dlg.h"
#include "WndGrid.h"
#include "select.h"
#include "GridView.h"
#include "GridViewRenderer.h"
#include "CameraView.h"
#include "ZView.h"
#include "Tool.h"

HWND g_hwndGrid[4];

WndGrid::WndGrid()
{
	name = "GridWindow";
	vbits = W_XY;
}


WndGrid::~WndGrid()
{
}

void WndGrid::Initialize(int winNum)
{
	gv = &g_vGrid[winNum];
	v = gv;
	instance = winNum;

	Create();
	g_hwndGrid[winNum] = wHwnd;

	gr = new GridViewRenderer(*gv);
	r = gr;

	// set the axis after creating the window so the title can be changed
	SetAxis((winNum + 2) % 3);
}

void WndGrid::SetAxis(int viewAxis) { SetAxis((eViewType_t)viewAxis); }
void WndGrid::SetAxis(eViewType_t viewAxis)
{
	gv->SetAxis(viewAxis);

	if (viewAxis == GRID_YZ)
		SetTitle("Side View (YZ)");
	else if (viewAxis == GRID_XZ)
		SetTitle("Front View (XZ)");
	else
		SetTitle("Top View (XY)");

	ForceUpdate();
}

void WndGrid::CycleAxis() { SetAxis((gv->GetAxis() + 1) % 3); }

#define SELECTION_POINTENT	1
#define SELECTION_BRUSHES	2
#define SELECTION_BRUSHENT	4

/*
============
GetSelectionInfo
============
*/
int GetSelectionInfo()
{
	int		retval = 0;
	Brush	*b;

	// lunaran TODO: what the fuck
	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		if (b->owner->IsPoint())
			retval = SELECTION_POINTENT;
		else if (b->owner->IsWorld())
			retval = SELECTION_BRUSHES;
		else
			retval = SELECTION_BRUSHENT;
	}

	return retval;
}

void WndGrid::DoPopupMenu(int x, int y)
{
	HMENU	hMenu;
	POINT	point;
	int		retval;
	int		selected = GetSelectionInfo();
	vec3	org;

	hMenu = GetSubMenu(LoadMenu(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDR_CONTEXT)), 0);

	GetCursorPos(&point);
	gv->ScreenToWorldSnapped(x, y, org);

	switch (selected)
	{
	case SELECTION_POINTENT:
		EnableMenuItem(hMenu, ID_SELECTION_CSGHOLLOW, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_CSGSUBTRACT, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_CSGMERGE, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_CSGCONVEXMERGE, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_SELECTCOMPLETETALL, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_SELECTTOUCHING, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_SELECTPARTIALTALL, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_SELECTINSIDE, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REGION_SETBRUSH, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REGION_SETTALLBRUSH, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REGION_SETSELECTION, MF_GRAYED);
		//	EnableMenuItem(hMenu, ID_MENU_CREATEANYENTITY, MF_GRAYED);
		EnableMenuItem(hMenu, ID_MENU_CREATEBRUSHENTITY, MF_GRAYED);
		EnableMenuItem(hMenu, ID_MENU_CREATEPOINTENTITY, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_UNGROUPENTITY, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_INSERTBRUSH, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_DESELECT, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_GROUPNEXTBRUSH, MF_GRAYED);
		break;
	case SELECTION_BRUSHENT:
		//	EnableMenuItem(hMenu, ID_MENU_CREATEANYENTITY, MF_GRAYED);
		EnableMenuItem(hMenu, ID_MENU_CREATEBRUSHENTITY, MF_GRAYED);
		EnableMenuItem(hMenu, ID_MENU_CREATEPOINTENTITY, MF_GRAYED);
		EnableMenuItem(hMenu, ID_MISC_SELECTENTITYCOLOR, MF_GRAYED);
		break;
	case SELECTION_BRUSHES:
		EnableMenuItem(hMenu, ID_SELECTION_CONNECT, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_UNGROUPENTITY, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_INSERTBRUSH, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_DESELECT, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_GROUPNEXTBRUSH, MF_GRAYED);
		//	EnableMenuItem(hMenu, ID_MISC_SELECTENTITYCOLOR, MF_GRAYED);
		break;
	default:
		EnableMenuItem(hMenu, ID_EDIT_CUT, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EDIT_COPY, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_CLONE, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_DESELECT, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_INVERT, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_DELETE, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_CSGHOLLOW, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_CSGSUBTRACT, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_CSGMERGE, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_CSGCONVEXMERGE, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_DESELECT, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_GROUPNEXTBRUSH, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_SELECTCOMPLETETALL, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_SELECTTOUCHING, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_SELECTPARTIALTALL, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_SELECTINSIDE, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_SELECTALLTYPE, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REGION_SETBRUSH, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REGION_SETTALLBRUSH, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REGION_SETSELECTION, MF_GRAYED);
	//	EnableMenuItem(hMenu, ID_MENU_CREATEANYENTITY, MF_GRAYED);
	//	EnableMenuItem(hMenu, ID_MENU_CREATEBRUSHENTITY, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_CONNECT, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_UNGROUPENTITY, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_INSERTBRUSH, MF_GRAYED);
		EnableMenuItem(hMenu, ID_MISC_SELECTENTITYCOLOR, MF_GRAYED);
		break;
	}

	EnableMenuItem(hMenu, ID_REGION_SETXZ, (g_wndGrid[2]->IsOpen() ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(hMenu, ID_REGION_SETYZ, (g_wndGrid[1]->IsOpen() ? MF_ENABLED : MF_GRAYED));

	retval = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON, point.x, point.y, 0, wHwnd, NULL);

	switch (retval)
	{
	case ID_MENU_CREATEANYENTITY:
		DoCreateEntity(true, true, true, org);
		break;
	case ID_MENU_CREATEBRUSHENTITY:
		DoCreateEntity(false, true, true, org);
		break;
	case ID_MENU_CREATEPOINTENTITY:
		org[gv->GetAxis()] = g_qeglobals.d_v3WorkMin[gv->GetAxis()];

		DoCreateEntity(true, false, false, org);
		break;
	default:
		PostMessage(g_hwndMain, WM_COMMAND, retval, 0);
	}

	DestroyMenu(hMenu);
}


int WndGrid::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (Tool::HandleInput2D(uMsg, wParam, lParam, *gv, *this))
	{
		if (uMsg > WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)
			Focus();
		return 1;
	}
	return 1;
}

int WndGrid::OnResized()
{
	gv->Resize(clientRect.right, clientRect.bottom);
	return 0;
}

void WndGrid::Render()
{
	gr->Draw();
}

/*
==============
WndGrid::MouseDown
==============
*/
void WndGrid::MouseDown(const int x, const int y, const int btndown, const int buttons)
{
	vec3	point, pointP;
	vec3	orgLocal, dir, right, up;

	gv->ScreenToWorld(x, y, point);
	orgLocal = point;

	orgLocal[gv->GetAxis()] = 8192;
	dir[gv->GetAxis()] = -1;
	right[gv->DimU()] = 1 / gv->GetScale();
	up[gv->DimV()] = 1 / gv->GetScale();

	Sys_GetCursorPos(&cursorX, &cursorY);

	if (btndown == MK_RBUTTON)
	{
		rMoved = false;
		mDownX = x;
		mDownY = y;
	}
	else if (btndown == MK_MBUTTON)
	{
		// Ctrl+MMB = place camera
		// Alt+MMB = place camera and drag to aim
		if (buttons & MK_CONTROL || GetKeyState(VK_MENU) < 0)
		{
			pointP = g_vCamera.GetOrigin();
			gv->CopyVectorPlanar(point, pointP);
			g_vCamera.SetOrigin(pointP);

			WndMain_UpdateWindows(W_SCENE);
			return;
		}
		// Shift+MMB = place z checker
		else if (buttons & MK_SHIFT)
		{
			gv->ScreenToWorldGrid(x, y, point);
			pointP = g_vZ.GetOrigin();
			gv->CopyVectorPlanar(point, pointP);
			g_vZ.SetOrigin(pointP);
			WndMain_UpdateWindows(W_XY | W_Z);
			return;
		}
		// MMB = angle camera
		else if (buttons == MK_MBUTTON)
		{
			gv->AngleCamera(point);
			return;
		}
	}
}


/*
==============
WndGrid::MouseUp
==============
*/
void WndGrid::MouseUp(const int x, const int y, const int btnup, const int buttons)
{
	if (btnup == MK_RBUTTON && !buttons && !rMoved)
		DoPopupMenu(x, y);
}

/*
==============
WndGrid::MouseMoved
==============
*/
void WndGrid::MouseMoved(const int x, const int y, const int buttons)
{
	vec3	point, pointP;
	vec3	tdp;
	char	xystring[256];
	int		cx, cy;

	Sys_GetCursorPos(&cx, &cy);
	if (cx == cursorX && cy == cursorY)
		return;

	if (!buttons || buttons ^ MK_RBUTTON)
	{
		gv->ScreenToWorldGrid(x, y, tdp);
		sprintf(xystring, "xyz Coordinates: (%d %d %d)", (int)tdp[0], (int)tdp[1], (int)tdp[2]);
		WndMain_Status(xystring, 0);
	}

	if (!buttons)
		return;

	// Ctrl+MMB = move camera
	if (buttons == (MK_CONTROL | MK_MBUTTON))
	{
		gv->ScreenToWorldGrid(x, y, point);
		pointP = g_vCamera.GetOrigin();
		gv->CopyVectorPlanar(point, pointP);
		g_vCamera.SetOrigin(pointP);

		WndMain_UpdateWindows(W_SCENE);
		return;
	}

	// Shift+MMB = move z checker
	if (buttons == (MK_SHIFT | MK_MBUTTON))
	{
		gv->ScreenToWorldGrid(x, y, point);
		pointP = g_vZ.GetOrigin();
		gv->CopyVectorPlanar(point, pointP);
		g_vZ.SetOrigin(pointP);
		WndMain_UpdateWindows(W_XY | W_Z);
		return;
	}

	// MMB = angle camera
	if (buttons == MK_MBUTTON)
	{
		gv->ScreenToWorldGrid(x, y, point);
		gv->AngleCamera(point);
		return;
	}

	// RMB = drag xy origin
	if (buttons == MK_RBUTTON)
	{
		if (!rMoved && (abs(mDownX - x) > 1 || abs(mDownY - y) > 1))
			rMoved = true;

		if (rMoved) SetCursor(NULL);

		if ((cx != cursorX || cy != cursorY) && rMoved)
		{
			gv->Shift(-(cx - cursorX) / gv->GetScale(), (cy - cursorY) / gv->GetScale());

			Sys_SetCursorPos(cursorX, cursorY);
			WndMain_UpdateWindows(W_XY | W_Z);

//			sprintf(xystring, "this Origin: (%d %d %d)", (int)origin[0], (int)origin[1], (int)origin[2]);
//			WndMain_Status(xystring, 0);
		}
		return;
	}

	// sikk---> Mouse Zoom
	// Ctrl+RMB = zoom xy view
	if (buttons == (MK_CONTROL | MK_RBUTTON))
	{
		SetCursor(NULL); // sikk - Remove Cursor

		if (cy != cursorY)
		{
			vec3 tgt;
			gv->ScreenToWorld(x, y, tgt);
			gv->Zoom(tgt, powf((cy > cursorY) ? 1.01f : 0.99f, fabs(cy - cursorY)));
			Sys_SetCursorPos(cursorX, cursorY);
			WndMain_UpdateWindows(W_XY);
		}
		return;
	}
	// <---sikk
}


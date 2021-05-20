//==============================
//	WndGrid.c
//==============================

#include "qe3.h"

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
	xyzv = &g_qeglobals.d_vXYZ[winNum];
	v = xyzv;
	instance = winNum;

	CreateWnd();
	g_qeglobals.d_hwndXYZ[winNum] = w_hwnd;

	// set the axis after creating the window so the title can be changed
	SetAxis((winNum + 2) % 3);
}

void WndGrid::SetAxis(int viewAxis)
{
	xyzv->SetAxis(viewAxis);

	if (viewAxis == YZ)
		SetTitle("Side View (YZ)");
	else if (viewAxis == XZ)
		SetTitle("Front View (XZ)");
	else
		SetTitle("Top View (XY)");

	ForceUpdate();
}

void WndGrid::CycleAxis()
{
	SetAxis((xyzv->GetAxis() + 1) % 3);
}

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

// only used by clip tool ATM
XYZView* XYZWnd_WinFromHandle(HWND xyzwin)
{
	for (int i = 0; i < 4; i++)
	{
		if (g_qeglobals.d_hwndXYZ[i] == xyzwin)
			return &g_qeglobals.d_vXYZ[i];
	}

	return nullptr;
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
	xyzv->ToGridPoint(x, y, org);

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

	EnableMenuItem(hMenu, ID_REGION_SETXZ, (g_qeglobals.d_wndGrid[2]->Open() ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(hMenu, ID_REGION_SETYZ, (g_qeglobals.d_wndGrid[1]->Open() ? MF_ENABLED : MF_GRAYED));

	retval = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON, point.x, point.y, 0, w_hwnd, NULL);

	switch (retval)
	{
	case ID_MENU_CREATEANYENTITY:
		DoCreateEntity(true, true, true, org);
		break;
	case ID_MENU_CREATEBRUSHENTITY:
		DoCreateEntity(false, true, true, org);
		break;
	case ID_MENU_CREATEPOINTENTITY:
		org[xyzv->GetAxis()] = g_qeglobals.d_v3WorkMin[xyzv->GetAxis()];

		DoCreateEntity(true, false, false, org);
		break;
	default:
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, retval, 0);
	}

	DestroyMenu(hMenu);
}


int		g_nMouseX, g_nMouseY;
bool	g_bMoved;

int WndGrid::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int		fwKeys, xPos, yPos;

	if (Tool::HandleInput2D(uMsg, wParam, lParam, *xyzv, *this))
	{
		if (uMsg > WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)
			Focus();
		return 1;
	}

	switch (uMsg)
	{
	case WM_KEYDOWN:
		return QE_KeyDown(wParam);
	case WM_KEYUP:
		return QE_KeyUp(wParam);

	case WM_MOUSEWHEEL:
		Focus();
		if ((short)HIWORD(wParam) < 0)
		{
			v->scale = max(0.05f, v->scale * 0.8f);
		}
		else
		{
			v->scale = min(32.0f, v->scale * 1.25f);
		}
		Sys_UpdateWindows(vbits);
		return 0;

	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
		Focus();
		SetCapture(w_hwnd);
		fwKeys = wParam;        // key flags 
		xPos = (short)LOWORD(lParam);  // horizontal position of cursor 
		yPos = (short)HIWORD(lParam);  // vertical position of cursor 
		yPos = (int)clientRect.bottom - 1 - yPos;
		v->MouseDown(xPos, yPos, fwKeys);
		// sikk---> Context Menu
		if (uMsg == WM_RBUTTONDOWN)
		{
			g_bMoved = false;
			g_nMouseX = xPos;
			g_nMouseY = yPos;
		}
		// <---sikk
		return 0;

	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_LBUTTONUP:
		fwKeys = wParam;        // key flags 
		xPos = (short)LOWORD(lParam);  // horizontal position of cursor 
		yPos = (short)HIWORD(lParam);  // vertical position of cursor 
		yPos = (int)clientRect.bottom - 1 - yPos;
		v->MouseUp(xPos, yPos, fwKeys);
		if (!(fwKeys & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
			ReleaseCapture();
		// sikk---> Context Menu
		if (uMsg == WM_RBUTTONUP && !g_bMoved)
			DoPopupMenu(xPos, yPos);
		// <---sikk
		return 0;

	case WM_MOUSEMOVE:
		fwKeys = wParam;        // key flags 
		xPos = (short)LOWORD(lParam);  // horizontal position of cursor 
		yPos = (short)HIWORD(lParam);  // vertical position of cursor 
		yPos = (int)clientRect.bottom - 1 - yPos;
		v->MouseMoved(xPos, yPos, fwKeys);
		// sikk---> Context Menu
		if (!g_bMoved && (g_nMouseX != xPos || g_nMouseY != yPos))
			g_bMoved = true;
		// <---sikk
		return 0;
	}
	return 1;
}

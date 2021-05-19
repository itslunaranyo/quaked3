//==============================
//	win_insp.c
//==============================

#include "qe3.h"

/*
===============================================================

INSPECTOR WINDOW

parent container for entity editor, texture browser, and console

===============================================================
*/


void InspWnd_ToTop()
{
	//	SetFocus(g_qeglobals.d_hwndInspector);
	if (GetTopWindow(g_qeglobals.d_hwndMain) != g_qeglobals.d_hwndInspector)
		BringWindowToTop(g_qeglobals.d_hwndInspector);
}

/*
==============
InspWnd_Create
==============
*/
void InspWnd_Create(HINSTANCE hInstance)
{
	WNDCLASS   wc;

	/* Register the entity class */
	memset(&wc, 0, sizeof(wc));

	wc.style = 0;
	wc.lpfnWndProc = (WNDPROC)InspWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = 0;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = INSPECTOR_WINDOW_CLASS;

	if (!RegisterClass(&wc))
		Error("InspWnd_Register: Failed.");

	// sikk---> changed window to "tool window"
	g_qeglobals.d_hwndInspector = CreateWindowEx(
		WS_EX_TOOLWINDOW,							// Extended window style
		INSPECTOR_WINDOW_CLASS,						// registered class name
		"Inspector",								// window name
		QE3_CHILD_STYLE,							// window style
		g_nScreenWidth - 320, g_nScreenHeight - 520,	// position of window
		300, 480,									// window size
		g_qeglobals.d_hwndMain,						// parent or owner window
		0,											// menu or child-window identifier
		hInstance,									// application instance
		NULL);										// window-creation data
													// <---sikk

	if (!g_qeglobals.d_hwndInspector)
		Error("Could not create Inspector Window.");

	EntWnd_Create(hInstance);
	TexWnd_Create(hInstance);
	ConsoleWnd_Create(hInstance);

	LoadWindowState(g_qeglobals.d_hwndInspector, "EntityWindow");
	ShowWindow(g_qeglobals.d_hwndInspector, SW_SHOW);
	InspWnd_SetMode(W_CONSOLE);
}


/*
==============
InspWnd_SetMode
==============
*/
void InspWnd_SetMode(int nType)
{
	HMENU	hMenu = GetMenu(g_qeglobals.d_hwndMain);

	// Is the caller asking us to cycle to the next window?
	if (nType == -1)
	{
		if (g_qeglobals.d_nInspectorMode == W_ENTITY)
			nType = W_TEXTURE;
		else if (g_qeglobals.d_nInspectorMode == W_TEXTURE)
			nType = W_CONSOLE;
		else
			nType = W_ENTITY;
	}

	// lunaran FIXME: ShowWindow does fuckall
	g_qeglobals.d_nInspectorMode = nType;
	switch (nType)
	{
	case W_ENTITY:
		SetWindowText(g_qeglobals.d_hwndInspector, "Entities");
		ShowWindow(g_qeglobals.d_hwndTexture, SW_HIDE);
		ShowWindow(g_qeglobals.d_hwndConsole, SW_HIDE);
		ShowWindow(g_qeglobals.d_hwndEntity, SW_SHOW);
	//	EnableMenuItem(hMenu, ID_MISC_SELECTENTITYCOLOR, MF_ENABLED);
		break;

	case W_TEXTURE:
		SetWindowText(g_qeglobals.d_hwndInspector, "Textures"); // title is set by textures.c
		ShowWindow(g_qeglobals.d_hwndEntity, SW_HIDE);
		ShowWindow(g_qeglobals.d_hwndConsole, SW_HIDE);
		ShowWindow(g_qeglobals.d_hwndTexture, SW_SHOW);
	//	EnableMenuItem(hMenu, ID_MISC_SELECTENTITYCOLOR, MF_GRAYED | MF_DISABLED);
		break;

	case W_CONSOLE:
		SetWindowText(g_qeglobals.d_hwndInspector, "Console");
		ShowWindow(g_qeglobals.d_hwndEntity, SW_HIDE);
		ShowWindow(g_qeglobals.d_hwndTexture, SW_HIDE);
		ShowWindow(g_qeglobals.d_hwndConsole, SW_SHOW);
	//	EnableMenuItem(hMenu, ID_MISC_SELECTENTITYCOLOR, MF_GRAYED | MF_DISABLED);
		break;

	default:
		break;
	}

	InspWnd_Resize();
	InspWnd_ToTop();
}

/*
==============
InspWnd_Move
==============
*/
void InspWnd_Move(HWND hwnd, int x, int y, int w, int h)
{
	SetWindowPos(hwnd, HWND_TOP, x, y, w, h, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOZORDER);
}

/*
==============
InspWnd_MoveRect
==============
*/
void InspWnd_MoveRect(HWND hwnd, RECT r)
{
	SetWindowPos(hwnd, HWND_TOP, r.left, r.top, r.right, r.bottom, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOZORDER);
}

/*
===============
InspWnd_Resize
===============
*/
void InspWnd_Resize()
{
	RECT rcOn, rcOff;

	// use right/bottom as width/height:
	GetClientRect(g_qeglobals.d_hwndInspector, &rcOn);
	rcOn.right -= rcOn.left;
	rcOn.bottom -= rcOn.top;
	rcOff = rcOn;
	rcOff.left += rcOn.right;

	if (rcOn.right < 100 || rcOn.bottom < 100)
		return;

	EntWnd_Resize((g_qeglobals.d_nInspectorMode == W_ENTITY) ? rcOn : rcOff);
	TexWnd_Resize((g_qeglobals.d_nInspectorMode == W_TEXTURE) ? rcOn : rcOff);
	ConsoleWnd_Resize((g_qeglobals.d_nInspectorMode == W_CONSOLE) ? rcOn : rcOff);

	//RedrawWindow(g_qeglobals.d_hwndInspector, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ERASENOW | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

/*
=========================
InspWndProc
=========================
*/
BOOL CALLBACK InspWndProc(
	HWND	hwndDlg,// handle to dialog box
	UINT	uMsg,	// message
	WPARAM	wParam,	// first message parameter
	LPARAM	lParam	// second message parameter
	)
{
	RECT	rc;

	GetClientRect(hwndDlg, &rc);
	switch (uMsg)
	{
	case WM_GETMINMAXINFO:
	{
		LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;;
		lpmmi->ptMinTrackSize.x = 320;
		lpmmi->ptMinTrackSize.y = 480; // 500
	}
	return 0;

	case WM_WINDOWPOSCHANGING:
	case WM_WINDOWPOSCHANGED:
		InspWnd_Resize();
		return 0;

		// sikk---> Window Management
	case WM_SIZING:
		if (TryDocking(hwndDlg, wParam, (LPRECT)lParam, 300))
			return TRUE;
		break;

	case WM_MOVING:
		if (TryDocking(hwndDlg, wParam, (LPRECT)lParam, 0))
			return TRUE;
		break;
		// <---sikk

		// sikk---> LMB Bring to Top
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
		if (GetTopWindow(g_qeglobals.d_hwndMain) != hwndDlg)
			BringWindowToTop(hwndDlg);
		break;
		// <---sikk
	}
	return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
}


// ==================================================
// the console will live here until it evolves into something worth its own file

/*
================
ConsoleWnd_Create
================
*/
void ConsoleWnd_Create(HINSTANCE hInstance)
{
	g_qeglobals.d_hwndConsole = CreateWindowEx(WS_EX_CLIENTEDGE,
		"RichEdit",
		NULL,
		ES_MULTILINE | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_VSCROLL,
		5, 100, 180, 99,
		g_qeglobals.d_hwndInspector,
		(HMENU)IDC_E_STATUS,
		g_qeglobals.d_hInstance,
		NULL);

	if (!g_qeglobals.d_hwndConsole)
		Error("CreateWindowEx: Failed.");

	SendMessage(g_qeglobals.d_hwndConsole, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)TRUE);
}

/*
===============
ConsoleWnd_Resize
===============
*/
void ConsoleWnd_Resize(RECT rc)
{
	InspWnd_MoveRect(g_qeglobals.d_hwndConsole, rc);
}




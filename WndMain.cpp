//==============================
//	WndMain.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "WndMain.h"
#include "WndCamera.h"
#include "WndGrid.h"
#include "WndZChecker.h"
#include "WndTexture.h"
#include "WndEntity.h"
#include "WndConsole.h"
#include "WndSurf.h"

#include "TextureView.h"
#include "Tool.h"
#include "Command.h"
#include "map.h"
#include "WndFiles.h"

HWND	g_hwndMain;
HWND	g_hwndSplash;


WndCamera	*g_wndCamera;
WndGrid		*g_wndGrid[4];
WndZChecker	*g_wndZ;
WndTexture	*g_wndTexture;
WndEntity	*g_wndEntity;
WndConsole	*g_wndConsole;

int		g_nUpdateBits = 0;
int		g_nScreenWidth;
int		g_nScreenHeight;
int		g_nInspectorMode;		// W_TEXTURE, W_ENTITY, or W_CONSOLE


/*
============
SplashDlgProc
============
*/
BOOL CALLBACK SplashDlgProc(
	HWND	hwndDlg,// handle to dialog box
	UINT	uMsg,	// message
	WPARAM	wParam,	// first message parameter
	LPARAM	lParam 	// second message parameter
)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		ShowWindow(hwndDlg, SW_SHOW);
		InvalidateRect(hwndDlg, NULL, FALSE);
		UpdateWindow(hwndDlg);
		SetTimer(hwndDlg, QE_TIMERSPLASH, 3000, nullptr);
		return FALSE;

	case WM_TIMER:
//	case WM_LBUTTONDOWN:
		DestroyWindow(hwndDlg);
		return FALSE;
	case WM_CLOSE:
		KillTimer(hwndDlg, QE_TIMERSPLASH);
		return FALSE;
	}
	return 0;
}

void WndSplash_Create()
{
	g_hwndSplash = CreateDialog(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_SPLASH), g_hwndMain, SplashDlgProc);
}
void WndSplash_Destroy() 
{
	DestroyWindow(g_hwndSplash);
}

/*
==============================================================================

	CHILD WINDOWS

==============================================================================
*/

/*
==================
WndMain_SwapGridCam
==================
*/
void WndMain_SwapGridCam()
{
	g_wndCamera->Swap(*g_wndGrid[0]);
}

/*
==================
WndMain_SyncInspectorPlacement

one day we'll have a nice gui toolkit with dockable tabbed panes and this will be
automatic and customizable and invisible and great
==================
*/
void WndMain_SyncInspectorPlacement()
{
	if (IsWindowVisible(g_hwndTexture))
	{
		g_wndConsole->CopyPositionFrom(*g_wndTexture);
		g_wndEntity->CopyPositionFrom(*g_wndTexture);
	}
	else if (IsWindowVisible(g_hwndConsole))
	{
		g_wndTexture->CopyPositionFrom(*g_wndConsole);
		g_wndEntity->CopyPositionFrom(*g_wndConsole);
	}
	else if(IsWindowVisible(g_hwndEntity))
	{
		g_wndConsole->CopyPositionFrom(*g_wndEntity);
		g_wndTexture->CopyPositionFrom(*g_wndEntity);
	}
}

/*
==================
WndMain_ForceInspectorMode
==================
*/
void WndMain_ForceInspectorMode(int nType)
{
	g_wndEntity->Hide();
	g_wndTexture->Hide();
	g_wndConsole->Hide();
	switch (nType)
	{
	case W_ENTITY:
		g_wndEntity->Show();
		break;
	case W_TEXTURE:
		g_wndTexture->Show();
		break;
	case W_CONSOLE:
	default:
		g_wndConsole->Show();
		g_wndConsole->ScrollToEnd();
		break;
	}

	g_nInspectorMode = nType;
}

/*
==================
WndMain_SetInspectorMode
==================
*/
void WndMain_SetInspectorMode(int nType)
{
	Window *from, *to;

	switch (g_nInspectorMode)
	{
	case W_ENTITY:
		from = g_wndEntity;
		break;
	case W_TEXTURE:
		from = g_wndTexture;
		break;
	case W_CONSOLE:
	default:
		from = g_wndConsole;
		break;
	}
	switch (nType)
	{
	case W_ENTITY:
		to = g_wndEntity;
		break;
	case W_TEXTURE:
		to = g_wndTexture;
		break;
	case W_CONSOLE:
	default:
		to = g_wndConsole;
		g_wndConsole->ScrollToEnd();
		break;
	}

	to->CopyPositionFrom(*from);
	to->Focus();
	WndMain_ForceInspectorMode(nType);
}

/*
==============
WndMain_DefaultLayout
=============
*/
void WndMain_DefaultLayout(int nStyle)
{
	RECT	parent, status, toolbar;
	int		hleft, h1, h2, h3, hright;
	int		vtop, v1, vbottom;

	GetWindowRect(g_hwndRebar, &toolbar);
	GetWindowRect(g_hwndStatus, &status);
	GetClientRect(g_hwndMain, &parent);
	parent.top = toolbar.bottom - toolbar.top;
	parent.bottom -= status.bottom - status.top;

	hleft = parent.left;
	hright = parent.right;
	vbottom = parent.bottom;
	vtop = parent.top;
	v1 = min(vbottom - 480, (vtop + vbottom) / 2);

	WndMain_SetInspectorMode(W_CONSOLE);

	switch (nStyle)
	{
	case 0:	// QE3 Default
		h1 = 64;
		h2 = hright * 0.625f;
		g_wndZ->SetPosition(hleft, vtop, h1, vbottom, true);

		g_wndGrid[0]->SetPosition(h1, vtop, h2, vbottom, true);
		g_wndGrid[1]->SetPosition(h1, vtop, h2, vbottom, false);
		g_wndGrid[2]->SetPosition(h1, vtop, h2, vbottom, false);

		g_wndCamera->SetPosition(h2, vtop, hright, v1, true);
		g_wndConsole->SetPosition(h2, v1, hright, vbottom, true);
		g_wndEntity->SetPosition(h2, v1, hright, vbottom, false);
		g_wndTexture->SetPosition(h2, v1, hright, vbottom, false);
		break;
	case 1:	// QE3 Reverse
		h1 = hright * 0.375f;
		h2 = hright - 64;
		g_wndCamera->SetPosition(hleft, vtop, h1, v1, true);
		g_wndConsole->SetPosition(hleft, v1, h1, vbottom, true);
		g_wndEntity->SetPosition(hleft, v1, h1, vbottom, false);
		g_wndTexture->SetPosition(hleft, v1, h1, vbottom, false);

		g_wndGrid[0]->SetPosition(h1, vtop, h2, vbottom, true);
		g_wndGrid[1]->SetPosition(h1, vtop, h2, vbottom, false);
		g_wndGrid[2]->SetPosition(h1, vtop, h2, vbottom, false);

		g_wndZ->SetPosition(h2, vtop, hright, vbottom, true);
		break;
	case 2:	// 4 Window w/ Z (Cam Left)
		h1 = 64;
		h3 = hright - 320;
		h2 = (h1 + h3) / 2;
		g_wndZ->SetPosition(hleft, vtop, h1, vbottom, true);

		g_wndCamera->SetPosition(h1, vtop, h2, v1, true);
		g_wndGrid[0]->SetPosition(h2, vtop, h3, v1, true);
		g_wndGrid[1]->SetPosition(h1, v1, h2, vbottom, true);
		g_wndGrid[2]->SetPosition(h2, v1, h3, vbottom, true);

		g_wndConsole->SetPosition(h3, vtop, hright, vbottom, true);
		g_wndEntity->SetPosition(h3, vtop, hright, vbottom, false);
		g_wndTexture->SetPosition(h3, vtop, hright, vbottom, false);
		break;
	case 3:	// 4 Window w/ Z (Cam Right)
		h1 = 64;
		h3 = hright - 320;
		h2 = (h1 + h3) / 2;
		g_wndZ->SetPosition(hleft, vtop, h1, vbottom, true);

		g_wndCamera->SetPosition(h2, vtop, h3, v1, true);
		g_wndGrid[0]->SetPosition(h1, vtop, h2, v1, true);
		g_wndGrid[1]->SetPosition(h1, v1, h2, vbottom, true);
		g_wndGrid[2]->SetPosition(h2, v1, h3, vbottom, true);

		g_wndConsole->SetPosition(h3, vtop, hright, vbottom, true);
		g_wndEntity->SetPosition(h3, vtop, hright, vbottom, false);
		g_wndTexture->SetPosition(h3, vtop, hright, vbottom, false);
		break;
	case 4:	// 4 Window w/o Z (Cam Left)
		h1 = 64;
		h3 = hright - 320;
		h2 = h3 / 2;
		g_wndZ->SetPosition(hleft, vtop, h1, vbottom, false);

		g_wndCamera->SetPosition(hleft, vtop, h2, v1, true);
		g_wndGrid[0]->SetPosition(h2, vtop, h3, v1, true);
		g_wndGrid[1]->SetPosition(hleft, v1, h2, vbottom, true);
		g_wndGrid[2]->SetPosition(h2, v1, h3, vbottom, true);

		g_wndConsole->SetPosition(h3, vtop, hright, vbottom, true);
		g_wndEntity->SetPosition(h3, vtop, hright, vbottom, false);
		g_wndTexture->SetPosition(h3, vtop, hright, vbottom, false);
		break;
	case 5:	// 4 Window w/o Z (Cam Right)
		h1 = 64;
		h3 = hright - 320;
		h2 = h3 / 2;
		g_wndZ->SetPosition(hleft, vtop, h1, vbottom, false);

		g_wndCamera->SetPosition(h2, vtop, h3, v1, true);
		g_wndGrid[0]->SetPosition(hleft, vtop, h2, v1, true);
		g_wndGrid[1]->SetPosition(hleft, v1, h2, vbottom, true);
		g_wndGrid[2]->SetPosition(h2, v1, h3, vbottom, true);

		g_wndConsole->SetPosition(h3, vtop, hright, vbottom, true);
		g_wndEntity->SetPosition(h3, vtop, hright, vbottom, false);
		g_wndTexture->SetPosition(h3, vtop, hright, vbottom, false);
		break;
	case 6:	// 4 Window Reverse w/ Z (Cam Left)
		h1 = 320;
		h3 = hright - 64;
		h2 = (h1 + h3) / 2;
		g_wndConsole->SetPosition(hleft, vtop, h1, vbottom, true);
		g_wndEntity->SetPosition(hleft, vtop, h1, vbottom, false);
		g_wndTexture->SetPosition(hleft, vtop, h1, vbottom, false);

		g_wndCamera->SetPosition(h1, vtop, h2, v1, true);
		g_wndGrid[0]->SetPosition(h2, vtop, h3, v1, true);
		g_wndGrid[1]->SetPosition(h1, v1, h2, vbottom, true);
		g_wndGrid[2]->SetPosition(h2, v1, h3, vbottom, true);

		g_wndZ->SetPosition(h3, vtop, hright, vbottom, true);
		break;
	case 7:	// 4 Window Reverse w/ Z (Cam Right)
		h1 = 320;
		h3 = hright - 64;
		h2 = (h1 + h3) / 2;
		g_wndConsole->SetPosition(hleft, vtop, h1, vbottom, true);
		g_wndEntity->SetPosition(hleft, vtop, h1, vbottom, false);
		g_wndTexture->SetPosition(hleft, vtop, h1, vbottom, false);

		g_wndCamera->SetPosition(h2, vtop, h3, v1, true);
		g_wndGrid[0]->SetPosition(h1, vtop, h2, v1, true);
		g_wndGrid[1]->SetPosition(h1, v1, h2, vbottom, true);
		g_wndGrid[2]->SetPosition(h2, v1, h3, vbottom, true);

		g_wndZ->SetPosition(h3, vtop, hright, vbottom, true);
		break;
	case 8:	// 4 Window Reverse w/o Z (Cam Left)
		h1 = 320;
		h3 = hright - 64;
		h2 = (h1 + hright) / 2;
		g_wndConsole->SetPosition(hleft, vtop, h1, vbottom, true);
		g_wndEntity->SetPosition(hleft, vtop, h1, vbottom, false);
		g_wndTexture->SetPosition(hleft, vtop, h1, vbottom, false);

		g_wndCamera->SetPosition(h1, vtop, h2, v1, true);
		g_wndGrid[0]->SetPosition(h2, vtop, hright, v1, true);
		g_wndGrid[1]->SetPosition(h1, v1, h2, vbottom, true);
		g_wndGrid[2]->SetPosition(h2, v1, hright, vbottom, true);

		g_wndZ->SetPosition(h3, vtop, hright, vbottom, false);
		break;
	case 9:	// 4 Window Reverse w/o Z (Cam Right)
		h1 = 320;
		h3 = hright - 64;
		h2 = (h1 + hright) / 2;
		g_wndConsole->SetPosition(hleft, vtop, h1, vbottom, true);
		g_wndEntity->SetPosition(hleft, vtop, h1, vbottom, false);
		g_wndTexture->SetPosition(hleft, vtop, h1, vbottom, false);

		g_wndCamera->SetPosition(h2, vtop, hright, v1, true);
		g_wndGrid[0]->SetPosition(h1, vtop, h2, v1, true);
		g_wndGrid[1]->SetPosition(h1, v1, h2, vbottom, true);
		g_wndGrid[2]->SetPosition(h2, v1, hright, vbottom, true);

		g_wndZ->SetPosition(h3, vtop, hright, vbottom, false);
		break;
	}
	//WndMain_UpdateWindows(W_ALL);
}

/*
==============
WndMain_SaveWindowStates
==============
*/
bool WndMain_SaveWindowStates()
{
	WndMain_SyncInspectorPlacement();
	for (auto wvIt = Window::windex.begin(); wvIt != Window::windex.end(); ++wvIt)
		(*wvIt)->SavePosition();
	WndMain_SaveRebarPositions();
	// TODO: save status bar visibility

	RECT rc;
	GetWindowRect(g_hwndMain, &rc);
	return Sys_RegistrySaveInfo("MainWindow", &rc, sizeof(rc));
}

/*
==============
WndMain_LoadStateForWindow
==============
*/
bool WndMain_LoadStateForWindow(HWND hWnd, const char *pszName)
{
	RECT rc;
	LONG lSize = sizeof(rc);

	if (Sys_RegistryLoadInfo(pszName, &rc, &lSize))
	{
		if (rc.left < 0)
			rc.left = 0;
		if (rc.top < 0)
			rc.top = 0;
		if (rc.right < rc.left + 16)
			rc.right = rc.left + 16;
		if (rc.bottom < rc.top + 16)
			rc.bottom = rc.top + 16;

		MoveWindow(hWnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, FALSE);
		return true;
	}

	return false;
}


/*
============
WndMain_WindowForPoint
============
*/
HWND WndMain_WindowForPoint(POINT point)
{
	HWND hwnd;
	ScreenToClient(g_hwndMain, &point);
	hwnd = ChildWindowFromPoint(g_hwndMain, point);

	return hwnd;
}


/*
==================
WndMain_UpdateWindows
==================
*/
void WndMain_UpdateWindows(int bits)
{
	g_nUpdateBits |= bits;
}

/*
==================
WndMain_ForceUpdateWindows
==================
*/
void WndMain_ForceUpdateWindows(int bits)
{
	if (!bits)
		bits |= g_nUpdateBits;

	// redo target lines first so they're available to draw right away
	if (g_nUpdateBits & W_TARGETGRAPH)
		g_map.targetGraph.Refresh(g_map.entities);

	// ensure the layout is rebuilt if necessary even if the texture window isn't on top yet
	if (g_nUpdateBits & W_TEXTURE)
		g_vTexture.Arrange();

	// update any windows now
	for (auto wIt = Window::windex.begin(); wIt != Window::windex.end(); ++wIt)
	{
		if ((*wIt)->vbits & g_nUpdateBits)
			(*wIt)->ForceUpdate();
	}

	if (g_nUpdateBits & W_TITLE)
		WndMain_UpdateTitle();
	if (g_nUpdateBits & W_SURF)
		WndSurf_UpdateUI();
	g_nUpdateBits = 0;
}

/*
==================
WndMain_CreateViews
==================
*/
void WndMain_CreateViews()
{
	Sys_Printf("Creating windows\n");
	g_wndConsole = new WndConsole();
	g_wndConsole->Initialize();

	g_wndCamera = new WndCamera();
	g_wndCamera->Initialize();
	for (int i = 0; i < 3; i++)
	{
		g_wndGrid[i] = new WndGrid();
		g_wndGrid[i]->Initialize(i);
	}
	g_wndZ = new WndZChecker();
	g_wndZ->Initialize();

	g_wndTexture = new WndTexture();
	g_wndTexture->Initialize();
	g_wndEntity = new WndEntity();
	g_wndEntity->Initialize();

	/*
	// this could happen just fine in window create, but delaying wglShareLists fixes
	// troubles in some contexts (namely the GLX emulation in WINE)
	// TODO: write new fantasy renderer that doesn't rely on crufty old stuff like wglShareLists
	g_wndCamera->ShareLists();
	for (int i = 0; i < 3; i++)
		g_wndGrid[i]->ShareLists();
	g_wndZ->ShareLists();
	g_wndTexture->ShareLists();
	*/
	WndMain_ForceInspectorMode(W_CONSOLE);
}





/*
=============================================================

MAIN WINDOW

=============================================================
*/

/*
==================
WndMain_UpdateTitle
==================
*/
void WndMain_UpdateTitle()
{
	char tmp[512];

	sprintf_s(tmp, "%s - [%s%s]", 
		g_qeAppName, 
		g_map.hasFilename ? g_map.name : "unnamed",
		g_cmdQueue.IsModified() ? " *" : "");

	SetWindowText(g_hwndMain, tmp);
}

/*
============
WMain_WndProc
============
*/
LONG WINAPI WMain_WndProc(
	HWND	hWnd,	// handle to window
	UINT	uMsg,	// message
	WPARAM	wParam,	// first message parameter
	LPARAM	lParam 	// second message parameter
)
{
	RECT	rect;
	LPTOOLTIPTEXT	lpToolTipText;
	char	szToolTip[128];
	time_t	lTime;
	//int		i, nBandIndex;	// sikk - Save Rebar Band Info

	GetClientRect(hWnd, &rect);

	switch (uMsg)
	{
	case WM_TIMER:	// 1/sec
		QE_CheckAutoSave();
		return 0;

	case WM_DESTROY:
		WndMain_DestroyMRU();
		g_qeconfig.Save();
		PostQuitMessage(0);
		KillTimer(hWnd, QE_TIMERAUTOSAVE);
		return 0;

	case WM_CREATE:
		g_hwndMain = hWnd;
		WndMain_CreateMRU();
		return 0;

	case WM_SIZE:
		// resize the status window
		MoveWindow(g_hwndStatus, -100, 100, 10, 10, true);
		return 0;

	case WM_CLOSE:
		// call destroy window to cleanup and go away
		if (!ConfirmModified())
			return TRUE;

		WndMain_SaveWindowStates();

		time(&lTime);	// sikk - Print current time for logging purposes
		Sys_Printf("\nSession Stopped: %s", ctime(&lTime));

		// TODO:
		// at program quit, the static slab lists are emptied before the global command queue
		// object is destroyed, so commands can't clean up any of their data
		g_cmdQueue.Clear();		// so we're doing this for now, until I come back and 
								// figure out why, ie forever

		DestroyWindow(hWnd);
		return 0;

	case WM_INITMENUPOPUP:
		WndMain_UpdateMenu();	// sikk - Update Menu & Toolbar Items
		return 0;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code)
		{
			// sikk---> Tooltip 
		case TTN_NEEDTEXT:
			// Display tool tip text.
			lpToolTipText = (LPTOOLTIPTEXT)lParam;
			LoadString(g_qeglobals.d_hInstance,
				lpToolTipText->hdr.idFrom,	// string ID == cmd ID
				szToolTip,
				sizeof(szToolTip));
			lpToolTipText->lpszText = szToolTip;
			break;
			// <---sikk

		case RBN_BEGINDRAG:	// sikk - Rebar (to handle movement)
			break;
		case RBN_ENDDRAG:	// sikk - Rebar (to handle movement)
			break;

		case TBN_HOTITEMCHANGE:	// sikk - For Toolbar Button Highlighting
			break;

		default:
			return TRUE;
			break;
		}
		return 0;

	case WM_KEYDOWN:
		if (Tool::HandleInput(uMsg, wParam, lParam))
			return 0;
		return QE_KeyDown(wParam);
		/*case WM_KEYUP:
			if (Tool::HandleInput(uMsg, wParam, lParam))
				return 0;
			return QE_KeyUp(wParam);
		*/
	case WM_MOUSEMOVE:
		if (Tool::HandleInput(uMsg, wParam, lParam))
			return 0;
		return 1;
	case WM_COMMAND:
		if (Tool::HandleInput(uMsg, wParam, lParam))
			return 0;
		return WndMain_Command(hWnd, wParam, lParam);
	default:
		break;
	}
	/*
	// lunaran: give tools a last chance to interpret key/mouse/menu inputs
	if (Tool::FilterInput(uMsg))
	{
		if (Tool::HandleInput(uMsg, wParam, lParam))
			return FALSE;

		if (uMsg == WM_KEYDOWN)
			return QE_KeyDown(wParam);
		//else if (uMsg == WM_KEYUP)
		//	return QE_KeyUp(wParam);
		else if (uMsg == WM_COMMAND)
			return WndMain_Command(hWnd, wParam, lParam);
	}
	*/
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


/*
==============
WndMain_Create
==============
*/
void WndMain_Create()
{
	WNDCLASS	wc;
	//int			i;
	HMENU		hMenu;
	HINSTANCE hInstance = g_qeglobals.d_hInstance;

	/* Register the main class */
	memset(&wc, 0, sizeof(wc));

	wc.style = 0;
	wc.lpfnWndProc = (WNDPROC)WMain_WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));	// sikk - MainFrame Icon (pretty)
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE + 1);
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	wc.lpszClassName = MAIN_WINDOW_CLASS;

	if (!RegisterClass(&wc))
		Error("Main_Register: Failed.");

	g_nScreenWidth = GetSystemMetrics(SM_CXFULLSCREEN);
	g_nScreenHeight = GetSystemMetrics(SM_CYFULLSCREEN);

	g_hwndMain = CreateWindow(MAIN_WINDOW_CLASS,		// registered class name
		"QuakeEd 3",												// window name
		QE3_MAIN_STYLE,												// window style
		0, 0,														// position of window
		g_nScreenWidth, g_nScreenHeight + GetSystemMetrics(SM_CYSIZE),	// window size
		0,															// parent or owner window
		0,															// menu or child-window identifier
		hInstance,													// application instance
		NULL);														// window-creation data

	if (!g_hwndMain)
		Error("Could not create Main Window.");

	// autosave timer
	SetTimer(g_hwndMain, QE_TIMERAUTOSAVE, 1000, NULL);
	WndMain_UpdateTitle();

#ifdef _DEBUG
	sprintf(g_qeAppName, "QuakeEd 3.%i DEBUG (build %i)", QE_VERSION_MINOR, QE_VERSION_BUILD);
#else
	sprintf(g_qeAppName, "QuakeEd 3.%i (build %i)", QE_VERSION_MINOR, QE_VERSION_BUILD);
#endif

	WndMain_LoadStateForWindow(g_hwndMain, "MainWindow");

	WndMain_CreateReBar(g_hwndMain, hInstance);
	WndMain_CreateStatusBar(g_hwndMain);

	if ((hMenu = GetMenu(g_hwndMain)) != 0)
	{
		WndMain_UpdateMenuFilters(hMenu);
		// by default all of these are checked because that's how they're defined in the menu editor

		if (g_cfgEditor.CubicClip)
		{
			CheckMenuItem(hMenu, ID_VIEW_CUBICCLIP, MF_CHECKED);
			SendMessage(g_hwndToolbar[7], TB_CHECKBUTTON, (WPARAM)ID_VIEW_CUBICCLIP,
				(g_cfgEditor.CubicClip ? (LPARAM)true : (LPARAM)false));
		}
	}

	ShowWindow(g_hwndMain, SW_SHOWMAXIMIZED);	// sikk - changed from "SW_SHOWDEFAULT" (personal preference)

	WndMain_CreateViews();
}
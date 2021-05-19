//==============================
//	win_yz.c
//==============================

// windows specific yz view code

#include "qe3.h"

static HDC   s_hdcYZ;
static HGLRC s_hglrcYZ;

static unsigned s_stippleYZ[32] =
{
	0xaaaaaaaa, 0x55555555,0xaaaaaaaa, 0x55555555,
	0xaaaaaaaa, 0x55555555,0xaaaaaaaa, 0x55555555,
	0xaaaaaaaa, 0x55555555,0xaaaaaaaa, 0x55555555,
	0xaaaaaaaa, 0x55555555,0xaaaaaaaa, 0x55555555,
	0xaaaaaaaa, 0x55555555,0xaaaaaaaa, 0x55555555,
	0xaaaaaaaa, 0x55555555,0xaaaaaaaa, 0x55555555,
	0xaaaaaaaa, 0x55555555,0xaaaaaaaa, 0x55555555,
	0xaaaaaaaa, 0x55555555,0xaaaaaaaa, 0x55555555
};

// sikk---> Context Menu
#define SELECTION_POINTENT	1
#define SELECTION_BRUSHES	2
#define SELECTION_BRUSHENT	4

int		g_nMouseX, g_nMouseY;
bool	g_bMoved;
vec3_t	g_v3Origin;


/*
============
DoYZPopupMenu
============
*/
void DoYZPopupMenu (int x, int y)
{
	HMENU	hMenu;
	POINT	point;
	int		retval;
	int		selected = GetSelectionInfo();

	hMenu = GetSubMenu(LoadMenu(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDR_CONTEXT)), 0);

	GetCursorPos(&point);

	switch (selected)
	{
	case SELECTION_POINTENT:
		EnableMenuItem(hMenu, ID_SELECTION_CSGHOLLOW, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_CSGSUBTRACT, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_CSGMERGE, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_SELECTCOMPLETETALL, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_SELECTTOUCHING, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_SELECTPARTIALTALL, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_SELECTINSIDE, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REGION_SETBRUSH, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REGION_SETTALLBRUSH, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REGION_SETSELECTION, MF_GRAYED);
		EnableMenuItem(hMenu, ID_MENU_CREATEANYENTITY, MF_GRAYED);
		EnableMenuItem(hMenu, ID_MENU_CREATEBRUSHENTITY, MF_GRAYED);
		EnableMenuItem(hMenu, ID_MENU_CREATEPOINTENTITY, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_UNGROUPENTITY, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_INSERTBRUSH, MF_GRAYED);
		break;
	case SELECTION_BRUSHENT:
		EnableMenuItem(hMenu, ID_MENU_CREATEANYENTITY, MF_GRAYED);
		EnableMenuItem(hMenu, ID_MENU_CREATEBRUSHENTITY, MF_GRAYED);
		EnableMenuItem(hMenu, ID_MENU_CREATEPOINTENTITY, MF_GRAYED);
		EnableMenuItem(hMenu, ID_MISC_SELECTENTITYCOLOR, MF_GRAYED);
		break;
	case SELECTION_BRUSHES:
		EnableMenuItem(hMenu, ID_SELECTION_CONNECT, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_UNGROUPENTITY, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_INSERTBRUSH, MF_GRAYED);
		EnableMenuItem(hMenu, ID_MISC_SELECTENTITYCOLOR, MF_GRAYED);
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
		EnableMenuItem(hMenu, ID_SELECTION_SELECTCOMPLETETALL, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_SELECTTOUCHING, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_SELECTPARTIALTALL, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_SELECTINSIDE, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_SELECTALLTYPE, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REGION_SETBRUSH, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REGION_SETTALLBRUSH, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REGION_SETSELECTION, MF_GRAYED);
		EnableMenuItem(hMenu, ID_MENU_CREATEANYENTITY, MF_GRAYED);
		EnableMenuItem(hMenu, ID_MENU_CREATEBRUSHENTITY, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_CONNECT, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_UNGROUPENTITY, MF_GRAYED);
		EnableMenuItem(hMenu, ID_SELECTION_INSERTBRUSH, MF_GRAYED);
		EnableMenuItem(hMenu, ID_MISC_SELECTENTITYCOLOR, MF_GRAYED);
		break;
	}

	EnableMenuItem(hMenu, ID_REGION_SETXZ, (g_qeglobals.d_savedinfo.bShow_XZ ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(hMenu, ID_REGION_SETYZ, (g_qeglobals.d_savedinfo.bShow_YZ ? MF_ENABLED : MF_GRAYED));
	
	retval = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON, point.x, point.y, 0, g_qeglobals.d_hwndYZ, NULL);

	switch (retval)
	{
	case ID_MENU_CREATEANYENTITY:
		DoCreateEntity(true, true, true, g_v3Origin);
		break;
	case ID_MENU_CREATEBRUSHENTITY:
		DoCreateEntity(false, true, true, g_v3Origin);
		break;
	case ID_MENU_CREATEPOINTENTITY:
		YZ_ToGridPoint(x, y, g_v3Origin);
		g_v3Origin[0] = g_qeglobals.d_v3WorkMax[0];
		DoCreateEntity(true, false, false, g_v3Origin);
		break;
	default:
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, retval, 0);
	}

	DestroyMenu(hMenu);
}
// <---sikk

/*
============
WYZ_WndProc
============
*/
LONG WINAPI WYZ_WndProc (
    HWND	hWnd,	// handle to window
    UINT	uMsg,	// message
    WPARAM	wParam,	// first message parameter
    LPARAM	lParam 	// second message parameter
	)
{
	int		fwKeys, xPos, yPos;
    RECT	rect;
    PAINTSTRUCT	ps;

    GetClientRect(hWnd, &rect);

    switch (uMsg)
    {
	case WM_CREATE:
		s_hdcYZ = GetDC(hWnd);

		QEW_SetupPixelFormat(s_hdcYZ, false);

		if ((s_hglrcYZ = wglCreateContext(s_hdcYZ)) == 0)
			Error("WYZ_WndProc: wglCreateContext failed.");
		if (!wglMakeCurrent(s_hdcYZ, s_hglrcYZ))
			Error("WYZ_WndProc: wglMakeCurrent failed.");
		if (!wglShareLists(g_qeglobals.d_hglrcBase, s_hglrcYZ))
			Error("WYZ_WndProc: wglShareLists failed.");

		glPolygonStipple((char *)s_stippleYZ);
		glLineStipple(3, 0x5555);
		return 0;

	case WM_DESTROY:
		QEW_StopGL(hWnd, s_hglrcYZ, s_hdcYZ);
		return 0;

	case WM_PAINT:
	    BeginPaint(hWnd, &ps);

		if (!wglMakeCurrent(s_hdcYZ, s_hglrcYZ))
			Error("wglMakeCurrent: Failed.");

		QE_CheckOpenGLForErrors();
		YZ_Draw();
		QE_CheckOpenGLForErrors();

		SwapBuffers(s_hdcYZ);

		EndPaint(hWnd, &ps);
		return 0;

	case WM_KEYDOWN:
		return QE_KeyDown(wParam);
		
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
		if (GetTopWindow(g_qeglobals.d_hwndMain) != hWnd)
			BringWindowToTop(hWnd);
		SetFocus(g_qeglobals.d_hwndYZ);
		SetCapture(g_qeglobals.d_hwndYZ);
		fwKeys = wParam;	// key flags 
		xPos = (short)LOWORD(lParam);	// horizontal position of cursor 
		yPos = (short)HIWORD(lParam);	// vertical position of cursor 
		yPos = (int)rect.bottom - 1 - yPos;
		YZ_MouseDown(xPos, yPos, fwKeys);
// sikk---> Context Menu
		if(uMsg == WM_RBUTTONDOWN)
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
		fwKeys = wParam;	// key flags 
		xPos = (short)LOWORD(lParam);	// horizontal position of cursor 
		yPos = (short)HIWORD(lParam);	// vertical position of cursor 
		yPos = (int)rect.bottom - 1 - yPos;
		YZ_MouseUp(xPos, yPos, fwKeys);
		if (!(fwKeys & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
			ReleaseCapture();
// sikk---> Context Menu
		if (uMsg == WM_RBUTTONUP && !g_bMoved)
			DoYZPopupMenu(xPos, yPos);
// <---sikk
		return 0;

// sikk---> Minimum Window Size
	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
			lpmmi->ptMinTrackSize.x = 96;
		}
		return 0;
// <---sikk

	case WM_MOUSEMOVE:
		fwKeys = wParam;	// key flags 
		xPos = (short)LOWORD(lParam);	// horizontal position of cursor 
		yPos = (short)HIWORD(lParam);	// vertical position of cursor 
		yPos = (int)rect.bottom - 1 - yPos;
		YZ_MouseMoved(xPos, yPos, fwKeys);
// sikk---> Context Menu
		if (!g_bMoved && (g_nMouseX != xPos || g_nMouseY != yPos))
			g_bMoved = true;
// <---sikk
		return 0;

	case WM_SIZE:
		g_qeglobals.d_yz.width = rect.right;
		g_qeglobals.d_yz.height = rect.bottom;
		InvalidateRect(g_qeglobals.d_hwndYZ, NULL, FALSE);
		return 0;
// sikk---> Window Management
	case WM_SIZING:
		if (TryDocking(hWnd, wParam, (LPRECT)lParam, 0))
			return TRUE;
		break;

	case WM_MOVING:
		if (TryDocking(hWnd, wParam, (LPRECT)lParam, 0))
			return TRUE;
		break;
// <---sikk
	case WM_NCCALCSIZE:	// don't let windows copy pixels
		DefWindowProc(hWnd, uMsg, wParam, lParam);
		return WVR_REDRAW;

	case WM_KILLFOCUS:
	case WM_SETFOCUS:
		SendMessage(hWnd, WM_NCACTIVATE, uMsg == WM_SETFOCUS, 0);
		return 0;

   	case WM_CLOSE:
        DestroyWindow(hWnd);
		return 0;
    }

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*
==============
WYZ_Create
==============
*/
void WYZ_Create (HINSTANCE hInstance)
{
    WNDCLASS   wc;

    /* Register the xz class */
	memset(&wc, 0, sizeof(wc));

    wc.style         = CS_OWNDC;
    wc.lpfnWndProc   = (WNDPROC)WYZ_WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = 0;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = YZ_WINDOW_CLASS;

    if (!RegisterClass(&wc))
        Error("WYZ_Register: Failed.");

// sikk---> changed window to "tool window"
	g_qeglobals.d_hwndYZ = CreateWindowEx(
		WS_EX_TOOLWINDOW,						// extended window style
		YZ_WINDOW_CLASS,						// registered class name
		"YZ View: Front",						// window name
		QE3_CHILD_STYLE,						// window style
		g_nScreenWidth - 384, 384,				// position of window
		384, 256,								// window size
		g_qeglobals.d_hwndMain,					// parent or owner window
		0,										// menu or child-window identifier
		hInstance,								// application instance
		NULL);									// window-creation data
// <---sikk
	if (!g_qeglobals.d_hwndYZ)
		Error("Could not create YZ Window.");

	LoadWindowState(g_qeglobals.d_hwndYZ, "YZWindow");
    ShowWindow(g_qeglobals.d_hwndYZ, SW_SHOWDEFAULT);
}

/*	// sikk - Unused
==================
WYZ_InitPixelFormat
==================
*//*
static void WYZ_InitPixelFormat (PIXELFORMATDESCRIPTOR *pPFD)
{
	memset(pPFD, 0, sizeof(*pPFD));

	pPFD->nSize			= sizeof(PIXELFORMATDESCRIPTOR);
	pPFD->nVersion		= 1;
	pPFD->dwFlags		= PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
	pPFD->iPixelType	= PFD_TYPE_RGBA;
	pPFD->cColorBits	= 24;
	pPFD->cDepthBits	= 32;
	pPFD->iLayerType	= PFD_MAIN_PLANE;
}
*/

/*
==================
WYZ_Print
==================
*/
void WYZ_Print ()
{
	PRINTDLG pd;
	DOCINFO	di;
	RECT	r;
	int		bmwidth = 320, bmheight = 320;
	int		pwidth, pheight;

	// initialize the PRINTDLG struct and execute it
	memset(&pd, 0, sizeof(pd));
	pd.lStructSize = sizeof(pd);
	pd.hwndOwner = g_qeglobals.d_hwndYZ;
	pd.Flags = PD_RETURNDC;
	pd.hInstance = 0;
	if (!PrintDlg(&pd) || !pd.hDC)
	{
		MessageBox(g_qeglobals.d_hwndMain, "Could not PrintDlg()", "QuakeEd 3: Print Error", MB_OK | MB_ICONERROR);
		return;
	}

	// StartDoc
	memset(&di, 0, sizeof(di));
	di.cbSize = sizeof(di);
	di.lpszDocName = "QE3";
	if (StartDoc(pd.hDC, &di) <= 0)
	{
		MessageBox(g_qeglobals.d_hwndMain, "Could not StartDoc()", "QuakeEd 3: Print Error", MB_OK | MB_ICONERROR);
		return;
	}

	// StartPage
	if (StartPage(pd.hDC) <= 0)
	{
		MessageBox(g_qeglobals.d_hwndMain, "Could not StartPage()", "QuakeEd 3: Print Error", MB_OK | MB_ICONERROR);
		return;
	}

	// read pixels from the YZ window
	GetWindowRect(g_qeglobals.d_hwndYZ, &r);

	bmwidth  = r.right - r.left;
	bmheight = r.bottom - r.top;

	pwidth  = GetDeviceCaps(pd.hDC, PHYSICALWIDTH) - GetDeviceCaps(pd.hDC, PHYSICALOFFSETX);
	pheight = GetDeviceCaps(pd.hDC, PHYSICALHEIGHT) - GetDeviceCaps(pd.hDC, PHYSICALOFFSETY);

	StretchBlt(pd.hDC,		// handle to destination device context
		0, 0,				// x, y coordinate of upper-left corner of destination rectangle
		pwidth, pheight,	// width, height of destination rectangle
		s_hdcYZ,			// handle to source device context
		0, 0,				// x, y coordinate of upper-left corner of source rectangle
		bmwidth, bmheight,	// width, height of source rectangle
		SRCCOPY);			// raster operation code

	// EndPage and EndDoc
	if (EndPage(pd.hDC) <= 0)
	{
		MessageBox(g_qeglobals.d_hwndMain, "Could not EndPage()", "QuakeEd 3: Print Error", MB_OK | MB_ICONERROR);
		return;
	}

	if (EndDoc(pd.hDC) <= 0)
	{
		MessageBox(g_qeglobals.d_hwndMain, "Could not EndDoc()", "QuakeEd 3: Print Error", MB_OK | MB_ICONERROR);
		return;
	}
}

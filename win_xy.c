//==============================
//	win_xy.c
//==============================

// windows specific xy view code

#include "qe3.h"

static HDC   s_hdcXY;
static HGLRC s_hglrcXY;

static unsigned s_stipple[32] =
{
	0xaaaaaaaa, 0x55555555,0xaaaaaaaa, 0x55555555,
	0xaaaaaaaa, 0x55555555,0xaaaaaaaa, 0x55555555,
	0xaaaaaaaa, 0x55555555,0xaaaaaaaa, 0x55555555,
	0xaaaaaaaa, 0x55555555,0xaaaaaaaa, 0x55555555,
	0xaaaaaaaa, 0x55555555,0xaaaaaaaa, 0x55555555,
	0xaaaaaaaa, 0x55555555,0xaaaaaaaa, 0x55555555,
	0xaaaaaaaa, 0x55555555,0xaaaaaaaa, 0x55555555,
	0xaaaaaaaa, 0x55555555,0xaaaaaaaa, 0x55555555,
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
GetSelectionInfo
============
*/
int GetSelectionInfo ()
{
	int		 retval = 0;
	brush_t	*b;

	for(b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		if(b->owner->eclass->fixedsize)
			retval = SELECTION_POINTENT;
		else if(!stricmp(b->owner->eclass->name, "worldspawn"))
			retval = SELECTION_BRUSHES;
		else
			retval = SELECTION_BRUSHENT;
	}

	return retval;
}

/*
============
DoXYPopupMenu
============
*/
void DoXYPopupMenu (int x, int y)
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
		
	retval = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON, point.x, point.y, 0, g_qeglobals.d_hwndXY, NULL);

	switch (retval)
	{
	case ID_MENU_CREATEANYENTITY:
		DoCreateEntity(true, true, true, g_v3Origin);
		break;
	case ID_MENU_CREATEBRUSHENTITY:
		DoCreateEntity(false, true, true, g_v3Origin);
		break;
	case ID_MENU_CREATEPOINTENTITY:
		XY_ToGridPoint(x, y, g_v3Origin);
		if (g_qeglobals.d_nViewType == XY)
			g_v3Origin[2] = g_qeglobals.d_v3WorkMax[2];
		else if (g_qeglobals.d_nViewType == YZ)
			g_v3Origin[0] = g_qeglobals.d_v3WorkMax[0];
		else
			g_v3Origin[1] = g_qeglobals.d_v3WorkMax[1];

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
WXY_WndProc
============
*/
LONG WINAPI WXY_WndProc (
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
		s_hdcXY = GetDC(hWnd);

		QEW_SetupPixelFormat(s_hdcXY, false);

		if ((s_hglrcXY = wglCreateContext(s_hdcXY)) == 0)
			Error("WXY_WndProc: wglCreateContext failed.");

		if (!wglMakeCurrent(s_hdcXY, s_hglrcXY))
			Error("WXY_WndProc: wglMakeCurrent failed.");

		if (!wglShareLists(g_qeglobals.d_hglrcBase, s_hglrcXY))
			Error("WXY_WndProc: wglShareLists failed.");

		glPolygonStipple((char *)s_stipple);
		glLineStipple(3, 0xaaaa);
		return 0;

	case WM_DESTROY:
		QEW_StopGL(hWnd, s_hglrcXY, s_hdcXY);
		return 0;

	case WM_PAINT:
	    BeginPaint(hWnd, &ps);

		if (!wglMakeCurrent(s_hdcXY, s_hglrcXY))
			Error("wglMakeCurrent: Failed.");

		QE_CheckOpenGLForErrors();
		XY_Draw();
		QE_CheckOpenGLForErrors();

		SwapBuffers(s_hdcXY);

		EndPaint(hWnd, &ps);
		return 0;

	case WM_KEYDOWN:
		return QE_KeyDown(wParam);
		
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
		if (GetTopWindow(g_qeglobals.d_hwndMain) != hWnd)
			BringWindowToTop(hWnd);
		SetFocus(g_qeglobals.d_hwndXY);
		SetCapture(g_qeglobals.d_hwndXY);
		fwKeys = wParam;        // key flags 
		xPos = (short)LOWORD(lParam);  // horizontal position of cursor 
		yPos = (short)HIWORD(lParam);  // vertical position of cursor 
		yPos = (int)rect.bottom - 1 - yPos;
		XY_MouseDown(xPos, yPos, fwKeys);
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
		fwKeys = wParam;        // key flags 
		xPos = (short)LOWORD(lParam);  // horizontal position of cursor 
		yPos = (short)HIWORD(lParam);  // vertical position of cursor 
		yPos = (int)rect.bottom - 1 - yPos;
		XY_MouseUp(xPos, yPos, fwKeys);
		if (!(fwKeys & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
			ReleaseCapture();
// sikk---> Context Menu
		if (uMsg == WM_RBUTTONUP && !g_bMoved)
			DoXYPopupMenu(xPos, yPos);
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
		fwKeys = wParam;        // key flags 
		xPos = (short)LOWORD(lParam);  // horizontal position of cursor 
		yPos = (short)HIWORD(lParam);  // vertical position of cursor 
		yPos = (int)rect.bottom - 1 - yPos;
		XY_MouseMoved(xPos, yPos, fwKeys);
// sikk---> Context Menu
		if (!g_bMoved && (g_nMouseX != xPos || g_nMouseY != yPos))
			g_bMoved = true;
// <---sikk
		return 0;

	case WM_SIZE:
		g_qeglobals.d_xyz.width = rect.right;
		g_qeglobals.d_xyz.height = rect.bottom;
		InvalidateRect(g_qeglobals.d_hwndXY, NULL, FALSE);
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
	case WM_NCCALCSIZE:// don't let windows copy pixels
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
WXY_Create
==============
*/
void WXY_Create (HINSTANCE hInstance)
{
    WNDCLASS   wc;

    /* Register the xy class */
	memset(&wc, 0, sizeof(wc));

    wc.style         = CS_OWNDC;
    wc.lpfnWndProc   = (WNDPROC)WXY_WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = 0;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = XY_WINDOW_CLASS;

    if (!RegisterClass(&wc))
        Error("WXY_Register: Failed.");

// sikk---> changed window to "tool window"
	g_qeglobals.d_hwndXY = CreateWindowEx(
		WS_EX_TOOLWINDOW,					// extended window style
		XY_WINDOW_CLASS,					// registered class name
		"XY View: Top",						// window name
		QE3_CHILD_STYLE,					// window style
		ZWIN_WIDTH, 64,						// position of window
		384, 256,							// window size
		g_qeglobals.d_hwndMain,				// parent or owner window
		0,									// menu or child-window identifier
		hInstance,							// application instance
		NULL);								// window-creation data
// <---sikk
	if (!g_qeglobals.d_hwndXY )
		Error("Could not create XY Window.");

	LoadWindowState(g_qeglobals.d_hwndXY, "XYWindow");
    ShowWindow(g_qeglobals.d_hwndXY, SW_SHOWDEFAULT);
}

/*
==================
WXY_InitPixelFormat
==================
*//*
static void WXY_InitPixelFormat (PIXELFORMATDESCRIPTOR *pPFD)
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
WXY_Print
==================
*/
void WXY_Print ()
{
	PRINTDLG pd;
	DOCINFO	di;
	RECT	r;
	int		bmwidth = 320, bmheight = 320;
	int		pwidth, pheight;

	// initialize the PRINTDLG struct and execute it
	memset(&pd, 0, sizeof(pd));
	pd.lStructSize = sizeof(pd);
	pd.hwndOwner = g_qeglobals.d_hwndXY;
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

	// read pixels from the XY window
	GetWindowRect(g_qeglobals.d_hwndXY, &r);

	bmwidth  = r.right - r.left;
	bmheight = r.bottom - r.top;

	pwidth  = GetDeviceCaps(pd.hDC, PHYSICALWIDTH) - GetDeviceCaps(pd.hDC, PHYSICALOFFSETX);
	pheight = GetDeviceCaps(pd.hDC, PHYSICALHEIGHT) - GetDeviceCaps(pd.hDC, PHYSICALOFFSETY);

	StretchBlt(pd.hDC,		// handle to destination device context
		0, 0,				// x, y coordinate of upper-left corner of destination rectangle
		pwidth, pheight,	// width, height of destination rectangle
		s_hdcXY,			// handle to source device context
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

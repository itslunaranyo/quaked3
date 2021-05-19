//==============================
//	win_z.c
//==============================

// windows specific z view code

#include "qe3.h"


static HDC   s_hdcZ;
static HGLRC s_hglrcZ;

/*
============
WZ_WndProc
============
*/
LONG WINAPI WZ_WndProc (
    HWND	hWnd,	// handle to window
    UINT	uMsg,	// message
    WPARAM	wParam,	// first message parameter
    LPARAM	lParam 	// second message parameter
	)
{
	int		fwKeys, xPos, yPos;
    RECT	rect;

    GetClientRect(hWnd, &rect);

    switch (uMsg)
    {

	case WM_DESTROY:
		QEW_StopGL(hWnd, s_hglrcZ, s_hdcZ);
		return 0;

	case WM_CREATE:
        s_hdcZ = GetDC(hWnd);
	    QEW_SetupPixelFormat(s_hdcZ, false);
		if ((s_hglrcZ = wglCreateContext(s_hdcZ)) == 0)
			Error("WZ_WndProc: wglCreateContext failed.");

        if (!wglMakeCurrent(s_hdcZ, s_hglrcZ))
			Error("WZ_WndProc: wglMakeCurrent failed.");

		if (!wglShareLists(g_qeglobals.d_hglrcBase, s_hglrcZ))
			Error("WZ_WndProc: wglShareLists failed.");
		return 0;

	case WM_PAINT:
        { 
		    PAINTSTRUCT	ps;

		    BeginPaint(hWnd, &ps);

            if (!wglMakeCurrent(s_hdcZ, s_hglrcZ))
				Error("wglMakeCurrent: Failed.");
			QE_CheckOpenGLForErrors();

			g_qeglobals.d_z.Draw();
		    SwapBuffers(s_hdcZ);

			EndPaint(hWnd, &ps);
        }
		return 0;


	case WM_KEYDOWN:
		QE_KeyDown(wParam);
		return 0;

	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
		if (GetTopWindow(g_qeglobals.d_hwndMain) != hWnd)
			BringWindowToTop(hWnd);

		SetFocus(g_qeglobals.d_hwndZ);
		SetCapture(g_qeglobals.d_hwndZ);
		fwKeys = wParam;				// key flags 
		xPos = (short)LOWORD(lParam);	// horizontal position of cursor 
		yPos = (short)HIWORD(lParam);	// vertical position of cursor 
		yPos = (int)rect.bottom - 1 - yPos;
		g_qeglobals.d_z.MouseDown(xPos, yPos, fwKeys);
		return 0;

	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_LBUTTONUP:
		fwKeys = wParam;        // key flags 
		xPos = (short)LOWORD(lParam);  // horizontal position of cursor 
		yPos = (short)HIWORD(lParam);  // vertical position of cursor 
		yPos = (int)rect.bottom - 1 - yPos;
		g_qeglobals.d_z.MouseUp(xPos, yPos, fwKeys);
		if (!(fwKeys & (MK_LBUTTON|MK_RBUTTON|MK_MBUTTON)))
			ReleaseCapture();
		return 0;

	case WM_MOUSEMOVE:
		fwKeys = wParam;        // key flags 
		xPos = (short)LOWORD(lParam);  // horizontal position of cursor 
		yPos = (short)HIWORD(lParam);  // vertical position of cursor 
		yPos = (int)rect.bottom - 1 - yPos;
		g_qeglobals.d_z.MouseMoved(xPos, yPos, fwKeys);
		return 0;

	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
			lpmmi->ptMinTrackSize.x = ZWIN_WIDTH;
		}
		return 0;

	case WM_SIZE:
		g_qeglobals.d_z.width = rect.right;
		g_qeglobals.d_z.height = rect.bottom;
		InvalidateRect(g_qeglobals.d_hwndZ, NULL, FALSE);
		return 0;
// sikk---> Window Management
	case WM_SIZING:
		if (TryDocking(hWnd, wParam, (LPRECT)lParam, ZWIN_WIDTH))
			return TRUE;
		break;

	case WM_MOVING:
		if (TryDocking(hWnd, wParam, (LPRECT)lParam, ZWIN_WIDTH))
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
        /* call destroy window to cleanup and go away */
        DestroyWindow(hWnd);
		return 0;
    }
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*
==============
WZ_Create
==============
*/
void WZ_Create ()
{
    WNDCLASS   wc;

    /* Register the z class */
	memset(&wc, 0, sizeof(wc));
	HINSTANCE hInstance = g_qeglobals.d_hInstance;

    wc.style         = CS_OWNDC;
    wc.lpfnWndProc   = (WNDPROC)WZ_WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = 0;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = Z_WINDOW_CLASS;

    if (!RegisterClass(&wc))
        Error("WZ_Register: Failed.");

// sikk---> changed window to "tool window"
	g_qeglobals.d_hwndZ = CreateWindowEx(
		WS_EX_TOOLWINDOW,				// extended window style
		Z_WINDOW_CLASS ,				// registered class name
		"Z View",						// window name
		QE3_CHILD_STYLE,				// window style
		0, 64,							// window position 
		ZWIN_WIDTH, g_nScreenHeight - 128,// window size
		g_qeglobals.d_hwndMain,			// parent or owner window
		0,								// menu or child-window identifier
		hInstance,						// application instance
		NULL);							// window-creation data
// <---sikk
	if (!g_qeglobals.d_hwndZ)
		Error("Could not create Z Window.");

	LoadWindowState(g_qeglobals.d_hwndZ, "ZWindow");
    ShowWindow(g_qeglobals.d_hwndZ, SW_SHOWDEFAULT);
}

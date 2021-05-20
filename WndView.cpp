//==============================
//	WndView.c
//==============================

#include "qe3.h"

std::vector<WndView*> WndView::wndviews;

WndView*	WndView::wvbInitializing;
HDC			WndView::w_hdcBase;
HGLRC		WndView::w_hglrcBase;
WNDCLASS	WndView::w_wcView;

#define WND_RESIZE_SNAP 8
#define WND_MOVE_SNAP 2

/*
==================
WndView
==================
*/
WndView::WndView() :
	w_hwnd(nullptr), w_hdc(nullptr), w_hglrc(nullptr), 
	name(nullptr), minWidth(64), minHeight(32), vbits(0), instance(0)
{
	wndviews.push_back(this);
}

/*
==================
~WndView
==================
*/
WndView::~WndView()
{
	wndviews.erase(std::find(wndviews.begin(), wndviews.end(), this));
}


/*
========================================================================

	VISIBILITY AND STATE

========================================================================
*/

/*
==================
WndView::IsOnTop
==================
*/
bool WndView::IsOnTop()
{
	return (GetTopWindow(g_qeglobals.d_hwndMain) == w_hwnd);
}

/*
==================
WndView::SavePosition
==================
*/
void WndView::SavePosition()
{
	wndViewPosition_s pos;
	char lpszName[64];

	sprintf(lpszName, "%s%i", name, instance);
	pos.vis = IsWindowVisible(w_hwnd);
	GetWindowRect(w_hwnd, &pos.rc);
	MapWindowPoints(NULL, g_qeglobals.d_hwndMain, (POINT *)&pos.rc, 2);
	SaveRegistryInfo(lpszName, &pos, sizeof(pos));
}

/*
==================
WndView::RestorePosition
==================
*/
void WndView::RestorePosition()
{
	wndViewPosition_s pos;
	LONG lSize = sizeof(pos);
	char lpszName[64];

	sprintf(lpszName, "%s%i", name, instance);

	if (!LoadRegistryInfo(lpszName, &pos, &lSize))
	{
		SetPosition(32, 64, 320, 240, true);
		return;
	}

	if (pos.rc.left < 0)
		pos.rc.left = 0;
	if (pos.rc.top < 0)
		pos.rc.top = 0;
	if (pos.rc.right < pos.rc.left + minWidth)
		pos.rc.right = pos.rc.left + minWidth;
	if (pos.rc.bottom < pos.rc.top + minHeight)
		pos.rc.bottom = pos.rc.top + minHeight;

	SetPosition(pos.rc.left, pos.rc.top, pos.rc.right, pos.rc.bottom, (pos.vis > 0));
}

void WndView::SetPosition(int l, int t, int r, int b, bool show)
{
	MoveWindow(w_hwnd, l, t, r - l, b - t, FALSE);
	InvalidateRect(w_hwnd, NULL, FALSE);

	if (show)
		Show();
	else
		Hide();
}

/*
==================
WndView::Open
==================
*/
bool WndView::Open()
{
	return IsWindowVisible(w_hwnd) > 0;
}

/*
==================
WndView::Toggle
==================
*/
void WndView::Toggle()
{
	if (IsWindowVisible(w_hwnd))
	{
		if (!IsOnTop())
		{
			Focus();
		}
		else
			Hide();
	}
	else
	{
		Show();
		Focus();
	}
}

/*
==================
WndView::Show
==================
*/
void WndView::Show()
{
	ShowWindow(w_hwnd, SW_SHOW);
	BringToTop();
}

/*
==================
WndView::Hide
==================
*/
void WndView::Hide()
{
	ShowWindow(w_hwnd, SW_HIDE);
}

/*
==================
WndView::SetWindowRect
==================
*/
void WndView::SetWindowRect(RECT *rc)
{
	MoveWindow(w_hwnd, rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top, TRUE);
}

void WndView::SetWindowRect(int x, int y, int w, int h)
{
	MoveWindow(w_hwnd, x, y, w, h, TRUE);
}

/*
==================
WndView::Swap
==================
*/
void WndView::Swap(WndView &other)
{
	WINDOWPLACEMENT myWP, otherWP;
	BOOL meVis, otherVis;

	meVis = IsWindowVisible(w_hwnd);
	otherVis = IsWindowVisible(other.w_hwnd);

	memset(&myWP, 0, sizeof(myWP));
	memset(&otherWP, 0, sizeof(otherWP));
	myWP.length = otherWP.length = sizeof(otherWP);

	GetWindowPlacement(w_hwnd, &myWP);
	GetWindowPlacement(other.w_hwnd, &otherWP);

	SetWindowPlacement(w_hwnd, &otherWP);
	SetWindowPlacement(other.w_hwnd, &myWP);

	if (meVis) other.Show();
	else other.Hide();
	if (otherVis) Show();
	else Hide();
}

/*
==================
WndView::GetMsgXY
==================
*/
void WndView::GetMsgXY(LPARAM l, int &x, int &y)
{
	x = (short)LOWORD(l);	// horizontal position of cursor 
	y = (int)clientRect.bottom - 1 - (short)HIWORD(l);	// vertical position of cursor 
}


/*
========================================================================

	DOCKING

========================================================================
*/

/*
==================
WndView::FindClosestHorizontal
==================
*/
int WndView::FindClosestHorizontal(int h, LPRECT field)
{
	RECT wr;
	int best;

	if (field->bottom - h < h - field->top)
		best = field->bottom;
	else
		best = field->top;

	for (auto wvIt = wndviews.begin(); wvIt != wndviews.end(); ++wvIt)
	{
		if ((*wvIt) == this)
			continue;
		if (!IsWindowVisible((*wvIt)->w_hwnd))
			continue;

		GetWindowRect((*wvIt)->w_hwnd, &wr);
		if (abs(wr.top - h) < abs(best - h))
			best = wr.top;

		if (abs(wr.bottom - h) < abs(best - h))
			best = wr.bottom;
	}
	return best;
}

/*
==================
WndView::SnapToClosestHorizontal
==================
*/
bool WndView::SnapToClosestHorizontal(LPRECT win, LPRECT field, bool down)
{
	int best;
	int h = down ? win->bottom : win->top;

	best = FindClosestHorizontal(h, field);

	if (abs(best - h) < WND_RESIZE_SNAP)
	{
		if (down)
			win->bottom = best;
		else
			win->top = best;
		return true;
	}
	return false;
}

/*
==================
WndView::FindClosestVertical
==================
*/
int WndView::FindClosestVertical(int v, LPRECT field)
{
	RECT wr;
	int best;

	if (field->right - v < v - field->left)
		best = field->right;
	else
		best = field->left;

	for (auto wvIt = wndviews.begin(); wvIt != wndviews.end(); ++wvIt)
	{
		if ((*wvIt) == this)
			continue;
		if (!IsWindowVisible((*wvIt)->w_hwnd))
			continue;

		GetWindowRect((*wvIt)->w_hwnd, &wr);
		if (abs(wr.left - v) < abs(best - v))
			best = wr.left;

		if (abs(wr.right - v) < abs(best - v))
			best = wr.right;
	}
	return best;
}

/*
==================
WndView::SnapToClosestVertical
==================
*/
bool WndView::SnapToClosestVertical(LPRECT win, LPRECT field, bool right)
{
	int best;
	int v = right ? win->right : win->left;

	best = FindClosestVertical(v, field);

	if (abs(best - v) < WND_RESIZE_SNAP)
	{
		if (right)
			win->right = best;
		else
			win->left = best;
		return true;
	}
	return false;
}

/*
==================
WndView::SnapMove
==================
*/
bool WndView::SnapMove(LPRECT win, LPRECT field)
{
	int bestLo, bestHi, best, margin;
	bool moved = false;

	bestLo = FindClosestVertical(win->left, field);
	bestHi = FindClosestVertical(win->right, field);

	if (abs(bestLo - win->left) < abs(bestHi - win->right))
	{
		best = bestLo;
		margin = bestLo - win->left;
	}
	else
	{
		best = bestHi;
		margin = bestHi - win->right;
	}

	if (abs(margin) < WND_MOVE_SNAP)
	{
		win->left += margin;
		win->right += margin;
		moved = true;
	}

	bestLo = FindClosestHorizontal(win->top, field);
	bestHi = FindClosestHorizontal(win->bottom, field);
	if (abs(bestLo - win->top) < abs(bestHi - win->bottom))
	{
		best = bestLo;
		margin = bestLo - win->top;
	}
	else
	{
		best = bestHi;
		margin = bestHi - win->bottom;
	}

	if (abs(margin) < WND_MOVE_SNAP)
	{
		win->top += margin;
		win->bottom += margin;
		moved = true;
	}

	return moved;
}

/*
==================
WndView::TryDocking
==================
*/
bool WndView::TryDocking(long side, LPRECT rect)
{
	RECT	field, statusrect, toolbarrect;
	GetWindowRect(g_qeglobals.d_hwndRebar, &toolbarrect);
	GetWindowRect(g_qeglobals.d_hwndStatus, &statusrect);

	// FIXME: assumes toolbar is at the top of the screen
	GetClientRect(g_qeglobals.d_hwndMain, &field);
	field.top = toolbarrect.bottom;
	field.bottom = statusrect.top;

	switch (side)
	{
	case 0:
		if (SnapMove(rect, &field))
			return true;
		break;
	case WMSZ_LEFT:
		if (SnapToClosestVertical(rect, &field, false))
			return true;
		break;
	case WMSZ_RIGHT:
		if (SnapToClosestVertical(rect, &field, true))
			return true;
		break;
	case WMSZ_TOP:
		if (SnapToClosestHorizontal(rect, &field, false))
			return true;
		break;
	case WMSZ_BOTTOM:
		if (SnapToClosestHorizontal(rect, &field, true))
			return true;
		break;
	case WMSZ_TOPLEFT:		// the + operators here avoid short circuiting out of one of the tests
		if (SnapToClosestHorizontal(rect, &field, false) + SnapToClosestVertical(rect, &field, false))
			return true;
		break;
	case WMSZ_TOPRIGHT:
		if (SnapToClosestHorizontal(rect, &field, false) + SnapToClosestVertical(rect, &field, true))
			return true;
		break;
	case WMSZ_BOTTOMLEFT:
		if (SnapToClosestHorizontal(rect, &field, true) + SnapToClosestVertical(rect, &field, false))
			return true;
		break;
	case WMSZ_BOTTOMRIGHT:
		if (SnapToClosestHorizontal(rect, &field, true) + SnapToClosestVertical(rect, &field, true))
			return true;
		break;
	}
	return false;
}


//========================================================================



/*
==================
WndView::ForceUpdate
==================
*/
void WndView::ForceUpdate()
{
	InvalidateRect(w_hwnd, &clientRect, FALSE);
	//UpdateWindow(w_hwnd);
}

/*
==================
WndView::SetTitle
==================
*/
void WndView::SetTitle(const char * title)
{
	SetWindowText(w_hwnd, title);
}

/*
==================
WndView::WndProc
==================
*/
LONG WINAPI WndView::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (WndView::wvbInitializing)
	{
		WndView::wvbInitializing->w_hwnd = hWnd;
		return WndView::wvbInitializing->WindowProcedure(uMsg, wParam, lParam);
	}

	for (auto wvIt = WndView::wndviews.begin(); wvIt != WndView::wndviews.end(); ++wvIt)
	{
		if ((*wvIt)->w_hwnd == hWnd)
			return (*wvIt)->WindowProcedure(uMsg, wParam, lParam);
	}

	Error("Couldn't find view window for hWnd");
	return 0;
}

/*
==================
WndView::WindowProcedure
==================
*/
int WndView::WindowProcedure(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		OnDestroy();
		ReleaseDC(w_hwnd, w_hdc);
		return 0;

	case WM_CREATE:
		w_hdc = GetDC(w_hwnd);
		OnCreate();
		return 0;

	case WM_PAINT:
		if (!IsWindowVisible(w_hwnd)) return 0;
		OnPaint();
		return 0;

	case WM_SIZE:
		GetClientRect(w_hwnd, &clientRect);
		v->Resize(clientRect.right, clientRect.bottom);
		InvalidateRect(w_hwnd, NULL, FALSE);
		return 0;

	// sikk---> Window Management
	case WM_SIZING:
		if (TryDocking(wParam, (LPRECT)lParam))
			return 1;
		break;

	case WM_MOVING:
		if (TryDocking(0, (LPRECT)lParam))
			return 1;
		break;
	// <---sikk

	case WM_NCCALCSIZE:// don't let windows copy pixels
		DefWindowProc(w_hwnd, uMsg, wParam, lParam);
		return WVR_REDRAW;

	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
			lpmmi->ptMinTrackSize.x = minWidth;
			lpmmi->ptMinTrackSize.y = minHeight;
		}
		return 0;

	case WM_KILLFOCUS:
	case WM_SETFOCUS:
		SendMessage(w_hwnd, WM_NCACTIVATE, uMsg == WM_SETFOCUS, 0);
		return 0;

	case WM_CLOSE:
		DestroyWindow(w_hwnd);
		return 0;

	case WM_NCLBUTTONDOWN:
	case WM_NCRBUTTONDOWN:
		Focus();
		break;
	}

	if (!OnMessage(uMsg, wParam, lParam))
		return 0;
	
	return DefWindowProc(w_hwnd, uMsg, wParam, lParam);
}

/*
==================
WndView::OnMessage
virtual
==================
*/
int WndView::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return 1;
}

void WndView::MoveRect(HWND hwnd, RECT r)
{
	SetWindowPos(hwnd, HWND_TOP, r.left, r.top, r.right, r.bottom, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOZORDER);
}

void WndView::MoveRect(HWND hwnd, int left, int top, int right, int bottom)
{
	SetWindowPos(hwnd, HWND_TOP, left, top, right, bottom, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOZORDER);
}


/*
==================
WndView::MakeFont
==================
*/
void WndView::MakeFont()
{
	HFONT	hfont;

	// create GL font
	hfont = CreateFont(
		10,	// logical height of font (default was 10)
		7,	// logical average character width (default was 7)
		0,	// angle of escapement 
		0,	// base-line orientation angle 
		1,	// font weight 
		0,	// italic attribute flag 
		0,	// underline attribute flag 
		0,	// strikeout attribute flag 
		1,	// character set identifier 
		0,	// output precision 
		0,	// clipping precision 
		0,	// output quality 
		0,	// pitch and family 
		0	// pointer to typeface name string (default was 0)
		);

	if (!hfont)
		Error("Could not create font.");

	SelectObject(w_hdcBase, hfont);

	if ((g_qeglobals.d_nFontList = glGenLists(256)) == 0)
		Error("Could not create font dlists.");

	// create the bitmap display lists
	// we're making images of glyphs 0 thru 255
	if (!wglUseFontBitmaps(w_hdcBase, 1, 255, g_qeglobals.d_nFontList))
		Error("WCam_WndProc: wglUseFontBitmaps failed.");

	// indicate start of glyph display lists
	glListBase(g_qeglobals.d_nFontList);
}

/*
==================
WndView::SetupPixelFormat
==================
*/
int WndView::SetupPixelFormat(HDC hDC, bool zbuffer)
{
	static PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),	// size of this pfd
		1,								// version number
		PFD_DRAW_TO_WINDOW |			// support window
		PFD_SUPPORT_OPENGL |			// support OpenGL
		PFD_DOUBLEBUFFER,				// double buffered
		PFD_TYPE_RGBA,					// RGBA type
		24,								// 24-bit color depth
		0, 0, 0, 0, 0, 0,				// color bits ignored
		0,								// no alpha buffer
		0,								// shift bit ignored
		0,								// no accumulation buffer
		0, 0, 0, 0,						// accum bits ignored
		32,							    // depth bits
		0,								// no stencil buffer
		0,								// no auxiliary buffer
		PFD_MAIN_PLANE,					// main layer
		0,								// reserved
		0, 0, 0							// layer masks ignored
	};
	int pixelformat = 0;

	zbuffer = true;
	if (!zbuffer)
		pfd.cDepthBits = 0;

	if ((pixelformat = ChoosePixelFormat(hDC, &pfd)) == 0)
	{
		printf("%d", GetLastError());
		Error("ChoosePixelFormat: Failed");
	}

	if (!SetPixelFormat(hDC, pixelformat, &pfd))
		Error("SetPixelFormat: Failed");

	return pixelformat;
}

/*
==================
WndView::OnCreate
==================
*/
void WndView::OnCreate()
{
	SetupPixelFormat(w_hdc, false);

	if ((w_hglrc = wglCreateContext(w_hdc)) == 0)
		Error("wglCreateContext failed.");

	if (!wglMakeCurrent(w_hdc, w_hglrc))
		Error("wglMakeCurrent failed.");

	// treat first created drawing context as base for sharing font display list
	if (!w_hdcBase || !w_hglrcBase)
	{
		w_hdcBase = w_hdc;
		w_hglrcBase = w_hglrc;

		MakeFont();
		Textures::SetTextureMode(g_cfgUI.TextureMode);

		// report OpenGL information
		Sys_Printf("GL_VENDOR: %s\n", glGetString(GL_VENDOR));
		Sys_Printf("GL_RENDERER: %s\n", glGetString(GL_RENDERER));
		Sys_Printf("GL_VERSION: %s\n", glGetString(GL_VERSION));
		Sys_Printf("GL_EXTENSIONS: %s\n", glGetString(GL_EXTENSIONS));

		//glPolygonStipple((GLubyte *)s_stipple);
		glLineStipple(3, 0xaaaa);
	}
	else if (!wglShareLists(w_hglrcBase, w_hglrc))
		Error("wglShareLists failed.");
}

/*
==================
WndView::OnDestroy
==================
*/
void WndView::OnDestroy()
{
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(w_hglrc);
}

/*
==================
WndView::OnPaint
==================
*/
void WndView::OnPaint()
{
	if (!wglMakeCurrent(w_hdc, w_hglrc))
		Error("wglMakeCurrent: Failed.");

	PAINTSTRUCT	ps;

	if (!BeginPaint(w_hwnd, &ps)) return;

	QE_CheckOpenGLForErrors();
	v->Draw();
	QE_CheckOpenGLForErrors();
	EndPaint(w_hwnd, &ps);
	SwapBuffers(w_hdc);
}


/*
==================
WndView::CreateWnd
==================
*/
void WndView::CreateWnd()
{
	if (w_wcView.hInstance != g_qeglobals.d_hInstance)
	{
		w_wcView.style = CS_OWNDC | CS_DBLCLKS;
		w_wcView.lpfnWndProc = (WNDPROC)WndView::WndProc;
		w_wcView.cbClsExtra = 0;
		w_wcView.cbWndExtra = 0;
		w_wcView.hInstance = g_qeglobals.d_hInstance;
		w_wcView.hIcon = 0;
		w_wcView.hCursor = LoadCursor(NULL, IDC_ARROW);
		w_wcView.hbrBackground = NULL;
		w_wcView.lpszMenuName = NULL;
		w_wcView.lpszClassName = VIEW_WINDOW_CLASS;

		if (!RegisterClass(&w_wcView))
			Error("ViewWnd RegisterClass failed");
	}

	// windowproc looks up hwnd to find which hwnd is being called, but createwindowex 
	// doesn't return the hwnd until after calling the windowproc a few times, so keep
	// a temporary note to bypass the still-null pointer during those calls
	wvbInitializing = this;
	w_hwnd = CreateWindowEx(
		WS_EX_TOOLWINDOW,		// extended window style	// sikk---> changed window to "tool window"
		VIEW_WINDOW_CLASS,		// registered class name
		name,					// window name
		QE3_CHILD_STYLE,		// window style
		64, 64,					// position of window	
		384, 256,				// window size
		g_qeglobals.d_hwndMain,	// parent or owner window
		NULL,					// menu or child-window identifier
		g_qeglobals.d_hInstance,// application instance
		NULL );					// window-creation data
	wvbInitializing = nullptr;
	
	if (!w_hwnd)
		Error("Could not create window: %s, error %i", name, GetLastError());

	RestorePosition();
//	ShowWindow(w_hwnd, SW_SHOWDEFAULT);
}


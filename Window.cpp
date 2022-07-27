//==============================
//	Window.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "Window.h"
#include "View.h"


#define WND_RESIZE_SNAP 8
#define WND_MOVE_SNAP 2

WNDCLASS Window::wBaseWC;
std::vector<Window*> Window::windex;
Window* Window::wbInitializing;


/*
==================
Window
==================
*/
Window::Window() :
	v(nullptr), wHwnd(nullptr), 
	instance(0), minWidth(64), minHeight(32), vbits(0), 
	clientRect({ 0,0,0,0 }),
	cursorX(-1), cursorY(-1), mouseWithin(false),
	name(nullptr)
{
	windex.push_back(this);
}

/*
==================
~Window
==================
*/
Window::~Window()
{
	windex.erase(std::find(windex.begin(), windex.end(), this));
}

/*
==================
Window::BaseWindowProc

map win32 msg hwnd to our window class and message the right instance
==================
*/
LONG WINAPI Window::BaseWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// CreateWindowEx doesn't return the hwnd until after calling the windowproc a 
	// few times, so keep a temporary note to bypass the still-null pointer during 
	// those calls
	if (Window::wbInitializing)
	{
		Window::wbInitializing->wHwnd = hWnd;
		return Window::wbInitializing->WindowProcedure(uMsg, wParam, lParam);
	}

	for (auto wvIt = Window::windex.begin(); wvIt != Window::windex.end(); ++wvIt)
	{
		if ((*wvIt)->wHwnd == hWnd)
			return (*wvIt)->WindowProcedure(uMsg, wParam, lParam);
	}

	Error("Couldn't find window for hWnd");
	return 0;
}

/*
==================
Window::Create
==================
*/
void Window::Create()
{
	if (wBaseWC.hInstance != g_qeglobals.d_hInstance)
	{
		wBaseWC.style = CS_OWNDC | CS_DBLCLKS;
		wBaseWC.lpfnWndProc = (WNDPROC)Window::BaseWindowProc;
		wBaseWC.cbClsExtra = 0;
		wBaseWC.cbWndExtra = 0;
		wBaseWC.hInstance = g_qeglobals.d_hInstance;
		wBaseWC.hIcon = 0;
		wBaseWC.hCursor = LoadCursor(NULL, IDC_ARROW);
		wBaseWC.hbrBackground = NULL;
		wBaseWC.lpszMenuName = NULL;
		wBaseWC.lpszClassName = BASE_WINDOW_CLASS;

		if (!RegisterClass(&wBaseWC))
			Error("ViewWnd RegisterClass failed");
	}

	wbInitializing = this;
	wHwnd = CreateWindowEx(
		WS_EX_TOOLWINDOW,		// extended window style	// sikk---> changed window to "tool window"
		BASE_WINDOW_CLASS,		// registered class name
		name,					// window name
		QE3_CHILD_STYLE,		// window style
		64, 64,					// position of window	
		384, 256,				// window size
		g_hwndMain,	// parent or owner window
		NULL,					// menu or child-window identifier
		g_qeglobals.d_hInstance,// application instance
		NULL);					// window-creation data
	wbInitializing = nullptr;

	if (!wHwnd)
		Error(_S("Could not create window: %s, error %i") << name << (int)GetLastError());

	RestorePosition();
}






/*
==================
Window::SavePosition
==================
*/
void Window::SavePosition()
{
	wndPos_t pos;
	char lpszName[64];

	sprintf(lpszName, "%s%i", name, instance);
	pos = GetPosition();
	Sys_RegistrySaveInfo(lpszName, &pos, sizeof(pos));
}

/*
==================
Window::RestorePosition
==================
*/
void Window::RestorePosition()
{
	wndPos_t pos;
	LONG lSize = sizeof(pos);
	char lpszName[64];

	sprintf(lpszName, "%s%i", name, instance);

	if (!Sys_RegistryLoadInfo(lpszName, &pos, &lSize))
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

	SetPosition(pos);
}

/*
==================
Window::SetPosition
==================
*/
void Window::SetPosition(int l, int t, int r, int b, bool show)
{
	MoveWindow(wHwnd, l, t, r - l, b - t, FALSE);
	InvalidateRect(wHwnd, NULL, FALSE);

	if (show)
		Show();
	else
		Hide();
}

void Window::SetPosition(wndPos_t pos)
{
	SetPosition(pos.rc.left, pos.rc.top, pos.rc.right, pos.rc.bottom, (pos.vis > 0));
}

Window::wndPos_t Window::GetPosition()
{
	wndPos_t pos;
	pos.vis = IsWindowVisible(wHwnd);
	GetWindowRect(wHwnd, &pos.rc);
	MapWindowPoints(NULL, g_hwndMain, (POINT *)&pos.rc, 2);
	return pos;
}



void Window::ForceUpdate()
{
	InvalidateRect(wHwnd, &clientRect, FALSE);
	//UpdateWindow(wHwnd);
}

/*
==================
Window::Toggle
==================
*/
void Window::Toggle()
{
	if (IsWindowVisible(wHwnd))
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
Window::Swap
==================
*/
void Window::Swap(Window &other)
{
	WINDOWPLACEMENT myWP, otherWP;
	BOOL meVis, otherVis;

	meVis = IsWindowVisible(wHwnd);
	otherVis = IsWindowVisible(other.wHwnd);

	memset(&myWP, 0, sizeof(myWP));
	memset(&otherWP, 0, sizeof(otherWP));
	myWP.length = otherWP.length = sizeof(otherWP);

	GetWindowPlacement(wHwnd, &myWP);
	GetWindowPlacement(other.wHwnd, &otherWP);

	SetWindowPlacement(wHwnd, &otherWP);
	SetWindowPlacement(other.wHwnd, &myWP);

	if (meVis) other.Show();
	else other.Hide();
	if (otherVis) Show();
	else Hide();
}

void Window::CopyPositionFrom(Window &other)
{
	wndPos_t mypos = GetPosition();
	wndPos_t pos = other.GetPosition();
	pos.vis = mypos.vis;
	SetPosition(pos);
}




/*
==================
Window::SetTitle
==================
*/
void Window::SetTitle(const char * title)
{
	SetWindowText(wHwnd, title);
}

/*
==================
Window::GetMsgXY
==================
*/
void Window::GetMsgXY(LPARAM l, int &x, int &y)
{
	x = (short)LOWORD(l);	// horizontal position of cursor 
	y = (int)clientRect.bottom - 1 - (short)HIWORD(l);	// vertical position of cursor 
}




/*
==================
Window::WindowProcedure
==================
*/
int Window::WindowProcedure(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		//ReleaseDC(wHwnd, w_hdc);
		return OnDestroy();

	case WM_CREATE:
		//w_hdc = GetDC(wHwnd);
		return OnCreate();

	case WM_PAINT:
		if (!IsWindowVisible(wHwnd)) return 0;
		if (OnPaint())
			return DefWindowProc(wHwnd, uMsg, wParam, lParam);
		return 1;

	case WM_SIZE:
		GetClientRect(wHwnd, &clientRect);
		OnResized();
		InvalidateRect(wHwnd, NULL, FALSE);
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
		DefWindowProc(wHwnd, uMsg, wParam, lParam);
		return WVR_REDRAW;

	case WM_GETMINMAXINFO:
	{
		LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
		lpmmi->ptMinTrackSize.x = minWidth;
		lpmmi->ptMinTrackSize.y = minHeight;
		return 0;
	}

	case WM_KILLFOCUS:
	case WM_SETFOCUS:
		SendMessage(wHwnd, WM_NCACTIVATE, uMsg == WM_SETFOCUS, 0);
		return 0;

	case WM_CLOSE:
		DestroyWindow(wHwnd);
		return 0;

	case WM_NCLBUTTONDOWN:
	case WM_NCRBUTTONDOWN:
		Focus();
		break;

	// generate mouseout events on all windows by default
	case WM_MOUSEMOVE:
		if (!mouseWithin)
		{
			mouseWithin = true;
			TRACKMOUSEEVENT tmev;
			tmev.cbSize = sizeof(TRACKMOUSEEVENT);
			tmev.hwndTrack = wHwnd;
			tmev.dwFlags = TME_LEAVE;

			TrackMouseEvent(&tmev);
		}
		break;
	case WM_MOUSELEAVE:
		mouseWithin = false;
		break;
	}

	if (!OnMessage(uMsg, wParam, lParam))
		return 0;

	return DefWindowProc(wHwnd, uMsg, wParam, lParam);
}

int Window::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) { return 1; }
int Window::OnCreate() { return 1; }
int Window::OnDestroy() { return 1; }
int Window::OnPaint() { return 1; }
int Window::OnResized() { return 1; }

void Window::MoveRect(HWND hwnd, RECT r)
{
	SetWindowPos(hwnd, HWND_TOP, r.left, r.top, r.right, r.bottom, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOZORDER);
}

void Window::MoveRect(HWND hwnd, int left, int top, int right, int bottom)
{
	SetWindowPos(hwnd, HWND_TOP, left, top, right, bottom, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOZORDER);
}



/*
========================================================================

	DOCKING

========================================================================
*/

/*
==================
Window::FindClosestHorizontal
==================
*/
int Window::FindClosestHorizontal(int h, LPRECT field)
{
	RECT wr;
	int best;

	if (field->bottom - h < h - field->top)
		best = field->bottom;
	else
		best = field->top;

	for (auto wvIt = windex.begin(); wvIt != windex.end(); ++wvIt)
	{
		if ((*wvIt) == this)
			continue;
		if (!IsWindowVisible((*wvIt)->wHwnd))
			continue;

		GetWindowRect((*wvIt)->wHwnd, &wr);
		if (abs(wr.top - h) < abs(best - h))
			best = wr.top;

		if (abs(wr.bottom - h) < abs(best - h))
			best = wr.bottom;
	}
	return best;
}

/*
==================
Window::SnapToClosestHorizontal
==================
*/
bool Window::SnapToClosestHorizontal(LPRECT win, LPRECT field, bool down)
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
Window::FindClosestVertical
==================
*/
int Window::FindClosestVertical(int v, LPRECT field)
{
	RECT wr;
	int best;

	if (field->right - v < v - field->left)
		best = field->right;
	else
		best = field->left;

	for (auto wvIt = windex.begin(); wvIt != windex.end(); ++wvIt)
	{
		if ((*wvIt) == this)
			continue;
		if (!IsWindowVisible((*wvIt)->wHwnd))
			continue;

		GetWindowRect((*wvIt)->wHwnd, &wr);
		if (abs(wr.left - v) < abs(best - v))
			best = wr.left;

		if (abs(wr.right - v) < abs(best - v))
			best = wr.right;
	}
	return best;
}

/*
==================
Window::SnapToClosestVertical
==================
*/
bool Window::SnapToClosestVertical(LPRECT win, LPRECT field, bool right)
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
Window::SnapMove
==================
*/
bool Window::SnapMove(LPRECT win, LPRECT field)
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
Window::TryDocking
==================
*/
bool Window::TryDocking(long side, LPRECT rect)
{
	RECT	field, statusrect, toolbarrect;
	GetWindowRect(g_hwndRebar, &toolbarrect);
	GetWindowRect(g_hwndStatus, &statusrect);

	// FIXME: assumes toolbar is at the top of the screen
	GetClientRect(g_hwndMain, &field);
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

//==============================
//	WndZChecker.c
//==============================

#include "qe3.h"

WndZChecker::WndZChecker()
{
	name = "ZWindow";
	vbits = W_Z;
}


WndZChecker::~WndZChecker()
{
}

void WndZChecker::Initialize()
{
	zv = &g_qeglobals.d_vZ;
	v = zv;

	CreateWnd();
	SetTitle("Z Checker");
	g_qeglobals.d_hwndZ = w_hwnd;
}

int WndZChecker::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int		fwKeys, xPos, yPos;

	switch (uMsg)
	{
	case WM_KEYDOWN:
		QE_KeyDown(wParam);
		return 0;

	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
		Focus();
		SetCapture(g_qeglobals.d_hwndZ);
		fwKeys = wParam;				// key flags 
		xPos = (short)LOWORD(lParam);	// horizontal position of cursor 
		yPos = (short)HIWORD(lParam);	// vertical position of cursor 
		yPos = (int)clientRect.bottom - 1 - yPos;
		zv->MouseDown(xPos, yPos, fwKeys);
		return 0;

	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_LBUTTONUP:
		fwKeys = wParam;				// key flags 
		xPos = (short)LOWORD(lParam);	// horizontal position of cursor 
		yPos = (short)HIWORD(lParam);	// vertical position of cursor 
		yPos = (int)clientRect.bottom - 1 - yPos;
		zv->MouseUp(xPos, yPos, fwKeys);
		if (!(fwKeys & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
			ReleaseCapture();
		return 0;

	case WM_MOUSEMOVE:
		fwKeys = wParam;				// key flags 
		xPos = (short)LOWORD(lParam);	// horizontal position of cursor 
		yPos = (short)HIWORD(lParam);	// vertical position of cursor 
		yPos = (int)clientRect.bottom - 1 - yPos;
		zv->MouseMoved(xPos, yPos, fwKeys);
		return 0;
	}
	return 1;
}

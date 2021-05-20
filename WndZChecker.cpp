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

	if (Tool::HandleInput1D(uMsg, wParam, lParam, *zv, *this))
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
		fwKeys = wParam;
		if (fwKeys & MK_CONTROL)
		{
			if ((short)HIWORD(wParam) < 0)
			{
				v->scale = max(0.05f, v->scale * 0.8f);
			}
			else
			{
				v->scale = min(32.0f, v->scale * 1.25f);
			}
		}
		else
		{
			float fwd = 64;
			if ((short)HIWORD(wParam) < 0) fwd *= -1;
			if (fwKeys & MK_SHIFT) fwd *= 4;
			v->origin.z += fwd / v->scale;
		}
		Sys_UpdateWindows(W_Z);
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

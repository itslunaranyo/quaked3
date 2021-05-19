//==============================
//	WndCamera.c
//==============================

#include "qe3.h"

WndCamera::WndCamera()
{
	name = "CameraWindow";
	vbits = W_CAMERA;
}

WndCamera::~WndCamera()
{
}

void WndCamera::Initialize()
{
	v = &g_qeglobals.d_vCamera;
	CreateWnd();
	SetTitle("Camera View");
	g_qeglobals.d_hwndCamera = w_hwnd;
}

int WndCamera::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int		fwKeys, xPos, yPos;

	switch (uMsg)
	{
	case WM_KEYDOWN:
		if (QE_KeyDown(wParam))
			return 0;
		else
			return DefWindowProc(w_hwnd, uMsg, wParam, lParam);

	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
		Focus();
		SetCapture(w_hwnd);
		fwKeys = wParam;				// key flags 
		xPos = (short)LOWORD(lParam);	// horizontal position of cursor 
		yPos = (short)HIWORD(lParam);	// vertical position of cursor 
		yPos = (int)clientRect.bottom - 1 - yPos;
		v->MouseDown(xPos, yPos, fwKeys);
		return 0;

	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_LBUTTONUP:
		fwKeys = wParam;				// key flags 
		xPos = (short)LOWORD(lParam);	// horizontal position of cursor 
		yPos = (short)HIWORD(lParam);	// vertical position of cursor 
		yPos = (int)clientRect.bottom - 1 - yPos;
		v->MouseUp(xPos, yPos, fwKeys);
		if (!(fwKeys & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
			ReleaseCapture();
		return 0;

	case WM_MOUSEMOVE:
		fwKeys = wParam;				// key flags 
		xPos = (short)LOWORD(lParam);	// horizontal position of cursor 
		yPos = (short)HIWORD(lParam);	// vertical position of cursor 
		yPos = (int)clientRect.bottom - 1 - yPos;
		v->MouseMoved(xPos, yPos, fwKeys);
		return 0;
	}
	return 1;
}

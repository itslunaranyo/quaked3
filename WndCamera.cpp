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
	cv = &g_qeglobals.d_vCamera;
	v = cv;
	CreateWnd();
	SetTitle("Camera View");
	g_qeglobals.d_hwndCamera = w_hwnd;
}

int WndCamera::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (Tool::HandleInput3D(uMsg, wParam, lParam, *cv, *this))
	{
		if (uMsg > WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)
			Focus();
		return 1;
	}
	/*
	int		fwKeys, xPos, yPos;
	switch (uMsg)
	{
	case WM_KEYDOWN:
		if (QE_KeyDown(wParam))
			return 0;
		else
			return DefWindowProc(w_hwnd, uMsg, wParam, lParam);
	//case WM_KEYUP:
	//	if (QE_KeyUp(wParam))
	//		return 0;
	//	else
	//		return DefWindowProc(w_hwnd, uMsg, wParam, lParam);

	case WM_MOUSEWHEEL:
		Focus();
		fwKeys = wParam;
		if (fwKeys & MK_CONTROL)
		{
			cv->ChangeFloor((short)HIWORD(wParam) > 0);
		}
		else
		{
			vec3 fwd;
			if (fwKeys & MK_RBUTTON && fwKeys & MK_SHIFT)
			{
				fwd = cv->vpn;	// orbiting
			}
			else
			{
				fwd = cv->forward;
				if (fwKeys & MK_SHIFT)
					fwd *= 4.0f;
			}
			if ((short)HIWORD(wParam) < 0) fwd *= -1;
			cv->origin += fwd * 64.0f;
		}
		Sys_UpdateWindows(W_SCENE);
		return 0;

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
	*/
	return 1;
}

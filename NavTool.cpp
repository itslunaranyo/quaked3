//==============================
//	NavTool.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "NavTool.h"
#include "WndCamera.h"
#include "WndGrid.h"
#include "WndZChecker.h"
#include "WndTexture.h"
#include "WndGrid.h"
#include "CameraView.h"
#include "GridView.h"
#include "ZView.h"
#include "TextureView.h"

NavTool::NavTool() : hotMButton(0), Tool("Navigation", false)	// always on (not modal)
{
}


NavTool::~NavTool()
{
}

bool NavTool::Input3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndCamera &vWnd)
{
	int		xPos, yPos;
	int mbtn = MouseButtonForMessage(uMsg);

	switch (uMsg)
	{
	case WM_KEYDOWN:
		if (hot)
		{
			if (wParam == VK_NEXT || wParam == VK_PRIOR)
				return false;
			MsgToXY(lParam, vWnd, xPos, yPos);
			if (wParam == VK_SHIFT)
				v.SetOrbit();	// TODO: what
			return true;
		}
		if (QE_KeyDown(wParam))
			return true;
		return false;

	/*case WM_KEYUP:
		if (QE_KeyUp(wParam))
			return 0;
		else
			return DefWindowProc(w_hwnd, uMsg, wParam, lParam);*/

	case WM_REALTIME:
		vWnd.RealtimeControl(g_deltaTime);
		return false;

	case WM_MOUSEWHEEL:
		//Focus();
		//if (hotMButton) return false;	// allow mwheel forward/backward/etc while rmb-aiming
		vWnd.GetMsgXY(lParam, xPos, yPos);
		vWnd.MouseWheel(xPos, yPos, (short)HIWORD(wParam) > 0, wParam);
		WndMain_UpdateWindows(W_SCENE);
		return true;

	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
		if (hotMButton) return false;
		hotMButton = mbtn;
		hot = true;
		SetCapture(vWnd.wHwnd);
		vWnd.GetMsgXY(lParam, xPos, yPos);
		vWnd.MouseDown(xPos, yPos, mbtn, wParam);
		return true;

	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_LBUTTONUP:
		if (mbtn != hotMButton) return false;
		vWnd.GetMsgXY(lParam, xPos, yPos);
		vWnd.MouseUp(xPos, yPos, mbtn, wParam);
		//if (!(wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
		if (!(wParam & hotMButton))
			ReleaseCapture();
		hotMButton = 0;
		hot = false;
		return false;

	case WM_MOUSEMOVE:
		vWnd.GetMsgXY(lParam, xPos, yPos);
		WPARAM mBtnFilter = wParam & ((MK_LBUTTON | MK_RBUTTON | MK_MBUTTON) - hotMButton);
		vWnd.MouseMoved(xPos, yPos, wParam - mBtnFilter); // filter buttons we aren't responding to
		return hot;
	}
	return hot;
}

bool NavTool::Input2D(UINT uMsg, WPARAM wParam, LPARAM lParam, GridView &v, WndGrid &vWnd)
{
	int		xPos, yPos;
	vec3	coord;
	int mbtn = MouseButtonForMessage(uMsg);

	switch (uMsg)
	{
	case WM_KEYDOWN:
		return (QE_KeyDown(wParam) || hot);
	//case WM_KEYUP:
	//	return QE_KeyUp(wParam);

	case WM_MOUSEWHEEL:
		if (hotMButton) return false;
		vWnd.GetMsgXY(lParam, xPos, yPos);
		v.ScreenToWorld(xPos, yPos, coord);
		if ((short)HIWORD(wParam) < 0)
			v.ZoomOut(coord);
		else
			v.ZoomIn(coord);
		WndMain_UpdateWindows(W_XY);
		return true;

	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
		if (hotMButton) return false;
		hotMButton = mbtn;
		hot = true;
		SetCapture(vWnd.wHwnd);
		vWnd.GetMsgXY(lParam, xPos, yPos);
		vWnd.MouseDown(xPos, yPos, mbtn, wParam);
		return true;

	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_LBUTTONUP:
		if (mbtn != hotMButton) return false;
		vWnd.GetMsgXY(lParam, xPos, yPos);
		vWnd.MouseUp(xPos, yPos, mbtn, wParam);
		//if (!(wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
		if (!(wParam & hotMButton))
			ReleaseCapture();
		hotMButton = 0;
		hot = false;
		return false;

	case WM_MOUSEMOVE:
		vWnd.GetMsgXY(lParam, xPos, yPos);
		WPARAM mBtnFilter = wParam & ((MK_LBUTTON | MK_RBUTTON | MK_MBUTTON) - hotMButton);
		vWnd.MouseMoved(xPos, yPos, wParam - mBtnFilter); // filter buttons we aren't responding to
		return false;
	}
	return false;
}

bool NavTool::Input1D(UINT uMsg, WPARAM wParam, LPARAM lParam, ZView &v, WndZChecker &vWnd)
{
	int		xPos, yPos;
	int mbtn = MouseButtonForMessage(uMsg);

	switch (uMsg)
	{
	case WM_KEYDOWN:
		return (QE_KeyDown(wParam) || hot);
	/*case WM_KEYUP:
		return QE_KeyUp(wParam);*/

	case WM_MOUSEWHEEL:
		if (hotMButton) return false;
		if (wParam & MK_CONTROL)
		{
			if ((short)HIWORD(wParam) < 0)
				v.ScaleDown();
			else
				v.ScaleUp();
		}
		else
		{
			float fwd = 64;
			if ((short)HIWORD(wParam) < 0) fwd *= -1;
			if (wParam & MK_SHIFT) fwd *= 4;
			v.Scroll(fwd);
		}
		WndMain_UpdateWindows(W_Z);
		return true;

	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
		if (hotMButton) return false;
		hotMButton = mbtn;
		SetCapture(g_hwndZ);
		MsgToXY(lParam, vWnd, xPos, yPos);
		vWnd.MouseDown(xPos, yPos, mbtn, wParam);
		WndMain_UpdateWindows(W_Z);
		return true;

	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_LBUTTONUP:
		if (mbtn != hotMButton) return false;
		MsgToXY(lParam, vWnd, xPos, yPos);
		vWnd.MouseUp(xPos, yPos, mbtn, wParam);
		WndMain_UpdateWindows(W_Z);
		//if (!(wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
		if (!(wParam & hotMButton))
			ReleaseCapture();
		hotMButton = 0;
		return false;

	case WM_MOUSEMOVE:
		MsgToXY(lParam, vWnd, xPos, yPos);
		WPARAM mBtnFilter = wParam & ((MK_LBUTTON | MK_RBUTTON | MK_MBUTTON) - hotMButton);
		vWnd.MouseMoved(xPos, yPos, wParam - mBtnFilter); // filter buttons we aren't responding to
		WndMain_UpdateWindows(W_Z);
		return false;
	}
	return hot;
}

bool NavTool::InputTex(UINT uMsg, WPARAM wParam, LPARAM lParam, TextureView &v, WndTexture &vWnd)
{
	int xPos, yPos;
	int mbtn = MouseButtonForMessage(uMsg);

	switch (uMsg)
	{
	case WM_KEYDOWN:
		return (QE_KeyDown(wParam) || hot);
	//case WM_KEYUP:
	//	return QE_KeyUp(wParam);

	case WM_MOUSEWHEEL:
		if (hotMButton) return false;
		if (wParam & MK_CONTROL)
		{
			if ((short)HIWORD(wParam) < 0)
				v.Scale(0.8f);
			else
				v.Scale(1.25f);
		}
		else
		{
			v.Scroll(((short)HIWORD(wParam) < 0) ? -64.0f : 64.0f, (wParam & MK_SHIFT) > 0);
		}
		WndMain_UpdateWindows(W_TEXTURE);
		return true;

	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
		if (hotMButton) return false;
		hotMButton = mbtn;
		SetCapture(vWnd.wHwnd);
		xPos = (short)LOWORD(lParam);  // horizontal position of cursor 
		yPos = (short)HIWORD(lParam);  // vertical position of cursor 

		vWnd.MouseDown(xPos, yPos, mbtn, wParam);
		return true;

	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_LBUTTONUP:
		if (mbtn != hotMButton) return false;
		xPos = (short)LOWORD(lParam);  // horizontal position of cursor 
		yPos = (short)HIWORD(lParam);  // vertical position of cursor 

		vWnd.MouseUp(xPos, yPos, mbtn, wParam);
		//if (!(wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
		if (!(wParam & hotMButton))
			ReleaseCapture();
		hotMButton = 0;

		return false;

	case WM_MOUSEMOVE:
		xPos = (short)LOWORD(lParam);  // horizontal position of cursor 
		yPos = (short)HIWORD(lParam);  // vertical position of cursor 

		WPARAM mBtnFilter = wParam & ((MK_LBUTTON | MK_RBUTTON | MK_MBUTTON) - hotMButton);
		vWnd.MouseMoved(xPos, yPos, wParam - mBtnFilter); // filter buttons we aren't responding to

		return false;
	}
	return false;
}

bool NavTool::Input(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return false;
}

int NavTool::MouseButtonForMessage(UINT msg)
{
	switch (msg) {
	case WM_MBUTTONUP:
	case WM_MBUTTONDOWN:
		return MK_MBUTTON;
	case WM_LBUTTONUP:
	case WM_LBUTTONDOWN:
		return MK_LBUTTON;
	case WM_RBUTTONUP:
	case WM_RBUTTONDOWN:
		return MK_RBUTTON;
	default:
		return 0;
	}
	return 0;
}

WPARAM NavTool::FilterWParam(const WPARAM wpIn, const int mbtn)
{
	int filt = (VK_LBUTTON | VK_MBUTTON | VK_RBUTTON) - mbtn;
	return wpIn - (wpIn & filt);
}

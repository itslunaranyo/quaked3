//==============================
//	NavTool.cpp
//==============================

#include "qe3.h"
#include "NavTool.h"

NavTool::NavTool() :
	Tool("Navigation", false)	// always on (not modal)
{
}


NavTool::~NavTool()
{
}

bool NavTool::Input3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndView &vWnd)
{
	int		xPos, yPos;

	switch (uMsg)
	{
	case WM_KEYDOWN:
		if (hot)
		{
			if (wParam == VK_NEXT || wParam == VK_PRIOR)
				return false;
			MsgToXY(lParam, vWnd, xPos, yPos);
			if (wParam == VK_SHIFT)
				v.SetOrbit(v.vpn);
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

	case WM_MOUSEWHEEL:
		//Focus();
		MsgToXY(lParam, vWnd, xPos, yPos);
		v.MouseWheel(xPos, yPos, (short)HIWORD(wParam) > 0, wParam);
		Sys_UpdateWindows(W_SCENE);
		return true;

	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
		hot = true;
		SetCapture(vWnd.w_hwnd);
		MsgToXY(lParam, vWnd, xPos, yPos);
		v.MouseDown(xPos, yPos, wParam);
		return true;

	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_LBUTTONUP:
		hot = false;
		MsgToXY(lParam, vWnd, xPos, yPos);
		v.MouseUp(xPos, yPos, wParam);
		if (!(wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
			ReleaseCapture();
		return false;

	case WM_MOUSEMOVE:
		MsgToXY(lParam, vWnd, xPos, yPos);
		v.MouseMoved(xPos, yPos, wParam);
		return hot;

	case WM_REALTIME:
		v.RealtimeControl(g_deltaTime);
		return false;
	}
	return hot;
}

bool NavTool::Input2D(UINT uMsg, WPARAM wParam, LPARAM lParam, XYZView &v, WndView &vWnd)
{
	int		xPos, yPos;
	switch (uMsg)
	{
	case WM_KEYDOWN:
		return (QE_KeyDown(wParam) || hot);
	//case WM_KEYUP:
	//	return QE_KeyUp(wParam);

	case WM_MOUSEWHEEL:
		if ((short)HIWORD(wParam) < 0)
			v.ScaleDown();
		else
			v.ScaleUp();
		return true;

	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
		SetCapture(vWnd.w_hwnd);
		MsgToXY(lParam, vWnd, xPos, yPos);
		v.MouseDown(xPos, yPos, wParam);
		// sikk---> Context Menu
		if (uMsg == WM_RBUTTONDOWN)
		{
			g_bMoved = false;
			g_nMouseX = xPos;
			g_nMouseY = yPos;
		}
		// <---sikk
		return true;

	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_LBUTTONUP:
		MsgToXY(lParam, vWnd, xPos, yPos);
		v.MouseUp(xPos, yPos, wParam);
		if (!(wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
			ReleaseCapture();
		// sikk---> Context Menu
		if (uMsg == WM_RBUTTONUP && !g_bMoved)
			dynamic_cast<WndGrid&>(vWnd).DoPopupMenu(xPos, yPos);
		// <---sikk
		return false;

	case WM_MOUSEMOVE:
		MsgToXY(lParam, vWnd, xPos, yPos);
		v.MouseMoved(xPos, yPos, wParam);
		// sikk---> Context Menu
		if (!g_bMoved && (g_nMouseX != xPos || g_nMouseY != yPos))
			g_bMoved = true;
		// <---sikk
		return false;
	}
	return false;
}

bool NavTool::Input1D(UINT uMsg, WPARAM wParam, LPARAM lParam, ZView &v, WndView &vWnd)
{
	int		xPos, yPos;

	switch (uMsg)
	{
	case WM_KEYDOWN:
		return (QE_KeyDown(wParam) || hot);
	/*case WM_KEYUP:
		return QE_KeyUp(wParam);*/

	case WM_MOUSEWHEEL:
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
		return true;

	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
		SetCapture(g_qeglobals.d_hwndZ);
		MsgToXY(lParam, vWnd, xPos, yPos);
		v.MouseDown(xPos, yPos, wParam);
		return true;

	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_LBUTTONUP:
		MsgToXY(lParam, vWnd, xPos, yPos);
		v.MouseUp(xPos, yPos, wParam);
		if (!(wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
			ReleaseCapture();
		return false;

	case WM_MOUSEMOVE:
		MsgToXY(lParam, vWnd, xPos, yPos);
		v.MouseMoved(xPos, yPos, wParam);
		return false;
	}
	return hot;
}

bool NavTool::InputTex(UINT uMsg, WPARAM wParam, LPARAM lParam, TextureView &v, WndView &vWnd)
{
	int xPos, yPos;
	switch (uMsg)
	{
	case WM_KEYDOWN:
		return (QE_KeyDown(wParam) || hot);
	//case WM_KEYUP:
	//	return QE_KeyUp(wParam);

	case WM_MOUSEWHEEL:
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
		Sys_UpdateWindows(W_TEXTURE);
		return true;

	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
		SetCapture(vWnd.w_hwnd);
		xPos = (short)LOWORD(lParam);  // horizontal position of cursor 
		yPos = (short)HIWORD(lParam);  // vertical position of cursor 

		v.MouseDown(xPos, yPos, wParam);
		// Context Menu
		if (uMsg == WM_RBUTTONDOWN)
		{
			g_bMoved = false;
			g_nMouseX = xPos;
			g_nMouseY = yPos;
		}
		return true;

	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_LBUTTONUP:
		xPos = (short)LOWORD(lParam);  // horizontal position of cursor 
		yPos = (short)HIWORD(lParam);  // vertical position of cursor 

		v.MouseUp(xPos, yPos, wParam);
		if (!(wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
			ReleaseCapture();

		// Context Menu
		if (uMsg == WM_RBUTTONUP && !g_bMoved)
			dynamic_cast<WndTexture&>(vWnd).DoPopupMenu(xPos, yPos);
		return false;

	case WM_MOUSEMOVE:
		xPos = (short)LOWORD(lParam);  // horizontal position of cursor 
		yPos = (short)HIWORD(lParam);  // vertical position of cursor 

		v.MouseMoved(xPos, yPos, wParam);

		// Context Menu
		if (!g_bMoved && (g_nMouseX != xPos || g_nMouseY != yPos))
			g_bMoved = true;
		return false;
	}
	return false;
}

bool NavTool::Input(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return false;
}

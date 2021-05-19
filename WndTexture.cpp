//==============================
//	WndTexture.cpp
//==============================

#include "qe3.h"

WndTexture::WndTexture()
{
	name = "TextureWindow";
	vbits = W_TEXTURE;
}

WndTexture::~WndTexture()
{
}

void WndTexture::Initialize()
{
	texv = &g_qeglobals.d_vTexture;
	v = texv;
	CreateWnd();
	SetTitle("Textures");
	g_qeglobals.d_hwndTexture = w_hwnd;
}

int WndTexture::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int xPos, yPos, fwKeys;

	if (Tool::HandleInputTex(uMsg, wParam, lParam, *texv, *this))
	{
		if (uMsg > WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)
			Focus();
		return 1;
	}

	switch (uMsg)
	{
	case WM_KEYDOWN:
		return QE_KeyDown(wParam);

	case WM_MOUSEWHEEL:
		Focus();
		fwKeys = wParam;
		if (fwKeys & MK_CONTROL)
		{
			if ((short)HIWORD(wParam) < 0)
				texv->Scale(0.8f);
			else
				texv->Scale(1.25f);
		}
		else
		{
			texv->Scroll(((short)HIWORD(wParam) < 0) ? -64.0f : 64.0f, (fwKeys & MK_SHIFT) > 0);
		}
		Sys_UpdateWindows(W_TEXTURE);
		return 0;

	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
		Focus();
		SetCapture(w_hwnd);
		xPos = (short)LOWORD(lParam);  // horizontal position of cursor 
		yPos = (short)HIWORD(lParam);  // vertical position of cursor 

		texv->MouseDown(xPos, yPos, wParam);
		return 0;

	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_LBUTTONUP:
		xPos = (short)LOWORD(lParam);  // horizontal position of cursor 
		yPos = (short)HIWORD(lParam);  // vertical position of cursor 

		texv->MouseUp(xPos, yPos, wParam);
		if (!(wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
			ReleaseCapture();
		return 0;

	case WM_MOUSEMOVE:
		xPos = (short)LOWORD(lParam);  // horizontal position of cursor 
		yPos = (short)HIWORD(lParam);  // vertical position of cursor 

		texv->MouseMoved(xPos, yPos, wParam);
		return 0;
	}

	return 1;
}

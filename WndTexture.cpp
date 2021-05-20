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


void WndTexture::DoPopupMenu(int x, int y)
{
	HMENU	hMenu;
	POINT	point;
	int		retval;

	TextureView::texWndGroup_t* twg = texv->TexGroupAtCursorPos(x, y);
	if (!twg) return;
	GetCursorPos(&point);
	hMenu = GetSubMenu(LoadMenu(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDR_CONTEXT_TEXGRP)), 0);

	//bool flushed = twg->tg->flushed;
	//QE_CheckMenuItem(hMenu, ID_CONTEXT_FLUSHUNUSED, flushed);
	//QE_CheckMenuItem(hMenu, ID_CONTEXT_LOADCOMPLETELY, !flushed);

	retval = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON, point.x, point.y, 0, w_hwnd, NULL);

	switch (retval)
	{
	case ID_CONTEXT_RELOAD:
		Textures::MenuReloadGroup(twg->tg);
		break;
	case ID_CONTEXT_FLUSHUNUSED:
		Textures::FlushUnused(twg->tg);
		break;
	case ID_CONTEXT_LOADCOMPLETELY:
		Textures::MenuLoadWad(twg->tg->name);
		break;
	default:
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, retval, 0);
	}

	DestroyMenu(hMenu);
}


int WndTexture::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	if (Tool::HandleInputTex(uMsg, wParam, lParam, *texv, *this))
	{
		if (uMsg > WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)
			Focus();
		return 1;
	}
	/*
	int xPos, yPos, fwKeys;
	switch (uMsg)
	{
	case WM_KEYDOWN:
		return QE_KeyDown(wParam);
	//case WM_KEYUP:
	//	return QE_KeyUp(wParam);

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
	*/
	return 1;
}

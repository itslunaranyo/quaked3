//==============================
//	WndTexture.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "TextureGroup.h"
#include "WndTexture.h"
#include "TextureView.h"
#include "TexBrowserRenderer.h"
#include "Tool.h"
#include "TextureTool.h"
#include "select.h"

HWND g_hwndTexture;

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
	texv = &g_vTexture;
	v = texv;
	Create();

	UpdateTitle();
	g_hwndTexture = wHwnd;

	tbr = new TexBrowserRenderer(*texv);
	r = tbr;
}

void WndTexture::UpdateTitle()
{
	if (g_cfgUI.HideUnusedTextures)
		SetTitle("Textures (used)");
	else
		SetTitle("Textures");
}

void WndTexture::DoPopupMenu(int x, int y)
{
	HMENU	hMenu;
	POINT	point;
	int		mnum, retval;

	Texture* tx = texv->TexAtCursorPos(x, y);
	TextureGroup* tg = texv->TexGroupAtCursorPos(x, y);
	
	if (tx) mnum = 2;
	else if (tg) mnum = 1;
	else mnum = 0;

	GetCursorPos(&point);
	hMenu = GetSubMenu(LoadMenu(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDR_CONTEXT_TEXGRP)), mnum);

	WndMain_CheckMenuItem(hMenu, ID_TEXTURES_HIDEUNUSED, g_cfgUI.HideUnusedTextures);

	retval = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON, point.x, point.y, 0, wHwnd, NULL);

	switch (retval)
	{
	case ID_TEXTURES_RELOADWAD:
		Textures::MenuReloadGroup(tg);
		break;
	case ID_TEXTURES_SELECTMATCHINGCTX:
		Selection::MatchingTextures(tx);
		break;
	case ID_TEXTURES_REPLACEMATCHINGCTX:
		g_texTool->FindTextureDialog(tx);
		break;
	default:
		PostMessage(g_hwndMain, WM_COMMAND, retval, 0);
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
		return QE_KeyDown(wParam, lParam);
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
		WndMain_UpdateWindows(W_TEXTURE);
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

int WndTexture::OnResized()
{
	texv->Resize(clientRect.right, clientRect.bottom);
	return 0;
}

void WndTexture::Render()
{
	tbr->Draw();
}

/*
==============
WndTexture::MouseDown
==============
*/
void WndTexture::MouseDown(const int x, const int y, const int btndown, const int buttons)
{
	Sys_GetCursorPos(&cursorX, &cursorY);	// necessary for scrolling
	
	// Context Menu
	if (buttons == MK_RBUTTON)
	{
		rMoved = false;
		mDownX = x;
		mDownY = y;
	}

	if (btndown == MK_LBUTTON)
		texv->FoldTextureGroup(x, y);
}

/*
==============
WndTexture::MouseMoved
==============
*/
void WndTexture::MouseMoved(const int x, const int y, const int buttons)
{
	int cx, cy;
	float scale;
	texv->Arrange();

	// sikk--->	Mouse Zoom Texture Window
	// rbutton+control = zoom texture view
	if (buttons == (MK_CONTROL | MK_RBUTTON))
	{
		SetCursor(NULL); // sikk - Remove Cursor
		Sys_GetCursorPos(&cx, &cy);
		if (cy != cursorY)
		{
			if (cy > cursorY)
				scale = powf(1.01f, fabs(cy - cursorY));
			else
				scale = powf(0.99f, fabs(cy - cursorY));

			texv->Scale(scale);
			Sys_SetCursorPos(cursorX, cursorY);

			WndMain_UpdateWindows(W_TEXTURE);
		}
		return;
	}
	// <---sikk

	// rbutton = drag texture origin
	if (buttons == MK_RBUTTON)
	{
		// Context Menu
		if (!rMoved && (abs(mDownX - x) > 1 || abs(mDownY - y) > 1))
			rMoved = true;

		if (rMoved) SetCursor(NULL);

		Sys_GetCursorPos(&cx, &cy);
		if ((cy != cursorY) && rMoved)
		{
			texv->Scroll(cy - cursorY, (buttons & MK_SHIFT) > 0);
			Sys_SetCursorPos(cursorX, cursorY);

			WndMain_UpdateWindows(W_TEXTURE);
		}
	}
	else
		MouseOver(x, y);
}

/*
============
WndTexture::MouseOver
============
*/
void WndTexture::MouseOver(const int x, const int y)
{
	char		texstring[256];
	Texture*	tex;

	tex = texv->TexAtCursorPos(x, y);
	if (tex)
	{
		sprintf(texstring, "%s (%dx%d)", tex->name.c_str(), tex->width, tex->height);
		WndMain_Status(texstring, 0);
		return;
	}
	sprintf(texstring, "");
	WndMain_Status(texstring, 0);
}

void WndTexture::OnShow()
{
	UpdateTitle();
}

void WndTexture::MouseUp(const int x, const int y, const int btnup, const int buttons)
{
	// Context Menu
	if (btnup == MK_RBUTTON && !buttons && !rMoved)
		DoPopupMenu(x, y);
}


//==============================
//	WndTexture.h
//==============================

#ifndef __WND_TEXTURE_H__
#define __WND_TEXTURE_H__

#include "WndView.h"
extern HWND g_hwndTexture;
extern WndTexture	*g_wndTexture;
class TextureView;

class WndTexture : public WndView
{
public:
	WndTexture();
	~WndTexture();

	TextureView *texv;

	void Initialize();
	void DoPopupMenu(int x, int y);
	int OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#endif

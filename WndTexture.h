//==============================
//	WndTexture.h
//==============================

#ifndef __WND_TEXTURE_H__
#define __WND_TEXTURE_H__

#include "WindowGL.h"
extern HWND g_hwndTexture;
extern WndTexture	*g_wndTexture;
class TextureView;
class TexBrowserRenderer;

class WndTexture : public WindowGL
{
public:
	WndTexture();
	~WndTexture();

	TextureView *texv;

	void	Initialize();

	void	DoPopupMenu(int x, int y);
	void	UpdateTitle();
	int		OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void	MouseDown(const int x, const int y, const int btndown, const int buttons);
	void	MouseUp(const int x, const int y, const int btnup, const int buttons);
	void	MouseMoved(const int x, const int y, const int buttons);
	void	MouseOver(const int x, const int y);
private:
	void	OnShow();
	TexBrowserRenderer* tbr;
	int		OnResized();
	void	Render();
	int	mDownX, mDownY;
	bool rMoved;
};

#endif

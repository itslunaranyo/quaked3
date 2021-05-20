//==============================
//	WndZChecker.h
//==============================

#ifndef __WND_ZCHECKER_H__
#define __WND_ZCHECKER_H__

#include "WindowGL.h"
extern HWND g_hwndZ;
extern WndZChecker	*g_wndZ;
class ZView;
class ZViewRenderer;

class WndZChecker : public WindowGL
{
public:
	WndZChecker();
	~WndZChecker();

	ZView *zv;

	void	Initialize();
	int		OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void	MouseDown(const int x, const int y, const int btndown, const int buttons);
	void	MouseUp(const int x, const int y, const int btnup, const int buttons);
	void	MouseMoved(const int x, const int y, const int buttons);
private:
	ZViewRenderer *zr;
	void Render();
	int OnResized();
};

#endif
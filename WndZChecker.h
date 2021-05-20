//==============================
//	WndZChecker.h
//==============================

#ifndef __WND_ZCHECKER_H__
#define __WND_ZCHECKER_H__

#include "WndView.h"
extern HWND g_hwndZ;
extern WndZChecker	*g_wndZ;
class ZView;

class WndZChecker : public WndView
{
public:
	WndZChecker();
	~WndZChecker();

	ZView *zv;

	void Initialize();
	int OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#endif
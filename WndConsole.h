//==============================
//	WndConsole.h
//==============================

#ifndef __WND_CONSOLE_H__
#define __WND_CONSOLE_H__

#include "Window.h"

extern HWND g_hwndConsole;
extern WndConsole	*g_wndConsole;

class WndConsole : public Window
{
public:
	WndConsole();
	~WndConsole();

	void Initialize();
	int WindowProcedure(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void ScrollToEnd();
	static void AddText(const char* txt);	// static so sys_printfs can happen before the window exists
	bool TryCopy();
private:
	HWND w_hwndCons;
	int OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#endif
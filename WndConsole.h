//==============================
//	WndConsole.h
//==============================

#ifndef __WND_CONSOLE_H__
#define __WND_CONSOLE_H__

#include "Window.h"

extern HWND g_hwndConsole;

class WndConsole : public Window
{
public:
	WndConsole();
	~WndConsole();

	void Initialize();
	void ScrollToEnd();
    //int OnPaint();
    int OnResized();
	static void Print(const char* txt);	// static so Log::Prints can happen before the window exists
	bool TryCopy();
	void ForceUpdate();
private:
	HWND w_hwndCons;
	static std::string buf;
	void AddText();
	int OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

extern WndConsole	*g_wndConsole;

#endif
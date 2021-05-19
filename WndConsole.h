//==============================
//	WndConsole.h
//==============================

#ifndef __WND_CONSOLE_H__
#define __WND_CONSOLE_H__


class WndConsole : public WndView
{
public:
	WndConsole();
	~WndConsole();

	void Initialize();
	int WindowProcedure(UINT uMsg, WPARAM wParam, LPARAM lParam);
	static void AddText(const char* txt);	// static so sys_printfs can happen before the window exists
private:
	HWND w_hwndCons;
	int OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

};

#endif
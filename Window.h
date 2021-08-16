//==============================
//	Window.h
//==============================

#ifndef __WND_BASE_H__
#define __WND_BASE_H__

class View;
class Window
{
public:
	Window();
	virtual ~Window();

	static WNDCLASS wBaseWC;
	static std::vector<Window*> windex;
	static Window *wbInitializing;
	static LONG WINAPI BaseWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void Create();

	View	*v;
	HWND	wHwnd;
	int		instance, minWidth, minHeight, vbits;
	RECT	clientRect;
	bool	mouseWithin;

	inline void BringToTop()	{ BringWindowToTop(wHwnd); }
	inline void Focus()			{ SetFocus(wHwnd); BringToTop(); OnFocus(); }
	inline bool HasFocus()		{ return (GetFocus() == wHwnd); }
	inline bool IsOpen()		{ return IsWindowVisible(wHwnd) > 0; }
	inline bool IsOnTop()		{ return GetTopWindow(g_hwndMain) == wHwnd; }
	inline void Show()			{ ShowWindow(wHwnd, SW_SHOW); BringToTop(); OnShow(); }
	inline void Hide()			{ ShowWindow(wHwnd, SW_HIDE); }
	void Toggle();
	void Swap(Window &other);
	void CopyPositionFrom(Window &other);
	void SetPosition(int l, int t, int r, int b, bool show);
	virtual void ForceUpdate();

	void GetMsgXY(LPARAM l, int &x, int &y);
	void SavePosition();
	void RestorePosition();

protected:
	char *name;
	int	cursorX, cursorY;
	virtual int WindowProcedure(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual int OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual int OnCreate();
	virtual int OnDestroy();
	virtual int OnPaint();
	virtual int OnResized();
	virtual void OnShow() {};
	virtual void OnFocus() {};
	void MoveRect(HWND hwnd, RECT r);
	void MoveRect(HWND hwnd, int left, int top, int right, int bottom);
	struct wndPos_t {
		RECT rc;
		BOOL vis;
	};
	void SetPosition(wndPos_t pos);
	wndPos_t GetPosition();

	bool TryDocking(long side, LPRECT rect);
	void SetTitle(const char* title);

private:
	int FindClosestHorizontal(int h, LPRECT field);
	int FindClosestVertical(int v, LPRECT field);
	bool SnapToClosestHorizontal(LPRECT win, LPRECT field, bool down);
	bool SnapToClosestVertical(LPRECT win, LPRECT field, bool right);
	bool SnapMove(LPRECT win, LPRECT field);

};

#endif // __WND_BASE_H__

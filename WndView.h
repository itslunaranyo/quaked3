//==============================
//	WndView.h
//==============================

#ifndef __WND_VIEW_H__
#define __WND_VIEW_H__

class View;

class WndView
{
public:
	WndView();
	virtual ~WndView();

	virtual void Initialize(int winNum) { instance = winNum; };
	View *v;
	char *name;
	int instance, minWidth, minHeight, vbits;
	RECT clientRect;
	bool mouseWithin;

	HWND	w_hwnd;
	HDC		w_hdc;
	HGLRC	w_hglrc;

	static HDC		w_hdcBase;
	static HGLRC	w_hglrcBase;
	static WNDCLASS w_wcView;

	void CreateWnd();

	void Focus()		{ SetFocus(w_hwnd); BringToTop(); }
	void BringToTop()	{ BringWindowToTop(w_hwnd); }
	bool HasFocus()		{ return (GetFocus() == w_hwnd); }
	bool IsOnTop();
	virtual void ForceUpdate();

	typedef struct {
		RECT rc;
		BOOL vis;
	} wndViewPosition_s;

	void SavePosition();
	void RestorePosition();
	void SetPosition(int l, int t, int r, int b, bool show);

	void SetTitle(const char* title);
	bool Open();
	void Toggle();
	void Show();
	void Hide();
	void SetWindowRect(RECT *rc);
	void SetWindowRect(int x, int y, int w, int h);
	void Swap(WndView &other);
	virtual void GetMsgXY(LPARAM l, int &x, int &y);

	static std::vector<WndView*> wndviews;
	static WndView *wvbInitializing;
	static LONG WINAPI WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	virtual int WindowProcedure(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual int OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void OnCreate();
	virtual void OnDestroy();
	virtual void OnPaint();
	bool TryDocking(long side, LPRECT rect);
	void MoveRect(HWND hwnd, RECT r);
	void MoveRect(HWND hwnd, int left, int top, int right, int bottom);

private:
	void MakeFont();
	int SetupPixelFormat(HDC hDC, bool zbuffer);

	int FindClosestHorizontal(int h, LPRECT field);
	int FindClosestVertical(int v, LPRECT field);
	bool SnapToClosestHorizontal(LPRECT win, LPRECT field, bool down);
	bool SnapToClosestVertical(LPRECT win, LPRECT field, bool right);
	bool SnapMove(LPRECT win, LPRECT field);

};

#endif
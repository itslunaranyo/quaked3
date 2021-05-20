//==============================
//	WndGrid.h
//==============================

#ifndef __WND_GRID_H__
#define __WND_GRID_H__

#include "WindowGL.h"

extern HWND		g_hwndGrid[4];		// lunaran - grid view reunification
extern WndGrid	*g_wndGrid[4];
class GridView;
class GridViewRenderer;

class WndGrid : public WindowGL
{
public:
	WndGrid();
	~WndGrid();

	GridView *gv;

	void Initialize(int winNum);
	void SetAxis(eViewType_t axis);
	void SetAxis(int axis);
	void CycleAxis();
	void DoPopupMenu(int x, int y);
	int OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

	void	MouseDown(const int x, const int y, const int btndown, const int buttons);
	void	MouseUp(const int x, const int y, const int btnup, const int buttons);
	void	MouseMoved(const int x, const int y, const int buttons);
private:
	GridViewRenderer *gr;
	void Render();
	int OnResized();
	int	mDownX, mDownY;
	bool rMoved;

};


#endif
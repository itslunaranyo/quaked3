//==============================
//	WndGrid.h
//==============================

#ifndef __WND_GRID_H__
#define __WND_GRID_H__

#include "WndView.h"

class WndGrid : public WndView
{
public:
	WndGrid();
	~WndGrid();

	XYZView *xyzv;

	void Initialize(int winNum);
	void SetAxis(int viewAxis);
	void CycleAxis();
	void DoPopupMenu(int x, int y);
	int OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

};

#endif
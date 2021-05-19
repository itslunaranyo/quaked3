//==============================
//	ManipTool.h
//==============================

#ifndef __MANIP_TOOL_H__
#define __MANIP_TOOL_H__

// General Manipulation Tool
// translates, plane slides, new brush draws

class ManipTool : public Tool
{
public:
	ManipTool();
	~ManipTool();

	bool Input3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndView &vWnd);
	bool Input2D(UINT uMsg, WPARAM wParam, LPARAM lParam, XYZView &v, WndView &vWnd);
	bool Input1D(UINT uMsg, WPARAM wParam, LPARAM lParam, ZView &v, WndView &vWnd);

	bool Draw3D(CameraView &v);
	bool Draw2D(XYZView &v);

private:
	enum {
		NONE,
		DRAGNEW,
		DRAGMOVE,
		DRAGPLANE,
	} state;
	int xDown, yDown;
	int vDim1, vDim2, vType;	// up/right axis indexes of view w/capture
	Brush *brDragNew;

	void DragStart(int x, int y);
	void DragMove(int x, int y);
	void DragFinish(int x, int y);
};

#endif
//==============================
//	xy.h
//==============================

// window system independent xy view code
#ifndef __XYZVIEW_H__
#define __XYZVIEW_H__
#include "v_view.h"

class XYZView : public View
{
public:
	XYZView();
	~XYZView();

	HDC		hdc;
	HGLRC	hglrc;
	int		dViewType;	// current orientation of this view

	void	MouseDown(int x, int y, int buttons);
	void	MouseUp(int x, int y, int buttons);
	void	MouseMoved(int x, int y, int buttons);
	void	PositionView();
	static void PositionAllViews();

	void	ToPoint(int x, int y, vec3_t point);
	void	ToGridPoint(int x, int y, vec3_t point);
	void	SnapToPoint(int x, int y, vec3_t point);

	void	Draw();

private:
	int		nRotate;

	void	Init();
	void	CopyVector(vec3_t in, vec3_t out);

	void	DrawGrid ();
	void	DrawBlockGrid ();
	void	DrawCoords ();	// sikk - made separate function so coords and axis layed on top
	void	DrawCameraIcon ();
	void	DrawLightRadius (Brush *pBrush, int nViewType);
	void	DrawSizeInfo (int nDim1, int nDim2, vec3_t vMinBounds, vec3_t vMaxBounds);
	void	DrawRotateIcon ();	// sikk - Free Rotate: Pivot Icon
	void	DrawZIcon ();

	bool	DragDelta(int x, int y, vec3_t move);
	void	DragNewBrush(int x, int y);
};


//========================================================================



#endif

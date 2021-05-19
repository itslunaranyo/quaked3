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

	void	ToPoint(int x, int y, vec3 &point);
	void	ToGridPoint(int x, int y, vec3 &point);
	void	SnapToPoint(int x, int y, vec3 &point);
	bool	GetBasis(vec3 &right, vec3 &up, vec3 &forward);

	void	Draw();

private:
	int		nRotate;

	void	Init();
	void	CopyVector(const vec3 in, vec3 &out);

	void	DrawGrid ();
	void	DrawBlockGrid ();
	void	DrawCoords ();	// sikk - made separate function so coords and axis layed on top
	void	DrawCameraIcon ();
	void	DrawLightRadius (Brush *pBrush, int nViewType);
	void	DrawSizeInfo (int nDim1, int nDim2, const vec3 vMinBounds, const vec3 vMaxBounds);
	void	DrawRotateIcon ();	// sikk - Free Rotate: Pivot Icon
	void	DrawZIcon ();

	bool	DragDelta(int x, int y, vec3 move);
	void	DragNewBrush(int x, int y);
};


//========================================================================



#endif

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

	void	SetAxis(int newViewType);
	int		GetAxis() { return dViewType; }
	bool	GetBasis(vec3 &right, vec3 &up, vec3 &forward);
	void	GetDims(int &d1, int &d2, int &vType) { d1 = nDim1; d2 = nDim2; vType = dViewType; }

	void	MouseDown(int x, int y, int buttons);
	void	MouseUp(int x, int y, int buttons);
	void	MouseMoved(int x, int y, int buttons);
	void	PositionView();
	static void PositionAllViews();

	void	const ToPoint(const int x, const int y, vec3 &point);
	void	const ToPoint(const int xIn, const int yIn, int &xOut, int &yOut);
	void	const ToGridPoint(const int x, const int y, vec3 &point);
	void	const ToGridPoint(const int xIn, const int yIn, int &xOut, int &yOut);
	void	const SnapToPoint(const int x, const int y, vec3 &point);
	vec3	const SnapPoint(const vec3 ptIn);

	mouseContext_t	const GetMouseContext(const int x, const int y);
	void	Draw();
	void	DrawSelection();
	void	BeginDrawSelection();
	void	EndDrawSelection();
	bool	CullBrush(Brush *b);
	void	DrawSizeInfo (const vec3 vMinBounds, const vec3 vMaxBounds);

private:
	int		nRotate;
	int		dViewType;	// current orientation of this view
	int		nDim1, nDim2;
			// XY - dim1: 0 dim2: 1
			// YZ - dim1: 1 dim2: 2
			// XZ - dim1: 0 dim2: 2
	vec3	vMins, vMaxs;

	void	Init();
	void	SetBounds();
	void	CopyVector(const vec3 in, vec3 &out);

	void	DrawGrid ();
	void	DrawBlockGrid ();
	void	DrawCoords ();	// sikk - made separate function so coords and axis layed on top
	void	DrawCameraIcon ();
	void	DrawLightRadius (Brush *pBrush, int nViewType);
	void	DrawRotateIcon ();	// sikk - Free Rotate: Pivot Icon
	void	DrawZIcon ();
	bool	DrawTools();

	bool	DragDelta(int x, int y, vec3 move);
	//void	DragNewBrush(int x, int y);
};


//========================================================================



#endif

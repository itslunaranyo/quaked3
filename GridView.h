//==============================
//	GridView.h
//==============================

// window system independent xy view code
#ifndef __GRIDVIEW_H__
#define __GRIDVIEW_H__

#include "DisplayView.h"

class GridView : public DisplayView
{
public:
	GridView();
	~GridView();

	eViewType_t GetAxis() { return viewType; }
	void	SetAxis(eViewType_t newViewType);

	// XY - dimU: 0 dimV: 1
	// YZ - dimU: 1 dimV: 2
	// XZ - dimU: 0 dimV: 2
	inline int const DimU() { const int dims[] = { 1,0,0 }; return dims[viewType]; }
	inline int const DimV() { const int dims[] = { 2,2,1 }; return dims[viewType]; }

	bool const GetBasis(vec3 &right, vec3 &up, vec3 &forward);
	void	GetDims(int &d1, int &d2, int &vType) { d1 = DimU(); d2 = DimV(); vType = viewType; }
	inline float const GetScale() { return scale; }
	inline vec3	const GetMins() { return vMins; }
	inline vec3	const GetMaxs() { return vMaxs; }
	mouseContext_t	const GetMouseContext(const int x, const int y);

	void	PositionView();
	void	ZoomIn();
	void	ZoomIn(vec3 tgt);
	void	ZoomOut();
	void	ZoomOut(vec3 tgt);
	void	Zoom(vec3 tgt, float zm);
	void	ResetZoom();
	void	Center(vec3 tgt);
	void	Shift(vec3 mv);
	void	Shift(float mvU, float mvV);
	void	Resize(const int w, const int h);

	void	const ScreenToWorld(const int x, const int y, vec3 &point);
	void	const ScreenToWorld(const int xIn, const int yIn, int &xOut, int &yOut);
	void	const ScreenToWorldSnapped(const int x, const int y, vec3 &point);
	void	const ScreenToWorldSnapped(const int xIn, const int yIn, int &xOut, int &yOut);
	void	const ScreenToWorldGrid(const int x, const int y, vec3 &point);

	bool	CullBrush(Brush *b);
	void	CopyVectorPlanar(const vec3 in, vec3 &out);
	void	AngleCamera(vec3 point);

	static GridView* FromHwnd(HWND xyzwin);

private:
	eViewType_t viewType;
	float scale;	// pixels per unit
	vec3	origin, vMins, vMaxs;

	void	Init();
	void	Scale(float sc);
	void	ScaleUp();
	void	ScaleDown();
	void	SetBounds();

};


//========================================================================

extern GridView	g_vGrid[4];

#endif

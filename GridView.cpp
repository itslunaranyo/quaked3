//==============================
//	GridView.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "GridView.h"
#include "map.h"
#include "select.h"
#include "WndGrid.h"
#include "CameraView.h"
#include "WndZChecker.h"
#include "Tool.h"

bool	g_bSnapCheck;
GridView	g_vGrid[4];


GridView::GridView() : scale(1.0f), viewType(GRID_XY), origin(vec3(0))
{
	Init();
}

GridView::~GridView()
{
}

/*
============
GridView::FromHwnd
for tools to quickly sort input messages between class-identical windows
============
*/
GridView* GridView::FromHwnd(HWND xyzwin)
{
	for (int i = 0; i < 4; i++)
	{
		if (g_hwndGrid[i] == xyzwin)
			return &g_vGrid[i];
	}

	return nullptr;
}

/*
============
GridView::Init
============
*/
void GridView::Init()
{
	SetBounds();
}

/*
============================================================================

	ORIENTATION/SIZING

2D operations remain orientation agnostic without branching by continually 
referencing the DimU()th and DimV()th elements of a vector. U/V are the 
horizontal and vertical axes in view space, and DimU/DimV/viewType correspond 
to the array indices of the world axis that matches U, V, or into/out of the 
screen.

============================================================================
*/

/*
==================
GridView::SetAxis
==================
*/
void GridView::SetAxis(eViewType_t newViewType)
{
	assert(newViewType >= 0 && newViewType <= 2);
	viewType = newViewType;

	PositionView();
}

/*
==================
GridView::PositionView
==================
*/
void GridView::PositionView()
{
	Brush	   *b;

	b = g_brSelectedBrushes.next;
	if (b && b->next != b)
	{
		Center(b->mins + (b->maxs - b->mins) * 0.5f);
	}
	else
	{
		Center(g_vCamera.GetOrigin());
	}
	SetBounds();
}

/*
============
GridView::Center
============
*/
void GridView::Center(vec3 tgt)
{
	Shift(tgt - origin);
}

/*
============
GridView::Shift
============
*/
void GridView::Shift(vec3 delta)
{
	delta[viewType] = 0;

	origin += delta;
	vMins += delta;
	vMaxs += delta;
}

void GridView::Shift(float mvU, float mvV)
{
	vec3 mv;
	mv[DimU()] = mvU;
	mv[DimV()] = mvV;
	mv[viewType] = 0;
	Shift(mv);
}

/*
============
GridView::ZoomIn
============
*/
// ins/del: stepped zoom, no center
void GridView::ZoomIn()
{
	ScaleUp();
	SetBounds();
}

// mwheel: stepped zoom, w/center
void GridView::ZoomIn(vec3 tgt)
{
	vec3 cenOld, cenNew;
	cenOld = (tgt - vMins) / (vMaxs - vMins) - vec3(0.5f);
	ScaleUp();
	SetBounds();
	cenNew = (tgt - vMins) / (vMaxs - vMins) - vec3(0.5f);
	Shift((cenNew - cenOld) * (vMaxs - vMins));
}

/*
============
GridView::ZoomOut
============
*/
// ins/del: stepped zoom, no center
void GridView::ZoomOut()
{
	ScaleDown();
	SetBounds();
}

// mwheel: stepped zoom, w/center
void GridView::ZoomOut(vec3 tgt)
{
	vec3 cenOld, cenNew;
	cenOld = (tgt - vMins) / (vMaxs - vMins) - vec3(0.5f);
	ScaleDown();
	SetBounds();
	cenNew = (tgt - vMins) / (vMaxs - vMins) - vec3(0.5f);
	Shift((cenNew - cenOld) * (vMaxs - vMins));
}

// rmb drag: gradual zoom, w/center
void GridView::Zoom(vec3 tgt, float zm)
{
	vec3 cenOld, cenNew;
	cenOld = (tgt - vMins) / (vMaxs - vMins) - vec3(0.5f);
	Scale(zm);
	SetBounds();
	cenNew = (tgt - vMins) / (vMaxs - vMins) - vec3(0.5f);
	Shift((cenNew - cenOld) * (vMaxs - vMins));
}

void GridView::ResetZoom()
{
	scale = 1.0f;
	SetBounds();
}

void GridView::ScaleUp() { Scale(1.25f); }	// this is zooming IN
void GridView::ScaleDown() { Scale(0.8f); }	// this is zooming OUT

void GridView::Scale(float sc)
{
	scale = min(max(scale * sc, 0.04f), 32.0f);
}

/*
============
GridView::Resize
============
*/
void GridView::Resize(const int w, const int h)
{
	width = w;
	height = h;
	SetBounds();
}

/*
==============
GridView::SetBounds
==============
*/
void GridView::SetBounds()
{
	float		w, h;
	w = width / 2 / GetScale();
	h = height / 2 / GetScale();

	vMins[DimU()] = origin[DimU()] - w;
	vMaxs[DimU()] = origin[DimU()] + w;
	vMins[DimV()] = origin[DimV()] - h;
	vMaxs[DimV()] = origin[DimV()] + h;
	vMins[viewType] = -g_cfgEditor.MapSize;
	vMaxs[viewType] = g_cfgEditor.MapSize;
}

/*
============================================================================

	CONVERSIONS

============================================================================
*/

/*
==================
GridView::CopyVectorPlanar
==================
*/
void GridView::CopyVectorPlanar(const vec3 in, vec3 &out)
{
	out[DimU()] = in[DimU()];
	out[DimV()] = in[DimV()];
}

/*
==================
GridView::ScreenToWorld
window client relative pixels to map units (y inverted)
==================
*/
void const GridView::ScreenToWorld(const int x, const int y, vec3 &point)
{
	float u, v;
	vec3 size = vMaxs - vMins;
	u = (float)x / width;
	v = 1 - (float)y / height;

	point[DimU()] = vMins[DimU()] + size[DimU()] * u;
	point[DimV()] = vMaxs[DimV()] - size[DimV()] * v;
	return;
}

/*
==================
GridView::ScreenToWorld
window client relative pixels to map units (y inverted)
==================
*/
void const GridView::ScreenToWorld(const int xIn, const int yIn, int &xOut, int &yOut)
{
	float u, v;
	vec3 size = vMaxs - vMins;
	u = (float)xIn / width;
	v = 1 - (float)yIn / height;
	xOut = vMins[DimU()] + size[DimU()] * u;
	yOut = vMaxs[DimV()] - size[DimV()] * v;
	return;
}

/*
==================
GridView::ScreenToWorld
window client relative pixels to map units, on current grid (y inverted)
==================
*/
void const GridView::ScreenToWorldSnapped(const int x, const int y, vec3 &point)
{
	ScreenToWorld(x, y, point);
	point[DimU()] = qround(point[DimU()], g_qeglobals.d_nGridSize);
	point[DimV()] = qround(point[DimV()], g_qeglobals.d_nGridSize);
	return;
}

/*
==================
GridView::ScreenToWorld
window client relative pixels to map units, on current grid (y inverted)
==================
*/
void const GridView::ScreenToWorldSnapped(const int xIn, const int yIn, int &xOut, int &yOut)
{
	ScreenToWorld(xIn, yIn, xOut, yOut);
	xOut = qround(xOut, g_qeglobals.d_nGridSize);
	yOut = qround(yOut, g_qeglobals.d_nGridSize);
	return;
}

/*
==================
GridView::ScreenToWorldGrid

window client relative pixels to map units, on current grid if snap is on (y inverted)

lunaran TODO: return bool on all these snapTo functions to throw back a false
if the point snapped didn't actually move (ie was snapped to own position)
==================
*/
void const GridView::ScreenToWorldGrid(const int x, const int y, vec3 &point)
{
	if (!g_qeglobals.bGridSnap)
		ScreenToWorld(x, y, point);
	else
		ScreenToWorldSnapped(x, y, point);
}

/*
==================
GridView::GetBasis
==================
*/
bool const GridView::GetBasis(vec3 &right, vec3 &up, vec3 &forward)
{
	right = vec3(0);
	up = vec3(0);
	forward = vec3(0);

	forward[viewType] = -1;
	right[DimU()] = 1;
	up[DimV()] = 1;

	return true;
}

/*
==================
GridView::GetMouseContext
==================
*/
mouseContext_t const GridView::GetMouseContext(const int x, const int y)
{
	mouseContext_t mc;

	ScreenToWorld(x, y, mc.org);

	// the screen axis in XY and YZ points out of the screen, but in XZ points in
	float rays[] = { 1, -1, 1 };

	mc.pt = vec3(x, y, 0);
	mc.org[viewType] = rays[viewType] * g_cfgEditor.MapSize / 2;
	mc.ray[viewType] = -rays[viewType];
	mc.right[DimU()] = 1;
	mc.up[DimV()] = 1;
	mc.dims = 2;	// 2D view

	return mc;
}

/*
==============
GridView::AngleCamera
==============
*/
void GridView::AngleCamera(vec3 point)
{
	float ang;
	point -= g_vCamera.GetOrigin();

	if (point[DimV()] || point[DimU()])
	{
		ang = 180 / Q_PI * atan2(point[DimV()], point[DimU()]);
		if (viewType == GRID_XY)
			g_vCamera.Turn(ang, true);
		else
			g_vCamera.Pitch(ang, true);
		WndMain_UpdateWindows(W_SCENE);
	}
	return;
}

/*
==============
GridView::CullBrush
==============
*/
bool GridView::CullBrush(Brush *b)
{
	return (b->mins[DimU()] > vMaxs[DimU()] ||
			b->maxs[DimU()] < vMins[DimU()] ||
			b->mins[DimV()] > vMaxs[DimV()] ||
			b->maxs[DimV()] < vMins[DimV()]);
}


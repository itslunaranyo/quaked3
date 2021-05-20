//==============================
//	ZView.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "map.h"
#include "select.h"
#include "CameraView.h"
#include "ZView.h"
#include "Tool.h"

ZView g_vZ;

ZView::ZView() : scale(1.0f), origin(vec3(0))
{
	Init();
}
ZView::~ZView() {}

/*
============
ZView::Init
============
*/
void ZView::Init()
{
}

/*
==================
ZView::ScreenToWorld
==================
*/
void ZView::ScreenToWorld(int x, int y, vec3 &point)
{
	point = origin;
	point.z = origin.z + (y - height / 2) / scale;
}

/*
==================
ZView::ScreenToWorldSnapped
==================
*/
void ZView::ScreenToWorldSnapped(int x, int y, vec3 &point)
{
	ScreenToWorld(x, y, point);
	point.z = floor(point.z / g_qeglobals.d_nGridSize + 0.5) * g_qeglobals.d_nGridSize;
}

/*
==================
ZView::GetMouseContext
==================
*/
mouseContext_t const ZView::GetMouseContext(const int x, const int y)
{
	mouseContext_t mc;

	ScreenToWorld(x, y, mc.org);

	// .ray and .right are 0,0,0 because their direction is meaningless

	mc.pt = vec3(x, y, 0);
	mc.up = vec3(0,0,1);
	mc.dims = 1;	// 1D view

	return mc;
}

void ZView::Scroll(float amt)
{
	origin.z += amt / scale;
}

/*
============
ZView::Resize
============
*/
void ZView::Resize(const int w, const int h)
{
	width = w;
	height = h;
}

/*
============
ZView::Scale
============
*/
void ZView::Scale(float sc)
{
	scale = min(max(scale * sc, 0.1f), 16.0f);
}
void ZView::ScaleUp()
{
	scale = min(32.0f, scale * 1.25f);
}
void ZView::ScaleDown()
{
	scale = max(0.05f, scale * 0.8f);
}

void ZView::ResetScale()
{
	scale = 1.0f;
}

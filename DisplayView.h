#ifndef __DISPLAYVIEW_H__
#define __DISPLAYVIEW_H__
//==============================
//	DisplayView.h
//==============================

// somewhat window system dependent view code

#include "View.h"

struct mouseContext_t {
	mouseContext_t() : pt(vec3(0)), up(vec3(0)), right(vec3(0)), ray(vec3(0)), org(vec3(0)), dims(0) {}
	vec3 pt;
	vec3 up;
	vec3 right;
	vec3 ray;
	vec3 org;
	int dims;
};

class DisplayView : public View
{
public:
	DisplayView();
	virtual ~DisplayView();

	virtual void Resize(const int w, const int h) { width = w; height = h; }
	inline int const GetWidth() { return width; }
	inline int const GetHeight() { return height; }

	virtual bool const GetBasis(vec3 &_right, vec3 &_up, vec3 &_forward) { return false; }
	virtual mouseContext_t const GetMouseContext(const int x, const int y) { return mouseContext_t(); }

protected:
	int width, height;
};

#endif
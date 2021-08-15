//==============================
//	ZView.h
//==============================
#ifndef __ZVIEW_H__
#define __ZVIEW_H__

// window system independent z view code
#include "DisplayView.h"

class ZView : public DisplayView
{
public:
	ZView();
	~ZView();

	inline vec3 GetOrigin() { return origin; }
	inline float GetScale() { return scale; }

	void	SetOrigin(vec3 norg) { origin = norg; }
	void	ScaleUp();
	void	ScaleDown();
	void	ResetScale();
	void	Scroll(float amt);
    void	PositionCenter();
	void	Resize(const int w, const int h);
	void	Scale(float sc);
	void	ScreenToWorld(int x, int y, vec3 &point);
	void	ScreenToWorldSnapped(int x, int y, vec3 &point);
	mouseContext_t const GetMouseContext(const int x, const int y);
private:
	float	scale;
	vec3	origin;

	void	Init();

};

//========================================================================

extern 	ZView g_vZ;

#endif

//==============================
//	z.h
//==============================
#ifndef __ZVIEW_H__
#define __ZVIEW_H__

// window system independent z view code
#include "View.h"

class ZView : public View
{
public:
	ZView();
	~ZView();

	void	MouseDown(const int x, const int y, const int buttons);
	void	MouseUp(const int x, const int y, const int buttons);
	void	MouseMoved(const int x, const int y, const int buttons);
	void	ScaleUp();
	void	ScaleDown();
	void	Scroll(float amt);
	void	Draw();
	void	DrawBrush(Brush *brush, vec3 selColor);
	bool	DrawTools();
	void	DrawSelection(vec3 selColor);

	void	ToPoint(int x, int y, vec3 &point);
	mouseContext_t const GetMouseContext(const int x, const int y);

private:
	struct zbr_t {
		float top, bottom;
		Brush* brush;
		Texture* tex;
		zbr_t() {};
		zbr_t(Brush &br);
	};

	void	Init();
	void	DrawGrid();
	void	DrawCameraIcon();
	void	DrawCoords();
	bool	TestBrush(zbr_t &zbr);
};

//========================================================================

extern 	ZView g_vZ;

#endif

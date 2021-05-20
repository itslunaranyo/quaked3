#ifndef __R_ZVIEW_H__
#define __R_ZVIEW_H__
//==============================
//	ZViewRenderer.h
//==============================

#include "Renderer.h"

class ZView;
class DisplayView;

class ZViewRenderer : public Renderer
{
public:
	ZViewRenderer(ZView &view);
	~ZViewRenderer();

	void	Draw();
	void	DrawSelection(vec3 selColor);
	bool	DrawTools();
	void	DrawBrush(Brush *brush, vec3 selColor);
	DisplayView* GetDisplayView() { return (DisplayView*)&zv; }

private:
	ZView &zv;

	struct zbr_t {
		float top, bottom;
		Brush* brush;
		Texture* tex;
		zbr_t() {};
		zbr_t(Brush &br);
	};

	bool	TestBrush(zbr_t &zbr);
	void	DrawGrid();
	void	DrawCameraIcon();
	void	DrawCoords();

};

#endif
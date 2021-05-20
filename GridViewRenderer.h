#ifndef __R_GRIDVIEW_H__
#define __R_GRIDVIEW_H__
//==============================
//	GridViewRenderer.h
//==============================

#include "Renderer.h"

class GridView;
class DisplayView;

class GridViewRenderer : public Renderer
{
public:
	GridViewRenderer(GridView &view);
	~GridViewRenderer();

	void	Draw();
	void	DrawSelection(vec3 selColor);
	void	BeginDrawSelection(vec3 selColor);
	void	EndDrawSelection();
	void	DrawBrushSelected(Brush* br);
	DisplayView* GetDisplayView() { return (DisplayView*)&gv; }

private:
	GridView &gv;
	void	DrawSizeInfo(const vec3 vMinBounds, const vec3 vMaxBounds);
	void	DrawGrid();
	void	DrawBlockGrid();
	void	DrawViewName();
	void	DrawCoords();
	void	DrawCameraIcon();
	void	DrawLightRadius(Brush *pBrush, int nViewType);
	void	DrawZIcon();
	bool	DrawTools();

	vec3 vMins, vMaxs;
};

#endif
//==============================
//	PolyTool.h
//==============================

#ifndef __POLY_TOOL_H__
#define __POLY_TOOL_H__

#include "Tool.h"
// Draw Polygonal Brushes Tool

class CmdPolyBrushConcave;

class PolyTool : public Tool
{
public:
	PolyTool();
	~PolyTool();

	bool Input3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndCamera &vWnd);
	bool Input2D(UINT uMsg, WPARAM wParam, LPARAM lParam, GridView &v, WndGrid &vWnd);
	bool Input(UINT uMsg, WPARAM wParam, LPARAM lParam);

	bool Draw3D(CameraRenderer &v);
	bool Draw2D(GridViewRenderer &v);

private:
	CmdPolyBrushConcave* pcmdPBC;

	std::vector<vec3> pointList;
	typedef std::vector<vec3>::iterator pointIt;
	pointIt ptMoving;
	int axis;
	bool valid;

	bool PointMoving() { return (ptMoving != pointList.end()); }
	void Reset();
	bool Commit();
	bool InputCommand(WPARAM w);

	PolyTool::pointIt XYGetNearestPoint(GridView * xyz, int x, int y);

	void AddPoint(GridView * xyz, int x, int y, bool back = false);
	void MovePoint(GridView * xyz, int x, int y);
	void EndPoint();
	void DeletePoint(bool first = false);
	bool PointsUpdated();

	void DrawSetColor();

	void DrawLoop(int plane);
	void DrawPegs();
	void DrawPoints();
};


#endif

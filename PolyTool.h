//==============================
//	PolyTool.h
//==============================

#ifndef __POLY_TOOL_H__
#define __POLY_TOOL_H__

// Draw Polygonal Brushes Tool

class CmdPolyBrushConcave;

class PolyTool : public Tool
{
public:
	PolyTool();
	~PolyTool();

	bool Input3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndView &vWnd);
	bool Input2D(UINT uMsg, WPARAM wParam, LPARAM lParam, XYZView &v, WndView &vWnd);
	bool Input(UINT uMsg, WPARAM wParam, LPARAM lParam);

	bool Draw3D(CameraView &v);
	bool Draw2D(XYZView &v);

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

	PolyTool::pointIt XYGetNearestPoint(XYZView * xyz, int x, int y);

	void AddPoint(XYZView * xyz, int x, int y);
	void MovePoint(XYZView * xyz, int x, int y);
	void EndPoint();
	void DeleteLastPoint();
	bool PointsUpdated();

	void DrawSetColor();

	void DrawLoop(int plane);
	void DrawPegs();
	void DrawPoints();
};


#endif

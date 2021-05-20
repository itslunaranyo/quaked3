//==============================
//	ClipTool.h
//==============================

#ifndef __CLIP_TOOL_H__
#define __CLIP_TOOL_H__

#include "Tool.h"
// 3-Point Clipping Tool

class CmdBrushClip;
class CameraRenderer;
class GridViewRenderer;

class ClipTool : public Tool
{
public:
	ClipTool();
	~ClipTool();

	bool Input3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndCamera &vWnd);
	bool Input2D(UINT uMsg, WPARAM wParam, LPARAM lParam, GridView &v, WndGrid &vWnd);
	bool Input(UINT uMsg, WPARAM wParam, LPARAM lParam);

	bool Draw3D(CameraRenderer &rc);
	bool Draw2D(GridViewRenderer &v);

	void SelectionChanged();

private:
	typedef struct
	{
		bool	set;
		vec3	point;      // the 3d point
		void Reset() { set = false; point = vec3(0); }
	} clippoint_t;

	//========================================================================

	clippoint_t	points[3];
	clippoint_t ptHover;
	clippoint_t *ptMoving;
	CmdBrushClip *g_pcmdBC;
	int axis;
	bool backside;

	bool InputCommand(WPARAM w);

	void Reset();
	vec3 AxisForClip();
	void PointsUpdated();
	void Clip();
	void Split();
	void Flip();

	void StartCommand();
	clippoint_t* StartNextPoint();

	void CamStartQuickClip(int x, int y);
	void CamEndQuickClip();
	void StartQuickClip(GridView* xyz, int x, int y);
	void EndQuickClip();

	bool CamPointOnSelection(int x, int y, vec3 & out);
	void CamDropPoint(int x, int y);
	clippoint_t* CamGetNearestClipPoint(int x, int y);
	void CamMovePoint(int x, int y);
	void CamEndPoint();

	void DropPoint(GridView* xyz, int x, int y);
	void GetCoordXY(int x, int y, vec3 & pt);
	clippoint_t* XYGetNearestClipPoint(GridView * xyz, int x, int y);
	void MovePoint(GridView* xyz, int x, int y);
	void EndPoint();

	void Draw();
	void DrawPoints();
	void DrawClipWire(std::vector<Brush*> *brList, vec3 color);
};


#endif

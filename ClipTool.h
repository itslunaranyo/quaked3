//==============================
//	ClipTool.h
//==============================

#ifndef __CLIP_H__
#define __CLIP_H__

// 3-Point Clipping Tool

#include "Tool.h"

class ClipTool : public Tool
{
public:
	ClipTool();
	~ClipTool();

	bool Input3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndView &vWnd);
	bool Input2D(UINT uMsg, WPARAM wParam, LPARAM lParam, XYZView &v, WndView &vWnd);
	bool Input(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void Draw();
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
	clippoint_t *ptMoving;
	CmdBrushClip *g_pcmdBC;
	int axis;
	bool backside;

	bool InputCommand(WPARAM w);

	void Reset();
	void PointsUpdated();
	void Clip();
	void Split();
	void Flip();

	void Crosshair(bool bCrossHair);
	void StartCommand();
	clippoint_t* StartNextPoint();

	void CamStartQuickClip(int x, int y);
	void CamEndQuickClip();
	void StartQuickClip(XYZView* xyz, int x, int y);
	void EndQuickClip();

	bool CamPointOnSelection(int x, int y, vec3 & out, int * outAxis);
	void CamDropPoint(int x, int y);
	clippoint_t* CamGetNearestClipPoint(int x, int y);
	void CamMovePoint(int x, int y);
	void CamEndPoint();

	void DropPoint(XYZView* xyz, int x, int y);
	void GetCoordXY(int x, int y, vec3 & pt);
	clippoint_t* XYGetNearestClipPoint(XYZView * xyz, int x, int y);
	void MovePoint(XYZView* xyz, int x, int y);
	void EndPoint();
};



//========================================================================

void SnapToPoint(vec3 &point);

#endif

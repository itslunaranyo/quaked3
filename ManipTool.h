//==============================
//	ManipTool.h
//==============================

#ifndef __MANIP_TOOL_H__
#define __MANIP_TOOL_H__

// General Manipulation Tool
// translates, plane slides, new brush draws

class ManipTool : public Tool
{
public:
	ManipTool();
	~ManipTool();

	bool Input3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndView &vWnd);
	bool Input2D(UINT uMsg, WPARAM wParam, LPARAM lParam, XYZView &v, WndView &vWnd);
	bool Input1D(UINT uMsg, WPARAM wParam, LPARAM lParam, ZView &v, WndView &vWnd);

	bool Draw3D(CameraView &v);
	bool Draw2D(XYZView &v);
	bool Draw1D(ZView &v);

private:
	enum {
		NONE,
		DRAGNEW,
		DRAGMOVE,
		DRAGPLANE,
	} state;
	Plane mousePlane;	// implied plane for intersecting 3D view mouse events
	vec3 ptDown;

	Brush *brDragNew;
	CmdPlaneShift *cmdPS;
	CmdTranslate *cmdTr;

	void DragStart1D(const mouseContext_t & mc);
	void DragStart(const mouseContext_t &vc);
	void DragMove(const mouseContext_t &vc);
	void DragFinish(const mouseContext_t &vc);

};

#endif
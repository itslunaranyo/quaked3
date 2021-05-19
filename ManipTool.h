//==============================
//	ManipTool.h
//==============================

#ifndef __MANIP_TOOL_H__
#define __MANIP_TOOL_H__

// General Manipulation Tool
// translates, plane slides, new brush draws, quick skews

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
		MT_OFF,
		MT_NEW,
		MT_TRANSLATE,
		MT_PLANESHIFT,
		MT_SHEAR
	} state;
	Plane mousePlane;	// implied plane for intersecting 3D view mouse events
	vec3 ptDown;

	Brush *brDragNew;
	CmdGeoMod *cmdGM;
	CmdPlaneShift *cmdPS;
	CmdTranslate *cmdTr;

	void DragStart3D(const mouseContext_t &mc);
	void DragStart2D(const mouseContext_t &mc, int vDim);
	void DragStart1D(const mouseContext_t &mc);
	void DragMove(const mouseContext_t &mc, vec3 point);
	void DragFinish(const mouseContext_t &mc);

	void StartTranslate();
	void StartQuickShear(std::vector<Face*>& fSides);
	void StartPlaneShift(std::vector<Face*>& fSides);
	void SideSelectFaces(const vec3 org, const vec3 ray, std::vector<Face*> &fSides);
	void SideSelectShearFaces(const vec3 org, const vec3 ray, std::vector<Face*>& fSides);
	void SideSelectBackFaces(std::vector<Face*>& fSides);
	void FrontSelectShearFaces(const Face * hit, std::vector<Face*>& fSides);
};

#endif
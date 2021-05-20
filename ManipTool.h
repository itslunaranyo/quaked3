//==============================
//	ManipTool.h
//==============================

#ifndef __MANIP_TOOL_H__
#define __MANIP_TOOL_H__

// General Manipulation Tool
// translates, plane slides, new brush draws, quick skews

class CmdGeoMod;
class CmdPlaneShift;
class CmdTranslate;
class CmdCompound;

class ManipTool : public Tool
{
public:
	ManipTool();
	~ManipTool();

	bool Input3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndView &vWnd);
	bool Input2D(UINT uMsg, WPARAM wParam, LPARAM lParam, XYZView &v, WndView &vWnd);
	bool Input1D(UINT uMsg, WPARAM wParam, LPARAM lParam, ZView &v, WndView &vWnd);
	bool Input(UINT uMsg, WPARAM wParam, LPARAM lParam);

	void SelectionChanged();

	bool Draw3D(CameraView &v);
	bool Draw2D(XYZView &v);
	bool Draw1D(ZView &v);

private:
	enum {
		MT_OFF,
		MT_NEW,
		MT_TRANSLATE,
		MT_CLONE,
		MT_PLANESHIFT,
		MT_SHEAR
	} state;
	Plane mousePlane;	// implied plane for intersecting 3D view mouse events
	vec3 ptDown;
	bool cloneReady;
	clock_t lastNudgeTime;

	Brush *brDragNew;
	CmdGeoMod *cmdGM;
	CmdPlaneShift *cmdPS;
	CmdTranslate *cmdTr, *lastNudge;
	CmdCompound *cmdCmpClone;

	void DragStart3D(const mouseContext_t &mc);
	void DragStart2D(const mouseContext_t &mc, int vDim);
	void DragStart1D(const mouseContext_t &mc);
	void DragMove(const mouseContext_t &mc, vec3 point);
	void DragFinish();
	//void DragFinish(const mouseContext_t &mc);

	void Nudge(int nudge, vec3 right, vec3 up);

	void SetupTranslate();
	void StartTranslate();
	void StartQuickShear(std::vector<Face*>& fSides);
	void StartPlaneShift(std::vector<Face*>& fSides);
	void SideSelectFaces(const vec3 org, const vec3 ray, std::vector<Face*> &fSides);
	void SideSelectShearFaces(const vec3 org, const vec3 ray, std::vector<Face*>& fSides);
	void SideSelectBackFaces(std::vector<Face*>& fSides);
	void FrontSelectShearFaces(const Face * hit, std::vector<Face*>& fSides);
};

#endif
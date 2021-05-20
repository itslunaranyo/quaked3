//==============================
//	GeoTool.h
//==============================

#ifndef __GEO_TOOL_H__
#define __GEO_TOOL_H__

#include "Tool.h"
#include "View.h"

// Geometry Modification Tool
// vertex/edge/face editing

class CmdGeoMod;
class WndView;

class GeoTool :	public Tool
{
public:
	enum gt_mode_t {
		GT_INVALID = 0x00,
		GT_VERTEX = 0x01,
		GT_EDGE = 0x02,
		GT_FACE = 0x04,
		GT_VE = GT_VERTEX | GT_EDGE,
		GT_VF = GT_VERTEX | GT_FACE,
		GT_EF = GT_FACE | GT_EDGE,
		GT_VEF = GT_FACE | GT_VERTEX | GT_EDGE
	};
	unsigned mode;

	GeoTool(gt_mode_t gtm);
	~GeoTool();

	static void ToggleMode(gt_mode_t gtm);

	void SelectionChanged();
	bool Input3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndView &vWnd);
	bool Input2D(UINT uMsg, WPARAM wParam, LPARAM lParam, XYZView &v, WndView &vWnd);
	bool Input(UINT uMsg, WPARAM wParam, LPARAM lParam);

	bool Draw3D(CameraView &v);
	bool Draw2D(XYZView &v);

private:
	void _toggleMode(gt_mode_t gtm);

	CmdGeoMod *cmdGM;
	Plane mousePlane;	// implied plane for intersecting 3D view mouse events
	vec3 ptDownWorld, trans, snapTrans;
	mouseContext_t mcDown, mcCurrent;
	View *hotView;

	enum {
		GT_NONE,
		GT_BOXSEL,
		GT_MOVE
	} state;

	struct handle {
		handle() : origin(0), points(nullptr), numPoints(0), selected(false), brSrc(nullptr) {}
		handle(vec3 org, vec3* pts, int np, Brush* brs) : origin(org), points(pts), numPoints(np), selected(false), brSrc(brs) {}
		vec3 origin;	// location of the handle itself
		int numPoints;	// length of points[]
						// vertex/edge/face status is derived from numPoints
		bool selected;
		vec3* points;	// list of vertex coordinates associated with handle
		Brush* brSrc;	// brush originally associated with this handle (pre-sort)
		std::vector<Brush*> brAffected;	// all brushes associated with this handle (post-sort)
	};
	std::vector<handle> handles;
	std::vector<handle*> handlesHit;
	vec3 *pointBuf;

	void DragStart(const mouseContext_t &mca, const mouseContext_t &mcb, const vec3 up);
	void DragMove(const mouseContext_t &mc);
	void DragFinish(const mouseContext_t &mc);
	void Hover(const mouseContext_t &mca, const mouseContext_t &mcb, const vec3 up);

	void DoSelect(std::vector<handle*> &hlist);
	bool BoxTestHandles(const vec3 org1, const vec3 dir1, const vec3 org2, const vec3 dir2, const vec3 up, std::vector<handle*>& hlist);
	void SelectHandles(std::vector<handle*> &hlist);
	void DeselectHandles(std::vector<handle*> &hlist);
	bool AllSelected(std::vector<handle*> &hlist);
	bool AnySelected(std::vector<handle*>& hlist);
	void DeselectAllHandles();
	void GenerateHandles();
	void CollapseHandles();

	static bool handlecmp(const handle &a, const handle &b);
	void SortHandles();
	void DrawSelectionBox();
	void DrawPoints();
	void DrawPointSet(std::vector<handle>::iterator & start, std::vector<handle>::iterator &end, vec3 color);
};

#endif
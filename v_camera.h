#ifndef __CAMERAVIEW_H__
#define __CAMERAVIEW_H__
//==============================
//	camera.h
//==============================

// window system independent camera view code
#include "v_view.h"

class CameraView : public View
{
public:
	vec3	focus;
	vec3	angles;
	vec3	forward, right, up;	// move matrix
	vec3	vup, vpn, vright;	// view matrix
	vec3	mpUp, mpRight;		// mouse manipulation plane
	int		nCamButtonState;

	void MouseDown (int x, int y, int buttons);
	void MouseUp (int x, int y, int buttons);
	void MouseMoved (int x, int y, int buttons);
	void MouseControl (float dtime);

	void GetAimPoint(vec3 &pt);
	void PointToRay(int x, int y, vec3 &rayOut);
	bool GetBasis(vec3 &right, vec3 &up, vec3 &forward);
	mouseContext_t	const GetMouseContext(const int x, const int y);

	void ChangeFloor(bool up);
	void PointAt(vec3 pt);
	void LevelView();
	void FreeLook();
	void PositionDrag();
	void PositionCenter();	// sikk - Center Camera on Selection
	void Orbit();
	void BoundAngles();

	void Draw ();
	bool CullBrush(Brush *b);

	void DrawSelected(Brush	*pList);

private:

	int		buttonX, buttonY;
	int		nCullv1[3], nCullv2[3];
	vec3	v3Cull1, v3Cull2;

	void Init ();
	void InitCull();
	void BuildMatrix ();
	void DrawGrid ();	// sikk - Camera Grid
	void DrawActive();
	bool DrawTools();
};

//========================================================================




#endif

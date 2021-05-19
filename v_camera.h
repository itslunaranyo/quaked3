#ifndef __CAMERAVIEW_H__
#define __CAMERAVIEW_H__
//==============================
//	camera.h
//==============================

// window system independent camera view code
#include "v_view.h"

typedef enum
{
	cd_wire,
	cd_solid,
	cd_texture,
	cd_blend
} camera_draw_mode;

class CameraView : public View
{
public:
	float	viewdistance;		// For rotating around a point
	vec3	angles;
	vec3	color;				// background 
	vec3	forward, right, up;	// move matrix
	vec3	vup, vpn, vright;	// view matrix
	vec3	mpUp, mpRight;		// mouse manipulation plane
	camera_draw_mode	draw_mode;
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
	void FreeLook();
	void PositionDrag();
	void PositionCenter();	// sikk - Center Camera on Selection
	void PositionRotate();
	void Rotate(int yaw, int pitch, const vec3 origin);
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

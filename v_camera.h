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
	vec_t	viewdistance;		// For rotating around a point
	vec3_t	angles;
	vec3_t	color;				// background 
	vec3_t	forward, right, up;	// move matrix
	vec3_t	vup, vpn, vright;	// view matrix
	camera_draw_mode	draw_mode;
	int		nCamButtonState;

	void MouseDown (int x, int y, int buttons);
	void MouseUp (int x, int y, int buttons);
	void MouseMoved (int x, int y, int buttons);
	void MouseControl (float dtime);

	void PointToRay(int x, int y, vec3_t rayOut);

	void ChangeFloor(bool up);
	void FreeLook();
	void PositionDrag();
	void PositionCenter();	// sikk - Center Camera on Selection
	void PositionRotate();
	void Rotate(int yaw, int pitch, vec3_t origin);
	void BoundAngles();

	void Draw ();

private:

	int		buttonX, buttonY;
	int		nCullv1[3], nCullv2[3];
	vec3_t	v3Cull1, v3Cull2;

	void Init ();
	void InitCull();
	bool CullBrush(Brush *b);
	void DrawGrid ();	// sikk - Camera Grid
	void DrawClipSplits();
	void BuildMatrix ();
};

//========================================================================




#endif

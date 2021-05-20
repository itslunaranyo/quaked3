#ifndef __CAMERAVIEW_H__
#define __CAMERAVIEW_H__
//==============================
//	camera.h
//==============================

// window system independent camera view code
#include "View.h"

class CameraView : public View
{
public:
	vec3	focus;
	vec3	angles;
	vec3	forward, right, up;	// move matrix
	vec3	vup, vpn, vright;	// view matrix
	vec3	mpUp, mpRight;		// mouse manipulation plane
	glm::mat4 matProj;
	int		nCamButtonState;

	void MouseDown (const int x, const int y, const int buttons);
	void MouseUp (const int x, const int y, const int buttons);
	void MouseMoved(const int x, const int y, const int buttons);
	void MouseWheel(const int x, const int y, bool up, const int buttons);
	void RealtimeControl (float dtime);

	void GetAimPoint(vec3 &pt);
	void PointToRay(int x, int y, vec3 &rayOut);
	bool GetBasis(vec3 &right, vec3 &up, vec3 &forward);
	mouseContext_t	const GetMouseContext(const int x, const int y);

	void ChangeFloor(bool up);
	void PointAt(vec3 pt);
	void LevelView();
	void FreeLook();
	void Strafe();
	void PositionCenter();	// sikk - Center Camera on Selection
	void Orbit();
	void Orbit(float pitch, float yaw);
	void SetOrbit(vec3 dir);
	void SetOrbit(int x, int y);
	void BoundAngles();

	void Draw ();
	bool CullBrush(Brush *b);

	void DrawSelected(Brush	*pList, vec3 selColor);

private:

	int		buttonX, buttonY;
	int		nCullv1[3], nCullv2[3];
	vec3	v3Cull1, v3Cull2;

	void Init ();
	void InitCull();
	glm::mat4 RotateMatrix(glm::mat4 mat);
	void BuildMatrix ();
	void DrawGrid();	// sikk - Camera Grid
	void DrawAxis();
	void DrawActive();
	bool DrawTools();
};

//========================================================================

extern CameraView g_vCamera;


#endif

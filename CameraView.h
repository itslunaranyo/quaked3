#ifndef __CAMERAVIEW_H__
#define __CAMERAVIEW_H__
//==============================
//	cameraview.h
//==============================

#include "DisplayView.h"

class CameraView : public DisplayView
{
public:
	void Init();
	void ChangeFloor(bool up);
	void PointAt(vec3 pt);
	void LevelView();
	void FreeLook(int dX, int dY);
	void Strafe(int dX, int dY);
	void PositionCenter();
	void Orbit(float yaw, float pitch);
	void SetOrbit(vec3 dir);
	void SetOrbit(int x, int y);
	void SetOrbit();
	void BoundAngles();

	void Reset();
	void SetOrigin(vec3 newOrg);
	void Step(float fwd, float rt, float up);
	void Move(float fwd, float rt, float up);
	void Turn(float ang, bool absolute = false);
	void Pitch(float ang, bool absolute = false);

	void GetAimPoint(vec3 &pt);
	void PointToRay(int x, int y, vec3 &rayOut);
	bool const GetBasis(vec3 &right, vec3 &up, vec3 &forward);
	bool CullBrush(Brush *b);

	mouseContext_t	const GetMouseContext(const int x, const int y);
	inline vec3 GetOrigin() { return origin; }
	inline vec3 GetAngles() { return angles; }
	inline vec3 GetViewDir() { return vpn; }
	inline vec3 GetViewUp() { return vup; }
	inline vec3 GetViewRight() { return vright; }
	inline vec3 GetPlanarUp() { return mpUp; }
	inline vec3 GetPlanarRight() { return mpRight; }
	inline glm::mat4 GetProjection() { return matProj; }

private:
	void InitCull();
	void BuildMatrix();
	int		nCullv1[3], nCullv2[3];
	vec3	v3Cull1, v3Cull2;

	vec3	origin, focus, angles;
	vec3	mvForward, mvRight;	// move matrix
	vec3	vup, vpn, vright;	// view matrix
	vec3	mpUp, mpRight;		// mouse manipulation plane
	glm::mat4 matProj;
};

//========================================================================

extern CameraView g_vCamera;


#endif

//==============================
//	v_camera.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "CameraView.h"
#include "map.h"
#include "points.h"
#include "select.h"
#include "Tool.h"

#include <glm/gtc/matrix_transform.hpp>

CameraView g_vCamera;

/*
============
CameraView::Init
============
*/
void CameraView::Init()
{
	Reset();
	//viewdistance = 256;
	BuildMatrix();
}

// sikk---> Center Camera on Selection. Same as PositionView() for XY View
/*
==================
CameraView::PositionCenter
==================
*/
void CameraView::PositionCenter ()
{
	vec3 smins, smaxs, sorg, dir;
	Selection::GetBounds(smins, smaxs);
	sorg = (smins + smaxs) * 0.5f;
	dir = normalize(origin - sorg);

	origin = sorg + dir * (float)(VectorLength(smaxs - smins) * 0.5f + 64.0f);
	PointAt(sorg);
	BuildMatrix();
}
// <---sikk


/*
===============
CameraView::BuildMatrix
===============
*/
void CameraView::BuildMatrix ()
{
	int		i;
	float	ya;

	ya = angles[1] / 180 * Q_PI;

	// the movement matrix is kept 2d
    mvForward[0] = cos(ya);
    mvForward[1] = sin(ya);
    mvRight[0] = mvForward[1];
    mvRight[1] = -mvForward[0];

	float yfov = atan((float)width / height);
	matProj = glm::perspective(yfov, (float)width / height, 2.0f, (float)g_cfgEditor.MapSize);
	matProj = RotateMatrix(matProj, angles[PITCH], angles[YAW]);
	matProj = glm::translate(matProj, -origin);

	for (i = 0; i < 3; i++)
	{
		vright[i] = matProj[i][0];
		vup[i] = matProj[i][1];
		vpn[i] = matProj[i][2];
	}

	VectorNormalize(vright);
	VectorNormalize(vup);
	VectorNormalize(vpn);

	mpUp = AxisForVector(vup);
	mpRight = AxisForVector(vright);

	InitCull();
}

/*
===============
CameraView::ChangeFloor
===============
*/
void CameraView::ChangeFloor (bool up)
{
	float		d, bestd, current;
	vec3		start, dir;
	Brush	   *b;

	start[0] = origin[0];
	start[1] = origin[1];
	start[2] = 8192;
	dir[0] = dir[1] = 0;
	dir[2] = -1;

	current = 8192 - (origin[2] - 48);
	if (up)
		bestd = 0;
	else
		bestd = 16384;

	for (b = g_map.brActive.Next(); b != &g_map.brActive; b = b->Next())
	{
		if (!b->RayTest(start, dir, &d))
			continue;
		if (up && d < current && d > bestd)
			bestd = d;
		if (!up && d > current && d < bestd)
			bestd = d;
	}

	if (bestd == 0 || bestd == 16384)
		return;

	origin[2] += current - bestd;

	BuildMatrix();
}

void CameraView::PointAt(vec3 pt)
{
	focus = pt;
	vpn = pt - origin;
	VectorToAngles(vpn, angles);
	BoundAngles();

	BuildMatrix();
}

void CameraView::LevelView()
{
	g_vCamera.angles[ROLL] = g_vCamera.angles[PITCH] = 0;
	g_vCamera.angles[YAW] = 22.5f * floor((g_vCamera.angles[YAW] + 11) / 22.5f);
	BoundAngles();

	BuildMatrix();
}


//===============================================

/*
================
CameraView::Strafe
================
*/
void CameraView::Strafe(int dX, int dY)
{
	SetCursor(NULL); // sikk - Remove Cursor

	if (dX != 0 || dY != 0)
	{
		origin = origin + (float)dX * vright;
		origin[2] -= dY;

		BuildMatrix();
	}
}


/*
==================
CameraView::Orbit
==================
*/
void CameraView::Orbit(float yaw, float pitch)
{
	float vecdist;
	angles[PITCH] -= pitch;
	angles[YAW] -= yaw;
	BoundAngles();
	vecdist = VectorLength(focus - origin);
	AngleVectors(angles, vpn, vright, vup);
	vpn[2] = -vpn[2];
	origin = focus - vecdist * vpn;

	BuildMatrix();
}

/*
==============
CameraView::SetOrbit
==============
*/
void CameraView::SetOrbit(vec3 dir)
{
	vec3 aim;
	trace_t vtest;
	vtest = Selection::TestRay(origin, dir, 0);
	if (vtest.brush)
	{
		if (vtest.brush->owner->IsPoint())
			focus = (vtest.brush->mins + vtest.brush->maxs) * 0.5f;
		else
			focus = origin + vtest.dist * dir;
	}
	else
	{
		focus = origin + 256.0f * dir;
	}
	PointAt(focus);
	BuildMatrix();
}

void CameraView::SetOrbit(int x, int y)
{
	vec3 dir;
	PointToRay(x, y, dir);
	SetOrbit(dir);
}

void CameraView::SetOrbit()
{
	SetOrbit(vpn);
}

/*
================
CameraView::FreeLook
================
*/
void CameraView::FreeLook(int dX, int dY)
{
	if (!dX && !dY)
		return;

	if (angles[PITCH] > 100)
		angles[PITCH] -= 360;

	// TODO: sensitivity preference
	angles[PITCH] -= 0.65f * dY;
	angles[YAW] -= 0.65f * dX;

	BoundAngles();
	BuildMatrix();
}


/*
================
CameraView::BoundAngles
================
*/
void CameraView::BoundAngles()
{
	angles[YAW] = fmod(angles[YAW], 360.0f);
	angles[PITCH] = fmod(angles[PITCH] + 180, 360.0f);
	angles[PITCH] = min(angles[PITCH], 270.0f);
	angles[PITCH] = max(angles[PITCH], 90.0f);
	angles[PITCH] -= 180;
}

/*
================
CameraView::Step
ground plane aligned movement for driving around
================
*/
void CameraView::Step(float fwd, float rt, float up)
{
	origin += fwd * mvForward + rt * mvRight;
	origin.z += up;
	BuildMatrix();
}

void CameraView::Reset()
{
	angles = vec3(0);
	origin = vec3(0, 0, 48);
	focus = vec3(256, 0, 48);
}

/*
================
CameraView::SetOrigin
================
*/
void CameraView::SetOrigin(vec3 newOrg)
{
	origin = newOrg;
	BuildMatrix();
}

/*
================
CameraView::Move
relative, for absolute use SetOrigin
================
*/
void CameraView::Move(float fwd, float rt, float up)
{
	origin += vup * up + vpn * fwd + vright * rt;
	BuildMatrix();
}

/*
================
CameraView::Turn
yaw only
================
*/
void CameraView::Turn(float ang, bool absolute)
{
	angles[YAW] = absolute ? ang : (angles[YAW] + ang);
	BoundAngles();
	BuildMatrix();
}

/*
================
CameraView::Pitch
================
*/
void CameraView::Pitch(float ang, bool absolute)
{
	angles[PITCH] = absolute ? ang : (angles[PITCH] + ang);
	BoundAngles();
	BuildMatrix();
}

/*
==============
CameraView::PointToRay
==============
*/
void CameraView::PointToRay(int x, int y, vec3 &rayOut)
{
	float	f, r, u;

	// calc ray direction
	u = (float)(y - height / 2.0f) / (width / 2.0f);
	r = (float)(x - width / 2.0f) / (width / 2.0f);
	f = 1.0f;

	// precisely match whatever wonky way GL feels like creating a projection from the 
	// window size by snatching v_up/v_right scaling directly from an unrotated frustum
	float yfov = atan((float)width / height);// *180 / Q_PI;
	glm::mat4 mat = glm::perspectiveFov(yfov, (float)width, (float)height, 2.0f, (float)g_cfgEditor.MapSize);

	u /= mat[0][0];
	r /= mat[0][0];

	for (int i = 0; i < 3; i++)
	{
		rayOut[i] = vpn[i] * f + vright[i] * r + vup[i] * u;
	}
	
	VectorNormalize(rayOut);
}



bool const CameraView::GetBasis(vec3 &_right, vec3 &_up, vec3 &_forward)
{
	_right = vright;
	_up = vup;
	_forward = vpn;
	return true;
}

mouseContext_t const CameraView::GetMouseContext(const int x, const int y)
{
	mouseContext_t mc;

	mc.pt = vec3(x, y, 0);
	mc.org = origin;
	mc.up = mpUp;
	mc.right = mpRight;
	PointToRay(x, y, mc.ray);
	mc.dims = 3;	// 3D view
	return mc;
}



/*
============
CameraView::GetAimPoint
pick a spot somewhere in front of the camera for dropping entities
============
*/
void CameraView::GetAimPoint(vec3 &pt)
{
	float		dist;
	trace_t		t;

	t = Selection::TestRay(origin, vpn, SF_NOFIXEDSIZE);
	dist = min(240.0f, t.dist);
	pt = origin + dist * vpn;
	pt = pointOnGrid(pt);
}

eViewType_t CameraView::GetForwardAxis()
{
	vec3 f = glm::abs(vpn);
	if (f.x > f.y && f.x > f.z)
		return GRID_YZ;
	if (f.y > f.z)
		return GRID_XZ;
	return GRID_XY;
}


/*
============
CameraView::InitCull
============
*/
void CameraView::InitCull ()
{
	int	i;

	v3Cull1 = vpn - vright;
	v3Cull2 = vpn + vright;

	for (i = 0; i < 3; i++)
	{
		if (v3Cull1[i] > 0)
			nCullv1[i] = 3 + i;
		else
			nCullv1[i] = i;

		if (v3Cull2[i] > 0)
			nCullv2[i] = 3 + i;
		else
			nCullv2[i] = i;
	}
}

/*
================
CameraView::CullBrush
================
*/
bool CameraView::CullBrush (Brush *b)
{
	int		i;
	float	d;
	vec3	point;

// sikk---> Cubic Clipping
	if (g_cfgEditor.CubicClip)
	{
		float fLevel = (int)g_cfgEditor.CubicScale * 64;

		point[0] = origin[0] - fLevel;
		point[1] = origin[1] - fLevel;
		point[2] = origin[2] - fLevel;

		for (i = 0; i < 3; i++)
			if (b->mins[i] < point[i] && b->maxs[i] < point[i])
				return true;

		point[0] = origin[0] + fLevel;
		point[1] = origin[1] + fLevel;
		point[2] = origin[2] + fLevel;
	
		for (i = 0; i < 3; i++)
			if (b->mins[i] > point[i] && b->maxs[i] > point[i])
				return true;
	}
// <---sikk

	// an explanation of this QE-original hack, for future reference:
	// selecting the appropriate corners of the bounds to test from the eight possibilities
	// is done with zero branching by exploiting the fact that a) array lookup is unbounded, 
	// and b) mins and maxs are stored adjacent in the brush structure (so mins[3] actually
	// yields maxs[0]). this will go bazonkas if the compiler should put padding between those 
	// two vec3s for some reason, so if culling is broken and your quest has led you here,
	// maybe look at that.	love, past lunaran
	for (i = 0; i < 3; i++)
		point[i] = ((float*)&b->mins)[nCullv1[i]] - origin[i];

	d = DotProduct(point, v3Cull1);
	if (d < -1)
		return true;

	for (i = 0; i < 3; i++)
		point[i] = ((float*)&b->mins)[nCullv2[i]] - origin[i];

	d = DotProduct(point, v3Cull2);
	if (d < -1)
		return true;

	return false;
}

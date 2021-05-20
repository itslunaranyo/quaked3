//==============================
//	mathlib.c
//==============================

// math primitives

#include "cmdlib.h"
#include "mathlib.h"
#include "qedefs.h"
#include <assert.h>

// for sorting arrays/lists of vectors with intent to strip duplicates
bool VectorCompareLT(const vec3 &v1, const vec3 &v2)
{
	if (v1.x < v2.x)
		return true;
	if (v1.x == v2.x)
	{
		if (v1.y < v2.y)
			return true;
		if (v1.y == v2.y)
		{
			if (v1.z < v2.z)
				return true;
		}
	}
	return false;
}

bool VectorCompare(const vec3 &v1, const vec3 &v2)
{
	for (int i = 0; i < 3; i++)
		if (fabs(v1[i] - v2[i]) > EQUAL_EPSILON)
			return false;

	return true;
}

float VectorNormalize(vec3 &v)
{
	float l = glm::length(v);
	v = v / l;
	return l;
}

static float Cross2D(const vec3 a, const vec3 b) { return (a[0] * b[1] - a[1] * b[0]); }

bool LineSegmentIntersect2D(const vec3 l1start, const vec3 l1end, const vec3 l2start, const vec3 l2end, vec3 &intersect)
{
	float t, u;
	vec3 l1v, l2v;

	l1v = l1end - l1start;
	l2v = l2end - l2start;

	t = Cross2D((l2start - l1start), l2v) / Cross2D(l1v, l2v);
	u = Cross2D((l1start - l2start), l1v) / Cross2D(l2v, l1v);

	if ((0 <= t && t <= 1) && (0 <= u && u <= 1))
	{
		intersect = l1start + l1v * t;
		return true;
	}
	return false;
}

void VectorRotate(const vec3 vIn, const vec3 vRotation, vec3 &out)
{
	vec3	vWork, va;
	double	dAngle;
	float	c, s;
	const int nIndex[3][2] = { 1,2,2,0,0,1 };

	va = vIn;
	vWork = va;
	/*
	nIndex[0][0] = 1;
	nIndex[0][1] = 2;
	nIndex[1][0] = 2;
	nIndex[1][1] = 0;
	nIndex[2][0] = 0;
	nIndex[2][1] = 1;
	*/
	for (int i = 0; i < 3; i++)
	{
		if (vRotation[i] != 0)
		{
			dAngle = vRotation[i] * Q_PI / 180.0;
			c = (float)cos(dAngle);
			s = (float)sin(dAngle);
			vWork[nIndex[i][0]] = va[nIndex[i][0]] * c - va[nIndex[i][1]] * s;
			vWork[nIndex[i][1]] = va[nIndex[i][0]] * s + va[nIndex[i][1]] * c;
		}
		va = vWork;
	}
	out = vWork;
}

void VectorRotate2(const vec3 vIn, const vec3 vRotation, const vec3 vOrigin, vec3 &out)
{
	vec3 v, vo;
	v = vIn - vOrigin;
	VectorRotate(v, vRotation, vo);
	out = vo + vOrigin;
}

void VectorPolar(vec3 &v, const float radius, const float theta, const float phi)
{
	v[0] = radius * (float)cos(theta) * (float)cos(phi);
	v[1] = radius * (float)sin(theta) * (float)cos(phi);
	v[2] = radius * (float)sin(phi);
}

void AddPointToBounds(const vec3 v, vec3 &mins, vec3 &maxs)
{
	for (int i = 0; i < 3; i++)
	{
		mins[i] = min(v[i], mins[i]);
		maxs[i] = max(v[i], maxs[i]);
	}
}

void AngleVectors(const vec3 angles, vec3 &forward, vec3 &right, vec3 &up)
{
	float	angle;
	float	sr, sp, sy, cr, cp, cy;

	angle = angles[YAW] * (float)(Q_PI / 180.0f);
	sy = (float)sin(angle);
	cy = (float)cos(angle);
	angle = angles[PITCH] * (float)(Q_PI / 180.0f);
	sp = (float)sin(angle);
	cp = (float)cos(angle);
	angle = angles[ROLL] * (float)(Q_PI / 180.0f);
	sr = (float)sin(angle);
	cr = (float)cos(angle);

	forward = vec3(	cp * cy, 
					cp * sy, 
					-sp);
	right = vec3(	-sr * sp * cy + cr * sy, 
					-sr * sp * sy - cr * cy, 
					-sr * cp);
	up = vec3(	cr * sp * cy + sr * sy, 
				cr * sp * sy - sr * cy, 
				cr * cp);
}

void VectorToAngles(const vec3 vec, vec3 &angles)
{
	float	forward;
	float	yaw, pitch;

	if ((vec[0] == 0) && (vec[1] == 0))
	{
		yaw = 0;
		if (vec[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
		yaw = (float)atan2(vec[1], vec[0]) * 180.0f / (float)Q_PI;
		if (yaw < 0)
			yaw += 360;

		forward = (float)sqrt(vec[0] * vec[0] + vec[1] * vec[1]);
		pitch = (float)atan2(vec[2], forward) * 180.0f / (float)Q_PI;
		if (pitch < 0)
			pitch += 360;
	}

	angles = vec3(pitch, yaw, 0);
}

bool Point_Equal(const vec3 p1, const vec3 p2, const float epsilon)
{
	for (int i = 0; i < 3; i++)
		if (fabs(p1[i] - p2[i]) > epsilon)
			return false;

	return true;
}

void rgbToHex(const vec3 vrgb, char *hex)
{
	int i, c;
	unsigned rgb[3];
	char hx[16] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };

	c = 0;
	hex[c++] = '#';
	for (i = 0; i < 3; i++)
	{
		rgb[i] = (unsigned)(255 * fabs(vrgb[i]) + 0.5f);
		hex[c++] = hx[rgb[i] >> 4];
		hex[c++] = hx[rgb[i] & 15];
	}
	hex[c] = 0;
}

void hexToRGB(const char *hex, vec3 &vrgb)
{
	int i;
	unsigned rgb[6];

	if (hex[0] != '#')
		return;

	for (i = 0; i < 6; i++)
	{
		if (hex[i + 1] >= 'A')
			rgb[i] = hex[i + 1] - 'A' + 10;
		else
			rgb[i] = hex[i + 1] - '0';
	}
	for (i = 0; i < 3; i++)
	{
		vrgb[i] = (rgb[i * 2] * 16 + rgb[i * 2 + 1]) / 255.0f;
	}
}

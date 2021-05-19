//==============================
//	mathlib.c
//==============================

// math primitives

#include "cmdlib.h"
#include "mathlib.h"
#include "qedefs.h"
#include <assert.h>

bool VectorCompare(const vec3 v1, const vec3 v2)
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

void VectorRotate(const vec3 vIn, const vec3 vRotation, vec3 &out)
{
	vec3	vWork, va;
	int		nIndex[3][2];
	double	dAngle;
	float	c, s;

	va = vIn;
	vWork = va;
	nIndex[0][0] = 1;
	nIndex[0][1] = 2;
	nIndex[1][0] = 2;
	nIndex[1][1] = 0;
	nIndex[2][0] = 0;
	nIndex[2][1] = 1;

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

#ifdef _OLD_VEC3
/*
==================
VectorLength
==================
*/
float VectorLength (vec3_t v)
{
	int		i;
	float	length;
	
	length = 0.0f;
	for (i = 0; i < 3; i++)
		length += v[i] * v[i];
	length = (float)sqrt(length);

	return length;
}

/*
==================
VectorCompare
==================
*/
bool VectorCompare (vec3_t v1, vec3_t v2)
{
	int		i;
	
	for (i = 0; i < 3; i++)
		if (fabs(v1[i] - v2[i]) > EQUAL_EPSILON)
			return false;
			
	return true;
}

/*
==================
Q_rint
==================
*/
float Q_rint (float in)
{
	return (float)floor(in + 0.5);
}

/*
==================
VectorMA
==================
*/
void VectorMA (vec3_t va, float scale, vec3_t vb, vec3_t vc)
{
	vc[0] = va[0] + scale * vb[0];
	vc[1] = va[1] + scale * vb[1];
	vc[2] = va[2] + scale * vb[2];
}

/*
==================
CrossProduct
==================
*/
void CrossProduct (vec3_t v1, vec3_t v2, vec3_t cross)
{
	cross[0] = v1[1] * v2[2] - v1[2] * v2[1];
	cross[1] = v1[2] * v2[0] - v1[0] * v2[2];
	cross[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

/*
==================
_DotProduct
==================
*/
float _DotProduct (vec3_t v1, vec3_t v2)
{
	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

/*
==================
_VectorSubtract
==================
*/
void _VectorSubtract (vec3_t va, vec3_t vb, vec3_t out)
{
	out[0] = va[0] - vb[0];
	out[1] = va[1] - vb[1];
	out[2] = va[2] - vb[2];
}

/*
==================
_VectorAdd
==================
*/
void _VectorAdd (vec3_t va, vec3_t vb, vec3_t out)
{
	out[0] = va[0] + vb[0];
	out[1] = va[1] + vb[1];
	out[2] = va[2] + vb[2];
}

/*
==================
_VectorCopy
==================
*/
void _VectorCopy (vec3_t in, vec3_t out)
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

/*
==================
VectorNormalize
==================
*/
float VectorNormalize (vec3_t v)
{
	int		i;
	float	length;
	
	length = 0.0f;
	for (i = 0; i < 3; i++)
		length += v[i] * v[i];
	length = (float)sqrt(length);
	if (length == 0)
		return (float)0;
		
	for (i = 0; i < 3; i++)
		v[i] /= length;	

	return length;
}

/*
==================
VectorInverse
==================
*/
void VectorInverse (vec3_t v)
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

/*
==================
VectorScale
==================
*/
void VectorScale (vec3_t v, float scale, vec3_t out)
{
	out[0] = v[0] * scale;
	out[1] = v[1] * scale;
	out[2] = v[2] * scale;
}

/*
==================
VectorRotate
==================
*/
void VectorRotate (vec3_t vIn, vec3_t vRotation, vec3_t out)
{
	vec3_t	vWork, va;
	int		nIndex[3][2];
	int		i;
	double	dAngle;
	double	c;
	double	s;
	
	va = vIn;
	vWork = va;
	nIndex[0][0] = 1;
	nIndex[0][1] = 2;
	nIndex[1][0] = 2;
	nIndex[1][1] = 0;
	nIndex[2][0] = 0;
	nIndex[2][1] = 1;
	
	for (i = 0; i < 3; i++)
	{
		if (vRotation[i] != 0)
		{
			dAngle = vRotation[i] * Q_PI / 180.0;
			c = cos(dAngle);
			s = sin(dAngle);
			vWork[nIndex[i][0]] = va[nIndex[i][0]] * (float)c - va[nIndex[i][1]] * (float)s;
			vWork[nIndex[i][1]] = va[nIndex[i][0]] * (float)s + va[nIndex[i][1]] * (float)c;
		}
		va = vWork;
	}
	out = vWork;
}

/*
==================
VectorRotate2
==================
*/
void VectorRotate2 (vec3_t vIn, vec3_t vRotation, vec3_t vOrigin, vec3_t out)
{
	vec3_t	vTemp, vTemp2;
	
	vTemp = vIn - vOrigin;
	VectorRotate(vTemp, vRotation, vTemp2);
	out = vTemp2 + vOrigin;
}

// sikk---> Brush Primitives
/*
==================
VectorPolar
==================
*/void VectorPolar (vec3_t v, float radius, float theta, float phi)
{
 	v[0] = radius * (float)cos(theta) * (float)cos(phi);
	v[1] = radius * (float)sin(theta) * (float)cos(phi);
	v[2] = radius * (float)sin(phi);
}
// <---sikk

/*
==================
ClearBounds
==================
*/
void ClearBounds (vec3_t mins, vec3_t maxs)
{
	mins[0] = mins[1] = mins[2] = 99999;
	maxs[0] = maxs[1] = maxs[2] = -99999;
}

/*
==================
AddPointToBounds
==================
*/
void AddPointToBounds (vec3_t v, vec3_t mins, vec3_t maxs)
{
	int		i;
	float	val;
	
	for (i = 0; i < 3; i++)
	{
		val = v[i];
		if (val < mins[i])
			mins[i] = val;
		if (val > maxs[i])
			maxs[i] = val;
	}
}

/*
==================
AngleVectors
==================
*/
void AngleVectors (vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
	float			angle;
	static float	sr, sp, sy, cr, cp, cy;	// static to help MS compiler fp bugs
	
	angle = angles[YAW] * ((float)Q_PI * 2 / 360.0f);
	sy = (float)sin(angle);
	cy = (float)cos(angle);
	angle = angles[PITCH] * ((float)Q_PI * 2 / 360.0f);
	sp = (float)sin(angle);
	cp = (float)cos(angle);
	angle = angles[ROLL] * ((float)Q_PI * 2 / 360.0f);
	sr = (float)sin(angle);
	cr = (float)cos(angle);
	
	if (forward)
	{
		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;
	}
	if (right)
	{
		right[0] = -sr * sp * cy + cr * sy;
		right[1] = -sr * sp * sy - cr * cy;
		right[2] = -sr * cp;
	}
	if (up)
	{
		up[0] = cr * sp * cy + sr * sy;
		up[1] = cr * sp * sy - sr * cy;
		up[2] = cr * cp;
	}
}

/*
==================
VectorToAngles
==================
*/
void VectorToAngles (vec3_t vec, vec3_t angles)
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
	
	angles[0] = pitch;
	angles[1] = yaw;
	angles[2] = 0;
}

/*
=================
Point_Equal
=================
*/
bool Point_Equal(vec3_t p1, vec3_t p2, float epsilon)
{
	int i;

	for (i = 0; i < 3; i++)
		if (fabs(p1[i] - p2[i]) > epsilon)
			return false;

	return true;
}

#endif
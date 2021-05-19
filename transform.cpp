//==============================
//	transform.cpp
//==============================

#include "qe3.h"

vec3	g_v3RotateOrigin;	// sikk - Free Rotate
vec3	g_v3SelectOrigin;
vec3	g_v3SelectMatrix[3];
bool	g_bSelectFlipOrder;


/*
================================================================================

TRANSFORMATIONS

===============================================================================
*/


/*
================
Transform_Move
================
*/
void Transform_Move(const vec3 delta)
{
	for (Brush *b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		if (b->owner->IsPoint())
			b->owner->Move(delta);
		else
			b->Move(delta);
	}
	//	Sys_UpdateWindows(W_ALL);
}


/*
===============
Transform_ApplyMatrix
===============
*/
void Transform_ApplyMatrix()
{
	Brush	*b;
	Face	*f;
	int		i, j;
	vec3	temp;

	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		for (f = b->basis.faces; f; f = f->fnext)
		{
			for (i = 0; i < 3; i++)
			{
				temp = f->plane.pts[i] - g_v3SelectOrigin;
				for (j = 0; j < 3; j++)
					f->plane.pts[i][j] = DotProduct(temp, g_v3SelectMatrix[j]) + g_v3SelectOrigin[j];
			}
			if (g_bSelectFlipOrder)
			{
				f->plane.Flip();
			}
			f->plane.Make();
		}
		b->Build();
	}
	Sys_UpdateWindows(W_SCENE);
}

/*
===============
Transform_FlipAxis
===============
*/
void Transform_FlipAxis(int axis)
{
	int		i;

	Selection::GetMid(g_v3SelectOrigin);
	for (i = 0; i < 3; i++)
	{
		g_v3SelectMatrix[i] = vec3(0);
		g_v3SelectMatrix[i][i] = 1;
	}
	g_v3SelectMatrix[axis][axis] = -1;

	g_bSelectFlipOrder = true;
	Transform_ApplyMatrix();
	Sys_UpdateWindows(W_SCENE);
}


/*
===============
Transform_RotateAxis
===============
*/
void Transform_RotateAxis(int axis, float deg, bool bMouse)
{
	/*
	vec3_t	temp;
	int		i, j;
	vec_t	c, s;

	if (deg == 0)
	return;

	Selection::GetMid(g_v3SelectOrigin);
	g_bSelectFlipOrder = false;

	if (deg == 90)
	{
	for (i = 0; i < 3; i++)
	{
	g_v3SelectMatrix[i] = vec3(0);
	g_v3SelectMatrix[i][i] = 1;
	}
	i = (axis + 1) % 3;
	j = (axis + 2) % 3;
	temp = g_v3SelectMatrix[i];
	g_v3SelectMatrix[i] = g_v3SelectMatrix[j];
	g_v3SelectMatrix[j] = vec3(0) - temp;
	}
	else
	{
	deg = -deg;
	if (deg == -180)
	{
	c = -1;
	s = 0;
	}
	else if (deg == -270)
	{
	c = 0;
	s = -1;
	}
	else
	{
	c = cos(deg * Q_PI / 180.0);
	s = sin(deg * Q_PI / 180.0);
	}

	for (i = 0; i < 3; i++)
	{
	g_v3SelectMatrix[i] = vec3(0);
	g_v3SelectMatrix[i][i] = 1;
	}

	switch (axis)
	{
	case 0:
	g_v3SelectMatrix[1][1] = c;
	g_v3SelectMatrix[1][2] = -s;
	g_v3SelectMatrix[2][1] = s;
	g_v3SelectMatrix[2][2] = c;
	break;
	case 1:
	g_v3SelectMatrix[0][0] = c;
	g_v3SelectMatrix[0][2] = s;
	g_v3SelectMatrix[2][0] = -s;
	g_v3SelectMatrix[2][2] = c;
	break;
	case 2:
	g_v3SelectMatrix[0][0] = c;
	g_v3SelectMatrix[0][1] = -s;
	g_v3SelectMatrix[1][0] = s;
	g_v3SelectMatrix[1][1] = c;
	break;
	}
	}
	*/
	int		i;
	vec_t	c, s;


	while (deg >= 360)
		deg -= 360;
	while (deg < 0)
		deg += 360;

	if (deg == 0)
		return;

	if (bMouse)
	{
		g_v3SelectOrigin = g_v3RotateOrigin;
	}
	else
		Selection::GetMid(g_v3SelectOrigin);
	g_bSelectFlipOrder = false;

	deg = -deg;
	c = cos(deg * Q_PI / 180.0);
	s = sin(deg * Q_PI / 180.0);

	for (i = 0; i < 3; i++)
	{
		g_v3SelectMatrix[i] = vec3(0);
		g_v3SelectMatrix[i][i] = 1;
	}

	switch (axis)
	{
	case 0:
		g_v3SelectMatrix[1][1] = c;
		g_v3SelectMatrix[1][2] = -s;
		g_v3SelectMatrix[2][1] = s;
		g_v3SelectMatrix[2][2] = c;
		break;
	case 1:
		g_v3SelectMatrix[0][0] = c;
		g_v3SelectMatrix[0][2] = s;
		g_v3SelectMatrix[2][0] = -s;
		g_v3SelectMatrix[2][2] = c;
		break;
	case 2:
		g_v3SelectMatrix[0][0] = c;
		g_v3SelectMatrix[0][1] = -s;
		g_v3SelectMatrix[1][0] = s;
		g_v3SelectMatrix[1][1] = c;
		break;
	}

	if (g_qeglobals.d_bTextureLock)
		Surf_RotateForTransform(axis, deg, g_v3SelectOrigin);

	Transform_ApplyMatrix();
	Sys_UpdateWindows(W_SCENE);
}

// sikk---> Brush Scaling
/*
===========
Transform_Scale
===========
*/
void Transform_Scale(float x, float y, float z)
{
	int			i;
	Brush	   *b;
	Face	   *f;

	Selection::GetMid(g_v3SelectOrigin);

	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		for (f = b->basis.faces; f; f = f->fnext)
		{
			f->plane.Translate(-g_v3SelectOrigin);
			for (i = 0; i < 3; i++)
			{
				f->plane.pts[i][0] *= x;
				f->plane.pts[i][1] *= y;
				f->plane.pts[i][2] *= z;
				//f->plane.pts[i][0] = floor(f->plane.pts[i][0] / g_qeglobals.d_nGridSize + 0.5) * g_qeglobals.d_nGridSize;
				//f->plane.pts[i][1] = floor(f->plane.pts[i][1] / g_qeglobals.d_nGridSize + 0.5) * g_qeglobals.d_nGridSize;
				//f->plane.pts[i][2] = floor(f->plane.pts[i][2] / g_qeglobals.d_nGridSize + 0.5) * g_qeglobals.d_nGridSize;
			}
			f->plane.Translate(g_v3SelectOrigin);
		}

		b->Build();
	}

	Sys_UpdateWindows(W_SCENE);
}
// <---sikk

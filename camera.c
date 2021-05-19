//==============================
//	camera.c
//==============================

#include "qe3.h"


#define	PAGEFLIPS	2

static int	buttonx, buttony;
static int	cursorx, cursory;

int		g_nCullv1[3], g_nCullv2[3];
vec3_t	g_v3Cull1, g_v3Cull2;

// sikk---> Transparent Brushes
brush_t    *g_pbrTransBrushes[MAX_MAP_BRUSHES];
int			g_nNumTransBrushes;
// <---sikk

/*
============
Cam_Init
============
*/
void Cam_Init ()
{
//	g_qeglobals.d_camera.draw_mode = cd_texture;
//	g_qeglobals.d_camera.draw_mode = cd_solid;
//	g_qeglobals.d_camera.draw_mode = cd_wire;
	g_qeglobals.d_camera.timing = false;
	g_qeglobals.d_camera.origin[0] = 0;
	g_qeglobals.d_camera.origin[1] = 0;	// sikk - changed from "20"
	g_qeglobals.d_camera.origin[2] = 0;	// sikk - changed from "46"	
	g_qeglobals.d_camera.viewdistance = 256;
}

// sikk---> Center Camera on Selection. Same as PositionView() for XY View
/*
==================
Cam_PositionView
==================
*/
void Cam_PositionView ()
{
	brush_t *b;

	b = g_brSelectedBrushes.next;
	if (b && b->next != b)
	{
		g_qeglobals.d_camera.origin[0] = b->mins[0] - 64;
		g_qeglobals.d_camera.origin[1] = b->mins[1] - 64;
		g_qeglobals.d_camera.origin[2] = b->mins[2] + 64;
		g_qeglobals.d_camera.angles[0] = -22.5;
		g_qeglobals.d_camera.angles[1] = 45;
		g_qeglobals.d_camera.angles[2] = 0;
	}
	else
	{
		g_qeglobals.d_camera.origin[0] = g_qeglobals.d_camera.origin[0];
		g_qeglobals.d_camera.origin[1] = g_qeglobals.d_camera.origin[1];
		g_qeglobals.d_camera.origin[2] = g_qeglobals.d_camera.origin[2];
	}
}
// <---sikk

/*
===============
Cam_BuildMatrix
===============
*/
void Cam_BuildMatrix ()
{
	int		i;
	float	xa, ya;
	float	matrix[4][4];

	xa = g_qeglobals.d_camera.angles[0] / 180 * Q_PI;
	ya = g_qeglobals.d_camera.angles[1] / 180 * Q_PI;

	// the movement matrix is kept 2d
    g_qeglobals.d_camera.forward[0] = cos(ya);
    g_qeglobals.d_camera.forward[1] = sin(ya);
    g_qeglobals.d_camera.right[0] = g_qeglobals.d_camera.forward[1];
    g_qeglobals.d_camera.right[1] = -g_qeglobals.d_camera.forward[0];

	glGetFloatv(GL_PROJECTION_MATRIX, &matrix[0][0]);

	for (i = 0; i < 3; i++)
	{
		g_qeglobals.d_camera.vright[i] = matrix[i][0];
		g_qeglobals.d_camera.vup[i] = matrix[i][1];
		g_qeglobals.d_camera.vpn[i] = matrix[i][2];
	}

	VectorNormalize(g_qeglobals.d_camera.vright);
	VectorNormalize(g_qeglobals.d_camera.vup);
	VectorNormalize(g_qeglobals.d_camera.vpn);
}

/*
===============
Cam_ChangeFloor
===============
*/
void Cam_ChangeFloor (bool up)
{
	float		d, bestd, current;
	vec3_t		start, dir;
	brush_t	   *b;

	start[0] = g_qeglobals.d_camera.origin[0];
	start[1] = g_qeglobals.d_camera.origin[1];
	start[2] = 8192;
	dir[0] = dir[1] = 0;
	dir[2] = -1;

	current = 8192 - (g_qeglobals.d_camera.origin[2] - 48);
	if (up)
		bestd = 0;
	else
		bestd = 16384;

	for (b = g_brActiveBrushes.next; b != &g_brActiveBrushes; b = b->next)
	{
		if (!Brush_Ray(start, dir, b, &d))
			continue;
		if (up && d < current && d > bestd)
			bestd = d;
		if (!up && d > current && d < bestd)
			bestd = d;
	}

	if (bestd == 0 || bestd == 16384)
		return;

	g_qeglobals.d_camera.origin[2] += current - bestd;

	Sys_UpdateWindows(W_CAMERA | W_Z);
}


//===============================================

/*
================
Cam_PositionDrag
================
*/
void Cam_PositionDrag ()
{
	int	x, y;

	SetCursor(NULL); // sikk - Remove Cursor
	Sys_GetCursorPos(&x, &y);

	if (x != cursorx || y != cursory)
	{
		x -= cursorx;
		VectorMA(g_qeglobals.d_camera.origin, x, g_qeglobals.d_camera.vright, g_qeglobals.d_camera.origin);
		y -= cursory;
		g_qeglobals.d_camera.origin[2] -= y;

		Sys_SetCursorPos(cursorx, cursory);
		Sys_UpdateWindows(W_CAMERA | W_XY);
	}
}

/*
================
Cam_Rotate
================
*/
void Cam_Rotate (int x, int y, vec3_t origin)
{
	int		i;
	vec_t	distance;
	vec3_t	work, forward, dir, vecdist;

	for (i = 0; i < 3; i++)
	{
		vecdist[i] = fabs((g_qeglobals.d_camera.origin[i] - origin[i]));
		vecdist[i] *= vecdist[i];
	}

	g_qeglobals.d_camera.viewdistance = distance = sqrt(vecdist[0] + vecdist[1] + vecdist[2]);
	VectorSubtract(g_qeglobals.d_camera.origin, origin, work);
	VectorToAngles(work, g_qeglobals.d_camera.angles);

	if(g_qeglobals.d_camera.angles[PITCH] > 100)
		g_qeglobals.d_camera.angles[PITCH] -= 360;

	g_qeglobals.d_camera.angles[PITCH] -= y;
	
	if(g_qeglobals.d_camera.angles[PITCH] > 85)
		g_qeglobals.d_camera.angles[PITCH] = 85;

	if(g_qeglobals.d_camera.angles[PITCH] < -85)
		g_qeglobals.d_camera.angles[PITCH] = -85;

	g_qeglobals.d_camera.angles[YAW] -= x;

	AngleVectors(g_qeglobals.d_camera.angles, forward, NULL, NULL);
	forward[2] = -forward[2];
	VectorMA(origin, distance, forward, g_qeglobals.d_camera.origin);

	VectorSubtract(origin, g_qeglobals.d_camera.origin, dir);
	VectorNormalize(dir);
	g_qeglobals.d_camera.angles[1] = atan2(dir[1], dir[0]) * 180 / Q_PI;
	g_qeglobals.d_camera.angles[0] = asin(dir[2]) * 180 / Q_PI;

	Cam_BuildMatrix();

	Sys_SetCursorPos(cursorx, cursory);
	Sys_UpdateWindows(W_XY | W_CAMERA | W_Z);
}

/*
==================
Cam_PositionRotate
==================
*/
void Cam_PositionRotate ()
{
	int			x, y, i, j;
	vec3_t		mins, maxs, forward, vecdist;
	vec3_t		origin;
	brush_t	   *b;
	face_t	   *f;

	SetCursor(NULL); // sikk - Remove Cursor
	Sys_GetCursorPos(&x, &y);

	if (x == cursorx && y == cursory)
		return;
	
	x -= cursorx;
	y -= cursory;

	if (Select_HasBrushes())
	{
		mins[0] = mins[1] = mins[2] = 99999;
		maxs[0] = maxs[1] = maxs[2] = -99999;
		for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
		{
			for (i = 0; i < 3; i++)
			{
				if (b->maxs[i] > maxs[i])
					maxs[i] = b->maxs[i];
				if (b->mins[i] < mins[i])
					mins[i] = b->mins[i];
			}
		}
		origin[0] = (mins[0] + maxs[0]) / 2;
		origin[1] = (mins[1] + maxs[1]) / 2;
		origin[2] = (mins[2] + maxs[2]) / 2;
	}
	else if (Select_HasFaces())
	{
		mins[0] = mins[1] = mins[2] = 99999;
		maxs[0] = maxs[1] = maxs[2] = -99999;

//		f = g_pfaceSelectedFace;
		// rotate around last selected face
		f = g_pfaceSelectedFaces[Select_NumFaces() - 1];	// sikk - Multiple Face Selection
		for (j = 0; j < f->face_winding->numpoints; j++)
		{
			for (i = 0; i < 3; i++)
			{
				if (f->face_winding->points[j][i] > maxs[i])
					maxs[i] = f->face_winding->points[j][i];
				if (f->face_winding->points[j][i] < mins[i])
					mins[i] = f->face_winding->points[j][i];
			}
		}

		origin[0] = (mins[0] + maxs[0]) / 2;
		origin[1] = (mins[1] + maxs[1]) / 2;
		origin[2] = (mins[2] + maxs[2]) / 2;
	}
	else
	{
		AngleVectors(g_qeglobals.d_camera.angles, forward, NULL, NULL);
		forward[2] = -forward[2];
		VectorMA(g_qeglobals.d_camera.origin, g_qeglobals.d_camera.viewdistance, forward, origin);
	}

	for (i = 0; i < 3; i++)
	{
		vecdist[i] = fabs((g_qeglobals.d_camera.origin[i] - origin[i]));
		vecdist[i] *= vecdist[i];
	}

	Cam_Rotate(x, y, origin);
}

/*
================
Cam_FreeLook
================
*/
void Cam_FreeLook ()
{
	int	x, y;

	SetCursor(NULL); // sikk - Remove Cursor
	Sys_GetCursorPos (&x, &y);

	if (x == cursorx && y == cursory)
		return;
	
	x -= cursorx;
	y -= cursory;

	if (g_qeglobals.d_camera.angles[PITCH] > 100)
		g_qeglobals.d_camera.angles[PITCH] -= 360;

	g_qeglobals.d_camera.angles[PITCH] -= y;
	
	if (g_qeglobals.d_camera.angles[PITCH] > 85)
		g_qeglobals.d_camera.angles[PITCH] = 85;

	if (g_qeglobals.d_camera.angles[PITCH] < -85)
		g_qeglobals.d_camera.angles[PITCH] = -85;

	g_qeglobals.d_camera.angles[YAW] -= x;

	Sys_SetCursorPos(cursorx, cursory);
	Sys_UpdateWindows(W_XY | W_CAMERA | W_Z);
}

/*
================
Cam_MouseControl
================
*/
void Cam_MouseControl (float dtime)
{
	int		xl, xh;
	int		yl, yh;
	float	xf, yf;

	if (g_nCamButtonState != MK_RBUTTON)
		return;

	xf = (float)(buttonx - g_qeglobals.d_camera.width / 2) / (g_qeglobals.d_camera.width / 2);
	yf = (float)(buttony - g_qeglobals.d_camera.height / 2) / (g_qeglobals.d_camera.height / 2);

	xl = g_qeglobals.d_camera.width / 3;
	xh = xl * 2;
	yl = g_qeglobals.d_camera.height / 3;
	yh = yl * 2;

#if 0
	// strafe
	if (buttony < yl && (buttonx < xl || buttonx > xh))
		VectorMA(g_qeglobals.d_camera.origin, xf * dtime * g_qeglobals.d_savedinfo.nCameraSpeed, g_qeglobals.d_camera.right, g_qeglobals.d_camera.origin);
	else
#endif
	{
		xf *= 1.0 - fabs(yf);
		if (xf < 0)
		{
			xf += 0.1f;
			if (xf > 0)
				xf = 0;
		}
		else
		{
			xf -= 0.1f;
			if (xf < 0)
				xf = 0;
		}
		
		VectorMA(g_qeglobals.d_camera.origin, yf * dtime * g_qeglobals.d_savedinfo.nCameraSpeed, g_qeglobals.d_camera.forward, g_qeglobals.d_camera.origin);
		g_qeglobals.d_camera.angles[YAW] += xf * -dtime * (g_qeglobals.d_savedinfo.nCameraSpeed * 0.5);
	}
	Sys_UpdateWindows(W_CAMERA | W_XY);
}

/*
==============
Cam_MouseDown
==============
*/
void Cam_MouseDown (int x, int y, int buttons)
{
	int		i;
	float	f, r, u;
	vec3_t	dir;

	// calc ray direction
	u = (float)(y - g_qeglobals.d_camera.height / 2) / (g_qeglobals.d_camera.width / 2);
	r = (float)(x - g_qeglobals.d_camera.width / 2) / (g_qeglobals.d_camera.width / 2);
	f = 1;

	for (i = 0; i < 3; i++)
		dir[i] = g_qeglobals.d_camera.vpn[i] * f + g_qeglobals.d_camera.vright[i] * r + g_qeglobals.d_camera.vup[i] * u;
	VectorNormalize(dir);

	Sys_GetCursorPos(&cursorx, &cursory);

	g_nCamButtonState = buttons;
	buttonx = x;
	buttony = y;

	// LBUTTON = manipulate selection
	// shift-LBUTTON = select
	// middle button = grab texture
	// ctrl-middle button = set entire brush to texture
	// ctrl-shift-middle button = set single face to texture
	if ((buttons == MK_LBUTTON)	|| 
		(buttons == (MK_LBUTTON | MK_SHIFT)) || 
		(buttons == (MK_LBUTTON | MK_CONTROL)) || 
		(buttons == (MK_LBUTTON | MK_CONTROL | MK_SHIFT)) || 
		(buttons == MK_MBUTTON) || 
		(buttons == (MK_MBUTTON | MK_SHIFT)) || 
		(buttons == (MK_MBUTTON | MK_CONTROL)) || 
		(buttons == (MK_MBUTTON | MK_CONTROL | MK_SHIFT)))
	{
		Drag_Begin(x, y, buttons, g_qeglobals.d_camera.vright, g_qeglobals.d_camera.vup, g_qeglobals.d_camera.origin, dir);
		return;
	}

	if ((buttons == MK_RBUTTON))
	{
		Cam_MouseControl(0.1f);
		return;
	}
}

/*
==============
Cam_MouseUp
==============
*/
void Cam_MouseUp (int x, int y, int buttons)
{
	g_nCamButtonState = 0;
	Drag_MouseUp();
}

/*
==============
Cam_MouseMoved
==============
*/
void Cam_MouseMoved (int x, int y, int buttons)
{
	int			i;
	float		f, r, u;
	char		camstring[256];
	vec3_t		dir;
	trace_t		t;

	g_nCamButtonState = buttons;
	if (!buttons)
	{
		//
		// calc ray direction
		//
		u = (float)(y - g_qeglobals.d_camera.height / 2) / (g_qeglobals.d_camera.width / 2);
		r = (float)(x - g_qeglobals.d_camera.width / 2) / (g_qeglobals.d_camera.width / 2);
		f = 1;

		for (i = 0; i < 3; i++)
			dir[i] = g_qeglobals.d_camera.vpn[i] * f + g_qeglobals.d_camera.vright[i] * r + g_qeglobals.d_camera.vup[i] * u;
		VectorNormalize(dir);
		t = Test_Ray(g_qeglobals.d_camera.origin, dir, false);

		if (t.brush)
		{
			brush_t	   *b;
			int			i;
			vec3_t		mins, maxs, size;

			for (i = 0; i < 3; i++)
			{
				mins[i] = 99999;
				maxs[i] = -99999;
			}

			b = t.brush;
			for (i = 0; i < 3; i++)
			{
				if (b->mins[i] < mins[i])
					mins[i] = b->mins[i];
				if (b->maxs[i] > maxs[i])
					maxs[i] = b->maxs[i];
			}

			VectorSubtract(maxs, mins, size);
			if (t.brush->owner->eclass->fixedsize)
				sprintf(camstring, "%s (%d %d %d)", t.brush->owner->eclass->name, (int)size[0], (int)size[1], (int)size[2]);
			else
				sprintf(camstring, "%s (%d %d %d) %s", t.brush->owner->eclass->name, (int)size[0], (int)size[1], (int)size[2], t.face->texdef.name);
		}
		else
			sprintf(camstring, "");

		Sys_Status(camstring, 0);

		return;
	}

	buttonx = x;
	buttony = y;

	if (buttons == (MK_RBUTTON | MK_CONTROL))
	{
		Cam_PositionDrag();
		Sys_UpdateWindows(W_XY | W_CAMERA | W_Z);
		return;
	}

	if (buttons == (MK_RBUTTON | MK_SHIFT))
	{
		Cam_PositionRotate();
		Sys_UpdateWindows(W_XY | W_CAMERA | W_Z);
		return;
	}

	if (buttons == (MK_RBUTTON | MK_CONTROL | MK_SHIFT))
	{
		Cam_FreeLook();
		Sys_UpdateWindows(W_XY | W_CAMERA | W_Z);
		return;
	}

// sikk---> Mouse Driven Texture Manipulation - TODO: Too sensitive...
	if (buttons & MK_LBUTTON)
	{			
		if (GetKeyState(VK_MENU) < 0)
		{
			Sys_GetCursorPos(&x, &y);
			/*
			if (GetKeyState(VK_SHIFT) < 0)
			{
				if (GetKeyState(VK_CONTROL) < 0)
					Select_RotateTexture(cursorx - x);
			}
			else if (GetKeyState(VK_CONTROL) < 0)
				Select_ScaleTexture(x - cursorx, cursory - y);

			else
				Select_ShiftTexture(cursorx - x, cursory - y);
			*/
			cursorx = x;
			cursory = y;
		}
// <---sikk
		else
		{
			Sys_GetCursorPos(&cursorx, &cursory);
			Drag_MouseMoved(x, y, buttons);
			Sys_UpdateWindows(W_XY | W_CAMERA | W_Z);
		}
	}
}

/*
============
Cam_InitCull
============
*/
void Cam_InitCull ()
{
	int	i;

	VectorSubtract(g_qeglobals.d_camera.vpn, g_qeglobals.d_camera.vright, g_v3Cull1);
	VectorAdd(g_qeglobals.d_camera.vpn, g_qeglobals.d_camera.vright, g_v3Cull2);

	for (i = 0; i < 3; i++)
	{
		if (g_v3Cull1[i] > 0)
			g_nCullv1[i] = 3 + i;
		else
			g_nCullv1[i] = i;

		if (g_v3Cull2[i] > 0)
			g_nCullv2[i] = 3 + i;
		else
			g_nCullv2[i] = i;
	}
}

/*
================
Cam_CullBrush
================
*/
bool Cam_CullBrush (brush_t *b)
{
	int		i;
	float	d;
	vec3_t	point;

// sikk---> Cubic Clipping
	if (g_qeglobals.d_savedinfo.bCubicClip)
	{
		float fLevel = g_qeglobals.d_savedinfo.nCubicScale * 64;

		point[0] = g_qeglobals.d_camera.origin[0] - fLevel;
		point[1] = g_qeglobals.d_camera.origin[1] - fLevel;
		point[2] = g_qeglobals.d_camera.origin[2] - fLevel;

		for (i = 0; i < 3; i++)
			if (b->mins[i] < point[i] && b->maxs[i] < point[i])
				return true;

		point[0] = g_qeglobals.d_camera.origin[0] + fLevel;
		point[1] = g_qeglobals.d_camera.origin[1] + fLevel;
		point[2] = g_qeglobals.d_camera.origin[2] + fLevel;
	
		for (i = 0; i < 3; i++)
			if (b->mins[i] > point[i] && b->maxs[i] > point[i])
				return true;
	}
// <---sikk

	for (i = 0; i < 3; i++)
		point[i] = b->mins[g_nCullv1[i]] - g_qeglobals.d_camera.origin[i];

	d = DotProduct(point, g_v3Cull1);
	if (d < -1)
		return true;

	for (i = 0; i < 3; i++)
		point[i] = b->mins[g_nCullv2[i]] - g_qeglobals.d_camera.origin[i];

	d = DotProduct(point, g_v3Cull2);
	if (d < -1)
		return true;

	return false;
}

/*
================
Cam_DrawClipSplits
================
*/
void Cam_DrawClipSplits ()
{
	g_qeglobals.d_pbrSplitList = NULL;

	if (g_qeglobals.d_bClipMode)
		if (g_cpClip1.bSet && g_cpClip2.bSet)
			g_qeglobals.d_pbrSplitList = ((g_qeglobals.d_nViewType == XZ) ? !g_qeglobals.d_bClipSwitch : g_qeglobals.d_bClipSwitch) ? &g_qeglobals.d_brFrontSplits : &g_qeglobals.d_brBackSplits;
}

// sikk---> Camera Grid
/*
==============
Cam_DrawGrid
==============
*/
void Cam_DrawGrid ()
{
	int x, y, i;

	i = g_qeglobals.d_savedinfo.nMapSize * 0.5;

	glColor3fv(g_qeglobals.d_savedinfo.v3Colors[COLOR_CAMERAGRID]);
//	glLineWidth(1);
	glBegin(GL_LINES);
	for (x = -i; x <= i; x += 256)
	{
		glVertex2i(x, -i);
		glVertex2i(x, i);
	}
	for (y = -i; y <= i; y += 256)
	{
		glVertex2i(-i, y);
		glVertex2i(i, y);
	}
	glEnd();
}
// <---sikk

/*
==============
Cam_Draw
==============
*/
void Cam_Draw ()
{
	int			i;
	double		start, end;
	float		screenaspect;
	float		yfov;
	brush_t    *pList;
	brush_t	   *brush;
	face_t	   *face;

	if (!g_brActiveBrushes.next)
		return;	// not valid yet

	if (g_qeglobals.d_camera.timing)
		start = Sys_DoubleTime();

	//
	// clear
	//
	QE_CheckOpenGLForErrors();

	glViewport(0, 0, g_qeglobals.d_camera.width, g_qeglobals.d_camera.height);
	glScissor(0, 0, g_qeglobals.d_camera.width, g_qeglobals.d_camera.height);
	glClearColor(g_qeglobals.d_savedinfo.v3Colors[COLOR_CAMERABACK][0],
				 g_qeglobals.d_savedinfo.v3Colors[COLOR_CAMERABACK][1],
				 g_qeglobals.d_savedinfo.v3Colors[COLOR_CAMERABACK][2],
				 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//
	// set up viewpoint
	//
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
/*
	// one directional light source directly behind the viewer
	{
		GLfloat inverse_cam_dir[4], ambient[4], diffuse[4];//, material[4];

		ambient[0] = ambient[1] = ambient[2] = 0.4f;
		ambient[3] = 1.0f;
		diffuse[0] = diffuse[1] = diffuse[2] = 0.4f;
		diffuse[3] = 1.0f;
//		material[0] = material[1] = material[2] = 0.8f;
//		material[3] = 1.0f;
    
		inverse_cam_dir[0] = g_qeglobals.d_camera.vpn[0];
		inverse_cam_dir[1] = g_qeglobals.d_camera.vpn[1];
		inverse_cam_dir[2] = g_qeglobals.d_camera.vpn[2];
		inverse_cam_dir[3] = 0;

		glLightfv(GL_LIGHT0, GL_POSITION, inverse_cam_dir);

		glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
		glEnable(GL_LIGHT0);

		glEnable(GL_LIGHTING);
		glEnable(GL_COLOR_MATERIAL);
		glEnableClientState(GL_NORMAL_ARRAY);
	}
*/
	screenaspect = (float)g_qeglobals.d_camera.width / g_qeglobals.d_camera.height;
	yfov = 2 * atan((float)g_qeglobals.d_camera.height / g_qeglobals.d_camera.width) * 180 / Q_PI;
    gluPerspective(yfov, screenaspect, 2, g_qeglobals.d_savedinfo.nMapSize);//8192);

    glRotatef(-90, 1, 0, 0);	// put Z going up
    glRotatef(90, 0, 0, 1);		// put Z going up
    glRotatef(g_qeglobals.d_camera.angles[0], 0, 1, 0);
    glRotatef(-g_qeglobals.d_camera.angles[1], 0, 0, 1);
    glTranslatef(-g_qeglobals.d_camera.origin[0], -g_qeglobals.d_camera.origin[1], -g_qeglobals.d_camera.origin[2]);

	Cam_BuildMatrix();
	Cam_InitCull();

	//
	// draw stuff
	//
	switch (g_qeglobals.d_camera.draw_mode)
	{
	case cd_wire:
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	    glDisable(GL_TEXTURE_2D);
	    glDisable(GL_TEXTURE_1D);
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
	    glColor3f(1.0, 1.0, 1.0);
//		glEnable (GL_LINE_SMOOTH);
		break;

	case cd_solid:
		glCullFace(GL_FRONT);
		glEnable(GL_CULL_FACE);
		glShadeModel(GL_FLAT);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		break;

	case cd_texture:
		glCullFace(GL_FRONT);
		glEnable(GL_CULL_FACE);
		glShadeModel(GL_FLAT);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_TEXTURE_2D);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

#if 0
		{
			GLfloat fogColor[4] = {0.0, 1.0, 0.0, 0.25};

			glFogi(GL_FOG_MODE, GL_LINEAR);
			glHint(GL_FOG_HINT, GL_NICEST);  /*  per pixel   */
			glFogf(GL_FOG_START, -8192);
			glFogf(GL_FOG_END, 65536);
			glFogfv(GL_FOG_COLOR, fogColor);
 
		}
#endif
		break;

	case cd_blend:
		glCullFace(GL_FRONT);
		glEnable(GL_CULL_FACE);
		glShadeModel(GL_FLAT);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_TEXTURE_2D);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
	}

// sikk---> Camera Grid
	if (g_qeglobals.d_savedinfo.bShow_CameraGrid)
		Cam_DrawGrid();
// <---sikk

	glMatrixMode(GL_TEXTURE);

	g_nNumTransBrushes = 0;	// sikk - Transparent Brushes

	for (brush = g_brActiveBrushes.next; brush != &g_brActiveBrushes; brush = brush->next)
	{
		if (Cam_CullBrush(brush))
			continue;
		if (FilterBrush(brush))
			continue;
// sikk---> Transparent Brushes
		// TODO: Make toggle via Preferences Option.   
		if (!strncmp(brush->brush_faces->d_texture->name, "*", 1) ||
			!strcmp(brush->brush_faces->d_texture->name, "clip") ||
			!strcmp(brush->brush_faces->d_texture->name, "trigger"))
			g_pbrTransBrushes[g_nNumTransBrushes++] = brush;
		else 
			Brush_Draw(brush);
	}

	glEnable(GL_BLEND);
	// I don't know enough about GL to do this correctly but the darker
	// the pixel the less transparent. I've tried many combinations and
	// this proved the best so it'll have to do for now.
	glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_COLOR);
	for (i = 0; i < g_nNumTransBrushes; i++ ) 
		Brush_Draw(g_pbrTransBrushes[i]);
	glDisable(GL_BLEND);
// <---sikk

	glMatrixMode(GL_PROJECTION);

// sikk---> Show Axis in center of view
	// TODO: Display Axis in lower left corner of window and rotate with camera orientation (e.g. blender)
	if (g_qeglobals.d_savedinfo.bShow_Axis)
	{
		glColor3f(1.0, 0.0, 0.0);
		glBegin(GL_LINES);
		glVertex2i(-g_qeglobals.d_savedinfo.nMapSize / 2, 0);
		glVertex2i(g_qeglobals.d_savedinfo.nMapSize / 2, 0);
		glEnd();
				
		glColor3f(0.0, 1.0, 0.0);
		glBegin(GL_LINES);
		glVertex2i(0, -g_qeglobals.d_savedinfo.nMapSize / 2);
		glVertex2i(0, g_qeglobals.d_savedinfo.nMapSize / 2);
		glEnd();

		glColor3f(0.0, 0.0, 1.0);
		glBegin(GL_LINES);
		glVertex3i(0, 0, -g_qeglobals.d_savedinfo.nMapSize / 2);
		glVertex3i(0, 0, g_qeglobals.d_savedinfo.nMapSize / 2);
		glEnd();
	}
// <---sikk

// sikk---> Show Map Boundry Box
	if (g_qeglobals.d_savedinfo.bShow_MapBoundry)
	{
		glColor3fv(g_qeglobals.d_savedinfo.v3Colors[COLOR_MAPBOUNDRY]);
		glBegin(GL_LINE_LOOP);
		glVertex3f(-g_qeglobals.d_savedinfo.nMapSize * 0.5, -g_qeglobals.d_savedinfo.nMapSize * 0.5, -g_qeglobals.d_savedinfo.nMapSize * 0.5);
		glVertex3f(g_qeglobals.d_savedinfo.nMapSize * 0.5, -g_qeglobals.d_savedinfo.nMapSize * 0.5, -g_qeglobals.d_savedinfo.nMapSize * 0.5);
		glVertex3f(g_qeglobals.d_savedinfo.nMapSize * 0.5, g_qeglobals.d_savedinfo.nMapSize * 0.5, -g_qeglobals.d_savedinfo.nMapSize * 0.5);
		glVertex3f(-g_qeglobals.d_savedinfo.nMapSize * 0.5, g_qeglobals.d_savedinfo.nMapSize * 0.5, -g_qeglobals.d_savedinfo.nMapSize * 0.5);
		glEnd();

		glBegin(GL_LINE_LOOP);
		glVertex3f(-g_qeglobals.d_savedinfo.nMapSize * 0.5, -g_qeglobals.d_savedinfo.nMapSize * 0.5, g_qeglobals.d_savedinfo.nMapSize * 0.5);
		glVertex3f(g_qeglobals.d_savedinfo.nMapSize * 0.5, -g_qeglobals.d_savedinfo.nMapSize * 0.5, g_qeglobals.d_savedinfo.nMapSize * 0.5);
		glVertex3f(g_qeglobals.d_savedinfo.nMapSize * 0.5, g_qeglobals.d_savedinfo.nMapSize * 0.5, g_qeglobals.d_savedinfo.nMapSize * 0.5);
		glVertex3f(-g_qeglobals.d_savedinfo.nMapSize * 0.5, g_qeglobals.d_savedinfo.nMapSize * 0.5, g_qeglobals.d_savedinfo.nMapSize * 0.5);
		glEnd();

		glBegin(GL_LINES);
		glVertex3f(-g_qeglobals.d_savedinfo.nMapSize * 0.5, -g_qeglobals.d_savedinfo.nMapSize * 0.5, -g_qeglobals.d_savedinfo.nMapSize * 0.5);
		glVertex3f(-g_qeglobals.d_savedinfo.nMapSize * 0.5, -g_qeglobals.d_savedinfo.nMapSize * 0.5, g_qeglobals.d_savedinfo.nMapSize * 0.5);
		glVertex3f(g_qeglobals.d_savedinfo.nMapSize * 0.5, -g_qeglobals.d_savedinfo.nMapSize * 0.5, -g_qeglobals.d_savedinfo.nMapSize * 0.5);
		glVertex3f(g_qeglobals.d_savedinfo.nMapSize * 0.5, -g_qeglobals.d_savedinfo.nMapSize * 0.5, g_qeglobals.d_savedinfo.nMapSize * 0.5);
		glVertex3f(-g_qeglobals.d_savedinfo.nMapSize * 0.5, g_qeglobals.d_savedinfo.nMapSize * 0.5, -g_qeglobals.d_savedinfo.nMapSize * 0.5);
		glVertex3f(-g_qeglobals.d_savedinfo.nMapSize * 0.5, g_qeglobals.d_savedinfo.nMapSize * 0.5, g_qeglobals.d_savedinfo.nMapSize * 0.5);
		glVertex3f(g_qeglobals.d_savedinfo.nMapSize * 0.5, g_qeglobals.d_savedinfo.nMapSize * 0.5, -g_qeglobals.d_savedinfo.nMapSize * 0.5);
		glVertex3f(g_qeglobals.d_savedinfo.nMapSize * 0.5, g_qeglobals.d_savedinfo.nMapSize * 0.5, g_qeglobals.d_savedinfo.nMapSize * 0.5);
		glEnd();
	}
// <---sikk

	//
	// now draw selected brushes
	//
	glTranslatef(g_qeglobals.d_v3SelectTranslate[0], 
				 g_qeglobals.d_v3SelectTranslate[1], 
				 g_qeglobals.d_v3SelectTranslate[2]);
	glMatrixMode(GL_TEXTURE);

	Cam_DrawClipSplits();

	pList = (g_qeglobals.d_bClipMode && g_qeglobals.d_pbrSplitList) ? g_qeglobals.d_pbrSplitList : &g_brSelectedBrushes;

	// draw normally
	for (brush = pList->next; brush != pList; brush = brush->next)
		Brush_Draw(brush);

	// blend on top
	glMatrixMode(GL_PROJECTION);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_BLEND);
//	glColor4f(1.0f, 0.0f, 0.0f, 0.3f);
//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// lunaran: brighten & clarify selection tint, use selection color preference
	glColor4f(g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES][0],
			g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES][1],
			g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES][2],
			0.5f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDisable(GL_TEXTURE_2D);

	for (brush = pList->next; brush != pList; brush = brush->next)
		for (face = brush->brush_faces; face; face = face->next)
			Face_Draw(face);

//	if (g_pfaceSelectedFace)
//		Face_Draw(g_pfaceSelectedFace);
// sikk---> Multiple Face Selection
//	if (Select_HasFaces())
//	{
		for (int i = 0; i < Select_NumFaces(); i++)
			Face_Draw(g_pfaceSelectedFaces[i]);
//	}
// <---sikk

	// non-zbuffered outline
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glColor3f(1, 1, 1);

	for (brush = pList->next; brush != pList; brush = brush->next)
		for (face = brush->brush_faces; face; face = face->next)
			Face_Draw(face);

	// edge / vertex flags
	if (g_qeglobals.d_selSelectMode == sel_vertex)
	{
		glPointSize(4);
		glColor3f(0, 1, 0);
		glBegin(GL_POINTS);
		for (i = 0; i < g_qeglobals.d_nNumPoints; i++)
			glVertex3fv(g_qeglobals.d_v3Points[i]);
		glEnd();
		glPointSize(1);
	}
	else if (g_qeglobals.d_selSelectMode == sel_edge)
	{
		float *v1, *v2;

		glPointSize(4);
		glColor3f(0, 0, 1);
		glBegin (GL_POINTS);
		for (i = 0; i < g_qeglobals.d_nNumEdges; i++)
		{
			v1 = g_qeglobals.d_v3Points[g_qeglobals.d_pEdges[i].p1];
			v2 = g_qeglobals.d_v3Points[g_qeglobals.d_pEdges[i].p2];
			glVertex3f((v1[0] + v2[0]) * 0.5, (v1[1] + v2[1]) * 0.5, (v1[2] + v2[2]) * 0.5);
		}
		glEnd();
		glPointSize(1);
	}

	//
	// draw pointfile
	//
	glEnable(GL_DEPTH_TEST);

	DrawPathLines();

	if (g_qeglobals.d_nPointfileDisplayList)
	{
		Pointfile_Draw();
//		glCallList(g_qeglobals.d_nPointfileDisplayList);
	}

	// bind back to the default texture so that we don't have problems
	// elsewhere using/modifying texture maps between contexts
	glBindTexture(GL_TEXTURE_2D, 0);

    glFinish();
	QE_CheckOpenGLForErrors();
//	Sys_EndWait();
	if (g_qeglobals.d_camera.timing)
	{
		end = Sys_DoubleTime();
		Sys_Printf("MSG: Camera: %d ms\n", (int)(1000 * (end - start)));
	}
}
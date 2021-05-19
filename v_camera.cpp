//==============================
//	camera.c
//==============================

#include "qe3.h"


// sikk---> Transparent Brushes
Brush		*g_pbrTransBrushes[MAX_MAP_BRUSHES];
int			g_nNumTransBrushes;
// <---sikk


/*
============
CameraView::Init
============
*/
void CameraView::Init ()
{
	timing = false;
	origin[0] = 0;
	origin[1] = 0;	// sikk - changed from "20"
	origin[2] = 0;	// sikk - changed from "46"	
	viewdistance = 256;
}

// sikk---> Center Camera on Selection. Same as PositionView() for XY View
/*
==================
CameraView::PositionCenter
==================
*/
void CameraView::PositionCenter ()
{
	Brush *b;

	b = g_brSelectedBrushes.next;
	if (b && b->next != b)
	{
		origin[0] = b->mins[0] - 64;
		origin[1] = b->mins[1] - 64;
		origin[2] = b->mins[2] + 64;
		angles[0] = -22.5;
		angles[1] = 45;
		angles[2] = 0;
	}
	else
	{
		origin[0] = origin[0];
		origin[1] = origin[1];
		origin[2] = origin[2];
	}
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
	float	xa, ya;
	float	matrix[4][4];

	xa = angles[0] / 180 * Q_PI;
	ya = angles[1] / 180 * Q_PI;

	// the movement matrix is kept 2d
    forward[0] = cos(ya);
    forward[1] = sin(ya);
    right[0] = forward[1];
    right[1] = -forward[0];

	glGetFloatv(GL_PROJECTION_MATRIX, &matrix[0][0]);

	for (i = 0; i < 3; i++)
	{
		vright[i] = matrix[i][0];
		vup[i] = matrix[i][1];
		vpn[i] = matrix[i][2];
	}

	VectorNormalize(vright);
	VectorNormalize(vup);
	VectorNormalize(vpn);
}

/*
===============
CameraView::ChangeFloor
===============
*/
void CameraView::ChangeFloor (bool up)
{
	float		d, bestd, current;
	vec3_t		start, dir;
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

	for (b = g_brActiveBrushes.next; b != &g_brActiveBrushes; b = b->next)
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

	Sys_UpdateWindows(W_CAMERA | W_Z);
}


//===============================================

/*
================
CameraView::PositionDrag
================
*/
void CameraView::PositionDrag ()
{
	int	x, y;

	SetCursor(NULL); // sikk - Remove Cursor
	Sys_GetCursorPos(&x, &y);

	if (x != cursorX || y != cursorY)
	{
		x -= cursorX;
		VectorMA(origin, x, vright, origin);
		y -= cursorY;
		origin[2] -= y;

		Sys_SetCursorPos(cursorX, cursorY);
		Sys_UpdateWindows(W_CAMERA | W_XY);
	}
}

/*
================
CameraView::Rotate
================
*/
void CameraView::Rotate (int yaw, int pitch, vec3_t org)
{
	int		i;
	vec_t	distance;
	vec3_t	work, forward, dir, vecdist;

	for (i = 0; i < 3; i++)
	{
		vecdist[i] = fabs((origin[i] - org[i]));
		vecdist[i] *= vecdist[i];
	}

	viewdistance = distance = sqrt(vecdist[0] + vecdist[1] + vecdist[2]);
	VectorSubtract(origin, org, work);
	VectorToAngles(work, angles);

	if(angles[PITCH] > 100)
		angles[PITCH] -= 360;

	angles[PITCH] -= pitch;
	angles[YAW] -= yaw;
	BoundAngles();

	AngleVectors(angles, forward, NULL, NULL);
	forward[2] = -forward[2];
	VectorMA(org, distance, forward, origin);

	VectorSubtract(org, origin, dir);
	VectorNormalize(dir);
	angles[1] = atan2(dir[1], dir[0]) * 180 / Q_PI;
	angles[0] = asin(dir[2]) * 180 / Q_PI;

	BuildMatrix();

	Sys_SetCursorPos(cursorX, cursorY);
	Sys_UpdateWindows(W_XY | W_CAMERA | W_Z);
}

/*
==================
CameraView::PositionRotate
==================
*/
void CameraView::PositionRotate ()
{
	int			x, y, i, j;
	vec3_t		mins, maxs, forward, vecdist;
	vec3_t		sorigin;
	Brush	   *b;
	Face	   *f;

	SetCursor(NULL); // sikk - Remove Cursor
	Sys_GetCursorPos(&x, &y);

	if (x == g_qeglobals.d_camera.cursorX && y == g_qeglobals.d_camera.cursorY)
		return;

	x -= g_qeglobals.d_camera.cursorX;
	y -= g_qeglobals.d_camera.cursorY;

	if (Select_HasBrushes())
	{
		ClearBounds(mins, maxs);
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
		sorigin[0] = (mins[0] + maxs[0]) / 2;
		sorigin[1] = (mins[1] + maxs[1]) / 2;
		sorigin[2] = (mins[2] + maxs[2]) / 2;
	}
	else if (Select_FaceCount())
	{
		ClearBounds(mins, maxs);

		//		f = g_pfaceSelectedFace;
		// rotate around last selected face
		f = g_pfaceSelectedFaces[Select_FaceCount() - 1];	// sikk - Multiple Face Selection
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

		sorigin[0] = (mins[0] + maxs[0]) / 2;
		sorigin[1] = (mins[1] + maxs[1]) / 2;
		sorigin[2] = (mins[2] + maxs[2]) / 2;
	}
	else
	{
		AngleVectors(g_qeglobals.d_camera.angles, forward, NULL, NULL);
		forward[2] = -forward[2];
		VectorMA(g_qeglobals.d_camera.origin, g_qeglobals.d_camera.viewdistance, forward, sorigin);
	}

	for (i = 0; i < 3; i++)
	{
		vecdist[i] = fabs((g_qeglobals.d_camera.origin[i] - sorigin[i]));
		vecdist[i] *= vecdist[i];
	}

	Rotate(x, y, sorigin);
}

/*
================
CameraView::BoundAngles
================
*/
void CameraView::BoundAngles()
{
//	angles[YAW] = fmod(angles[YAW], 360.0f);

	angles[PITCH] = fmin(angles[PITCH], 90);
	angles[PITCH] = fmax(angles[PITCH], -90);
}

/*
================
CameraView::FreeLook
================
*/
void CameraView::FreeLook ()
{
	int	x, y;

	SetCursor(NULL); // sikk - Remove Cursor
	Sys_GetCursorPos (&x, &y);

	if (x == cursorX && y == cursorY)
		return;
	
	x -= cursorX;
	y -= cursorY;

	if (angles[PITCH] > 100)
		angles[PITCH] -= 360;

	angles[PITCH] -= y;
	angles[YAW] -= x;

	BoundAngles();

	Sys_SetCursorPos(cursorX, cursorY);
	Sys_UpdateWindows(W_XY | W_CAMERA | W_Z);
}




/*
================
CameraView::MouseControl
================
*/
void CameraView::MouseControl (float dtime)
{
	int		xl, xh;
	int		yl, yh;
	float	xf, yf;

	if (nCamButtonState != MK_RBUTTON)
		return;

	xf = (float)(buttonX - width / 2) / (width / 2);
	yf = (float)(buttonY - height / 2) / (height / 2);

	xl = width / 3;
	xh = xl * 2;
	yl = height / 3;
	yh = yl * 2;

#if 0
	// strafe
	if (buttonY < yl && (buttonX < xl || buttonX > xh))
		VectorMA(origin, xf * dtime * g_qeglobals.d_savedinfo.nCameraSpeed, right, origin);
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
		
		VectorMA(origin, yf * dtime * g_qeglobals.d_savedinfo.nCameraSpeed, forward, origin);
		angles[YAW] += xf * -dtime * (g_qeglobals.d_savedinfo.nCameraSpeed * 0.5);
	}
	Sys_UpdateWindows(W_CAMERA | W_XY);
}

/*
==============
CameraView::PointToRay
==============
*/
void CameraView::PointToRay(int x, int y, vec3_t rayOut)
{
	float	f, r, u;

	// calc ray direction
	u = (float)(y - height / 2) / (width / 2);
	r = (float)(x - width / 2) / (width / 2);
	f = 1;

	for (int i = 0; i < 3; i++)
		rayOut[i] = vpn[i] * f + vright[i] * r + vup[i] * u;
	VectorNormalize(rayOut);
}

/*
==============
CameraView::MouseDown
==============
*/
void CameraView::MouseDown (int x, int y, int buttons)
{
	vec3_t	dir;
	PointToRay(x, y, dir);

	Sys_GetCursorPos(&cursorX, &cursorY);

	nCamButtonState = buttons;
	buttonX = x;
	buttonY = y;

	// clipper
	if ((buttons & MK_LBUTTON) && g_qeglobals.d_bClipMode)
	{
		if (Drag_TrySelect(buttons, origin, dir))
			return;

		// lunaran - alt quick clip
		if (GetKeyState(VK_MENU) < 0)
			Clip_CamStartQuickClip(x, y);
		else
			Clip_CamDropPoint(x, y);
		Sys_UpdateWindows(W_XY|W_CAMERA);
	}

	// LBUTTON = manipulate selection
	// shift-LBUTTON = select
	// middle button = grab texture
	// ctrl-middle button = set entire brush to texture
	// ctrl-shift-middle button = set single face to texture
	else if ((buttons == MK_LBUTTON)	|| 
		(buttons == (MK_LBUTTON | MK_SHIFT)) || 
		(buttons == (MK_LBUTTON | MK_CONTROL)) || 
		(buttons == (MK_LBUTTON | MK_CONTROL | MK_SHIFT)) || 
		(buttons == MK_MBUTTON) || 
		(buttons == (MK_MBUTTON | MK_SHIFT)) || 
		(buttons == (MK_MBUTTON | MK_CONTROL)) || 
		(buttons == (MK_MBUTTON | MK_CONTROL | MK_SHIFT)))
	{
		Drag_Begin(x, y, buttons, vright, vup, origin, dir);
		return;
	}

	if ((buttons == MK_RBUTTON))
	{
		MouseControl(0.1f);
		return;
	}
}

/*
==============
CameraView::MouseUp
==============
*/
void CameraView::MouseUp (int x, int y, int buttons)
{
	// clipper
	if (g_qeglobals.d_bClipMode)
	{
		// lunaran - alt quick clip
		if (GetKeyState(VK_MENU) < 0)
			Clip_CamEndQuickClip();
		else
			Clip_CamEndPoint();
	}
	else
	{
		Drag_MouseUp();
	}
	Sys_UpdateWindows(W_ALL);
	nCamButtonState = 0;
}

/*
==============
CameraView::MouseMoved
==============
*/
void CameraView::MouseMoved (int x, int y, int buttons)
{
	int			i;
	float		f, r, u;
	char		camstring[256];
	vec3_t		dir;
	trace_t		t;

	nCamButtonState = buttons;

	if ((!buttons || buttons & MK_LBUTTON) && g_qeglobals.d_bClipMode)
	{
		Clip_CamMovePoint(x, y);
	}
	else if (!buttons)
	{
		//
		// calc ray direction
		//
		u = (float)(y - height / 2) / (width / 2);
		r = (float)(x - width / 2) / (width / 2);
		f = 1;

		for (i = 0; i < 3; i++)
			dir[i] = vpn[i] * f + vright[i] * r + vup[i] * u;
		VectorNormalize(dir);
		t = Test_Ray(origin, dir, false);

		if (t.brush)
		{
			Brush	   *b;
			int			i;
			vec3_t		mins, maxs, size;

			ClearBounds(mins, maxs);

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

	buttonX = x;
	buttonY = y;

	if (buttons == (MK_RBUTTON | MK_CONTROL))
	{
		PositionDrag();
		Sys_UpdateWindows(W_XY | W_CAMERA | W_Z);
		return;
	}

	if (buttons == (MK_RBUTTON | MK_SHIFT))
	{
		PositionRotate();
		Sys_UpdateWindows(W_XY | W_CAMERA | W_Z);
		return;
	}

	if (buttons == (MK_RBUTTON | MK_CONTROL | MK_SHIFT))
	{
		FreeLook();
		Sys_UpdateWindows(W_XY | W_CAMERA | W_Z);
		return;
	}

// sikk---> Mouse Driven Texture Manipulation - TODO: Too sensitive...
	if (buttons & MK_LBUTTON  && !g_qeglobals.d_bClipMode)
	{			
		if (GetKeyState(VK_MENU) < 0)
		{
			Sys_GetCursorPos(&x, &y);
			/*
			if (GetKeyState(VK_SHIFT) < 0)
			{
				if (GetKeyState(VK_CONTROL) < 0)
					Surf_RotateTexture(cursorX - x);
			}
			else if (GetKeyState(VK_CONTROL) < 0)
				Surf_ScaleTexture(x - cursorX, cursorY - y);

			else
				Surf_ShiftTexture(cursorX - x, cursorY - y);
			*/
			cursorX = x;
			cursorY = y;
		}
// <---sikk
		else
		{
			Sys_GetCursorPos(&cursorX, &cursorY);
			Drag_MouseMoved(x, y, buttons);
			Sys_UpdateWindows(W_XY | W_CAMERA | W_Z);
		}
	}
}

/*
============
CameraView::InitCull
============
*/
void CameraView::InitCull ()
{
	int	i;

	VectorSubtract(vpn, vright, v3Cull1);
	VectorAdd(vpn, vright, v3Cull2);

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
	vec3_t	point;

// sikk---> Cubic Clipping
	if (g_qeglobals.d_savedinfo.bCubicClip)
	{
		float fLevel = g_qeglobals.d_savedinfo.nCubicScale * 64;

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

	for (i = 0; i < 3; i++)
		point[i] = b->mins[nCullv1[i]] - origin[i];

	d = DotProduct(point, v3Cull1);
	if (d < -1)
		return true;

	for (i = 0; i < 3; i++)
		point[i] = b->mins[nCullv2[i]] - origin[i];

	d = DotProduct(point, v3Cull2);
	if (d < -1)
		return true;

	return false;
}

/*
================
CameraView::DrawClipSplits
================
*/
void CameraView::DrawClipSplits ()
{
	g_qeglobals.d_pbrSplitList = NULL;

	if (g_qeglobals.d_bClipMode)
		Clip_ProduceSplitLists();
		if (g_cpClip1.bSet && g_cpClip2.bSet)
			g_qeglobals.d_pbrSplitList = (g_qeglobals.d_bClipSwitch) ? &g_qeglobals.d_brFrontSplits : &g_qeglobals.d_brBackSplits;
}

// sikk---> Camera Grid
/*
==============
CameraView::DrawGrid
==============
*/
void CameraView::DrawGrid ()
{
	if (!g_qeglobals.d_savedinfo.bShow_CameraGrid)
		return;

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
CameraView::Draw
==============
*/
void CameraView::Draw ()
{
	int			i;
	double		start, end;
	float		screenaspect;
	float		yfov;
	Brush    *pList;
	Brush	   *brush;
	Face	   *face;

	if (!g_brActiveBrushes.next)
		return;	// not valid yet

	if (timing)
		start = Sys_DoubleTime();

	// clear
	QE_CheckOpenGLForErrors();

	glViewport(0, 0, width, height);
	glScissor(0, 0, width, height);
	glClearColor(g_qeglobals.d_savedinfo.v3Colors[COLOR_CAMERABACK][0],
				 g_qeglobals.d_savedinfo.v3Colors[COLOR_CAMERABACK][1],
				 g_qeglobals.d_savedinfo.v3Colors[COLOR_CAMERABACK][2],
				 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// set up viewpoint
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

	screenaspect = (float)width / height;
	yfov = 2 * atan((float)height / width) * 180 / Q_PI;
    gluPerspective(yfov, screenaspect, 2, g_qeglobals.d_savedinfo.nMapSize);//8192);

    glRotatef(-90, 1, 0, 0);	// put Z going up
    glRotatef(90, 0, 0, 1);		// put Z going up
    glRotatef(angles[0], 0, 1, 0);
    glRotatef(-angles[1], 0, 0, 1);
    glTranslatef(-origin[0], -origin[1], -origin[2]);

	BuildMatrix();
	InitCull();

	// draw stuff
	switch (draw_mode)
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
	DrawGrid();
// <---sikk

	glMatrixMode(GL_TEXTURE);

	g_nNumTransBrushes = 0;	// sikk - Transparent Brushes

	for (brush = g_brActiveBrushes.next; brush != &g_brActiveBrushes; brush = brush->next)
	{
		if (CullBrush(brush))
			continue;
		if (brush->IsFiltered())
			continue;
// sikk---> Transparent Brushes
		// TODO: Make toggle via Preferences Option. 
		assert(brush->brush_faces->d_texture);
		if (!strncmp(brush->brush_faces->d_texture->name, "*", 1) ||
			!strcmp(brush->brush_faces->d_texture->name, "clip") ||
			!strncmp(brush->brush_faces->d_texture->name, "hint", 4) ||
			!strcmp(brush->brush_faces->d_texture->name, "trigger"))
			g_pbrTransBrushes[g_nNumTransBrushes++] = brush;
		else
		{
			brush->Draw();
		}
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	for (i = 0; i < g_nNumTransBrushes; i++ )
		g_pbrTransBrushes[i]->Draw();
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

	// now draw selected brushes
	glTranslatef(g_qeglobals.d_v3SelectTranslate[0], 
				 g_qeglobals.d_v3SelectTranslate[1], 
				 g_qeglobals.d_v3SelectTranslate[2]);
	glMatrixMode(GL_TEXTURE);

//	Cam_DrawClipSplits();
//	pList = (g_qeglobals.d_bClipMode && g_qeglobals.d_pbrSplitList) ? g_qeglobals.d_pbrSplitList : &g_brSelectedBrushes;
	pList = &g_brSelectedBrushes;

	// draw normally
	for (brush = pList->next; brush != pList; brush = brush->next)
		brush->Draw();

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
			0.3f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDisable(GL_TEXTURE_2D);

	for (brush = pList->next; brush != pList; brush = brush->next)
		for (face = brush->brush_faces; face; face = face->next)
			face->Draw();

//	if (g_pfaceSelectedFace)
//		Face_Draw(g_pfaceSelectedFace);
// sikk---> Multiple Face Selection
//	if (Select_FaceCount())
//	{
		for (int i = 0; i < Select_FaceCount(); i++)
			g_pfaceSelectedFaces[i]->Draw();
//	}
// <---sikk

	// non-zbuffered outline
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glColor3f(1, 1, 1);

	for (brush = pList->next; brush != pList; brush = brush->next)
		for (face = brush->brush_faces; face; face = face->next)
			face->Draw();

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

	Clip_DrawPoints();

	glEnable(GL_DEPTH_TEST);

	DrawPathLines();

	if (g_qeglobals.d_nPointfileDisplayList)
		Pointfile_Draw();

	// bind back to the default texture so that we don't have problems
	// elsewhere using/modifying texture maps between contexts
	glBindTexture(GL_TEXTURE_2D, 0);

    glFinish();
	QE_CheckOpenGLForErrors();
//	Sys_EndWait();
	if (timing)
	{
		end = Sys_DoubleTime();
		Sys_Printf("MSG: Camera: %d ms\n", (int)(1000 * (end - start)));
	}
}
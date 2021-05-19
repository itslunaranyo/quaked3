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
	origin = vec3(0);
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

	mpUp = AxisForVector(vup);
	mpRight = AxisForVector(vright);
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

	for (b = g_map.brActive.next; b != &g_map.brActive; b = b->next)
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
		origin = origin + (float)x * vright;
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
void CameraView::Rotate (int yaw, int pitch, const vec3 org)
{
	int		i;
	float	distance;
	vec3	work, forward, dir, vecdist;

	for (i = 0; i < 3; i++)
	{
		vecdist[i] = fabs((origin[i] - org[i]));
		vecdist[i] *= vecdist[i];
	}

	viewdistance = distance = sqrt(vecdist[0] + vecdist[1] + vecdist[2]);
	work = origin - org;
	VectorToAngles(work, angles);

	if(angles[PITCH] > 100)
		angles[PITCH] -= 360;

	angles[PITCH] -= pitch;
	angles[YAW] -= yaw;
	BoundAngles();

	AngleVectors(angles, forward, vec3(0), vec3(0));
	forward[2] = -forward[2];
	origin = org + distance * forward;

	dir = org - origin;
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
	vec3		mins, maxs, forward, vecdist;
	vec3		sorigin;
	Brush	   *b;
	Face	   *f;

	SetCursor(NULL); // sikk - Remove Cursor
	Sys_GetCursorPos(&x, &y);

	if (x == g_qeglobals.d_vCamera.cursorX && y == g_qeglobals.d_vCamera.cursorY)
		return;

	x -= g_qeglobals.d_vCamera.cursorX;
	y -= g_qeglobals.d_vCamera.cursorY;

	if (Selection::HasBrushes())
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
		sorigin = (mins + maxs) / 2.0f;
	}
	else if (Selection::NumFaces())
	{
		ClearBounds(mins, maxs);

		//		f = g_pfaceSelectedFace;
		// rotate around last selected face
		//f = g_vfSelectedFaces[Selection::NumFaces() - 1];	// sikk - Multiple Face Selection
		f = Selection::faces.back();
		for (j = 0; j < f->face_winding->numpoints; j++)
		{
			for (i = 0; i < 3; i++)
			{
				if (f->face_winding->points[j].point[i] > maxs[i])
					maxs[i] = f->face_winding->points[j].point[i];
				if (f->face_winding->points[j].point[i] < mins[i])
					mins[i] = f->face_winding->points[j].point[i];
			}
		}

		sorigin = (mins + maxs) / 2.0f;
	}
	else
	{
		AngleVectors(g_qeglobals.d_vCamera.angles, forward, vec3(0), vec3(0));
		forward[2] = -forward[2];
		sorigin = g_qeglobals.d_vCamera.origin + g_qeglobals.d_vCamera.viewdistance * forward;
	}

	for (i = 0; i < 3; i++)
	{
		vecdist[i] = fabs((g_qeglobals.d_vCamera.origin[i] - sorigin[i]));
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

	angles[PITCH] = min(angles[PITCH], 90.0f);
	angles[PITCH] = max(angles[PITCH], -90.0f);
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
		origin = origin + xf * dtime * g_qeglobals.d_savedinfo.nCameraSpeed * right;
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
		
		origin = origin + yf * dtime * g_qeglobals.d_savedinfo.nCameraSpeed * forward;
		angles[YAW] += xf * -dtime * (g_qeglobals.d_savedinfo.nCameraSpeed * 0.5);
	}
	Sys_UpdateWindows(W_CAMERA | W_XY);
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
	u = (float)(y - height / 2) / (width / 2);
	r = (float)(x - width / 2) / (width / 2);
	f = 1;

	for (int i = 0; i < 3; i++)
		rayOut[i] = vpn[i] * f + vright[i] * r + vup[i] * u;
	VectorNormalize(rayOut);
}



bool CameraView::GetBasis(vec3 &_right, vec3 &_up, vec3 &_forward)
{
	_right = vright;
	_up = vup;
	_forward = vpn;
	return true;
}

mouseContext_t const CameraView::GetMouseContext(const int x, const int y)
{
	mouseContext_t mc;

	mc.org = origin;
	mc.up = mpUp;
	mc.right = mpRight;
	PointToRay(x, y, mc.ray);
	mc.scale = 1;	// what use?
	mc.dims = 3;	// 3D view
	return mc;
}

/*
==============
CameraView::MouseDown
==============
*/
void CameraView::MouseDown (int x, int y, int buttons)
{
	vec3	dir;
	PointToRay(x, y, dir);

	Sys_GetCursorPos(&cursorX, &cursorY);

	nCamButtonState = buttons;
	buttonX = x;
	buttonY = y;
	
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
		Drag_Begin(x, y, buttons, vright, vup, origin, dir);
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
	Drag_MouseUp();
	Sys_UpdateWindows(W_SCENE);
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
	vec3		dir;
	trace_t		t;

	nCamButtonState = buttons;

	if (!buttons)
	{
		// calc ray direction
		u = (float)(y - height / 2) / (width / 2);
		r = (float)(x - width / 2) / (width / 2);
		f = 1;

		for (i = 0; i < 3; i++)
			dir[i] = vpn[i] * f + vright[i] * r + vup[i] * u;
		VectorNormalize(dir);
		t = Selection::TestRay(origin, dir, false);

		if (t.brush)
		{
			vec3	size;
			/*
			Brush	*b;
			vec3	mins, maxs;

			ClearBounds(mins, maxs);
			b = t.brush;
			for (i = 0; i < 3; i++)
			{
				if (b->mins[i] < mins[i])
					mins[i] = b->mins[i];
				if (b->maxs[i] > maxs[i])
					maxs[i] = b->maxs[i];
			}
			size = maxs - mins;
			*/
			size = t.brush->maxs - t.brush->mins;
			if (t.brush->owner->IsPoint())
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
	if (buttons & MK_LBUTTON)
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
		point[i] = ((float*)&b->mins)[nCullv1[i]] - origin[i];	// lunaran: FIXME this hack

	d = DotProduct(point, v3Cull1);
	if (d < -1)
		return true;

	for (i = 0; i < 3; i++)
		point[i] = ((float*)&b->mins)[nCullv2[i]] - origin[i];	// lunaran: FIXME this hack too

	d = DotProduct(point, v3Cull2);
	if (d < -1)
		return true;

	return false;
}

// sikk---> Camera Grid
/*
==============
CameraView::Draw2D
==============
*/
void CameraView::DrawGrid ()
{
	if (!g_qeglobals.d_savedinfo.bShow_CameraGrid)
		return;

	int x, y, i;

	i = g_qeglobals.d_savedinfo.nMapSize * 0.5;

	glColor3fv(&g_qeglobals.d_savedinfo.v3Colors[COLOR_CAMERAGRID].r);
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

draw active (unselected) brushes, sorting out the transparent ones for a separate draw
==============
*/
void CameraView::DrawActive()
{
	int i, numTransBrushes = 0;

	for (Brush *brush = g_map.brActive.next; brush != &g_map.brActive; brush = brush->next)
	{
		if (brush->IsFiltered())
			continue;
		if (CullBrush(brush))
			continue;
	// sikk---> Transparent Brushes
		// TODO: Make toggle via Preferences Option. 
		assert(brush->faces->texdef.tex);
		/*
		if (!strncmp(brush->faces->texdef.tex->name, "*", 1) ||
			!strcmp(brush->faces->texdef.tex->name, "clip") ||
			!strncmp(brush->faces->texdef.tex->name, "hint", 4) ||
			!strcmp(brush->faces->texdef.tex->name, "trigger"))
			*/
		if (brush->showFlags & BFL_TRANS)
			g_pbrTransBrushes[numTransBrushes++] = brush;
		else
		{
			brush->Draw();
		}
	}

	// draw the transparent brushes
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	for (i = 0; i < numTransBrushes; i++ )
		g_pbrTransBrushes[i]->Draw();
	glDisable(GL_BLEND);
	// <---sikk
}

/*
==============
CameraView::DrawSelected

draw selected brushes textured, then in red, then in wireframe
==============
*/
void CameraView::DrawSelected(Brush	*pList)
{
	Brush	*brush;
	Face	*face;

	// Draw selected brushes
	glMatrixMode(GL_PROJECTION);
	/*
	glTranslatef(g_qeglobals.d_v3SelectTranslate[0], 
				 g_qeglobals.d_v3SelectTranslate[1], 
				 g_qeglobals.d_v3SelectTranslate[2]);
				 */
	// draw brushes first normally
	for (brush = pList->next; brush != pList; brush = brush->next)
		brush->Draw();

	// redraw tint on top
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_BLEND);

	// lunaran: brighten & clarify selection tint, use selection color preference
	glColor4f(g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES][0],
			g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES][1],
			g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES][2],
			0.3f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDisable(GL_TEXTURE_2D);

	// fully selected brushes, then loose faces
	for (brush = pList->next; brush != pList; brush = brush->next)
		for (face = brush->faces; face; face = face->fnext)
			face->Draw();

	for (auto fIt = Selection::faces.begin(); fIt != Selection::faces.end(); ++fIt)
		(*fIt)->Draw();

	// non-zbuffered outline
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glColor3f(1, 1, 1);

	for (brush = pList->next; brush != pList; brush = brush->next)
		for (face = brush->faces; face; face = face->fnext)
			face->Draw();
}

/*
==============
CameraView::DrawTools
==============
*/
bool CameraView::DrawTools()
{
	for (auto tIt = g_qeglobals.d_tools.rbegin(); tIt != g_qeglobals.d_tools.rend(); ++tIt)
	{
		if ((*tIt)->Draw3D(*this))
			return true;
	}
	return false;
}

/*
==============
CameraView::Draw
==============
*/
void CameraView::Draw ()
{
	int		i;
	double	start, end;
	float	screenaspect;
	float	yfov;

	if (!g_map.brActive.next)
		return;	// not valid yet

	if (timing)
		start = Sys_DoubleTime();

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

// sikk---> Camera Grid/Axis/Map Boundry Box
	DrawGrid();

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

	if (g_qeglobals.d_savedinfo.bShow_MapBoundry)
	{
		glColor3fv(&g_qeglobals.d_savedinfo.v3Colors[COLOR_MAPBOUNDRY].r);
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

	//glMatrixMode(GL_TEXTURE);

	// ----------------------------------------------------------------

	DrawActive();
	if (!DrawTools())
		DrawSelected(&g_brSelectedBrushes);

	// draw edge / vertex markers in edge/vert mode
	// lunaran TODO: move this into a vert/edge tool once it exists
	if (g_qeglobals.d_selSelectMode == sel_vertex)
	{
		glPointSize(4);
		glColor3f(0, 1, 0);
		glBegin(GL_POINTS);
		for (i = 0; i < g_qeglobals.d_nNumPoints; i++)
			glVertex3fv(&g_qeglobals.d_v3Points[i].x);
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
			v1 = &g_qeglobals.d_v3Points[g_qeglobals.d_pEdges[i].p1].x;
			v2 = &g_qeglobals.d_v3Points[g_qeglobals.d_pEdges[i].p2].x;
			glVertex3f((v1[0] + v2[0]) * 0.5, (v1[1] + v2[1]) * 0.5, (v1[2] + v2[2]) * 0.5);
		}
		glEnd();
		glPointSize(1);
	}

	// ----------------------------------------------------------------

	glEnable(GL_DEPTH_TEST);

	DrawPathLines();

	if (g_qeglobals.d_nPointfileDisplayList)
		Pointfile_Draw();

	// bind back to the default texture
	glBindTexture(GL_TEXTURE_2D, 0);

    glFinish();
//	Sys_EndWait();
	if (timing)
	{
		end = Sys_DoubleTime();
		Sys_Printf("MSG: Camera: %d ms\n", (int)(1000 * (end - start)));
	}
}
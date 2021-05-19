//==============================
//	z.c
//==============================

#include "qe3.h"

#define CAM_HEIGHT	48	// height of main part
#define CAM_GIZMO	8	// height of the gizmo


ZView::ZView()
{
	Init();
}

ZView::~ZView()
{

}

/*
============
ZView::Init
============
*/
void ZView::Init()
{
	origin[0] = 0;
	origin[1] = 0;	// sikk - changed from "20"
	origin[2] = 0;	// sikk - changed from "46"
	scale = 1;

	if (!g_qeglobals.d_savedinfo.bShow_Z)
		ShowWindow(g_qeglobals.d_hwndZ, SW_HIDE);

}


/*
============================================================================

	MOUSE ACTIONS

============================================================================
*/

/*
==============
Z_MouseDown
==============
*/
void ZView::MouseDown(int x, int y, int buttons) 
{
	vec3_t		org, dir, vup, vright;
	Brush	   *b;

	Sys_GetCursorPos(&cursorX, &cursorY);

	vup[0] = 0; 
	vup[1] = 0; 
	vup[2] = 1 / scale;

	VectorCopy(origin, org);
	org[2] += (y - (height / 2)) / scale;
	org[1] = -8192;

	b = g_brSelectedBrushes.next;
	if (b != &g_brSelectedBrushes)
		org[0] = (b->mins[0] + b->maxs[0]) / 2;

	dir[0] = 0; 
	dir[1] = 1; 
	dir[2] = 0;

	vright[0] = 0; 
	vright[1] = 0; 
	vright[2] = 0;

	// LBUTTON = manipulate selection
	// shift-LBUTTON = select
	// middle button = grab texture
	// ctrl-middle button = set entire brush to texture
	// ctrl-shift-middle button = set single face to texture
	if ((buttons == MK_LBUTTON) || 
		(buttons == (MK_LBUTTON | MK_SHIFT)) || 
		(buttons == MK_MBUTTON)	|| 
		(buttons == (MK_MBUTTON | MK_SHIFT | MK_CONTROL)))
	{
		Drag_Begin(x, y, buttons, vright, vup, org, dir);
		return;
	}

	// control mbutton = move camera
	if ((buttons == (MK_CONTROL | MK_MBUTTON)) || 
		(buttons == (MK_CONTROL | MK_LBUTTON)))
	{	
		g_qeglobals.d_camera.origin[2] = org[2];
		Sys_UpdateWindows(W_CAMERA | W_XY | W_Z);
	}
}

/*
==============
Z_MouseUp
==============
*/
void ZView::MouseUp(int x, int y, int buttons) 
{
	Drag_MouseUp();
}

/*
==============
Z_MouseMoved
==============
*/
void ZView::MouseMoved(int x, int y, int buttons) 
{
	char	zstring[256];
	float	fz;

	if (!buttons)
	{
		fz = origin[2] + (y - (height / 2)) / scale;
		fz = floor(fz / g_qeglobals.d_nGridSize + 0.5) * g_qeglobals.d_nGridSize;

		sprintf(zstring, "z Coordinate: (%d)", (int)fz);
		Sys_Status(zstring, 0);

		return;
	}

	if (buttons == MK_LBUTTON)
	{
		Drag_MouseMoved(x, y, buttons);
		Sys_UpdateWindows(W_Z | W_CAMERA);
		return;
	}
	// rbutton = drag z origin
	if (buttons == MK_RBUTTON)
	{
		SetCursor(NULL); // sikk - Remove Cursor
		Sys_GetCursorPos(&x, &y);
		if (y != cursorY)
		{
			origin[2] += (y - cursorY) / scale;

			Sys_GetCursorPos(&cursorX, &cursorY);
			Sys_UpdateWindows(W_Z);

			sprintf(zstring, "z Origin: (%d)", (int)origin[2]);
			Sys_Status(zstring, 0);
		}
		return;
	}

	// control mbutton = move camera
	if ((buttons == (MK_CONTROL | MK_MBUTTON)) ||
		(buttons == (MK_CONTROL | MK_LBUTTON)))
	{	
		g_qeglobals.d_camera.origin[2] = (y - (height / 2)) / scale;
		Sys_UpdateWindows(W_CAMERA | W_XY | W_Z);
	}

// sikk---> Mouse Zoom
	// control rbutton = zoom z view
	if (buttons == (MK_CONTROL | MK_RBUTTON))
	{
		SetCursor(NULL); // sikk - Remove Cursor
		Sys_GetCursorPos(&x, &y);

		if (y != cursorY)
		{
			if (y > cursorY)
				scale *= powf(1.01f, fabs(y - cursorY));
			else
				scale *= powf(0.99f, fabs(y - cursorY));

			scale = max(0.1f, min(scale, 16));


			Sys_SetCursorPos(cursorX, cursorY);
			Sys_UpdateWindows(W_Z);
		}
		return;
	}
// <---sikk
}


/*
============================================================================

	DRAWING

============================================================================
*/

/*
==============
ZView::DrawGrid
==============
*/
void ZView::DrawGrid ()
{
	int		w, h;
	float	zz, zb, ze;
	int		nSize;

	if (g_qeglobals.d_nGridSize == 128)
		nSize = 128;
	else if (g_qeglobals.d_nGridSize == 256)
		nSize = 256;
	else
		nSize = 64;

	w = width / 2;// / scale;
	h = height / 2 / scale;

	zb = origin[2] - h;
	if (zb < g_v3RegionMins[2])
		zb = g_v3RegionMins[2];
	zb = nSize * floor(zb / nSize);

	ze = origin[2] + h;
	if (ze > g_v3RegionMaxs[2])
		ze = g_v3RegionMaxs[2];
	ze = nSize * ceil(ze / nSize);

	// draw major blocks
	glColor3fv(g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDMAJOR]);

	glBegin(GL_LINES);
	glVertex2f(0, zb);
	glVertex2f(0, ze);
	for (zz = zb; zz < ze; zz += nSize)
	{
		glVertex2f(-w, zz);
		glVertex2f(w, zz);
	}
	glEnd();

	// draw minor blocks
	if (g_qeglobals.d_bShowGrid && g_qeglobals.d_nGridSize * scale >= 4)
	{
		glColor3fv(g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDMINOR]);

		glBegin(GL_LINES);
		for (zz = zb; zz < ze; zz += g_qeglobals.d_nGridSize)
		{
			if (!((int)zz & 63))
				continue;
			glVertex2f(-w, zz);
			glVertex2f(w, zz);
		}
		glEnd();
	}
}

/*
==============
ZView::DrawCoords
==============
*/
void ZView::DrawCoords ()
{
	int		w, h;
	float	z, zb, ze;
	char	text[8];
	int		nSize;

	if (g_qeglobals.d_nGridSize == 128)
		nSize = 128;
	else if (g_qeglobals.d_nGridSize == 256)
		nSize = 256;
	else
		nSize = 64;

	w = width / 2;// / scale;
	h = height / 2 / scale;

	zb = origin[2] - h;
	if (zb < g_v3RegionMins[2])
		zb = g_v3RegionMins[2];
	zb = nSize * floor(zb / nSize);

	ze = origin[2] + h;
	if (ze > g_v3RegionMaxs[2])
		ze = g_v3RegionMaxs[2];
	ze = nSize * ceil(ze / nSize);

	glColor3fv(g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDTEXT]);

	for (z = zb; z <= ze; z += nSize)	// sikk - 'z <= ze' instead of 'z < ze' so last coord is drawn
	{
		glRasterPos2f(-w + 1, z);
		sprintf(text, "%d",(int)z);
		glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);
	}
}

/*
==============
ZView::DrawCameraIcon
==============
*/
void ZView::DrawCameraIcon ()
{
	float	x, y;
	int		xCam = width / 4;

	x = 0;
	y = g_qeglobals.d_camera.origin[2];

	glColor3f (0.0, 0.0, 1.0);
	glBegin(GL_LINE_STRIP);
	glVertex3f(x - xCam, y, 0);
	glVertex3f(x, y + CAM_GIZMO, 0);
	glVertex3f(x + xCam, y, 0);
	glVertex3f(x, y - CAM_GIZMO, 0);
	glVertex3f(x - xCam, y, 0);
	glVertex3f(x + xCam, y, 0);
	glVertex3f(x + xCam, y - CAM_HEIGHT, 0);
	glVertex3f(x - xCam, y - CAM_HEIGHT, 0);
	glVertex3f(x - xCam, y, 0);
	glEnd();

}

GLbitfield g_glbitClear = GL_COLOR_BUFFER_BIT; // HACK

/*
==============
ZView::Draw
==============
*/
void ZView::Draw ()
{
    Brush	   *brush;
	float		w, h;
	float		top, bottom;
	double		start, end;
	qtexture_t *q;
	vec3_t		org_top, org_bottom, dir_up, dir_down;

	int xCam = width / 3;

	if (!g_brActiveBrushes.next)
		return;	// not valid yet

	if (timing)
		start = Sys_DoubleTime();

	//
	// clear
	//
	glViewport(0, 0, width, height);
	glClearColor(g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDBACK][0],
				 g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDBACK][1],
				 g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDBACK][2],
				 0);

    /* GL Bug */ 
	/* When not using hw acceleration, gl will fault if we clear the depth 
	buffer bit on the first pass. The hack fix is to set the GL_DEPTH_BUFFER_BIT
	only after Z_Draw() has been called once. Yeah, right. */
	glClear(g_glbitClear); 
	g_glbitClear |= GL_DEPTH_BUFFER_BIT;
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

	w = width / 2;// / scale;
	h = height / 2 / scale;
	
	glOrtho(-w, w, origin[2] - h, origin[2] + h, -8, 8);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	//
	// now draw the grid
	//
	DrawGrid();

	//
	// draw stuff
	//
	glDisable(GL_CULL_FACE);
	glShadeModel(GL_FLAT);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	// draw filled interiors and edges
	dir_up[0] = 0; 
	dir_up[1] = 0; 
	dir_up[2] = 1;
	dir_down[0] = 0; 
	dir_down[1] = 0; 
	dir_down[2] = -1;
	VectorCopy(origin, org_top);
	org_top[2] = 4096;
	VectorCopy(origin, org_bottom);
	org_bottom[2] = -4096;

	for (brush = g_brActiveBrushes.next; brush != &g_brActiveBrushes; brush = brush->next)
	{
		if (brush->mins[0] >= origin[0] || 
			brush->maxs[0] <= origin[0]	|| 
			brush->mins[1] >= origin[1]	|| 
			brush->maxs[1] <= origin[1])
			continue;

		if (!brush->RayTest(org_top, dir_down, &top))
			continue;
		top = org_top[2] - top;

		if (!brush->RayTest(org_bottom, dir_up, &bottom))
			continue;
		bottom = org_bottom[2] + bottom;

		q = Texture_ForName(brush->brush_faces->texdef.name);

		glColor3f(q->color[0], q->color[1], q->color[2]);
		glBegin(GL_QUADS);
		glVertex2f(-xCam, bottom);
		glVertex2f(xCam, bottom);
		glVertex2f(xCam, top);
		glVertex2f(-xCam, top);
		glEnd();

		glColor3f(1, 1, 1);
		glBegin(GL_LINE_LOOP);
		glVertex2f(-xCam, bottom);
		glVertex2f(xCam, bottom);
		glVertex2f(xCam, top);
		glVertex2f(-xCam, top);
		glEnd();
	}

	//
	// now draw selected brushes
	//
	for (brush = g_brSelectedBrushes.next; brush != &g_brSelectedBrushes; brush = brush->next)
	{
		if (!(brush->mins[0] >= origin[0] || 
			  brush->maxs[0] <= origin[0] || 
			  brush->mins[1] >= origin[1] || 
			  brush->maxs[1] <= origin[1]))
		{
			if (brush->RayTest(org_top, dir_down, &top))
			{
				top = org_top[2] - top;
				if (brush->RayTest(org_bottom, dir_up, &bottom))
				{
					bottom = org_bottom[2] + bottom;
					q = Texture_ForName(brush->brush_faces->texdef.name);

					glColor3f(q->color[0], q->color[1], q->color[2]);
					glBegin(GL_QUADS);
					glVertex2f(-xCam, bottom);
					glVertex2f(xCam, bottom);
					glVertex2f(xCam, top);
					glVertex2f(-xCam, top);
					glEnd();
				}
			}
		}

		glColor3fv(g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES]);
		glBegin(GL_LINE_LOOP);
		glVertex2f(-xCam, brush->mins[2]);
		glVertex2f(xCam, brush->mins[2]);
		glVertex2f(xCam, brush->maxs[2]);
		glVertex2f(-xCam, brush->maxs[2]);
		glEnd();
	}

	DrawCameraIcon();

	// draw coordinate text if needed
	if (g_qeglobals.d_savedinfo.bShow_Coordinates)	// sikk - Toggle By Menu Command
		DrawCoords();	// sikk - Draw Coords last so they are on top
    glFinish();

	QE_CheckOpenGLForErrors();

	if (timing)
	{
		end = Sys_DoubleTime();
		Sys_Printf("MSG: Z: %d ms\n", (int)(1000 * (end - start)));
	} 
}

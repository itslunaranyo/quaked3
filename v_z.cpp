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
}

/*
==================
ZView::ToPoint
==================
*/
void ZView::ToPoint(int x, int y, vec3 &point)
{
	point = origin;
	point.z = origin.z + (y - height / 2) / scale;
}

/*
==================
ZView::GetMouseContext
==================
*/
mouseContext_t const ZView::GetMouseContext(const int x, const int y)
{
	mouseContext_t mc;

	ToPoint(x, y, mc.org);

	// .ray and .right are 0,0,0 because their direction is meaningless

	mc.pt = vec3(x, y, 0);
	mc.up = vec3(0,0,1);
	mc.dims = 1;	// 1D view

	return mc;
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
	vec3		org, dir, vup, vright;
	Brush	   *b;

	Sys_GetCursorPos(&cursorX, &cursorY);

	vup[0] = 0; 
	vup[1] = 0; 
	vup[2] = 1 / scale;

	org = origin;
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

	/*
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
	*/
	// control mbutton = move camera
	if ((buttons == (MK_CONTROL | MK_MBUTTON)) || 
		(buttons == (MK_CONTROL | MK_LBUTTON)))
	{	
		g_qeglobals.d_vCamera.origin[2] = org[2];
		Sys_UpdateWindows(W_SCENE);
	}
}

/*
==============
Z_MouseUp
==============
*/
void ZView::MouseUp(int x, int y, int buttons) 
{
	//Drag_MouseUp();
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
	//	Drag_MouseMoved(x, y, buttons);
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

			Sys_SetCursorPos(cursorX, cursorY);
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
		g_qeglobals.d_vCamera.origin[2] = (y - (height / 2)) / scale;
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

			scale = max(0.1f, min(scale, 16.0f));


			Sys_SetCursorPos(cursorX, cursorY);
			Sys_UpdateWindows(W_Z);
		}
		return;
	}
// <---sikk
}

void ZView::ScaleUp()
{
	scale = min(32.0f, scale * 1.25f);
	Sys_UpdateWindows(W_Z);
}
void ZView::ScaleDown()
{
	scale = max(0.05f, scale * 0.8f);
	Sys_UpdateWindows(W_Z);
}
void ZView::Scroll(float amt)
{
	origin.z += amt / scale;
	Sys_UpdateWindows(W_Z);
}

/*
============================================================================

	DRAWING

============================================================================
*/

/*
==============
ZView::Draw2D
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
	if (zb < g_map.regionMins[2])
		zb = g_map.regionMins[2];
	zb = nSize * floor(zb / nSize);

	ze = origin[2] + h;
	if (ze > g_map.regionMaxs[2])
		ze = g_map.regionMaxs[2];
	ze = nSize * ceil(ze / nSize);

	// draw major blocks
	glColor3fv(&g_colors.gridMajor.x);

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
		glColor3fv(&g_colors.gridMinor.x);

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
	if (zb < g_map.regionMins[2])
		zb = g_map.regionMins[2];
	zb = nSize * floor(zb / nSize);

	ze = origin[2] + h;
	if (ze > g_map.regionMaxs[2])
		ze = g_map.regionMaxs[2];
	ze = nSize * ceil(ze / nSize);

	glColor3fv(&g_colors.gridText.x);

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
	y = g_qeglobals.d_vCamera.origin[2];

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


/*
==============
ZView::DrawTools
==============
*/
bool ZView::DrawTools()
{
	for (auto tIt = g_qeglobals.d_tools.rbegin(); tIt != g_qeglobals.d_tools.rend(); ++tIt)
	{
		if ((*tIt)->Draw1D(*this))
			return true;
	}
	return false;
}

/*
==============
ZView::TestBrush
==============
*/
bool ZView::TestBrush(Brush &br, zbr_t &zbr)
{
	vec3	org_top, org_bottom;

	org_top = origin;
	org_top[2] = g_cfgEditor.MapSize / 2;
	org_bottom = origin;
	org_bottom[2] = -g_cfgEditor.MapSize / 2;

	if (br.mins[0] >= origin[0] ||
		br.maxs[0] <= origin[0] ||
		br.mins[1] >= origin[1] ||
		br.maxs[1] <= origin[1])
		return false;

	if (!br.RayTest(org_top, vec3(0, 0, -1), &zbr.top))
		return false;

	if (!br.RayTest(org_bottom, vec3(0, 0, 1), &zbr.bottom))
		return false;

	zbr.brush = &br;
	zbr.top = org_top[2] - zbr.top;
	zbr.bottom = org_bottom[2] + zbr.bottom;
	zbr.tex = Textures::ForName(br.faces->texdef.name);

	return true;
}

/*
==============
ZView::DrawSelection
==============
*/
void ZView::DrawSelection(vec3 selColor)
{
	Brush*	brush;
	float	top, bottom;
	vec3	org_top, org_bottom;
	Texture *q;
	int xCam = width / 3;

	zbr_t zbtemp;
	std::vector<zbr_t> brDraw;

	for (brush = g_brSelectedBrushes.next; brush != &g_brSelectedBrushes; brush = brush->next)
	{
		if (!TestBrush(*brush, zbtemp))
			continue;
		brDraw.push_back(zbtemp);
	}

	// draw selected brushes as quads filled with the brush color
	for (auto zbIt = brDraw.begin(); zbIt != brDraw.end(); ++zbIt)
	{
		glColor3f(zbIt->tex->color[0], zbIt->tex->color[1], zbIt->tex->color[2]);
		glBegin(GL_QUADS);
		glVertex2f(-xCam, zbIt->bottom);
		glVertex2f(xCam, zbIt->bottom);
		glVertex2f(xCam, zbIt->top);
		glVertex2f(-xCam, zbIt->top);
		glEnd();
	}

	// lunaran: draw all selection borders over the colored quads so nothing in the selection is obscured
	glColor4f(selColor[0], selColor[1], selColor[2], 1.0f);
	for (auto zbIt = brDraw.begin(); zbIt != brDraw.end(); ++zbIt)
	{
		glBegin(GL_LINE_LOOP);
		glVertex2f(-xCam, zbIt->brush->mins[2]);
		glVertex2f(xCam, zbIt->brush->mins[2]);
		glVertex2f(xCam, zbIt->brush->maxs[2]);
		glVertex2f(-xCam, zbIt->brush->maxs[2]);
		glEnd();
	}
}

/*
==============
ZView::DrawBrush
==============
*/
void ZView::DrawBrush(Brush *brush, vec3 color)
{
	int xCam = width / 3;
	zbr_t zb;

	if (!TestBrush(*brush, zb))
		return;

	glColor3f(zb.tex->color[0], zb.tex->color[1], zb.tex->color[2]);
	glBegin(GL_QUADS);
	glVertex2f(-xCam, zb.bottom);
	glVertex2f(xCam, zb.bottom);
	glVertex2f(xCam, zb.top);
	glVertex2f(-xCam, zb.top);
	glEnd();

	glColor4f(color[0], color[1], color[2], 1.0f);
	glBegin(GL_LINE_LOOP);
	glVertex2f(-xCam, zb.brush->mins[2]);
	glVertex2f(xCam, zb.brush->mins[2]);
	glVertex2f(xCam, zb.brush->maxs[2]);
	glVertex2f(-xCam, zb.brush->maxs[2]);
	glEnd();
}

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
	zbr_t zbtemp;
	std::vector<zbr_t> brDraw;

	int xCam = width / 3;

	if (!g_map.brActive.next)
		return;	// not valid yet

	if (timing)
		start = Sys_DoubleTime();

	// clear
	glViewport(0, 0, width, height);
	glClearColor(g_colors.gridBackground[0],
				 g_colors.gridBackground[1],
				 g_colors.gridBackground[2],
				 0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

	w = width / 2;// / scale;
	h = height / 2 / scale;
	
	glOrtho(-w, w, origin[2] - h, origin[2] + h, -8, 8);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	DrawGrid();

	// draw stuff
	glDisable(GL_CULL_FACE);
	glShadeModel(GL_FLAT);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

	glDisable(GL_DEPTH_TEST);

	// draw filled interiors and edges
	for (brush = g_map.brActive.next; brush != &g_map.brActive; brush = brush->next)
	{
		if (!TestBrush(*brush, zbtemp))
			continue;
		brDraw.push_back(zbtemp);
	}

	for (auto zbIt = brDraw.begin(); zbIt != brDraw.end(); ++zbIt)
	{
		glColor3f(zbIt->tex->color[0], zbIt->tex->color[1], zbIt->tex->color[2]);
		glBegin(GL_QUADS);
		glVertex2f(-xCam, zbIt->bottom);
		glVertex2f(xCam, zbIt->bottom);
		glVertex2f(xCam, zbIt->top);
		glVertex2f(-xCam, zbIt->top);
		glEnd();
	}

	// lunaran: draw all borders over the colored quads so nothing in the selection is obscured
	glColor4f(g_colors.brush[0], g_colors.brush[1], g_colors.brush[2], 1.0f);
	for (auto zbIt = brDraw.begin(); zbIt != brDraw.end(); ++zbIt)
	{
		glBegin(GL_LINE_LOOP);
		glVertex2f(-xCam, zbIt->brush->mins[2]);
		glVertex2f(xCam, zbIt->brush->mins[2]);
		glVertex2f(xCam, zbIt->brush->maxs[2]);
		glVertex2f(-xCam, zbIt->brush->maxs[2]);
		glEnd();
	}

	if (!DrawTools())
		DrawSelection(g_colors.selection);

	DrawCameraIcon();

	// draw coordinate text if needed
	if (g_cfgUI.ShowCoordinates)	// sikk - Toggle By Menu Command
		DrawCoords();	// sikk - Draw Coords last so they are on top
    glFinish();

	if (timing)
	{
		end = Sys_DoubleTime();
		Sys_Printf("Z: %d ms\n", (int)(1000 * (end - start)));
	} 
}


//==============================
//	ZViewRenderer.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "ZViewRenderer.h"
#include "ZView.h"
#include "Tool.h"
#include "CameraView.h"
#include "map.h"
#include "select.h"

#define CAM_HEIGHT	48	// height of main part
#define CAM_GIZMO	8	// height of the gizmo


ZViewRenderer::ZViewRenderer(ZView &view) : zv(view)
{
}

ZViewRenderer::~ZViewRenderer()
{
}

/*
==============
ZViewRenderer::Draw2D
==============
*/
void ZViewRenderer::DrawGrid()
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

	w = zv.GetWidth() / 2;
	h = zv.GetHeight() / 2 / zv.GetScale();

	zb = zv.GetOrigin()[2] - h;
	if (zb < g_map.regionMins[2])
		zb = g_map.regionMins[2];
	zb = nSize * floor(zb / nSize);

	ze = zv.GetOrigin()[2] + h;
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
	if (g_qeglobals.d_bShowGrid && g_qeglobals.d_nGridSize * zv.GetScale() >= 4)
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
ZViewRenderer::DrawCoords
==============
*/
void ZViewRenderer::DrawCoords()
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

	if (zv.GetScale() < 0.2f)
		nSize = max(128, nSize);
	if (zv.GetScale() < 0.05f)
		nSize = max(256, nSize);

	w = zv.GetWidth() / 2;
	h = zv.GetHeight() / 2 / zv.GetScale();

	zb = zv.GetOrigin()[2] - h;
	if (zb < g_map.regionMins[2])
		zb = g_map.regionMins[2];
	zb = nSize * floor(zb / nSize);

	ze = zv.GetOrigin()[2] + h;
	if (ze > g_map.regionMaxs[2])
		ze = g_map.regionMaxs[2];
	ze = nSize * ceil(ze / nSize);

	glColor3fv(&g_colors.gridText.x);

	for (z = zb; z <= ze; z += nSize)	// sikk - 'z <= ze' instead of 'z < ze' so last coord is drawn
	{
		glRasterPos2f(-w + 1, z);
		sprintf(text, "%d", (int)z);
		glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);
	}
}

/*
==============
ZViewRenderer::DrawCameraIcon
==============
*/
void ZViewRenderer::DrawCameraIcon()
{
	float	x, y;
	int		xCam = zv.GetWidth() / 4;

	x = 0;
	y = g_vCamera.GetOrigin().z;

	glColor3f(0.0, 0.0, 1.0);
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
ZViewRenderer::DrawTools
==============
*/
bool ZViewRenderer::DrawTools()
{
	for (auto tIt = Tool::stack.rbegin(); tIt != Tool::stack.rend(); ++tIt)
	{
		if ((*tIt)->Draw1D(*this))
			return true;
	}
	return false;
}

/*
==============
ZViewRenderer::TestBrush
==============
*/
bool ZViewRenderer::TestBrush(zbr_t &zbr)
{
	vec3	origin, org_top, org_bottom;
	bool	result = true;

	if (zbr.brush->IsFiltered())
		return false;

	origin = zv.GetOrigin();
	if (zbr.brush->mins[0] >= origin[0] ||
		zbr.brush->maxs[0] <= origin[0] ||
		zbr.brush->mins[1] >= origin[1] ||
		zbr.brush->maxs[1] <= origin[1])
		//result = false;
		return false;

	org_top = origin;
	org_top[2] = g_cfgEditor.MapSize / 2;
	org_bottom = origin;
	org_bottom[2] = -g_cfgEditor.MapSize / 2;

	if (!zbr.brush->RayTest(org_top, vec3(0, 0, -1), &zbr.top))
		result = false;

	if (!zbr.brush->RayTest(org_bottom, vec3(0, 0, 1), &zbr.bottom))
		result = false;

	zbr.top = org_top[2] - zbr.top;
	zbr.bottom = org_bottom[2] + zbr.bottom;

	return result;
}

/*
==============
ZViewRenderer::zbr_t::zbr_t
==============
*/
ZViewRenderer::zbr_t::zbr_t(Brush & br)
{
	brush = &br;
	top = br.maxs[2];
	bottom = br.mins[2];
	tex = Textures::ForName(br.faces->texdef.name);
}


/*
==============
ZViewRenderer::DrawSelection
==============
*/
void ZViewRenderer::DrawSelection(vec3 selColor)
{
	Brush*	brush;
	vec3	org_top, org_bottom;
	int xCam = zv.GetWidth() / 3;

	std::vector<zbr_t> brDraw;

	for (brush = g_brSelectedBrushes.next; brush != &g_brSelectedBrushes; brush = brush->next)
	{
		zbr_t zbtemp(*brush);
		if (!TestBrush(zbtemp))
		{
			brDraw.push_back(zbtemp);
			continue;
		}

		// draw selected brushes as quads filled with the brush color
		glColor3f(zbtemp.tex->color[0], zbtemp.tex->color[1], zbtemp.tex->color[2]);
		glBegin(GL_QUADS);
		glVertex2f(-xCam, zbtemp.bottom);
		glVertex2f(xCam, zbtemp.bottom);
		glVertex2f(xCam, zbtemp.top);
		glVertex2f(-xCam, zbtemp.top);
		glEnd();
		brDraw.push_back(zbtemp);
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
ZViewRenderer::DrawBrush
==============
*/
void ZViewRenderer::DrawBrush(Brush *brush, vec3 color)
{
	zbr_t zb(*brush);

	if (!TestBrush(zb))
		return;

	int xCam = zv.GetWidth() / 3;

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
ZViewRenderer::Draw
==============
*/
void ZViewRenderer::Draw()
{
	Brush	   *brush;
	float		w, h;
	double		start, end;
	std::vector<zbr_t> brDraw;

	int xCam = zv.GetWidth() / 3;

	if (!g_map.brActive.next)
		return;	// not valid yet

	if (timing)
		start = Sys_DoubleTime();

	// clear
	glViewport(0, 0, zv.GetWidth(), zv.GetHeight());
	glClearColor(g_colors.gridBackground[0],
		g_colors.gridBackground[1],
		g_colors.gridBackground[2],
		0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	w = zv.GetWidth() / 2;// / scale;
	h = zv.GetHeight() / 2 / zv.GetScale();

	glOrtho(-w, w, zv.GetOrigin()[2] - h, zv.GetOrigin()[2] + h, -8, 8);
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
		zbr_t zbtemp(*brush);
		if (!TestBrush(zbtemp))
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


//==============================
//	GridViewRenderer.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "GridViewRenderer.h"
#include "GridView.h"
#include "CameraView.h"
#include "ZView.h"
#include "WndGrid.h"
#include "WndZChecker.h"
#include "Tool.h"
#include "select.h"
#include "map.h"

// a renderer is bound to one view for its entire lifetime
GridViewRenderer::GridViewRenderer(GridView &view) : gv(view) {}
GridViewRenderer::~GridViewRenderer() {}

/*
==============
GridViewRenderer::DrawGrid
==============
*/
void GridViewRenderer::DrawGrid()
{
	float	x, y, xb, xe, yb, ye;
	int		majorSize;

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	majorSize = max(64, g_qeglobals.d_nGridSize);

	xb = max(vMins[gv.DimU()], g_map.regionMins[gv.DimU()]);
	xb = majorSize * floor(xb / majorSize);

	xe = min(vMaxs[gv.DimU()], g_map.regionMaxs[gv.DimU()]);
	xe = majorSize * ceil(xe / majorSize);

	yb = max(vMins[gv.DimV()], g_map.regionMins[gv.DimV()]);
	yb = majorSize * floor(yb / majorSize);

	ye = min(vMaxs[gv.DimV()], g_map.regionMaxs[gv.DimV()]);
	ye = majorSize * ceil(ye / majorSize);

	// draw major blocks
	glColor3fv(&g_colors.gridMajor.r);

	if (g_qeglobals.d_bShowGrid)
	{
		glBegin(GL_LINES);

		for (x = xb; x <= xe; x += majorSize)
		{
			glVertex2f(x, yb);
			glVertex2f(x, ye);
		}
		for (y = yb; y <= ye; y += majorSize)
		{
			glVertex2f(xb, y);
			glVertex2f(xe, y);
		}
		glEnd();

		// draw minor blocks
		if (g_qeglobals.d_nGridSize * gv.GetScale() >= 4)
		{
			glColor3fv(&g_colors.gridMinor.r);

			glBegin(GL_LINES);
			for (x = xb; x < xe; x += g_qeglobals.d_nGridSize)
			{
				if (!((int)x & 63))
					continue;
				glVertex2f(x, yb);
				glVertex2f(x, ye);
			}
			for (y = yb; y < ye; y += g_qeglobals.d_nGridSize)
			{
				if (!((int)y & 63))
					continue;
				glVertex2f(xb, y);
				glVertex2f(xe, y);
			}
			glEnd();
		}

		// draw grid axis
		// lunaran - grid axis now block color, not grid major * 65%
		glColor3fv(&g_colors.gridBlock.r);
		glBegin(GL_LINES);
		glVertex2f(xb, 0);
		glVertex2f(xe, 0);
		glVertex2f(0, yb);
		glVertex2f(0, ye);
		glEnd();
	}

	// show current work zone?
	// the work zone is used to place dropped points and brushes
	if (g_cfgUI.ShowWorkzone)
	{
		glColor3f(1.0f, 0.0f, 0.0f);
		glBegin(GL_LINES);
		glVertex2f(xb, g_qeglobals.d_v3WorkMin[gv.DimV()]);
		glVertex2f(xe, g_qeglobals.d_v3WorkMin[gv.DimV()]);
		glVertex2f(xb, g_qeglobals.d_v3WorkMax[gv.DimV()]);
		glVertex2f(xe, g_qeglobals.d_v3WorkMax[gv.DimV()]);
		glVertex2f(g_qeglobals.d_v3WorkMin[gv.DimU()], yb);
		glVertex2f(g_qeglobals.d_v3WorkMin[gv.DimU()], ye);
		glVertex2f(g_qeglobals.d_v3WorkMax[gv.DimU()], yb);
		glVertex2f(g_qeglobals.d_v3WorkMax[gv.DimU()], ye);
		glEnd();
	}

	DrawBlockGrid();
}

/*
==============
GridViewRenderer::DrawBlockGrid
==============
*/
void GridViewRenderer::DrawBlockGrid()
{
	if (!g_cfgUI.ShowBlocks)
		return;

	float	x, y, xb, xe, yb, ye;
	char	text[32];

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	xb = gv.GetMins()[gv.DimU()];
	if (xb < g_map.regionMins[gv.DimU()])
		xb = g_map.regionMins[gv.DimU()];
	xb = 1024 * floor(xb / 1024);

	xe = gv.GetMaxs()[gv.DimU()];
	if (xe > g_map.regionMaxs[gv.DimU()])
		xe = g_map.regionMaxs[gv.DimU()];
	xe = 1024 * ceil(xe / 1024);

	yb = gv.GetMins()[gv.DimV()];
	if (yb < g_map.regionMins[gv.DimV()])
		yb = g_map.regionMins[gv.DimV()];
	yb = 1024 * floor(yb / 1024);

	ye = gv.GetMaxs()[gv.DimV()];
	if (ye > g_map.regionMaxs[gv.DimV()])
		ye = g_map.regionMaxs[gv.DimV()];
	ye = 1024 * ceil(ye / 1024);

	// draw major blocks
	glColor3fv(&g_colors.gridBlock.r);
	glLineWidth(2);
	glBegin(GL_LINES);

	for (x = xb; x <= xe; x += 1024)
	{
		glVertex2f(x, yb);
		glVertex2f(x, ye);
	}
	for (y = yb; y <= ye; y += 1024)
	{
		glVertex2f(xb, y);
		glVertex2f(xe, y);
	}

	glEnd();
	glLineWidth(1);

	// draw coordinate text if needed
	for (x = xb; x < xe; x += 1024)
		for (y = yb; y < ye; y += 1024)
		{
			glRasterPos2f(x + 512, y + 512);
			sprintf(text, "%d,%d", (int)floor(x / 1024), (int)floor(y / 1024));
			glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);
		}
}

/*
==============
GridViewRenderer::DrawViewName
sikk: View Axis - cleaner, more intuitive look
lunaran TODO: unify with axis in camera view
==============
*/
void GridViewRenderer::DrawViewName()
{
	// lunaran: always draw view name if we have one grid view, and never in 3-view mode
	if ((!g_wndGrid[1] || g_wndGrid[1]->IsOpen()) &&
		(!g_wndGrid[2] || g_wndGrid[2]->IsOpen()) &&
		(!g_wndGrid[3] || g_wndGrid[3]->IsOpen()))
		return;

	float *p1, *p2;
	float fColor[][3] = { { 1.0f, 0.0f, 0.0f },{ 0.0f, 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } };
	char *szView;
	float cornerU, cornerV;

	switch (gv.GetAxis())
	{
	case GRID_XY:
		p1 = fColor[0];
		p2 = fColor[1];
		szView = "XY";
		break;
	case GRID_XZ:
		p1 = fColor[0];
		p2 = fColor[2];
		szView = "XZ";
		break;
	case GRID_YZ:
	default:
		p1 = fColor[1];
		p2 = fColor[2];
		szView = "YZ";
		break;
	}

	cornerU = gv.GetMins()[gv.DimU()];
	cornerV = gv.GetMaxs()[gv.DimV()];

	glColor3fv(&g_colors.gridText.r);
	glRasterPos2f(cornerU + 68 / gv.GetScale(),
		cornerV - 51 / gv.GetScale());
	glCallLists(1, GL_UNSIGNED_BYTE, &szView[0]);

	glColor3fv(&g_colors.gridText.r);
	glRasterPos2f(cornerU + 44 / gv.GetScale(),
		cornerV - 28 / gv.GetScale());
	glCallLists(1, GL_UNSIGNED_BYTE, &szView[1]);

	glLineWidth(2);
	glColor3fv(p1);
	glBegin(GL_LINES);
	glVertex2f(cornerU + 48 / gv.GetScale(),
		cornerV - 48 / gv.GetScale());
	glVertex2f(cornerU + 64 / gv.GetScale(),
		cornerV - 48 / gv.GetScale());
	glEnd();

	glColor3fv(p2);
	glBegin(GL_LINES);
	glVertex2f(cornerU + 48 / gv.GetScale(),
		cornerV - 48 / gv.GetScale());
	glVertex2f(cornerU + 48 / gv.GetScale(),
		cornerV - 32 / gv.GetScale());
	glEnd();
	glLineWidth(1);
}

/*
==============
GridViewRenderer::DrawCoords
==============
*/
void GridViewRenderer::DrawCoords()
{
	if (!g_cfgUI.ShowCoordinates) return;

	float	x, y, xb, xe, yb, ye;
	char	text[8];
	int		nSize;

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	// sikk---> Filter Coords so they don't become bunched and unreadable 
	nSize = 64;
	if (gv.GetScale() <= 0.6)
		nSize = 128;
	if (gv.GetScale() <= 0.3)
		nSize = 256;
	if (gv.GetScale() <= 0.15)
		nSize = 512;
	if (gv.GetScale() <= 0.075)
		nSize = 1024;

	xb = gv.GetMins()[gv.DimU()];
	if (xb < g_map.regionMins[gv.DimU()])
		xb = g_map.regionMins[gv.DimU()];

	xe = gv.GetMaxs()[gv.DimU()];
	if (xe > g_map.regionMaxs[gv.DimU()])
		xe = g_map.regionMaxs[gv.DimU()];

	yb = gv.GetMins()[gv.DimV()];
	if (yb < g_map.regionMins[gv.DimV()])
		yb = g_map.regionMins[gv.DimV()];

	ye = gv.GetMaxs()[gv.DimV()];
	if (ye > g_map.regionMaxs[gv.DimV()])
		ye = g_map.regionMaxs[gv.DimV()];

	xb = nSize * floor(xb / nSize);
	xe = nSize * ceil(xe / nSize);
	yb = nSize * floor(yb / nSize);
	ye = nSize * ceil(ye / nSize);

	glColor3fv(&g_colors.gridText.r);

	for (x = xb; x <= xe; x += nSize)	// sikk - 'x <= xe' instead of 'x < xe' so last coord is drawn 
	{
		glRasterPos2f(x, gv.GetMaxs()[gv.DimV()] - 6 / gv.GetScale());
		sprintf(text, "%d", (int)x);
		glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);
	}
	for (y = yb; y <= ye; y += nSize)	// sikk - 'y <= ye' instead of 'y < ye' so last coord is drawn
	{
		glRasterPos2f(gv.GetMins()[gv.DimU()] + 1 / gv.GetScale(), y);
		sprintf(text, "%d", (int)y);
		glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);
	}
}

/*
==================
GridViewRenderer::DrawCameraIcon
==================
*/
void GridViewRenderer::DrawCameraIcon()
{
	float	x, y, a;
	//	char	text[128];
	vec3	camOrg;

	camOrg = g_vCamera.GetOrigin();

	x = camOrg[gv.DimU()];
	y = camOrg[gv.DimV()];

	if (gv.GetAxis() == GRID_XY)
		a = g_vCamera.GetAngles()[YAW] / 180 * Q_PI;
	else
		a = g_vCamera.GetAngles()[PITCH] / 180 * Q_PI;

	glColor3f(0.0, 0.0, 1.0);
	glBegin(GL_LINE_STRIP);
	glVertex3f(x - 16, y, 0);
	glVertex3f(x, y + 8, 0);
	glVertex3f(x + 16, y, 0);
	glVertex3f(x, y - 8, 0);
	glVertex3f(x - 16, y, 0);
	glVertex3f(x + 16, y, 0);
	glEnd();

	glBegin(GL_LINE_STRIP);
	glVertex3f(x + 48 * cos(a + Q_PI / 4), y + 48 * sin(a + Q_PI / 4), 0);
	glVertex3f(x, y, 0);
	glVertex3f(x + 48 * cos(a - Q_PI / 4), y + 48 * sin(a - Q_PI / 4), 0);
	glEnd();
}

/*
==================
GridViewRenderer::DrawZIcon
==================
*/
void GridViewRenderer::DrawZIcon()
{
	if (gv.GetAxis() != GRID_XY) return;

	float x, y;

	x = g_vZ.GetOrigin()[0];
	y = g_vZ.GetOrigin()[1];

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0.0, 0.0, 1.0, 0.25);

	glBegin(GL_QUADS);
	glVertex3f(x - 8, y - 8, 0);
	glVertex3f(x + 8, y - 8, 0);
	glVertex3f(x + 8, y + 8, 0);
	glVertex3f(x - 8, y + 8, 0);
	glEnd();

	glDisable(GL_BLEND);
	glColor4f(0.0, 0.0, 1.0, 1);

	glBegin(GL_LINE_LOOP);
	glVertex3f(x - 8, y - 8, 0);
	glVertex3f(x + 8, y - 8, 0);
	glVertex3f(x + 8, y + 8, 0);
	glVertex3f(x - 8, y + 8, 0);
	glEnd();

	glBegin(GL_LINE_STRIP);
	glVertex3f(x - 4, y + 4, 0);
	glVertex3f(x + 4, y + 4, 0);
	glVertex3f(x - 4, y - 4, 0);
	glVertex3f(x + 4, y - 4, 0);
	glEnd();
}

/*
==================
GridViewRenderer::DrawTools
==================
*/
bool GridViewRenderer::DrawTools()
{
	for (auto tIt = Tool::stack.rbegin(); tIt != Tool::stack.rend(); ++tIt)
	{
		if ((*tIt)->Draw2D(*this))
			return true;
	}
	return false;
}

/*
==================
GridViewRenderer::DrawSizeInfo

lunaran TODO: simplify
==================
*/
void GridViewRenderer::DrawSizeInfo(const vec3 vMinBounds, const vec3 vMaxBounds)
{
	if (!g_cfgUI.ShowSizeInfo)
		return;

	vec3	vSize;
	char	dimstr[128];
	const char *g_pszDimStrings[] = { "x:%.f", "y:%.f", "z:%.f" };
	const char *g_pszOrgStrings[] = { "(x:%.f  y:%.f)", "(x:%.f  z:%.f)", "(y:%.f  z:%.f)" };
	float six, ten;

	vSize = vMaxBounds - vMinBounds;

	glColor3f(g_colors.selection[0] * .65,
		g_colors.selection[1] * .65,
		g_colors.selection[2] * .65);

	six = 6.0f / gv.GetScale();
	ten = 10.0f / gv.GetScale();

	if (gv.GetAxis() == GRID_XY)	// dimU = 0, dimV = 1, axis = 2
	{
		glBegin(GL_LINES);

		glVertex3f(vMinBounds[gv.DimU()], vMinBounds[gv.DimV()] - six, 0.0f);
		glVertex3f(vMinBounds[gv.DimU()], vMinBounds[gv.DimV()] - ten, 0.0f);

		glVertex3f(vMinBounds[gv.DimU()], vMinBounds[gv.DimV()] - ten, 0.0f);
		glVertex3f(vMaxBounds[gv.DimU()], vMinBounds[gv.DimV()] - ten, 0.0f);

		glVertex3f(vMaxBounds[gv.DimU()], vMinBounds[gv.DimV()] - six, 0.0f);
		glVertex3f(vMaxBounds[gv.DimU()], vMinBounds[gv.DimV()] - ten, 0.0f);

		glVertex3f(vMaxBounds[gv.DimU()] + six, vMinBounds[gv.DimV()], 0.0f);
		glVertex3f(vMaxBounds[gv.DimU()] + ten, vMinBounds[gv.DimV()], 0.0f);

		glVertex3f(vMaxBounds[gv.DimU()] + ten, vMinBounds[gv.DimV()], 0.0f);
		glVertex3f(vMaxBounds[gv.DimU()] + ten, vMaxBounds[gv.DimV()], 0.0f);

		glVertex3f(vMaxBounds[gv.DimU()] + six, vMaxBounds[gv.DimV()], 0.0f);
		glVertex3f(vMaxBounds[gv.DimU()] + ten, vMaxBounds[gv.DimV()], 0.0f);

		glEnd();

		glColor4f(g_colors.selection[0],
			g_colors.selection[1],
			g_colors.selection[2],
			1.0f);

		glRasterPos3f((vMinBounds[gv.DimU()] + vMaxBounds[gv.DimU()]) * 0.5f, vMinBounds[gv.DimV()] - 20.0 / gv.GetScale(), 0.0f);
		sprintf(dimstr, g_pszDimStrings[gv.DimU()], vSize[gv.DimU()]);
		glCallLists(strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);

		glRasterPos3f(vMaxBounds[gv.DimU()] + 16.0 / gv.GetScale(), (vMinBounds[gv.DimV()] + vMaxBounds[gv.DimV()]) * 0.5f, 0.0f);
		sprintf(dimstr, g_pszDimStrings[gv.DimV()], vSize[gv.DimV()]);
		glCallLists(strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);

		glRasterPos3f(vMinBounds[gv.DimU()] + 4, vMaxBounds[gv.DimV()] + 8 / gv.GetScale(), 0.0f);
		sprintf(dimstr, g_pszOrgStrings[0], vMinBounds[gv.DimU()], vMaxBounds[gv.DimV()]);
		glCallLists(strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);
	}
	else if (gv.GetAxis() == GRID_XZ)	// dimU = 0, dimV = 2, axis = 1
	{
		glBegin(GL_LINES);

		glVertex3f(vMinBounds[gv.DimU()], 0.0f, vMinBounds[gv.DimV()] - six);
		glVertex3f(vMinBounds[gv.DimU()], 0.0f, vMinBounds[gv.DimV()] - ten);

		glVertex3f(vMinBounds[gv.DimU()], 0.0f, vMinBounds[gv.DimV()] - ten);
		glVertex3f(vMaxBounds[gv.DimU()], 0.0f, vMinBounds[gv.DimV()] - ten);

		glVertex3f(vMaxBounds[gv.DimU()], 0.0f, vMinBounds[gv.DimV()] - six);
		glVertex3f(vMaxBounds[gv.DimU()], 0.0f, vMinBounds[gv.DimV()] - ten);

		glVertex3f(vMaxBounds[gv.DimU()] + six, 0.0f, vMinBounds[gv.DimV()]);
		glVertex3f(vMaxBounds[gv.DimU()] + ten, 0.0f, vMinBounds[gv.DimV()]);

		glVertex3f(vMaxBounds[gv.DimU()] + ten, 0.0f, vMinBounds[gv.DimV()]);
		glVertex3f(vMaxBounds[gv.DimU()] + ten, 0.0f, vMaxBounds[gv.DimV()]);

		glVertex3f(vMaxBounds[gv.DimU()] + six, 0.0f, vMaxBounds[gv.DimV()]);
		glVertex3f(vMaxBounds[gv.DimU()] + ten, 0.0f, vMaxBounds[gv.DimV()]);

		glEnd();

		glColor3fv((GLfloat*)&g_colors.selection);
		glRasterPos3f((vMinBounds[gv.DimU()] + vMaxBounds[gv.DimU()]) * 0.5f, 0.0f, vMinBounds[gv.DimV()] - 20.0 / gv.GetScale());
		sprintf(dimstr, g_pszDimStrings[gv.DimU()], vSize[gv.DimU()]);
		glCallLists(strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);

		glRasterPos3f(vMaxBounds[gv.DimU()] + 16.0 / gv.GetScale(), 0.0f, (vMinBounds[gv.DimV()] + vMaxBounds[gv.DimV()]) * 0.5f);
		sprintf(dimstr, g_pszDimStrings[gv.DimV()], vSize[gv.DimV()]);
		glCallLists(strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);

		glRasterPos3f(vMinBounds[gv.DimU()] + 4, 0.0f, vMaxBounds[gv.DimV()] + 8 / gv.GetScale());
		sprintf(dimstr, g_pszOrgStrings[1], vMinBounds[gv.DimU()], vMaxBounds[gv.DimV()]);
		glCallLists(strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);
	}
	else	// YZ	// dimU = 1, dimV = 2, axis = 0
	{
		glBegin(GL_LINES);

		glVertex3f(0.0f, vMinBounds[gv.DimU()], vMinBounds[gv.DimV()] - six);
		glVertex3f(0.0f, vMinBounds[gv.DimU()], vMinBounds[gv.DimV()] - ten);

		glVertex3f(0.0f, vMinBounds[gv.DimU()], vMinBounds[gv.DimV()] - ten);
		glVertex3f(0.0f, vMaxBounds[gv.DimU()], vMinBounds[gv.DimV()] - ten);

		glVertex3f(0.0f, vMaxBounds[gv.DimU()], vMinBounds[gv.DimV()] - six);
		glVertex3f(0.0f, vMaxBounds[gv.DimU()], vMinBounds[gv.DimV()] - ten);

		glVertex3f(0.0f, vMaxBounds[gv.DimU()] + six, vMinBounds[gv.DimV()]);
		glVertex3f(0.0f, vMaxBounds[gv.DimU()] + ten, vMinBounds[gv.DimV()]);

		glVertex3f(0.0f, vMaxBounds[gv.DimU()] + ten, vMinBounds[gv.DimV()]);
		glVertex3f(0.0f, vMaxBounds[gv.DimU()] + ten, vMaxBounds[gv.DimV()]);

		glVertex3f(0.0f, vMaxBounds[gv.DimU()] + six, vMaxBounds[gv.DimV()]);
		glVertex3f(0.0f, vMaxBounds[gv.DimU()] + ten, vMaxBounds[gv.DimV()]);

		glEnd();

		glColor3fv((GLfloat*)&g_colors.selection);
		glRasterPos3f(0.0f, (vMinBounds[gv.DimU()] + vMaxBounds[gv.DimU()]) * 0.5f, vMinBounds[gv.DimV()] - 20.0 / gv.GetScale());
		sprintf(dimstr, g_pszDimStrings[gv.DimU()], vSize[gv.DimU()]);
		glCallLists(strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);

		glRasterPos3f(0.0f, vMaxBounds[gv.DimU()] + 16.0 / gv.GetScale(), (vMinBounds[gv.DimV()] + vMaxBounds[gv.DimV()]) * 0.5f);
		sprintf(dimstr, g_pszDimStrings[gv.DimV()], vSize[gv.DimV()]);
		glCallLists(strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);

		glRasterPos3f(0.0f, vMinBounds[gv.DimU()] + 4.0, vMaxBounds[gv.DimV()] + 8 / gv.GetScale());
		sprintf(dimstr, g_pszOrgStrings[2], vMinBounds[gv.DimU()], vMaxBounds[gv.DimV()]);
		glCallLists(strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);
	}
}

/*
==============
GridViewRenderer::DrawLightRadius
==============
*/
void GridViewRenderer::DrawLightRadius(Brush *pBrush, int nViewType)
{
	float		f, fRadius, fWait;
	float		fOrigX, fOrigY, fOrigZ;
	double		fStep = (2.0f * Q_PI) / 200.0f;

	if (!(pBrush->owner->eclass->showFlags & EFL_LIGHT))
		return;

	// get the value of the "light" key
	fRadius = pBrush->owner->GetKeyValueFloat("light");
	fWait = pBrush->owner->GetKeyValueFloat("wait");
	// if entity's "light" key is not found, default to 300
	if (!fRadius)
		fRadius = 300;
	if (fWait)
		fRadius /= fWait;

	// find the center
	fOrigX = (pBrush->mins[0] + ((pBrush->maxs[0] - pBrush->mins[0]) / 2));
	fOrigY = (pBrush->mins[1] + ((pBrush->maxs[1] - pBrush->mins[1]) / 2));
	fOrigZ = (pBrush->mins[2] + ((pBrush->maxs[2] - pBrush->mins[2]) / 2));

	glDisable(GL_LINE_STIPPLE);
	glLineWidth(1);

	glColor3f(g_colors.selection[0] * 0.5,
		g_colors.selection[1] * 0.5,
		g_colors.selection[2] * 0.5);
	glBegin(GL_LINE_STRIP);
	for (f = 0; f <= 8; f += fStep)
	{
		if (gv.GetAxis() == GRID_XY)
			glVertex3f(fOrigX + fRadius * cos(f), fOrigY + fRadius * sin(f), fOrigZ);
		else if (gv.GetAxis() == GRID_XZ)
			glVertex3f(fOrigX + fRadius * cos(f), fOrigY, fOrigZ + fRadius * sin(f));
		else
			glVertex3f(fOrigX, fOrigY + fRadius * cos(f), fOrigZ + fRadius * sin(f));
	}
	glEnd();

	glColor3f(g_colors.selection[0] * 0.75,
		g_colors.selection[1] * 0.75,
		g_colors.selection[2] * 0.75);
	glBegin(GL_LINE_STRIP);
	for (f = 0; f <= 8; f += fStep)
	{
		if (gv.GetAxis() == GRID_XY)
			glVertex3f(fOrigX + (fRadius * 0.667) * cos(f), fOrigY + (fRadius * 0.667) * sin(f), fOrigZ);
		else if (gv.GetAxis() == GRID_XZ)
			glVertex3f(fOrigX + (fRadius * 0.667) * cos(f), fOrigY, fOrigZ + (fRadius * 0.667) * sin(f));
		else
			glVertex3f(fOrigX, fOrigY + (fRadius * 0.667) * cos(f), fOrigZ + (fRadius * 0.667) * sin(f));
	}
	glEnd();

	glColor4f(g_colors.selection[0],
		g_colors.selection[1],
		g_colors.selection[2],
		1.0f);
	glBegin(GL_LINE_STRIP);
	for (f = 0; f <= 8; f += fStep)
	{
		if (gv.GetAxis() == GRID_XY)
			glVertex3f(fOrigX + (fRadius * 0.334) * cos(f), fOrigY + (fRadius * 0.334) * sin(f), fOrigZ);
		else if (gv.GetAxis() == GRID_XZ)
			glVertex3f(fOrigX + (fRadius * 0.334) * cos(f), fOrigY, fOrigZ + (fRadius * 0.334) * sin(f));
		else
			glVertex3f(fOrigX, fOrigY + (fRadius * 0.334) * cos(f), fOrigZ + (fRadius * 0.334) * sin(f));
	}
	glEnd();

	glLineWidth(2);
	if (g_cfgUI.Stipple)
		glEnable(GL_LINE_STIPPLE);
}


/*
==============
GridViewRenderer::BeginDrawSelection
==============
*/
void GridViewRenderer::BeginDrawSelection(vec3 selColor)
{
	if (g_cfgUI.Stipple)
		glEnable(GL_LINE_STIPPLE);
	if (Selection::g_selMode != sel_face)
	{
		glLineStipple(3, 0xaaaa);
		glLineWidth(2);
	}
	else
	{
		glLineStipple(2, 0xaaaa);
	}
	glColor4f(selColor[0], selColor[1], selColor[2], 1.0f);
}

/*
==============
GridViewRenderer::EndDrawSelection
==============
*/
void GridViewRenderer::EndDrawSelection()
{
	glDisable(GL_LINE_STIPPLE);
	glLineWidth(1);
}

void GridViewRenderer::DrawBrushSelected(Brush *br)
{
	BeginDrawSelection(g_colors.selection);
	br->DrawXY(gv.GetAxis());
	EndDrawSelection();
	DrawSizeInfo(br->mins, br->maxs);
}

/*
==============
GridViewRenderer::DrawSelection
==============
*/
void GridViewRenderer::DrawSelection(vec3 selColor)
{
	bool	bFixedSize;
	vec3	vMinBounds, vMaxBounds;
	Brush	*brush;

	BeginDrawSelection(selColor);

	if (Selection::g_selMode == sel_face)
	{
		for (auto fIt = Selection::faces.begin(); fIt != Selection::faces.end(); ++fIt)
		{
			(*fIt)->DrawWire();
		}
		EndDrawSelection();
		return;
	}

	// paint size
	ClearBounds(vMinBounds, vMaxBounds);
	bFixedSize = false;

	for (brush = g_brSelectedBrushes.Next(); brush != &g_brSelectedBrushes; brush = brush->Next())
	{
		brush->DrawXY(gv.GetAxis());

		// sikk---> Light Radius
		if (g_cfgUI.ShowLightRadius)
			DrawLightRadius(brush, gv.GetAxis());
		// <---sikk

		// paint size
		if (g_cfgUI.ShowSizeInfo)
		{
			if (!bFixedSize)
			{
				if (brush->owner->IsPoint())
					bFixedSize = true;
				for (int i = 0; i < 3; i++)
				{
					vMinBounds[i] = min(vMinBounds[i], brush->mins[i]);
					vMaxBounds[i] = max(vMaxBounds[i], brush->maxs[i]);
				}
			}
		}
	}

	EndDrawSelection();

	// paint size
	if (!bFixedSize)
		DrawSizeInfo(vMinBounds, vMaxBounds);
}

/*
==============
GridViewRenderer::Draw
==============
*/
void GridViewRenderer::Draw()
{
	Brush	*brush;
	Entity	*e;
	double	start, end;
	vec3	mins, maxs;

	//if (!g_map.brActive.next)
	//	return;	// not valid yet

	if (timing)
		start = Sys_DoubleTime();

	glViewport(0, 0, gv.GetWidth(), gv.GetHeight());
	glClearColor(g_colors.gridBackground[0],
		g_colors.gridBackground[1],
		g_colors.gridBackground[2],
		0);
	glClear(GL_COLOR_BUFFER_BIT);

	// set up viewpoint
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	vMins = gv.GetMins();
	vMaxs = gv.GetMaxs();

	glOrtho(vMins[gv.DimU()], vMaxs[gv.DimU()],
			vMins[gv.DimV()], vMaxs[gv.DimV()],
			vMins[gv.GetAxis()], vMaxs[gv.GetAxis()]);

	// now draw the grid
	DrawGrid();

	// draw stuff
	glShadeModel(GL_FLAT);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glColor3f(0, 0, 0);
	//	glEnable (GL_LINE_SMOOTH);

	if (gv.GetAxis() != GRID_XY)
	{
		glPushMatrix();
		if (gv.GetAxis() == GRID_YZ)
			glRotatef(-90, 0, 1, 0);	// put Z going up
		glRotatef(-90, 1, 0, 0);	    // put Z going up
	}

	e = g_map.world;
	for (brush = g_map.brActive.Next(); brush != &g_map.brActive; brush = brush->Next())
	{
		if (brush->IsFiltered())
			continue;
		if (gv.CullBrush(brush))
			continue;

		if (brush->owner != e && brush->owner)
			glColor3fv(&brush->owner->eclass->color.r);
		else
			glColor3fv(&g_colors.brush.r);

		brush->DrawXY(gv.GetAxis());
	}

	DrawPathLines();

	// draw pointfile
	if (g_qeglobals.d_nPointfileDisplayList)
		glCallList(g_qeglobals.d_nPointfileDisplayList);

	if (!DrawTools())
		DrawSelection(g_colors.selection);

	if (gv.GetAxis() != GRID_XY)
		glPopMatrix();

	// now draw camera point
	DrawCameraIcon();
	//if (g_wndZ->IsOpen()) // VIEWREFACTOR fixme
		DrawZIcon();
	DrawCoords();	// sikk - Draw Coords last so they are on top
	DrawViewName();

	glFinish();

	if (timing)
	{
		end = Sys_DoubleTime();
		Log::Print(_S("Grid: %d ms\n")<< (int)(1000 * (end - start)));
	}
}

//==============================
//	CameraRenderer.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "CameraRenderer.h"
#include "CameraView.h"
#include "map.h"
#include "points.h"
#include "select.h"
#include "Tool.h"

#include <glm/gtc/matrix_transform.hpp>


// a renderer is bound to one view for its entire lifetime
CameraRenderer::CameraRenderer(CameraView &view) : cv(view) {}
CameraRenderer::~CameraRenderer() {}



/*
==============
CameraRenderer::DrawActive

Display Axis in lower left corner of window and rotate with camera orientation
==============
*/
void CameraRenderer::DrawAxis()
{
	if (!g_cfgUI.ShowAxis)
		return;

	glDisable(GL_DEPTH_TEST);

	glViewport(0, 0, 80, 80);
	glm::mat4 proj = glm::perspective(0.75f, 1.0f, 1.0f, 256.0f);
	proj = RotateMatrix(proj, cv.GetAngles()[0], cv.GetAngles()[1]);
	proj = glm::translate(proj, cv.GetViewDir() * 64.0f);
	glLoadMatrixf(&proj[0][0]);

	glLineWidth(3.0);
	glColor3f(1.0, 0.0, 0.0);
	glBegin(GL_LINES);
	glVertex3f(0, 0, 0);
	glVertex3f(16, 0, 0);
	glEnd();

	glColor3f(0.0, 1.0, 0.0);
	glBegin(GL_LINES);
	glVertex3f(0, 0, 0);
	glVertex3f(0, 16, 0);
	glEnd();

	glColor3f(0.0f, 0.3f, 1.0f);
	glBegin(GL_LINES);
	glVertex3f(0, 0, 0);
	glVertex3f(0, 0, 16);
	glEnd();

	glLineWidth(1);

	// draw an indicator on the axis showing which two axes are mapped to the view plane
	vec3 vaxis = cv.GetPlanarUp() + cv.GetPlanarRight();
	if (vaxis[0] == 0)
	{
		// y and z
		glColor3f(0.0f, 0.3f, 1.0f);
		glBegin(GL_LINES);
		glVertex3f(0, 0, 8);
		glVertex3f(0, 8, 8);
		glEnd();

		glColor3f(0.0, 1.0, 0.0);
		glBegin(GL_LINES);
		glVertex3f(0, 8, 0);
		glVertex3f(0, 8, 8);
		glEnd();
	}
	else if (vaxis[1] == 0)
	{
		// x and z
		glColor3f(0.0f, 0.3f, 1.0f);
		glBegin(GL_LINES);
		glVertex3f(0, 0, 8);
		glVertex3f(8, 0, 8);
		glEnd();

		glColor3f(1.0, 0.0, 0.0);
		glBegin(GL_LINES);
		glVertex3f(8, 0, 0);
		glVertex3f(8, 0, 8);
		glEnd();
	}
	else
	{
		// x and y
		glColor3f(0.0, 1.0, 0.0);
		glBegin(GL_LINES);
		glVertex3f(0, 8, 0);
		glVertex3f(8, 8, 0);
		glEnd();

		glColor3f(1.0, 0.0, 0.0);
		glBegin(GL_LINES);
		glVertex3f(8, 0, 0);
		glVertex3f(8, 8, 0);
		glEnd();
	}

	glEnable(GL_DEPTH_TEST);
}

/*
==============
CameraRenderer::DrawGrid
==============
*/
void CameraRenderer::DrawGrid()
{
	if (!g_cfgUI.ShowCameraGrid)
		return;

	int x, y, i;

	i = g_cfgEditor.MapSize / 2;

	glColor3fv(&g_colors.camGrid.r);
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

/*
==============
CameraRenderer::DrawBoundary
==============
*/
void CameraRenderer::DrawBoundary()
{
	if (!g_cfgUI.ShowMapBoundary) return;

	int bound = g_cfgEditor.MapSize / 2;
	glColor3fv(&g_colors.camGrid.r);
	glBegin(GL_LINE_LOOP);
	glVertex3f(-bound, -bound, -bound);
	glVertex3f(bound, -bound, -bound);
	glVertex3f(bound, bound, -bound);
	glVertex3f(-bound, bound, -bound);
	glEnd();

	glBegin(GL_LINE_LOOP);
	glVertex3f(-bound, -bound, bound);
	glVertex3f(bound, -bound, bound);
	glVertex3f(bound, bound, bound);
	glVertex3f(-bound, bound, bound);
	glEnd();

	glBegin(GL_LINES);
	glVertex3f(-bound, -bound, -bound);
	glVertex3f(-bound, -bound, bound);
	glVertex3f(bound, -bound, -bound);
	glVertex3f(bound, -bound, bound);
	glVertex3f(-bound, bound, -bound);
	glVertex3f(-bound, bound, bound);
	glVertex3f(bound, bound, -bound);
	glVertex3f(bound, bound, bound);
	glEnd();
}

/*
==============
CameraRenderer::DrawActive

draw active (unselected) brushes, sorting out the transparent ones for a separate draw
==============
*/
void CameraRenderer::DrawActive()
{
	transBrushes.clear();

	for (Brush *brush = g_map.brActive.Next(); brush != &g_map.brActive; brush = brush->Next())
	{
		if (brush->IsFiltered())
			continue;
		if (cv.CullBrush(brush))
			continue;
		// TODO: Make toggle via Preferences Option. 
		assert(brush->faces->texdef.tex);
		if (brush->showFlags & BFL_TRANS)
			transBrushes.push_back(brush);
		else
		{
			brush->Draw();
		}
	}

	// draw the transparent brushes
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_BLEND);
	//glEnable(GL_TEXTURE_2D);
	//glDepthFunc(GL_LESS);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glDepthMask(GL_FALSE);
	for (auto tbIt = transBrushes.begin(); tbIt != transBrushes.end(); ++tbIt)
		(*tbIt)->Draw();
	//glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

}

/*
==============
CameraRenderer::DrawSelected

draw selected brushes textured, then in red, then in wireframe
==============
*/
void CameraRenderer::DrawSelected(Brush	*pList, vec3 selColor)
{
	Brush	*brush;
	Face	*face;

	// Draw selected brushes
	glMatrixMode(GL_PROJECTION);

	// draw brushes first normally
	for (brush = pList->Next(); brush != pList; brush = brush->Next())
		brush->Draw();

	// redraw tint on top
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_BLEND);

	// lunaran: brighten & clarify selection tint, use selection color preference
	//glColor4f(selColor[0], selColor[1], selColor[2], 0.3f);
	GLSelectionColorAlpha(0.3f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDisable(GL_TEXTURE_2D);

	// fully selected brushes, then loose faces
	for (brush = pList->Next(); brush != pList; brush = brush->Next())
		for (face = brush->faces; face; face = face->fnext)
			face->Draw();

	for (auto fIt = Selection::faces.begin(); fIt != Selection::faces.end(); ++fIt)
		(*fIt)->Draw();

	// non-zbuffered outline
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glColor3f(1, 1, 1);

	for (brush = pList->Next(); brush != pList; brush = brush->Next())
		for (face = brush->faces; face; face = face->fnext)
			face->Draw();

	glEnable(GL_DEPTH_TEST);

}

/*
==============
CameraRenderer::DrawTools
==============
*/
bool CameraRenderer::DrawTools()
{
	for (auto tIt = Tool::stack.rbegin(); tIt != Tool::stack.rend(); ++tIt)
	{
		if ((*tIt)->Draw3D(*this))
			return true;
	}
	return false;
}

void CameraRenderer::DrawMode(int mode)
{
	switch (mode)
	{
	case CD_WIRE:
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		glDisable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_1D);
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);

		glColor3f(1.0, 1.0, 1.0);
		//glEnable (GL_LINE_SMOOTH);
		break;

	case CD_FLAT:
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glCullFace(GL_FRONT);
		glShadeModel(GL_FLAT);
		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
		break;

	case CD_TEXTURED:
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glCullFace(GL_FRONT);
		glShadeModel(GL_FLAT);
		glEnable(GL_CULL_FACE);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDisable(GL_BLEND);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		break;

		/*
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
		break;*/
	}
}

/*
==============
CameraRenderer::Draw
==============
*/
void CameraRenderer::Draw()
{
	double	start, end;

	//if (!g_map.brActive.next)
	//	return;	// not valid yet

	if (timing)
		start = Sys_DoubleTime();

	glViewport(0, 0, cv.GetWidth(), cv.GetHeight());
	//glScissor(0, 0, cv.width, cv.height);
	glClearColor(g_colors.camBackground[0],
		g_colors.camBackground[1],
		g_colors.camBackground[2],
		0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// set up viewpoint
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	matProj = cv.GetProjection();
	glLoadMatrixf(&matProj[0][0]);

	DrawMode(g_cfgUI.DrawMode);

	// sikk---> Camera Grid/Axis/Map Boundary Box
	DrawGrid();
	DrawBoundary();
	// <---sikk

	DrawActive();
	if (!DrawTools())
		DrawSelected(&g_brSelectedBrushes, g_colors.selection);

	glEnable(GL_DEPTH_TEST);
	DrawPathLines();
	if (g_qeglobals.d_nPointfileDisplayList)
		Pointfile_Draw();
	DrawAxis();

	glBindTexture(GL_TEXTURE_2D, 0);
	glFinish();
	if (timing)
	{
		end = Sys_DoubleTime();
		Sys_Printf("Camera: %d ms\n", (int)(1000 * (end - start)));
	}
}
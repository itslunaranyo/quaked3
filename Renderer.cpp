//==============================
//	Renderer.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "Renderer.h"
#include "map.h"


Renderer::Renderer() : timing(false)
{
}

Renderer::~Renderer()
{
}

void Renderer::Draw()
{
	glClearColor((float)rand() / RAND_MAX, (float)rand() / RAND_MAX, (float)rand() / RAND_MAX, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}


/*
==================
Renderer::DrawPathLines

Draws connections between entities.
Called for both camera view and xy view.
TODO: TargetGraphRenderer
==================
*/
void Renderer::DrawPathLines()
{
	if (!g_cfgUI.PathlineMode)
		return;

	TargetGraph::edgeGeo line;
	while (g_map.targetGraph.YieldEdge(line))
	{
		vec3 dir = line.end - line.start;
		vec3 right = CrossProduct(dir, vec3(0, 0, 1));
		float len = VectorNormalize(dir);
		if (VectorNormalize(right) < EQUAL_EPSILON)
			right = vec3(1, 0, 0);
		glColor3fv(&line.color.r);

		glBegin(GL_LINES);
		glVertex3fv(&line.start.x);
		glVertex3fv(&line.end.x);

		int arrows = (int)(len / 256) + 1;
		for (int i = 0; i < arrows; i++)
		{
			vec3 a, b, c;
			float f = len * (i + 0.5f) / arrows;
			b = line.start + f * dir;

			a = b + 5.0f * right - 8.0f * dir;
			glVertex3fv(&a.x);
			glVertex3fv(&b.x);

			c = b - 5.0f * right - 8.0f * dir;
			glVertex3fv(&b.x);
			glVertex3fv(&c.x);

		}
		glEnd();
	}
}

void Renderer::GLSelectionColor()
{
	if (GetKeyState(VK_SPACE) < 0 && g_cfgEditor.CloneStyle == CLONE_DRAG)
		glColor4f(g_colors.tool[0],
			g_colors.tool[1],
			g_colors.tool[2],
			1.0f);
	else
		glColor4f(g_colors.selection[0],
			g_colors.selection[1],
			g_colors.selection[2],
			1.0f);
}

void Renderer::GLSelectionColorAlpha(float alpha)
{
	if (GetKeyState(VK_SPACE) < 0 && g_cfgEditor.CloneStyle == CLONE_DRAG)
		glColor4f(g_colors.tool[0],
			g_colors.tool[1],
			g_colors.tool[2],
			alpha);
	else
		glColor4f(g_colors.selection[0],
			g_colors.selection[1],
			g_colors.selection[2],
			alpha);
}

void Renderer::Text(const char *str, int x, int y)
{
	glRasterPos2f(x,y);
	glCallLists(strlen(str), GL_UNSIGNED_BYTE, str);
}


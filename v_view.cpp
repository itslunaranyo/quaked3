//==============================
//	view.cpp
//==============================

#include "qe3.h"



View::View() : origin(0), scale(1), timing(false)
{
}


View::~View()
{
}


void View::Draw()
{
	glClearColor((float)rand() / RAND_MAX, (float)rand() / RAND_MAX, (float)rand() / RAND_MAX, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/*
==================
View::DrawPathLines

Draws connections between entities.
Needs to consider all entities, not just ones on screen,
because the lines can be visible when neither end is.
Called for both camera view and xy view.
==================
*/
void View::DrawPathLines()
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
/*
void View::DrawPathLines()
{
	int			i, j, k;
	vec3		mid, mid1;
	Entity		*se, *te;
	Brush		*sb, *tb;
	char		*psz;
	vec3		dir, s1, s2;
	float		len, f;
	int			arrows;
	int			num_entities;
	char		*ent_target[MAX_MAP_ENTITIES];
	Entity		*ent_entity[MAX_MAP_ENTITIES];

	if (!g_cfgUI.PathlineMode)
		return;

	num_entities = 0;
	for (te = g_map.entities.next; te != &g_map.entities && num_entities != MAX_MAP_ENTITIES; te = te->next)
	{
		ent_target[num_entities] = te->GetKeyValue("target");
		if (ent_target[num_entities][0])
		{
			ent_entity[num_entities] = te;
			num_entities++;
		}
	}

	for (se = g_map.entities.next; se != &g_map.entities; se = se->next)
	{

		psz = se->GetKeyValue("targetname");

		if (psz == NULL || psz[0] == '\0')
			continue;

		sb = se->brushes.onext;
		if (sb == &se->brushes)
			continue;

		for (k = 0; k < num_entities; k++)
		{
			if (strcmp(ent_target[k], psz))
				continue;

			te = ent_entity[k];
			tb = te->brushes.onext;
			if (tb == &te->brushes)
				continue;

			// sikk---> Don't Draw Path Line if entity on either side is hidden/filtered/regioned off
			if (tb->IsFiltered() || se->brushes.onext->IsFiltered())
				continue;
			if (g_map.IsBrushFiltered(tb) || g_map.IsBrushFiltered(se->brushes.onext))
				continue;
			// <---sikk

			for (i = 0; i < 3; i++)
				mid[i] = (sb->mins[i] + sb->maxs[i]) * 0.5;

			for (i = 0; i < 3; i++)
				mid1[i] = (tb->mins[i] + tb->maxs[i]) * 0.5;

			dir = mid1 - mid;
			len = VectorNormalize(dir);
			s1[0] = -dir[1] * 8 + dir[0] * 8;
			s2[0] = dir[1] * 8 + dir[0] * 8;
			s1[1] = dir[0] * 8 + dir[1] * 8;
			s2[1] = -dir[0] * 8 + dir[1] * 8;

			glColor3f(se->eclass->color[0], se->eclass->color[1], se->eclass->color[2]);

			glBegin(GL_LINES);
			glVertex3fv(&mid.x);
			glVertex3fv(&mid1.x);

			arrows = (int)(len / 256) + 1;

			for (i = 0; i < arrows; i++)
			{
				f = len * (i + 0.5) / arrows;

				for (j = 0; j < 3; j++)
					mid1[j] = mid[j] + f * dir[j];
				glVertex3fv(&mid1.x);
				glVertex3f(mid1[0] + s1[0], mid1[1] + s1[1], mid1[2]);
				glVertex3fv(&mid1.x);
				glVertex3f(mid1[0] + s2[0], mid1[1] + s2[1], mid1[2]);
			}

			glEnd();
		}
	}
}
*/


void View::GLSelectionColor()
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

void View::GLSelectionColorAlpha(float alpha)
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



// returns false if the selection hasn't been drawn in a special way by a tool
bool View::DrawTools()
{
	return false;
}

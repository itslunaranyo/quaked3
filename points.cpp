//==============================
//	points.c
//==============================

#include "pre.h"
#include "qe3.h"
#include "points.h"
#include "map.h"
#include "CameraView.h"
#include "XYZView.h"


static vec3		s_pointvecs[MAX_POINTFILE];
static int		s_num_points, s_check_point;

/*
==================
Pointfile_Delete
==================
*/
void Pointfile_Delete ()
{
	char name[1024];

	strcpy(name, g_map.name);
	StripExtension(name);
	strcat(name, ".pts");

	remove(name);
}

/*
==================
Pointfile_Next

advance camera to next point
==================
*/
void Pointfile_Next ()
{
	vec3 dir;

	if (s_check_point >= s_num_points - 2)
	{
		Sys_Printf("End of pointfile.\n");
		return;
	}
	s_check_point++;
	g_vCamera.origin = s_pointvecs[s_check_point];
	// lunaran - grid view reunification
	for (int i = 0; i < 4; i++)
	{
		g_vXYZ[i].origin = s_pointvecs[s_check_point];
	}
	dir = s_pointvecs[s_check_point + 1] - g_vCamera.origin;
	VectorNormalize(dir);
	g_vCamera.angles[1] = atan2(dir[1], dir[0]) * 180 / Q_PI;
	g_vCamera.angles[0] = asin(dir[2]) * 180 / Q_PI;

	WndMain_UpdateWindows(W_SCENE);
}

/*
==================
Pointfile_Prev

advance camera to previous point
==================
*/
void Pointfile_Prev ()
{
	vec3	dir;

	if (s_check_point == 0)
	{
		Sys_Printf("Beginning of pointfile.\n");
		return;
	}
	s_check_point--;
	g_vCamera.origin = s_pointvecs[s_check_point];
	// lunaran - grid view reunification
	for (int i = 0; i < 4; i++)
	{
		g_vXYZ[i].origin = s_pointvecs[s_check_point];
	}
	dir = s_pointvecs[s_check_point + 1] - g_vCamera.origin;
	VectorNormalize(dir);
	g_vCamera.angles[1] = atan2(dir[1], dir[0]) * 180 / Q_PI;
	g_vCamera.angles[0] = asin(dir[2]) * 180 / Q_PI;

	WndMain_UpdateWindows(W_SCENE);
}

/*
==================
Pointfile_Check
==================
*/
bool Pointfile_Check ()
{
	char	name[1024];
	FILE   *f;
	vec3	v;

	strcpy(name, g_map.name);
	StripExtension(name);
	strcat(name, ".pts");

	f = fopen(name, "r");
	if (!f)
		return false;

	Sys_Printf("Reading pointfile %s\n", name);

	if (!g_qeglobals.d_nPointfileDisplayList)
		g_qeglobals.d_nPointfileDisplayList = glGenLists(1);

	s_num_points = 0;
    glNewList(g_qeglobals.d_nPointfileDisplayList,  GL_COMPILE);
	glColor3f(1, 0, 0);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_1D);
	glLineWidth(4);
	glBegin(GL_LINE_STRIP);
	do
	{
		if (fscanf(f, "%f %f %f\n", &v[0], &v[1], &v[2]) != 3)
			break;
		if (s_num_points < MAX_POINTFILE)
		{
			s_pointvecs[s_num_points] = v;
			s_num_points++;
		}
		glVertex3fv(&v.x);
	} while (1);
	glEnd();
	glLineWidth(1);
	glEndList();

	s_check_point = 0;
	fclose(f);
	Pointfile_Next();

	return true;
}

/*
==================
Pointfile_Draw
==================
*/
void Pointfile_Draw ()
{
	int i;

	glColor3f(1.0f, 0.0f, 0.0f);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_1D);
	glLineWidth(4);
	glBegin(GL_LINE_STRIP);

	for (i = 0; i < s_num_points; i++)
		glVertex3fv(&s_pointvecs[i].x);

	glEnd();
	glLineWidth(1);
}

/*
==================
Pointfile_Clear
==================
*/
void Pointfile_Clear ()
{
	if (!g_qeglobals.d_nPointfileDisplayList)
		return;

	glDeleteLists(g_qeglobals.d_nPointfileDisplayList, 1);
	g_qeglobals.d_nPointfileDisplayList = 0;

	WndMain_UpdateWindows(W_SCENE);
}

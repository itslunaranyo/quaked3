//==============================
//	CmdCzgCylinder.cpp
//==============================

#include "qe3.h"
#include "CmdCzgCylinder.h"

CmdCzgCylinder::CmdCzgCylinder() : degree(1), target(nullptr), axis(2), Command("Make CZG Cylinder") {}
CmdCzgCylinder::~CmdCzgCylinder() {}

void CmdCzgCylinder::SetDegree(int d)
{
	if (d < 1)
		Error("Couldn't create czg cylinder: invalid degree (minimum: 1)");

	if (6 * (1 << d) >= MAX_POINTS_ON_WINDING - 2)
		Error("Couldn't create czg cylinder: too many sides (maximum degree: 3)");

	degree = d;
}

void CmdCzgCylinder::SetAxis(int ax)
{
	if (ax < 0 || ax > 2)
		Error("Bad axis specified (%i)", ax);

	axis = ax;
}

void CmdCzgCylinder::UseBrush(Brush *br)
{
	if (br->owner->IsPoint())
		Error("Can't make a czg cylinder out of a point entity");
	target = br;
	state = LIVE;
}

//==============================

float *CmdCzgCylinder::PatternForDegree(int d)
{
	int i, j, step;
	float *temp, *pattern;

	// seed pattern
	temp = nullptr;
	pattern = new float[3];
	pattern[0] = 0;
	pattern[1] = 0.5f;
	pattern[2] = 1;

	// a czg curve(tm) can be refined by clipping off all the corners where the faces intersect
	// the grid at +/- 25% segment length, so we derive the pattern of vertex offsets one
	// degree at a time because it's more fun than just hardcoding a big array
	for (i = 0; i < d; i++)
	{
		if (temp) delete[] temp;
		temp = pattern;
		step = 6 * (1 << i);
		pattern = new float[step];
		pattern[0] = 0;
		pattern[step - 1] = 1;

		for (j = 0; j < step/2-1; j++)
		{
			pattern[2*j + 1] = temp[j] * 0.75 + temp[j + 1] * 0.25;
			pattern[2*j + 2] = temp[j] * 0.25 + temp[j + 1] * 0.75;
		}
	}

	// mirror it now so we don't have to try to address it backwards for the other semicircle
	delete[] temp;
	temp = pattern;
	pattern = new float[step*2];
	for (i = 0; i < step; i++)
		pattern[i] = temp[i];
	for (i = 0; i < step; i++)
		pattern[2*step-i-1] = temp[i];

	delete[] temp;
	return pattern;
}

void CmdCzgCylinder::Do_Impl()
{
	vec3 mins, maxs, mid, size;
	vec3 p0, p1, p2;
	Face *f;
	TexDef td;
	int i, j, x, y, z, sides;
	float *pattern;

	z = axis;
	x = (axis + 1) % 3;
	y = (axis + 2) % 3;

	sides = 6 * (1 << degree);	// 12, 24, 48
	pattern = PatternForDegree(degree);

	td = target->faces->texdef;
	mins = target->mins;
	maxs = target->maxs;
	size = maxs - mins;

	// find center of brush
	mid = (mins + maxs) * 0.5f;

	cmdBM.ModifyBrush(target);
	target->ClearFaces();

	// create top face
	f = new Face(target);
	f->texdef = td;

	p0[x] = maxs[x];
	p0[y] = maxs[y];
	p0[z] = maxs[z];
	p1[x] = maxs[x];
	p1[y] = mins[y];
	p1[z] = maxs[z];
	p2[x] = mins[x];
	p2[y] = mins[y];
	p2[z] = maxs[z];
	f->plane.FromPoints(p0, p1, p2);

	// create bottom face
	f = new Face(target);
	f->texdef = td;

	p0[x] = mins[x];
	p0[y] = mins[y];
	p0[z] = mins[z];
	p1[x] = maxs[x];
	p1[y] = mins[y];
	p1[z] = mins[z];
	p2[x] = maxs[x];
	p2[y] = maxs[y];
	p2[z] = mins[z];
	f->plane.FromPoints(p0, p1, p2);

	for (i = 0; i < sides; i++)
	{
		j = (i + (sides / 4)) % sides;
		int x1, y1, x2, y2;

		x1 = mins[x] + floor(pattern[i] * size[x] + 0.5);
		x2 = mins[x] + floor(pattern[(i + 1) % sides] * size[x] + 0.5);
		y1 = mins[y] + floor(pattern[j] * size[y] + 0.5);
		y2 = mins[y] + floor(pattern[(j + 1) % sides] * size[y] + 0.5);

		f = new Face(target);
		f->texdef = td;

		p0[x] = x1;
		p0[y] = y1;
		p0[z] = mins[z];
		p1[x] = x2;
		p1[y] = y2;
		p1[z] = mins[z];
		p2[x] = x2;
		p2[y] = y2;
		p2[z] = maxs[z];
		f->plane.FromPoints(p0, p1, p2);
	}

	target->Build();
	cmdBM.Do();	// does nothing, but make sure cmdBM's state is set to DONE

	delete[] pattern;
}

void CmdCzgCylinder::Undo_Impl()
{
	cmdBM.Undo();
}

void CmdCzgCylinder::Redo_Impl()
{
	cmdBM.Redo();
}

void CmdCzgCylinder::Sel_Impl()
{
	Selection::SelectBrush(target);
}



//==============================
//	CmdCzgCylinder.cpp
//==============================

#include "qe3.h"
#include "CmdCzgCylinder.h"

CmdCzgCylinder::CmdCzgCylinder() : degree(1), target(nullptr), axis(2), Command("Make CZG Cylinder")
{
	selectOnDo = true;
	selectOnUndo = true;
}

CmdCzgCylinder::~CmdCzgCylinder() {}

void CmdCzgCylinder::SetDegree(int d)
{
	if (d < 1)
		CmdError("Couldn't create czg cylinder: invalid degree (minimum: 1)");

	if (6 * (1 << d) >= MAX_POINTS_ON_WINDING - 2)
		CmdError("Couldn't create czg cylinder: too many sides (maximum degree: 3)");

	degree = d;
}

void CmdCzgCylinder::SetAxis(int ax)
{
	if (ax < 0 || ax > 2)
		CmdError("Bad axis specified (%i)", ax);

	axis = ax;
}

void CmdCzgCylinder::UseBrush(Brush *br)
{
	if (br->owner->IsPoint())
		CmdError("Can't make a czg cylinder out of a point entity");
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
	vec3 mins, maxs, size;
	int i, x, y, z, sides;
	float *pattern;
	std::vector<vec3> points;

	z = axis;
	x = (axis + 1) % 3;
	y = (axis + 2) % 3;

	sides = 6 * (1 << degree);	// 12, 24, 48
	pattern = PatternForDegree(degree);

	mins = target->mins;
	maxs = target->maxs;
	size = maxs - mins;

	for (i = 0; i < sides; i++)
	{
		vec3 pt;
		int j = (i + (sides / 4)) % sides;

		pt[x] = mins[x] + floor(pattern[i] * size[x] + 0.5);
		pt[y] = mins[y] + floor(pattern[j] * size[y] + 0.5);
		pt[z] = 0;

		points.push_back(pt);
	}

	cmdPB.SetAxis(axis);
	cmdPB.SetBounds(mins[axis],maxs[axis]);
	cmdPB.SetTexDef(target->faces->texdef);
	cmdPB.SetPoints(points);

	cmdAR.RemovedBrush(target);

	cmdAR.Do();
	cmdPB.Do();

	delete[] pattern;
}

void CmdCzgCylinder::Undo_Impl()
{
	cmdPB.Undo();
	cmdAR.Undo();
}

void CmdCzgCylinder::Redo_Impl()
{
	cmdAR.Redo();
	cmdPB.Redo();
}

void CmdCzgCylinder::Sel_Impl()
{
	if (state == DONE)
		cmdPB.Select();
	else
		cmdAR.Select();
}



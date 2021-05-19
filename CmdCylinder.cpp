//==============================
//	CmdCylinder.cpp
//==============================

#include "qe3.h"

CmdCylinder::CmdCylinder() : sides(0), target(nullptr), axis(2) {}
CmdCylinder::~CmdCylinder() {}

void CmdCylinder::SetSides(int s)
{
	if (s < 3)
		Error("Couldn't create cylinder: too few sides (minimum: 3)");

	if (s >= MAX_POINTS_ON_WINDING - 4)
		Error("Couldn't create cylinder: too many sides (maximum: %i)", MAX_POINTS_ON_WINDING - 4);

	sides = s;
}

void CmdCylinder::SetAxis(int ax)
{
	if (ax < 0 || ax > 2)
		Error("Bad axis specified (%i)", ax);

	axis = ax;
}

void CmdCylinder::UseBrush(Brush *br)
{
	if (br->owner->IsPoint())
		Error("Can't make a cylinder out of a point entity");
	target = br;
	state = LIVE;
}

//==============================

void CmdCylinder::Do_Impl()
{
	vec3 mins, maxs, mid;
	int width;
	Face *f;
	texdef_t td;
	int up, right, fwd;

	up = axis;
	right = (axis + 1) % 3;
	fwd = (axis + 2) % 3;

	td = target->basis.faces->texdef;
	mins = target->basis.mins;
	maxs = target->basis.maxs;

	cmdBM.ModifyBrush(target);
	target->ClearFaces();

	// find center of brush
	width = 8;

	for (int i = 0; i < 3; i++)
	{
		mid[i] = (maxs[i] + mins[i]) * 0.5;
		if (i == axis)
			continue;

		if ((maxs[i] - mins[i]) * 0.5 > width)
			width = (maxs[i] - mins[i]) * 0.5;
	}

	// create top face
	f = new Face(target);
	f->texdef = td;

	f->planepts[0][right] =	maxs[right];
	f->planepts[0][fwd] =	maxs[fwd];
	f->planepts[0][up] =	maxs[up];
	f->planepts[1][right] =	maxs[right];
	f->planepts[1][fwd] =	mins[fwd];
	f->planepts[1][up] =	maxs[up];
	f->planepts[2][right] =	mins[right];
	f->planepts[2][fwd] =	mins[fwd];
	f->planepts[2][up] =	maxs[up];

	// create bottom face
	f = new Face(target);
	f->texdef = td;

	f->planepts[0][right] =	mins[right];
	f->planepts[0][fwd] =	mins[fwd];
	f->planepts[0][up] =	mins[up];
	f->planepts[1][right] =	maxs[right];
	f->planepts[1][fwd] =	mins[fwd];
	f->planepts[1][up] =	mins[up];
	f->planepts[2][right] =	maxs[right];
	f->planepts[2][fwd] =	maxs[fwd];
	f->planepts[2][up] =	mins[up];

	for (int i = 0; i < sides; i++)
	{
		float sv, cv;
		f = new Face(target);
		f->texdef = td;

		sv = sin(i * Q_PI * 2 / sides);
		cv = cos(i * Q_PI * 2 / sides);

		f->planepts[0][right] =	floor(mid[right] + width * cv + 0.5);
		f->planepts[0][fwd] =	floor(mid[fwd] + width * sv + 0.5);
		f->planepts[0][up] =	mins[up];

		f->planepts[1][right] =	f->planepts[0][right];
		f->planepts[1][fwd] =	f->planepts[0][fwd];
		f->planepts[1][up] =	maxs[up];

		f->planepts[2][right] =	floor(f->planepts[0][right] - width * sv + 0.5);
		f->planepts[2][fwd] =	floor(f->planepts[0][fwd] + width * cv + 0.5);
		f->planepts[2][up] =	mins[up];
	}

	target->Build();
	cmdBM.Do();	// does nothing, but make sure cmdBM's state is set to DONE
}

void CmdCylinder::Undo_Impl()
{
	cmdBM.Undo();
}

void CmdCylinder::Redo_Impl()
{
	cmdBM.Redo();
}

void CmdCylinder::Sel_Impl()
{
	Selection::SelectBrush(target);
}



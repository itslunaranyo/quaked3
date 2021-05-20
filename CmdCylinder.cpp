//==============================
//	CmdCylinder.cpp
//==============================

#include "qe3.h"
#include "CmdCylinder.h"

CmdCylinder::CmdCylinder() : sides(0), target(nullptr), axis(2), Command("Make Cylinder") {}
CmdCylinder::~CmdCylinder() {}

void CmdCylinder::SetSides(int s)
{
	if (s < 3)
		CmdError("Couldn't create cylinder: too few sides (minimum: 3)");

	if (s >= MAX_POINTS_ON_WINDING - 4)
		CmdError("Couldn't create cylinder: too many sides (maximum: %i)", MAX_POINTS_ON_WINDING - 4);

	sides = s;
}

void CmdCylinder::SetAxis(int ax)
{
	if (ax < 0 || ax > 2)
		CmdError("Bad axis specified (%i)", ax);

	axis = ax;
}

void CmdCylinder::UseBrush(Brush *br)
{
	if (br->owner->IsPoint())
		CmdError("Can't make a cylinder out of a point entity");
	target = br;
	state = LIVE;
}

//==============================

void CmdCylinder::Do_Impl()
{
	vec3 mins, maxs, mid;
	vec3 p0, p1, p2;
	int width;
	Face *f;
	TexDef td;
	int up, right, fwd;

	up = axis;
	right = (axis + 1) % 3;
	fwd = (axis + 2) % 3;

	td = target->faces->texdef;
	mins = target->mins;
	maxs = target->maxs;

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

	p0[right] =	maxs[right];
	p0[fwd] =	maxs[fwd];
	p0[up] =	maxs[up];
	p1[right] =	maxs[right];
	p1[fwd] =	mins[fwd];
	p1[up] =	maxs[up];
	p2[right] =	mins[right];
	p2[fwd] =	mins[fwd];
	p2[up] =	maxs[up];
	f->plane.FromPoints(p0, p1, p2);

	// create bottom face
	f = new Face(target);
	f->texdef = td;

	p0[right] =	mins[right];
	p0[fwd] =	mins[fwd];
	p0[up] =	mins[up];
	p1[right] =	maxs[right];
	p1[fwd] =	mins[fwd];
	p1[up] =	mins[up];
	p2[right] =	maxs[right];
	p2[fwd] =	maxs[fwd];
	p2[up] =	mins[up];
	f->plane.FromPoints(p0, p1, p2);

	for (int i = 0; i < sides; i++)
	{
		float sv, cv;
		f = new Face(target);
		f->texdef = td;

		sv = sin(i * Q_PI * 2 / sides);
		cv = cos(i * Q_PI * 2 / sides);

		p0[right] =	floor(mid[right] + width * cv + 0.5);
		p0[fwd] =	floor(mid[fwd] + width * sv + 0.5);
		p0[up] =	mins[up];

		p1[right] =	p0[right];
		p1[fwd] =	p0[fwd];
		p1[up] =	maxs[up];

		p2[right] =	floor(p0[right] - width * sv + 0.5);
		p2[fwd] =	floor(p0[fwd] + width * cv + 0.5);
		p2[up] =	mins[up];
		f->plane.FromPoints(p0, p1, p2);
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



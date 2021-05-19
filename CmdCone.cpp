//==============================
//	CmdCone.cpp
//==============================

#include "qe3.h"

CmdCone::CmdCone() : sides(0), target(nullptr), axis(2) {}
CmdCone::~CmdCone() {}

void CmdCone::SetSides(int s)
{
	if (s < 3)
		Error("Couldn't create cone: too few sides (minimum: 3)");

	if (s >= MAX_POINTS_ON_WINDING - 4)
		Error("Couldn't create cone: too many sides (maximum: %i)", MAX_POINTS_ON_WINDING - 4);

	sides = s;
}

void CmdCone::SetAxis(int ax)
{
	if (ax < 0 || ax > 2)
		Error("Bad axis specified (%i)", ax);

	axis = ax;
}

void CmdCone::UseBrush(Brush *br)
{
	if (br->owner->IsPoint())
		Error("Can't make a cone out of a point entity");
	target = br;
	state = LIVE;
}

//==============================

void CmdCone::Do_Impl()
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

	// top face
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

	// side faces
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

		f->planepts[1][right] =	mid[right];
		f->planepts[1][fwd] =	mid[fwd];
		f->planepts[1][up] =	maxs[up];

		f->planepts[2][right] =	floor(f->planepts[0][right] - width * sv + 0.5);
		f->planepts[2][fwd] =	floor(f->planepts[0][fwd] + width * cv + 0.5);
		f->planepts[2][up] =	mins[up];
	}

	target->Build();
	cmdBM.Do();	// does nothing, but make sure cmdBM's state is set to DONE
}

void CmdCone::Undo_Impl()
{
	cmdBM.Undo();
}

void CmdCone::Redo_Impl()
{
	cmdBM.Redo();
}

void CmdCone::Select_Impl()
{
	Select_SelectBrush(target);
}



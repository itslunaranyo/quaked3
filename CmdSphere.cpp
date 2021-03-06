//==============================
//	CmdSphere.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "select.h"
#include "CmdSphere.h"

CmdSphere::CmdSphere() : sides(0), target(nullptr), Command("Make Sphere") {}
CmdSphere::~CmdSphere() {}

void CmdSphere::SetSides(int s)
{
	if (s < 4)
		CmdError("Couldn't create sphere: too few sides (minimum: 4)");

	sides = s;
}


void CmdSphere::UseBrush(Brush *br)
{
	if (br->owner->IsPoint())
		CmdError("Can't make a cylinder out of a point entity");
	target = br;
	state = LIVE;
}

//==============================

void CmdSphere::Do_Impl()
{
	vec3 mins, maxs, mid;
	vec3 p0, p1, p2;
	int radius;
	Face *f;
	TexDef td;
	float dt, dp, t, p;
	int i, j;

	td = target->faces->texdef;
	mins = target->mins;
	maxs = target->maxs;

	cmdBM.ModifyBrush(target);
	target->ClearFaces();

	// find center of brush
	radius = 8;

	for (i = 0; i < 2; i++)
	{
		mid[i] = (maxs[i] + mins[i]) * 0.5;
		if (maxs[i] - mins[i] > radius)
			radius = maxs[i] - mins[i];
	}

	radius /= 2;

	dt = 2 * Q_PI / sides;
	dp = Q_PI / sides;

	for (i = 0; i <= sides - 1; i++)
	{
		for (j = 0; j <= sides - 2; j++)
		{
			t = i * dt;
			p = (j * dp - Q_PI / 2);

			f = new Face(target);
			f->texdef = td;

			VectorPolar(p0, radius, t, p);
			VectorPolar(p1, radius, t, p + dp);
			VectorPolar(p2, radius, t + dt, p + dp);
			f->plane.FromPoints(p0 + mid, p1 + mid, p2 + mid);
		}
	}

	p = ((sides - 1) * dp - Q_PI / 2);

	for (i = 0; i <= sides - 1; i++)
	{
		t = i * dt;

		f = new Face(target);
		f->texdef = td;

		VectorPolar(p0, radius, t, p);
		VectorPolar(p1, radius, t + dt, p + dp);
		VectorPolar(p2, radius, t + dt, p);
		f->plane.FromPoints(p0 + mid, p1 + mid, p2 + mid);
	}

	target->Build();
	cmdBM.Do();	// does nothing, but make sure cmdBM's state is set to DONE
}

void CmdSphere::Undo_Impl()
{
	cmdBM.Undo();
}

void CmdSphere::Redo_Impl()
{
	cmdBM.Redo();
}

void CmdSphere::Sel_Impl()
{
	Selection::SelectBrush(target);
}



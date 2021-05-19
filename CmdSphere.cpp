//==============================
//	CmdSphere.cpp
//==============================

#include "qe3.h"

CmdSphere::CmdSphere() : sides(0), target(nullptr) {}
CmdSphere::~CmdSphere() {}

void CmdSphere::SetSides(int s)
{
	if (s < 4)
		Error("Couldn't create sphere: too few sides (minimum: 4)");

	sides = s;
}


void CmdSphere::UseBrush(Brush *br)
{
	if (br->owner->IsPoint())
		Error("Can't make a cylinder out of a point entity");
	target = br;
	state = LIVE;
}

//==============================

void CmdSphere::Do_Impl()
{
	vec3 mins, maxs, mid;
	int radius;
	Face *f;
	texdef_t td;
	float dt, dp, t, p;
	int i, j, k;

	td = target->basis.faces->texdef;
	mins = target->basis.mins;
	maxs = target->basis.maxs;

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

			VectorPolar(f->planepts[0], radius, t, p);
			VectorPolar(f->planepts[1], radius, t, p + dp);
			VectorPolar(f->planepts[2], radius, t + dt, p + dp);

			for (k = 0; k < 3; k++)
				f->planepts[k] = f->planepts[k] + mid;
		}
	}

	p = ((sides - 1) * dp - Q_PI / 2);

	for (i = 0; i <= sides - 1; i++)
	{
		t = i * dt;

		f = new Face(target);
		f->texdef = td;

		VectorPolar(f->planepts[0], radius, t, p);
		VectorPolar(f->planepts[1], radius, t + dt, p + dp);
		VectorPolar(f->planepts[2], radius, t + dt, p);

		for (k = 0; k < 3; k++)
			f->planepts[k] = f->planepts[k] + mid;
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

void CmdSphere::Select_Impl()
{
	Select_SelectBrush(target);
}



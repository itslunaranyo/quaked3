//==============================
//	CmdPolyBrush.cpp
//==============================

#include "qe3.h"
#include "CmdPolyBrush.h"


/*
=============
CmdPolyBrush

create a single prism from a list of points and optional z-bounds (defaults to
workzone bounds if left unspecified). polyBrush doesn't perform sanity checking
for t-junctions, coincident points, intersecting segments, or other degeneracy,
and is intended for internal use by other commands which do. if you need such
guardrails, use CmdPolyBrushConcave.

the input point list must be clockwise.
=============
*/


CmdPolyBrush::CmdPolyBrush() : Command("CmdPolyBrush"), 
	work(nullptr), axis(XY),
	texdef(g_qeglobals.d_workTexDef),
	lowBound(g_qeglobals.d_v3WorkMin[axis]),
	highBound(g_qeglobals.d_v3WorkMax[axis])
{
	if (!texdef.tex)
		texdef.Set(Textures::nulltexture);
	// state = LIVE;
}

CmdPolyBrush::~CmdPolyBrush()
{
}

void CmdPolyBrush::SetPoints(std::vector<vec3> &points)
{
	if (points.size() < 3)
		CmdError("not enough points!");

	state = LIVE;
	pointList = points;
	if (*pointList.begin() != *(pointList.end()-1))
		pointList.push_back(points[0]);	// loop it
}

void CmdPolyBrush::SetBounds(int low, int high)
{
	if (low == high)
		CmdError("cannot create a brush of 0 thickness");

	state = LIVE;
	lowBound = min(low,high);
	highBound = max(low, high);
}

void CmdPolyBrush::SetTexDef(TexDef &tdef)
{
	state = LIVE;
	texdef = tdef;
}

void CmdPolyBrush::SetAxis(int ax)
{
	if (ax < 0 || ax > 2)
		CmdError("Bad axis specified (%i)", ax);

	state = LIVE;
	axis = ax;
}


//==============================

void CmdPolyBrush::Do_Impl()
{
	assert(!work);
	if (pointList.size() < 3)
		CmdError("not enough points!");

	Face *f;
	vec3 mins, maxs;
	vec3 v0, v1, v2;
	vec3 pt1, pt2;
	int x, y, z;

	z = axis;
	x = (axis + 1) % 3;
	y = (axis + 2) % 3;

	for (auto pt = pointList.begin(); pt != pointList.end(); ++pt)
	{
		mins = glm::min(mins, *pt);
		maxs = glm::max(maxs, *pt);
	}
	mins[z] = lowBound;
	maxs[z] = highBound;

	work = new Brush();
	work->owner = g_map.world;
	cmdAR.AddedBrush(work);

	// create top face
	f = new Face(work);
	f->texdef = texdef;

	v0[x] = maxs[x];
	v0[y] = maxs[y];
	v0[z] = maxs[z];
	v1[x] = maxs[x];
	v1[y] = mins[y];
	v1[z] = maxs[z];
	v2[x] = mins[x];
	v2[y] = mins[y];
	v2[z] = maxs[z];
	f->plane.FromPoints(v0, v1, v2);

	// create bottom face
	f = new Face(work);
	f->texdef = texdef;

	v0[x] = mins[x];
	v0[y] = mins[y];
	v0[z] = mins[z];
	v1[x] = maxs[x];
	v1[y] = mins[y];
	v1[z] = mins[z];
	v2[x] = maxs[x];
	v2[y] = maxs[y];
	v2[z] = mins[z];
	f->plane.FromPoints(v0, v1, v2);

	for (auto pt = pointList.begin(); pt != pointList.end() - 1; ++pt)
	{
		pt1 = *pt;
		pt2 = *(pt + 1);
		f = new Face(work);
		f->texdef = texdef;

		v0[x] = pt1[x];
		v0[y] = pt1[y];
		v0[z] = mins[z];
		v1[x] = pt2[x];
		v1[y] = pt2[y];
		v1[z] = mins[z];
		v2[x] = pt2[x];
		v2[y] = pt2[y];
		v2[z] = maxs[z];
		f->plane.FromPoints(v0, v1, v2);
	}

	if (!work->Build())	// brush has no geometry, is probably inside-out
	{
		// if we except here, cmdAR safely destroys the work brush for us
		CmdError("brush had zero bounds - was the point list clockwise?");
	}

	// must set addRemove's state to DONE
	cmdAR.Do();
}

void CmdPolyBrush::Undo_Impl()
{
	cmdAR.Undo();
}

void CmdPolyBrush::Redo_Impl()
{
	cmdAR.Redo();
}

void CmdPolyBrush::Sel_Impl()
{
	cmdAR.Select();
}



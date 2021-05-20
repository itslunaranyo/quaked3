//==============================
//	CmdPolyBrushConcave.cpp
//==============================

#include "qe3.h"
#include "CmdPolyBrushConcave.h"
#include "CmdPolyBrush.h"

CmdPolyBrushConcave::CmdPolyBrushConcave() : Command("CmdPolyBrushConcave"),
	axis(XY), texdef(g_qeglobals.d_workTexDef),
	lowBound(g_qeglobals.d_v3WorkMin[axis]),
	highBound(g_qeglobals.d_v3WorkMax[axis])
{
	selectOnDo = true;
	// state = LIVE;
}

CmdPolyBrushConcave::~CmdPolyBrushConcave()
{
	for (auto loopIt = convexLoops.begin(); loopIt != convexLoops.end(); ++loopIt)
		delete (*loopIt);

	for (auto pbIt = cmdPBs.begin(); pbIt != cmdPBs.end(); ++pbIt)
		delete (*pbIt);
}


/*
=============
CmdPolyBrushConcave::SetPoints

sanitize and digest input point list for internal use
throw if there's something unfixably wrong with it, and let the encapsulating UI or
	script editor handle it
=============
*/
void CmdPolyBrushConcave::SetPoints(std::vector<vec3> &points)
{
	std::vector<vec3> pointsOut;

	if (points.size() < 3)
		CmdError("too few points (need 3+, provided %i)", points.size());

	int upAxis, rightAxis;

	//DetermineAxis(pointsIn);
	upAxis = (axis + 1) % 3;
	rightAxis = (axis + 2) % 3;

	// copy points to internal storage, rotating them to the XY plane
	for (auto pt = points.begin(); pt != points.end(); ++pt)
		pointsOut.push_back(vec3((*pt)[upAxis], (*pt)[rightAxis], 0));

	RemoveDuplicates(pointsOut);
	RemoveCollinear(pointsOut);
	if (pointsOut.size() < 3)
		CmdError("too few points after pruning collinear/duplicates");

	VerifyPoints(pointsOut);
	MakeClockwise(pointsOut);

	pointList.clear();
	pointList = pointsOut;
	state = LIVE;
}

void CmdPolyBrushConcave::SetBounds(int low, int high)
{
	if (low == high)
		CmdError("cannot create brushes of 0 thickness");

	if (low < high)
	{
		lowBound = low;
		highBound = high;
		return;
	}
	lowBound = high;
	highBound = low;
}

void CmdPolyBrushConcave::SetTexDef(TexDef &tdef)
{
	texdef = tdef;
}

void CmdPolyBrushConcave::SetAxis(int ax)
{
	assert(ax >= YZ && ax <= XY);
	axis = ax;
}

//==============================

/*
=============
CmdPolyBrushConcave::ToWorldSpace
=============
*/
void CmdPolyBrushConcave::ToWorldSpace(std::vector<vec3>& points)
{
	if (axis == XY)
		return;

	int upAxis, rightAxis;
	upAxis = (axis + 1) % 3;
	rightAxis = (axis + 2) % 3;

	// rotate points back to requested axis
	for (auto pt = points.begin(); pt != points.end(); ++pt)
	{
		vec3 temp;
		temp[upAxis] = (*pt)[0];
		temp[rightAxis] = (*pt)[1];
		temp[axis] = (*pt)[2];

		*pt = temp;
	}
}


/*
=============
CmdPolyBrushConcave::DetermineAxis

determine vertical axis from input vec3s
we can't assume this from the active viewport because a point list could come from
a script command, meaning it could be anything
=============
void CmdPolyBrushConcave::DetermineAxis(std::vector<vec3>& points)
{
	Plane p;
	p.FromPoints(points[0], points[1], points[2]);

	if (points.size() > 3)
	{
		for (auto pt = points.begin() + 3; pt != points.end(); ++pt)
		{
			if (!p.PointOn(*pt))
				CmdError("input points are not coplanar");
		}
	}

	// FIXME: assumes normal is axial
	if (p.normal[0] != 0)
		axis = YZ;
	else if (p.normal[1] != 0)
		axis = XZ;
	else
		axis = XY;
}
*/


/*
=============
CmdPolyBrushConcave::RemoveDuplicates

merge adjacent duplicates
=============
*/
void CmdPolyBrushConcave::RemoveDuplicates(std::vector<vec3>& points)
{
	std::vector< std::vector<vec3>::iterator > dupes;
	for (auto pt = points.begin(); pt != points.end() - 1; ++pt)
	{
		if (Point_Equal(*pt, *(pt + 1), (float)EQUAL_EPSILON))
			dupes.push_back(pt);
	}
	for (auto dpt = dupes.rbegin(); dpt != dupes.rend(); ++dpt)
	{
		points.erase(*dpt);
	}
}

/*
=============
CmdPolyBrushConcave::RemoveCollinear

remove mid-edge verts

yes it also annoys me that 'collinear' is spelled with two Ls

lost a spelling bee because of 'collinear'

so if I have to put up with it having two Ls then so do you
=============
*/
void CmdPolyBrushConcave::RemoveCollinear(std::vector<vec3>& points)
{
	std::vector< std::vector<vec3>::iterator > removals;

	for (int i = 0; i < (int)points.size(); i++)
	{
		vec3 a, b, c;
		a = points[(i - 1 + points.size()) % points.size()];
		b = points[i];
		c = points[(i + 1) % points.size()];

		if (CrossProduct(b - a, c - a).z == 0)
			removals.push_back(points.begin() + i);
	}

	for (auto rIt = removals.rbegin(); rIt != removals.rend(); ++rIt)
	{
		points.erase(*rIt);
	}
}

/*
=============
CmdPolyBrushConcave::VerifyPoints

check for degenerate cases like t-junctions
adjacent coincident points can just be merged but non-adjacent cannot
intersecting line segments are always illegal
=============
*/
void CmdPolyBrushConcave::VerifyPoints(const std::vector<vec3>& points)
{
	int len = (int)points.size();

	// make sure no edge touches any other edge except at neighbors' endpoints
	for (int i = 0; i < len; i++)
	{
		vec3 sect;		
		for(int j = i + 2; j < len; j++)
		{
			if ((j + 1) % len == i)
				continue;
			if (LineSegmentIntersect2D(points[i], points[(i + 1) % len], points[j], points[(j + 1) % len], sect))
			{
				CmdError("edge list intersects itself at %f %f %f", sect[0], sect[1], sect[2]);
			}
		}
	}
}

/*
=============
CmdPolyBrushConcave::MakeClockwise

put points in clockwise order so downstream code can make ~* a s s u m p t i o n s *~
=============
*/
void CmdPolyBrushConcave::MakeClockwise(std::vector<vec3>& points)
{
	int len = (int)points.size();
	int v = len + 1;
	float leastX = 9999999;

	// shape might be concave, but the point with the lowest x or y coordinate is
	// guaranteed to be an interior angle, so it's safe to test for handedness
	for (int i = 0; i < len; i++)
	{
		if (points[i].x >= leastX)
			continue;
		leastX = points[i].x;
		v = i;
	}

	int prev, next;
	prev = (v - 1 + len) % (len);
	next = (v + 1) % (len);

	vec3 cp = CrossProduct(points[prev] - points[v], points[next] - points[v]);
	if (cp.z < 0)
		std::reverse(points.begin(), points.end()); 
}

//============================================================

/*
=============
CmdPolyBrushConcave::IsReflexVertex
=============
*/
bool CmdPolyBrushConcave::IsReflexVertex(const std::vector<vec3> &loop, const int v)
{
	vec3 cp;
	int prev, next;
	prev = (v - 1 + loop.size()) % loop.size();
	next = (v + 1) % loop.size();
	cp = CrossProduct(loop[prev] - loop[v], loop[next] - loop[v]);
	return (cp.z <= 0);
}

/*
=============
CmdPolyBrushConcave::PickReflexVertex

search a point loop for reflex vertices and return one to split from
give preference to verts with an axial or diagonal adjacent edge
=============
*/
int CmdPolyBrushConcave::PickReflexVertex(std::vector<vec3> &loop)
{
	std::vector<int> rvList;
	rvList.reserve(loop.size());
	int len = (int)loop.size();

	for (int rv = 0; rv < len; rv++)
		if (IsReflexVertex(loop, rv))
			rvList.push_back(rv);

	if (rvList.empty())
		return -1;

	// axial
	for (auto rvIt = rvList.begin(); rvIt != rvList.end(); ++rvIt)
	{
		vec3 dir;
		dir = loop[(*rvIt + 1) % len] - loop[*rvIt];
		if (dir.x == 0 || dir.y == 0)
			return *rvIt;
		dir = loop[(*rvIt - 1 + len) % len] - loop[*rvIt];
		if (dir.x == 0 || dir.y == 0)
			return *rvIt;
	}
	// diagonal
	for (auto rvIt = rvList.begin(); rvIt != rvList.end(); ++rvIt)
	{
		vec3 dir;
		dir = loop[(*rvIt + 1) % len] - loop[*rvIt];
		if (fabs(dir.x / dir.y - 1) < EQUAL_EPSILON)
			return *rvIt;
		dir = loop[(*rvIt - 1 + len) % len] - loop[*rvIt];
		if (fabs(dir.x / dir.y - 1) < EQUAL_EPSILON)
			return *rvIt;
	}
	// whatever
	return rvList.front();
}

/*
=============
CmdPolyBrushConcave::InitialRay

create a cardinal splitting ray, and test its intersection/endpoint
=============
*/
CmdPolyBrushConcave::loopRay_t CmdPolyBrushConcave::InitialRay(const std::vector<vec3> &loop, const int rv, const vec3 dir)
{
	int len = (int)loop.size();
	int e;
	vec3 sect;
	loopRay_t ray(loop[rv] + dir, -1, -1);

	// find what edge or vert this ray hits
	for (e = 0; e < len; e++)
	{
		if (e == rv || (e + 1) % len == rv)	// don't test the edges that share the origin vert
			continue;
		if (LineSegmentIntersect2D(loop[rv], loop[rv] + dir, loop[e], loop[(e + 1) % len], sect))
		{
			ray.endv1 = e;
			ray.endv2 = (e + 1) % len;
			break;
		}
	}
	if (e == len)
		return ray;

	if (sect == loop[e])
		ray.endv2 = ray.endv1;
	else if (sect == loop[(e + 1) % len])
		ray.endv1 = ray.endv2;

	ray.end = sect;
	return ray;
}


/*
=============
CmdPolyBrushConcave::VertexRays

generate cardinal splitting rays
=============
*/
void CmdPolyBrushConcave::InitialRays(std::vector<loopRay_t> &rays, const std::vector<vec3> &loop, const int rv)
{
	// determine largest axis size
	vec3 size, mins, maxs;
	float w;
	ClearBounds(mins, maxs);
	for (int i = 0; i < (int)loop.size(); i++)
		AddPointToBounds(loop[i], mins, maxs);
	size = maxs - mins;
	w = max(size.x, size.y);

	rays.push_back( InitialRay(loop, rv, vec3(w, 0, 0)) );
	rays.push_back( InitialRay(loop, rv, vec3(-w, 0, 0)) );
	rays.push_back( InitialRay(loop, rv, vec3(0, w, 0)) );
	rays.push_back( InitialRay(loop, rv, vec3(0, -w, 0)) );

	rays.push_back( InitialRay(loop, rv, vec3(w, w, 0)) );
	rays.push_back( InitialRay(loop, rv, vec3(-w, w, 0)) );
	rays.push_back( InitialRay(loop, rv, vec3(-w, -w, 0)) );
	rays.push_back( InitialRay(loop, rv, vec3(w, -w, 0)) );
}

/*
=============
CmdPolyBrushConcave::VertexRays

generate valid splitting rays to every other vertex 'in sight'
=============
*/
void CmdPolyBrushConcave::VertexRays(std::vector<loopRay_t> &rays, const std::vector<vec3> &loop, const int rv)
{
	int len = (int)loop.size();
	for (int v = 0; v < len; v++)
	{
		// don't test the vert where we're starting or its two neighbors
		if ((v == rv) || (v == (rv - 1 + len) % len) || (v == (rv + 1) % len))
			continue;

		vec3 dest = loop[v];
		bool ok = true;

		// cull if it crosses other edges before reaching destination
		for (int l = 0; l < len; l++)
		{
			vec3 sect;
			if (l == v || (l + 1) % len == v || 	// don't test the edges that share the destination vert
				l == rv || (l + 1) % len == rv )	// don't test the edges that share the origin vert
				continue;
			if (LineSegmentIntersect2D(loop[rv], dest, loop[l], loop[(l + 1) % len], sect))
			{
				ok = false;
				break;
			}
		}
		if (ok)
			rays.emplace_back(dest, v, v);
	}
}

/*
=============
CmdPolyBrushConcave::BestSplit

find the best place to split a concave shape in two, originating at a given vertex

"best" is defined as "most likely to match how an experienced mapper would choose 
	to hand-create the convex brushes that form the desired shape"
=============
*/
CmdPolyBrushConcave::loopRay_t CmdPolyBrushConcave::BestSplit(const std::vector<vec3> &loop, const int rv)
{
	vec3 prevEdge, nextEdge;
	float pc, nc, x;

	int len = loop.size();

	// send out a ray on all 90s and all 45s
	std::vector<loopRay_t> rays;
	InitialRays(rays, loop, rv);

	// send out a ray to every other vertex
	VertexRays(rays, loop, rv);

	// measure various attributes of a split candidate for later use in preferential weighting
	prevEdge = loop[(rv - 1 + len) % len] - loop[rv];
	nextEdge = loop[(rv + 1) % len] - loop[rv];

	for (auto rayIt = rays.begin(); rayIt != rays.end(); ++rayIt)
	{
		// weight by position in quadrants formed by the intersection of the two edges
		// cull entirely (weight 0) if it's in the outside-the-shape quadrant
		vec3 raydir = rayIt->end - loop[rv];
		pc = CrossProduct(prevEdge, raydir).z;
		nc = CrossProduct(nextEdge, raydir).z;

		if (pc <= 0 && nc >= 0)	// starts in the 'void'
		{
			rayIt->wflags = 0;
			continue;	// no point in doing the rest of the weighting
		}
		else if (pc == 0 || nc == 0)	// on edge of ideal cone - this is good as it will lead to brushplanes merging
		{
			rayIt->wflags |= collinear;
		}
		else if (pc >= 0 && nc <= 0)
		{
			vec3 coneCenter = glm::normalize(-prevEdge) + glm::normalize(-nextEdge);
			vec3 rayNorm = raydir;
			VectorNormalize(coneCenter);
			VectorNormalize(rayNorm);
			if (DotProduct(coneCenter, rayNorm) > 0.98f)	// very close to the center of the cone
				rayIt->wflags |= cone_core;	
			else
				rayIt->wflags |= cone_reflex;	// otherwise within cone
		}
		else
		{
			rayIt->wflags |= cone_side;	// within two side quadrants
		}

		// weight by angle
		x = fabs(raydir.x / raydir.y);
		if (raydir.x == 0 || raydir.y == 0)
			rayIt->wflags |= axial;
		else if (fabs(x - 1.0f) < EQUAL_EPSILON || // a 45 degree diagonal?
				 fabs(x - 0.5f) < EQUAL_EPSILON || // or that sweet sweet 2:1 slope?
				 fabs(x - 2.0f) < EQUAL_EPSILON )
			rayIt->wflags |= diagonal;
		else
			rayIt->wflags |= angular;

		assert(rayIt->endv1 != -1);

		// weight by situation at the end of the ray
		if (rayIt->endv1 == rayIt->endv2)
		{
			rayIt->wflags |= IsReflexVertex(loop, rayIt->endv1) ? vertex_reflex : vertex;
		}
		else
		{
			vec3 edir = loop[rayIt->endv1] - loop[rayIt->endv2];
			if (edir.x == 0 || edir.y == 0)
			{
				rayIt->wflags |= edge_straight;	// hits a straight edge
			}
			else
			{
				rayIt->wflags |= edge;	// hits an angled edge
			}
		}
		// weight being on-grid, the higher the better
		if (fabs(rayIt->end.x - qround(rayIt->end.x, 32)) == 0 &&
			fabs(rayIt->end.y - qround(rayIt->end.y, 32)) == 0)
			rayIt->wflags |= edge_grid32;
		else if (fabs(rayIt->end.x - qround(rayIt->end.x, 8)) == 0 &&
			fabs(rayIt->end.y - qround(rayIt->end.y, 8)) == 0)
			rayIt->wflags |= edge_grid8;
		else if (fabs(rayIt->end.x - qround(rayIt->end.x, 2)) == 0 &&
			fabs(rayIt->end.y - qround(rayIt->end.y, 2)) == 0)
			rayIt->wflags |= edge_grid2;
	}


	// pick best weighted ray
	auto bestRayIt = rays.end();
	float bestRayWeight = -1.0f;
	for (auto rayIt = rays.begin(); rayIt != rays.end(); ++rayIt)
	{
		float weight = SplitWeight(loop[rv], *rayIt);
		if (weight > bestRayWeight)
		{
			bestRayWeight = weight;
			bestRayIt = rayIt;
		}
	}

	return *bestRayIt;
}

/*
=============
CmdPolyBrushConcave::SplitWeight

weight a splitting ray according to preferability for convex decomposition

- short line > long line
- axial line > diagonal line > other-angled line
	(for friendliness to QBSP - fewer angled planes to split by means less chance of micro leaves/faces)
- within 'bayazit cone' > outside it (https://mpen.ca/406/bayazit)
	(inside the cone removes a reflex vertex from both result shapes, while outside removes it from only one)
- to another reflex vert > to a steiner split in a straight edge > to a non-reflex vert > to a steiner split in an angled edge
	(taking another reflex vert with us is more efficient)
	(splitting angled edges can create verts off the grid, so do that as a last resort)
- on high grid > on low grid > off grid
	(for QBSP friendliness and general clean construction)
=============
*/
float CmdPolyBrushConcave::SplitWeight(const vec3 rp, const loopRay_t &split)
{
	if (!split.wflags)
		return 0;

	float len, weight, lw;
	const float factors[14] = {
		1.0f,	// axial 
		0.7f,	// diagonal 
		0.65f,	// angular 

		1.0f,	// collinear 
		0.7f,	// cone_reflex 
		0.4f,	// cone_side
		1.3f,	// cone_core

		2.0f,	// vertex 
		3.0f,	// vertex_reflex 
		0.1f,	// edge 
		1.0f,	// edge_straight 
		3.0f,	// edge_grid2
		3.1f,	// edge_grid8
		3.2f,	// edge_grid32
	};

	// base weight is by length, preferring shorter splits
	len = VectorLength(rp - split.end);
	lw = (128.0f / len + 1); // but not so severely

	weight = 1;
	for (int i = 0; i < 14; i++)
	{
		if (split.wflags & (1 << i))
			weight *= factors[i];
	}

	// the fudge zone
	// add some manual extra bias for or against factors in tandem
	if (split.wflags & axial)
	{
		if (split.wflags & collinear)
			weight *= len / 32.0f;// 1.5f;
	}
	else
	{
		if (split.wflags & (edge | edge_straight))
			weight *= 0.5f;
		if (split.wflags & (vertex_reflex | vertex))
			weight *= min(1.0f, 32.0f / len);// 1.5f;
	}

	// little cuts should always be made straight across if possible (for trim strips)
	if (split.wflags & cone_core)
	{
		if (len < 32.0f) 
			weight *= 2.0f;
		if (len < 16.0f) 
			weight *= 2.0f;
		if (split.wflags & (vertex_reflex | vertex))
			weight *= 2.0f;
	}

	return weight * lw;
}

/*
=============
CmdPolyBrushConcave::CleaveLoop

divide a point loop into two across a given divide, so that the two shapes
defined by the new point loops share one new edge
=============
*/
void CmdPolyBrushConcave::CleaveLoop(const std::vector<vec3>&loop, const int rv, const loopRay_t split, std::vector<vec3>& loopLeft, std::vector<vec3>& loopRight)
{
	int i;
	int len = loop.size();

	loopLeft.reserve(len);	// too greedy?
	loopRight.reserve(len);
		
	for (i = rv; i % len != split.endv1; i++)
		loopLeft.push_back(loop[i % len]);
	loopLeft.push_back(loop[split.endv1]);
	if (split.endv1 != split.endv2)
	{
		loopLeft.push_back(split.end);
		loopRight.push_back(split.end);
	}
	for (i = split.endv2; i % len != rv; i++)
		loopRight.push_back(loop[i % len]);
	loopRight.push_back(loop[rv]);

	RemoveCollinear(loopLeft);
	RemoveCollinear(loopRight);
}

/*
=============
CmdPolyBrushConcave::Decompose

recursively divide a non-convex point loop into two loops by repeatedly splitting 
them in half from a reflex (ie non-convex) vertex to some other point, until only 
convex loops remain
=============
*/
void CmdPolyBrushConcave::Decompose()
{
	// start with empty list of convex (finished) loops, and list of concave (unfinished) loops containing the original
	convexLoops.clear();
	std::list< std::vector<vec3>* > concaveLoops;
	concaveLoops.push_back(new std::vector<vec3>(pointList));

	while (!concaveLoops.empty())
	{
		int rv;

		// peel a concave loop off the end of the list
		auto loopIt = concaveLoops.end();
		--loopIt;
		std::vector<vec3> &loop = **loopIt;

		// decide best start point for split
		rv = PickReflexVertex(loop);
		if (rv == -1)
		{
			// shape can already be a valid brush, move to convex list
			convexLoops.push_back(*loopIt);
			concaveLoops.erase(loopIt);
			continue;
		}

		// decide best end point for split
		loopRay_t split = BestSplit(loop, rv);

		// cleave pointloop into two, adding chosen split as edge (and new vert if necessary)
		std::vector<vec3> *loopLeft = new std::vector<vec3>();
		std::vector<vec3> *loopRight = new std::vector<vec3>();
		CleaveLoop(loop, rv, split, *loopLeft, *loopRight);

		// erase this pointloop from the unfinished list
		concaveLoops.erase(loopIt);

		// push the two new pointloops onto unfinished list
		concaveLoops.push_back(loopLeft);
		concaveLoops.push_back(loopRight);
	}
	assert(concaveLoops.empty());
}

//============================================================

void CmdPolyBrushConcave::Do_Impl()
{
	Decompose();

	CmdPolyBrush* cmdPB;
	for (auto loopIt = convexLoops.begin(); loopIt != convexLoops.end(); ++loopIt)
	{
		ToWorldSpace(**loopIt);
		cmdPB = new CmdPolyBrush();
		cmdPB->SetAxis(axis);
		cmdPB->SetTexDef(texdef);
		cmdPB->SetPoints(**loopIt);
		cmdPB->SetBounds(lowBound, highBound);
		cmdPBs.push_back(cmdPB);
	}
	for (auto pbIt = cmdPBs.begin(); pbIt != cmdPBs.end(); ++pbIt)
	{
		(*pbIt)->Do();
	}
}

void CmdPolyBrushConcave::Undo_Impl()
{
	for (auto pbIt = cmdPBs.begin(); pbIt != cmdPBs.end(); ++pbIt)
	{
		(*pbIt)->Undo();
	}
}

void CmdPolyBrushConcave::Redo_Impl()
{
	for (auto pbIt = cmdPBs.begin(); pbIt != cmdPBs.end(); ++pbIt)
	{
		(*pbIt)->Redo();
	}
}

void CmdPolyBrushConcave::Sel_Impl()
{
	for (auto pbIt = cmdPBs.begin(); pbIt != cmdPBs.end(); ++pbIt)
	{
		(*pbIt)->Select();
	}
}

int CmdPolyBrushConcave::BrushDelta()
{
	int i = 0;
	for (auto pbIt = cmdPBs.begin(); pbIt != cmdPBs.end(); ++pbIt)
		i += (*pbIt)->BrushDelta();
	return i;
}

int CmdPolyBrushConcave::EntityDelta()
{
	int i = 0;
	for (auto pbIt = cmdPBs.begin(); pbIt != cmdPBs.end(); ++pbIt)
		i += (*pbIt)->EntityDelta();
	return i;
}



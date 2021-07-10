//==============================
//	CmdGeoMod.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "CmdGeoMod.h"
#include "winding.h"
#include <algorithm>

/*
========================================================================

GEOMETRY MODIFICATION

the unifying action beneath plane shears, vertex moves, and edge moves.
takes a list of brushes, a list of point coordinates corresponding to
vertices on those brushes, and a delta to translate them by. upconverts
each brush face into a polygon, translates the requested verts, then
resolves each polygon back to one or more new planes. 

the move is rejected as invalid if no convex solution is found for at 
least one polygon (a polygon was dragged sliver/degenerate, a vertex was
absorbed or merged, or the brush is otherwise wonky).

========================================================================
*/

CmdGeoMod::CmdGeoMod() : Command("Geometry Mod"), modState(NOTHING_DONE)
{
}

CmdGeoMod::~CmdGeoMod() {}

//==============================

// Step 1: set brush list - necessary to have an advance list of all 
// points in all brushes with no duplicates

void CmdGeoMod::SetBrush(Brush *br)
{
	assert(modState == NOTHING_DONE);

	if (br->owner->IsPoint())
		return;
	brMods.push_back(br);

	state = LIVE;
}

void CmdGeoMod::SetBrushes(Brush *brList)
{
	assert(modState == NOTHING_DONE);

	// minimize needless copying
	int numBrushes = 0;
	for (Brush* b = brList->Next(); b != brList; b = b->Next())
	{
		if (b->owner->IsBrush())
			numBrushes++;
	}
	brMods.reserve(brMods.size() + numBrushes);

	for (Brush* b = brList->Next(); b != brList; b = b->Next())
		if (b->owner->IsBrush())
			brMods.push_back(b);

	if (brMods.size())
		state = LIVE;
}

void CmdGeoMod::SetBrushes(std::vector<Brush*>& brList)
{
	assert(modState == NOTHING_DONE);

	//brMods.insert(brMods.end(), brList.begin(), brList.end());
	for (auto brIt = brList.begin(); brIt != brList.end(); ++brIt)
	{
		if ((*brIt)->owner->IsBrush())
			brMods.push_back((*brIt));
	}

	if (brMods.size())
		state = LIVE;
}

//==============================

// Step 2: set manipulated points per brush - simply goes by coordinate location,
// not any kind of component numbering system (which would go out the window the
// instant brush geometry was modified anyway)

void CmdGeoMod::SetPoint(Brush* br, const vec3 pt)
{
	if (brMods.empty())
		CmdError("Specified brush not set");

	assert(modState == NOTHING_DONE || modState == BRUSHES_DONE);
	if (modState == NOTHING_DONE)
		FinalizeBrushList();

	Mesh* m = EnsureMesh(br);

	vec3 p;
	p = glm::round(pt);
	auto svert = vertMaster.begin();
	while (svert != vertMaster.end())
	{
		if (*svert == p)
			break;
		++svert;
	}
	if (svert == vertMaster.end())
		CmdError("Point not on brush geometry (%f %f %f)", pt.x, pt.y, pt.z);

	vec3 *vStat, *vDyn;
	vStat = &*svert;
	vDyn = &*(svert + 1);
	
	m->SetPointDynamic(vStat, vDyn);
}

void CmdGeoMod::SetPoints(Brush* br, const std::vector<vec3> &pts)
{
	if (brMods.empty())
		CmdError("Specified brush not set");

	assert(modState == NOTHING_DONE || modState == BRUSHES_DONE);
	if (modState == NOTHING_DONE)
		FinalizeBrushList();

	Mesh* m = EnsureMesh(br);

	for (auto ptIt = pts.begin(); ptIt != pts.end(); ++ptIt)
	{
		vec3 p;
		p = glm::round(*ptIt);
		auto svert = vertMaster.begin();
		while (svert != vertMaster.end())
		{
			if (*svert == p)
				break;
			++svert;
		}
		if (svert == vertMaster.end())
			CmdError("Point not on brush geometry (%f %f %f)", p.x, p.y, p.z);

		vec3 *vStat, *vDyn;
		vStat = &*svert;
		vDyn = &*(svert + 1);

		m->SetPointDynamic(vStat, vDyn);
	}
}

void CmdGeoMod::SetPoint(const vec3 pt, const std::vector<Brush*>& brList)
{
	if (brMods.empty())
		CmdError("Specified brush not set");

	assert(modState == NOTHING_DONE || modState == BRUSHES_DONE);
	if (modState == NOTHING_DONE)
		FinalizeBrushList();

	vec3 p;
	p = glm::round(pt);
	auto svert = vertMaster.begin();
	while (svert != vertMaster.end())
	{
		if (*svert == p)
			break;
		++svert;
	}
	if (svert == vertMaster.end())
		CmdError("Point not on brush geometry (%f %f %f)", p.x, p.y, p.z);

	vec3 *vStat, *vDyn;
	vStat = &*svert;
	vDyn = &*(svert + 1);

	for (auto brIt = brList.begin(); brIt != brList.end(); ++brIt)
	{
		Mesh* m = EnsureMesh(*brIt);
		m->SetPointDynamic(vStat, vDyn);
	}
}

CmdGeoMod::Mesh *CmdGeoMod::EnsureMesh(Brush *br)
{
	// we MUST be done changing the master vert list at this point, because we
	// need the memory addresses of their entries not to change any further and
	// a vector realloc would mess that up
	auto brMod = std::find(brMods.begin(), brMods.end(), br);
	if (brMod == brMods.end())
		CmdError("Specified brush not set");

	auto bmIt = brushMeshes.begin();
	for (bmIt; bmIt != brushMeshes.end(); ++bmIt)
	{
		if (bmIt->bOrig == br)
			return &*bmIt;
	}

	brushMeshes.emplace_back(*this, br);
	return &brushMeshes.back();
}

void CmdGeoMod::FinalizeBrushList()
{
	assert(modState == NOTHING_DONE);
	if (brMods.empty())
		CmdError("No brushes specified for geometry mod!");

	Face* f;
	std::vector<vec3> allverts;

	allverts.reserve(brMods.size() * 32);
	vertMaster.reserve(brMods.size() * 16);

	for (auto brIt = brMods.begin(); brIt != brMods.end(); ++brIt)
	{
		for (f = (*brIt)->faces; f; f = f->fnext)
		{
			auto w = f->GetWinding();
			if (!w)
				continue;
			for (int i = 0; i < w->numpoints; i++)
			{
				allverts.push_back(glm::round(w->points[i].point));
			}
		}
	}

	// every point appears in the list twice, once to represent a static vert (even)
	// and once to represent one being moved (odd) - marking a point on a brush as
	// being dragged means simply offsetting the pointer by one (from even to odd),
	// translating is simply adding to every other coordinate, and reverting all
	// translations is just copying odd values over evens.
	UniquePoints(allverts, vertMaster);
	int vs = vertMaster.size();
	vertMaster.resize(vs * 2);
	for (int i = vs - 1; i >= 0; i--)
	{
		vertMaster[i * 2 + 1] = vertMaster[i];
		vertMaster[i * 2] = vertMaster[i];
	}

	brushMeshes.reserve(brMods.size());	// make sure the meshes aren't copied around
	modState = BRUSHES_DONE;
}

//==============================

// Step 3: transform manipulated points - can be repeated any number of times
// before the command is completed, provided the brush list and set of manipulated
// points does not change

bool CmdGeoMod::Translate(const vec3 trans, bool relative)
{
	if (brMods.empty())
		return false;
	assert(modState == BRUSHES_DONE || modState == POLIES_DONE);
	if (modState == BRUSHES_DONE)
		FinalizePolygons();		// turn the brush faces into polygon soup

	// do the move
	vec3 tMod = (relative) ? trans : (trans - transTotal);
	ApplyTranslation(tMod);

	int i = 0;
	// try to turn the polygons back into brush planes that don't hate each other
	for (auto bmIt = brushMeshes.begin(); bmIt != brushMeshes.end(); ++bmIt)
	{
		//Log::Print("mesh %i:\n", i++);
		if (!bmIt->Resolve())
		{
			//Log::Print("Couldn't resolve\n");
			ApplyTranslation(-tMod);
			return false;
		}
	}

	transTotal += tMod;

	// all brush meshes successfully resolved into happy convex solids, apply the
	// new geometry to the brushes
	for (auto bmIt = brushMeshes.begin(); bmIt != brushMeshes.end(); ++bmIt)
	{
		bmIt->bOrig->ClearFaces();
		for (auto nfIt = bmIt->newFaces.begin(); nfIt != bmIt->newFaces.end(); ++nfIt)
		{
			(*nfIt)->owner = bmIt->bOrig;
			(*nfIt)->fnext = bmIt->bOrig->faces;
			bmIt->bOrig->faces = (*nfIt);
		}
		bmIt->bOrig->Build();
		bmIt->newFaces.clear();
	}

	return true;
}

void CmdGeoMod::ApplyTranslation(vec3 tr)
{
	for (auto vIt = vertMaster.begin(); vIt < vertMaster.end(); ++vIt)
	{
		++vIt;	// do every second one
		*vIt += tr;
	//	for (int i = 0; i < 3; i++)
	//	{
	//		if (tr[i] != 0)
	//			(*vIt)[i] = qround((*vIt)[i], g_qeglobals.d_nGridSize);
	//	}
	}
}

void CmdGeoMod::FinalizePolygons()
{
	assert(modState == BRUSHES_DONE);

	cmdBM.ModifyBrushes(brMods);	// backup the geometry into undo now

	for (auto brIt = brushMeshes.begin(); brIt != brushMeshes.end(); ++brIt)
	{
		// put static polygons last, so dynamic ones go first for merging
		std::sort(brIt->polies.begin(), brIt->polies.end(), Polygon::dyncmp);
		brIt->LinkNeighbors();
	}

	modState = POLIES_DONE;
}

//==============================

void CmdGeoMod::UniquePoints(std::vector<vec3> &bigList, std::vector<vec3>& uList)
{
	assert(!bigList.empty() && uList.empty());

	// strip out the duplicates: sort the list first so they're all adjacent
	std::sort(bigList.begin(), bigList.end(), VectorCompareLT);

	// then copy each one out only once
	vec3 v = bigList.back();	// guaranteed not to match the first one
	for (auto vIt = bigList.begin(); vIt != bigList.end(); ++vIt)
	{
		if (v != *vIt)
		{
			v = *vIt;
			uList.push_back(v);
		}
	}
}

void CmdGeoMod::UniquePoints(std::vector<vec3*> &bigList, std::vector<vec3*>& uList)
{
	assert(!bigList.empty() && uList.empty());

	// strip out the duplicates: sort the list first so they're all adjacent
	std::sort(bigList.begin(), bigList.end());

	// then copy each one out only once
	vec3* v = bigList.back();	// guaranteed not to match the first one
	for (auto vIt = bigList.begin(); vIt != bigList.end(); ++vIt)
	{
		if (v != *vIt)
		{
			v = *vIt;
			uList.push_back(v);
		}
	}
}

//==============================

bool CmdGeoMod::Mesh::Resolve()
{
	for (auto fIt = newFaces.begin(); fIt != newFaces.end(); ++fIt)
		delete *fIt;
	newFaces.clear();

	// reset markers
	for (auto pIt = polies.begin(); pIt != polies.end(); ++pIt)
		pIt->solved = false;

	int i = 1;
	// see if every individual polygon can be resolved
	for (auto pIt = polies.begin(); pIt != polies.end(); ++pIt)
	{
		// each polygon checks itself for validity and returns at least one face
		//Log::Print(" poly %i:\n", i++);
		if (!pIt->Resolve())
			return false;
	}

	for (auto pIt = polies.begin(); pIt != polies.end(); ++pIt)
	{
#ifdef _DEBUG
		// verify: sanity-check that all polygons' results are concave to each other
		for (auto fIt = pIt->newFaces.begin(); fIt != pIt->newFaces.end(); ++fIt)
		{
			for (auto pIt2 = pIt + 1; pIt2 != polies.end(); ++pIt2)
			{
				for (auto fIt2 = pIt2->newFaces.begin(); fIt2 != pIt2->newFaces.end(); ++fIt2)
				{
					assert(!(*fIt)->plane.EqualTo(&(*fIt2)->plane, false));
					/*{
						delete (*fIt);
						*fIt = nullptr;
						break;
					}*/
					assert((*fIt)->plane.ConvexTo(&(*fIt2)->plane));
						//return false;
				}
				if (!*fIt) break;
			}
			if (!*fIt) continue;
		}
#endif
		// hand the face pointers off from the polygons to the meshes
		for (auto fIt = pIt->newFaces.begin(); fIt != pIt->newFaces.end(); ++fIt)
		{
			if (!*fIt)
				continue;
			newFaces.push_back(*fIt);
		}
		pIt->newFaces.clear();
	}
	return true;
}

void CmdGeoMod::Mesh::LinkNeighbors()
{
	for (auto pIt = polies.begin(); pIt != polies.end(); ++pIt)
	{
		for (auto pIt2 = pIt + 1; pIt2 != polies.end(); ++pIt2)
		{
			if (pIt->Adjacent(*pIt2))
			{
				pIt->neighbors.push_back(&*pIt2);
				pIt2->neighbors.push_back(&*pIt);
			}
		}
	}
}

void CmdGeoMod::Mesh::SetPointDynamic(vec3 *vStat, vec3 *vDyn)
{
	// switch the pointer on all polygons
	for (auto pIt = polies.begin(); pIt != polies.end(); ++pIt)
	{
		auto pv = std::find(pIt->vertices.begin(), pIt->vertices.end(), vStat);
		if (pv != pIt->vertices.end())
		{
			*pv = vDyn;
			pIt->dynamic = true;
		}
	}
	// switch the pointer in our mesh list also
	auto pv = std::find(vertices.begin(), vertices.end(), vStat);
	if (pv != vertices.end())
		*pv = vDyn;
}

CmdGeoMod::Mesh::Mesh(CmdGeoMod &gm, Brush *br) : bOrig(br)
{
	for (Face* f = br->faces; f; f = f->fnext)
	{
		if (f->GetWinding())
			polies.emplace_back(gm, f, this);
	}

	std::vector<vec3*> pVerts;
	for (auto pIt = polies.begin(); pIt != polies.end(); ++pIt)
		pVerts.insert(pVerts.end(), pIt->vertices.begin(), pIt->vertices.end());
	gm.UniquePoints(pVerts, vertices);
}

void CmdGeoMod::Mesh::Clear()
{
	for (auto pIt = polies.begin(); pIt != polies.end(); ++pIt)
		pIt->Clear();
	for (auto fIt = newFaces.begin(); fIt != newFaces.end(); ++fIt)
		delete *fIt;
	newFaces.clear();
}

//==============================

bool CmdGeoMod::Polygon::Resolve()
{
	for (auto fIt = newFaces.begin(); fIt != newFaces.end(); ++fIt)
		delete *fIt;
	newFaces.clear();

	if (solved)		// some prior polygon had to absorb our points to resolve itself
		return true;
	/*
	for (int i = 0; i < vertices.size(); i++)
	{
		Log::Print("  %f %f %f\n", vertices[i]->x, vertices[i]->y, vertices[i]->z);
	}
	*/
	if (!dynamic)	// all our verts are static, so just copy the original
	{
		// this will add erroneous faces if a vert was dragged positive to this plane,
		// as it won't bother to test for those verts, so we ensure elsewhere that
		// these static faces are tested after all dynamic ones
		Face *f = new Face();
		f->texdef = tdOrig;
		f->plane = pOrig;
		newFaces.push_back(f);
		return true;
	}

	Plane p;
	std::vector<Plane> pBuf;
	std::vector<vec3*> polyVerts;
	int x, y, z;
	unsigned tries;
	int rCount, vCount;
	double dot;

	polyVerts = vertices;	// start with just our own verts

	/* ================
	the convex solution-finding algorithm:
	- try to make a plane from every combination of verts in the poly. they're in the same order
		they came from the face winding, so they're guaranteed clockwise.
	- when planes are found with other mesh points on the positive side, and those mesh points
		are on neighboring polies: remember the point, don't make the plane, and keep going.
	- if not all points in this polygon are 'spoken for' by at least one plane when we run out
		of points, try again with the remembered points included, to 'merge' the adjacent poly
		into this one (since a point could be dragged far outside the brush and require the
		convex wrap to expand over other polygons).
	================ */
#ifdef _DEBUG
	int i;
	for (i = 0; i < (int)mesh->polies.size(); i++) // we can't loop more times than there are polygons to absorb anyway
#else
	while (1)
#endif
	{
		std::vector<Polygon*> mergePolies;
		std::vector<vec3*> vertQueue = polyVerts;

		vCount = polyVerts.size();
		pBuf.clear();
		pBuf.reserve(vCount - 2);	// max possible new triangles

		/* ================
		the tri-finding algorithm:
		- take three verts in order from the list (0,1,2), try to be-plane them
		- for every fail, advance by 1 (1,2,3) and keep trying
		- when a success is found, remove the *second* vert of the three from the list, and keep going
		- rotate around the shrinking list, continually filling in tris and popping verts
		- when only two remain, we've filled in the entire shape with clockwise triangles
		================ */
		tries = 0;
		x = 0;
		//for (x = 0; x < vCount - 2; x++) for (y = x + 1; y < vCount - 1; y++) for (z = y + 1; z < vCount; z++)
		for (x = 0; tries < vertQueue.size() && vertQueue.size() > 2; x = (x + 1) % vertQueue.size())
		{
			y = (x + 1) % vertQueue.size();
			z = (x + 2) % vertQueue.size();
			if (!p.FromPoints(*vertQueue[x], *vertQueue[y], *vertQueue[z]))
				return false;

			rCount = 0;
			// compare all other points in this mesh to the triplane
			for (auto mvIt = mesh->vertices.begin(); mvIt != mesh->vertices.end(); ++mvIt)
			{
				if (*mvIt == vertQueue[x] || *mvIt == vertQueue[y] || *mvIt == vertQueue[z])
					continue;

				dot = DotProduct(dvec3(*(*mvIt)), p.normal) - p.dist;
				if (dot >= -ZERO_EPSILON)	// found a point on or outside the plane
				{
					if (std::find(polyVerts.begin(), polyVerts.end(), *mvIt) != polyVerts.end())
					//if (HasPoint)
					{
						if (dot < ZERO_EPSILON)
						{	// it's part of our polygon and coplanar, don't worry about it
							continue;
						}
						rCount++;
						break;
					}
					// TODO: expand a local list of neighbors to include mergePolies' neighbors
					// how to keep steep deformations from being too greedy? two polygons have to be
					// unsolvably concave to *each other* before being merged - how to handle?
					Polygon* np = FindNeighborForMerge(neighbors,*mvIt);
					if (!np)
					{
						rCount++;
						break;
					}
					mergePolies.push_back(np);
					rCount++;
				}
			}

			if (rCount)
			{
				tries++;
				continue;
			}

			pBuf.push_back(p);
			vertQueue.erase(vertQueue.begin() + y);
			tries = 0;
		}

		assert((int)pBuf.size() <= vCount - 2);	// too many planes == something went wrong

		if (pBuf.size() != vCount - 2)
		{	// some planes were skipped due to incompatibility with other polygons, resolve is incomplete
			if (mergePolies.empty())
				return false;	// no good way to fix it without destroying verts
			for (auto mpIt = mergePolies.begin(); mpIt != mergePolies.end(); ++mpIt)
			{
				// absorb the neighboring polies that we are unavoidably concave to and try again
				Merge(*mpIt, polyVerts);
			}
		}
		else break;	// polygon solved, we're done
	}

#ifdef _DEBUG
	if (i == (int)mesh->polies.size())
		return false;	// we looped too many times, something went wrong
#endif

	// make all our nice new totally valid faces
	Face *f;
	for (auto pIt = pBuf.begin(); pIt != pBuf.end(); ++pIt)
	{
		// cull coplanar duplicates now
		auto pIt2 = pIt + 1;
		for (pIt2; pIt2 != pBuf.end(); ++pIt2)
		{
			if (pIt2->EqualTo(&*pIt, false))
				break;
		}
		if (pIt2 != pBuf.end())
			continue;	// plane appears later in the list, skip it until then

		f = new Face();
		f->plane = *pIt;
		f->texdef = tdOrig;
		newFaces.push_back(f);
	}

	// if we haven't falsed out yet, we should have a set of planes that are not
	// concave or coincident to any other planes in the brushMesh
	return true;
}

bool CmdGeoMod::Polygon::Adjacent(const Polygon &other)
{
	int matches = 0;
	for (auto vIt = vertices.begin(); vIt != vertices.end(); ++vIt)
	{
		for (auto voIt = other.vertices.begin(); voIt != other.vertices.end(); ++voIt)
		{
			if (*vIt == *voIt)
				matches++;
		}
	}
	assert(matches < 3);
	return (matches == 2);
}

bool CmdGeoMod::Polygon::Merge(Polygon *other, std::vector<vec3*> &verts)
{
	assert(std::find(neighbors.begin(), neighbors.end(), other) != neighbors.end());

	if (other->solved)
		return false;

	std::vector<vec3*>::iterator match1, match2, othermatch1, othermatch2;
	// find the two endpoints of the shared edge between these two polygons
	match1 = verts.begin();
	while (true)
	{
		othermatch1 = std::find(other->vertices.begin(), other->vertices.end(), *match1);
		if (othermatch1 != other->vertices.end())
			break;
		++match1;
	}
	match2 = match1 + 1;
	while (true)
	{
		othermatch2 = std::find(other->vertices.begin(), other->vertices.end(), *match2);
		if (othermatch2 != other->vertices.end())
			break;
		++match2;
	}
	if (match2 != match1 + 1)
	{
		std::swap(match2, match1);
		std::swap(othermatch2, othermatch1);
	}
	std::rotate(other->vertices.begin(), othermatch2, other->vertices.end());
	verts.insert(match1 + 1, other->vertices.begin() + 2, other->vertices.end());

	other->solved = true;	// mark other as solved so it isn't reiterated
	return true;
}

bool CmdGeoMod::Polygon::HasPoint(const vec3 *v)
{
	return (std::find(vertices.begin(), vertices.end(), v) != vertices.end());
}

bool CmdGeoMod::Polygon::dyncmp(const Polygon &a, const Polygon &b)
{
	return (a.dynamic && !b.dynamic);
}

CmdGeoMod::Polygon *CmdGeoMod::Polygon::FindNeighborForMerge(const std::vector<Polygon*> &neighbs, const vec3 *v)
{
	Polygon* neighbor = nullptr;

	for (auto npIt = neighbs.begin(); npIt != neighbs.end(); ++npIt)
	{
		if ((*npIt)->HasPoint(v))
		{
			// there should geometrically only be one match - more than one
			// means we'll be subsuming this vert if we merge all the 
			// matching polygons
			if (neighbor)
				return nullptr;
			neighbor = *npIt;
		}
	}

	return neighbor;
}

CmdGeoMod::Polygon::Polygon(CmdGeoMod &gm, Face *f, Mesh *m) : 
	dynamic(false), solved(false), tdOrig(f->texdef), mesh(m)
{
	auto w = f->GetWinding();
	assert(w);
	vec3 pt;

	// make new plane from original winding, since we don't know where the hell
	// the original plane points might be and we need them within the face bounds
	// for proper convexity testing later
	pOrig.FromPoints(w->points[0].point,
					w->points[1].point,
					w->points[2].point);

	// build a list of pointers into our vertex arrays that correspond to the winding pts
	// so that all adjacent polygons have shared pointers to their shared vertices
	for (int i = 0; i < w->numpoints; i++)
	{
		pt = glm::round(w->points[i].point);
		auto vert = std::find(gm.vertMaster.begin(), gm.vertMaster.end(), pt);
		assert(vert != gm.vertMaster.end());
		vertices.push_back(&(*vert));
	}
}

void CmdGeoMod::Polygon::Clear()
{
	for (auto fIt = newFaces.begin(); fIt != newFaces.end(); ++fIt)
		delete *fIt;
	newFaces.clear();
}

//==============================

void CmdGeoMod::Do_Impl()
{
	std::vector<Brush*> brUnModified = brMods;
	for (auto bmIt = brushMeshes.begin(); bmIt != brushMeshes.end(); ++bmIt)
	{
		brUnModified.erase(std::find(brUnModified.begin(), brUnModified.end(), bmIt->bOrig));
		bmIt->Clear();
	}
	brushMeshes.clear();
	if (transTotal == vec3(0) || state == NOOP)
	{
		state = NOOP;
		cmdBM.RevertAll();
	}

	cmdBM.RevertBrushes(brUnModified);
	cmdBM.Do();
}

void CmdGeoMod::Undo_Impl()
{
	cmdBM.Undo();
}

void CmdGeoMod::Redo_Impl()
{
	cmdBM.Redo();
}

void CmdGeoMod::Sel_Impl()
{
	cmdBM.Select();
}


//==============================
//	CmdFaceMod.cpp
//==============================

#include "qe3.h"
#include "CmdFaceMod.h"
#include <algorithm>

CmdFaceMod::CmdFaceMod() : Command("Face Mod")
{
}

CmdFaceMod::~CmdFaceMod()
{
	if (state == LIVE)
		SwapAll();	// we aren't done, so restore the geometry that was backed up
}

// ModifyFace: clone off the data of these Faces and keep it
// lunaran TODO: should this eliminate duplicates, or rely on tools to do that selectively?
void CmdFaceMod::ModifyFace(Face *f)
{
	assert(state == LIVE || state == NOOP);
	faceCache.push_back(fBasis(f));
	state = LIVE;
}

void CmdFaceMod::ModifyFaces(std::vector<Face*> &fList)
{
	assert(state == LIVE || state == NOOP);
	for (auto fIt = fList.begin(); fIt != fList.end(); ++fIt)
		faceCache.push_back(fBasis(*fIt));
	state = LIVE;
}

void CmdFaceMod::ModifyFaces(Face* fList)
{
	assert(state == LIVE || state == NOOP);
	for (Face* f = fList; f; f = f->fnext)
		faceCache.push_back(fBasis(f));
	state = LIVE;
}

// RestoreFace: restore cloned data from these Faces
void CmdFaceMod::RestoreFace(Face *f)
{
	for (auto fbIt = faceCache.begin(); fbIt != faceCache.end(); ++fbIt)
	{
		if (fbIt->f == f)
		{
			fbIt->f->plane = fbIt->plane;	// recopy backup geometry
			fbIt->f->texdef = fbIt->texdef;
			break;
		}
	}
}

void CmdFaceMod::RestoreFaces(std::vector<Face*> &fList)
{
	for (auto fIt = fList.begin(); fIt != fList.end(); ++fIt)
		RestoreFace(*fIt);
}

void CmdFaceMod::RestoreFaces(Face* fList)
{
	for (Face* f = fList; f; f = f->fnext)
		RestoreFace(f);
}

void CmdFaceMod::RestoreAll()
{
	assert(state == LIVE || state == NOOP);

	for (auto fbIt = faceCache.begin(); fbIt != faceCache.end(); ++fbIt)
	{
		fbIt->f->plane = fbIt->plane;	// recopy backup geometry
		fbIt->f->texdef = fbIt->texdef;
	}
	state = NOOP;
}

// UnmodifyFace: throw away cloned data from these Faces and stop tracking - does NOT restore
void CmdFaceMod::UnmodifyFace(Face *f)
{
	assert(state == LIVE || state == NOOP);

	for (auto fbIt = faceCache.begin(); fbIt != faceCache.end(); ++fbIt)
	{
		if (fbIt->f == f)
		{
			// deletes entry as well as the cloned geometry in the basis
			faceCache.erase(fbIt);
			break;
		}
	}
	if (faceCache.size() == 0)
		state = NOOP;
}

void CmdFaceMod::UnmodifyFaces(std::vector<Face*> &fList)
{
	for (auto fIt = fList.begin(); fIt != fList.end(); ++fIt)
		UnmodifyFace(*fIt);
}

void CmdFaceMod::UnmodifyFaces(Face* fList)
{
	for (Face* f = fList; f; f = f->fnext)
		UnmodifyFace(f);
}

void CmdFaceMod::UnmodifyAll()
{
	assert(state == LIVE || state == NOOP);

	// deletes entry as well as the cloned geometry in the basis
	faceCache.clear();

	state = NOOP;
}

// RevertFace: restore cloned data from these Faces and stop tracking
void CmdFaceMod::RevertFace(Face *f)
{
	assert(state == LIVE || state == NOOP);

	for (auto fbIt = faceCache.begin(); fbIt != faceCache.end(); ++fbIt)
	{
		if (fbIt->f == f)
		{
			Swap(*fbIt);	// switch old data back into the scene first
			faceCache.erase(fbIt);	// deletes entry as well as the swapped garbage
			break;
		}
	}
	if (faceCache.size() == 0)
		state = NOOP;
}

void CmdFaceMod::RevertFaces(std::vector<Face*> &fList)
{
	for (auto fIt = fList.begin(); fIt != fList.end(); ++fIt)
		RevertFace(*fIt);
}

void CmdFaceMod::RevertFaces(Face* fList)
{
	for (Face* f = fList; f; f = f->fnext)
		RevertFace(f);
}

void CmdFaceMod::RevertAll()
{
	assert(state == LIVE || state == NOOP);

	SwapAll();	// switch old geometry back into the scene first
	faceCache.clear();

	state = NOOP;
}

void CmdFaceMod::RebuildAll()
{
	Brush *last;
	last = nullptr;
	for (auto fbIt = faceCache.begin(); fbIt != faceCache.end(); ++fbIt)
	{
		assert(fbIt->f);
		if (last != fbIt->f->owner)
		{
			if (last) last->FullBuild();
			last = fbIt->f->owner;
		}
	}
	last->FullBuild();
}

//==============================

void CmdFaceMod::Swap(fBasis &fb)
{
	assert(fb.f);
	fBasis temp;

	temp.plane = fb.f->plane;
	fb.f->plane = fb.plane;
	fb.plane = temp.plane;

	temp.texdef = fb.f->texdef;
	fb.f->texdef = fb.texdef;
	fb.texdef = temp.texdef;

	fb.f->owner->FullBuild();
}

void CmdFaceMod::SwapAll()
{
	fBasis temp;
	Brush *last;
	last = nullptr;
	for (auto fbIt = faceCache.begin(); fbIt != faceCache.end(); ++fbIt)
	{
		assert(fbIt->f);

		temp.plane = fbIt->f->plane;
		fbIt->f->plane = fbIt->plane;
		fbIt->plane = temp.plane;

		temp.texdef = fbIt->f->texdef;
		fbIt->f->texdef = fbIt->texdef;
		fbIt->texdef = temp.texdef;

		if (last != fbIt->f->owner)
		{
			if (last) last->FullBuild();
			last = fbIt->f->owner;
		}
	}
	last->FullBuild();
}

bool CmdFaceMod::fbasisCmp(const fBasis &a, const fBasis &b)
{
	return (a.f->owner < b.f->owner);
}

void CmdFaceMod::Do_Impl()
{
	// sort cache by brush for easy brush rebuilds on swap
	std::sort(faceCache.begin(), faceCache.end(), fbasisCmp);
}

void CmdFaceMod::Undo_Impl() { SwapAll(); }
void CmdFaceMod::Redo_Impl() { SwapAll(); }

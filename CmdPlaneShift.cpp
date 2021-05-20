//==============================
//	CmdPlaneShift.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "select.h"
#include "CmdPlaneShift.h"

CmdPlaneShift::CmdPlaneShift() : Command("Plane Shift") {}
CmdPlaneShift::~CmdPlaneShift() {}

void CmdPlaneShift::SetFaces(std::vector<Face*> &faceList)
{
	if (fShifted.size())
		CmdError("Faces already set on CmdPlaneShift");	// one time operation
	assert(planeShift == vec3(0));	// faces before translate

	// filter out duplicates now
	for (auto flIt = faceList.begin(); flIt != faceList.end(); ++flIt)
	{
		if (std::find(fShifted.begin(), fShifted.end(), *flIt) == fShifted.end())
			fShifted.push_back(*flIt);
	}

	// make a list of affected brushes now, for less derping around later to derive it
	for (auto fsIt = fShifted.begin(); fsIt != fShifted.end(); ++fsIt)
	{
		if (std::find(bChanged.begin(), bChanged.end(), (*fsIt)->owner) == bChanged.end())
			bChanged.push_back((*fsIt)->owner);
	}
	cmdFM.ModifyFaces(faceList);
}

// preventCrush: invalidate any plane translate that causes a brush to be dragged negative
// lunaran TODO: when preventCrush is allowed, track lost brushes with CmdAddRemove
void CmdPlaneShift::Translate(vec3 shift, bool preventCrush)
{
	assert(fShifted.size());	// faces before translate
	vec3 shiftMod;

	shiftMod = shift - planeShift;
	planeShift = shift;

	if (shiftMod == vec3(0))
		return;

	//Sys_Printf("Shift: %f %f %f Total %f %f %f\n", shiftMod.x, shiftMod.y, shiftMod.z, planeShift.x, planeShift.y, planeShift.z);

	for (auto fIt = fShifted.begin(); fIt != fShifted.end(); ++fIt)
	{
		(*fIt)->plane.Translate(shiftMod);
	}
	for (auto bcIt = bChanged.begin(); bcIt != bChanged.end(); ++bcIt)
	{
		if (!(*bcIt)->Build() && preventCrush)
		{
			// brush dragged backwards, must undo move
			Translate(planeShift - shiftMod, false);	// don't recurse if the state we're returning to is 
														// also invalid, although it Shouldn't Be(tm)
			break;
		}
	}

	if (planeShift == vec3(0))
		state = NOOP;
	else
		state = LIVE;
}

//==============================

void CmdPlaneShift::Do_Impl()
{
	if (planeShift == vec3(0))
	{
		state = NOOP;
		return;
	}

	// check that planeshift isn't perpendicular to at least one face, or else
	// all plane points were slid within their plane and no geometry was changed
	bool ok = false;
	for (auto fIt = fShifted.begin(); fIt != fShifted.end(); ++fIt)
	{
		if (DotProduct(dvec3(planeShift), (*fIt)->plane.normal))
		{
			ok = true;
			break;
		}
	}
	if (!ok)
	{
		state = NOOP;
		return;
	}

	cmdFM.Do();
}

void CmdPlaneShift::Undo_Impl() { cmdFM.Undo(); }
void CmdPlaneShift::Redo_Impl() { cmdFM.Redo(); }

void CmdPlaneShift::Sel_Impl()
{
	for (auto bcIt = bChanged.begin(); bcIt != bChanged.end(); ++bcIt)
		Selection::HandleBrush(*bcIt, false);
}



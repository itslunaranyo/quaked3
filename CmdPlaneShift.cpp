//==============================
//	CmdPlaneShift.cpp
//==============================

#include "qe3.h"

CmdPlaneShift::CmdPlaneShift()
{
	// state = LIVE;
}

CmdPlaneShift::~CmdPlaneShift()
{
	
}

void CmdPlaneShift::AddFaces(std::vector<Face*> &faceList)
{
	assert(!fShifted.size());	// one time operation
	fShifted = faceList;	// copies vector contents

	Brush* lastBrush;	// face list should come from the tool already sorted by brush
	lastBrush = nullptr;
	for (auto fIt = fShifted.begin(); fIt != fShifted.end(); ++fIt)
	{
		//fShifted.push_back(*fIt);
		if ((*fIt)->owner == lastBrush)
			continue;
		lastBrush = (*fIt)->owner;

		// will except if faces were not sorted right
		cmdBM.ModifyBrush(lastBrush);
	}
	state = LIVE;
}

void CmdPlaneShift::Translate(vec3 shift)
{
	vec3 shiftMod;

	if (planeShift != vec3(0))
		shiftMod = shift - planeShift;

	if (shiftMod == vec3(0))
		return;	// no change

	Brush* lastBrush;
	lastBrush = nullptr;
	for (auto fIt = fShifted.begin(); fIt != fShifted.end(); ++fIt)
	{
		(*fIt)->plane.Translate(shiftMod);
		if ((*fIt)->owner != lastBrush)
		{
			// rebuild brush if we've moved the last face
			if (lastBrush)
				lastBrush->Build();
			lastBrush = (*fIt)->owner;
		}
	}
}

//==============================

void CmdPlaneShift::Do_Impl()
{
	if (planeShift == vec3(0))
	{
		state = NOOP;
		return;
	}
	// TODO: check that planeshift isn't perpendicular to at least one face, or else
	// all plane points were slid within their plane and no geometry was changed

	cmdBM.Do();
	return;
}

void CmdPlaneShift::Undo_Impl()
{
	cmdBM.Undo();
}

void CmdPlaneShift::Redo_Impl()
{
	cmdBM.Redo();
}

void CmdPlaneShift::Select_Impl()
{
	
}



//==============================
//	CmdTextureMod.cpp
//==============================

#include "qe3.h"

CmdTextureMod::CmdTextureMod() : action(TM_NOTSET), Command("Modify Texture")
{
	// 
}

CmdTextureMod::~CmdTextureMod()
{
	
}

void CmdTextureMod::UseFace(Face *f)
{
	assert(action == TM_NOTSET);
	faceList.push_back(f);
	cmdFM.ModifyFace(f);
	state = LIVE;
}

void CmdTextureMod::UseFaces(std::vector<Face*> &fList)
{
	assert(action == TM_NOTSET);
	faceList.insert(faceList.end(), fList.begin(), fList.end());
	cmdFM.ModifyFaces(fList);
	state = LIVE;
}

void CmdTextureMod::UseBrushes(Brush *brList)
{
	assert(action == TM_NOTSET);

	std::vector<Face*> fq;

	for (Brush* b = brList->next; b != brList; b = b->next)
	{
		for (Face* f = b->faces; f; f = f->fnext)
			fq.push_back(f);
	}
	UseFaces(fq);
}

void CmdTextureMod::Shift(int x, int y)
{
	assert(!faceList.empty());
	assert(action == TM_NOTSET || action == TM_SHIFT);

	for (auto flIt = faceList.begin(); flIt != faceList.end(); ++flIt)
	{
		(*flIt)->texdef.shift[0] += x;
		(*flIt)->texdef.shift[1] += y;
	}

	cmdFM.RebuildAll();
	action = TM_SHIFT;
}

void CmdTextureMod::Scale(float x, float y)
{
	assert(!faceList.empty());
	assert(action == TM_NOTSET || action == TM_SCALE);

	for (auto flIt = faceList.begin(); flIt != faceList.end(); ++flIt)
	{
		(*flIt)->texdef.scale[0] += x;
		(*flIt)->texdef.scale[1] += y;
	}

	cmdFM.RebuildAll();
	action = TM_SCALE;
}

void CmdTextureMod::Rotate(float r)
{
	assert(!faceList.empty());
	assert(action == TM_NOTSET || action == TM_ROTATE);

	for (auto flIt = faceList.begin(); flIt != faceList.end(); ++flIt)
	{
		(*flIt)->texdef.rotate += r;

		while ((*flIt)->texdef.rotate > 180)
			(*flIt)->texdef.rotate -= 360;
		while ((*flIt)->texdef.rotate <= -180)
			(*flIt)->texdef.rotate += 360;
	}

	cmdFM.RebuildAll();
	action = TM_ROTATE;
}


//==============================

void CmdTextureMod::Do_Impl() { cmdFM.Do(); }
void CmdTextureMod::Undo_Impl() { cmdFM.Undo(); }
void CmdTextureMod::Redo_Impl() { cmdFM.Redo(); }
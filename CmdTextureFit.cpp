//==============================
//	CmdTextureFit.cpp
//==============================

#include "qe3.h"

CmdTextureFit::CmdTextureFit() : Command("Fit Texture")
{
	// state = LIVE;
}

CmdTextureFit::~CmdTextureFit()
{
}

void CmdTextureFit::UseFace(Face *f)
{
	faceList.push_back(f);
	cmdFM.ModifyFace(f);
	state = LIVE;
}

void CmdTextureFit::UseFaces(std::vector<Face*> &fList)
{
	faceList.insert(faceList.end(), fList.begin(), fList.end());
	cmdFM.ModifyFaces(fList);
	state = LIVE;
}

void CmdTextureFit::UseBrushes(Brush *brList)
{
	std::vector<Face*> fq;
	for (Brush* b = brList->next; b != brList; b = b->next)
	{
		for (Face* f = b->faces; f; f = f->fnext)
			fq.push_back(f);
	}
	UseFaces(fq);
}

void CmdTextureFit::Fit(float x, float y)
{
	assert(faceList.size());

	for (auto flIt = faceList.begin(); flIt != faceList.end(); ++flIt)
		(*flIt)->FitTexture(x, y);
}

//==============================

void CmdTextureFit::Do_Impl()
{
	cmdFM.Do();
	cmdFM.RebuildAll();
}
void CmdTextureFit::Undo_Impl() { cmdFM.Undo(); }
void CmdTextureFit::Redo_Impl() { cmdFM.Redo(); }
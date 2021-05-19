//==============================
//	CmdTextureApply.cpp
//==============================

#include "qe3.h"

CmdTextureApply::CmdTextureApply()
{
	// state = LIVE;
}

CmdTextureApply::~CmdTextureApply()
{
	
}

void CmdTextureApply::UseFace(Face *f)
{
	faceList.push_back(f);
	cmdFM.ModifyFace(f);
	state = LIVE;
}

void CmdTextureApply::UseFaces(std::vector<Face*> &fList)
{
	faceList.insert(faceList.end(), fList.begin(), fList.end());
	cmdFM.ModifyFaces(fList);
	state = LIVE;
}

void CmdTextureApply::UseBrush(Brush *br)
{
	std::vector<Face*> fq;
	for (Face* f = br->basis.faces; f; f = f->fnext)
		fq.push_back(f);
	UseFaces(fq);
}

void CmdTextureApply::UseBrushes(Brush *brList)
{
	std::vector<Face*> fq;
	for (Brush* b = brList->next; b != brList; b = b->next)
	{
		for (Face* f = b->basis.faces; f; f = f->fnext)
			fq.push_back(f);
	}
	UseFaces(fq);
}

void CmdTextureApply::Apply(TexDef &td, unsigned skipFlags)
{
	for (auto fIt = faceList.begin(); fIt != faceList.end(); ++fIt)
	{
		if (skipFlags)
		{
			if (!(skipFlags & SFI_NAME))
			{
				strncpy((*fIt)->texdef.name, td.name, MAX_TEXNAME);
				(*fIt)->texdef.tex = td.tex;
			}
			if (!(skipFlags & SFI_SHIFTX))
				(*fIt)->texdef.shift[0] = td.shift[0];
			if (!(skipFlags & SFI_SHIFTY))
				(*fIt)->texdef.shift[1] = td.shift[1];
			if (!(skipFlags & SFI_SCALEX))
				(*fIt)->texdef.scale[0] = td.scale[0];
			if (!(skipFlags & SFI_SCALEY))
				(*fIt)->texdef.scale[1] = td.scale[1];
			if (!(skipFlags & SFI_ROTATE))
				(*fIt)->texdef.rotate = td.rotate;
		}
		else
		{
			(*fIt)->texdef = td;
		}
	}
}

//==============================

void CmdTextureApply::Do_Impl()
{
	cmdFM.Do();
	cmdFM.RebuildAll();
}
void CmdTextureApply::Undo_Impl() { cmdFM.Undo(); }
void CmdTextureApply::Redo_Impl() { cmdFM.Redo(); }


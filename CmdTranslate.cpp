//==============================
//	CmdTranslate.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "CmdTranslate.h"
#include "select.h"
#include <glm/gtc/matrix_transform.hpp>


CmdTranslate::CmdTranslate() : textureLock(false), postDrag(false), Command("Translate"), mat(mat4(1))
{
}

CmdTranslate::~CmdTranslate() {}


void CmdTranslate::UseBrush(Brush *br)
{
	if (std::find(brMoved.begin(), brMoved.end(), br) != brMoved.end())
		return;

	if (br->owner->IsBrush())
		brMoved.push_back(br);
	else
		entMoved.push_back(br->owner);

	// translate in place if fewer than some small number of brushes is being
	// moved, and do the camera transform trick only for larger selections
	if (brMoved.size() + entMoved.size() > 8)
		postDrag = true;

	state = LIVE;
}

void CmdTranslate::UseBrushes(Brush *brList)
{
	for (Brush* b = brList->Next(); b != brList; b = b->Next())
		UseBrush(b);
}

void CmdTranslate::Translate(vec3 tr, const bool relative)
{
	assert(brMoved.size() || entMoved.size());

	vec3 tMod;
	bool doFM = false;

	if (relative)
	{
		tMod = tr;
		trans += tr;
		postDrag = false;
	}
	else
	{
		tMod = tr - trans;
		trans = tr;
	}

	mat = glm::translate(mat4(1), trans);

	if (relative || (!textureLock && !postDrag))
	{
		if (tMod == vec3(0))
			return;
		if (cmdFM.state == NOOP && brMoved.size())
			doFM = true;
		mat4 matmod = glm::translate(mat4(1), tMod);
		for (auto brIt = brMoved.begin(); brIt != brMoved.end(); ++brIt)
		{
			if (doFM)
				cmdFM.ModifyFaces((*brIt)->faces);
			(*brIt)->Transform(matmod, textureLock);
		}
		for (auto eIt = entMoved.begin(); eIt != entMoved.end(); ++eIt)
		{
			(*eIt)->Transform(matmod);
		}
	}
}

//==============================

void CmdTranslate::Do_Impl()
{
	if (trans == vec3(0))
	{
		state = NOOP;
		return;
	}
	if (postDrag)
	{
		for (auto brIt = brMoved.begin(); brIt != brMoved.end(); ++brIt)
		{
			cmdFM.ModifyFaces((*brIt)->faces);
			(*brIt)->Transform(mat, textureLock);
		}
		for (auto eIt = entMoved.begin(); eIt != entMoved.end(); ++eIt)
		{
			(*eIt)->Transform(mat);
		}
	}

	if (brMoved.size())
		cmdFM.Do();
}

void CmdTranslate::Undo_Impl()
{
	mat4 matinv = glm::inverse(mat);
	for (auto eIt = entMoved.begin(); eIt != entMoved.end(); ++eIt)
	{
		(*eIt)->Transform(matinv);
	}

	if (brMoved.size())
		cmdFM.Undo();
}

void CmdTranslate::Redo_Impl()
{
	for (auto eIt = entMoved.begin(); eIt != entMoved.end(); ++eIt)
	{
		(*eIt)->Transform(mat);
	}

	if (brMoved.size())
		cmdFM.Redo();
}

void CmdTranslate::Sel_Impl()
{
	for (auto brIt = brMoved.begin(); brIt != brMoved.end(); ++brIt)
		Selection::SelectBrush((*brIt));
	for (auto eIt = entMoved.begin(); eIt != entMoved.end(); ++eIt)
		Selection::SelectBrush((*eIt)->brushes.ENext());
}



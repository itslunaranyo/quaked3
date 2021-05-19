//==============================
//	CmdTranslate.cpp
//==============================

#include "qe3.h"
#include "CmdTranslate.h"
#include <glm/gtc/matrix_transform.hpp>


CmdTranslate::CmdTranslate() : textureLock(false), postDrag(false), Command("Translate")
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
	if (brMoved.size() > 8)
		postDrag = true;

	state = LIVE;
}

void CmdTranslate::UseBrushes(Brush *brList)
{
	for (Brush* b = brList->next; b != brList; b = b->next)
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
	}
	else
	{
		tMod = tr - trans;
		trans = tr;
	}

	mat = glm::translate(mat4(1), trans);

	if (!textureLock && !postDrag)
	{
		if (tMod == vec3(0))
			return;
		if (cmdFM.state != LIVE && brMoved.size())
			doFM = true;
		mat4 matmod = glm::translate(mat4(1), tMod);
		for (auto brIt = brMoved.begin(); brIt != brMoved.end(); ++brIt)
		{
			if (doFM)
				cmdFM.ModifyFaces((*brIt)->faces);
			(*brIt)->Transform(matmod, false);
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
		Selection::SelectBrush((*eIt)->brushes.next);
}



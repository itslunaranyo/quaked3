//==============================
//	CmdTranslate.cpp
//==============================

#include "qe3.h"
#include <glm/gtc/matrix_transform.hpp>


CmdTranslate::CmdTranslate() : textureLock(false), postDrag(false)
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

	if (!textureLock && !postDrag)
	{
		if (tMod == vec3(0))
			return;
		if (cmdFM.state != LIVE)
			doFM = true;
		for (auto brIt = brMoved.begin(); brIt != brMoved.end(); ++brIt)
		{
			if (doFM)
				cmdFM.ModifyFaces((*brIt)->basis.faces);
			(*brIt)->Move(tMod, false);
		}
	}

	//mat = glm::translate(glm::mat4(1), trans);
}

//==============================

void CmdTranslate::Do_Impl()
{
	if (postDrag)
	{
		for (auto brIt = brMoved.begin(); brIt != brMoved.end(); ++brIt)
		{
			cmdFM.ModifyFaces((*brIt)->basis.faces);
			(*brIt)->Move(trans, textureLock);
		}
	}
	for (auto eIt = entMoved.begin(); eIt != entMoved.end(); ++eIt)
	{
		(*eIt)->Move(trans);
	}

	//mat = glm::translate(glm::mat4(1), -trans);
	cmdFM.Do();
}

void CmdTranslate::Undo_Impl()
{
	for (auto eIt = entMoved.begin(); eIt != entMoved.end(); ++eIt)
	{
		(*eIt)->Move(-trans);
	}

	//mat = glm::translate(glm::mat4(1), trans);
	cmdFM.Undo();
}

void CmdTranslate::Redo_Impl()
{
	for (auto eIt = entMoved.begin(); eIt != entMoved.end(); ++eIt)
	{
		(*eIt)->Move(trans);
	}

	//mat = glm::translate(glm::mat4(1), -trans);
	cmdFM.Redo();
}

void CmdTranslate::Sel_Impl()
{
	for (auto brIt = brMoved.begin(); brIt != brMoved.end(); ++brIt)
		Selection::SelectBrush((*brIt));
	for (auto eIt = entMoved.begin(); eIt != entMoved.end(); ++eIt)
		Selection::SelectBrush((*eIt)->brushes.next);
}



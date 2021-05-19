//==============================
//	CmdTranslate.cpp
//==============================

#include "qe3.h"
#include <glm/gtc/matrix_transform.hpp>

CmdTranslate::CmdTranslate()
{
}

CmdTranslate::~CmdTranslate()
{
}

void CmdTranslate::UseBrush(Brush *br)
{
	if (std::find(brMoved.begin(), brMoved.end(), br) != brMoved.end())
		return;
	brMoved.push_back(br);

	if (br->owner->IsBrush())
		cmdBM.ModifyBrush(br);

	state = LIVE;
}

void CmdTranslate::UseBrushes(Brush *brList)
{
	for (Brush* b = brList->next; b != brList; b = brList->next)
		UseBrush(b);
}

void CmdTranslate::Translate(vec3 tr, const bool relative)
{
	if (relative)
		trans += tr;
	else
		trans = tr;

	mat = glm::translate(glm::mat4(1), trans);
}

//==============================

void CmdTranslate::Do_Impl()
{
	Brush* br;

	for (auto brIt = brMoved.begin(); brIt != brMoved.end(); ++brIt)
	{
		br = *brIt;
		if (br->owner->IsBrush())
			br->Transform(mat, textureLock);
		else
			br->owner->Transform(mat);
	}
	mat = glm::translate(glm::mat4(1), -trans);
	cmdBM.Do();
}

void CmdTranslate::Undo_Impl()
{
	Brush* br;

	for (auto brIt = brMoved.begin(); brIt != brMoved.end(); ++brIt)
	{
		br = *brIt;
		if (br->owner->IsPoint())
			br->owner->Transform(mat);
	}
	mat = glm::translate(glm::mat4(1), trans);
	cmdBM.Undo();
}

void CmdTranslate::Redo_Impl()
{
	Brush* br;

	for (auto brIt = brMoved.begin(); brIt != brMoved.end(); ++brIt)
	{
		br = *brIt;
		if (br->owner->IsPoint())
			br->owner->Transform(mat);
	}
	mat = glm::translate(glm::mat4(1), -trans);
	cmdBM.Redo();
}

void CmdTranslate::Select_Impl()
{
	
}



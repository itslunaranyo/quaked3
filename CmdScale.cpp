//==============================
//	CmdScale.cpp
//==============================

#include "qe3.h"
#include "CmdScale.h"
#include <glm/gtc/matrix_transform.hpp>

CmdScale::CmdScale() : textureLock(false), Command("Scale") {}
CmdScale::~CmdScale() {}

//==============================

void CmdScale::UseBrush(Brush *br)
{
	if (std::find(brScaled.begin(), brScaled.end(), br) != brScaled.end())
		return;

	if (br->owner->IsBrush())
		brScaled.push_back(br);
	else
		entMoved.push_back(br->owner);

	state = LIVE;
}

void CmdScale::UseBrushes(Brush *brList)
{
	for (Brush* b = brList->next; b != brList; b = b->next)
		UseBrush(b);
}

void CmdScale::Scale(const vec3 pivot, const vec3 scale)
{
	if (scale == vec3(1))
		return;
	if (glm::any(glm::equal(scale, vec3(0))))
		return;

	mat4 tr1, sc, tr2;

	tr1 = glm::translate(mat4(1), pivot);
	sc = glm::scale(mat4(1), scale);
	tr2 = glm::translate(mat4(1), -pivot);

	mat = (tr1 * sc) * tr2;
}

//==============================

void CmdScale::Do_Impl()
{
	if (mat == mat4(1))	// identity
	{
		state = NOOP;
		return;
	}

	for (auto brIt = brScaled.begin(); brIt != brScaled.end(); ++brIt)
	{
		cmdFM.ModifyFaces((*brIt)->faces);
		(*brIt)->Transform(mat, textureLock);
	}
	for (auto eIt = entMoved.begin(); eIt != entMoved.end(); ++eIt)
	{
		(*eIt)->Transform(mat);
	}

	if (brScaled.size())
		cmdFM.Do();
}

void CmdScale::Undo_Impl()
{
	mat4 matinv = glm::inverse(mat);
	for (auto eIt = entMoved.begin(); eIt != entMoved.end(); ++eIt)
	{
		(*eIt)->Transform(matinv);
	}

	if (brScaled.size())
		cmdFM.Undo();
}

void CmdScale::Redo_Impl()
{
	for (auto eIt = entMoved.begin(); eIt != entMoved.end(); ++eIt)
	{
		(*eIt)->Transform(mat);
	}

	if (brScaled.size())
		cmdFM.Redo();
}

void CmdScale::Sel_Impl()
{
	for (auto brIt = brScaled.begin(); brIt != brScaled.end(); ++brIt)
		Selection::SelectBrush((*brIt));
	for (auto eIt = entMoved.begin(); eIt != entMoved.end(); ++eIt)
		Selection::SelectBrush((*eIt)->brushes.next);
}
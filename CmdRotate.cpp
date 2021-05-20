//==============================
//	CmdRotate.cpp
//==============================

#include "qe3.h"
#include "CmdRotate.h"
#include <glm/gtc/matrix_transform.hpp>


CmdRotate::CmdRotate() : textureLock(false), Command("Rotate"), entAngMod(0) {}
CmdRotate::~CmdRotate() {}

void CmdRotate::UseBrush(Brush *br)
{
	if (std::find(brMoved.begin(), brMoved.end(), br) != brMoved.end())
		return;

	if (br->owner->IsBrush())
		brMoved.push_back(br);
	else
		entMoved.push_back(br->owner);

	state = LIVE;
}

void CmdRotate::UseBrushes(Brush *brList)
{
	for (Brush* b = brList->next; b != brList; b = b->next)
		UseBrush(b);
}

void CmdRotate::Rotate(const float degrees, const vec3 pivot, const vec3 axis)
{
	if (degrees == 0)
		return;

	float radians;
	mat4 tr1, rot, tr2;

	radians = degrees * Q_DEG2RAD;
	tr1 = glm::translate(mat4(1), pivot);
	rot = glm::rotate(mat4(1), radians, axis);
	tr2 = glm::translate(mat4(1), -pivot);

	mat = (tr1 * rot) * tr2;

	// remember angle difference on z for changing point entity angle
	entAngMod = 360.0f + DotProduct(axis, vec3(0, 0, 1)) * degrees;
}

//==============================

void CmdRotate::Do_Impl()
{
	if (mat == mat4(1))	// identity
	{
		state = NOOP;
		return;
	}

	for (auto brIt = brMoved.begin(); brIt != brMoved.end(); ++brIt)
	{
		cmdFM.ModifyFaces((*brIt)->faces);
		(*brIt)->Transform(mat, textureLock);
	}
	for (auto eIt = entMoved.begin(); eIt != entMoved.end(); ++eIt)
	{
		Entity *e = *eIt;
		e->Transform(mat);
		if (e->eclass->form & EntClass::ECF_ANGLE)
			e->SetKeyValue("angle", qround(e->GetKeyValueFloat("angle") + entAngMod, 1) % 360);
	}

	if (brMoved.size())
		cmdFM.Do();
}

void CmdRotate::Undo_Impl()
{
	mat4 matinv = glm::inverse(mat);
	for (auto eIt = entMoved.begin(); eIt != entMoved.end(); ++eIt)
	{
		Entity *e = *eIt;
		e->Transform(matinv);
		if (e->eclass->form & EntClass::ECF_ANGLE)
			e->SetKeyValue("angle", qround(e->GetKeyValueFloat("angle") - entAngMod, 1) % 360);
	}

	if (brMoved.size())
		cmdFM.Undo();
}

void CmdRotate::Redo_Impl()
{
	for (auto eIt = entMoved.begin(); eIt != entMoved.end(); ++eIt)
	{
		Entity *e = *eIt;
		e->Transform(mat);
		if (e->eclass->form & EntClass::ECF_ANGLE)
			e->SetKeyValue("angle", qround(e->GetKeyValueFloat("angle") + entAngMod, 1) % 360);
	}

	if (brMoved.size())
		cmdFM.Redo();
}

void CmdRotate::Sel_Impl()
{
	for (auto brIt = brMoved.begin(); brIt != brMoved.end(); ++brIt)
		Selection::SelectBrush((*brIt));
	for (auto eIt = entMoved.begin(); eIt != entMoved.end(); ++eIt)
		Selection::SelectBrush((*eIt)->brushes.onext);
}



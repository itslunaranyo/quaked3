//==============================
//	CmdCreatePointEntity.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "CmdCreatePointEntity.h"
#include "map.h"
#include "select.h"

// classname hack confirmation is done by the UI before creating this command,
// which assumes that whatever classname it is asked to create as a point entity
// is what was actually desired

CmdCreatePointEntity::CmdCreatePointEntity(const std::string& classname, const vec3 origin) : ent(nullptr), Command("Create Point Entity")
{
	selectOnDo = true;
	modifiesSelection = true;

	if (classname == "worldspawn")
		CmdError("Cannot create a new worldspawn entity.");

	EntClass* ec = EntClass::ForName(classname, false, false);
	CreatePointEntity(ec, origin);

	state = LIVE;
}

CmdCreatePointEntity::CmdCreatePointEntity(EntClass* eclass, const vec3 origin) : ent(nullptr), Command("Create Point Entity")
{
	selectOnDo = true;
	modifiesSelection = true;

	if (eclass == EntClass::Worldspawn())
		CmdError("Cannot create a new worldspawn entity.");

	CreatePointEntity(eclass, origin);

	state = LIVE;
}

CmdCreatePointEntity::~CmdCreatePointEntity()
{
	if (state == LIVE || state == UNDONE)
		delete ent;
}

void CmdCreatePointEntity::CreatePointEntity(EntClass* eclass, const vec3 origin)
{
	ent = new Entity();
	ent->SetKeyValueIVector("origin", origin);
	ent->ChangeClass(eclass);
}

//==============================

void CmdCreatePointEntity::Do_Impl()
{
	ent->AddToList(&g_map.entities);
	ent->brushes.ENext()->AddToList(g_map.brActive);
}

void CmdCreatePointEntity::Undo_Impl()
{
	ent->RemoveFromList();
	ent->brushes.ENext()->RemoveFromList();
}

void CmdCreatePointEntity::Redo_Impl()
{
	ent->AddToList(&g_map.entities);
	ent->brushes.ENext()->AddToList(g_map.brActive);
}

void CmdCreatePointEntity::Sel_Impl()
{
	if (state != DONE) return;

	ent->brushes.ENext()->RemoveFromList();
	ent->brushes.ENext()->AddToList(g_brSelectedBrushes);
}

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

CmdCreatePointEntity::CmdCreatePointEntity(const char *classname, const vec3 origin) : ent(nullptr), Command("Create Point Entity")
{
	selectOnDo = true;
	modifiesSelection = true;

	if (!strcmp(classname, "worldspawn"))
		CmdError("Cannot create a new worldspawn entity.");

	CreatePointEntity(classname, origin);

	state = LIVE;
}

CmdCreatePointEntity::~CmdCreatePointEntity()
{
	if (state == LIVE || state == UNDONE)
		delete ent;
}

void CmdCreatePointEntity::CreatePointEntity(const char *classname, const vec3 origin)
{
	EntClass* ec = EntClass::ForName(classname, false, false);
	ent = new Entity();

	ent->eclass = ec;
	ent->SetKeyValue("classname", classname);

	ent->SetKeyValueIVector("origin", origin);
	ent->origin = origin;

	ent->MakeBrush();
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

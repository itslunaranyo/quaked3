//==============================
//	CmdCreateBrushEntity.cpp
//==============================

#include "qe3.h"

CmdCreateBrushEntity::CmdCreateBrushEntity(const char* classname)
{
	if (!strcmp(classname, "worldspawn"))
		Error("Cannot create a new worldspawn entity.");

	EntClass *ec = EntClass::ForName(classname, true, false);
	ent = new Entity();
	ent->eclass = ec;
	ent->SetKeyValue("classname", classname);
	cmdRPB.Destination(ent);

	state = LIVE;
}

CmdCreateBrushEntity::~CmdCreateBrushEntity()
{
	if (state == UNDONE || state == LIVE)
	{
		delete ent;
	}
}

void CmdCreateBrushEntity::AddBrush(Brush *br) { cmdRPB.AddBrush(br); }
void CmdCreateBrushEntity::AddBrushes(Brush *brList) { cmdRPB.AddBrushes(brList); }

//==============================

void CmdCreateBrushEntity::Do_Impl()
{
	ent->AddToList(&g_map.entities);
	cmdRPB.Do();
}

void CmdCreateBrushEntity::Undo_Impl()
{ 
	cmdRPB.Undo();
	ent->RemoveFromList();
}

void CmdCreateBrushEntity::Redo_Impl()
{
	ent->AddToList(&g_map.entities);
	cmdRPB.Redo();
}
void CmdCreateBrushEntity::Select_Impl() { cmdRPB.Select(); }



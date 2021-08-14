//==============================
//	CmdCreateBrushEntity.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "CmdCreateBrushEntity.h"
#include "map.h"

CmdCreateBrushEntity::CmdCreateBrushEntity(const std::string& classname) : Command("Create Brush Entity")
{
	selectOnDo = true;
	selectOnUndo = true;
	modifiesSelection = true;
	EntClass *ec = EntClass::ForName(classname, true, false);
	Init(ec);
	state = LIVE;
}

CmdCreateBrushEntity::CmdCreateBrushEntity(EntClass* eclass) : Command("Create Brush Entity")
{
	selectOnDo = true;
	selectOnUndo = true;
	modifiesSelection = true;
	Init(eclass);
	state = LIVE;
}

void CmdCreateBrushEntity::Init(EntClass* eclass)
{
	if (eclass == EntClass::Worldspawn())
		CmdError("Cannot create a new worldspawn entity.");

	ent = new Entity();
	ent->ChangeClass(eclass);
	cmdRPB.Destination(ent);
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
void CmdCreateBrushEntity::Sel_Impl() { cmdRPB.Select(); }



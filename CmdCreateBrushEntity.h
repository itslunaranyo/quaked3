//==============================
//	CmdCreateBrushEntity.h
//==============================

#ifndef __COMMAND_CREATE_BRUSH_ENTITY_H__
#define __COMMAND_CREATE_BRUSH_ENTITY_H__

#include "Command.h"
#include "CmdReparentBrush.h"

class EntClass;

class CmdCreateBrushEntity : public Command
{
public:
	CmdCreateBrushEntity(const std::string& classname);
	CmdCreateBrushEntity(EntClass* eclass);
	~CmdCreateBrushEntity();

	void AddBrush(Brush* br);
	void AddBrushes(Brush* brList);

	int EntityDelta() { return cmdRPB.EntityDelta(); };
private:
	void Init(EntClass* eclass);
	CmdReparentBrush cmdRPB;
	Entity *ent;

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Sel_Impl();
};

#endif	// __CREATE_BRUSH_ENTITY_H__

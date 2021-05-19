//==============================
//	CmdCreateBrushEntity.h
//==============================

#ifndef __CREATE_BRUSH_ENTITY_H__
#define __CREATE_BRUSH_ENTITY_H__

#include "qe3.h"

class CmdCreateBrushEntity : public Command
{
public:
	CmdCreateBrushEntity(const char* classname);
	~CmdCreateBrushEntity();

	void AddBrush(Brush* br);
	void AddBrushes(Brush* brList);
private:
	CmdReparentBrush cmdRPB;
	Entity *ent;

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Select_Impl();
};

#endif	// __CREATE_BRUSH_ENTITY_H__

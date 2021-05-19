//==============================
//	CmdCreatePointEntity.h
//==============================

#ifndef __CREATE_POINT_ENTITY_H__
#define __CREATE_POINT_ENTITY_H__
#include "qe3.h"

class CmdCreatePointEntity : public Command
{
public:
	CmdCreatePointEntity(const char* classname, vec3_t origin);
	~CmdCreatePointEntity();

private:
	EntClass* ec;
	Entity *ent;
	void CreatePointEntity(const char* classname, vec3_t origin);

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Select_Impl();
};

#endif	// __CREATE_POINT_ENTITY_H__

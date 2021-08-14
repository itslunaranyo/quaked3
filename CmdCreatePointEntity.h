//==============================
//	CmdCreatePointEntity.h
//==============================

#ifndef __COMMAND_CREATE_POINT_ENTITY_H__
#define __COMMAND_CREATE_POINT_ENTITY_H__

#include "qe3.h"
#include "Command.h"

class EntClass;

class CmdCreatePointEntity : public Command
{
public:
	CmdCreatePointEntity(const std::string& classname, const vec3 origin);
	CmdCreatePointEntity(EntClass* eclass, const vec3 origin);
	~CmdCreatePointEntity();

	int EntityDelta() { return 1; };
private:
	EntClass* ec;
	Entity *ent;
	void CreatePointEntity(EntClass* eclass, const vec3 origin);

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Sel_Impl();
};

#endif	// __CREATE_POINT_ENTITY_H__

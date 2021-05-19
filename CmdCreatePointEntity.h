//==============================
//	CmdCreatePointEntity.h
//==============================

#ifndef __COMMAND_CREATE_POINT_ENTITY_H__
#define __COMMAND_CREATE_POINT_ENTITY_H__

class Entity;
class EntClass;

class CmdCreatePointEntity : public Command
{
public:
	CmdCreatePointEntity(const char* classname, const vec3 origin);
	~CmdCreatePointEntity();

	int EntityDelta() { return 1; };
private:
	EntClass* ec;
	Entity *ent;
	void CreatePointEntity(const char* classname, const vec3 origin);

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Sel_Impl();
};

#endif	// __CREATE_POINT_ENTITY_H__

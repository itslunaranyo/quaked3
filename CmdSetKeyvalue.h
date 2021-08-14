//==============================
//	CmdSetKeyvalue.h
//==============================

#ifndef __COMMAND_SET_KEYVALUE_H__
#define __COMMAND_SET_KEYVALUE_H__

#include "qe3.h"
#include "EPair.h"
#include "Command.h"

class CmdSetKeyvalue : public Command
{
public:
	CmdSetKeyvalue(const std::string& key, const std::string& value);
	~CmdSetKeyvalue();

	void AddEntity(Entity* e);

private:
	struct keyvalue_change_s {
		Entity* ent;
		std::string val;
		keyvalue_change_s(Entity *_e, const std::string&_v);
		keyvalue_change_s(Entity *_e, const char *_cv);
	};
	EPair newKV; // todo: this can just be two strings
	std::vector<keyvalue_change_s> kvchanges;

	void SetNew();
	void SetOld();

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Sel_Impl();
};

#endif	// __SET_KEYVALUE_H__

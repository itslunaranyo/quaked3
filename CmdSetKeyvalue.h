//==============================
//	CmdSetKeyvalue.h
//==============================

#ifndef __COMMAND_SET_KEYVALUE_H__
#define __COMMAND_SET_KEYVALUE_H__

class CmdSetKeyvalue : public Command
{
public:
	CmdSetKeyvalue(const char *key, const char *value);
	~CmdSetKeyvalue();

	void AddEntity(Entity* e);

private:
	struct keyvalue_change_s {
		Entity* ent;
		qeBuffer val;
		keyvalue_change_s(Entity *_e, const qeBuffer &_v);
		keyvalue_change_s(Entity *_e, const char *_cv);
	};
	EPair newKV;
	std::vector<keyvalue_change_s> kvchanges;

	void SetNew();
	void SetOld();

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Select_Impl();
};

#endif	// __SET_KEYVALUE_H__

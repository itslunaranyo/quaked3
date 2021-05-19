//==============================
//	CmdSetSpawnflag.h
//==============================

#ifndef __COMMAND_SET_SPAWNFLAG_H__
#define __COMMAND_SET_SPAWNFLAG_H__

#include "qe3.h"

class CmdSetSpawnflag : public Command
{
public:
	CmdSetSpawnflag(int flag, bool val);
	~CmdSetSpawnflag();

	void AddEntity(Entity* e);

private:
	struct spawnflag_change_s {
		Entity* ent;
		bool oldval;
		spawnflag_change_s(Entity *_e, bool ov);
	};
	int flagnum;
	bool newval;
	std::vector<spawnflag_change_s> sfchanges;

	void SetNew();
	void SetOld();

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Sel_Impl();

};

#endif	// __SET_SPAWNFLAG_H__

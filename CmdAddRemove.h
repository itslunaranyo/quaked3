//==============================
//	cmdaddremove.h
//==============================

#ifndef __COMMAND_ADDREMOVE_H__
#define __COMMAND_ADDREMOVE_H__

#include "qe3.h"

class CmdAddRemove : public Command
{
public:
	CmdAddRemove();
	~CmdAddRemove();

	void RemovedBrush(Brush* br);		// mark brush as sequestered from the scene
	void RemovedBrushes(Brush* brList);	// mark brushes as sequestered from the scene

	void AddedBrush(Brush* br);			// mark brush as contributed to the scene
	void AddedBrushes(Brush* brList);	// mark brushes as contributed to the scene

	void RemovedEntity(Entity* ent);	// mark entity as sequestered from the scene
	void AddedEntity(Entity* ent);		// mark entity as contributed to the scene

private:
	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Select_Impl();

	std::vector<Brush*> brAdded, brRemoved;
	std::vector<Entity*> entAdded, entRemoved;

	void Sequester(std::vector<Brush*> &brList);
	void Sequester(std::vector<Entity*> &entList);
	void Restore(std::vector<Brush*> &brList);
	void Restore(std::vector<Entity*> &entList);
	void Delete(std::vector<Brush*> &brList);
	void Delete(std::vector<Entity*> &entList);
};


#endif
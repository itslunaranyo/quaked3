//==============================
//	cmdaddremove.h
//==============================

#ifndef __COMMAND_ADDREMOVE_H__
#define __COMMAND_ADDREMOVE_H__

#include "Command.h"

/*
========================================================================

ADD/REMOVE

For any change which modifies the number of brushes or entities in the
scene, or otherwise edits brushes in a destructive way. handles addition
and removal simultaneously.
- shifts removed brushes and entities out of the scene lists but does not
  modify their memory locations
- inserts added brushes and entities into the scene lists
- simply does the reverse of the above on undo

========================================================================
*/

class CmdAddRemove : public Command
{
public:
	CmdAddRemove();
	~CmdAddRemove();

	void RemovedBrush(Brush* br);		// mark brush as sequestered from the scene
	void RemovedBrushes(Brush* brList);	// mark brushes as sequestered from the scene
	void RemovedBrushes(std::vector<Brush*> &brList);

	void AddedBrush(Brush* br);			// mark brush as contributed to the scene
	void AddedBrushes(std::vector<Brush*> &brList);
	//void AddedBrushes(Brush* brList);	// a brush list cannot be added because brushes in the limbo 
										// not-added state are assumed not to be in a list

	void RemovedEntity(Entity* ent);	// mark entity as sequestered from the scene

	void AddedEntity(Entity* ent);		// mark entity as contributed to the scene
	void AddedEntities(std::vector<Entity*> &entList);
	//void AddedEntities(Entity* entList);	// same as above

	int BrushDelta();
	int EntityDelta();

private:
	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Sel_Impl();

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
//==============================
//	CmdReparentBrush.h
//==============================

#ifndef __REPARENT_BRUSH_H__
#define __REPARENT_BRUSH_H__

#include "qe3.h"
#include <vector>

class CmdReparentBrush : public Command
{
public:
	CmdReparentBrush();
	~CmdReparentBrush();

	void Destination(Entity *dest);
	void AddBrush(Brush* br);
	void AddBrushes(Brush* brList);

private:
	struct brush_reparent_s
	{
		Brush* br;
		Entity *oldowner;
		brush_reparent_s(Brush* _br) : br(_br), oldowner(_br->owner) {};
	};
	Entity *newowner;
	std::vector<brush_reparent_s> reparents;
	std::vector<Entity*> entRemoved;

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Select_Impl();

};

#endif	// __REPARENT_BRUSH_H__

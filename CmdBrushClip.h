//==============================
//	CmdBrushClip.h
//==============================

#ifndef __COMMAND_BRUSH_CLIP_H__
#define __COMMAND_BRUSH_CLIP_H__

#include "Command.h"
#include "CmdAddRemove.h"

class CmdBrushClip : public Command
{
public:
	CmdBrushClip();
	~CmdBrushClip();

	enum clipside { CLIP_FRONT, CLIP_BACK, CLIP_BOTH };

	void StartBrushes(Brush* brList);
	void SetPoints(vec3 p1, vec3 p2, vec3 p3);
	void UnsetPoints();
	void SetSide(clipside s);

	std::vector<Brush*> brIn, brFront, brBack;

	int BrushDelta() { return cmdAR.BrushDelta(); };
	int EntityDelta() { return cmdAR.EntityDelta(); };
private:
	CmdAddRemove cmdAR;
	clipside side;
	vec3 cp1, cp2, cp3;
	bool pointsSet;

	void CreateSplitLists();
	void ClearSplitLists();

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Sel_Impl();

};

#endif	// __COMMAND_BRUSH_CLIP_H__

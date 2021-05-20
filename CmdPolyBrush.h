//==============================
//	CmdPolyBrush.h
//==============================

#ifndef __COMMAND_POLY_BRUSH_H__
#define __COMMAND_POLY_BRUSH_H__

#include "CmdAddRemove.h"

class CmdPolyBrush : public Command
{
public:
	CmdPolyBrush();
	~CmdPolyBrush();

	void SetPoints(std::vector<vec3> &points);
	void SetBounds(int low, int high);	// from workzone
	void SetTexDef(TexDef &tdef);		// from workzone
	void SetAxis(int ax);

	int BrushDelta() { return cmdAR.BrushDelta(); }
	int EntityDelta() { return cmdAR.EntityDelta(); }

private:
	Brush* work;
	CmdAddRemove cmdAR;
	std::vector<vec3> pointList;
	int axis, lowBound, highBound;
	TexDef texdef;

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Sel_Impl();

};

#endif	// __COMMAND_POLY_BRUSH_H__

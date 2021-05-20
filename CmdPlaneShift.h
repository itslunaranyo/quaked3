//==============================
//	CmdPlaneShift.h
//==============================

#ifndef __COMMAND_PLANE_SHIFT_H__
#define __COMMAND_PLANE_SHIFT_H__

#include "Command.h"
#include "CmdFaceMod.h"

class CmdPlaneShift : public Command
{
public:
	CmdPlaneShift();
	~CmdPlaneShift();

	void SetFaces(std::vector<Face*> &faceList);
	void Translate(vec3 shift, bool preventCrush = true);

private:
	vec3 planeShift;
	std::vector<Face*> fShifted;
	std::vector<Brush*> bChanged;

	CmdFaceMod cmdFM;

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Sel_Impl();

};

#endif	// __COMMAND_PLANE_SHIFT_H__

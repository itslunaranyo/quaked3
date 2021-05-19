//==============================
//	CmdPlaneShift.h
//==============================

#ifndef __COMMAND_PLANE_SHIFT_H__
#define __COMMAND_PLANE_SHIFT_H__

class CmdPlaneShift : public Command
{
public:
	CmdPlaneShift();
	~CmdPlaneShift();

	void AddFaces(std::vector<Face*> &faceList);
	void Translate(vec3 shift);

private:
	vec3 planeShift;
	std::vector<Face*> fShifted;

	CmdBrushMod cmdBM;

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Select_Impl();

};

#endif	// __COMMAND_PLANE_SHIFT_H__

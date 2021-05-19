//==============================
//	CmdTransform.h
//==============================

#ifndef __COMMAND_TRANSFORM_H__
#define __COMMAND_TRANSFORM_H__

class CmdTransform : public Command
{
public:
	CmdTransform();
	~CmdTransform();

	void Pivot(vec3 piv);

	void Translation(vec3 tr, bool relative = false);
	void Rotation(vec3 rot, bool relative = false);
	void Scale(vec3 sc, bool relative = false);

	void TextureLock(bool on) { textureLock = on; }

private:
	CmdBrushMod cmdBM;

	vec3 pivot;
	bool translated, rotated, scaled;
	glm::dmat4 translate, rotate, scale;
	bool textureLock;

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Select_Impl();

};

#endif	// __COMMAND_TRANSFORM_H__

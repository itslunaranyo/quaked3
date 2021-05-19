//==============================
//	CmdRotate.h
//==============================

#ifndef __COMMAND_ROTATE_H__
#define __COMMAND_ROTATE_H__

#include "Command.h"
#include "CmdFaceMod.h"

class CmdRotate : public Command
{
public:
	CmdRotate();
	~CmdRotate();

	void UseBrush(Brush *br);
	void UseBrushes(Brush *brList);

	void Rotate(const float degrees, const vec3 pivot, const vec3 axis);
	void TextureLock(bool lock) { textureLock = lock; }

private:
	bool textureLock;

	mat4 mat;
	std::vector<Brush*> brMoved;
	std::vector<Entity*> entMoved;

	CmdFaceMod cmdFM;

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Sel_Impl();

};

#endif	// __COMMAND_ROTATE_H__

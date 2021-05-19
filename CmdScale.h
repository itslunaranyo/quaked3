//==============================
//	CmdScale.h
//==============================

#ifndef __COMMAND_SCALE_H__
#define __COMMAND_SCALE_H__

#include "Command.h"
#include "CmdFaceMod.h"

class CmdScale : public Command
{
public:
	CmdScale();
	~CmdScale();

	void UseBrush(Brush *br);
	void UseBrushes(Brush *brList);

	void Scale(const vec3 pivot, const vec3 scale);
	void TextureLock(bool lock) { textureLock = lock; }

private:
	bool textureLock;

	mat4 mat;
	std::vector<Brush*> brScaled;
	std::vector<Entity*> entMoved;

	CmdFaceMod cmdFM;

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Sel_Impl();

};

#endif	// __COMMAND_SCALE_H__

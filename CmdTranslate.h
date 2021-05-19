//==============================
//	CmdTranslate.h
//==============================

#ifndef __COMMAND_TRANSLATE_H__
#define __COMMAND_TRANSLATE_H__

#include "Command.h"
#include "CmdFaceMod.h"

class CmdTranslate : public Command
{
public:
	CmdTranslate();
	~CmdTranslate();

	void UseBrush(Brush *br);
	void UseBrushes(Brush *brList);

	void Translate(vec3 tr, const bool relative = false);
	void TextureLock(bool lock) { textureLock = lock; if (lock) postDrag = true; }

	vec3 trans;
	bool postDrag;
private:
	bool textureLock;

	mat4 mat;
	std::vector<Brush*> brMoved;
	std::vector<Entity*> entMoved;

	CmdFaceMod cmdFM;	// for moving brushes (doesn't need to be a BrushMod bc facecounts don't change)

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Sel_Impl();

};

#endif	// __COMMAND_TRANSLATE_H__

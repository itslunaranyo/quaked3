//==============================
//	CmdTextureApply.h
//==============================

#ifndef __COMMAND_TEXTURE_APPLY_H__
#define __COMMAND_TEXTURE_APPLY_H__

#include "qe3.h"
#include "Command.h"
#include "CmdFaceMod.h"

class CmdTextureApply : public Command
{
public:
	CmdTextureApply();
	~CmdTextureApply();

	void UseFace(Face* f);
	void UseFaces(std::vector<Face*> &fList);
	void UseBrush(Brush *br);
	void UseBrushes(Brush *brList);

	void Apply(TexDef &td, unsigned skipFlags = 0);

private:
	CmdFaceMod cmdFM;
	std::vector<Face*> faceList;

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();

};

#endif	// __COMMAND_TEXTURE_APPLY_H__

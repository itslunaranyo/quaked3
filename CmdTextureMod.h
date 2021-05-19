//==============================
//	CmdTextureMod.h
//==============================

#ifndef __COMMAND_TEXTURE_MOD_H__
#define __COMMAND_TEXTURE_MOD_H__

class CmdTextureMod : public Command
{
public:
	CmdTextureMod();
	~CmdTextureMod();

	void UseFace(Face* f);
	void UseFaces(std::vector<Face*> &fList);
	void UseBrushes(Brush *brList);

	void Shift(int x, int y);
	void Scale(float x, float y);
	void Rotate(float r);

	texModType_t action;

private:
	CmdFaceMod cmdFM;
	std::vector<Face*> faceList;

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
};

#endif	// __COMMAND_TEXTURE_MOD_H__

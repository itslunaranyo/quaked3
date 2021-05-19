//==============================
//	CmdTextureFit.h
//==============================

#ifndef __COMMAND_TEXTURE_FIT_H__
#define __COMMAND_TEXTURE_FIT_H__

class CmdTextureFit : public Command
{
public:
	CmdTextureFit();
	~CmdTextureFit();

	void UseFace(Face* f);
	void UseFaces(std::vector<Face*> &fList);
	void UseBrushes(Brush *brList);

	void Fit(float x, float y);

private:
	CmdFaceMod cmdFM;
	std::vector<Face*> faceList;

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
};

#endif	// __COMMAND_TEXTURE_FIT_H__

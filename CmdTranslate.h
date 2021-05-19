//==============================
//	CmdTranslate.h
//==============================

#ifndef __COMMAND_TRANSLATE_H__
#define __COMMAND_TRANSLATE_H__

class CmdTranslate : public Command
{
public:
	CmdTranslate();
	~CmdTranslate();

	void UseBrush(Brush *br);
	void UseBrushes(Brush *brList);

	void Translate(vec3 tr, const bool relative = false);

private:
	vec3 trans;
	bool textureLock;
	glm::mat4 mat;
	std::vector<Brush*> brMoved;

	CmdBrushMod cmdBM;		// for moving brushes

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Select_Impl();

};

#endif	// __COMMAND_TRANSLATE_H__

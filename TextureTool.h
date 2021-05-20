//==============================
//	TextureTool.h
//==============================

#ifndef __TEXTURE_TOOL_H__
#define __TEXTURE_TOOL_H__

class CmdTextureMod;
class CmdCompound;

class TextureTool :	public Tool
{
public:
	TextureTool();
	~TextureTool();

	bool Input3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndView &vWnd);
	bool Input2D(UINT uMsg, WPARAM wParam, LPARAM lParam, XYZView &v, WndView &vWnd);

	void SelectionChanged();

	void FitTexture(float x, float y);
	void ShiftTexture(int x, int y);
	void ScaleTexture(float x, float y);
	void RotateTexture(float r);
private:
	CmdTextureMod *lastTexMod;
	CmdCompound *cmdCmp;
	clock_t lastTexModTime;
	Face *lastWrap, *lastWrap2;

	void GetTexModCommand(texModType_t tm);
};

#endif
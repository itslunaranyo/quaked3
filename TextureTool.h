//==============================
//	TextureTool.h
//==============================

#ifndef __TEXTURE_TOOL_H__
#define __TEXTURE_TOOL_H__

#include "Tool.h"
class CmdTextureMod;
class CmdCompound;

class TextureTool :	public Tool
{
public:
	TextureTool();
	~TextureTool();
	HWND hwndReplaceDlg;

	bool Input3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndCamera &vWnd);
	bool InputTex(UINT uMsg, WPARAM wParam, LPARAM lParam, TextureView &v, WndTexture &vWnd);
	bool Input(UINT uMsg, WPARAM wParam, LPARAM lParam);

	void SelectionChanged();

	bool InputReplaceDlg(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void FindTextureDialog();

	void FitTexture(float x, float y);
	void ShiftTexture(int x, int y);
	void ScaleTexture(float x, float y);
	void RotateTexture(float r);
private:
	CmdTextureMod *lastTexMod;
	CmdCompound *cmdCmp;
	clock_t lastTexModTime;
	Face *lastWrap, *lastWrap2;

	std::vector<std::string> findHistory, replaceHistory;
	enum sampleState_e {
		TTSS_NONE = 0,
		TTSS_FIND,
		TTSS_REPLACE
	} sampleState;

	void GetTexModCommand(texModType_t tm);
	void UpdateFindReplaceHistories();
	bool SelectWadDlg(std::string& outstr);
	void DoAddWad();
};

extern TextureTool* g_texTool;

#endif
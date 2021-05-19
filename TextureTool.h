//==============================
//	TextureTool.h
//==============================

#ifndef __TEXTURE_TOOL_H__
#define __TEXTURE_TOOL_H__

class TextureTool :	public Tool
{
public:
	TextureTool();
	~TextureTool();

	bool Input3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndView &vWnd);
	bool Input2D(UINT uMsg, WPARAM wParam, LPARAM lParam, XYZView &v, WndView &vWnd);
	bool Input(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void Draw();
	void SelectionChanged();
};

#endif
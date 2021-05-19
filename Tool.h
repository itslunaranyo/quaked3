//==============================
//	Tool.h
//==============================

#ifndef __TOOL_H__
#define __TOOL_H__

class CameraView;
class XYZView;
class ZView;
class TextureView;

class Tool
{
public:
	Tool(const char* n, bool isModal);
	virtual ~Tool();

	const char* name;

	bool hot;	// true during drags or other actions that must capture all input
	const bool modal;	// modal tools are mutually exclusive, and delete each other from the stack

	virtual void SelectionChanged() {}
	virtual bool Draw3D(CameraView &v) { return false; }
	virtual bool Draw2D(XYZView &v) { return false; }
	virtual bool Draw1D(ZView &v) { return false; }

	virtual bool Input3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndView &vWnd);
	virtual bool Input2D(UINT uMsg, WPARAM wParam, LPARAM lParam, XYZView &v, WndView &vWnd);
	virtual bool Input1D(UINT uMsg, WPARAM wParam, LPARAM lParam, ZView &v, WndView &vWnd);
	virtual bool InputTex(UINT uMsg, WPARAM wParam, LPARAM lParam, TextureView &v, WndView &vWnd);
	virtual bool Input(UINT uMsg, WPARAM wParam, LPARAM lParam);

	static int HandleInput3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndView &vWnd);
	static int HandleInput2D(UINT uMsg, WPARAM wParam, LPARAM lParam, XYZView &v, WndView &vWnd);
	static int HandleInput1D(UINT uMsg, WPARAM wParam, LPARAM lParam, ZView &v, WndView &vWnd);
	static int HandleInputTex(UINT uMsg, WPARAM wParam, LPARAM lParam, TextureView &v, WndView &vWnd);
	static int HandleInput(UINT uMsg, WPARAM wParam, LPARAM lParam);

	static Tool* HotTool();
	static bool FilterInput(UINT uMsg);

	bool ShiftDown() { return (GetKeyState(VK_SHIFT) < 0); }
	bool CtrlDown() { return (GetKeyState(VK_CONTROL) < 0); }
	bool AltDown() { return (GetKeyState(VK_MENU) < 0); }
};

#endif
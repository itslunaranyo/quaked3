//==============================
//	SelectTool.h
//==============================

#ifndef __SELECT_TOOL_H__
#define __SELECT_TOOL_H__

#include "Tool.h"

class SelectTool : public Tool
{
public:
	SelectTool();
	~SelectTool();

	bool Input3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndCamera &vWnd);
	bool Input2D(UINT uMsg, WPARAM wParam, LPARAM lParam, GridView &v, WndGrid &vWnd);
	bool Input1D(UINT uMsg, WPARAM wParam, LPARAM lParam, ZView &v, WndZChecker &vWnd);
	bool Input(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	int xDown, yDown;
	bool selecting;	// vs deselecting, obvs
	int selFlags;
	bool TrySelect(int buttons, const vec3 origin, const vec3 dir, int &flags);
	bool InputCommand(WPARAM w);
};

#endif
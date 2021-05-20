//==============================
//	NavTool.h
//==============================

#ifndef __NAV_TOOL_H__
#define __NAV_TOOL_H__

#include "Tool.h"
// Navigation Tool
// not a tool as such because it's purely for manipulating views, not constructing commands,
// but some navigation modes need to be modal in exactly the same way tools are and this Works

class NavTool : public Tool
{
public:
	NavTool();
	~NavTool();

	bool Input3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndCamera &vWnd);
	bool Input2D(UINT uMsg, WPARAM wParam, LPARAM lParam, GridView &v, WndGrid &vWnd);
	bool Input1D(UINT uMsg, WPARAM wParam, LPARAM lParam, ZView &v, WndZChecker &vWnd);
	bool InputTex(UINT uMsg, WPARAM wParam, LPARAM lParam, TextureView &v, WndTexture &vWnd);
	bool Input(UINT uMsg, WPARAM wParam, LPARAM lParam);
private:
	int MouseButtonForMessage(UINT msg);
	WPARAM FilterWParam(const WPARAM wpIn, const int mbtn);
	int hotMButton;
};

#endif
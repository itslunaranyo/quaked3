//==============================
//	TextureTool.cpp
//==============================

#include "qe3.h"
#include "TextureTool.h"

TextureTool::TextureTool() : Tool("Texturing", false)	// always on (not modal)
{
}


TextureTool::~TextureTool()
{
}

bool TextureTool::Input3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndView & vWnd)
{
	return 0;
}

bool TextureTool::Input2D(UINT uMsg, WPARAM wParam, LPARAM lParam, XYZView &v, WndView & vWnd)
{
	return 0;
}

bool TextureTool::Input(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return 0;
}

bool TextureTool::Draw3D(CameraView &v)
{
	return false;
}

bool TextureTool::Draw2D(XYZView &v)
{
	return false;
}


void TextureTool::SelectionChanged()
{
}

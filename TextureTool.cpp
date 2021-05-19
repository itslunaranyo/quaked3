//==============================
//	TextureTool.cpp
//==============================

#include "qe3.h"
#include "TextureTool.h"

TextureTool::TextureTool() : Tool("fake tool not here", false)	// always on (not modal)
{
}


TextureTool::~TextureTool()
{
}

bool TextureTool::Input3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView & v, WndView & vWnd)
{
	return 0;
}

bool TextureTool::Input2D(UINT uMsg, WPARAM wParam, LPARAM lParam, XYZView & v, WndView & vWnd)
{
	return 0;
}

bool TextureTool::Input(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return 0;
}

void TextureTool::Draw()
{
}

void TextureTool::SelectionChanged()
{
}

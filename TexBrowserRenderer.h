#ifndef __R_TEXBROWSER_H__
#define __R_TEXBROWSER_H__
//==============================
//	TexBrowserRenderer.h
//==============================

#include "Renderer.h"

class TextureView;
class DisplayView;

class TexBrowserRenderer : public Renderer
{
public:
	TexBrowserRenderer(TextureView &view);
	~TexBrowserRenderer();

	void Draw();
	DisplayView* GetDisplayView() { return (DisplayView*)&tv; }

private:
	TextureView &tv;
};

#endif
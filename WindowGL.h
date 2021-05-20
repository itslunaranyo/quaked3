//==============================
//	WindowGL.h
//==============================

#ifndef __WND_GL_H__
#define __WND_GL_H__

#include "Window.h"

class Renderer;

class WindowGL : public Window
{
public:
	WindowGL();
	virtual ~WindowGL();

	static int		nFontList;
	static HDC		wBaseHDC;
	static HGLRC	wBaseHGLRC;
	HDC		wHDC;
	HGLRC	wHGLRC;

	Renderer *r;

protected:
	void MakeFont();
	int SetupPixelFormat(HDC hDC, bool zbuffer);
	virtual int OnCreate();
	virtual int OnDestroy();
	int OnPaint();
	virtual void Render();
};

#endif
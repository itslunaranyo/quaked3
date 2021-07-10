//==============================
//	WindowGL.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "WindowGL.h"
#include "Renderer.h"
#include "View.h"

int		WindowGL::nFontList = -1;
HDC		WindowGL::wBaseHDC = 0;
HGLRC	WindowGL::wBaseHGLRC = 0;

WindowGL::WindowGL() : wHDC(nullptr), wHGLRC(nullptr), r(nullptr)
{
}

WindowGL::~WindowGL()
{
}


/*
==================
WindowGL::MakeFont

TODO: this is a horrible way to draw text now
==================
*/
void WindowGL::MakeFont()
{
	HFONT	hfont;

	// create GL font
	hfont = CreateFont(
		10,	// logical height of font (default was 10)
		7,	// logical average character width (default was 7)
		0,	// angle of escapement 
		0,	// base-line orientation angle 
		1,	// font weight 
		0,	// italic attribute flag 
		0,	// underline attribute flag 
		0,	// strikeout attribute flag 
		1,	// character set identifier 
		0,	// output precision 
		0,	// clipping precision 
		0,	// output quality 
		0,	// pitch and family 
		0	// pointer to typeface name string (default was 0)
	);

	if (!hfont)
		Error("Could not create font.");

	SelectObject(WindowGL::wBaseHDC, hfont);

	if ((nFontList = glGenLists(256)) == 0)
		Error("Could not create font dlists.");

	// create the bitmap display lists
	// we're making images of glyphs 0 thru 255
	if (!wglUseFontBitmaps(WindowGL::wBaseHDC, 1, 255, nFontList))
		Error("WCam_WndProc: wglUseFontBitmaps failed.");

	// indicate start of glyph display lists
	glListBase(nFontList);
}

/*
==================
WindowGL::SetupPixelFormat
==================
*/
int WindowGL::SetupPixelFormat(HDC hDC, bool zbuffer)
{
	static PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),	// size of this pfd
		1,								// version number
		PFD_DRAW_TO_WINDOW |			// support window
		PFD_SUPPORT_OPENGL |			// support OpenGL
		PFD_DOUBLEBUFFER,				// double buffered
		PFD_TYPE_RGBA,					// RGBA type
		24,								// 24-bit color depth
		0, 0, 0, 0, 0, 0,				// color bits ignored
		0,								// no alpha buffer
		0,								// shift bit ignored
		0,								// no accumulation buffer
		0, 0, 0, 0,						// accum bits ignored
		32,							    // depth bits
		0,								// no stencil buffer
		0,								// no auxiliary buffer
		PFD_MAIN_PLANE,					// main layer
		0,								// reserved
		0, 0, 0							// layer masks ignored
	};
	int pixelformat = 0;

	zbuffer = true;	// ?
	if (!zbuffer)
		pfd.cDepthBits = 0;

	if ((pixelformat = ChoosePixelFormat(hDC, &pfd)) == 0)
	{
		Error(_S("ChoosePixelFormat: Failed (error %d)") << (int)GetLastError());
	}

	if (!SetPixelFormat(hDC, pixelformat, &pfd))
		Error("SetPixelFormat: Failed");

	return pixelformat;
}

/*
==================
WindowGL::OnCreate
==================
*/
int WindowGL::OnCreate()
{
	wHDC = GetDC(wHwnd);

	SetupPixelFormat(wHDC, false);

	if ((wHGLRC = wglCreateContext(wHDC)) == 0)
		Error("wglCreateContext failed.");

	if (!wglMakeCurrent(wHDC, wHGLRC))
		Error("wglMakeCurrent failed.");

	// treat first created drawing context as base for sharing font display list
	if (!WindowGL::wBaseHDC || !WindowGL::wBaseHGLRC)
	{
		WindowGL::wBaseHDC = wHDC;
		WindowGL::wBaseHGLRC = wHGLRC;

		MakeFont();
		Textures::SetTextureMode(g_cfgUI.TextureMode);

		// report OpenGL information
		Log::Print(_S("GL_VENDOR: %s\n") << (const char*)glGetString(GL_VENDOR));
		Log::Print(_S("GL_RENDERER: %s\n") << (const char*)glGetString(GL_RENDERER));
		Log::Print(_S("GL_VERSION: %s\n") << (const char*)glGetString(GL_VERSION));
#ifdef _DEBUG
		Log::Print(_S("GL_EXTENSIONS: %s\n") << (const char*)glGetString(GL_EXTENSIONS));
#endif

		//glPolygonStipple((GLubyte *)s_stipple);
		glLineStipple(3, 0xaaaa);
	}
	else if (!wglShareLists(WindowGL::wBaseHGLRC, wHGLRC))
		Error("wglShareLists failed.");

	return 0;
}

/*
==================
WindowGL::OnDestroy
==================
*/
int WindowGL::OnDestroy()
{
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(wHGLRC);
	ReleaseDC(wHwnd, wHDC);
	return 0;
}

/*
==================
WindowGL::OnPaint
==================
*/
int WindowGL::OnPaint()
{
	if (!wHDC || !wHGLRC)
		Error("WindowGL not initialized properly\n");
	if (!wglMakeCurrent(wHDC, wHGLRC))
		Error("wglMakeCurrent: Failed.");
	PAINTSTRUCT	ps;
	if (!BeginPaint(wHwnd, &ps))
		return 1;

	QE_CheckOpenGLForErrors();
	Render();
	QE_CheckOpenGLForErrors();

	EndPaint(wHwnd, &ps);
	SwapBuffers(wHDC);
	return 0;
}

/*
==================
WindowGL::Render
==================
*/
void WindowGL::Render()
{
	r->Draw();
}

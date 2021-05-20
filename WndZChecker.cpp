//==============================
//	WndZChecker.c
//==============================

#include "pre.h"
#include "qe3.h"
#include "WndZChecker.h"
#include "CameraView.h"
#include "ZView.h"
#include "ZViewRenderer.h"
#include "Tool.h"

HWND g_hwndZ;

WndZChecker::WndZChecker()
{
	name = "ZWindow";
	vbits = W_Z;
}


WndZChecker::~WndZChecker()
{
}

void WndZChecker::Initialize()
{
	zv = &g_vZ;
	v = zv;

	Create();
	SetTitle("Z Checker");
	g_hwndZ = wHwnd;

	zr = new ZViewRenderer(*zv);
	r = zr;

}

int WndZChecker::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	if (Tool::HandleInput1D(uMsg, wParam, lParam, *zv, *this))
	{
		if (uMsg > WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)
			Focus();
		return 1;
	}
	return 1;
}


int WndZChecker::OnResized()
{
	zv->Resize(clientRect.right, clientRect.bottom);
	return 0;
}

void WndZChecker::Render()
{
	zr->Draw();
}


/*
==============
WndZChecker::MouseDown
==============
*/
void WndZChecker::MouseDown(const int x, const int y, const int btndown, const int buttons)
{
	Sys_GetCursorPos(&cursorX, &cursorY);

	// control mbutton = move camera
	if (buttons == (MK_CONTROL | MK_MBUTTON))
	{
		vec3 corg = g_vCamera.GetOrigin();
		corg.z = zv->GetOrigin().z;
		g_vCamera.SetOrigin(corg);
		WndMain_UpdateWindows(W_SCENE);
	}
}

/*
==============
WndZChecker::MouseUp
==============
*/
void WndZChecker::MouseUp(const int x, const int y, const int btnup, const int buttons)
{
	//Drag_MouseUp();
}

/*
==============
WndZChecker::MouseMoved
==============
*/
void WndZChecker::MouseMoved(const int x, const int y, const int buttons)
{
	char	zstring[256];
	int		cx, cy;
	vec3	o;

	if (!buttons)
	{
		zv->ScreenToWorldSnapped(x, y, o);
		sprintf(zstring, "z Coordinate: (%d)", (int)o.z);
		WndMain_Status(zstring, 0);
		return;
	}

	// rbutton = drag z origin
	if (buttons == MK_RBUTTON)
	{
		SetCursor(NULL); // sikk - Remove Cursor
		Sys_GetCursorPos(&cx, &cy);
		if (cy != cursorY)
		{
			zv->Scroll(cy - cursorY);
			Sys_SetCursorPos(cursorX, cursorY);
			WndMain_UpdateWindows(W_Z);

			sprintf(zstring, "z Origin: (%d)", (int)zv->GetOrigin().z);
			WndMain_Status(zstring, 0);
		}
		return;
	}

	// control mbutton = move camera
	if (buttons == (MK_CONTROL | MK_MBUTTON))
	{
		Sys_GetCursorPos(&cx, &cy);
		if (cy != cursorY)
		{
			zv->Scroll(cy - cursorY);
			Sys_SetCursorPos(cursorX, cursorY);
			WndMain_UpdateWindows(W_Z);

			sprintf(zstring, "z Origin: (%d)", (int)zv->GetOrigin().z);
			WndMain_Status(zstring, 0);
		}
		return;

		g_vCamera.Move(0, 0, (y - cursorY) / zv->GetScale());
		WndMain_UpdateWindows(W_SCENE);
	}

	// control rbutton = zoom z view
	if (buttons == (MK_CONTROL | MK_RBUTTON))
	{
		SetCursor(NULL); // sikk - Remove Cursor
		Sys_GetCursorPos(&cx, &cy);

		if (cy != cursorY)
		{
			zv->Scale(powf((cy > cursorY) ? 1.01f : 0.99f, fabs(cy - cursorY)));
			Sys_SetCursorPos(cursorX, cursorY);
			WndMain_UpdateWindows(W_Z);
		}
		return;
	}
}
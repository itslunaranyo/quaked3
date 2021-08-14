//==============================
//	WndCamera.c
//==============================

#include "pre.h"
#include "qe3.h"
#include "CameraView.h"
#include "CameraRenderer.h"
#include "WndCamera.h"
#include "Tool.h"
#include "select.h"

HWND g_hwndCamera;

WndCamera::WndCamera() : cv(nullptr), cr(nullptr)
{
	name = "CameraWindow";
	vbits = W_CAMERA;
}

WndCamera::~WndCamera()
{
	delete r;
}

void WndCamera::Initialize()
{
	cv = &g_vCamera;
	v = cv;
	Create();
	cv->Init();

	SetTitle("Camera View");
	g_hwndCamera = wHwnd;

	cr = new CameraRenderer(*cv);
	r = cr;
}

int WndCamera::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (Tool::HandleInput3D(uMsg, wParam, lParam, *cv, *this))
	{
		if (uMsg > WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)
			Focus();
		return 1;
	}

	return 1;
}

int WndCamera::OnResized()
{
	cv->Resize(clientRect.right, clientRect.bottom);
	return 0;
}

void WndCamera::Render()
{
	cr->Draw();
}

/*
================
WndCamera::RealtimeControl
================
*/
void WndCamera::RealtimeControl(float dtime)
{
	int		xl, xh;
	int		yl, yh;
	float	xf, yf;
	float	width, height;

	if (!(nCamButtonState & MK_RBUTTON))
		return;

	width = clientRect.right;
	height = clientRect.bottom;

	if (g_cfgEditor.CameraMoveStyle == CAMERA_CLASSIC)
	{
		if (nCamButtonState != MK_RBUTTON)
			return;

		xf = (float)(buttonX - width / 2) / (width / 2);
		yf = (float)(buttonY - height / 2) / (height / 2);

		xl = width / 3;
		xh = xl * 2;
		yl = height / 3;
		yh = yl * 2;

		xf *= 1.0 - fabs(yf);
		if (xf < 0)
		{
			xf += 0.1f;
			if (xf > 0)
				xf = 0;
		}
		else
		{
			xf -= 0.1f;
			if (xf < 0)
				xf = 0;
		}

		if (xf == 0 && yf == 0)
			return;
		cv->Step(yf * dtime * (float)g_cfgEditor.CameraSpeed, 0, 0);
		cv->Turn(xf * -dtime * ((int)g_cfgEditor.CameraSpeed / 2));
	}
	else if (g_cfgEditor.CameraMoveStyle == CAMERA_WASD)
	{
		float fwd, rt;
		fwd = 0;
		rt = 0;
		if (nCamButtonState & MK_SHIFT)	// orbiting
		{
			float yaw = 0;
			if (GetKeyState('D') < 0 || GetKeyState(VK_RIGHT) < 0) yaw -= 1;
			if (GetKeyState('A') < 0 || GetKeyState(VK_LEFT) < 0) yaw += 1;
			if (yaw)
				cv->Orbit(0, yaw * dtime * (float)g_cfgEditor.CameraSpeed * 0.0625f);
		}
		else	// flying
		{
			if (GetKeyState('D') < 0 || GetKeyState(VK_RIGHT) < 0) rt += 1;
			if (GetKeyState('A') < 0 || GetKeyState(VK_LEFT) < 0) rt -= 1;
			//if (GetKeyState(VK_SHIFT) < 0) move *= 4.0f;
		}
		if (GetKeyState('W') < 0 || GetKeyState(VK_UP) < 0) fwd += 1;
		if (GetKeyState('S') < 0 || GetKeyState(VK_DOWN) < 0) fwd -= 1;
		if (!fwd && !rt)
			return;
		cv->Move(fwd * dtime * (float)g_cfgEditor.CameraSpeed, rt * dtime * (float)g_cfgEditor.CameraSpeed, 0);
	}

	WndMain_UpdateWindows(W_SCENE);
}

/*
==============
WndCamera::MouseDown
==============
*/
void WndCamera::MouseDown(const int x, const int y, const int btndown, const int buttons)
{
	Sys_GetCursorPos(&cursorX, &cursorY);

	nCamButtonState = buttons;
	buttonX = x;
	buttonY = y;

	// look-at for starting the camera orbit
	if (btndown == MK_RBUTTON && buttons == (MK_RBUTTON | MK_SHIFT))
	{
		cv->SetOrbit(x, y);
		WndMain_UpdateWindows(W_CAMERA);
	}
}

/*
==============
WndCamera::MouseUp
==============
*/
void WndCamera::MouseUp(const int x, const int y, const int btnup, const int buttons)
{
	//Drag_MouseUp();
	WndMain_UpdateWindows(W_SCENE);
	nCamButtonState = 0;
}

/*
==============
WndCamera::MouseMoved
==============
*/
void WndCamera::MouseMoved(const int x, const int y, const int buttons)
{
	std::string camstring;
	vec3		dir;
	trace_t		t;
	int			nx, ny;

	nCamButtonState = buttons;

	if (!buttons)
	{
		// calc ray direction
		cv->PointToRay(x, y, dir);
		t = Selection::TestRay(cv->GetOrigin(), dir, false);

		if (t.brush)
		{
			vec3	size;
			size = t.brush->maxs - t.brush->mins;
			if (t.brush->owner->IsPoint())
				camstring = (_S("%s (%0v)") << t.brush->owner->eclass->name << size).Emit();
			else
				camstring = (_S("%s (%0v) %s") << t.brush->owner->eclass->name << size << t.face->texdef.name).Emit();
		}
		else
			camstring = "";

		WndMain_Status(camstring.data(), 0);

		return;
	}

	buttonX = x;
	buttonY = y;

	Sys_GetCursorPos(&nx, &ny);

	if (buttons & MK_RBUTTON)
	{
		int dX, dY;
		dX = nx - cursorX;
		dY = ny - cursorY;
		SetCursor(NULL); 

		// SetCursorPos below in a response to WM_MOUSEMOVE is bad because it generates WM_MOUSEMOVE,
		// so return early if the comparison is 0 to throw away the second WM_MOUSEMOVE
		if (!dX && !dY)
			return;

		if (buttons == (MK_RBUTTON | MK_CONTROL))
		{
			cv->Strafe(dX, dY);
		}
		else if (buttons == (MK_RBUTTON | MK_SHIFT))
		{
			cv->Orbit(dX,dY);
		}
		else if (buttons == (MK_RBUTTON | MK_CONTROL | MK_SHIFT) ||
			(buttons == MK_RBUTTON && g_cfgEditor.CameraMoveStyle == CAMERA_WASD))
		{
			cv->FreeLook(dX,dY);
		}
		Sys_SetCursorPos(cursorX, cursorY);	// doing this in a response to WM_MOUSEMOVE is bad because it generates WM_MOUSEMOVE...
		WndMain_UpdateWindows(W_SCENE);
	}

	/*
	if (buttons & MK_LBUTTON)
	{
		if (GetKeyState(VK_MENU) < 0)
		{
			Sys_GetCursorPos(&x, &y);
			cursorX = x;
			cursorY = y;
		}
		else
		{
			Sys_GetCursorPos(&cursorX, &cursorY);
			WndMain_UpdateWindows(W_SCENE);
		}
	}
	*/
}

/*
============
WndCamera::MouseWheel
============
*/
void WndCamera::MouseWheel(const int x, const int y, bool up, const int buttons)
{
	if (buttons & MK_CONTROL)
	{
		cv->ChangeFloor(up);
	}
	else
	{	// cameraSpeed is huge because it's scaled for driving around
		float fwd = (float)g_cfgEditor.CameraSpeed * 0.0625f;
		fwd *= up ? 1 : -1;
		if (buttons == (MK_RBUTTON | MK_SHIFT) || g_cfgEditor.CameraMoveStyle == CAMERA_WASD)
		{	// orbiting
			cv->Move(fwd, 0, 0);
		}
		else
		{	// classic horizontal-only movement
			if (buttons & MK_SHIFT)
				fwd *= 4.0f;
			cv->Step(fwd, 0, 0);
		}
	}
}

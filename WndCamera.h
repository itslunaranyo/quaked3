//==============================
//	WndCamera.h
//==============================

#ifndef __WND_CAMERA_H__
#define __WND_CAMERA_H__

#include "WindowGL.h"

extern HWND g_hwndCamera;
class CameraView;
class CameraRenderer;

class WndCamera : public WindowGL
{
public:
	WndCamera();
	~WndCamera();

	CameraView *cv;

	void Initialize();
	int OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

	void	MouseDown(const int x, const int y, const int btndown, const int buttons);
	void	MouseUp(const int x, const int y, const int btnup, const int buttons);
	void	MouseMoved(const int x, const int y, const int buttons);
	void	MouseWheel(const int x, const int y, bool up, const int buttons);
	void	RealtimeControl(float dtime);
private:
	CameraRenderer *cr;
	void Render();
	int OnResized();
	int nCamButtonState;
	int buttonX, buttonY;
};

#endif
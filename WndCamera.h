//==============================
//	WndCamera.h
//==============================

#ifndef __WND_CAMERA_H__
#define __WND_CAMERA_H__

#include "WndView.h"

extern HWND g_hwndCamera;
class CameraView;

class WndCamera : public WndView
{
public:
	WndCamera();
	~WndCamera();

	CameraView *cv;

	void Initialize();
	int OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#endif
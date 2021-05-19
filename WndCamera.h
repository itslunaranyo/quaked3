//==============================
//	WndCamera.h
//==============================

#ifndef __WND_CAMERA_H__
#define __WND_CAMERA_H__

#include "WndView.h"

class WndCamera : public WndView
{
public:
	WndCamera();
	~WndCamera();

	void Initialize();
	int OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#endif
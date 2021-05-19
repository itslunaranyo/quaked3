//==============================
//	WndZChecker.h
//==============================

#ifndef __WND_ZCHECKER_H__
#define __WND_ZCHECKER_H__

class WndZChecker : public WndView
{
public:
	WndZChecker();
	~WndZChecker();

	ZView *zv;

	void Initialize();
	int OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#endif
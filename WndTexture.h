//==============================
//	WndTexture.h
//==============================

#ifndef __WND_TEXTURE_H__
#define __WND_TEXTURE_H__

class WndTexture : public WndView
{
public:
	WndTexture();
	~WndTexture();

	TextureView *texv;

	void Initialize();
	void DoPopupMenu(int x, int y);
	int OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#endif
